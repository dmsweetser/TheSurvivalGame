// THE SURVIVAL GAME — mall/panel.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 1380-1533 in the Nov 2026 fine-grained refactor; code unchanged.

static void draw_mall_panel(int st){
    Player *me=&G.p[G.store_pi];
    Buddy *bud=&G.bud[G.store_pi];
    int sel = (G.p2_slot>=0 && G.store_pi==G.p2_slot)? G.store_sel2 : G.store_sel;
    int y=STORE_Y, h=STORE_H;
    int w = G.ui_compact? 308 : STORE_W;
    int x = G.ui_compact? G.view_x0 + (G.view_w - w)/2 : STORE_X;
    static const u32 BAND[4] = { COL(74,36,34), COL(64,40,84), COL(36,74,52), COL(120,66,32) };
    if (!G.ui_compact) rect_fill(0, 0, VW, VH, COL(16,14,28));
    else rect_fill(G.view_x0, G.view_y0, G.view_w, G.view_h>0? G.view_h : VH, COL(16,14,28));
    rect_fill(x, y+1, w+8, h+8, COL(8,6,14));
    rect_fill(x-4, y-4, w+8, h+8, OUTLINE);
    rect_fill(x, y, w, h, COL(250,244,225));
    rect_line(x+3, y+3, w-6, h-6, COL(198,164,84));
    rect_fill(x, y, w, 32, st==7? COL(96,26,26) : st==6? COL(32,58,74) : st==5? COL(96,64,24) : BAND[st&3]);
    rect_fill(x, y+31, w, 2, COL(198,164,84));
    draw_text_center(x+w/2, y+3, MALL_STATION[st], 2, COL(255,226,150));
    { char b[96]; char *q=b;
      if (st==7){ q=sappend(q, "ONE NAME AT A TIME - PAID IN WEIGHT AND WISDOM"); }
      else if (st==6){ q=sappend(q, "THE SAME CHALLENGE FOR EVERYBODY - NEW EVERY MONDAY"); }
      else if (st==5){ q=sappend(q, "THIS SLOT'S PLAQUES - THE FAME IS EVERYONE'S"); }
      else if (st==1){ q=sappend(q, "PURE VANITY - $0.67 EACH - NO GAMEPLAY EFFECT"); }
      else if (st==2){ q=sappend(q, "BANKED: "); q=sapp_int(q, (int)me->banked); q=sappend(q, " KG - ON YOUR HEAD: "); q=sapp_int(q, (int)me->carried); q=sappend(q, " KG"); }
      else if (bud_stage[G.store_pi]==1 || bud_stage[G.store_pi]==3){ q=sappend(q, "NAMES OF THE ELDER DAYS - PAGE "); q=sapp_int(q, bud_page[G.store_pi]+1); q=sappend(q, " OF "); q=sapp_int(q, (BUD_NAMES_N+11)/12); }
      else if (bud_stage[G.store_pi]==2 || bud_stage[G.store_pi]==4){ q=sappend(q, "STANDARD COATS - AND ONE VERY NEON RAINBOW"); }
      else if (bud->active){ q=sappend(q, BUD_ELF[bud->name_idx%BUD_NAMES_N]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[bud->species%BUDN]); q=sappend(q, " - XP "); q=sapp_int(q, bud->xp);
          q=sappend(q, " - HP "); q=sapp_int(q, (int)bud->hp); q=sappend(q, "/"); q=sapp_int(q, (int)bud->maxhp);
          if (bud->fainted) q=sappend(q, " - FAINTED"); }
      else q=sappend(q, "NO BUDDY YET - EVERY DEV NEEDS A FRIEND");
      draw_text_center(x+w/2, y+20, b, 1, COL(255,240,205)); }
    if (!G.ui_compact)
    { const char *sh = "SHIELD: UP";
      draw_text(x+w-14-text_w(sh,1), y+8, sh, 1, COL(150,220,255)); }
    if (st==1 && whale_code_mode[G.store_pi]){
        // typing/pasting an unlock code replaces the price list
        draw_text_center(x+w/2, y+64, "TYPE OR PASTE YOUR UNLOCK CODE", 2, COL(40,36,50));
        rect_fill(x+16, y+108, w-32, 46, COL(255,252,240));
        rect_line(x+16, y+108, w-32, 46, COL(198,164,84));
        { const char *cb = whale_code_buf[G.store_pi]; int cl = whale_code_len[G.store_pi];
          int show = cl>56? cl-56 : 0;
          char disp[64]; int dn=0; for (int i=show;i<cl && dn<56;i++) disp[dn++]=cb[i]; disp[dn]=0;
          draw_text_center(x+w/2, y+118, dn? disp : "...", 1, COL(40,36,50));
          char cb2[24]; char *q2=cb2; q2=sapp_int(q2, cl); q2=sappend(q2, " / 142 CHARS"); *q2=0;
          draw_text_center(x+w/2, y+136, cb2, 1, COL(120,110,90)); }
        draw_text_center(x+w/2, y+176, "THE CODE IS YOUR RECEIPT - KEEP IT SAFE.", 1, COL(90,80,60));
        draw_text_center(x+w/2, y+192, "UNLOCKS LIVE IN YOUR SAVE DATA; THE CODE RESTORES THEM.", 1, COL(90,80,60));
        draw_text_center(x+w/2, y+224, hint_dev()==DEV_PAD? "A - REDEEM     BACK - CANCEL" : "ENTER - REDEEM     ESC - CANCEL", 1, COL(40,36,50));
        return;
    }
    int rows = station_rows(st, me);
    int rowh = STORE_ROWH;   // Whales Only scrolls at full size now (Nov 5)
    int wsc = 0;             // the visible window's first row (Whales Only + the Trophy Case)
    if (st==1){ whale_scroll_sync(whale_slot_of(G.store_pi), sel, rows); wsc = whale_scroll[whale_slot_of(G.store_pi)]; }
    if (st==5){ case_scroll_sync(whale_slot_of(G.store_pi), sel, rows); wsc = case_scroll[whale_slot_of(G.store_pi)]; }
    int rows_end = rows;
    if ((st==1 || st==5) && rows_end > wsc+WHALE_VIS) rows_end = wsc+WHALE_VIS;
    char nb[40], rb[24];
    for (int i=wsc;i<rows_end;i++){
        int ry = MALL_ROW0 + (i-wsc)*rowh;
        if (i%2==0) rect_fill(x+8, ry-2, w-16, rowh, COL(243,236,210));
        rect_line(x+8, ry-2, w-16, rowh, COL(228,219,192));
        if (i==sel){
            rect_fill(x+8, ry-2, w-16, rowh, COL(255,224,138));
            rect_fill(x+8, ry-2, 3, rowh, COL(190,60,40));
            rect_line(x+8, ry-2, w-16, rowh, COL(198,164,84));
        }
        const char *nm = station_row_name(st, i, me, nb);
        const char *rt = station_row_right(st, i, me, rb);
        if (st==5){
            // TROPHY CASE: a plaque chip at the row's left — filled
            // in its colour when earned, a hollow ring while it waits
            int cid = case_row_id(me, i);
            int hdr = (cid==CR_HDR_CAB || cid==CR_HDR_MED || cid==CR_HDR_CREW);
            if (!hdr){
                int ccx = x + (G.ui_compact? 14 : 17), ccy = ry + (G.ui_compact? 8 : 9), cr = G.ui_compact? 3 : 4;
                if (case_earned(me, cid)) disc_fill(ccx, ccy, cr, case_chip_col(me, cid));
                else circle_out(ccx, ccy, cr, COL(170,160,130));
            }
            if (G.ui_compact) draw_text(x+23, ry+5, nm, 1, hdr? COL(150,95,30) : COL(40,36,50));
            else draw_text(x+28, ry+2, nm, 2, hdr? COL(150,95,30) : COL(40,36,50));
        }
        else if (st==6){
            // CHALLENGE BOARD: section headers in amber, like the
            // case's — the rows are a card and a ledger, not a shop
            int wid = wk_row_id(i);
            int hdr = (wid==WK_HDR_WEEK || wid==WK_HDR_LEDGER);
            if (G.ui_compact) draw_text(x+16, ry+5, nm, 1, hdr? COL(150,95,30) : COL(40,36,50));
            else draw_text(x+16, ry+2, nm, 2, hdr? COL(150,95,30) : COL(40,36,50));
        }
        else if (st==7){
            // BOUNTY BOARD (feat 8): headers in amber, the wanted
            // name itself in wanted-poster red
            int bid = bn_row_id(i);
            int hdr = (bid==BN_HDR_WANTED || bid==BN_HDR_LEDGER);
            u32 nc = (bid==BN_BOUNTY)? COL(150,50,40) : hdr? COL(150,95,30) : COL(40,36,50);
            if (G.ui_compact) draw_text(x+16, ry+5, nm, 1, nc);
            else draw_text(x+16, ry+2, nm, 2, nc);
        }
        else if (G.ui_compact) draw_text(x+16, ry+5, nm, 1, COL(40,36,50));
        else draw_text(x+16, ry+2, nm, 2, COL(40,36,50));
        if (st==3 && (bud_stage[G.store_pi]==2 || bud_stage[G.store_pi]==4)){
            // a live swatch beside each coat — the rainbow one cycles
            Buddy *pb = &G.bud[G.store_pi]; int keep = pb->color; pb->color = i;
            u32 sw = bud_coat(pb, COL(168,164,158)); pb->color = keep;
            int swx = x+w-40, swy = ry + (G.ui_compact? 4:6), sww = G.ui_compact? 14:18, swh = G.ui_compact? 9:12;
            rect_fill(swx, swy, sww, swh, sw); rect_line(swx, swy, sww, swh, COL(90,80,60));
        }
        u32 pc = COL(150,95,30);
        if (G.ui_compact) draw_text(x+w-14-text_w(rt,1), ry+5, rt, 1, pc);
        else draw_text(x+w-14-text_w(rt,2), ry+2, rt, 2, pc);
    }
    if ((st==1 || st==5) && rows>WHALE_VIS){
        // the scrollbar: a track in the panel margin, a thumb that
        // says where in the sixteen rows you are (craft menu style)
        int track_y = MALL_ROW0-2, track_h = WHALE_VIS*STORE_ROWH-4;
        rect_fill(x+w-6, track_y, 3, track_h, COL(214,204,178));
        int th = track_h*WHALE_VIS/rows; if (th<12) th=12;
        int ty2 = track_y + (track_h-th)*wsc/(rows-WHALE_VIS);
        rect_fill(x+w-6, ty2, 3, th, COL(190,60,40));
    }
    int by = y+h-40;
    int bh = G.ui_compact? 24 : 20;
    rect_fill(x+8, by-4, w-16, bh, COL(240,232,205));
    rect_line(x+8, by-4, w-16, bh, COL(198,164,84));
    rect_fill(x+8, by-4, 3, bh, COL(190,60,40));
    if (G.ui_compact){
        char bb[128]; scpy(bb, station_blurb(st, sel, me), 128);
        int cut=-1; for (int i=0; bb[i] && i<34; i++) if (bb[i]==' ') cut=i;
        if (cut>0 && bb[cut+1]){ bb[cut]=0;
            draw_text(x+14, by+1, bb, 1, COL(80,66,44));
            draw_text(x+14, by+12, bb+cut+1, 1, COL(80,66,44));
        } else draw_text(x+14, by+2, bb, 1, COL(80,66,44));
    }
    else draw_text(x+14, by+2, station_blurb(st, sel, me), 1, COL(80,66,44));
    if (pay_active) draw_text_center(x+w/2, y+h-13, "COUNTING CARCASSES...", 1, COL(190,60,40));
    else if (G.store_msg_t>0) draw_text_center(x+w/2, y+h-13, G.store_msg, 1, COL(60,140,60));
    else if (G.ui_compact){ if (hint_dev()==DEV_TOUCH) draw_text_center(x+w/2, y+h-13, "TAP A ROW - TAP AGAIN TO SELECT", 1, COL(88,76,54));
        else { char b[80]; char *q=b; q=sappend(q, "SHIELD UP - "); q=sappend(q, lbla(H_FIRE)); q=sappend(q, " SELECTS - "); q=sappend(q, lbla(H_BACK)); q=sappend(q, " LEAVES");
               draw_text_center(x+w/2, y+h-13, b, 1, COL(88,76,54)); } }
    else if (hint_dev()==DEV_TOUCH) draw_text_center(x+w/2, y+h-13, "TAP A ROW - TAP AGAIN TO SELECT - TAP OUTSIDE TO LEAVE", 1, COL(88,76,54));
    else { char b[96]; char *q=b; q=sappend(q, lbla(H_MOVE)); q=sappend(q, " CHOOSE - "); q=sappend(q, lbla(H_CONFIRM));
           q=sappend(q, " SELECTS - "); q=sappend(q, lbla(H_INT)); q=sappend(q, " LEAVES");
           draw_text_center(x+w/2, y+h-13, b, 1, COL(88,76,54)); }
}
// ---------------------------------------------------------------------------
// HAT PORTRAIT BOOTH (v1.25 feature 9) — its own station (index 8)
// since the MALL ENLARGEMENT: booth_stage 0 is the booth (live pose),
// 1 is YOUR PORTRAIT (the frozen record under glass). The pose follows
// render_over's game-over portrait: a synthetic hero Player, the
// camera inverted at zoom 1 so it stands at an exact screen spot.
// The buddy is the REAL G.bud[pi] with its pose fields borrowed and
// restored — never a temp Buddy: draw_buddy's coat path does owner
// arithmetic on the real struct (the feat-5 warning).
// ---------------------------------------------------------------------------
