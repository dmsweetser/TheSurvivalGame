// THE SURVIVAL GAME — mall/srcterm.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
//
// THE SOURCE TERMINAL (v1.26 Item 1c + Item 3) — station 9, inside the
// Source Room. Five views over the game's own embedded source
// (game/src_blob.c): a file BROWSER, a full-screen EDITOR (the game's
// bitmap font; v1.27: keyboard OR touch can edit — the pad is
// view-only), an unsaved-changes / reset PROMPT, a BUILD view (host
// compile via platform/srcbuild.c; on the web it explains itself),
// and the LIVE TUNING list (Item 3) for files that carry registry
// entries. v1.27: long lines SOFT-WRAP in the source/editor views —
// wrapping is display-only; the file bytes and the edit model never
// change, and every scroll offset below counts DISPLAY rows.
// Persistence is the overlay model in game/srcedit.c (plat_mod_*);
// RESET STOCK clears edits, tuning overrides and the custom page.

#define SRV_BROWSE 1
#define SRV_EDIT 2
#define SRV_PROMPT 3
#define SRV_BUILD 4
#define SRV_TUNE 5
#define SRCT_CAP 524288              // the editor's per-file cap (512 KB)
#define SRCT_MAXROWS 240
#define SRCT_ROW_BUILD (-2)
#define SRCT_ROW_RESET (-3)
#define SRCT_ROW_CLOSE (-4)

static char src_edit_buf[2][SRCT_CAP];
static char src_subst_buf[SRCT_CAP];
static int srct_view[2];
static int srct_file[2];             // blob index in the editor/tune, -1 none
static int srct_rows[2][SRCT_MAXROWS];
static char srct_rowhdr[2][SRCT_MAXROWS][24];
static int srct_nrows[2];
static int srct_bsel[2], srct_bscroll[2];
static char srct_note[2][128];
static int srct_elen[2], srct_cur[2], srct_dirty[2];
static int srct_top[2];              // v1.27: a DISPLAY-row offset (soft wrap)
// touch drag-scroll state (v1.27): last finger y + sub-row remainder
static int srct_drag_on[2], srct_drag_y[2], srct_drag_acc[2];
static int srct_repkey[2], srct_rep_t[2];
static int srct_prompt[2], srct_psel[2];        // prompt: 0 unsaved, 1 reset confirm
static int srct_tsel[2], srct_tscroll[2], srct_tn[2];
static int srct_tidx[2][64];
static char srct_pend[2][64][20];
static int srct_build_scroll[2];
// Esc/Interact the terminal consumed to change views this frame: the
// frame's close check must not ALSO fire on it (one Esc = one step).
static int srct_swallow[2];

static void srct_note_set(int s, const char *t){ scpy(srct_note[s], t, 128); }
// v1.27: touch reaches parity — a keyboard OR a touch device may
// modify source (Daniel, Nov 22). The pad stays view/scroll-only.
static int srct_can_edit(int pi){
    return pi==G.my_id && (dev_local()==DEV_KB || mobile_ui());
}
// the panel geometry, one derivation for input and drawing alike
static int srct_panel_w(void){ return G.ui_compact? 308 : STORE_W; }
static int srct_panel_x(void){ int w = srct_panel_w(); return G.ui_compact? G.view_x0 + (G.view_w - w)/2 : STORE_X; }
static int srct_panel_cols(void){ return (srct_panel_w()-20)/8; }
// editor text rows on screen: the touch footer (buttons + a status
// row) eats the bottom of the panel on mobile, so fewer rows fit
static int srct_editor_lines(void){ return mobile_ui()? (STORE_H-40-52)/16 : (STORE_H-40-24)/16; }

// where the frame may close the panel outright: only the browser
// view. Inner views own Esc/Interact (the editor's Esc is "back",
// the prompt's whole job is deciding) — mall/frame.c asks here.
static int srcterm_blocks_close(int pi){
    int s = whale_slot_of(pi);
    return srct_view[s]!=SRV_BROWSE || srct_swallow[s];
}
static void srcterm_on_close(int pi){ srct_view[whale_slot_of(pi)]=0; }
static int srct_open_for(int pi){
    if (pi==G.my_id) return (G.state==GS_STORE && G.store_station==9) || (lstore[0] && lstation[0]==9);
    if (G.p2_slot>=0 && pi==G.p2_slot) return lstore[1] && lstation[1]==9;
    return 0;
}
static int srcterm_view_for(int pi){ return srct_open_for(pi)? srct_view[whale_slot_of(pi)] : 0; }
static int srcterm_file_idx(int pi){ return srct_file[whale_slot_of(pi)]; }
// the soft-keyboard bridge (wasm exp_text_mode): 5 while the local
// terminal sits in EDIT or TUNE on a touch device. The mobile gate
// is essential — on a desktop the shell's hidden field must never
// hold focus, or its keydown path would swallow the editor's
// Ctrl+S / Ctrl+T chords as printable text.
static int srcterm_text_wanted(void){
    if (!mobile_ui()) return 0;
    if (!srct_open_for(G.my_id)) return 0;
    int s = whale_slot_of(G.my_id);
    return (srct_view[s]==SRV_EDIT || srct_view[s]==SRV_TUNE)? 5 : 0;
}

// --- browser rows ------------------------------------------------------
static void srct_build_rows(int s){
    int n=0;
    char last_folder[64]; last_folder[0]=0;
    for (int i=0;i<src_count() && n<SRCT_MAXROWS-3;i++){
        const char *p = src_path(i);
        char folder[64]; scpy(folder, p, 64);
        { char *sl=0; for (char *c=folder; *c; c++) if (*c=='/') sl=c;
          if (sl) *sl=0; else folder[0]=0; }
        if (scmp(folder, last_folder)){
            scpy(last_folder, folder, 64);
            srct_rows[s][n]=-1;
            scpy(srct_rowhdr[s][n], folder[0]? folder : "src", 24);
            n++;
        }
        srct_rows[s][n++]=i;
    }
    srct_rows[s][n++]=SRCT_ROW_BUILD;
    srct_rows[s][n++]=SRCT_ROW_RESET;
    srct_rows[s][n++]=SRCT_ROW_CLOSE;
    srct_nrows[s]=n;
    srct_bsel[s]=0; srct_bscroll[s]=0;
    // land on the first selectable row
    while (srct_bsel[s]<n && srct_rows[s][srct_bsel[s]]==-1) srct_bsel[s]++;
}
static void srct_open_browser(int s){
    srct_build_rows(s);
    srct_view[s]=SRV_BROWSE;
    srct_note[s][0]=0;
}

