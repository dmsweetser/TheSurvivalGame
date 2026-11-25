// THE SURVIVAL GAME — war/touch.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 3453-3605 in the Nov 2026 fine-grained refactor; code unchanged.

// --- touch controls inside Banzai World (Daniel, Nov 5) ----------------
// The main game's HUD drew the stick and buttons; the minigames never
// did, so on a phone they were invisible — and an invisible control
// scheme reads as a broken one. Same fixed spots as the meadow, with
// the button captions naming what they DO in this mode.
static void bw_draw_touch(int pi){
    if (pi!=G.my_id || !mobile_ui() || pad_mode()) return;
    Buddy *b=&G.bud[pi]; BwSim *S=&BWS[pi];
    if (S->bat.active) return;                    // battles run on taps + menus
    if (b->bw_mode==8) return;                    // WAR! is tap-only: its command bar owns the bottom strip
    int vw2 = G.view_w>0? G.view_w : VW;
    if (vw2<VW) return;                           // split view: fixed buttons are full-screen furniture
    // v1.31 (audit P3/F19): the deck captions ARE the prompt names —
    // lbl2()'s touch words for the same buttons (FIRE / USE / WALL)
    // in every mode and both schemes, so a prompt and the button it
    // names can never disagree again. Button PRESENCE per mode is
    // unchanged (Run has no USE and no WALL; Tank has no USE).
    // the joystick deck (v1.33: the only touch scheme — the DRAG
    // branch that drew a lone WALL button, plus its v1.31 Tank
    // carve-out, is gone whole; Tank gets this deck's FIRE + WALL-
    // position weapon switch exactly as the stick deck drew it):
    // FIRE + USE + WALL (presence per mode as before), plus the
    // stick itself
    int have_use = 1, have_wall = 1;
    if (b->bw_mode==3){ have_use=0; have_wall=0; }
    else if (b->bw_mode==5){ have_use=0; }
    if (!S->craft_menu){
        tbtn_base(TP_FIRE_CX, TP_FIRE_CY, 34, (IN.tp_held&32)? COL(224,88,62):COL(158,54,46));
        { u32 lc = COL(255,240,230); int gy = TP_FIRE_CY-6;
          rect_fill(TP_FIRE_CX-1, gy-10, 3, 6, lc); rect_fill(TP_FIRE_CX-1, gy+5, 3, 6, lc);
          rect_fill(TP_FIRE_CX-10, gy-1, 6, 3, lc); rect_fill(TP_FIRE_CX+5, gy-1, 6, 3, lc);
          disc_fill(TP_FIRE_CX, gy, 2, lc); }
        draw_text(TP_FIRE_CX-text_w("FIRE",1)/2, TP_FIRE_CY+14, "FIRE", 1, COL(255,240,230));
        if (have_use){
            tbtn_base(TP_ACT_CX, TP_ACT_CY, 22, COL(56,92,62));
            { u32 lc = COL(230,255,235); int gy = TP_ACT_CY-3;
              tri_fill(TP_ACT_CX, gy-8, TP_ACT_CX-6, gy+1, TP_ACT_CX+6, gy+1, lc);
              rect_fill(TP_ACT_CX-2, gy+1, 5, 5, lc); }
            draw_text(TP_ACT_CX-text_w("USE",1)/2, TP_ACT_CY+9, "USE", 1, COL(230,255,235));
        }
        if (have_wall){
            tbtn_base(TP_WALL_CX, TP_WALL_CY, 24, COL(62,56,86));
            if (b->bw_mode==5) draw_text(TP_WALL_CX-text_w("WALL",1)/2, TP_WALL_CY-4, "WALL", 1, COL(255,244,200));
            else { u32 br = COL(232,186,108);
              rect_fill(TP_WALL_CX-10, TP_WALL_CY-11, 9, 4, br); rect_fill(TP_WALL_CX+1, TP_WALL_CY-11, 9, 4, br);
              rect_fill(TP_WALL_CX-10, TP_WALL_CY-6, 6, 4, br);  rect_fill(TP_WALL_CX-3, TP_WALL_CY-6, 6, 4, br);
              rect_fill(TP_WALL_CX+4, TP_WALL_CY-6, 6, 4, br);
              draw_text(TP_WALL_CX-text_w("WALL",1)/2, TP_WALL_CY+4, "WALL", 1, COL(255,244,200)); }
        }
    }
    if (IN.joy_on){
        circle_out(IN.joy_ax, IN.joy_ay, 42, COL(34,31,52));
        circle_shade(IN.joy_ax, IN.joy_ay, 39, COL(40,36,58));
        disc_fill(IN.joy_ax, IN.joy_ay, 25, COL(30,27,46));
        circle_out(IN.joy_kx, IN.joy_ky, 17, COL(198,204,220));
        circle_shade(IN.joy_kx, IN.joy_ky, 14, COL(198,204,220));
        disc_fill(IN.joy_kx-5, IN.joy_ky-5, 4, COL(244,246,250));
    } else if (!S->craft_menu){
        disc_fill(TP_DPAD_CX, TP_DPAD_CY, 42, COL(24,22,38));
        circle_out(TP_DPAD_CX, TP_DPAD_CY, 15, COL(52,48,72));
    }
}
static void bw_render(int pi){
    g_dev_ctx = dev_of(pi);   // v1.25: this view's hints name this player's device
    bw_render_modes(pi);
    bw_draw_touch(pi);
    // the START chip (v1.26 Item 2): drawn here, not in bw_draw_touch,
    // whose early returns would hide it exactly where it is needed.
    // Tap side: bw_fill_p1.
    if (pi==G.my_id && bw_start_chip_live()){
        int cx=G.view_x0+8, cy=G.view_y0+8;
        rect_fill(cx, cy, BW_CHIP_W, BW_CHIP_H, COL(16,18,28)); rect_line(cx, cy, BW_CHIP_W, BW_CHIP_H, COL(198,164,84));
        rect_fill(cx+8, cy+7, 3, 10, COL(255,214,64)); rect_fill(cx+14, cy+7, 3, 10, COL(255,214,64));
        draw_text(cx+26, cy+8, "START", 1, COL(255,244,200));
    }
    // the Start-menu and its CONTROLS panel, over this player's view
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    if (S->bw_menu){
        int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
        if (S->bw_menu==2){ draw_controls_panel(vx, vy, vw2, vh2, b->bw_mode<2? 1 : (b->bw_mode==5? 6 : (b->bw_mode==6? 10 : b->bw_mode))); return; }   // v1.30 (audit F9): Castle has its own context (10), not Tank's
        int pw=300, ph=140, px=vx+(vw2-pw)/2, py=vy+(vh2-ph)/2;
        blend_rect(vx, vy, vw2, vh2, COL(8,8,14), 120);
        rect_fill(px, py, pw, ph, COL(16,18,28)); rect_line(px, py, pw, ph, COL(198,164,84));
        draw_text_center(px+pw/2, py+12, "PAUSED", 2, COL(255,214,64));
        { static const char *rows[3] = { "RESUME", "CONTROLS", "LEAVE" };
          for (int i=0;i<3;i++){
              int ry = py+48+i*26;
              if (i==S->bw_msel){ rect_fill(px+10, ry-4, pw-20, 19, COL(255,205,40)); draw_text_center(px+pw/2, ry, rows[i], 2, COL(30,26,44)); }
              else draw_text_center(px+pw/2, ry, rows[i], 2, COL(235,238,245));
          } }
        // v1.30 (audit F16/F14): the viewing player's device, and the
        // real FIRE bind on keyboard — P2's pad view could be shown the
        // keyboard text when P1 was on keys.
        { char fb[64]; char *q=fb;
          if (hint_dev()==DEV_PAD) q=sappend(q, "STICK/DPAD + A");
          else if (hint_dev()==DEV_TOUCH) q=sappend(q, "TAP A ROW");
          else { q=sappend(q, "ARROWS + "); q=sappend(q, key_name(G.bind[ACT_FIRE])); }
          *q=0; draw_text_center(px+pw/2, py+ph-16, fb, 1, COL(140,150,170)); }
    }
}
static void bw_render_battle(int pi){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    int band = vh2<300? vh2/2 : 150;
    rect_fill(vx, vy, vw2, vh2, COL(22,40,28));
    rect_fill(vx, vy+vh2-band, vw2, band, COL(30,56,34));
    ellipse_fill(vx+vw2/2, vy+vh2*58/100, vw2*39/100, vh2*12/100, COL(52,92,50));
    ellipse_fill(vx+vw2/2, vy+vh2*58/100, vw2*26/100, vh2*8/100, COL(66,110,58));
    // fighters on a stage far from any real map
    float sx = b->x, sy = b->y, sa = b->aim;
    G.camx = 20000; G.camy = 20000;
    // spotlight pools under the fighters (glow-up)
    { int bxp = SX(20000-112), byp = SY(20000+34), exp2 = SX(20000+118), eyp = SY(20000-24);
      ellipse_fill(bxp, byp+9, 46, 13, COL(84,128,66)); ellipse_fill(bxp, byp+9, 30, 8, COL(102,148,80));
      ellipse_fill(exp2, eyp+17, 46, 13, COL(84,128,66)); ellipse_fill(exp2, eyp+17, 30, 8, COL(102,148,80)); }
    b->x = 20000-112; b->y = 20000+34; b->aim = 0;
    draw_buddy(b);
    b->x = sx; b->y = sy; b->aim = sa;
    if (S->bat.boss) bw_draw_big_hacker(20000+118, 20000-14, S->bat.boss, 0);
    else bw_draw_hacker(20000+118, 20000-24, S->bat.lvl, (S->bat.phase==2 && S->bat.t>0.85f)? 1.0f : 0.0f, 0);
    // foe plate
    rect_fill(vx+14, vy+12, 212, 34, COL(16,20,16)); rect_line(vx+14, vy+12, 212, 34, COL(255,120,110));
    { char buf[64]; char *q=buf; q=sappend(q, bw_foe_name(S)); q=sappend(q, "  LV "); q=sapp_int(q, S->bat.lvl); *q=0;
      draw_text(vx+20, vy+17, buf, 1, COL(255,210,200)); }
    { float f = S->bat.hmax>0? S->bat.hhp/S->bat.hmax : 0; if (f<0) f=0;
      rect_fill(vx+20, vy+31, 150, 5, COL(40,30,40)); rect_fill(vx+20, vy+31, (int)(150*f), 5, COL(255,120,90));
      char hb[24]; char *q=hb; q=sapp_int(q, (int)S->bat.hhp); q=sappend(q, "/"); q=sapp_int(q, (int)S->bat.hmax); *q=0;
      draw_text(vx+174, vy+30, hb, 1, COL(255,225,215)); }
    // buddy plate (bottom-right of the view, above the command box)
    { int px0 = vx+vw2-224, py0 = vy+vh2-128-((S->bat.natt>6)? ((S->bat.natt+1)/2-3)*15 : 0); if (py0 < vy+52) py0 = vy+52;
    rect_fill(px0, py0, 210, 40, COL(16,20,16)); rect_line(px0, py0, 210, 40, COL(198,164,84));
    { char buf[64]; char *q=buf; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[b->species%BUDN]); *q=0;
      draw_text(px0+6, py0+5, buf, 1, COL(255,244,210)); }
    { float f = b->maxhp>0? b->hp/b->maxhp : 0; if (f<0) f=0;
      rect_fill(px0+6, py0+17, 130, 5, COL(40,30,40)); rect_fill(px0+6, py0+17, (int)(130*f), 5, f>0.35f? COL(120,230,90) : COL(255,170,60));
      char hb[24]; char *q=hb; q=sapp_int(q, (int)b->hp); q=sappend(q, "/"); q=sapp_int(q, (int)b->maxhp); *q=0;
      draw_text(px0+140, py0+16, hb, 1, COL(230,240,230));
      float ef = b->energy/BUD_ENERGY_MAX; if (ef<0) ef=0;
      rect_fill(px0+6, py0+26, 130, 4, COL(40,30,40)); rect_fill(px0+6, py0+26, (int)(130*ef), 4, COL(120,200,255));
      char eb[24]; char *q2=eb; q2=sapp_int(q2, (int)b->energy); q2=sappend(q2, " EN"); *q2=0;
      draw_text(px0+140, py0+25, eb, 1, COL(170,220,255)); } }
    // the bottom box: attack menu or message (taller once items join)
    int rows = (S->bat.natt+1)/2, bh = 84 + (rows>3? (rows-3)*15 : 0), rstep = rows>3? 15 : 18;
    rect_fill(vx, vy+vh2-bh, vw2, bh, COL(14,16,22)); rect_line(vx, vy+vh2-bh, vw2, bh, COL(198,164,84));
    if (S->bat.phase==0){
        draw_text(vx+16, vy+vh2-bh+6, "CHOOSE AN ATTACK:", 1, COL(255,244,210));
        for (int i=0;i<S->bat.natt;i++){
            int mv = S->bat.attacks[i];
            const char *nm; float cost, mult; bw_attack_info(b, mv, &nm, &cost, &mult);
            int cxp = vx+16 + (i%2)*(vw2/2-20), cyp = vy+vh2-bh+22 + (i/2)*rstep;
            int afford = b->energy >= cost;
            if (i==S->bat.sel) draw_text(cxp, cyp, ">", 1, COL(255,220,90));
            draw_text(cxp+12, cyp, nm, 1, afford? COL(240,248,240) : COL(110,110,120));
            if (mv>=6){ char cb[20]; char *q=cb; q=sappend(q, "x"); q=sapp_int(q, b->items[mv-6]); *q=0;
                draw_text(cxp + vw2/2-130, cyp, cb, 1, COL(255,210,130)); }
            else if (cost>0){ char cb[20]; char *q=cb; q=sapp_int(q, (int)cost); q=sappend(q, " EN"); *q=0;
                draw_text(cxp + vw2/2-130, cyp, cb, 1, afford? COL(170,220,255) : COL(110,110,120)); }
            else if (mv!=5) draw_text(cxp + vw2/2-130, cyp, "FREE", 1, COL(160,230,160));
        }
    } else {
        draw_text(vx+16, vy+vh2-bh+14, S->bat.msg, 1, COL(255,250,235));
        if (S->bat.msg2[0]) draw_text(vx+16, vy+vh2-bh+32, S->bat.msg2, 1, COL(200,214,235));
    }
}

