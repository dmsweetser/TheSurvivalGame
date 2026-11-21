// THE SURVIVAL GAME — banzai/cabinet.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from banzai.c lines 1048-1185 in the Nov 2026 fine-grained refactor; code unchanged.

// --- the Arcade cabinet panel -------------------------------------------------
// One cabinet per game: PLAY launches the cabinet's game with the
// buddy, LEAVE keeps you in the arcade. Mirrors the gate panel's
// behaviour (keys, pad, taps) so every device drives it the same way.
static void arc_gate_try(int pi){
    Player *p=&G.p[pi]; Buddy *b=&G.bud[pi];
    if (G.net_mode==NET_CLIENT) return;
    if (G.net_mode==NET_HOST && any_remote_connected()){ spawn_floater(p->x, p->y-24, "THE ARCADE IS A SOLO TRIP", COL(255,200,150)); sfx(16); return; }
    if (!b->active){ spawn_floater(p->x, p->y-24, "YOU NEED A BANZAI BUDDY FIRST", COL(255,150,150)); sfx(16); return; }
    if (b->fainted){ spawn_floater(p->x, p->y-24, "YOUR BUDDY FAINTED - REVIVE IT FIRST", COL(255,150,150)); sfx(16); return; }
    arcgate[pi]=1; arcgate_sel[pi]=0; sfx(19);
}
// CASTLE SHUFFLE (v1.25 feat 7): once the buddy has escaped the
// castle once (cas_best > 0), the CASTLE cabinet's panel — and only
// that panel — offers a third row, PLAY SHUFFLE, between PLAY
// CLASSIC and LEAVE. Every other panel, and CASTLE before the
// first clear, is the shipped two rows.
// DEEPER DOORS (v1.25 feat 11): the same courtesy at the ENTRANCES
// cabinet — once the buddy has got out of Entrances once
// (ent_done > 0), its panel offers CHAPTER 1 / DEEPER DOORS /
// LEAVE. The two unlocks are independent; each panel checks its
// own buddy's own record.
static int arc_gate_rows(int pi){
    if (arc_cab[pi]%7==4 && G.bud[pi].cas_best>0) return 3;
    if (arc_cab[pi]%7==0 && G.bud[pi].ent_done>0) return 3;
    return 2;
}
static void arc_gate_draw(int pi, int vx, int vy, int vw2, int vh2){
    Buddy *b=&G.bud[pi];
    int cab = arc_cab[pi]%7;
    int rows = arc_gate_rows(pi);
    int w = vw2<340? vw2-16 : 320, h = rows==3? 152 : 132;
    int x = vx + (vw2-w)/2, y = vy + (vh2-h)/2;
    if (y < vy+4) y = vy+4;
    rect_fill(x, y, w, h, COL(14,16,30)); rect_line(x, y, w, h, COL(120,240,255));
    draw_text_center(x+w/2, y+10, ARC_NAME[cab], 2, COL(140,255,255));
    { char buf[64]; char *q=buf; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[b->species%BUDN]); q=sappend(q, " GRABS THE CONTROLS"); *q=0;
      draw_text_center(x+w/2, y+32, buf, 1, COL(200,214,200)); }
    static const char *DESC[7] = {
        "A COZY HOTEL OF DOORS. HIDE IN THE BUSHES. MIND THE ZIP BUG.",
        "A SIDE-SCROLLING RUN. STOMP BUGS. PLANT THE FLAG.",
        "AN ENDLESS WILD. CHOP, MINE, CRAFT, BUILD.",
        "DRIVE THE TANK. FREE THE CAPTIVES. BREAK THE BOSS.",
        "A CASTLE TO PLUNDER! 13 TREASURES, ONE KEY, ONE GATE.",
        "GRAND PRIX! 4 RACES, 8 REGIONS, 2 CUPS. RANK BY POINTS.",
        "A TAP-FIRST RTS. HARVEST BITS, RAISE BUDDIES, CONQUER 8 REGIONS." };
    for (int r=0;r<rows;r++){
        int ry = y + 52 + r*20;
        const char *nm = rows==3? (cab==0? (r==0? "CHAPTER 1" : r==1? "DEEPER DOORS" : "LEAVE")   // DEEPER DOORS (feat 11)
                                          : (r==0? "PLAY CLASSIC" : r==1? "PLAY SHUFFLE" : "LEAVE"))   // CASTLE SHUFFLE (feat 7)
                                : (r==0? "PLAY" : "LEAVE");
        if (arcgate_sel[pi]==r){ rect_fill(x+10, ry-3, w-20, 15, COL(30,60,80)); draw_text(x+16, ry, ">", 1, COL(140,255,255)); }
        draw_text(x+30, ry, nm, 1, arcgate_sel[pi]==r? COL(235,255,255) : COL(170,180,190));
    }
    draw_text_center(x+w/2, y+h-16, DESC[cab], 1, COL(150,170,190));
    if (mobile_ui() && !pad_mode() && pi==G.my_id)
        draw_text_center(x+w/2, y+h-30, "TAP TO PLAY", 1, COL(120,132,150));
    else { char buf[64]; char *q=buf; q=sappend(q, lbl_fire()); q=sappend(q, " - PLAY    "); q=sappend(q, lbl_exit()); q=sappend(q, " - LEAVE"); *q=0;
      draw_text_center(x+w/2, y+h-30, buf, 1, COL(120,132,150)); }
}
static void arc_gate_pick(int pi){
    int rows = arc_gate_rows(pi);
    if (arcgate_sel[pi]==0 || (arcgate_sel[pi]==1 && rows==3)){
        int cab = arc_cab[pi]%7;
        arcgate[pi]=0;
        bw_arc_src[pi]=1; bw_arc_cab[pi]=cab;
        // both pending requests are cabinet-specific (feat 11):
        // while only CASTLE had a third row, setting the shuffle
        // request on any 3-row pick was safe; with ENTRANCES
        // offering one too, an unguarded request would leak a
        // stale shuffle into a later Castle entry
        bw_cas_shuffle_req[pi] = (cab==4 && arcgate_sel[pi]==1 && rows==3)? 1 : 0;   // CASTLE SHUFFLE (feat 7)
        bw_ent_ch2_req[pi] = (cab==0 && arcgate_sel[pi]==1 && rows==3)? 1 : 0;       // DEEPER DOORS (feat 11)
        bw_enter_mode(pi, ARC_MODE[cab]);
    } else { arcgate[pi]=0; sfx(6); }
}
static void arc_gate_tap(int pi, int vx, int vy, int vw2, int vh2){
    int rows = arc_gate_rows(pi);
    int w = vw2<340? vw2-16 : 320, h = rows==3? 152 : 132;
    int x = vx + (vw2-w)/2, y = vy + (vh2-h)/2;
    if (y < vy+4) y = vy+4;
    IN.tapped = 0;
    if (IN.tap_x<x || IN.tap_x>=x+w || IN.tap_y<y || IN.tap_y>=y+h){ arcgate[pi]=0; sfx(6); return; }
    for (int r=0;r<rows;r++){
        int ry = y + 52 + r*20;
        if (IN.tap_y>=ry-4 && IN.tap_y<=ry+15){ arcgate_sel[pi]=r; arc_gate_pick(pi); return; }
    }
}
// P1's current view rectangle in co-op, computed exactly as the
// render pass computes view 0 (shared view, or P1's half of the split)
static void p1_view_rect(int *x, int *y, int *w, int *h){
    if (vsplit_f <= 0.02f && !vsplit){ *x=0; *y=0; *w=VW; *h=VH; return; }
    if (vorient){
        int seam = vp1first? VW - (int)((VW/2)*vsplit_f) : (int)((VW/2)*vsplit_f);
        *y=0; *h=VH;
        if (vp1first){ *x=0; *w=seam; } else { *x=seam; *w=VW-seam; }
    } else {
        int seam = vp1first? VH - (int)((VH/2)*vsplit_f) : (int)((VH/2)*vsplit_f);
        *x=0; *w=VW;
        if (vp1first){ *y=0; *h=seam; } else { *y=seam; *h=VH-seam; }
    }
}
// the acting player's OWN view rectangle (v1.25 feat 13): view 0
// is p1_view_rect above; view 1 is the complementary half, computed
// exactly as coop_render computes it. Solo / no second player, every
// pi gets view 0's answer — so P1's callers see no change at all.
static void pi_view_rect(int pi, int *x, int *y, int *w, int *h){
    if (pi==G.my_id || G.p2_slot<0){ p1_view_rect(x, y, w, h); return; }
    if (vsplit_f <= 0.02f && !vsplit){ *x=0; *y=0; *w=VW; *h=VH; return; }
    if (vorient){
        int seam = vp1first? VW - (int)((VW/2)*vsplit_f) : (int)((VW/2)*vsplit_f);
        *y=0; *h=VH;
        if (vp1first){ *x=seam; *w=VW-seam; } else { *x=0; *w=seam; }
    } else {
        int seam = vp1first? VH - (int)((VH/2)*vsplit_f) : (int)((VH/2)*vsplit_f);
        *x=0; *w=VW;
        if (vp1first){ *y=seam; *h=VH-seam; } else { *y=0; *h=seam; }
    }
}
static void bw_exit(int pi, int fainted){
    BwSim *S = &BWS[pi];
    Player *p=&G.p[pi]; Buddy *b=&G.bud[pi];
    if (b->bw_mode==2 || b->bw_mode==3 || b->bw_mode==5 || b->bw_mode==6 || b->bw_mode==7 || b->bw_mode==8){ b->bw_x = 0; b->bw_y = 56; }
    else if (b->bw_mode==4){ b->craft_x = b->x; b->craft_y = b->y; }
    else { b->bw_x = b->x; b->bw_y = b->y; }
    S->bat.active = 0;
    S->bw_menu = 0;
    mus_bw_note(pi, 0.0f);   // feat 15: the danger stays behind the gate
    b->in_bw = 0;
    if (S->from_arcade){ int cb2 = S->arc_cab%7; p->x = ARC_SX[cb2]; p->y = ARC_CY + ARC_ROWY[cb2] + 66; }   // back to your cabinet, whichever row it stands in
    else { p->x = BW_SX; p->y = MALL_CY+136; }
    S->from_arcade = 0;
    p->aim = -PI/2;
    b->x = p->x - 30; b->y = p->y + 10;
    do_save();
    sfx(19);
    if (fainted) spawn_floater(p->x, p->y-26, "YOU WAKE UP IN THE MALL. YOUR BUDDY FAINTED.", COL(255,180,150));
}