// --- editor ------------------------------------------------------------
static int srct_stock_len(int idx){ int n=0; src_text(idx, &n); return n; }
static int srct_edit_limit(int idx){
    int lim = srct_stock_len(idx)*3;
    if (lim>SRCT_CAP-1) lim = SRCT_CAP-1;
    if (lim<4096) lim = 4096;               // tiny files still get room to grow
    return lim;
}
static void srct_load_editor(int s, int idx){
    srct_file[s]=idx;
    int n = src_file_current(idx, src_edit_buf[s], SRCT_CAP);
    if (n>=SRCT_CAP) n = SRCT_CAP-1;
    srct_elen[s]=n; src_edit_buf[s][n]=0;
    srct_cur[s]=0; srct_dirty[s]=0; srct_top[s]=0;
    srct_drag_on[s]=0; srct_drag_acc[s]=0;
    srct_repkey[s]=-1; srct_rep_t[s]=0;
}
static void srct_save_editor(int s){
    if (srct_file[s]<0) return;
    plat_mod_save(src_path(srct_file[s]), src_edit_buf[s], srct_elen[s]);
    srct_dirty[s]=0;
    srct_note_set(s, "SAVED");
    sfx(11);
}
// cursor line/col (0-based) from the byte offset, plus the line's start
static void srct_cursor_lc(int s, int *line, int *col, int *lstart){
    int l=0, ls=0;
    for (int i=0;i<srct_cur[s] && i<srct_elen[s];i++)
        if (src_edit_buf[s][i]=='\n'){ l++; ls=i+1; }
    *line=l; *col=srct_cur[s]-ls; *lstart=ls;
}
static int srct_line_start(int s, int line){
    int l=0, i=0;
    while (l<line && i<srct_elen[s]){ if (src_edit_buf[s][i]=='\n') l++; i++; }
    return i;
}
static int srct_line_len(int s, int ls){
    int n=0; while (ls+n<srct_elen[s] && src_edit_buf[s][ls+n]!='\n') n++;
    return n;
}
// --- soft wrap (v1.27, display-only) -----------------------------------
// A logical line of llen chars at `cols` columns takes this many
// display rows. The cursor's display row is the sum of the segments
// of every line above it plus its own segment; a cursor sitting at
// the end of a line whose length is an exact multiple of cols stays
// on that line's LAST segment (offset cols), never on a phantom row.
static int srct_line_segs(int llen, int cols){
    if (cols<1) cols=1;
    return llen<=0? 1 : (llen+cols-1)/cols;
}
static int srct_total_lines(int s){
    int n=1; for (int i=0;i<srct_elen[s];i++) if (src_edit_buf[s][i]=='\n') n++;
    return n;
}
static int srct_total_drows(int s, int cols){
    // one pass over the buffer (this runs per frame — no per-line
    // rescans from byte 0)
    int total=0, llen=0;
    for (int i=0;i<srct_elen[s];i++){
        if (src_edit_buf[s][i]=='\n'){ total += srct_line_segs(llen, cols); llen=0; }
        else llen++;
    }
    return total + srct_line_segs(llen, cols);
}
// the cursor's segment index within its line + its column offset
// inside that segment (the end-of-line adjustment lives here)
static void srct_cursor_seg(int s, int cols, int *seg, int *off){
    int line, col, ls; srct_cursor_lc(s, &line, &col, &ls);
    int llen = srct_line_len(s, ls);
    if (col==llen && llen>0 && (llen%cols)==0){ *seg = llen/cols - 1; *off = cols; }
    else { *seg = col/cols; *off = col - (*seg)*cols; }
    (void)line;
}
static void srct_cursor_display(int s, int cols, int *drow, int *doff){
    int line, col, ls; srct_cursor_lc(s, &line, &col, &ls);
    int d=0, llen=0;
    for (int i=0;i<ls;i++){
        if (src_edit_buf[s][i]=='\n'){ d += srct_line_segs(llen, cols); llen=0; }
        else llen++;
    }
    int seg, off; srct_cursor_seg(s, cols, &seg, &off);
    *drow = d + seg; *doff = off;
    (void)line; (void)col;
}
// a tap on display row drow / display column dcol -> the byte offset
// it points at (clamped to that line's end); one pass over the text
static int srct_display_to_offset(int s, int cols, int drow, int dcol){
    int acc=0, i=0;
    for (;;){
        int ls=i, llen=0;
        while (i<srct_elen[s] && src_edit_buf[s][i]!='\n'){ i++; llen++; }
        int segs = srct_line_segs(llen, cols);
        if (drow < acc+segs){
            int c = (drow-acc)*cols + dcol;
            if (c>llen) c=llen;
            return ls + c;
        }
        acc += segs;
        if (i>=srct_elen[s]) break;
        i++;   // the '\n'
    }
    return srct_elen[s];
}
// keep the cursor's display row inside the visible window
static void srct_ensure_cursor(int s){
    int cols = srct_panel_cols(), lines = srct_editor_lines();
    int drow, doff; srct_cursor_display(s, cols, &drow, &doff);
    if (drow < srct_top[s]) srct_top[s] = drow;
    if (drow > srct_top[s]+lines-1) srct_top[s] = drow-lines+1;
    int total = srct_total_drows(s, cols);
    if (srct_top[s] > total-lines) srct_top[s] = total-lines;
    if (srct_top[s]<0) srct_top[s]=0;
    (void)doff;
}
// --- touch drag scroll (v1.27) -------------------------------------------
// IN.touch_down carries the live finger (the drag scheme's mirror of
// tp[]); stick-scheme touches in the store never set it, so fall back
// to the first live zone-0 touch point. Returns whole rows scrolled
// this frame: positive = the view moves DOWN (finger swept up).
static int srct_drag_pos(int *x, int *y){
    if (IN.touch_down){ *x=IN.touch_x; *y=IN.touch_y; return 1; }
    for (int i=0;i<6;i++) if (IN.tp[i].active && IN.tp[i].zone==0){ *x=IN.tp[i].x; *y=IN.tp[i].y; return 1; }
    return 0;
}
static int srct_drag_rows(int s, int row_h){
    int x, y;
    if (!srct_drag_pos(&x, &y)){ srct_drag_on[s]=0; srct_drag_acc[s]=0; return 0; }
    if (!srct_drag_on[s]){ srct_drag_on[s]=1; srct_drag_y[s]=y; srct_drag_acc[s]=0; return 0; }
    srct_drag_acc[s] += y - srct_drag_y[s];
    srct_drag_y[s] = y;
    int rows=0;
    while (srct_drag_acc[s]<=-row_h){ rows++; srct_drag_acc[s]+=row_h; }
    while (srct_drag_acc[s]>= row_h){ rows--; srct_drag_acc[s]-=row_h; }
    return rows;
}
static void srct_insert(int s, char c){
    if (srct_elen[s]+1 > srct_edit_limit(srct_file[s])){
        srct_note_set(s, "FILE SIZE CAP REACHED - CANNOT GROW THIS FILE FURTHER");
        sfx(16); return;
    }
    for (int i=srct_elen[s]; i>srct_cur[s]; i--) src_edit_buf[s][i]=src_edit_buf[s][i-1];
    src_edit_buf[s][srct_cur[s]++]=c;
    srct_elen[s]++; src_edit_buf[s][srct_elen[s]]=0;
    srct_dirty[s]=1;
}
static void srct_backspace(int s){
    if (srct_cur[s]<=0) return;
    for (int i=srct_cur[s]-1;i<srct_elen[s];i++) src_edit_buf[s][i]=src_edit_buf[s][i+1];
    srct_cur[s]--; srct_elen[s]--; src_edit_buf[s][srct_elen[s]]=0;
    srct_dirty[s]=1;
}
static void srct_delete(int s){
    if (srct_cur[s]>=srct_elen[s]) return;
    for (int i=srct_cur[s];i<srct_elen[s];i++) src_edit_buf[s][i]=src_edit_buf[s][i+1];
    srct_elen[s]--; src_edit_buf[s][srct_elen[s]]=0;
    srct_dirty[s]=1;
}
// v1.27: vertical movement steps DISPLAY rows — one step crosses to
// the next wrapped segment of the same line before it ever leaves
// the line, keeping the segment column offset where it can.
static void srct_move_display(int s, int drows){
    int cols = srct_panel_cols();
    int step = drows<0? -1 : 1, n = drows<0? -drows : drows;
    for (int k=0;k<n;k++){
        int line, col, ls; srct_cursor_lc(s, &line, &col, &ls);
        int llen = srct_line_len(s, ls);
        int seg, off; srct_cursor_seg(s, cols, &seg, &off);
        if (step>0){
            if (seg+1 < srct_line_segs(llen, cols)){
                int nc = (seg+1)*cols + off; if (nc>llen) nc=llen;
                srct_cur[s] = ls + nc;
            } else {
                int nls = ls + llen + 1;
                if (nls>srct_elen[s]) break;
                int nl = srct_line_len(s, nls);
                srct_cur[s] = nls + (off<nl? off : nl);
            }
        } else {
            if (seg>0){
                int nc = (seg-1)*cols + off; if (nc>llen) nc=llen;
                srct_cur[s] = ls + nc;
            } else {
                if (line<=0) break;
                int pls = srct_line_start(s, line-1), pl = srct_line_len(s, pls);
                int nc = (srct_line_segs(pl, cols)-1)*cols + off; if (nc>pl) nc=pl;
                srct_cur[s] = pls + nc;
            }
        }
        (void)col;
    }
}
static void srct_move(int s, int pk){
    int line, col, ls; srct_cursor_lc(s, &line, &col, &ls);
    int ll = srct_line_len(s, ls);
    if (pk==PK_LEFT){ if (srct_cur[s]>0) srct_cur[s]--; }
    else if (pk==PK_RIGHT){ if (srct_cur[s]<srct_elen[s]) srct_cur[s]++; }
    else if (pk==PK_HOME){ srct_cur[s]=ls; }
    else if (pk==PK_END){ srct_cur[s]=ls+ll; }
    else if (pk==PK_UP) srct_move_display(s, -1);
    else if (pk==PK_DOWN) srct_move_display(s, 1);
    else if (pk==PK_PGUP) srct_move_display(s, -10);
    else if (pk==PK_PGDN) srct_move_display(s, 10);
    (void)line; (void)col;
}
// one editor key step (edge or auto-repeat) for movement/delete keys
static void srct_editor_key(int s, int pk){
    if (pk==PK_BACKSPACE) srct_backspace(s);
    else if (pk==PK_DELETE) srct_delete(s);
    else srct_move(s, pk);
}
static int srct_is_nav(int pk){
    return pk==PK_LEFT || pk==PK_RIGHT || pk==PK_UP || pk==PK_DOWN
        || pk==PK_BACKSPACE || pk==PK_DELETE || pk==PK_HOME || pk==PK_END
        || pk==PK_PGUP || pk==PK_PGDN;
}
static int srct_memeq(const char *a, const char *b, int n){
    for (int i=0;i<n;i++) if (a[i]!=b[i]) return 0;
    return 1;
}

