// THE SURVIVAL GAME — game/srcedit.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).

// ---------------------------------------------------------------------------
// THE SOURCE ROOM (v1.26): the edit overlay model. A source file's
// CURRENT text is its edited text when one is saved (plat_mod_*),
// else the stock blob text. The overlay is independent of the game
// saves — it lives in localStorage on the web, in the save home's
// tsg-modsrc/ folder natively — and RESET STOCK clears it wholesale.
// One shared scratch buffer serves every "give me file i's current
// text" reader (the terminal's editor load, the host build's push);
// readers consume it before the next call.
// ---------------------------------------------------------------------------
#define SRC_SCRATCH_CAP 524288   // 512 KB: the editor's per-file cap too
static char src_scratch[SRC_SCRATCH_CAP];

static int src_file_edited(int idx){
    if (idx<0 || idx>=src_count()) return 0;
    return plat_mod_exists(src_path(idx));
}
static int src_edited_count(void){
    int n=0;
    for (int i=0;i<src_count();i++) n += src_file_edited(i);
    return n;
}
// file idx's current text, copied into buf (NUL-terminated when it
// fits); returns the full length (which may exceed cap-1).
static int src_file_current(int idx, char *buf, int cap){
    int len=0; const char *stock = src_text(idx, &len);
    if (src_file_edited(idx)){
        int n = plat_mod_load(src_path(idx), buf, cap);
        if (n>=0) return n;
    }
    if (buf && cap>0){
        int c = len<cap-1? len : cap-1;
        smemcpy(buf, stock, c); buf[c]=0;
    }
    return len;
}
// the shared-scratch variant: current text + length, valid until the
// next src_file_current/src_file_current_scratch call.
static const char *src_file_current_scratch(int idx, int *len){
    int n = src_file_current(idx, src_scratch, SRC_SCRATCH_CAP);
    if (len) *len = n;
    return src_scratch;
}

// LIVE TUNING (Item 3): files whose live values currently differ from
// their overlay text — set when a TUNE apply could not substitute the
// new literal into the text (the player hand-edited that line). The
// override still applies live; a later host compile uses the raw text.
#define SRC_DIFFERS_MAX 16
static char src_differs_tab[SRC_DIFFERS_MAX][64];
static int src_differs_n = 0;
static void src_file_set_differs(const char *path){
    for (int i=0;i<src_differs_n;i++) if (!scmp(src_differs_tab[i], path)) return;
    if (src_differs_n<SRC_DIFFERS_MAX){
        scpy(src_differs_tab[src_differs_n], path, 64);
        src_differs_n++;
    }
}
static int src_file_differs(const char *path){
    for (int i=0;i<src_differs_n;i++) if (!scmp(src_differs_tab[i], path)) return 1;
    return 0;
}
static void src_differs_clear(void){ src_differs_n = 0; }
