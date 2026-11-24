// THE SURVIVAL GAME — war/render.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 2583-3243 in the Nov 2026 fine-grained refactor; code unchanged.

static void war_hp_bar(int x, int y, int w, float f, int enemy){
    if (f<0) f=0; if (f>1) f=1;
    rect_fill(x, y, w, 3, COL(24,20,28));
    rect_fill(x, y, (int)(w*f), 3, enemy? COL(255,110,90) : COL(120,230,90));
}
static void war_render_map(int pi, int vx, int vy, int vw2, int vh2){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    rect_fill(vx, vy, vw2, vh2, COL(12,10,24));
    for (int i=0;i<40;i++){
        u32 h = hash2(i*7+1, i*13+5, 0x5EEDu);
        int sx = vx + (int)(h % (u32)(vw2>0? vw2 : 1)), sy = vy + (int)((h>>9) % (u32)(vh2>0? vh2 : 1));
        pset(sx, sy, COL(70,66,100));
    }
    draw_text_center(vx+vw2/2, vy+14, "WAR!", 3, COL(255,120,90));
    draw_text_center(vx+vw2/2, vy+42, "CONQUER THE EIGHT REGIONS", 1, COL(200,210,230));
    int cx = vx+vw2/2, cy = vy+vh2/2+8;
    for (int r=0;r<8;r++){
        int nx, ny; war_node_pos(vx, vy, vw2, vh2, r, &nx, &ny);
        line_thick(cx, cy, nx, ny, 1, r < b->war_prog? REGION_GA[r] : COL(52,48,78));
    }
    disc_fill(cx, cy, 10, COL(255,90,70)); disc_fill(cx, cy, 5, COL(255,200,120));
    draw_text_center(cx, cy+13, "EPICENTER", 1, COL(255,170,150));
    for (int r=0;r<8;r++){
        int nx, ny; war_node_pos(vx, vy, vw2, vh2, r, &nx, &ny);
        int done = r < b->war_prog, frontier = r == b->war_prog;
        if (frontier){
            int pr = 27 + (int)(fsin(G.anim_t*4.0f)*2.5f);
            circle_out(nx, ny, pr, COL(255,240,200));
        }
        if (war_snap_on[pi] && war_snap_reg[pi]==r){   // v1.18: a battle is parked HERE — tapping this node resumes it, and the player must be able to see that
            int pr2 = 34 + (int)(fsin(G.anim_t*4.0f + 1.5f)*3.0f);
            circle_out(nx, ny, pr2, COL(140,255,170));
            draw_text_center(nx, ny+44, "IN PROGRESS", 1, COL(160,255,190));
        }
        if (r == S->war_map_sel) circle_out(nx, ny, 31, COL(255,220,90));
        disc_fill(nx, ny, 24, done? REGION_GA[r] : frontier? col_shade(REGION_GA[r], 0.75f) : COL(52,50,66));
        circle_out(nx, ny, 24, done || frontier? col_shade(REGION_GA[r], 0.6f) : COL(84,80,104));
        if (done){
            line_thick(nx-8, ny, nx-2, ny+7, 2, COL(20,30,20));
            line_thick(nx-2, ny+7, nx+9, ny-8, 2, COL(20,30,20));
        } else if (!frontier){
            rect_fill(nx-6, ny-2, 12, 9, COL(120,116,140));
            rect_fill(nx-4, ny-8, 8, 6, COL(120,116,140));
            disc_fill(nx, ny+2, 2, COL(40,38,54));
        } else {
            char nb[4]; char *q=nb; q=sapp_int(q, r+1); *q=0;
            draw_text_center(nx, ny-7, nb, 2, COL(16,16,24));
        }
        static const char *SHORT[8] = { "MEADOWS", "SPAGHETTI", "LEGACY", "BADLANDS", "CACHE FIELDS", "RAM FLATS", "CABLE JUNGLE", "THE DEPTHS" };
        draw_text_center_fit(nx, r==4? ny-40 : ny+30, 150, SHORT[r], 1,
                             done? col_shade(REGION_GA[r], 1.25f) : frontier? COL(240,244,250) : COL(110,106,130));
    }
    { char buf[48]; char *q=buf; q=sappend(q, "REGIONS CONQUERED: "); q=sapp_int(q, b->war_prog); q=sappend(q, " / 8"); *q=0;
      draw_text_center(vx+vw2/2, vy+vh2-34, buf, 1, COL(255,220,120)); }
    { int bx, by; war_skirmish_btn(vx, vy, vw2, vh2, &bx, &by);   // v1.19: the skirmish door, bottom-right of the map
      int selb = (S->war_map_sel==8);   // v1.25: the selection's 9th stop lights the button like a picked node
      rect_fill(bx, by, 128, 28, selb? COL(56,64,88) : COL(38,44,60));
      rect_line(bx, by, 128, 28, selb? COL(255,220,120) : COL(255,170,90));
      draw_text_center(bx+64, by+9, "SKIRMISH", 2, COL(255,210,150)); }
    if (b->war_prog>=8){ char b2[96]; char *q=b2; q=sappend(q, "ALL REGIONS CONQUERED! ");
        q=sappend(q, hint_dev()==DEV_PAD? "A ON ANY REGION" : (hint_dev()==DEV_TOUCH? "TAP ANY REGION" : "CLICK ANY REGION"));
        q=sappend(q, " TO FIGHT IT AGAIN."); *q=0;
        draw_text_center(vx+vw2/2, vy+vh2-19, b2, 1, COL(200,230,200)); }
    else { char fb[96]; char *q=fb;   // v1.30 (audit F14): name the real FIRE bind, not a hard-coded SPACE
           if (hint_dev()==DEV_PAD) q=sappend(q, "STICK + A - INVADE - LAST STOP IS SKIRMISH");
           else if (hint_dev()==DEV_TOUCH) q=sappend(q, "TAP A REGION TO INVADE IT");
           else { q=sappend(q, "CLICK A REGION - ARROWS + "); q=sappend(q, key_name(G.bind[ACT_FIRE])); q=sappend(q, " - S: SKIRMISH"); }
           *q=0; draw_text_center(vx+vw2/2, vy+vh2-19, fb, 1, COL(150,160,185)); }
    if (S->war_msgt>0) draw_text_center(vx+vw2/2, vy+58, S->war_msg, 1, COL(255,170,150));
    if (S->war_sk_panel){   // v1.29: the saved-skirmish panel over the map (rects shared with the tap handler in map.c)
        blend_rect(vx, vy, vw2, vh2, COL(8,8,14), 150);
        int px, py, pw, ph; war_sk_panel_rects(vx, vy, vw2, vh2, &px, &py, &pw, &ph);
        rect_fill(px, py, pw, ph, COL(16,18,28)); rect_line(px, py, pw, ph, COL(255,170,90));
        draw_text_center(px+pw/2, py+14, "A SKIRMISH IS WAITING", 2, COL(255,210,150));
        draw_text_center(px+pw/2, py+38, "A SAVED SKIRMISH IS PARKED ON THIS FIELD.", 1, COL(200,214,200));
        for (int n=0;n<2;n++){
            int ry = py+58+n*38;
            int sel = (S->war_sk_psel==n);
            rect_fill(px+16, ry, pw-32, 32, sel? COL(56,64,88) : COL(38,44,60));
            rect_line(px+16, ry, pw-32, 32, sel? COL(255,220,120) : COL(198,164,84));
            const char *lbl = n==0? "RESUME SKIRMISH" : (S->war_sk_confirm>0? "NEW SKIRMISH? AGAIN TO DISCARD IT" : "NEW SKIRMISH");
            draw_text_center(px+pw/2, ry+10, lbl, 2, COL(240,244,250));
        }
        // v1.30 (audit F4/F14): on a pad the keep-it action is the BACK
        // button, not "ESC"; the keyboard half names the real FIRE bind.
        { char fb[80]; char *q=fb;
          if (hint_dev()==DEV_PAD) q=sappend(q, "STICK + A - BACK KEEPS IT");
          else if (hint_dev()==DEV_TOUCH) q=sappend(q, "TAP A ROW");
          else { q=sappend(q, "ARROWS + "); q=sappend(q, key_name(G.bind[ACT_FIRE])); q=sappend(q, " - ESC KEEPS IT"); }
          *q=0; draw_text_center(px+pw/2, py+ph-16, fb, 1, COL(150,160,185)); }
    }
}
static void war_draw_bar_btn(BwSim *S, int bx, int by, int act, int cost, const char *name, int hl){
    int afford = S->war_bits >= cost;
    if ((act==0 || act==4 || act==5 || act==8 || act==9 || act==12) && S->war_selstruct>=0 && S->war_qlen[S->war_selstruct]>=9) afford = 0;   // a full queue greys its train buttons out (addendum 2)
    u32 fill = afford? COL(38,44,60) : COL(26,26,34);
    u32 edge = afford? COL(198,164,84) : COL(70,66,84);
    if (act>=20 && act<=22 && act-20==S->war_bcat){ fill = COL(56,64,88); edge = COL(255,220,120); }   // the open category, lit
    if (act==13){   // v1.21 cosmetic: DEMOLISH wears a warning red — and once ARMED, its edge pulses so the state can't be missed
        int armed = S->war_selstruct>=0 && S->war_demoarm==S->war_selstruct;
        edge = armed? (((int)(G.anim_t*5.0f)&1)? COL(255,70,55) : COL(255,170,60)) : COL(205,95,80);
        if (armed) fill = COL(64,34,34);
    }
    rect_fill(bx, by+6, 84, 34, fill);
    rect_line(bx, by+6, 84, 34, edge);
    draw_text_center(bx+42, by+11, name, 1, afford? COL(240,244,250) : COL(130,126,150));
    if (cost>0){
        char cb[16]; char *q=cb; q=sapp_int(q, cost); q=sappend(q, " BITS"); *q=0;
        draw_text_center(bx+42, by+24, cb, 1, afford? COL(255,205,40) : COL(120,110,90));
    } else if (act>=20 && act<=22) draw_text_center(bx+42, by+24, act-20==S->war_bcat? "OPEN" : "PICK", 1, COL(160,170,190));
    else if (act==13 && S->war_selstruct>=0){   // v1.21: DEMOLISH advertises its refund, not "FREE"
        char cb[16]; char *q=cb; q=sappend(q, "+"); q=sapp_int(q, WAR_BUILD_COST[S->war_stype[S->war_selstruct]]/2); q=sappend(q, " BITS"); *q=0;
        draw_text_center(bx+42, by+24, cb, 1, COL(255,205,40));
    }
    else draw_text_center(bx+42, by+24, act==6? "STOP" : "FREE", 1, COL(160,170,190));
    // a train button carries the building's QUEUE (addendum 2): the
    // front item's progress tick plus the depth in the corner
    if ((act==0 || act==4 || act==5 || act==8 || act==9 || act==12) && S->war_selstruct>=0 && S->war_qlen[S->war_selstruct]>0){
        int ut = S->war_queue[S->war_selstruct][0];
        float f = 1.0f - S->war_qt[S->war_selstruct] / WAR_TRAIN_TIME[ut];
        if (f<0) f=0; if (f>1) f=1;
        rect_fill(bx+4, by+36, 76, 3, COL(20,18,30));
        rect_fill(bx+4, by+36, (int)(76*f), 3, COL(120,230,90));
        char qb[8]; char *q=qb; *q++='x'; q=sapp_int(q, S->war_qlen[S->war_selstruct]); *q=0;
        draw_text(bx+84-text_w(qb,1)-2, by+8, qb, 1, COL(160,255,190));
    }
    if (hl){   // the pad's LB/RB highlight (v1.16): a bright double frame — A presses THIS button
        rect_line(bx-2, by+4, 88, 38, COL(255,240,160));
        rect_line(bx, by+6, 84, 34, COL(255,240,160));
    }
}
static void war_render_bar(int pi, int vx, int vy, int vw2, int vh2){
    BwSim *S = &BWS[pi];
    int by = vy + vh2 - WAR_BAR_H;
    rect_fill(vx, by, vw2, WAR_BAR_H, COL(14,13,22));
    rect_fill(vx, by, vw2, 2, COL(52,48,74));
    rect_fill(vx, by+2, vw2, 1, COL(120,100,50));   // v1.21 cosmetic: a brass pinstripe under the panel's top edge (styling only — the bar's geometry is untouched)
    int acts[5], costs[5]; const char *names[5];
    int n = war_bar_list(S, acts, costs, names);
    if (S->war_bcat>=0 && S->war_place<0){
        // two rows (addendum 1): the open category's options in the
        // strip directly above the bar, the categories in the bar
        // itself. The pad highlight walks the OPTIONS (the current
        // row), so it is drawn up there; WAR_BAR_H never changes.
        // v1.20: under bar capture the highlight may walk the
        // category row instead — each row draws it only when it is
        // the walked row (war_barhl_row); outside capture the row
        // is always 0 and this renders exactly as before.
        int sy2 = by - WAR_SUB_H;
        rect_fill(vx, sy2, vw2, WAR_SUB_H, COL(18,17,28));
        rect_fill(vx, sy2, vw2, 2, COL(52,48,74));
        rect_fill(vx, sy2+2, vw2, 1, COL(120,100,50));   // v1.21 cosmetic: the strip wears the same pinstripe
        for (int i=0;i<n;i++) war_draw_bar_btn(S, vx + 6 + i*90, sy2, acts[i], costs[i], names[i], i==S->war_barhl && S->war_barhl_row==0);
        { const char *cl = S->war_bcat==0? "TRAIN" : S->war_bcat==1? "BUILD" : "DEFENSE";
          int ix = vx + 6 + n*90 + 10;
          if (ix + text_w(cl,1) < vx+vw2) draw_text(ix, sy2+17, cl, 1, COL(255,220,120)); }
        int ca[5], cc[5]; const char *cn[5];
        int cn2 = war_bar_cats(S, ca, cc, cn);
        for (int i=0;i<cn2;i++) war_draw_bar_btn(S, vx + 6 + i*90, by, ca[i], cc[i], cn[i], i==S->war_barhl && S->war_barhl_row==1);
    } else {
        for (int i=0;i<n;i++) war_draw_bar_btn(S, vx + 6 + i*90, by, acts[i], costs[i], names[i], i==S->war_barhl && S->war_barhl_row==0);
    }
    if (vw2 >= VW){   // the info zone only earns its keep at full width
        int ix = vx + 6 + n*90 + 10;
        rect_fill(ix-6, by+7, 1, WAR_BAR_H-14, COL(52,48,74));   // v1.21 cosmetic: a divider between the buttons and the readout
        if (S->war_place>=0) draw_text(ix, by+17, hint_dev()==DEV_PAD? "A PLACES IT WHERE THE CURSOR SITS" : (hint_dev()==DEV_TOUCH? "TAP GROUND TO PLACE IT" : "CLICK THE GROUND TO PLACE IT"), 1, COL(255,220,150));
        else if (S->war_selstruct>=0 && war_struct_alive(S, S->war_selstruct)){
            int s = S->war_selstruct;
            static const char *SN[7] = { "COMPILER CORE", "PATCH BAY", "ZAPPER TOWER", "FIREWALL", "BUG HIVE", "ASSEMBLY PLANT", "GATE" };
            char buf[56]; char *q=buf; q=sappend(q, SN[S->war_stype[s]]); q=sappend(q, "  ");
            q=sapp_int(q, (int)S->war_shp[s]); q=sappend(q, "/"); q=sapp_int(q, S->war_smaxhp[s]);
            if (S->war_qlen[s]>0){ q=sappend(q, "  TRAINING x"); q=sapp_int(q, S->war_qlen[s]); }
            *q=0;
            draw_text(ix, by+17, buf, 1, COL(200,214,220));
        }
        else if (war_sel_count(S)>0){
            char buf[64]; char *q=buf; q=sapp_int(q, war_sel_count(S));
            q=sappend(q, hint_dev()==DEV_PAD? " SELECTED - A ON GROUND: MOVE, A ON A BUG: ATTACK" : (hint_dev()==DEV_TOUCH? " SELECTED - TAP GROUND: MOVE, TAP A BUG: ATTACK" : " SELECTED - CLICK GROUND: MOVE, CLICK A BUG: ATTACK")); *q=0;
            draw_text(ix, by+17, buf, 1, COL(200,230,200));
        }
        else draw_text(ix, by+17, hint_dev()==DEV_PAD? "A ON A BUDDY OR A BUILDING" : (hint_dev()==DEV_TOUCH? "TAP A BUDDY OR A BUILDING" : "CLICK A BUDDY OR A BUILDING"), 1, COL(150,160,185));
    }
}
static void war_draw_struct(BwSim *S, int s){
    float x = S->war_sx[s], y = S->war_sy[s];
    int type = S->war_stype[s];
    int sx = SX(x), sy = SY(y);
    int rad = WAR_SRAD[type];
    ellipse_fill(sx, sy + rad*3/4, rad, rad/3, COL(16,14,24));
    if (S->war_selstruct == s)   // a selection disc BEHIND the body: a green rim (circle_out fills, so it must go first)
        disc_fill(sx, sy, rad + 5 + (int)(fsin(G.anim_t*5.0f)*1.5f), COL(140,255,140));
    if (type==WAR_S_HQ){
        rect_fill(sx-30, sy-26, 60, 52, COL(52,58,78)); rect_line(sx-30, sy-26, 60, 52, COL(120,140,180));
        rect_fill(sx-30, sy-26, 60, 8, COL(70,80,110));
        int pr = 10 + (int)(fsin(G.anim_t*2.4f)*1.5f);
        war_blend_disc(sx, sy+2, pr+11, COL(110,235,255), 34);   // v1.21 cosmetic: the Core's soft halo
        disc_fill(sx, sy+2, pr+4, COL(30,90,110));
        disc_fill(sx, sy+2, pr, COL(110,235,255));
        disc_fill(sx-3, sy-1, 3, COL(240,255,255));
        rect_fill(sx-2, sy-40, 3, 14, COL(120,140,180));
        disc_fill(sx, sy-42, 3, COL(255,120,90));
        draw_text_center(sx, sy+32, "COMPILER CORE", 1, COL(200,230,255));
    } else if (type==WAR_S_PATCH){
        rect_fill(sx-24, sy-16, 48, 34, COL(88,98,118)); rect_line(sx-24, sy-16, 48, 34, COL(160,170,200));
        rect_fill(sx-24, sy-16, 48, 7, COL(240,170,60));
        line_thick(sx-8, sy+8, sx+6, sy-6, 3, COL(220,226,235));
        disc_fill(sx+9, sy-9, 4, COL(220,226,235));
        // v1.21 cosmetic: the work light — blinking green while the queue cooks, a dim ember while idle
        disc_fill(sx+19, sy-12, 2, S->war_qlen[s]>0? (((int)(G.anim_t*2.0f)&1)? COL(140,255,160) : COL(60,140,80)) : COL(52,72,58));
        draw_text_center(sx, sy+24, "PATCH BAY", 1, COL(220,228,240));
    } else if (type==WAR_S_ZAP){
        rect_fill(sx-11, sy+2, 22, 10, COL(70,76,96)); rect_line(sx-11, sy+2, 22, 10, COL(140,150,180));
        rect_fill(sx-2, sy-16, 4, 18, COL(120,130,160));
        u32 orb = S->war_scd[s] > 0.4f? COL(255,255,255) : COL(120,220,255);
        war_blend_disc(sx, sy-20, 11 + (int)(fsin(G.anim_t*3.0f + (float)s)*1.5f), COL(120,220,255), S->war_scd[s] > 0.4f? 64 : 30);   // v1.21 cosmetic: the charge glow, hottest just after a shot
        disc_fill(sx, sy-20, 7, orb); disc_fill(sx, sy-20, 3, COL(230,250,255));
        draw_text_center(sx, sy+18, "ZAPPER", 1, COL(200,230,255));
    } else if (type==WAR_S_WALL){
        // segments on the 24u grid join into ONE barrier (Daniel,
        // Nov 16): a centre tile plus an arm toward every friendly
        // firewall segment a grid step away, so runs, columns and
        // corners all read as a single wall.
        int armE=0, armW=0, armN=0, armS=0;
        for (int o=0;o<WAR_MAXS;o++){
            if (o==s || S->war_shp[o]<=0 || S->war_stype[o]!=WAR_S_WALL) continue;
            float dx=S->war_sx[o]-x, dy=S->war_sy[o]-y;
            if (dy>-4.0f && dy<4.0f){ if (dx>20.0f && dx<28.0f) armE=1; else if (dx<-20.0f && dx>-28.0f) armW=1; }
            else if (dx>-4.0f && dx<4.0f){ if (dy>20.0f && dy<28.0f) armS=1; else if (dy<-20.0f && dy>-28.0f) armN=1; }
        }
        if (armW) rect_fill(sx-24, sy-7, 24, 14, COL(196,92,44));
        if (armE) rect_fill(sx, sy-7, 24, 14, COL(196,92,44));
        if (armN) rect_fill(sx-7, sy-24, 14, 24, COL(196,92,44));
        if (armS) rect_fill(sx-7, sy, 14, 24, COL(196,92,44));
        rect_fill(sx-13, sy-7, 26, 14, COL(196,92,44)); rect_line(sx-13, sy-7, 26, 14, COL(255,170,80));
        for (int f=0; f<3; f++){
            int fx = sx-11 + f*10, fh = 5 + (int)(fsin(G.anim_t*9.0f + (float)(s*7+f*3))*2.0f);
            tri_fill(fx-4, sy-7, fx+4, sy-7, fx, sy-7-fh, COL(255,170,60));
        }
        if (!armE && !armW && !armN && !armS) draw_text_center(sx, sy+14, "FIREWALL", 1, COL(255,200,150));
    } else if (type==WAR_S_GATE){
        // the gate (addendum 1): two gold posts and the energy panel
        // between them. The panel brightens while any of YOUR units
        // stands close — the door is open — and dims when the field
        // is clear. Bugs only ever see it shut.
        int open = 0;
        for (int i=0;i<WAR_MAXU;i++){
            if (S->war_uhp[i]<=0 || war_u_side(S->war_utype[i])!=0) continue;
            float dx=S->war_ux[i]-x, dy=S->war_uy[i]-y;
            if (dx*dx+dy*dy < 30.0f*30.0f){ open=1; break; }
        }
        blend_rect(sx-11, sy-9, 22, 18, COL(255,205,80), open? 170 : 90);
        rect_fill(sx-13, sy-11, 4, 22, COL(120,90,20)); rect_line(sx-13, sy-11, 4, 22, COL(255,205,80));
        rect_fill(sx+9, sy-11, 4, 22, COL(120,90,20)); rect_line(sx+9, sy-11, 4, 22, COL(255,205,80));
        disc_fill(sx-11, sy-13, 3, COL(255,205,80)); disc_fill(sx+11, sy-13, 3, COL(255,205,80));
        draw_text_center(sx, sy+16, "GATE", 1, COL(255,225,150));
    } else if (type==WAR_S_ASM){
        rect_fill(sx-26, sy-14, 52, 32, COL(96,88,110)); rect_line(sx-26, sy-14, 52, 32, COL(170,160,190));
        rect_fill(sx-26, sy-14, 52, 7, COL(90,160,220));
        rect_fill(sx+14, sy-26, 8, 12, COL(70,64,88)); rect_line(sx+14, sy-26, 8, 12, COL(140,132,160));
        disc_fill(sx-8, sy+3, 8, COL(52,48,66)); disc_fill(sx-8, sy+3, 4, COL(200,205,220));
        line_thick(sx-8, sy-5, sx-8, sy+11, 2, COL(200,205,220)); line_thick(sx-16, sy+3, sx, sy+3, 2, COL(200,205,220));
        rect_fill(sx+6, sy-2, 12, 10, COL(255,205,40)); rect_line(sx+6, sy-2, 12, 10, COL(120,90,20));
        // v1.21 cosmetic: the work light — amber and pulsing while the plant is building, dark when it isn't
        if (S->war_qlen[s]>0) disc_fill(sx+18, sy-20, 2, ((int)(G.anim_t*3.0f)&1)? COL(255,190,80) : COL(150,100,40));
        else disc_fill(sx+18, sy-20, 2, COL(70,58,40));
        draw_text_center(sx, sy+26, "ASSEMBLY", 1, COL(220,228,240));
    } else {   // the HIVE
        int strain = REGION_BUG[S->war_region]>=0? REGION_BUG[S->war_region] : 0;
        u32 hc = col_shade(BUG_COL[strain], 0.8f);
        int breathe = (int)(fsin(G.anim_t*1.8f + (float)s*2.0f)*1.5f);   // v1.21 cosmetic: the hive breathes
        ellipse_fill(sx, sy+6, 30 + breathe, 20, hc);
        ellipse_fill(sx, sy-4, 22, 16, col_shade(BUG_COL[strain], 0.95f));
        for (int k=0;k<5;k++){
            int kx = sx-20 + k*10;
            tri_fill(kx-4, sy-14, kx+4, sy-14, kx, sy-26-((k%2)? 4:0), col_shade(BUG_COL[strain], 0.6f));
        }
        int pr = 6 + (int)(fsin(G.anim_t*3.0f + (float)s)*1.5f);
        disc_fill(sx, sy-2, pr, BUG_BELLY[strain]);
        disc_fill(sx-9, sy-8, 3, COL(24,16,30)); disc_fill(sx+9, sy-8, 3, COL(24,16,30));
        draw_text_center(sx, sy+32, "BUG HIVE", 1, COL(255,190,170));
    }
    if (S->war_sflash[s] > 0)   // v1.21 cosmetic: the hit flash
        war_blend_disc(sx, sy, rad+3, COL(255,236,220), (int)(115.0f * (S->war_sflash[s]/0.12f)));
    if (S->war_shp[s] < S->war_smaxhp[s])
        war_hp_bar(sx - rad, sy - rad - 12, rad*2, S->war_shp[s] / (float)S->war_smaxhp[s], S->war_sside[s]);
    if (type==WAR_S_HIVE){   // v1.21: the hive's purse, printed under its name — you can watch the enemy economy breathe (this draw is already fog-gated by the caller)
        char pb[24]; char *q=pb; q=sapp_int(q, S->war_hbits[s]); q=sappend(q, " BITS"); *q=0;
        draw_text_center(sx, sy+44, pb, 1, COL(255,205,40));
    }
}
static void war_draw_unit(BwSim *S, Buddy *b, int i){
    float x = S->war_ux[i], y = S->war_uy[i];
    int type = S->war_utype[i];
    int sx = SX(x), sy = SY(y);
    int veh = (type==WAR_U_SCOUT || type==WAR_U_SIEGE);
    if (S->war_usel[i]) circle_out(sx, sy+4, type==WAR_U_SIEGE? 20 : type==WAR_U_SCOUT? 15 : 12, COL(140,255,140));
    war_blend_disc(sx, sy+6, veh? (type==WAR_U_SIEGE? 19:15) : 11, COL(10,8,18), 75);   // v1.21 cosmetic: a soft drop shadow under every unit
    ellipse_fill(sx, sy+6, veh? (type==WAR_U_SIEGE? 17:13) : 8, 3, COL(16,14,24));
    if (war_u_side(type)==0){
        u32 coat = bud_coat(b, WAR_SPCOL[b->species % BUDN]);
        int bob = (S->war_ustate[i]!=0 || S->war_urepair[i]!=0)? ((int)(G.anim_t*9.0f + (float)i*1.7f) & 1) : 0;   // repair work bobs too — a patcher under autoseek has no move state set
        if (type==WAR_U_SCOUT){
            // the scout buggy: light frame, four wheels, the driver's
            // head up top and a little pop-gun out front
            disc_fill(sx-8, sy+5, 3, COL(24,22,30)); disc_fill(sx+8, sy+5, 3, COL(24,22,30));
            disc_fill(sx-8, sy-5, 3, COL(24,22,30)); disc_fill(sx+8, sy-5, 3, COL(24,22,30));
            rect_fill(sx-11, sy-5, 22, 11, col_shade(coat, 0.9f)); rect_line(sx-11, sy-5, 22, 11, col_shade(coat, 0.6f));
            disc_fill(sx-2, sy-3-bob, 5, coat);
            disc_fill(sx-3, sy-4-bob, 1, COL(24,20,28)); disc_fill(sx, sy-4-bob, 1, COL(24,20,28));
            line_thick(sx+8, sy-1, sx+15, sy-1, 2, COL(220,226,235));
        } else if (type==WAR_U_SIEGE){
            // the siege tank: treads, a heavy hull, turret + long gun,
            // and the driver peeking out of the hatch
            rect_fill(sx-17, sy+3, 34, 6, COL(30,28,38)); rect_fill(sx-17, sy-10, 34, 6, COL(30,28,38));
            for (int w=0;w<4;w++){ disc_fill(sx-13+w*9, sy+6, 2, COL(90,86,100)); disc_fill(sx-13+w*9, sy-7, 2, COL(90,86,100)); }
            rect_fill(sx-15, sy-7, 30, 14, col_shade(coat, 0.85f)); rect_line(sx-15, sy-7, 30, 14, col_shade(coat, 0.55f));
            disc_fill(sx-3, sy-1, 7, col_shade(coat, 0.7f)); disc_fill(sx-3, sy-1, 4, col_shade(coat, 0.5f));
            line_thick(sx+2, sy-1, sx+21, sy-1, 3, COL(52,48,66));
            disc_fill(sx-10, sy-6-bob, 4, coat);
            disc_fill(sx-11, sy-7-bob, 1, COL(24,20,28)); disc_fill(sx-8, sy-7-bob, 1, COL(24,20,28));
        } else {
        disc_fill(sx, sy - bob, 8, coat);
        disc_fill(sx-3, sy-3-bob, 2, COL(24,20,28)); disc_fill(sx+3, sy-3-bob, 2, COL(24,20,28));
        if (type==WAR_U_HARV){
            rect_fill(sx-6, sy-10-bob, 12, 4, COL(255,205,40));   // the hard hat
            if (S->war_ucarry[i]>0) disc_fill(sx+8, sy+2-bob, 3, COL(255,205,40));
            for (int p=0;p<3;p++)   // v1.21 cosmetic: load pips — one lit pip per third of a full load, so a loaded harvester reads at a glance
                if (S->war_ucarry[i] > p*10) disc_fill(sx-9, sy+3-bob-p*4, 1, COL(255,205,40));
        } else if (type==WAR_U_MELEE){
            line_thick(sx+5, sy+4-bob, sx+11, sy-4-bob, 2, COL(220,226,235));   // the wrench
            disc_fill(sx+12, sy-5-bob, 2, COL(220,226,235));
        } else if (type==WAR_U_PATCH){
            // v1.21 (Daniel): a FIELD MEDIC, not a nurse — an olive
            // patrol helmet carrying the red cross on a white disc,
            // and a medkit satchel on its back with a cross of its
            // own. Draw code only; the patcher underneath is untouched.
            rect_fill(sx-11, sy-1-bob, 5, 8, COL(240,244,250)); rect_line(sx-11, sy-1-bob, 5, 8, COL(150,60,55));   // the medkit satchel
            rect_fill(sx-10, sy+2-bob, 3, 2, COL(255,90,80));
            rect_fill(sx-9, sy+1-bob, 1, 4, COL(255,90,80));
            ellipse_fill(sx, sy-8-bob, 8, 4, COL(104,116,72));   // the helmet
            disc_fill(sx, sy-8-bob, 3, COL(240,244,250));
            rect_fill(sx-1, sy-10-bob, 2, 5, COL(255,90,80));
            rect_fill(sx-2, sy-9-bob, 4, 2, COL(255,90,80));
        } else {
            disc_fill(sx, sy-9-bob, 3, COL(120,235,255));   // the bolt buddy's charged cap
            rect_fill(sx-1, sy-14-bob, 2, 4, COL(120,235,255));
        }
        }
    } else {
        int strain = REGION_BUG[S->war_region]>=0? REGION_BUG[S->war_region] : (i % 4);
        int legph = (int)(G.anim_t*11.0f + (float)i*2.1f) & 1;
        for (int l=0;l<3;l++){
            int lx = sx-6 + l*6;
            line_thick(lx, sy+3, lx-2, sy+8, 1, col_shade(BUG_COL[strain], 0.55f));
            line_thick(lx, sy+3, lx+3, sy+8 - (legph? 2:0), 1, col_shade(BUG_COL[strain], 0.55f));
        }
        ellipse_fill(sx, sy, type==WAR_U_SPIT? 10:9, 7, BUG_COL[strain]);
        ellipse_fill(sx, sy+2, 6, 4, type==WAR_U_BHARV? COL(168,124,52) : BUG_BELLY[strain]);   // v1.21 cosmetic: the bug harvester's bronze work-belly sets it apart from the fighters
        if (type==WAR_U_SPIT) rect_fill(sx+6, sy-2, 6, 4, col_shade(BUG_COL[strain], 0.8f));
        if (type==WAR_U_BHARV){   // v1.21: the bug harvester wears a gold-rimmed bit pack — the same gold your harvester hauls, and the same gold dot when it's carrying
            rect_fill(sx-5, sy-8, 10, 5, COL(150,110,40)); rect_line(sx-5, sy-8, 10, 5, COL(255,205,40));
            if (S->war_ucarry[i]>0) disc_fill(sx+8, sy+2, 3, COL(255,205,40));
            for (int p=0;p<3;p++)   // load pips beside the pack, same language as your harvester's
                if (S->war_ucarry[i] > p*10) disc_fill(sx+9, sy-6+p*4, 1, COL(255,205,40));
        }
        disc_fill(sx-3, sy-3, 2, COL(255,250,240)); disc_fill(sx+3, sy-3, 2, COL(255,250,240));
        disc_fill(sx-3, sy-3, 1, COL(16,12,20)); disc_fill(sx+3, sy-3, 1, COL(16,12,20));
    }
    if (S->war_uflash[i] > 0)   // v1.21 cosmetic: the hit flash
        war_blend_disc(sx, sy-2, veh? 17:11, COL(255,236,220), (int)(115.0f * (S->war_uflash[i]/0.12f)));
    if (S->war_uhp[i] < S->war_umaxhp[i])
        war_hp_bar(sx - (veh? 13:9), sy - (type==WAR_U_SIEGE? 22:16), veh? 26:18, S->war_uhp[i] / (float)S->war_umaxhp[i], war_u_side(type));
}
static void war_blend_disc(int cx, int cy, int r, u32 c, int alpha){
    for (int dy=-r; dy<=r; dy++){ int dx=(int)fsqrt((float)(r*r-dy*dy)); blend_rect(cx-dx, cy+dy, dx*2, 1, c, alpha); }
}
static void war_draw_terrain(BwSim *S, int t){
    float x = S->war_tx[t], y = S->war_ty[t];
    int sx = SX(x), sy = SY(y), kind = S->war_ttype[t], rad = S->war_trad[t];
    u32 vary = hash2(t, 11, 0x7E44u);
    if (S->war_thp[t]<=0){   // destroyed: only the fading scorch remains
        if (S->war_tdeadt[t]>0){
            ellipse_fill(sx, sy + rad*3/4, rad, rad/3, COL(28,24,22));
            disc_fill(sx - rad/3, sy + rad/2, 2, COL(44,38,32));
            disc_fill(sx + rad/4, sy + rad*2/3, 2, COL(38,32,28));
        }
        return;
    }
    ellipse_fill(sx, sy + rad*3/4, rad, rad/3, COL(16,14,24));
    if (kind==0){   // a tree: trunk, then a two-tone canopy in the region's tint
        u32 can = WAR_TREE_COL[S->war_region];
        rect_fill(sx-2, sy, 4, rad, COL(90,64,40));
        disc_fill(sx, sy - rad/2, rad + 2, col_shade(can, 0.75f));
        disc_fill(sx - rad/3, sy - rad/2 - 3, rad - 2, can);
        disc_fill(sx + rad/4, sy - rad/3, 3, col_shade(can, 1.3f));
    } else if (kind==1){   // rocks: grey lumps
        u32 rk = col_shade(COL(128,124,118), 0.85f + (float)(vary % 30u) * 0.01f);
        ellipse_fill(sx, sy, rad, rad*2/3, rk);
        ellipse_fill(sx - rad/2, sy + 2, rad/2, rad/3, col_shade(rk, 0.8f));
        disc_fill(sx + rad/4, sy - rad/3, 2, col_shade(rk, 1.25f));
    } else if (kind==2){   // a broken terminal, left by whoever debugged here last
        rect_fill(sx-9, sy-8, 18, 15, COL(40,42,54)); rect_line(sx-9, sy-8, 18, 15, COL(90,96,120));
        rect_fill(sx-6, sy-5, 12, 8, COL(24,60,66));
        line_thick(sx-5, sy-4, sx+1, sy+1, 1, COL(120,235,255)); line_thick(sx+1, sy+1, sx+5, sy-3, 1, COL(120,235,255));
        rect_fill(sx-3, sy+7, 6, 3, COL(52,54,68));
    } else if (kind==3){   // a cable spool
        ellipse_fill(sx, sy, rad, rad*3/4, COL(110,82,50));
        ellipse_fill(sx, sy, rad*2/3, rad/2, COL(84,60,36));
        disc_fill(sx, sy, 3, COL(52,40,26));
        line_thick(sx - rad*2/3, sy - rad/4, sx + rad*2/3, sy + rad/4, 2, COL(38,34,48));
    } else if (kind==4){   // rubble
        tri_fill(sx - rad, sy + 4, sx - rad/3, sy - rad/2, sx + 1, sy + 4, COL(104,96,88));
        tri_fill(sx - 2, sy + 4, sx + rad/3, sy - rad/3, sx + rad, sy + 4, COL(88,80,74));
        disc_fill(sx + 2, sy + 1, 2, COL(130,120,108));
    } else {   // kind 5 (v1.18): a barrier slab — dark slate, a lit top edge, one cyan seam. It will outlive us all.
        u32 sl = col_shade(COL(52,58,72), 0.9f + (float)(vary % 20u) * 0.01f);
        rect_fill(sx-rad, sy-rad, rad*2, rad*2, sl);
        rect_fill(sx-rad, sy-rad, rad*2, 3, col_shade(sl, 1.45f));
        rect_fill(sx-1, sy-rad+4, 2, rad*2-8, COL(110,225,235));
        rect_line(sx-rad, sy-rad, rad*2, rad*2, col_shade(sl, 0.6f));
    }
    if (S->war_thp[t] < (float)S->war_tmaxhp[t])   // the house health bar, same as units and buildings
        war_hp_bar(sx - rad, sy - rad - 12, rad*2, S->war_thp[t] / (float)S->war_tmaxhp[t], 0);
}
// the property tint, drawn while a build is in hand: your land glows
// softly, so you can see exactly how far your building rights reach.
// The faint outer ring is the adjacency reach for the build in hand:
// its footprint may poke that far past the zone edge and still count
// as touching your land (the ghost is the exact judge either way).
static void war_render_zones(BwSim *S){
    if (S->war_place<0) return;
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0 || S->war_sside[s]!=0) continue;
        int sx = SX(S->war_sx[s]), sy = SY(S->war_sy[s]);
        int zr = (int)war_zone_rad(S->war_stype[s]);
        int reach = zr + WAR_SRAD[S->war_place];
        war_blend_disc(sx, sy, reach, COL(120,220,255), 12);
        war_blend_disc(sx, sy, zr, COL(120,220,255), 34);
        circle_out(sx, sy, zr, COL(120,220,255));
        circle_out(sx, sy, reach, COL(90,160,190));
    }
}
// the placement ghost: green where the build is legal, red where
// it is not — drawn at a world point, whoever's finger is asking.
static void war_ghost_at(BwSim *S, float wx, float wy){
    int kind = S->war_place;
    if (kind==WAR_S_WALL){ wx = war_snap24(wx, WAR_OX); wy = war_snap24(wy, WAR_OY); }
    int okp = war_place_ok(S, kind, wx, wy);
    int sx = SX(wx), sy = SY(wy), rad = WAR_SRAD[kind];
    int ga = 95 + (int)(fsin(G.anim_t*6.0f)*25.0f);   // v1.21 cosmetic: the ghost breathes, and its footprint is squared so the fit is judgeable
    war_blend_disc(sx, sy, rad, okp? COL(120,255,120) : COL(255,90,80), ga);
    circle_out(sx, sy, rad, okp? COL(140,255,140) : COL(255,110,90));
    rect_line(sx-rad, sy-rad, rad*2, rad*2, okp? COL(140,255,140) : COL(255,110,90));
}
// the placement ghost under your finger. (Touch gives no hover, so
// the ghost lives while a finger or mouse button is down.) The pad
// cursor is a finger too (v1.16): while it is on and no touch point
// is asking, the ghost hovers at the cursor instead, so a pad player
// can judge a placement before pressing A.
static int war_view_pi = -1;   // feat 13: the player whose VIEW war_render is drawing right now (-1 outside a render)
static void war_render_ghost(int pi, int vx, int vy, int vw2, int vh2){
    BwSim *S = &BWS[pi];
    if (S->war_place<0) return;
    if (war_view_pi==G.my_id){   // the touch points belong to the local player — only their own view ghosts them (feat 13: the viewer, not the battle owner)
        for (int i=0;i<6;i++){
            if (!IN.tp[i].active || IN.tp[i].y >= vy+vh2-WAR_BAR_H) continue;
            float wx = S->war_camx + (float)(IN.tp[i].x - vx - vw2/2);
            float wy = S->war_camy + WAR_BAR_H*0.5f + (float)(IN.tp[i].y - vy - vh2/2);
            war_ghost_at(S, wx, wy);
            return;
        }
    }
    if (S->war_curon) war_ghost_at(S, S->war_curx, S->war_cury);
}
static void war_render(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    int vx = G.view_x0, vy = G.view_y0, vw2 = G.view_w>0? G.view_w : VW, vh2 = G.view_h>0? G.view_h : VH;
    if (S->war_state==0){ war_render_map(pi, vx, vy, vw2, vh2); return; }
    if (S->war_state==4){ war_render_setup(pi, vx, vy, vw2, vh2); return; }   // v1.19: the setup panel over the map
    G.zoom = 1.0f;
    G.camx = S->war_camx; G.camy = S->war_camy + WAR_BAR_H*0.5f;
    int rg = S->war_region;
    rect_fill(vx, vy, vw2, vh2, col_shade(REGION_GB[rg], 0.5f));
    { float wx0 = S->war_camx - vw2/2 - 80, wy0 = S->war_camy - vh2/2 - 80;
      float wx1 = S->war_camx + vw2/2 + 80, wy1 = S->war_camy + vh2/2 + 80;
      int gx0 = (int)(wx0/80.0f), gy0 = (int)(wy0/80.0f), gx1 = (int)(wx1/80.0f), gy1 = (int)(wy1/80.0f);
      for (int gy=gy0; gy<=gy1; gy++) for (int gx=gx0; gx<=gx1; gx++){
          float cxp = gx*80.0f+40, cyp = gy*80.0f+40;
          int inside = cxp>WAR_OX && cxp<WAR_OX+S->war_aw && cyp>WAR_OY && cyp<WAR_OY+S->war_ah;
          u32 c = inside? (((gx+gy)&1)? REGION_GA[rg] : REGION_GB[rg]) : col_shade(REGION_GB[rg], 0.45f);
          c = col_shade(c, 0.95f + hash_f(gx, gy, 0xB17u)*0.09f);
          rect_fill(SX(gx*80.0f), SY(gy*80.0f), 81, 81, c);
          if (inside){
              // v1.21 cosmetic: a faint survey grid + speckle, so
              // the ground has grain and movement reads against it
              int csx = SX(gx*80.0f), csy = SY(gy*80.0f);
              rect_fill(csx, csy, 81, 1, col_shade(c, 0.9f)); rect_fill(csx, csy, 1, 81, col_shade(c, 0.9f));
              disc_fill(csx + 12 + (int)(hash2(gx, gy, 21u) % 56), csy + 14 + (int)(hash2(gx, gy, 23u) % 52), 1, col_shade(c, 0.86f));
              disc_fill(csx + 10 + (int)(hash2(gx, gy, 27u) % 60), csy + 12 + (int)(hash2(gx, gy, 29u) % 56), 1, col_shade(c, 1.1f));
              if (hash2(gx, gy, 0x7A55u) % 5 == 0)
                  disc_fill(SX(gx*80.0f + 20 + (int)(hash2(gx, gy, 3u) % 36)), SY(gy*80.0f + 22 + (int)(hash2(gx, gy, 9u) % 34)), 3, REGION_TUFT[rg]);
          }
      }
      // the arena fence
      int ax0 = SX(WAR_OX), ay0 = SY(WAR_OY), ax1 = SX(WAR_OX+S->war_aw), ay1 = SY(WAR_OY+S->war_ah);
      rect_fill(ax0-5, ay0-5, (int)S->war_aw+10, 5, COL(70,44,44)); rect_fill(ax0-5, ay1, (int)S->war_aw+10, 5, COL(70,44,44));
      rect_fill(ax0-5, ay0-5, 5, (int)S->war_ah+10, COL(70,44,44)); rect_fill(ax1, ay0-5, 5, (int)S->war_ah+10, COL(70,44,44));
    }
    war_render_zones(S);
    for (int t=0;t<S->war_tn;t++)   // terrain is scenery: remembered once explored
        if (war_fog_at(S, S->war_tx[t], S->war_ty[t])>=1) war_draw_terrain(S, t);
    for (int n=0;n<S->war_nn;n++){
        if (S->war_namt[n]<=0) continue;
        if (war_fog_at(S, S->war_nx[n], S->war_ny[n])<1) continue;   // an unmapped node is just a rumour
        int sx = SX(S->war_nx[n]), sy = SY(S->war_ny[n]);
        // v1.21 cosmetic: the node is a crystal cluster with a soft
        // breathing glow — and it VISIBLY DEPLETES: the crystals
        // shrink and dim as the bits drain, so both economies read
        // at a glance (full > 500, working > 200, nearly dry below)
        float dsc = S->war_namt[n] > 500? 1.0f : S->war_namt[n] > 200? 0.72f : 0.45f;
        u32 cc1 = col_shade(COL(255,205,40), 0.6f + 0.4f*dsc);
        u32 cc2 = col_shade(COL(255,190,30), 0.6f + 0.4f*dsc);
        u32 cc3 = col_shade(COL(240,180,40), 0.6f + 0.4f*dsc);
        int pulse = (int)(fsin(G.anim_t*2.2f + (float)n*1.3f)*3.0f);
        war_blend_disc(sx, sy, 17 + pulse, COL(255,205,40), 20 + pulse*2);
        ellipse_fill(sx, sy+8, 16, 5, COL(90,74,30));
        tri_fill(sx-12, sy+8, sx-5, sy+8 - (int)(20.0f*dsc), sx+1, sy+8, cc1);
        tri_fill(sx-2, sy+8, sx+4, sy+8 - (int)(24.0f*dsc), sx+10, sy+8, cc2);
        tri_fill(sx+4, sy+8, sx+10, sy+8 - (int)(16.0f*dsc), sx+15, sy+8, cc3);
        disc_fill(sx+3, sy-6, 2, COL(255,250,220));
        { char nb[8]; char *q=nb; q=sapp_int(q, S->war_namt[n]); *q=0;
          draw_text_center(sx, sy+14, nb, 1, COL(255,230,150)); }
    }
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0) continue;
        if (S->war_sside[s]==1 && war_fog_at(S, S->war_sx[s], S->war_sy[s])<1) continue;   // a hive shows once scouted, and stays on the map
        war_draw_struct(S, s);
    }
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0) continue;
        if (war_u_side(S->war_utype[i])==1 && war_fog_at(S, S->war_ux[i], S->war_uy[i])<2) continue;   // bugs only while watched
        war_draw_unit(S, b, i);
    }
    for (int i=0;i<24;i++){   // v1.21 cosmetic: particles — death poofs and harvest sparkles, at the units' own layer
        if (S->war_ft[i]<=0) continue;
        if (war_fog_at(S, S->war_fx[i], S->war_fy[i])<1) continue;
        int sx = SX(S->war_fx[i]), sy = SY(S->war_fy[i]);
        float dur = S->war_fk[i]==2? 0.7f : S->war_fk[i]==1? 0.5f : 0.45f;
        float a = 1.0f - S->war_ft[i]/dur; if (a<0) a=0; if (a>1) a=1;
        if (S->war_fk[i]==1){   // the harvest sparkle: a small gold plus that rises and shrinks
            int arm = (int)(5.0f*(1.0f-a)) + 1, yy = sy - (int)(a*10.0f);
            line_thick(sx-arm, yy, sx+arm, yy, 1, COL(255,220,90));
            line_thick(sx, yy-arm, sx, yy+arm, 1, COL(255,220,90));
            disc_fill(sx, yy, 1, COL(255,250,220));
        } else {   // the poof: a soft expanding smoke disc (bigger for a building)
            int r = (S->war_fk[i]==2? 10 : 5) + (int)(a * (S->war_fk[i]==2? 20.0f : 11.0f));
            war_blend_disc(sx, sy, r, S->war_fk[i]==2? COL(130,120,115) : COL(205,198,190), (int)(120.0f*(1.0f-a)));
            war_blend_disc(sx - r/2, sy - r/3, r/2, COL(90,84,80), (int)(80.0f*(1.0f-a)));
        }
    }
    for (int i=0;i<WAR_MAXP;i++){
        if (S->war_pt[i]<=0) continue;
        if (S->war_pside[i]==1 && war_fog_at(S, S->war_px[i], S->war_py[i])<2) continue;
        int sx = SX(S->war_px[i]), sy = SY(S->war_py[i]);
        { float tx, ty;   // v1.21 cosmetic: a tracer streak behind the shot, aimed by its target
          if (war_ref_alive(S, S->war_ptar[i], &tx, &ty)){
              float dx = tx - S->war_px[i], dy = ty - S->war_py[i], d = fsqrt(dx*dx+dy*dy);
              if (d > 4.0f) line_thick(sx - (int)(dx/d*11.0f), sy - (int)(dy/d*11.0f), sx, sy, 2, S->war_pside[i]==0? COL(70,140,170) : COL(90,150,70));
          } }
        disc_fill(sx, sy, 3, S->war_pside[i]==0? COL(140,240,255) : COL(170,255,120));
        disc_fill(sx, sy, 1, COL(255,255,255));
    }
    // the fog itself: unexplored is solid dark, explored-but-unwatched
    // is dimmed. It sits above the field and below every marker.
    { float wx0 = S->war_camx - vw2/2, wy0 = S->war_camy - vh2/2;
      float wx1 = S->war_camx + vw2/2, wy1 = S->war_camy + vh2/2;
      int cx0 = (int)((wx0 - WAR_OX)/40.0f), cx1 = (int)((wx1 - WAR_OX)/40.0f);
      int cy0 = (int)((wy0 - WAR_OY)/40.0f), cy1 = (int)((wy1 - WAR_OY)/40.0f);
      if (cx0<0) cx0=0; if (cy0<0) cy0=0;
      if (cx1>=S->war_gw) cx1=S->war_gw-1; if (cy1>=S->war_gh) cy1=S->war_gh-1;   // v1.19: fog is drawn only over the live field's grid
      for (int cy=cy0; cy<=cy1; cy++) for (int cx=cx0; cx<=cx1; cx++){
          int f = S->war_fog[cy*WAR_FOG_W + cx];
          if (f==2) continue;
          int sx = SX(WAR_OX + cx*40.0f), sy = SY(WAR_OY + cy*40.0f);
          if (f==0){
              // v1.21 cosmetic: a STEPPED soft edge — an unexplored
              // cell bordering watched (or merely explored) ground
              // fades in two steps instead of ending at a hard
              // tile line
              int near2 = 0, near1 = 0;
              for (int oy=-1; oy<=1; oy++) for (int ox=-1; ox<=1; ox++){
                  int nx = cx+ox, ny = cy+oy;
                  if (nx<0 || ny<0 || nx>=S->war_gw || ny>=S->war_gh) continue;
                  int nf = S->war_fog[ny*WAR_FOG_W + nx];
                  if (nf==2) near2 = 1; else if (nf==1) near1 = 1;
              }
              if (near2) blend_rect(sx, sy, 41, 41, COL(6,6,12), 150);
              else if (near1) blend_rect(sx, sy, 41, 41, COL(6,6,12), 205);
              else rect_fill(sx, sy, 41, 41, COL(6,6,12));
          }
          else blend_rect(sx, sy, 41, 41, COL(4,4,10), 145);
      } }
    for (int i=0;i<WAR_MAXU;i++){   // selected units redraw above the fog, marker disc and all —
        if (S->war_uhp[i]<=0 || !S->war_usel[i]) continue;   // a bare circle_out here would bury the buddy under its own marker
        war_draw_unit(S, b, i);
    }
    war_render_ghost(pi, vx, vy, vw2, vh2);
    if (S->war_curon && S->war_state==1 && !S->war_barcap){   // the pad cursor rides above the fog, like the selected units do (v1.16). v1.20: while the command bar has captured pad control, the field cursor hides — the highlight is the pointer now. feat 13: no pi gate — war_render only ever runs for a local view (the owner's own, or a guest's by exchange), and the loaded view's curon is the VIEWER's cursor
        int csx = SX(S->war_curx), csy = SY(S->war_cury);
        int cr = 10 + (int)(fsin(G.anim_t*5.0f)*1.5f);
        circle_out(csx, csy, cr+3, COL(120,100,40));
        circle_out(csx, csy, cr, COL(255,240,160));
        line_thick(csx-cr-5, csy, csx-cr+2, csy, 2, COL(255,240,160));
        line_thick(csx+cr-2, csy, csx+cr+5, csy, 2, COL(255,240,160));
        line_thick(csx, csy-cr-5, csx, csy-cr+2, 2, COL(255,240,160));
        line_thick(csx, csy+cr-2, csx, csy+cr+5, 2, COL(255,240,160));
        disc_fill(csx, csy, 2, COL(255,240,160));
    }
    if (S->war_ordt > 0){
        int sx = SX(S->war_ordx), sy = SY(S->war_ordy);
        int r = 6 + (int)((0.5f - S->war_ordt) * 14.0f);
        circle_out(sx, sy, r, COL(140,255,140));
        disc_fill(sx, sy, 2, COL(140,255,140));
    }
    if (S->war_selstruct>=0 && war_struct_alive(S, S->war_selstruct) && S->war_rallyx[S->war_selstruct]!=0){
        // the rally flag (addendum 3): while a producer with a rally
        // set is selected, a thin line runs from the building to the
        // flag. Like the order marker, it ignores fog.
        int ss = S->war_selstruct;
        int bx = SX(S->war_sx[ss]), by2 = SY(S->war_sy[ss]);
        int fx = SX(S->war_rallyx[ss]), fy = SY(S->war_rallyy[ss]);
        line_thick(bx, by2, fx, fy, 1, COL(140,255,140));
        line_thick(fx, fy, fx, fy-14, 2, COL(200,220,200));
        tri_fill(fx, fy-14, fx, fy-7, fx+9, fy-10, COL(140,255,140));
        disc_fill(fx, fy, 2, COL(140,255,140));
    }
    // the top strip: bits, region, what's left to kill
    rect_fill(vx, vy, vw2, 20, COL(10,10,18));
    disc_fill(vx+12, vy+10, 5, COL(255,205,40));
    { char buf[64]; char *q=buf; q=sappend(q, "BITS "); q=sapp_int(q, S->war_bits); *q=0;
      draw_text(vx+22, vy+6, buf, 1, COL(255,230,150)); }
    draw_text_center(vx+vw2/2, vy+6, S->war_skirmish? "SKIRMISH" : REGION_NAME[rg], 1, COL(240,244,250));
    { char buf[48]; char *q=buf; q=sappend(q, "HIVES "); q=sapp_int(q, war_hives_alive(S));
      q=sappend(q, "   ARMY "); q=sapp_int(q, war_count_units(S, 0)); q=sappend(q, "/24"); *q=0;
      draw_text(vx+vw2-text_w(buf, 1)-8, vy+6, buf, 1, COL(255,190,170)); }
    if (S->war_msgt > 0){
        int tw = text_w(S->war_msg, 1);
        rect_fill(vx+vw2/2 - tw/2 - 6, vy+25, tw+12, 15, COL(16,14,24));
        rect_line(vx+vw2/2 - tw/2 - 6, vy+25, tw+12, 15, COL(198,164,84));   // v1.21 cosmetic: the banner wears a gold frame
        draw_text_center(vx+vw2/2, vy+28, S->war_msg, 1, COL(255,240,200));
    }
    war_render_bar(pi, vx, vy, vw2, vh2);
    if (S->war_state==2 || S->war_state==3){
        blend_rect(vx, vy, vw2, vh2, COL(8,8,14), 110);
        int pw = vw2<420? vw2-24 : 396, ph = 108;
        int px = vx+(vw2-pw)/2, py = vy+(vh2-ph)/2 - 20;
        rect_fill(px, py, pw, ph, COL(16,18,28)); rect_line(px, py, pw, ph, S->war_state==2? COL(255,205,40) : COL(255,110,90));
        if (S->war_state==2){
            draw_text_center(px+pw/2, py+16, S->war_skirmish? "SKIRMISH WON!" : "REGION CONQUERED!", 2, COL(255,220,120));
            draw_text_center(px+pw/2, py+44, S->war_skirmish? "SKIRMISH" : REGION_NAME[rg], 1, COL(240,244,250));
            draw_text_center(px+pw/2, py+62, S->war_skirmish? "THE FIELD IS YOURS... THIS TIME." : "THE SWARM RETREATS... FOR NOW.", 1, COL(200,220,200));
        } else {
            draw_text_center(px+pw/2, py+16, "THE CORE HAS FALLEN", 2, COL(255,140,120));
            draw_text_center(px+pw/2, py+44, S->war_skirmish? "SKIRMISH" : REGION_NAME[rg], 1, COL(240,244,250));
            draw_text_center(px+pw/2, py+62, S->war_skirmish? "THE FIELD KEEPS ITS SECRETS. TRY AGAIN!" : "THE REGION STAYS WILD. REBUILD AND TRY AGAIN!", 1, COL(220,200,200));
        }
        { char b2[48]; char *q=b2; q=sappend(q, lbla(H_POINT)); q=sappend(q, " TO RETURN TO THE MAP"); *q=0;
          draw_text_center(px+pw/2, py+ph-16, b2, 1, COL(150,160,185)); }
    }
    if (S->war_pause){   // the pause menu (v1.17): same rects the tap handler and pad use
        blend_rect(vx, vy, vw2, vh2, COL(8,8,14), 150);
        int px, py, pw, ph; war_pause_rects(vx, vy, vw2, vh2, &px, &py, &pw, &ph);
        rect_fill(px, py, pw, ph, COL(16,18,28)); rect_line(px, py, pw, ph, COL(255,205,40));
        draw_text_center(px+pw/2, py+14, "PAUSED", 2, COL(255,220,120));
        { char buf[48]; char *q=buf; q=sappend(q, S->war_skirmish? "SKIRMISH" : REGION_NAME[rg]); q=sappend(q, " - THE BATTLE WAITS"); *q=0;
          draw_text_center(px+pw/2, py+36, buf, 1, COL(200,210,230)); }
        const char *PLBL[3] = { "RESUME", "RESTART BATTLE", "SAVE & QUIT" };   // v1.29: a skirmish parks in its own slot now — the third choice saves for both battle kinds
        for (int n=0;n<3;n++){
            int by = war_pause_btn_y(py, n);
            rect_fill(px+40, by, pw-80, 36, COL(38,44,60));
            rect_line(px+40, by, pw-80, 36, COL(198,164,84));
            draw_text_center(px+pw/2, by+12, PLBL[n], 2, COL(240,244,250));
        }
        draw_text_center(px+pw/2, py+ph-16, hint_dev()==DEV_PAD? "A - PICK A CHOICE - B RESUMES" : (hint_dev()==DEV_TOUCH? "TAP A CHOICE" : "KEYS 1 / 2 / 3 - ESC RESUMES"), 1, COL(150,160,185));
    }
}