// --- LIVE TUNING substitution ------------------------------------------
// find the elem-th numeric literal after `anchor` in txt (skipping //
// comments); returns its span via s_out/e_out, or -1.
static int srct_literal_span(const char *txt, int len, const char *anchor, int elem, int *s_out, int *e_out){
    int alen = slen(anchor), pos=-1;
    for (int i=0;i+alen<=len;i++) if (srct_memeq(txt+i, anchor, alen)){ pos=i+alen; break; }
    if (pos<0) return -1;
    int count=0, i=pos;
    while (i<len){
        if (txt[i]=='/' && i+1<len && txt[i+1]=='/'){ while (i<len && txt[i]!='\n') i++; continue; }
        int isnum = (txt[i]>='0' && txt[i]<='9');
        int isneg = (txt[i]=='-' && i+1<len && txt[i+1]>='0' && txt[i+1]<='9'
                     && (i==pos || txt[i-1]==' ' || txt[i-1]=='{' || txt[i-1]==',' || txt[i-1]=='\t' || txt[i-1]=='='));
        if (isnum || isneg){
            int st=i; if (isneg) i++;
            while (i<len && ((txt[i]>='0' && txt[i]<='9') || txt[i]=='.')) i++;
            if (i<len && txt[i]=='f') i++;
            if (count==elem){ *s_out=st; *e_out=i; return 0; }
            count++;
            continue;
        }
        i++;
    }
    return -1;
}
// substitute entry e's literal in its file's SAVED overlay text and
// persist the result. 1 ok, 0 could-not (anchor gone / line rewritten).
static int srct_substitute(const TuneEntry *e, float newv){
    int idx = src_find(e->file);
    if (idx<0) return 0;
    int len=0; const char *txt = src_file_current_scratch(idx, &len);
    int ls=0, le=0;
    if (srct_literal_span(txt, len, e->anchor, e->elem, &ls, &le)) return 0;
    char lit[32]; tune_fmt_lit(e, newv, lit);
    int ll = slen(lit), nl = len - (le-ls) + ll;
    if (nl>=SRCT_CAP) return 0;
    smemcpy(src_subst_buf, txt, ls);
    smemcpy(src_subst_buf+ls, lit, ll);
    smemcpy(src_subst_buf+ls+ll, txt+le, len-le);
    src_subst_buf[nl]=0;
    plat_mod_save(e->file, src_subst_buf, nl);
    return 1;
}

// --- actions -------------------------------------------------------------
static void srct_build_kick(int s){
    srct_view[s]=SRV_BUILD; srct_build_scroll[s]=0; srct_note[s][0]=0;
    if (!plat_srcbuild_available()) return;   // the view explains itself
    plat_srcbuild_begin();
    for (int i=0;i<src_count();i++){
        int len=0; const char *t = src_file_current_scratch(i, &len);
        plat_srcbuild_file(src_path(i), t, len);
    }
    plat_srcbuild_run_async();
}
static void srct_reset_stock(int s){
    plat_mod_clear();
    livedata_reset();
    src_differs_clear();
    plat_srcbuild_clear();
    if (srct_file[s]>=0) srct_load_editor(s, srct_file[s]);
    srct_note_set(s, "STOCK RESTORED - ALL EDITS AND TUNING CLEARED");
    sfx(11);
}
static void srct_tune_open(int s){
    srct_tn[s] = livedata_file_entries(src_path(srct_file[s]), srct_tidx[s], 64);
    for (int i=0;i<srct_tn[s];i++) srct_pend[s][i][0]=0;
    srct_tsel[s]=0; srct_tscroll[s]=0;
    srct_view[s]=SRV_TUNE;
    if (srct_dirty[s]) srct_note_set(s, "UNSAVED EDITS: TUNING APPLIES TO THE SAVED TEXT");
    else srct_note[s][0]=0;
}
static void srct_tune_apply(int s){
    // validate EVERY pending value first — one bad value rejects all
    for (int r=0;r<srct_tn[s];r++){
        if (!srct_pend[s][r][0]) continue;
        const TuneEntry *e = livedata_entry(srct_tidx[s][r]);
        float v; char err[64];
        if (!tune_parse_num(srct_pend[s][r], &v)){
            char b[128]; char *q=b; q=sappend(q, "REJECTED: "); q=sappend(q, e->name); q=sappend(q, " IS NOT A NUMBER"); *q=0;
            srct_note_set(s, b); sfx(16); return;
        }
        if (livedata_validate(srct_tidx[s][r], v, err, (int)sizeof(err))){
            char b[160]; char *q=b; q=sappend(q, "REJECTED: "); q=sappend(q, e->name); q=sappend(q, " "); q=sappend(q, err); *q=0;
            srct_note_set(s, b); sfx(16); return;
        }
    }
    int applied=0, subfail=0;
    for (int r=0;r<srct_tn[s];r++){
        if (!srct_pend[s][r][0]) continue;
        const TuneEntry *e = livedata_entry(srct_tidx[s][r]);
        float v=0; tune_parse_num(srct_pend[s][r], &v);
        livedata_apply(srct_tidx[s][r], v, 0, 0);
        if (!srct_substitute(e, v)) subfail=1;
        applied++;
    }
    livedata_persist_now();
    if (subfail) src_file_set_differs(src_path(srct_file[s]));
    if (!srct_dirty[s] && srct_file[s]>=0) srct_load_editor(s, srct_file[s]);
    { char b[128]; char *q=b; q=sappend(q, "LIVE TUNING APPLIED ("); q=sapp_int(q, applied); q=sappend(q, ")");
      if (subfail) q=sappend(q, " - ONE LINE DIFFERS FROM TEXT");
      if (srct_dirty[s]) q=sappend(q, " - UNSAVED EDITS UNTOUCHED");
      *q=0; srct_note_set(s, b); }
    sfx(11);
    srct_view[s]=SRV_EDIT;
}

