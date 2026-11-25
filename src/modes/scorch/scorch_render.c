// CRISPY DIRT — the render half (v1.35). The logic lives in
// modes/scorch/scorch.c; everything here only reads BwSim state.
// All geometry comes from the shared helpers in scorch.c
// (scr_layout / scr_chip_rect / scr_shop_rect / scr_view_xf), so
// what the player sees is exactly what the input side claims.
static void scr_draw_unit(int pi, BwSim *S, ScrUnit *u){
    float ox, oy, sc; scr_view_xf(G.view_x0, G.view_y0, G.view_w>0? G.view_w : VW, G.view_h>0? G.view_h : VH, &ox, &oy, &sc);
    int X = (int)(ox + u->x*sc), Y = (int)(oy + u->y*sc);
    int R = (int)(6*sc); if (R<3) R=3;
    shadow_ellipse(X, Y+2, (int)(13*sc)+2, (int)(4*sc)+1);
    int cur = (S->scr_phase==SP_AIM || S->scr_phase==SP_FLY) && u->alive && scr_unit_of(S, S->scr_turn)==(int)(u - S->scr_u);
    if (u->kind==0){
        // The catapult: two wheels, a bed, an arm at the side's
        // current aim, and the buddy working it.
        int side = u->side;
        float th = S->scr_aim[side]*3.14159f/180.0f;
        float dirx = side==0? 1.0f : -1.0f;
        u32 bed = side==0? COL(122,84,46) : COL(150,60,52);
        u32 dark = side==0? COL(84,56,30) : COL(104,40,36);
        disc_fill(X-(int)(7*sc), Y-(int)(3*sc), (int)(4*sc)+1, dark);
        disc_fill(X+(int)(7*sc), Y-(int)(3*sc), (int)(4*sc)+1, dark);
        rect_fill(X-(int)(12*sc), Y-(int)(9*sc), (int)(24*sc), (int)(4*sc)+1, bed);
        int pvx = X, pvy = Y-(int)(12*sc);
        int ex = pvx + (int)(dirx*fcos(th)*22*sc), ey = pvy - (int)(fsin(th)*22*sc);
        line_thick(pvx, pvy, ex, ey, (int)(3*sc)+1, bed);
        disc_fill(ex, ey, (int)(3*sc)+1, COL(120,160,70));   // the loaded carcass, waiting
        // The buddy: a body and a head in their own coat (the rival
        // catapult in hot-seat wears the challenger red).
        u32 coat = side==0? bud_coat(&G.bud[pi], COL(90,150,240)) : COL(232,120,90);
        rect_fill(X-(int)(dirx*11*sc)- (int)(3*sc), pvy-(int)(2*sc), (int)(6*sc)+1, (int)(8*sc)+1, coat);
        disc_fill(X-(int)(dirx*11*sc), pvy-(int)(5*sc), (int)(4*sc)+1, coat);
        if (S->scr_2p) draw_text_center(X, Y-(int)(40*sc), side==0? "P1" : "P2", 1, COL(240,240,230));
    } else if (u->kind==1){
        // Crawler: three shrinking segments and too many legs.
        for (int sgm=2; sgm>=0; sgm--){
            int sx2 = X + (u->side==1? 1 : -1)*(int)((sgm-1)*7*sc);
            disc_fill(sx2, Y-(int)((4+ (sgm==1? 2 : 0))*sc), (int)((6-sgm)*sc)+1, sgm==1? COL(104,178,80) : COL(84,150,64));
        }
        pset(X-(int)(3*sc), Y-(int)(8*sc), COL(10,10,12)); pset(X+(int)(1*sc), Y-(int)(8*sc), COL(10,10,12));
        line_thick(X-(int)(6*sc), Y-1, X-(int)(9*sc), Y+2, 1, COL(60,110,44));
        line_thick(X+(int)(6*sc), Y-1, X+(int)(9*sc), Y+2, 1, COL(60,110,44));
    } else {
        // Spitter: a round body and a snout tracking its side's aim.
        float th = S->scr_aim[u->side]*3.14159f/180.0f;
        float dirx = u->side==0? 1.0f : -1.0f;
        disc_fill(X, Y-(int)(6*sc), R+1, COL(140,110,200));
        disc_fill(X-(int)(2*sc), Y-(int)(9*sc), (int)(2*sc)+1, COL(240,240,240));
        int ex = X + (int)(dirx*fcos(th)*12*sc), ey = Y-(int)(6*sc) - (int)(fsin(th)*12*sc);
        line_thick(X, Y-(int)(6*sc), ex, ey, (int)(3*sc)+1, COL(110,84,160));
        disc_fill(ex, ey, (int)(2*sc)+1, COL(200,170,240));
    }
    if (u->alive && u->hp<100){
        int bw2 = 28, bx = X-bw2/2, by = Y-(int)(34*sc);
        rect_fill(bx, by, bw2, 4, COL(24,24,30));
        int fw = bw2 * (u->hp<0? 0 : u->hp) / 100;
        rect_fill(bx, by, fw, 4, u->hp>40? COL(110,220,90) : COL(240,120,70));
    }
    if (cur){
        int bob = (int)(fsin(G.anim_t*4.0f)*2.0f);
        int ty = Y-(int)(46*sc)+bob;
        tri_fill(X-5, ty, X+5, ty, X, ty+7, COL(255,214,80));
    }
}

