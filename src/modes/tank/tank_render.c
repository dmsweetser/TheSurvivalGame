// THE SURVIVAL GAME — modes/tank/tank_render.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from tank.c lines 500-741 in the Nov 2026 fine-grained refactor; code unchanged.

static void tank_render(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    float camx = b->x, half = vw2/2.0f;
    if (camx < half) camx = half;
    if (camx > TANK_W*BW_TILE - half) camx = TANK_W*BW_TILE - half;
    float camy = b->y, halfy = vh2/2.0f;
    if (camy < halfy) camy = halfy;
    if (camy > TANK_LEN*BW_TILE - halfy) camy = TANK_LEN*BW_TILE - halfy;
    G.camx = camx; G.camy = camy;
    rect_fill(vx, vy, vw2, vh2, COL(26,28,22));
    int warm = (S->tank_lvl*3)%18;
    int tx0=bw_tileof(camx - half - BW_TILE), tx1=bw_tileof(camx + half + BW_TILE);
    int ty0=bw_tileof(camy - halfy - BW_TILE), ty1=bw_tileof(camy + halfy + BW_TILE);
    for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
        int x=SX(tx*BW_TILE), y=SY(ty*BW_TILE);
        int t = tank_tile(S, tx, ty);
        u32 h = hash2(tx, ty, (u32)S->tank_seed);
        if (ty<2 && tx>=0 && tx<TANK_W){   // the finish strip
            rect_fill(x, y, BW_TILE+1, BW_TILE+1, ((tx+ty)&1)? COL(232,232,238) : COL(30,30,36));
            continue;
        }
        if (ty<TANK_GATE_TY && tx>=0 && tx<TANK_W){   // the arena plaza
            rect_fill(x, y, BW_TILE+1, BW_TILE+1, ((tx+ty)&1)? COL(122,124,130) : COL(112,114,120));
            if (t==0) continue;
        } else {   // the field: two-tone scrub, warmed a step per level
            u32 g = ((tx+ty)&1)? COL(104+warm, 146, 66) : COL(95+warm, 137, 59);
            rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(g, 0.95f + (float)((h>>20)%10)*0.01f));
        }
        if (t==1){   // water
            rect_fill(x, y, BW_TILE+1, BW_TILE+1, ((tx+ty)&1)? COL(52,110,178) : COL(46,100,168));
            if (h % 7 == 0){ int wxp = x + (int)((h>>6) % 30) + 6, wyp = y + (int)((h>>11) % 32) + 8;
                line_thick(wxp, wyp, wxp+7, wyp, 1, COL(150,200,235)); }
        } else if (t==2){   // concrete
            rect_fill(x+2, y+2, BW_TILE-4, BW_TILE-4, COL(148,150,156));
            rect_fill(x+2, y+2, BW_TILE-4, 4, COL(172,174,180));
            rect_line(x+2, y+2, BW_TILE-4, BW_TILE-4, COL(96,98,104));
            disc_fill(x+7, y+7, 1, COL(104,106,112)); disc_fill(x+BW_TILE-8, y+BW_TILE-8, 1, COL(104,106,112));
        } else if (t==3){   // trees
            rect_fill(x+22, y+26, 4, 12, COL(110,76,44));
            disc_fill(x+16, y+18, 11, COL(52,116,52)); disc_fill(x+31, y+15, 10, COL(62,130,58));
            disc_fill(x+24, y+26, 11, COL(44,104,48)); disc_fill(x+19, y+13, 3, COL(120,180,110));
        } else if (t==4){   // sandbags (cracked once they've taken a hit)
            rect_fill(x+3, y+22, 20, 10, COL(198,168,110)); rect_fill(x+25, y+22, 20, 10, COL(188,158,102));
            rect_fill(x+13, y+12, 20, 10, COL(204,176,118)); rect_fill(x+8, y+32, 15, 9, COL(190,160,104));
            rect_fill(x+26, y+32, 15, 9, COL(196,166,108));
            if (S->tank_hit[ty*TANK_W+tx]>0){ line_thick(x+10, y+14, x+22, y+26, 1, COL(90,70,40)); line_thick(x+30, y+24, x+38, y+36, 1, COL(90,70,40)); }
        } else if (t==5){   // the gate: a striped steel band, shut tight
            rect_fill(x, y+14, BW_TILE+1, 20, COL(58,58,64));
            for (int s2=0;s2<6;s2++) rect_fill(x+s2*8, y+17, 4, 14, (s2&1)? COL(240,200,60) : COL(24,24,28));
            rect_fill(x, y+14, BW_TILE+1, 3, COL(90,90,98)); rect_fill(x, y+31, BW_TILE+1, 3, COL(90,90,98));
        }
    }
    // helipads
    for (int k2=0;k2<3;k2++){
        int pt = tank_pad_ty(k2);
        int x=SX(11*BW_TILE), y=SY(pt*BW_TILE), w=3*BW_TILE, hgt=2*BW_TILE;
        if (x+w<vx || x>vx+vw2 || y+hgt<vy || y>vy+vh2) continue;
        rect_fill(x, y, w, hgt, COL(52,54,58));
        rect_line(x+2, y+2, w-4, hgt-4, COL(240,200,60));
        draw_text_center(x+w/2, y+hgt/2-12, "H", 3, COL(240,200,60));
    }
    // entities
    for (int j=0;j<S->tank_en_n;j++){
        TankEn *e=&S->tank_en[j];
        if (e->kind<0) continue;
        int x=SX(e->x), y=SY(e->y);
        if (x<vx-70 || x>vx+vw2+70 || y<vy-70 || y>vy+vh2+70) continue;
        if (e->kind==3){   // the wrench, bobbing on its box
            int bob = (int)(fsin(S->tank_t*3 + j)*2.0f);
            shadow_ellipse(x, y+10, 10, 3);
            rect_fill(x-10, y-8+bob, 20, 16, COL(238,240,244)); rect_line(x-10, y-8+bob, 20, 16, COL(150,60,60));
            rect_fill(x-2, y-6+bob, 4, 12, COL(200,50,50)); rect_fill(x-6, y-2+bob, 12, 4, COL(200,50,50));
        } else if (e->kind==6){   // HEALTH (v1.27): a medkit — bigger than the wrench box, bold red cross, pale halo so it reads as HELP from across the field
            int bob = (int)(fsin(S->tank_t*3 + j)*2.0f);
            shadow_ellipse(x, y+12, 13, 3);
            disc_fill(x, y+bob, 19, COL(196,246,208));
            disc_fill(x, y+bob, 16, COL(232,250,238));
            rect_fill(x-13, y-10+bob, 26, 20, COL(248,250,252));
            rect_line(x-13, y-10+bob, 26, 20, COL(200,40,40)); rect_line(x-12, y-9+bob, 24, 18, COL(200,40,40));
            rect_fill(x-3, y-7+bob, 6, 14, COL(224,32,32)); rect_fill(x-7, y-3+bob, 14, 6, COL(224,32,32));
        } else if (e->kind==7){   // the GRENADE LAUNCHER (v1.27): a fat lobber tube on a crate, orange muzzle band — a gun silhouette, nothing like the medkit
            int bob = (int)(fsin(S->tank_t*3 + j)*2.0f);
            shadow_ellipse(x, y+12, 14, 3);
            rect_fill(x-13, y+3+bob, 26, 9, COL(122,84,46)); rect_line(x-13, y+3+bob, 26, 9, COL(84,56,28));
            line_thick(x-9, y+5+bob, x+8, y-10+bob, 8, COL(52,84,44));
            line_thick(x+5, y-7+bob, x+10, y-12+bob, 9, COL(244,140,36));
            disc_fill(x-9, y+5+bob, 4, COL(28,38,22));
            disc_fill(x+13, y+6+bob, 4, COL(52,84,44)); disc_fill(x+13, y+5+bob, 1, COL(200,230,180));   // a spare grenade by the crate
        } else if (e->kind==2){   // a prison hut
            shadow_ellipse(x, y+16, 24, 6);
            rect_fill(x-22, y-8, 44, 26, COL(146,104,62)); rect_line(x-22, y-8, 44, 26, COL(96,66,36));
            tri_fill(x, y-24, x-26, y-8, x+26, y-8, COL(120,62,44));
            rect_fill(x-5, y+2, 10, 16, COL(74,50,30));
            rect_fill(x+10, y-4, 9, 9, COL(40,36,44));
            rect_fill(x+12, y-4, 1, 9, COL(190,196,204)); rect_fill(x+15, y-4, 1, 9, COL(190,196,204)); rect_fill(x+18, y-4, 1, 9, COL(190,196,204));
            if (e->hp<40){ int w = 40*e->hp/40; rect_fill(x-20, y-30, 40, 4, COL(40,20,20)); rect_fill(x-20, y-30, w, 4, COL(240,120,70)); }
        } else if (e->kind==0){   // a turret: base, dome, a barrel that watches you
            shadow_ellipse(x, y+10, 16, 5);
            disc_fill(x, y, 15, COL(104,108,98)); circle_out(x, y, 15, COL(70,72,64));
            line_thick(x, y, x + (int)(fcos(e->aim)*23), y + (int)(fsin(e->aim)*23), 4, COL(52,54,48));
            disc_fill(x, y, 8, COL(126,130,118)); disc_fill(x, y, 3, COL(220,60,50));
            { int mx = 30 + S->tank_lvl*4; if (e->hp<mx){ int w = 28*e->hp/mx; if (w<0) w=0; rect_fill(x-14, y-22, 28, 3, COL(40,20,20)); rect_fill(x-14, y-22, w, 3, COL(240,120,70)); } }
        } else if (e->kind==1){   // a patrol tank
            shadow_ellipse(x, y+11, 18, 5);
            int horiz = (e->dir==1 || e->dir==-1);
            if (horiz){ rect_fill(x-17, y-12, 34, 7, COL(40,40,44)); rect_fill(x-17, y+5, 34, 7, COL(40,40,44));
                        rect_fill(x-14, y-8, 28, 16, COL(128,84,60)); }
            else { rect_fill(x-12, y-17, 7, 34, COL(40,40,44)); rect_fill(x+5, y-17, 7, 34, COL(40,40,44));
                   rect_fill(x-8, y-14, 16, 28, COL(128,84,60)); }
            line_thick(x, y, x + (int)(fcos(e->aim)*22), y + (int)(fsin(e->aim)*22), 4, COL(70,50,36));
            disc_fill(x, y, 6, COL(146,100,70));
            { int mx = 40 + S->tank_lvl*5; if (e->hp<mx){ int w = 32*e->hp/mx; if (w<0) w=0; rect_fill(x-16, y-22, 32, 3, COL(40,20,20)); rect_fill(x-16, y-22, w, 3, COL(240,120,70)); } }
        } else if (e->kind==5){   // the blockade gun: a sandbagged bunker squatting on the road
            shadow_ellipse(x, y+20, 34, 8);
            rect_fill(x-32, y-16, 64, 34, COL(120,96,58)); rect_line(x-32, y-16, 64, 34, COL(84,66,38));
            rect_fill(x-32, y-16, 64, 7, COL(198,168,110)); rect_fill(x-32, y+11, 64, 7, COL(198,168,110));
            line_thick(x-8, y+6, x-8 + (int)(fcos(e->aim)*26), y+6 + (int)(fsin(e->aim)*26), 4, COL(52,54,48));
            line_thick(x+8, y+6, x+8 + (int)(fcos(e->aim)*26), y+6 + (int)(fsin(e->aim)*26), 4, COL(52,54,48));
            disc_fill(x, y-4, 9, COL(126,130,118)); disc_fill(x, y-4, 3, COL(220,60,50));
            draw_text_center(x, y-30, "BLOCKADE", 1, COL(255,220,150));
            if (S->tank_mid_max>0 && e->hp<S->tank_mid_max){ int w = 56*e->hp/S->tank_mid_max; if (w<0) w=0; rect_fill(x-28, y+22, 56, 4, COL(40,20,20)); rect_fill(x-28, y+22, w, 4, COL(240,120,70)); }
        } else if (e->kind==4){   // the fortress core
            shadow_ellipse(x, y+40, 48, 12);
            rect_fill(x-46, y-42, 92, 84, COL(96,100,108)); rect_line(x-46, y-42, 92, 84, COL(60,62,68));
            rect_fill(x-46, y-42, 92, 8, COL(120,124,132)); rect_fill(x-46, y+6, 92, 4, COL(74,76,84));
            disc_fill(x-34, y-30, 6, COL(70,72,78)); disc_fill(x+34, y-30, 6, COL(70,72,78));
            disc_fill(x-34, y+30, 6, COL(70,72,78)); disc_fill(x+34, y+30, 6, COL(70,72,78));
            line_thick(x, y+10, x + (int)(fcos(e->aim)*34), y + 10 + (int)(fsin(e->aim)*34), 6, COL(52,54,60));
            { float pulse = 0.75f + 0.25f*fsin(S->tank_t*4);
              disc_fill(x, y, 19, COL(140,60,60)); disc_fill(x, y, (int)(12*pulse)+4, COL(255,90,70));
              disc_fill(x-4, y-4, 3, COL(255,200,170)); }
            draw_text_center(x, y-58, "FORTRESS CORE", 1, COL(255,170,150));
        }
    }
    // captives, standing free and waving
    for (int i=0;i<24;i++) if (S->tank_cap_a[i]){
        int x=SX(S->tank_cap_x[i]), y=SY(S->tank_cap_y[i]);
        if (x<vx-30 || x>vx+vw2+30 || y<vy-30 || y>vy+vh2+30) continue;
        shadow_ellipse(x, y+8, 7, 2);
        rect_fill(x-3, y-4, 7, 10, COL(240,140,40));   // the orange suit
        disc_fill(x, y-8, 4, COL(238,196,150));
        int wv = (int)(fsin(G.anim_t*7 + i*1.3f)*4.0f);
        line_thick(x+3, y-3, x+8+wv, y-10, 2, COL(238,196,150));
        line_thick(x-3, y-3, x-6, y+2, 2, COL(238,196,150));
    }
    // shells
    for (int i=0;i<24;i++) if (S->tank_sh_a[i]){
        int x=SX(S->tank_sh_x[i]), y=SY(S->tank_sh_y[i]);
        disc_fill(x, y, 3, COL(255,240,170)); disc_fill(x, y, 1, COL(255,255,255));
    }
    for (int i=0;i<32;i++) if (S->tank_es_a[i]){
        int x=SX(S->tank_es_x[i]), y=SY(S->tank_es_y[i]);
        disc_fill(x, y, 3, COL(255,90,60)); disc_fill(x, y, 1, COL(255,180,140));
    }
    // grenades in flight, and their blasts
    for (int i=0;i<10;i++) if (S->tank_gr_a[i]){
        int x=SX(S->tank_gr_x[i]), y=SY(S->tank_gr_y[i]);
        disc_fill(x, y+2, 4, COL(20,30,16));
        disc_fill(x, y, 4, COL(52,84,44)); disc_fill(x-1, y-1, 1, COL(200,230,180));
    }
    for (int i=0;i<6;i++) if (S->tank_boom_t[i]>0){
        int x=SX(S->tank_boom_x[i]), y=SY(S->tank_boom_y[i]);
        int rr = (int)((0.3f - S->tank_boom_t[i]) * 260.0f) + 8;
        glow_ellipse(x, y, rr, rr*2/3, COL(255,170,80), 110);   // v1.22: a hot flash inside the shockwave rings
        circle_out(x, y, rr, COL(255,190,90)); circle_out(x, y, rr*2/3 + 2, COL(255,130,60));
        disc_fill(x, y, rr/3 + 2, COL(255,220,150));
    }
    // your tank (gone while it burns)
    if (S->tank_deadt<=0 && !(S->tank_grace>0.6f && ((int)(S->tank_t*14)&1))){
        int x=SX(b->x), y=SY(b->y);
        float fx=S->tank_fx, fy=S->tank_fy, px=-fy, py=fx;
        shadow_ellipse(x, y+11, 19, 6);
        for (int s2=-1;s2<=1;s2+=2){   // treads, marks scrolling with the walk phase
            float cx2 = x + px*13*s2, cy2 = y + py*13*s2;
            line_thick((int)(cx2 - fx*15), (int)(cy2 - fy*15), (int)(cx2 + fx*15), (int)(cy2 + fy*15), 7, COL(38,38,42));
            for (int m=0;m<3;m++){
                int off = -12 + ((m*10 + (int)(b->walk_ph*8)) % 30);
                int mxp = (int)(cx2 + fx*off), myp = (int)(cy2 + fy*off);
                line_thick((int)(mxp - px*3), (int)(myp - py*3), (int)(mxp + px*3), (int)(myp + py*3), 2, COL(74,74,80));
            }
        }
        disc_fill(x, y, 16, COL(96,110,52)); circle_out(x, y, 16, COL(64,74,36));
        disc_fill(x, y, 12, COL(110,126,60));
        // the passengers ride behind the turret, heads up
        for (int p2=0;p2<S->tank_pass;p2++){
            int hx2 = (int)(x - fx*11 + px*(p2*7 - 10)), hy2 = (int)(y - fy*11 + py*(p2*7 - 10));
            disc_fill(hx2, hy2, 3, COL(238,196,150));
        }
        line_thick((int)(x + fx*6), (int)(y + fy*6), (int)(x + fx*27), (int)(y + fy*27), 4, COL(52,60,34));
        disc_fill(x, y, 8, COL(80,94,44));
        disc_fill(x, y, 5, bud_coat(b, COL(199,152,94)));   // the driver's head, out of the hatch
    }
    // HUD
    rect_fill(vx, vy, vw2, 20, COL(16,20,28));
    { char buf[96]; char *q=buf;
      q=sappend(q, "LEVEL "); q=sapp_int(q, S->tank_lvl);
      q=sappend(q, "   SCORE "); q=sapp_int(q, S->tank_score);
      q=sappend(q, "   LIVES "); q=sapp_int(q, S->tank_lives);
      q=sappend(q, "   ABOARD "); q=sapp_int(q, S->tank_pass); q=sappend(q, "/4");
      *q=0; draw_text(vx+8, vy+6, buf, 1, COL(240,244,250)); }
    { char buf[128]; char *q=buf;
      // v1.27: while the GRENADE LAUNCHER is aboard its tag rides the
      // weapon line for the whole run, and the grenade count reads
      // "--" (bottomless) instead of a number
      if (S->tank_wep==0){ q=sappend(q, "WEAPON: CANNON  ("); q=sappend(q, lbl_wall()); q=sappend(q, ": GRENADES x"); }
      else { q=sappend(q, "WEAPON: GRENADE x"); }
      if (tank_glauncher_st[pi]) q=sappend(q, "--"); else q=sapp_int(q, S->tank_gren);
      if (S->tank_wep) { q=sappend(q, "  ("); q=sappend(q, lbl_wall()); q=sappend(q, ": CANNON)"); }
      else q=sappend(q, ")");
      if (tank_glauncher_st[pi]) q=sappend(q, "  [GRENADE LAUNCHER]");
      *q=0; draw_text(vx+8, vy+24, buf, 1, S->tank_wep? COL(255,190,90) : COL(170,190,200)); }
    { int bw3 = 120, bx = vx+vw2-bw3-10, by = vy+6;
      draw_text(bx-32, by, "HULL", 1, COL(240,244,250));
      rect_fill(bx-1, by-1, bw3+2, 9, COL(8,8,12));
      int hw = S->tank_hull*bw3/100; if (hw<0) hw=0;
      rect_fill(bx, by, hw, 7, S->tank_hull>50? COL(90,200,90) : S->tank_hull>25? COL(240,190,70) : COL(240,80,70)); }
    if (S->tank_boss_hp>0 && b->y < 16*BW_TILE){   // the core's health, once you're in its shadow
        int bw3 = 260, bx = vx+(vw2-bw3)/2, by = vy+26;
        draw_text_center(vx+vw2/2, by-11, "FORTRESS CORE", 1, COL(255,170,150));
        rect_fill(bx-1, by-1, bw3+2, 8, COL(8,8,12));
        int hw = S->tank_boss_max>0? S->tank_boss_hp*bw3/S->tank_boss_max : 0; if (hw<0) hw=0;
        rect_fill(bx, by, hw, 6, COL(240,80,70));
    }
    if (S->tank_mid_hp>0 && b->y < 58*BW_TILE){   // the blockade's health, once you're in its shadow
        int bw3 = 260, bx = vx+(vw2-bw3)/2, by = vy+26;
        draw_text_center(vx+vw2/2, by-11, "BLOCKADE GUN", 1, COL(255,220,150));
        rect_fill(bx-1, by-1, bw3+2, 8, COL(8,8,12));
        int hw = S->tank_mid_max>0? S->tank_mid_hp*bw3/S->tank_mid_max : 0; if (hw<0) hw=0;
        rect_fill(bx, by, hw, 6, COL(240,170,70));
    }
    // the mission line: one objective, always on screen, always current
    { const char *ob = S->tank_obj==0? "RESCUE THE CAPTIVES" : S->tank_obj==1? "DELIVER TO A HELIPAD" :
                       S->tank_obj==2? "BREAK THE BLOCKADE!" : S->tank_obj==3? "DESTROY THE FORTRESS CORE!" : "CROSS THE FINISH LINE!";
      char buf[96]; char *q=buf; q=sappend(q, "OBJECTIVE: "); q=sappend(q, ob);
      if (S->tank_obj<=1){ q=sappend(q, "  (DELIVERED "); q=sapp_int(q, S->tank_deliv); q=sappend(q, "/"); q=sapp_int(q, S->tank_quota); q=sappend(q, ")"); }
      *q=0;
      rect_fill(vx, vy+38, vw2, 16, COL(16,20,28));
      draw_text_center(vx+vw2/2, vy+42, buf, 1, COL(255,220,90)); }
    // the yellow arrow: a bouncing chevron over the target when it's on
    // screen, a bold pointer pinned to the screen edge when it isn't
    if (S->tank_deadt<=0){
        float txp = (float)SX(S->tank_obj_x), typ = (float)SY(S->tank_obj_y);
        float pxp = (float)SX(b->x), pyp = (float)SY(b->y);
        int onscr = txp>vx+16 && txp<vx+vw2-16 && typ>vy+104 && typ<vy+vh2-16;
        if (onscr){ int bob = (int)(fsin(S->tank_t*5)*3.0f); int ay = (int)typ - 42 + bob;
            tri_fill((int)txp, ay+11, (int)txp-7, ay, (int)txp+7, ay, COL(255,220,90)); }
        else { float ang = fatan2(typ-pyp, txp-pxp);
            float ax = txp, ay = typ;
            if (ax<vx+30) ax=(float)vx+30; if (ax>vx+vw2-30) ax=(float)vx+vw2-30;
            if (ay<vy+66) ay=(float)vy+66; if (ay>vy+vh2-30) ay=(float)vy+vh2-30;
            float ca=fcos(ang), sa=fsin(ang), pxx=-sa, pyy=ca;
            tri_fill((int)(ax+ca*14), (int)(ay+sa*14), (int)(ax-ca*10+pxx*8), (int)(ay-sa*10+pyy*8), (int)(ax-ca*10-pxx*8), (int)(ay-sa*10-pyy*8), COL(24,20,8));
            tri_fill((int)(ax+ca*11), (int)(ay+sa*11), (int)(ax-ca*8+pxx*6), (int)(ay-sa*8+pyy*6), (int)(ax-ca*8-pxx*6), (int)(ay-sa*8-pyy*6), COL(255,220,90)); }
    }
    if (S->tank_cleart>0) draw_text_center(vx+vw2/2, vy+vh2/3, "LEVEL CLEAR!", 2, COL(255,244,180));
    if (S->note_t>0){
        rect_fill(vx, vy+vh2-22, vw2, 22, COL(14,14,20));
        draw_text_center(vx+vw2/2, vy+vh2-15, S->note, 1, COL(255,240,200));
    }
}