// --- test seams (observe only, never saved) ------------------------------
static int srcterm_dbg_drows(void){
    int s = whale_slot_of(G.my_id);
    return srct_view[s]==SRV_EDIT? srct_total_drows(s, srct_panel_cols()) : -1;
}
static int srcterm_dbg_drow(void){
    int s = whale_slot_of(G.my_id);
    if (srct_view[s]!=SRV_EDIT) return -1;
    int d, o; srct_cursor_display(s, srct_panel_cols(), &d, &o);
    return d;
}
static int srcterm_dbg_top(void){ return srct_top[whale_slot_of(G.my_id)]; }
static int srcterm_dbg_cursor(void){
    int s = whale_slot_of(G.my_id);
    int l, c, ls; srct_cursor_lc(s, &l, &c, &ls);
    return l*100000 + c;
}
static int srcterm_dbg_browse(void){
    int s = whale_slot_of(G.my_id);
    return srct_bsel[s]*10000 + srct_bscroll[s];
}
static int srcterm_dbg_canedit(int pi){ return srct_can_edit(pi); }

// --- input ---------------------------------------------------------------
static void srct_browser_input(int pi, int s, int up, int down, int activate){
    int n = srct_nrows[s];
    if (up){ do { srct_bsel[s] = (srct_bsel[s]+n-1)%n; } while (srct_rows[s][srct_bsel[s]]==-1); sfx(6); }
    if (down){ do { srct_bsel[s] = (srct_bsel[s]+1)%n; } while (srct_rows[s][srct_bsel[s]]==-1); sfx(6); }
    if (pi==G.my_id && raw_edge(PK_PGUP)){ for (int k=0;k<10;k++){ int p=(srct_bsel[s]+n-1)%n; if (srct_rows[s][p]!=-1) srct_bsel[s]=p; } }
    if (pi==G.my_id && raw_edge(PK_PGDN)){ for (int k=0;k<10;k++){ int p=(srct_bsel[s]+1)%n; if (srct_rows[s][p]!=-1) srct_bsel[s]=p; } }
    // v1.27: swipe/drag scrolls the list (17px rows); the selection
    // rides along so it never leaves the window the draw will show
    if (pi==G.my_id && mobile_ui()){
        int drows = srct_drag_rows(s, 17);
        if (drows){
            int vis = (STORE_H-40-22)/17;
            srct_bscroll[s] += drows;
            if (srct_bscroll[s] > n-vis) srct_bscroll[s] = n-vis;
            if (srct_bscroll[s]<0) srct_bscroll[s]=0;
            if (srct_bsel[s] < srct_bscroll[s]){
                int r = srct_bscroll[s]; while (r<n && srct_rows[s][r]==-1) r++;
                if (r<n) srct_bsel[s]=r;
            }
            if (srct_bsel[s] > srct_bscroll[s]+vis-1){
                int r = srct_bscroll[s]+vis-1; while (r>=0 && srct_rows[s][r]==-1) r--;
                if (r>=0) srct_bsel[s]=r;
            }
        }
    }
    int row = srct_rows[s][srct_bsel[s]];
    if (pi==G.my_id && raw_edge('B'-'A')){ srct_build_kick(s); return; }
    if (pi==G.my_id && raw_edge('R'-'A')){ srct_prompt[s]=1; srct_psel[s]=1; srct_view[s]=SRV_PROMPT; return; }
    if (pi==G.my_id && IN.tapped){
        int w = G.ui_compact? 308 : STORE_W;
        int x = G.ui_compact? G.view_x0 + (G.view_w - w)/2 : STORE_X;
        if (IN.tap_x<x-6 || IN.tap_x>x+w+6 || IN.tap_y<STORE_Y-6 || IN.tap_y>STORE_Y+STORE_H+6){ G.tap_close_store=1; return; }
        int vis = (STORE_H-40-22)/17;
        for (int vi=0; vi<vis; vi++){
            int ri = srct_bscroll[s]+vi;
            if (ri>=n) break;
            int ry = STORE_Y+40 + vi*17;
            if (IN.tap_y>=ry-2 && IN.tap_y<=ry+15 && srct_rows[s][ri]!=-1){
                if (ri==srct_bsel[s]) activate=1; else { srct_bsel[s]=ri; sfx(6); }
                break;
            }
        }
        row = srct_rows[s][srct_bsel[s]];
    }
    if (!activate) return;
    if (row>=0){ srct_load_editor(s, row); srct_view[s]=SRV_EDIT; srct_note[s][0]=0; sfx(5); }
    else if (row==SRCT_ROW_BUILD){ srct_build_kick(s); }
    else if (row==SRCT_ROW_RESET){ srct_prompt[s]=1; srct_psel[s]=1; srct_view[s]=SRV_PROMPT; }
    else if (row==SRCT_ROW_CLOSE){ if (pi==G.my_id) G.tap_close_store=1; }
}
// the editor's way back — Esc, and the touch footer's BACK zone,
// are the exact same step (dirty files ask first)
static void srct_editor_back(int s){
    srct_swallow[s]=1;
    if (srct_dirty[s]){ srct_prompt[s]=0; srct_psel[s]=0; srct_view[s]=SRV_PROMPT; }
    else srct_view[s]=SRV_BROWSE;
}
static void srct_editor_input(int pi, int s, int up, int down, int activate){
    (void)up; (void)down; (void)activate;
    if (IN.esc_pressed || raw_edge(PK_ESC)){ srct_editor_back(s); return; }
    int cur0 = srct_cur[s];
    // v1.27 touch: drag scrolls by DISPLAY rows (16px), a tap on the
    // text positions the cursor through the wrap mapping, and the
    // footer zones [ SAVE ] [ NEW LINE ] [ TUNE ] [ BACK ] run the
    // exact Ctrl+S / Enter / Ctrl+T / Esc actions — so saving never
    // depends on soft-keyboard quirks. A drag is not a tap (tapped
    // only fires for quick, still touches), so the two never fight.
    if (pi==G.my_id && mobile_ui()){
        int drows = srct_drag_rows(s, 16);
        if (drows){ srct_top[s]+=drows; if (srct_top[s]<0) srct_top[s]=0; }
        if (IN.tapped){
            int x = srct_panel_x(), y = STORE_Y, w = srct_panel_w(), h = STORE_H;
            int fy = y+h-26;
            if (IN.tap_y>=fy && IN.tap_y<y+h && IN.tap_x>=x && IN.tap_x<x+w){
                int zone = (IN.tap_x-x)*4/w; if (zone>3) zone=3;
                if (zone==0){ if (srct_can_edit(pi)) srct_save_editor(s); }
                else if (zone==1){ if (srct_can_edit(pi)){ srct_insert(s, '\n'); srct_ensure_cursor(s); } }
                else if (zone==2){
                    if (srct_file[s]>=0 && livedata_file_has(src_path(srct_file[s]))) srct_tune_open(s);
                    else srct_note_set(s, "NO LIVE TUNABLES IN THIS FILE");
                }
                else srct_editor_back(s);
                return;
            }
            if (srct_can_edit(pi) && IN.tap_x>=x+10 && IN.tap_x<x+w-6 && IN.tap_y>=y+40 && IN.tap_y<y+40+srct_editor_lines()*16){
                int cols = srct_panel_cols();
                int drow = srct_top[s] + (IN.tap_y-(y+40))/16;
                int dcol = (IN.tap_x-(x+10))/8; if (dcol<0) dcol=0; if (dcol>cols) dcol=cols;
                srct_cur[s] = srct_display_to_offset(s, cols, drow, dcol);
                srct_repkey[s]=-1;
            }
        }
    }
    if (!srct_can_edit(pi)){
        // view-only (the pad): menu up/down scroll a display row at a time
        if (up && srct_top[s]>0) srct_top[s]--;
        if (down) srct_top[s]++;
        return;
    }
    if (raw_dn(PK_CTRL) && raw_edge('S'-'A')){ srct_save_editor(s); return; }
    if (raw_dn(PK_CTRL) && raw_edge('T'-'A')){
        if (srct_file[s]>=0 && livedata_file_has(src_path(srct_file[s]))) srct_tune_open(s);
        else srct_note_set(s, "NO LIVE TUNABLES IN THIS FILE");
        return;
    }
    // navigation + deletion: edges, then hold-to-repeat (whale idiom)
    static const int navs[10] = { PK_LEFT, PK_RIGHT, PK_UP, PK_DOWN, PK_BACKSPACE, PK_DELETE, PK_HOME, PK_END, PK_PGUP, PK_PGDN };
    int held_any=0;
    for (int k=0;k<10;k++){
        int pk = navs[k];
        if (raw_edge(pk)){ srct_editor_key(s, pk); srct_repkey[s]=pk; srct_rep_t[s]=0; held_any=1; }
        else if (pk==srct_repkey[s] && raw_dn(pk)){
            held_any=1;
            srct_rep_t[s]++;
            if (srct_rep_t[s]>21 && (srct_rep_t[s]%3)==0) srct_editor_key(s, pk);
        }
    }
    if (!held_any && srct_repkey[s]>=0 && !raw_dn(srct_repkey[s])) srct_repkey[s]=-1;
    if (raw_edge(PK_TAB)){ srct_insert(s, ' '); srct_insert(s, ' '); }
    // the shells reserve Space as the fire/select key and never emit it
    // as a typed char — in the editor it is text, so take the raw edge
    if (raw_edge(PK_SPACE)) srct_insert(s, ' ');
    if (IN.enter_pressed) srct_insert(s, '\n');
    for (int i=0;i<IN.typed_n;i++){
        char c = IN.typed[i];
        // space comes from the PK_SPACE edge only on physical-keyboard
        // paths (native shells ALSO emit it as a char — one path, no
        // doubles). v1.27: on a touch session the soft-keyboard bridge
        // is the ONLY source of typed chars, so its space is text.
        if (c==' ' && !mobile_ui()) continue;
        if (c>=32 && c<127) srct_insert(s, c);
    }
    // a move or edit pulls the view back to the cursor; a frame in
    // which the cursor never moved (a drag-scroll, a save) leaves
    // the player's chosen scroll offset alone
    if (srct_cur[s]!=cur0) srct_ensure_cursor(s);
}
static void srct_prompt_input(int pi, int s, int up, int down, int activate){
    int rows = srct_prompt[s]==0? 3 : 2;
    if (IN.esc_pressed || raw_edge(PK_ESC)){
        srct_swallow[s]=1;
        srct_view[s] = srct_prompt[s]==0? SRV_EDIT : SRV_BROWSE;
        return;
    }
    if (up){ srct_psel[s] = (srct_psel[s]+rows-1)%rows; sfx(6); }
    if (down){ srct_psel[s] = (srct_psel[s]+1)%rows; sfx(6); }
    // v1.27: taps work the browser's idiom — tap selects, tapping the
    // selected row activates it (the box math mirrors the draw)
    if (pi==G.my_id && IN.tapped){
        int w = srct_panel_w(), x = srct_panel_x(), y = STORE_Y, h = STORE_H;
        int bw = (w-16<340)? w-16 : 340, bh = 110;
        int bx = x + (w-bw)/2, by = y + (h-bh)/2;
        int base = srct_prompt[s]==0? by+44 : by+58;
        for (int i=0;i<rows;i++){
            int ry = base + i*20;
            if (IN.tap_x>=bx+8 && IN.tap_x<bx+bw-8 && IN.tap_y>=ry-2 && IN.tap_y<=ry+17){
                if (i==srct_psel[s]) activate=1; else { srct_psel[s]=i; sfx(6); }
                break;
            }
        }
    }
    if (!activate) return;
    if (srct_prompt[s]==0){
        if (srct_psel[s]==0){ srct_save_editor(s); srct_view[s]=SRV_BROWSE; }
        else if (srct_psel[s]==1){ if (srct_file[s]>=0) srct_load_editor(s, srct_file[s]); srct_view[s]=SRV_BROWSE; srct_note_set(s, "EDITS DISCARDED"); }
        else srct_view[s]=SRV_EDIT;
    } else {
        if (srct_psel[s]==0){ srct_reset_stock(s); srct_build_rows(s); }
        srct_view[s]=SRV_BROWSE;
    }
    sfx(5);
}
static void srct_build_input(int pi, int s, int up, int down, int activate){
    (void)activate;
    if (IN.esc_pressed || raw_edge(PK_ESC)){ srct_swallow[s]=1; srct_view[s]=SRV_BROWSE; return; }
    // v1.30 (audit F3): a tap goes back too — the footer promises it for
    // touch, which otherwise had no way out of this view but the shell X.
    if (pi==G.my_id && IN.tapped){ srct_swallow[s]=1; srct_view[s]=SRV_BROWSE; return; }
    if (up && srct_build_scroll[s]>0) srct_build_scroll[s]--;
    if (down) srct_build_scroll[s]++;
    // v1.27: drag scrolls the (wrapped) log, 16px display rows
    if (pi==G.my_id && mobile_ui()){
        int drows = srct_drag_rows(s, 16);
        if (drows){ srct_build_scroll[s]+=drows; if (srct_build_scroll[s]<0) srct_build_scroll[s]=0; }
    }
}
static void srct_tune_input(int pi, int s, int up, int down, int activate){
    int rows = srct_tn[s]+2;
    if (IN.esc_pressed || raw_edge(PK_ESC)){ srct_swallow[s]=1; srct_view[s]=SRV_EDIT; return; }
    if (up){ srct_tsel[s] = (srct_tsel[s]+rows-1)%rows; sfx(6); }
    if (down){ srct_tsel[s] = (srct_tsel[s]+1)%rows; sfx(6); }
    // v1.27 touch: drag scrolls the list (17px rows, selection rides
    // along), taps select-then-activate like the browser's
    if (pi==G.my_id && mobile_ui()){
        int drows = srct_drag_rows(s, 17);
        if (drows){
            int vis = (STORE_H-40-38)/17;
            srct_tscroll[s] += drows;
            if (srct_tscroll[s] > rows-vis) srct_tscroll[s] = rows-vis;
            if (srct_tscroll[s]<0) srct_tscroll[s]=0;
            if (srct_tsel[s] < srct_tscroll[s]) srct_tsel[s] = srct_tscroll[s];
            if (srct_tsel[s] > srct_tscroll[s]+vis-1) srct_tsel[s] = srct_tscroll[s]+vis-1;
        }
        if (IN.tapped){
            int x = srct_panel_x(), y = STORE_Y, w = srct_panel_w(), h = STORE_H;
            int vis = (h-40-38)/17;
            if (IN.tap_x>=x && IN.tap_x<x+w){
                for (int vi=0; vi<vis; vi++){
                    int ri = srct_tscroll[s]+vi;
                    if (ri>=rows) break;
                    int ry = y+40 + vi*17;
                    if (IN.tap_y>=ry-2 && IN.tap_y<=ry+15){
                        if (ri==srct_tsel[s]) activate=1; else { srct_tsel[s]=ri; sfx(6); }
                        break;
                    }
                }
            }
        }
    }
    if (srct_can_edit(pi) && srct_tsel[s]<srct_tn[s]){
        char *pd = srct_pend[s][srct_tsel[s]];
        int pl = slen(pd);
        for (int i=0;i<IN.typed_n;i++){
            char c = IN.typed[i];
            if (((c>='0' && c<='9') || c=='.' || c=='-') && pl<19){ pd[pl++]=c; pd[pl]=0; }
        }
        if (IN.backspace && pl>0){ pd[pl-1]=0; }
    }
    if (!activate) return;
    if (srct_tsel[s]==srct_tn[s]) srct_tune_apply(s);
    else if (srct_tsel[s]==srct_tn[s]+1){ srct_view[s]=SRV_EDIT; }
}
static void srcterm_input(int pi, int up, int down, int activate){
    int s = whale_slot_of(pi);
    srct_swallow[s]=0;
    if (srct_view[s]==0) srct_open_browser(s);
    if (srct_view[s]==SRV_BROWSE) srct_browser_input(pi, s, up, down, activate);
    else if (srct_view[s]==SRV_EDIT) srct_editor_input(pi, s, up, down, activate);
    else if (srct_view[s]==SRV_PROMPT) srct_prompt_input(pi, s, up, down, activate);
    else if (srct_view[s]==SRV_BUILD) srct_build_input(pi, s, up, down, activate);
    else if (srct_view[s]==SRV_TUNE) srct_tune_input(pi, s, up, down, activate);
}