static void scr_render(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    float ox, oy, sc; scr_view_xf(vx, vy, vw2, vh2, &ox, &oy, &sc);
    // Dusk over the dirt: three sky bands, hashed stars, a low sun.
    rect_fill(vx, vy, vw2, vh2, COL(40,36,68));
    rect_fill(vx, vy+(int)(vh2*0.34f), vw2, (int)(vh2*0.30f), COL(88,60,98));
    rect_fill(vx, vy+(int)(vh2*0.64f), vw2, vh2-(int)(vh2*0.64f), COL(146,92,84));
    for (int i=0;i<40;i++){
        u32 h = hash2(i*3+1, 7, (u32)(S->scr_seed&0xFFFF));
        int sx2 = vx + (int)((h%1000)/1000.0f*vw2), sy2 = vy + (int)(((h>>10)%1000)/1000.0f*vh2*0.30f);
        pset(sx2, sy2, COL(210,210,225));
    }
    disc_fill((int)(ox+500*sc), (int)(oy+120*sc), (int)(17*sc), COL(250,196,110));
    // The dirt itself: one column per field column, topsoil over deep.
    int cw = (int)(sc)+1; if (cw<1) cw=1;
    for (int cx=0;cx<SCR_W;cx++){
        int x0 = (int)(ox + cx*sc);
        int top = (int)(oy + S->scr_h[cx]*sc);
        int bot = (int)(oy + 360*sc);
        rect_fill(x0, top, cw, bot-top, COL(122,84,46));
        int td = (int)(7*sc); if (td<2) td=2;
        rect_fill(x0, top, cw, td, COL(168,120,66));
        if ((hash2(cx, 3, (u32)(S->scr_seed&0xFFFF))%23)==0) pset(x0, top+td+2, COL(96,64,36));
    }
    // Honey pools burn amber on the surface they cover.
    for (int k=0;k<4;k++) if (S->scr_pool_left[k]>0){
        for (int cx=S->scr_pool_x0[k]; cx<=S->scr_pool_x1[k]; cx++){
            int x0 = (int)(ox + cx*sc);
            int top = (int)(oy + (S->scr_h[cx]-3)*sc);
            rect_fill(x0, top, cw, (int)(4*sc)+1, ((cx/3)%2)? COL(255,176,66) : COL(230,140,40));
        }
    }
    for (int i=0;i<S->scr_nu;i++) if (S->scr_u[i].alive) scr_draw_unit(pi, S, &S->scr_u[i]);
    // Buried units wear the dirt over their lower halves: redraw
    // the columns in front of any unit the ground has swallowed.
    for (int i=0;i<S->scr_nu;i++){
        ScrUnit *u = &S->scr_u[i];
        if (!u->alive) continue;
        for (int cx=(int)u->x-11; cx<=(int)u->x+11; cx++){
            if (cx<0||cx>=SCR_W) continue;
            if (S->scr_h[cx] < u->y - 2.0f){
                int x0 = (int)(ox + cx*sc);
                int top = (int)(oy + S->scr_h[cx]*sc);
                int bot = (int)(oy + (u->y+2)*sc);
                rect_fill(x0, top, cw, bot-top, COL(130,90,50));
            }
        }
    }
    // Shells in flight, dressed by weapon.
    for (int i=0;i<3;i++) if (S->scr_pact[i]){
        int X = (int)(ox + S->scr_px[i]*sc), Y = (int)(oy + S->scr_py[i]*sc);
        int tx = X - (int)(S->scr_pvx[i]*0.05f*sc), ty = Y - (int)(S->scr_pvy[i]*0.05f*sc);
        line_thick(tx, ty, X, Y, 1, COL(230,220,200));
        int w = S->scr_pweap;
        if (w==SW_TRA) disc_fill(X, Y, 2, COL(245,245,240));
        else if (w==SW_BIG) disc_fill(X, Y, (int)(6*sc)+1, COL(120,160,70));
        else if (w==SW_DIRT) disc_fill(X, Y, (int)(4*sc)+1, COL(146,102,58));
        else if (w==SW_HON) disc_fill(X, Y, (int)(4*sc)+1, COL(255,176,66));
        else if (w==SW_BEAN) disc_fill(X, Y, (int)(5*sc)+1, ((int)(G.anim_t*6)%2)? COL(170,90,220) : COL(120,220,120));
        else { disc_fill(X, Y, (int)(4*sc)+1, COL(120,160,70)); pset(X-1, Y-1, COL(235,225,200)); }
    }
    // Burst particles.
    for (int i=0;i<40;i++) if (S->scr_fxt[i]<S->scr_fxl[i]){
        int X = (int)(ox + S->scr_fxx[i]*sc), Y = (int)(oy + S->scr_fxy[i]*sc);
        rect_fill(X, Y, 3, 3, S->scr_fxc[i]);
    }
    // ---- top bar: the cabinet name, the round, the wind, the bits.
    blend_rect(vx, vy, vw2, 20, COL(14,14,24), 205);
    draw_text(vx+6+bw_chip_dx(pi), vy+6, "CRISPY DIRT", 1, COL(255,214,130));
    {
        char buf[64]; char *q=buf;
        q=sappend(q, "ROUND "); q=sapp_int(q, S->scr_round); q=sappend(q, "  ");
        *q=0;
        int tw = text_w(buf, 1);
        draw_text(vx+(vw2-tw)/2-30, vy+6, buf, 1, COL(230,230,220));
        for (int s2=0;s2<2;s2++) for (int d=0;d<2;d++)
            disc_fill(vx+(vw2-tw)/2+tw+4 + s2*26 + d*9, vy+10, 3, S->scr_wins[s2]>d? (s2==0? COL(255,214,80) : COL(240,110,90)) : COL(70,70,84));
    }
    {
        char buf[48]; char *q=buf;
        q=sappend(q, "WIND "); q=sapp_int(q, S->scr_wind<0? -S->scr_wind : S->scr_wind); *q=0;
        int tw = text_w(buf, 1);
        int wx = vx+vw2-118;
        draw_text(wx, vy+6, buf, 1, COL(170,220,255));
        int axx = wx+tw+6, ayy = vy+10;
        int dir = S->scr_wind>0? 1 : (S->scr_wind<0? -1 : 0);
        if (dir){ line_thick(axx, ayy, axx+dir*12, ayy, 2, COL(170,220,255)); tri_fill(axx+dir*16, ayy, axx+dir*9, ayy-3, axx+dir*9, ayy+3, COL(170,220,255)); }
        char bb[32]; q=bb; q=sappend(q, "BITS "); q=sapp_int(q, S->scr_bits[S->scr_turn]); *q=0;
        draw_text(vx+vw2-text_w(bb,1)-6, vy+6, bb, 1, COL(255,220,120));
    }
    // ---- the line under the bar: whose shot, and how to play it.
    if (S->scr_exit_arm>0)
        draw_text_center(vx+vw2/2, vy+24, "ESC AGAIN TO FORFEIT THE MATCH", 1, COL(255,110,90));
    else if (S->scr_phase==SP_AIM){
        const char *who = scr_side_ai(S, S->scr_turn)? "THE BATTERY AIMS..." :
                          (S->scr_2p? (S->scr_turn==0? "P1 - YOUR SHOT" : "P2 - YOUR SHOT") : "YOUR SHOT");
        draw_text_center(vx+vw2/2, vy+24, who, 1, COL(255,240,200));
        if (!scr_side_ai(S, S->scr_turn)){
            int dev = hint_dev();
            const char *hint = dev==DEV_TOUCH? "DRAG THE GAUGES - TAP A CHIP - FIRE WHEN READY"
                             : dev==DEV_PAD? "DPAD AIMS - LB/RB WEAPON - A FIRES"
                             : "ARROWS AIM - TAB WEAPON - SPACE FIRES";
            draw_text(vx+6, vy+24, hint, 1, COL(190,190,205));
        }
    }
    // ---- the weapon chips and the control bar.
    int bar_y, ax, aw, px, pw, fx, fy;
    scr_layout(vx,vy,vw2,vh2, &bar_y,&ax,&aw,&px,&pw,&fx,&fy);
    for (int c=0;c<7;c++){
        int cx,cy,cwid,chh; scr_chip_rect(vx,vy,vw2,vh2, c, &cx,&cy,&cwid,&chh);
        int side = S->scr_turn;
        int owned = c==0 || S->scr_inv[side][c]>0;
        int sel = S->scr_weap[side]==c && S->scr_phase==SP_AIM;
        rect_fill(cx, cy, cwid, chh, sel? COL(70,58,26) : COL(26,26,36));
        rect_line(cx, cy, cwid, chh, sel? COL(255,214,80) : (owned? COL(120,120,140) : COL(60,60,74)));
        draw_text_center(cx+cwid/2, cy+3, SCR_WSHORT[c], 1, owned? COL(240,240,230) : COL(120,120,135));
        char nb[20]; char *q=nb;
        if (c==0) q=sappend(q, "FREE");
        else if (owned){ q=sappend(q, "x"); q=sapp_int(q, S->scr_inv[side][c]); }
        else { q=sapp_int(q, SCR_PRICE[c]); q=sappend(q, " BITS"); }
        *q=0;
        draw_text_center(cx+cwid/2, cy+12, nb, 1, owned? COL(200,220,160) : COL(130,130,145));
    }
    blend_rect(vx, bar_y, vw2, 46, COL(14,14,24), 215);
    draw_text(vx+8, bar_y+5, "ANGLE", 1, COL(200,200,215));
    { char nb[8]; char *q=nb; q=sapp_int(q, (int)(S->scr_aim[S->scr_turn]+0.5f)); *q=0; draw_text(ax+aw+6, bar_y+5, nb, 1, COL(255,240,200)); }
    rect_fill(ax, bar_y+17, aw, 10, COL(40,40,54));
    rect_fill(ax, bar_y+17, (int)(aw * S->scr_aim[S->scr_turn]/180.0f), 10, COL(255,190,80));
    draw_text_center(ax+11, bar_y+18, "-", 1, COL(20,20,28));
    draw_text_center(ax+aw-11, bar_y+18, "+", 1, COL(20,20,28));
    draw_text(px-50, bar_y+5, "POWER", 1, COL(200,200,215));
    { char nb[8]; char *q=nb; q=sapp_int(q, (int)(S->scr_pow[S->scr_turn]+0.5f)); *q=0; draw_text(px+pw+6, bar_y+5, nb, 1, COL(255,240,200)); }
    rect_fill(px, bar_y+17, pw, 10, COL(40,40,54));
    rect_fill(px, bar_y+17, (int)(pw * S->scr_pow[S->scr_turn]/100.0f), 10, COL(240,120,80));
    draw_text_center(px+11, bar_y+18, "-", 1, COL(20,20,28));
    draw_text_center(px+pw-11, bar_y+18, "+", 1, COL(20,20,28));
    {
        int live = S->scr_phase==SP_AIM && !scr_side_ai(S, S->scr_turn);
        disc_fill(fx+22, fy+22, 20, live? COL(216,64,54) : COL(90,50,48));
        draw_text_center(fx+22, fy+18, "FIRE", 1, COL(255,240,235));
    }
    // ---- phase panels.
    if (S->scr_phase==SP_BANNER){
        blend_rect(vx, vy+vh2/2-44, vw2, 76, COL(10,10,18), 170);
        if (S->scr_settle_stage==1){
            int winner = S->scr_lastwin;
            const char *t = winner<0? "ROUND TO NOBODY - MUTUAL DIRT"
                          : S->scr_2p? (winner==0? "ROUND TO P1" : "ROUND TO P2")
                                     : (winner==0? "ROUND TO YOU" : "ROUND TO THE BATTERY");
            draw_text_center(vx+vw2/2, vy+vh2/2-30, t, 2, COL(255,220,130));
            char buf[40]; char *q=buf; q=sappend(q, "WINS "); q=sapp_int(q, S->scr_wins[0]); q=sappend(q, " - "); q=sapp_int(q, S->scr_wins[1]); *q=0;
            draw_text_center(vx+vw2/2, vy+vh2/2+6, buf, 1, COL(230,230,220));
        } else {
            char buf[40]; char *q=buf; q=sappend(q, "ROUND "); q=sapp_int(q, S->scr_round); *q=0;
            draw_text_center(vx+vw2/2, vy+vh2/2-30, buf, 2, COL(255,220,130));
            draw_text_center(vx+vw2/2, vy+vh2/2+6, S->scr_round==1? "FIRST TO 2 ROUNDS TAKES THE MATCH" : "THE DIRT REMEMBERS NOTHING. NEW HILLS.", 1, COL(230,230,220));
        }
    }
    if (S->scr_phase==SP_SHOP){
        int sx0, sy0, sw, sh; scr_shop_rect(vx,vy,vw2,vh2, &sx0,&sy0,&sw,&sh);
        rect_fill(sx0, sy0, sw, sh, COL(22,22,34));
        rect_line(sx0, sy0, sw, sh, COL(255,214,80));
        const char *who = S->scr_2p? (S->scr_shop_side==0? "P1 SHOPS" : "P2 SHOPS") : "FIELD SHOP";
        draw_text(sx0+12, sy0+10, who, 2, COL(255,220,130));
        { char bb[40]; char *q=bb; q=sappend(q, "BITS "); q=sapp_int(q, S->scr_bits[S->scr_shop_side]); *q=0;
          draw_text(sx0+sw-text_w(bb,1)-12, sy0+16, bb, 1, COL(255,220,120)); }
        draw_text(sx0+12, sy0+36, S->scr_2p? "STOCK UP - THE NEXT HILLS ARE FRESH." : "THE BATTERY HAS RESTOCKED. YOUR TURN.", 1, COL(190,190,205));
        for (int r=0;r<7;r++){
            int rx, ry, rw, rh; scr_shop_row_rect(vx,vy,vw2,vh2, r, &rx,&ry,&rw,&rh);
            int sel = S->scr_shop_sel==r;
            if (sel){ rect_fill(rx, ry, rw, rh, COL(52,46,24)); rect_line(rx, ry, rw, rh, COL(255,214,80)); }
            if (r<6){
                int w = r+1;
                draw_text(rx+6, ry+5, SCR_WNAME[w], 1, COL(240,240,230));
                char rb[28]; char *q=rb;
                if (S->scr_inv[S->scr_shop_side][w]>0){ q=sappend(q, "OWN "); q=sapp_int(q, S->scr_inv[S->scr_shop_side][w]); q=sappend(q, " - "); }
                q=sapp_int(q, SCR_PRICE[w]); q=sappend(q, " BITS"); *q=0;
                draw_text(rx+rw-text_w(rb,1)-6, ry+5, rb, 1, S->scr_bits[S->scr_shop_side]>=SCR_PRICE[w]? COL(200,220,160) : COL(150,110,100));
            } else {
                draw_text(rx+6, ry+5, "TO BATTLE", 1, COL(255,220,130));
                draw_text(rx+rw-text_w(">",1)-6, ry+5, ">", 1, COL(255,220,130));
            }
        }
        { int sel = S->scr_shop_sel;
          if (sel<6) draw_text_center(vx+vw2/2, sy0+sh-16, SCR_WDESC[sel+1], 1, COL(170,220,255)); }
    }
    if (S->scr_phase==SP_END){
        int pw2 = vw2<380? vw2-16 : 360, ph2 = 148;
        int px0 = vx+(vw2-pw2)/2, py0 = vy+(vh2-ph2)/2;
        rect_fill(px0, py0, pw2, ph2, COL(22,22,34));
        rect_line(px0, py0, pw2, ph2, COL(255,214,80));
        const char *t = S->scr_wins[0]>=2? (S->scr_2p? "P1 TAKES THE MATCH" : "YOU TAKE THE MATCH")
                                         : (S->scr_2p? "P2 TAKES THE MATCH" : "THE BATTERY TAKES IT");
        draw_text_center(vx+vw2/2, py0+18, t, 2, COL(255,220,130));
        int ly = py0+52;
        if (S->scr_xp_paid){
            char buf[48]; char *q=buf; q=sappend(q, "+"); q=sapp_int(q, S->scr_xp_amt); q=sappend(q, " BUDDY XP"); *q=0;
            draw_text_center(vx+vw2/2, ly, buf, 1, COL(170,255,170)); ly += 16;
        }
        if (!S->scr_2p && S->scr_wins[0]>=2 && S->scr_diff==2 && b->scr_champ){
            draw_text_center(vx+vw2/2, ly, "THE CRISPY DIRT PLAQUE IS YOURS", 1, COL(255,214,80)); ly += 16;
        }
        if (S->scr_2p) { draw_text_center(vx+vw2/2, ly, "GLORY ONLY - NO XP AT A SHARED SCREEN", 1, COL(190,190,205)); ly += 16; }
        draw_text_center(vx+vw2/2, py0+ph2-22, "FIRE / TAP - BACK TO THE ARCADE", 1, COL(190,190,205));
    }
    if (S->note_t>0 && S->note[0])
        draw_text_center(vx+vw2/2, bar_y-40, S->note, 1, COL(255,240,200));
}