// --- drawing ---------------------------------------------------------------
static void srct_draw_note(int s, int x, int y, int w){
    // v1.27: notes run long ("FILE SIZE CAP REACHED - ...") — fit-draw
    // so nothing ever spills past the panel at compact width
    if (srct_note[s][0]) draw_text_center_fit(x+w/2, y, w-16, srct_note[s], 1, COL(255,214,130));
}
static void srct_draw_browser(int s, int x, int y, int w, int h){
    { char b[128]; char *q=b;
      q=sapp_int(q, src_count()); q=sappend(q, " FILES");
      int ec = src_edited_count();
      if (ec>0){ q=sappend(q, " - MODDED ("); q=sapp_int(q, ec); q=sappend(q, ")"); }
      int ov = livedata_overrides();
      if (ov>0){ q=sappend(q, " - LIVE TUNING: "); q=sapp_int(q, ov); q=sappend(q, " OVERRIDES"); }
      *q=0; draw_text_center_fit(x+w/2, y+20, w-16, b, 1, COL(190,240,205)); }
    int vis = (h-40-22)/17;
    if (srct_bsel[s] < srct_bscroll[s]) srct_bscroll[s] = srct_bsel[s];
    if (srct_bsel[s] > srct_bscroll[s]+vis-1) srct_bscroll[s] = srct_bsel[s]-vis+1;
    if (srct_bscroll[s] > srct_nrows[s]-vis) srct_bscroll[s] = srct_nrows[s]-vis;
    if (srct_bscroll[s] < 0) srct_bscroll[s] = 0;
    for (int vi=0; vi<vis; vi++){
        int ri = srct_bscroll[s]+vi;
        if (ri>=srct_nrows[s]) break;
        int ry = y+40 + vi*17;
        int row = srct_rows[s][ri];
        if (row==-1){
            draw_text(x+14, ry+2, srct_rowhdr[s][ri], 1, COL(120,180,140));
            rect_fill(x+14+text_w(srct_rowhdr[s][ri],1)+6, ry+8, w-40-text_w(srct_rowhdr[s][ri],1), 1, COL(50,80,60));
            continue;
        }
        if (ri==srct_bsel[s]){
            rect_fill(x+8, ry-1, w-16, 16, COL(26,54,36));
            rect_fill(x+8, ry-1, 3, 16, COL(120,255,170));
        }
        if (row>=0){
            const char *p = src_path(row);
            const char *base = p;
            for (const char *c=p; *c; c++) if (*c=='/') base=c+1;
            // v1.27: the name is fit-trimmed into the space the row's
            // tags leave behind, so a long name + LIVE + TEXT!=LIVE
            // can never run past the panel edge at compact width
            int reserve = 0;
            if (src_file_edited(row)) reserve += 12;
            if (src_file_differs(p)) reserve += 34 + 88;
            else if (livedata_file_has(p)) reserve += 34;
            char db[96]; int dn=0;
            while (base[dn] && dn<94){ db[dn]=base[dn]; dn++; }
            db[dn]=0;
            int maxbw = w - 36 - reserve;
            if (text_w(db,1) > maxbw){
                while (dn>0 && (dn+2)*8 > maxbw) db[--dn]=0;
                if (dn>0){ db[dn]='.'; db[dn+1]='.'; db[dn+2]=0; }
            }
            draw_text(x+22, ry+2, db, 1, COL(215,240,220));
            int ex = x+22 + text_w(db,1) + 6;
            if (src_file_edited(row)){ draw_text(ex, ry+2, "*", 1, COL(255,214,130)); ex += 12; }
            if (livedata_file_has(p)) draw_text(ex, ry+2, "LIVE", 1, COL(120,255,170));
            if (src_file_differs(p)) draw_text(ex+34, ry+2, "TEXT!=LIVE", 1, COL(255,150,120));
        } else {
            const char *lbl = row==SRCT_ROW_BUILD? "[ BUILD ]" : row==SRCT_ROW_RESET? "[ RESET STOCK ]" : "[ CLOSE ]";
            draw_text(x+22, ry+2, lbl, 1, COL(255,226,150));
        }
    }
    srct_draw_note(s, x, y+h-20, w);
    if (!srct_note[s][0])
        // v1.30 (audit F2/F14): the pad footer used to promise "B - BUILD" —
        // B is a physical-keyboard shortcut (raw_edge) and on a pad it
        // closes the panel; the pad's build path is the [ BUILD ] row.
        // The keyboard half now names the player's actual FIRE bind
        // instead of a hard-coded SPACE.
        { char fb[96]; char *q=fb;
          if (hint_dev()==DEV_PAD) q=sappend(q, "D-PAD + A - OPEN");
          else if (hint_dev()==DEV_TOUCH) q=sappend(q, "TAP - OPEN    DRAG - SCROLL");
          else { q=sappend(q, "ARROWS + "); q=sappend(q, key_name(G.bind[ACT_FIRE])); q=sappend(q, " - OPEN    B - BUILD    R - RESET"); }
          *q=0; draw_text_center_fit(x+w/2, y+h-20, w-16, fb, 1, COL(140,170,150)); }
}
static void srct_draw_editor(int s, int x, int y, int w, int h){
    const char *p = srct_file[s]>=0? src_path(srct_file[s]) : "";
    { char b[160]; char *q=b;
      q=sappend(q, p);
      if (srct_dirty[s]) q=sappend(q, " *");
      if (srct_file[s]>=0 && livedata_file_has(p)) q=sappend(q, "  [LIVE]");
      if (src_file_differs(p)) q=sappend(q, "  LIVE VALUE DIFFERS FROM TEXT");
      *q=0; draw_text_center_fit(x+w/2, y+20, w-16, b, 1, COL(190,240,205)); }
    int tx = x+10, ty = y+40;
    int cols = (w-20)/8;
    int mob = mobile_ui();
    int lines = mob? (h-40-52)/16 : (h-40-24)/16;
    int total_lines = srct_total_lines(s);
    int total_drows = srct_total_drows(s, cols);
    if (srct_top[s] > total_drows-lines) srct_top[s] = total_drows-lines;
    if (srct_top[s]<0) srct_top[s]=0;
    // walk to the logical line + segment the top display row shows
    // (line starts advance incrementally — one pass, no rescans),
    // then render segment by segment (v1.27 soft wrap)
    int ln=0, segi=0, acc=0, lstart=0;
    while (ln<total_lines){
        int segs = srct_line_segs(srct_line_len(s, lstart), cols);
        if (srct_top[s] < acc+segs){ segi = srct_top[s]-acc; break; }
        acc += segs; ln++; lstart += srct_line_len(s, lstart)+1;
    }
    if (ln>=total_lines){ ln = total_lines-1; segi = 0; lstart = srct_line_start(s, ln); }
    char seg[600];
    for (int vi=0; vi<lines; vi++){
        if (ln>=total_lines) break;
        int llen = srct_line_len(s, lstart);
        int dn=0;
        int c0 = segi*cols, c1 = c0+cols; if (c1>llen) c1=llen;
        for (int ci=c0; ci<c1 && dn<(int)sizeof(seg)-1; ci++){
            char c = src_edit_buf[s][lstart+ci];
            seg[dn++] = (c=='\t')? ' ' : c;
        }
        seg[dn]=0;
        draw_text(tx, ty+vi*16, seg, 1, COL(205,235,212));
        if (++segi >= srct_line_segs(llen, cols)){ segi=0; ln++; lstart += llen+1; }
    }
    int line, col, ls; srct_cursor_lc(s, &line, &col, &ls);
    if (srct_can_edit(G.store_pi)){
        int drow, doff; srct_cursor_display(s, cols, &drow, &doff);
        if (drow>=srct_top[s] && drow<srct_top[s]+lines && doff<=cols){
            int cx = tx + doff*8, cy = ty + (drow-srct_top[s])*16;
            rect_fill(cx, cy, 8, 16, COL(110,230,160));
            if (srct_cur[s]<srct_elen[s] && src_edit_buf[s][srct_cur[s]]!='\n'){
                char one[2]; one[0]=src_edit_buf[s][srct_cur[s]]; one[1]=0;
                draw_text(cx, cy, one, 1, COL(8,20,12));
            }
        }
        if (!mob){
            { char b[96]; char *q=b;
              q=sappend(q, "LN "); q=sapp_int(q, line+1); q=sappend(q, " COL "); q=sapp_int(q, col+1);
              q=sappend(q, "    CTRL+S SAVE");
              if (srct_file[s]>=0 && livedata_file_has(p)) q=sappend(q, "    CTRL+T TUNE");
              q=sappend(q, "    ESC BACK"); *q=0;
              draw_text_center_fit(x+w/2, y+h-20, w-16, b, 1, COL(140,170,150)); }
        }
    } else if (!mob) {
        draw_text_center_fit(x+w/2, y+h-20, w-16, "EDITING NEEDS A KEYBOARD - VIEW ONLY", 1, COL(255,214,130));
    }
    if (mob){
        // the status row (LN/COL, or the note when one is up) and the
        // tappable touch footer (v1.27): the input side hit-tests the
        // exact quarters drawn here
        if (!srct_note[s][0]){
            char b[64]; char *q=b;
            q=sappend(q, "LN "); q=sapp_int(q, line+1); q=sappend(q, " COL "); q=sapp_int(q, col+1);
            if (srct_dirty[s]) q=sappend(q, "  * UNSAVED");
            *q=0;
            draw_text_center_fit(x+w/2, y+h-48, w-16, b, 1, COL(140,170,150));
        }
        static const char *FL[4] = { "SAVE", "NEW LINE", "TUNE", "BACK" };
        int fy = y+h-26, zw = w/4;
        for (int i=0;i<4;i++){
            int bx = x + i*zw, bw2 = (i==3)? (x+w)-bx : zw;
            int dim = (i==2 && !(srct_file[s]>=0 && livedata_file_has(p)));
            rect_fill(bx+2, fy, bw2-4, 22, dim? COL(24,30,26) : COL(26,54,36));
            rect_line(bx+2, fy, bw2-4, 22, COL(90,220,150));
            draw_text_center(bx+bw2/2, fy+7, FL[i], 1, dim? COL(120,140,125) : COL(230,245,232));
        }
    }
    srct_draw_note(s, x, mob? y+h-48 : y+h-36, w);
    (void)ls;
}
static void srct_draw_prompt(int s, int x, int y, int w, int h){
    srct_draw_editor(s, x, y, w, h);
    // v1.27: the box fits INSIDE the compact panel (340px never did)
    int bw = (w-16<340)? w-16 : 340, bh = 110, bx = x + (w-bw)/2, by = y + (h-bh)/2;
    rect_fill(bx-4, by-4, bw+8, bh+8, OUTLINE);
    rect_fill(bx, by, bw, bh, COL(20,26,22));
    rect_line(bx+3, by+3, bw-6, bh-6, COL(90,220,150));
    if (srct_prompt[s]==0){
        draw_text_center_fit(bx+bw/2, by+12, bw-16, "UNSAVED CHANGES", 2, COL(255,226,150));
        static const char *PR[3] = { "SAVE AND CLOSE FILE", "DISCARD CHANGES", "KEEP EDITING" };
        for (int i=0;i<3;i++){
            int ry = by+44+i*20;
            if (i==srct_psel[s]){ rect_fill(bx+16, ry-2, bw-32, 17, COL(26,54,36)); rect_fill(bx+16, ry-2, 3, 17, COL(120,255,170)); }
            draw_text_center_fit(bx+bw/2, ry, bw-32, PR[i], 1, COL(230,245,232));
        }
    } else {
        draw_text_center_fit(bx+bw/2, by+12, bw-16, "RESET TO STOCK?", 2, COL(255,226,150));
        draw_text_center_fit(bx+bw/2, by+36, bw-16, "EVERY EDIT AND TUNING OVERRIDE GOES.", 1, COL(200,220,205));
        static const char *PR2[2] = { "RESET EVERYTHING", "KEEP MY EDITS" };
        for (int i=0;i<2;i++){
            int ry = by+58+i*20;
            if (i==srct_psel[s]){ rect_fill(bx+16, ry-2, bw-32, 17, COL(26,54,36)); rect_fill(bx+16, ry-2, 3, 17, COL(120,255,170)); }
            draw_text_center_fit(bx+bw/2, ry, bw-32, PR2[i], 1, COL(230,245,232));
        }
    }
}
static void srct_draw_build(int s, int x, int y, int w, int h){
    draw_text_center(x+w/2, y+20, "BUILD", 1, COL(190,240,205));
    if (!plat_srcbuild_available()){
        draw_text_center_fit(x+w/2, y+90, w-16, "BUILD NEEDS THE HOST APP", 2, COL(255,226,150));
        draw_text_center_fit(x+w/2, y+130, w-16, "YOUR EDITS ARE SAVED AND WILL BUILD THERE.", 1, COL(215,240,220));
        draw_text_center_fit(x+w/2, y+150, w-16, "RUN THE DESKTOP APP, OPEN THIS TERMINAL,", 1, COL(215,240,220));
        draw_text_center_fit(x+w/2, y+166, w-16, "AND CHOOSE BUILD - IT COMPILES AND SERVES", 1, COL(215,240,220));
        draw_text_center_fit(x+w/2, y+182, w-16, "YOUR PAGE FROM ITS OWN WINDOW.", 1, COL(215,240,220));
        draw_text_center_fit(x+w/2, y+h-20, w-16, hint_dev()==DEV_PAD? "BACK - BACK" : hint_dev()==DEV_TOUCH? "TAP - BACK" : "ESC - BACK", 1, COL(140,170,150));
        return;
    }
    int st = plat_srcbuild_state();
    const char *sl = st==1? "BUILDING..." : st==2? "DONE - YOUR BUILD IS LIVE" : st==3? "BUILD FAILED - THE LAST GOOD PAGE STAYS SERVED" : "IDLE";
    draw_text_center_fit(x+w/2, y+44, w-16, sl, 1, st==3? COL(255,150,120) : COL(170,255,200));
    if (st==2){
        draw_text_center_fit(x+w/2, y+62, w-16, "THE HOST NOW SERVES YOUR PAGE AT ITS USUAL ADDRESS.", 1, COL(215,240,220));
        draw_text_center_fit(x+w/2, y+76, w-16, "THIS WINDOW STILL RUNS STOCK CODE - ONLY THE", 1, COL(215,240,220));
        draw_text_center_fit(x+w/2, y+90, w-16, "SERVED WEB PAGE CHANGED.", 1, COL(215,240,220));
    }
    static char logbuf[8192];
    plat_srcbuild_log(logbuf, (int)sizeof(logbuf));
    int lines = (h-120-22)/16;
    // v1.27: log lines soft-wrap like the editor's (a 79-char line
    // never fit even the full-width panel); scroll counts display rows
    int cols = (w-24)/8; if (cols<8) cols=8;
    int total=0;
    { const char *c = logbuf;
      while (*c){ int ll=0; while (*c && *c!='\n'){ ll++; c++; } if (*c=='\n') c++; total += ll<=0? 1 : (ll+cols-1)/cols; } }
    int start = srct_build_scroll[s];
    if (start > total-lines) start = total-lines;
    if (start<0) start=0;
    srct_build_scroll[s] = start;
    { const char *c = logbuf; int drow=0, vi=0;
      while (*c && vi<lines){
          char lb[512]; int ll=0;
          while (*c && *c!='\n' && ll<511) lb[ll++]=*c++;
          if (*c=='\n') c++;
          lb[ll]=0;
          int segs = ll<=0? 1 : (ll+cols-1)/cols;
          for (int sg=0; sg<segs && vi<lines; sg++){
              if (drow+sg >= start){
                  char sb[512]; int dn=0;
                  for (int ci=sg*cols; ci<ll && ci<(sg+1)*cols && dn<511; ci++) sb[dn++]=lb[ci];
                  sb[dn]=0;
                  draw_text(x+12, y+110+vi*16, sb, 1, COL(180,215,190));
                  vi++;
              }
          }
          drow += segs;
      } }
    draw_text_center_fit(x+w/2, y+h-20, w-16, hint_dev()==DEV_PAD? "D-PAD - SCROLL LOG    BACK - BACK" : hint_dev()==DEV_TOUCH? "DRAG - SCROLL LOG    TAP - BACK" : "UP/DOWN - SCROLL LOG    ESC - BACK", 1, COL(140,170,150));
}
static void srct_draw_tune(int s, int x, int y, int w, int h){
    const char *p = srct_file[s]>=0? src_path(srct_file[s]) : "";
    { char b[128]; char *q=b; q=sappend(q, "LIVE TUNING - "); q=sappend(q, p); *q=0;
      draw_text_center_fit(x+w/2, y+20, w-16, b, 1, COL(190,240,205)); }
    int vis = (h-40-38)/17;
    int rows = srct_tn[s]+2;
    if (srct_tsel[s] < srct_tscroll[s]) srct_tscroll[s] = srct_tsel[s];
    if (srct_tsel[s] > srct_tscroll[s]+vis-1) srct_tscroll[s] = srct_tsel[s]-vis+1;
    if (srct_tscroll[s] > rows-vis) srct_tscroll[s] = rows-vis;
    if (srct_tscroll[s]<0) srct_tscroll[s]=0;
    for (int vi=0; vi<vis; vi++){
        int ri = srct_tscroll[s]+vi;
        if (ri>=rows) break;
        int ry = y+40 + vi*17;
        if (ri==srct_tsel[s]){
            rect_fill(x+8, ry-1, w-16, 16, COL(26,54,36));
            rect_fill(x+8, ry-1, 3, 16, COL(120,255,170));
        }
        if (ri<srct_tn[s]){
            const TuneEntry *e = livedata_entry(srct_tidx[s][ri]);
            char vb[40];
            if (srct_pend[s][ri][0]){ scpy(vb, srct_pend[s][ri], 40); }
            else tune_fmt_num(livedata_get(srct_tidx[s][ri]), vb);
            if (livedata_is_over(srct_tidx[s][ri])){ char *q=vb; while (*q) q++; q=sappend(q, " *"); }
            // v1.27: the name fit-trims into the space the value leaves
            draw_text_fit(x+22, ry+2, (w-36-text_w(vb,1))>0? (w-36-text_w(vb,1)) : 8, e->name, 1, COL(215,240,220));
            draw_text(x+w-14-text_w(vb,1), ry+2, vb, 1, srct_pend[s][ri][0]? COL(255,226,150) : COL(190,240,205));
        } else {
            draw_text(x+22, ry+2, ri==srct_tn[s]? "[ APPLY ]" : "[ BACK ]", 1, COL(255,226,150));
        }
    }
    if (!srct_can_edit(G.store_pi))
        draw_text_center_fit(x+w/2, y+h-36, w-16, "EDITING NEEDS A KEYBOARD - VIEW ONLY", 1, COL(255,214,130));
    srct_draw_note(s, x, y+h-20, w);
    // v1.30 (audit F3): the type-a-value instruction only makes sense for
    // a viewer who can edit — a view-only pad player was told to type.
    if (!srct_note[s][0] && srct_can_edit(G.store_pi))
        draw_text_center_fit(x+w/2, y+h-20, w-16, "TYPE A NEW VALUE, THEN APPLY - IT GOES LIVE AT ONCE", 1, COL(140,170,150));
}
static void draw_srcterm_panel(void){
    int s = whale_slot_of(G.store_pi);
    if (srct_view[s]==0) srct_open_browser(s);
    int y=STORE_Y, h=STORE_H;
    int w = G.ui_compact? 308 : STORE_W;
    int x = G.ui_compact? G.view_x0 + (G.view_w - w)/2 : STORE_X;
    if (!G.ui_compact) rect_fill(0, 0, VW, VH, COL(10,12,14));
    else rect_fill(G.view_x0, G.view_y0, G.view_w, G.view_h>0? G.view_h : VH, COL(10,12,14));
    rect_fill(x, y+1, w+8, h+8, COL(4,6,5));
    rect_fill(x-4, y-4, w+8, h+8, OUTLINE);
    rect_fill(x, y, w, h, COL(13,18,15));
    rect_line(x+3, y+3, w-6, h-6, COL(60,140,90));
    rect_fill(x, y, w, 32, COL(16,36,24));
    rect_fill(x, y+31, w, 2, COL(90,220,150));
    draw_text_center(x+w/2, y+3, "THE SOURCE", 2, COL(170,255,200));
    if (srct_view[s]==SRV_BROWSE) srct_draw_browser(s, x, y, w, h);
    else if (srct_view[s]==SRV_EDIT) srct_draw_editor(s, x, y, w, h);
    else if (srct_view[s]==SRV_PROMPT) srct_draw_prompt(s, x, y, w, h);
    else if (srct_view[s]==SRV_BUILD) srct_draw_build(s, x, y, w, h);
    else if (srct_view[s]==SRV_TUNE) srct_draw_tune(s, x, y, w, h);
}
