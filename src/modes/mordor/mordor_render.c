// THE SURVIVAL GAME — modes/mordor/mordor_render.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).

// ===========================================================================
// MORDOR FUN RUN — the render (v1.28). The road is drawn the way it
// is simulated: a function of distance. Ground bands shift with the
// road's fraction (Shire green -> forest -> golden plains -> rock ->
// the ash of Mordor), the dirt road follows the centreline, scenery
// is a hash of the tile, and the milestone markers are MOOSE
// markers — a post, a board, and the moose count banked there. At
// the start line stands THE MOOSE itself, the living ruler, beside
// the measured ONE MOOSE track (two posts exactly MOR_MOOSE_PX
// apart along the road's edge). At the far end: the Black Gate and
// the mountain, smoking.
// ===========================================================================
static u32 mor_band_col(float f, int checker){
    // the road's country, south to north; f<0 is behind the Shire
    // fence (still green), f>1 is past the gate (ash and dark)
    if (f < 0.18f) return checker? COL(104,178,80) : COL(92,164,70);          // THE SHIRE
    if (f < 0.38f) return checker? COL(74,138,64) : COL(66,126,58);           // HILLS + FOREST
    if (f < 0.62f) return checker? COL(182,166,98) : COL(170,154,90);         // THE PLAINS
    if (f < 0.82f) return checker? COL(130,118,106) : COL(120,108,97);        // THE ROCKY WASTES
    return checker? COL(76,70,74) : COL(66,61,66);                            // MORDOR
}
static void mor_draw_moose(int x, int y, float t){
    // THE MOOSE (at scale, more or less — it is a MOOSE): the living
    // ruler by the start line. Brown bulk, long face, and the
    // famous flat antlers. It stands ~190 units nose to tail; the
    // measured track beside it is the honest 460.
    int bob = (int)(fsin(t*0.9f)*1.5f);
    shadow_ellipse(x, y+34, 74, 12);
    // legs
    rect_fill(x-58, y+8+bob, 9, 30, COL(96,70,44)); rect_fill(x-38, y+8+bob, 9, 30, COL(104,76,48));
    rect_fill(x+34, y+8+bob, 9, 30, COL(96,70,44)); rect_fill(x+54, y+8+bob, 9, 30, COL(104,76,48));
    // the bulk, with its shoulder hump
    disc_fill(x-30, y-6+bob, 26, COL(122,88,54)); disc_fill(x+8, y-10+bob, 30, COL(130,95,58));
    disc_fill(x-52, y-16+bob, 17, COL(138,102,62));
    rect_fill(x-58, y-22+bob, 118, 30, COL(126,92,56));
    disc_fill(x-58, y-7+bob, 15, COL(126,92,56)); disc_fill(x+60, y-7+bob, 15, COL(118,86,52));
    // neck + the long face, hanging low and thoughtful
    rect_fill(x+52, y-16+bob, 20, 26, COL(118,86,52));
    rect_fill(x+58, y+2+bob, 26, 13, COL(108,78,47));
    disc_fill(x+86, y+8+bob, 8, COL(96,68,40));
    pset(x+80, y+5+bob, COL(20,16,12)); pset(x+66, y-6+bob, COL(20,16,12)); pset(x+67, y-6+bob, COL(20,16,12));
    // the antlers: two flat fans on lines
    line_thick(x+56, y-16+bob, x+40, y-34+bob, 3, COL(196,170,120));
    line_thick(x+40, y-34+bob, x+26, y-30+bob, 3, COL(196,170,120));
    line_thick(x+40, y-34+bob, x+38, y-44+bob, 3, COL(196,170,120));
    line_thick(x+64, y-16+bob, x+76, y-34+bob, 3, COL(186,160,112));
    line_thick(x+76, y-34+bob, x+90, y-28+bob, 3, COL(186,160,112));
    line_thick(x+76, y-34+bob, x+72, y-44+bob, 3, COL(186,160,112));
    // tail
    line_thick(x-72, y-12+bob, x-78, y+4+bob, 3, COL(96,70,44));
}
static void mor_draw_foe(MorEn *e, int x, int y, float t, int idx){
    int flash = e->hit_t>0;
    if (e->kind==0){   // a goblin: small, green, all ears and elbows
        u32 body = flash? COL(255,255,255) : COL(96,168,64), dark = flash? COL(255,255,255) : COL(64,120,44);
        shadow_ellipse(x, y+9, 10, 3);
        rect_fill(x-5, y-4, 10, 12, body);
        disc_fill(x, y-8, 6, body);
        tri_fill(x-6, y-12, x-11, y-6, x-5, y-6, dark); tri_fill(x+6, y-12, x+11, y-6, x+5, y-6, dark);
        pset(x-2, y-8, COL(20,20,16)); pset(x+2, y-8, COL(20,20,16));
        line_thick(x-5, y+2, x-9, y+6, 2, dark); line_thick(x+5, y+2, x+9, y+6, 2, dark);
    } else if (e->kind==1){   // a wolf: low, grey, quick
        u32 body = flash? COL(255,255,255) : COL(142,146,156), dark = flash? COL(255,255,255) : COL(104,108,120);
        int run = (int)(fsin(t*10 + idx*1.7f)*2.0f);
        shadow_ellipse(x, y+8, 15, 3);
        rect_fill(x-13, y-4+run/2, 26, 9, body);
        disc_fill(x-13, y, 5, body); disc_fill(x+13, y, 5, body);
        disc_fill(x+15, y-4+run/2, 6, body);
        tri_fill(x+12, y-10+run/2, x+14, y-5+run/2, x+17, y-9+run/2, dark);
        pset(x+16, y-4+run/2, COL(16,16,20));
        line_thick(x-15, y, x-21, y-5-run/2, 2, dark);
        rect_fill(x-9, y+4, 3, 6, dark); rect_fill(x+6, y+4, 3, 6, dark);
    } else if (e->kind==2){   // a spider: dark, round, too many legs
        u32 body = flash? COL(255,255,255) : COL(52,46,66), leg = flash? COL(255,255,255) : COL(38,34,50);
        shadow_ellipse(x, y+8, 13, 3);
        for (int l=0;l<4;l++){
            int wig = (int)(fsin(t*7 + idx*2.1f + l*1.3f)*2.0f);
            line_thick(x-4, y-2+l*3, x-15, y-6+l*4+wig, 2, leg);
            line_thick(x+4, y-2+l*3, x+15, y-6+l*4-wig, 2, leg);
        }
        disc_fill(x, y, 10, body); disc_fill(x, y+5, 6, body);
        pset(x-3, y-2, COL(255,80,60)); pset(x+3, y-2, COL(255,80,60)); pset(x, y-4, COL(255,80,60));
    } else {   // an orc: the big one — broad, brown-green, underbite
        u32 body = flash? COL(255,255,255) : COL(110,128,62), dark = flash? COL(255,255,255) : COL(78,94,44);
        shadow_ellipse(x, y+13, 15, 4);
        rect_fill(x-10, y-6, 20, 18, body);
        disc_fill(x, y-11, 8, body);
        rect_fill(x-8, y-6, 16, 3, dark);
        pset(x-3, y-12, COL(255,210,90)); pset(x+3, y-12, COL(255,210,90));
        pset(x-2, y-6, COL(240,240,230)); pset(x+3, y-6, COL(240,240,230));   // the underbite
        line_thick(x-10, y+2, x-15, y+9, 3, dark); line_thick(x+10, y+2, x+15, y+9, 3, dark);
    }
    if (e->hp < e->maxhp){
        int w = (int)(26 * e->hp / e->maxhp); if (w<0) w=0;
        rect_fill(x-13, y-24, 26, 3, COL(40,20,20)); rect_fill(x-13, y-24, w, 3, COL(240,120,70));
    }
}
static void mor_render(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    int total = mor_total_units(pi);
    G.camx = b->x; G.camy = b->y;
    rect_fill(vx, vy, vw2, vh2, COL(30,30,34));
    // --- the ground: tiles coloured by their distance along the road ---
    int tx0=bw_tileof(b->x - vw2/2 - BW_TILE), tx1=bw_tileof(b->x + vw2/2 + BW_TILE);
    int ty0=bw_tileof(b->y - vh2/2 - BW_TILE), ty1=bw_tileof(b->y + vh2/2 + BW_TILE);
    for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
        int x=SX(tx*BW_TILE), y=SY(ty*BW_TILE);
        float sc = (float)MOR_Y0 - (ty*BW_TILE + BW_TILE/2);
        float f = sc / (float)total;
        u32 h = hash2(tx, ty, (u32)b->bw_seed ^ 0x40D2u);
        int checker = ((tx+ty)&1)==0;
        u32 base = mor_band_col(f, checker);
        float latc = (tx*BW_TILE + BW_TILE/2) - mor_cx(sc);
        if (latc<0) latc=-latc;
        if (sc > -60 && sc < (float)total + 60 && latc < MOR_HALF)
            base = checker? COL(172,152,100) : COL(162,142,92);       // the dirt road
        else if (sc > -60 && sc < (float)total + 60 && latc < MOR_HALF + 34)
            base = col_shade(base, 0.88f);                            // its trampled edge
        rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(base, 0.95f + (float)((h>>20)%10)*0.01f));
        // v1.32 dressing: the road itself wears its traffic — pebbles
        // and scuffs on the dirt, a stray tuft on the trampled edge
        if (sc > -60 && sc < (float)total + 60 && latc < MOR_HALF){
            int px3 = x + (int)((h>>3)%36)+6, py3 = y + (int)((h>>8)%36)+6;
            pset(px3, py3, col_shade(base, 0.84f)); pset(px3+5, py3+3, col_shade(base, 1.10f));
            if ((h>>15)%4==0) disc_fill(px3+2, py3+5, 1, col_shade(base, 0.80f));
        } else if (sc > -60 && sc < (float)total + 60 && latc < MOR_HALF + 34 && (h>>17)%3==0){
            int px3 = x + (int)((h>>5)%30)+9, py3 = y + (int)((h>>12)%30)+9;
            line_thick(px3, py3, px3-2, py3-4, 1, col_shade(base, 0.78f));
        }
        // scenery, off the road only: a hash of the tile, dressed by band
        if (latc > MOR_HALF + 46 && sc > -80 && sc < (float)total + 80){
            int px2 = x + (int)((h>>6)%26)+10, py2 = y + (int)((h>>11)%24)+12;
            if (f < 0.38f){   // trees in the green country
                if (h%3==0){
                    rect_fill(px2-2, py2, 4, 11, COL(110,76,44));
                    disc_fill(px2-5, py2-4, 9, f<0.18f? COL(52,116,52) : COL(44,100,46));
                    disc_fill(px2+5, py2-5, 8, f<0.18f? COL(62,130,58) : COL(50,110,50));
                    disc_fill(px2, py2-9, 8, f<0.18f? COL(46,108,48) : COL(40,94,42));
                } else if (h%7==0) disc_fill(px2, py2, 4, COL(58,124,46));
            } else if (f < 0.62f){   // plains: tufts and the odd stone
                if (h%5==0){ line_thick(px2, py2+4, px2-2, py2-4, 1, COL(140,128,66)); line_thick(px2+4, py2+4, px2+5, py2-3, 1, COL(150,138,74)); }
                else if (h%11==0) disc_fill(px2, py2, 5, COL(146,138,116));
            } else if (f < 0.82f){   // the rocky wastes
                if (h%3==0){ disc_fill(px2, py2, 6, COL(104,96,90)); disc_fill(px2-3, py2-3, 3, COL(128,120,112)); }
                else if (h%8==0){ line_thick(px2-6, py2+4, px2+2, py2-4, 2, COL(96,88,82)); line_thick(px2+2, py2-4, px2+8, py2+3, 2, COL(96,88,82)); }
            } else {   // Mordor: black rocks, and the ground itself glows in the cracks
                if (h%3==0){ disc_fill(px2, py2, 6, COL(40,36,40)); disc_fill(px2-2, py2-2, 2, COL(70,60,58)); }
                else if (h%6==0){ line_thick(px2-7, py2+3, px2+1, py2-2, 1, COL(255,110,40)); line_thick(px2+1, py2-2, px2+7, py2+2, 1, COL(230,84,30)); }
            }
        }
    }
    // --- the measured ONE MOOSE track + the start line ---------------------
    { float sx0 = mor_cx(0) + MOR_HALF + 44, sx1 = mor_cx(MOR_MOOSE_PX) + MOR_HALF + 44;
      int x0=SX(sx0), y0=SY(MOR_Y0), x1=SX(sx1), y1=SY(MOR_Y0 - MOR_MOOSE_PX);
      if (y0>vy-60 && y1<vy+vh2+60){
          line_thick(x0, y0, x1, y1, 2, COL(255,244,210));
          rect_fill(x0-3, y0-14, 6, 14, COL(146,104,62)); rect_fill(x1-3, y1-14, 6, 14, COL(146,104,62));
          draw_text_center((x0+x1)/2 + 34, (y0+y1)/2, "ONE MOOSE", 1, COL(255,244,210));
      }
      // the start line itself: a checkered band across the road
      int ly = SY(MOR_Y0);
      if (ly>vy-30 && ly<vy+vh2+30){
          float cx0 = mor_cx(0);
          for (int q2=0;q2<8;q2++){
              int qx = SX(cx0 - MOR_HALF + q2*(MOR_HALF/4));
              rect_fill(qx, ly-5, MOR_HALF/4, 10, (q2&1)? COL(30,30,36) : COL(232,232,238));
          }
          draw_text_center(SX(cx0), ly-26, "THE SHIRE", 2, COL(255,244,210));
      }
      // THE MOOSE, standing west of the start, being the ruler
      int mxp = SX(mor_cx(0) - MOR_HALF - 150), myp = SY(MOR_Y0 + 30);
      if (mxp>vx-220 && mxp<vx+vw2+220 && myp>vy-120 && myp<vy+vh2+120) mor_draw_moose(mxp, myp, G.anim_t);
    }
    // --- the milestone markers: moose posts every 50 moose ------------------
    { float spacing = mor_spacing(pi);
      for (int m=1;m<MOR_MILES;m++){
          float sm = m * spacing;
          int x = SX(mor_cx(sm) + MOR_HALF + 26), y = SY((float)MOR_Y0 - sm);
          if (x<vx-70 || x>vx+vw2+70 || y<vy-70 || y>vy+vh2+70) continue;
          int banked = m <= b->mor_mile;
          u32 trim = banked? COL(255,200,60) : COL(210,214,220);
          rect_fill(x-3, y-16, 6, 30, COL(120,84,48)); rect_line(x-3, y-16, 6, 30, COL(84,58,32));
          rect_fill(x-17, y-30, 34, 16, COL(146,104,62)); rect_line(x-17, y-30, 34, 16, trim);
          { char buf[16]; char *q=buf; q=sapp_int(q, m * mor_total_moose(pi) / MOR_MILES); *q=0;
            draw_text_center(x, y-26, buf, 1, COL(255,248,225)); }
          if (m == b->mor_mile + 1 && S->mor_cleart<=0)   // the next one wears a little flag
              tri_fill(x+3, y-34, x+3, y-24, x+13, y-29, COL(255,120,90));
      } }
    // --- the far end: the Black Gate and the mountain -------------------------
    { float gy = (float)MOR_Y0 - total;
      int y = SY(gy);
      if (y>vy-320 && y<vy+vh2+80){
          float cx0 = mor_cx((float)total);
          int gx = SX(cx0);
          // the mountain behind the gate, smoking
          tri_fill(gx, y-300, gx-190, y-40, gx+190, y-40, COL(48,42,48));
          tri_fill(gx, y-300, gx-44, y-208, gx+44, y-208, COL(66,56,54));
          disc_fill(gx, y-296, 13, COL(255,110,40)); disc_fill(gx, y-296, 7, COL(255,170,70));
          { int sm = (int)(fsin(G.anim_t*0.6f)*6.0f);
            disc_fill(gx+sm, y-322, 10, COL(90,84,90)); disc_fill(gx+16+sm, y-338, 13, COL(80,74,82)); }
          // two black towers and the gate between them
          rect_fill(gx-MOR_HALF-56, y-96, 44, 110, COL(26,24,30)); rect_line(gx-MOR_HALF-56, y-96, 44, 110, COL(70,64,70));
          rect_fill(gx+MOR_HALF+12, y-96, 44, 110, COL(26,24,30)); rect_line(gx+MOR_HALF+12, y-96, 44, 110, COL(70,64,70));
          rect_fill(gx-MOR_HALF-56, y-104, 44, 10, COL(16,14,20)); rect_fill(gx+MOR_HALF+12, y-104, 44, 10, COL(16,14,20));
          rect_fill(gx-MOR_HALF, y-58, MOR_HALF*2, 72, COL(34,30,36));
          for (int q2=0;q2<6;q2++) rect_fill(gx-MOR_HALF + 12 + q2*38, y-58, 3, 72, COL(80,72,76));
          draw_text_center(gx, y-84, "MORDOR", 2, COL(255,150,90));
      } }
    // --- the foes ---------------------------------------------------------------
    for (int i=0;i<S->mor_en_n;i++){
        MorEn *e=&S->mor_en[i];
        int x=SX(e->x), y=SY(e->y);
        if (x<vx-60 || x>vx+vw2+60 || y<vy-60 || y>vy+vh2+60) continue;
        mor_draw_foe(e, x, y, G.anim_t, i);
    }
    // --- the road powerups (v1.29): bobbing gifts on the road ---------------------
    for (int pk=0; pk<MOR_POW_N; pk++){
        if (mor_pow_taken(S, pk)) continue;
        float ps, plat; int pkind; mor_pow_pos(pi, pk, &ps, &plat, &pkind);
        int x = SX(mor_cx(ps) + plat), y = SY((float)MOR_Y0 - ps);
        if (x<vx-50 || x>vx+vw2+50 || y<vy-50 || y>vy+vh2+50) continue;
        int bob = (int)(fsin(G.anim_t*2.4f + pk*1.3f)*3.0f);
        int pulse = 13 + (int)(fsin(G.anim_t*3.0f + pk)*3.0f);
        disc_fill(x, y+bob, pulse, pkind==0? COL(120,40,40) : pkind==1? COL(120,80,20) : COL(30,80,110));
        if (pkind==0){   // health: a white box with a red cross
            rect_fill(x-9, y-9+bob, 18, 18, COL(240,240,244)); rect_line(x-9, y-9+bob, 18, 18, COL(160,60,60));
            rect_fill(x-2, y-6+bob, 4, 12, COL(220,50,50)); rect_fill(x-6, y-2+bob, 12, 4, COL(220,50,50));
        } else if (pkind==1){   // amplification: an orange sword
            line_thick(x-6, y+8+bob, x+5, y-3+bob, 3, COL(255,190,80));
            line_thick(x+3, y-8+bob, x+8, y-3+bob, 2, COL(255,230,170));
            line_thick(x-8, y+4+bob, x-3, y+9+bob, 3, COL(150,100,40));
        } else {   // speed: a cyan lightning bolt
            tri_fill(x+2, y-11+bob, x-6, y+1+bob, x-1, y+1+bob, COL(140,230,255));
            tri_fill(x-1, y-1+bob, x-7, y+11+bob, x+5, y-2+bob, COL(140,230,255));
        }
    }
    // --- kill drops (v1.33): the same gift icons as the stations, drawn as
    //     dropped loot — smaller, settled on the ground (a shadow and a
    //     barely-there bob instead of the stations' high bob), a dimmer
    //     halo, and a square loot frame around the icon so a drop never
    //     reads as a station from across the road ---------------------------
    for (int dp=0; dp<MOR_DROP_N; dp++){
        if (!S->mor_drop_on[dp]) continue;
        float ds = S->mor_drop_s[dp]; int dkind = S->mor_drop_kind[dp];
        int x = SX(mor_cx(ds) + S->mor_drop_lat[dp]), y = SY((float)MOR_Y0 - ds);
        if (x<vx-40 || x>vx+vw2+40 || y<vy-40 || y>vy+vh2+40) continue;
        int bob = (int)(fsin(G.anim_t*2.4f + dp*1.7f)*1.5f);
        int pulse = 9 + (int)(fsin(G.anim_t*3.0f + dp)*2.0f);
        shadow_ellipse(x, y+8, 11, 3);
        disc_fill(x, y+bob, pulse, dkind==0? COL(96,32,32) : dkind==1? COL(96,64,16) : COL(24,64,88));
        rect_line(x-8, y-8+bob, 16, 16, dkind==0? COL(255,150,150) : dkind==1? COL(255,200,110) : COL(150,225,255));   // the loot frame
        if (dkind==0){   // health: the station's cross, dropped size
            rect_fill(x-6, y-6+bob, 12, 12, COL(240,240,244));
            rect_fill(x-1, y-4+bob, 3, 8, COL(220,50,50)); rect_fill(x-4, y-1+bob, 8, 3, COL(220,50,50));
        } else if (dkind==1){   // amplification: the station's sword, dropped size
            line_thick(x-4, y+5+bob, x+3, y-2+bob, 2, COL(255,190,80));
            line_thick(x+2, y-5+bob, x+5, y-2+bob, 2, COL(255,230,170));
            line_thick(x-5, y+2+bob, x-2, y+6+bob, 2, COL(150,100,40));
        } else {   // speed: the station's bolt, dropped size
            tri_fill(x+1, y-7+bob, x-4, y+1+bob, x-1, y+1+bob, COL(140,230,255));
            tri_fill(x-1, y-1+bob, x-4, y+7+bob, x+3, y-1+bob, COL(140,230,255));
        }
    }
    // --- the runner ----------------------------------------------------------------
    if (S->mor_faintt<=0){
        draw_buddy(b);
        if (S->mor_swing_t>0){   // the swing's slash, in the facing arc
            int x=SX(b->x), y=SY(b->y);
            float a0 = b->aim - 0.9f, a1 = b->aim + 0.9f;
            for (int q2=0;q2<=4;q2++){
                float a = a0 + (a1-a0)*q2/4;
                int x1 = x + (int)(fcos(a)*30), y1 = y + (int)(fsin(a)*30);
                int x2 = x + (int)(fcos(a)*48), y2 = y + (int)(fsin(a)*48);
                line_thick(x1, y1, x2, y2, 2, COL(255,250,220));
            }
        }
    }
    if (S->mor_hurt_t>0) blend_rect(vx, vy, vw2, vh2, COL(200,40,30), (int)(90 * S->mor_hurt_t / 0.3f));
    // --- HUD -----------------------------------------------------------------------
    rect_fill(vx, vy, vw2, 20, COL(16,20,28));
    draw_text(vx+8+bw_chip_dx(pi), vy+6, "MORDOR FUN RUN", 1, COL(255,214,120));   // v1.30 (audit C3): clear of the START chip
    { int moose_now = b->mor_dist / MOR_MOOSE_PX, moose_tot = mor_total_moose(pi);
      char buf[64]; char *q=buf;
      q=sapp_int(q, moose_now); q=sappend(q, " / "); q=sapp_int(q, moose_tot); q=sappend(q, " MOOSE");
      *q=0; draw_text_center(vx+vw2/2, vy+6, buf, 1, COL(240,244,250));
      int bw3 = 220, bx = vx+vw2/2-bw3/2, by = vy+15;
      rect_fill(bx-1, by-1, bw3+2, 5, COL(8,8,12));
      int pw = (int)((float)b->mor_dist / (float)total * bw3); if (pw>bw3) pw=bw3;
      rect_fill(bx, by, pw, 3, COL(255,200,60));
      for (int m=1;m<MOR_MILES;m++){ int tick = bx + bw3*m/MOR_MILES; rect_fill(tick, by, 1, 3, COL(16,20,28)); } }
    { int mx2 = mor_maxhp(b), bw3 = 120, bx = vx+vw2-bw3-10, by = vy+6;
      draw_text(bx-32, by, "HP", 1, COL(240,244,250));
      rect_fill(bx-1, by-1, bw3+2, 9, COL(8,8,12));
      int hw = mx2>0? b->mor_hp*bw3/mx2 : 0; if (hw<0) hw=0;
      rect_fill(bx, by, hw, 7, b->mor_hp > mx2/2? COL(90,200,90) : b->mor_hp > mx2/4? COL(240,190,70) : COL(240,80,70)); }
    { char buf[96]; char *q=buf;
      q=sappend(q, "MARKER "); q=sapp_int(q, b->mor_mile); q=sappend(q, "/"); q=sapp_int(q, MOR_MILES);
      q=sappend(q, "   TIME "); q=sapp_int(q, b->mor_secs/60); q=sappend(q, ":"); if (b->mor_secs%60<10) q=sappend(q, "0"); q=sapp_int(q, b->mor_secs%60);
      if (b->mor_mile < MOR_MILES){ q=sappend(q, "   NEXT MARKER ");
          q=sapp_int(q, (int)(((b->mor_mile+1) * mor_spacing(pi) - b->mor_dist) / MOR_MOOSE_PX + 0.5f)); q=sappend(q, " MOOSE"); }
      *q=0;
      rect_fill(vx, vy+21, vw2, 15, COL(16,20,28));
      draw_text_center(vx+vw2/2, vy+24, buf, 1, COL(200,214,200)); }
    if (S->mor_amp_t>0 || S->mor_spd_t>0){   // v1.29: the active buffs, Tank-tag style, with the seconds left
        char buf[64]; char *q=buf;
        if (S->mor_amp_t>0){ q=sappend(q, "[WEAPON x2 "); q=sapp_int(q, (int)(S->mor_amp_t + 0.999f)); q=sappend(q, "S]"); }
        if (S->mor_amp_t>0 && S->mor_spd_t>0) q=sappend(q, "  ");
        if (S->mor_spd_t>0){ q=sappend(q, "[SPEED "); q=sapp_int(q, (int)(S->mor_spd_t + 0.999f)); q=sappend(q, "S]"); }
        *q=0;
        rect_fill(vx, vy+37, vw2, 15, COL(16,20,28));
        draw_text_center(vx+vw2/2, vy+40, buf, 1, S->mor_amp_t>0? COL(255,190,90) : COL(140,230,255));
    }
    if (S->mor_faintt>0) draw_text_center(vx+vw2/2, vy+vh2/3, "DOWN!", 3, COL(255,150,110));
    if (S->mor_cleart>0){
        draw_text_center(vx+vw2/2, vy+vh2/3, "MORDOR!", 3, COL(255,190,90));
        draw_text_center(vx+vw2/2, vy+vh2/3+34, "THE WHOLE ROAD, ON FOOT. THE MOOSE SALUTES YOU.", 1, COL(255,244,210));
    }
    // --- the entry panel -------------------------------------------------------------
    if (S->mor_panel){
        int x,y,w,h; mor_panel_rect(vx,vy,vw2,vh2,&x,&y,&w,&h);
        blend_rect(vx, vy, vw2, vh2, COL(8,8,14), 110);
        rect_fill(x, y, w, h, COL(14,16,30)); rect_line(x, y, w, h, COL(255,200,60));
        draw_text_center(x+w/2, y+10, "MORDOR FUN RUN", 2, COL(255,214,120));
        { char buf[96]; char *q=buf;
          q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[b->species%BUDN]);
          q=sappend(q, " LEFT A RUN ON THIS ROAD:"); *q=0;
          draw_text_center(x+w/2, y+34, buf, 1, COL(200,214,200)); }
        { char buf[96]; char *q=buf;
          q=sapp_int(q, b->mor_dist/MOR_MOOSE_PX); q=sappend(q, " / "); q=sapp_int(q, mor_total_moose(pi)); q=sappend(q, " MOOSE - MARKER ");
          q=sapp_int(q, b->mor_mile); q=sappend(q, " - TIME "); q=sapp_int(q, b->mor_secs/60); q=sappend(q, ":");
          if (b->mor_secs%60<10) q=sappend(q, "0"); q=sapp_int(q, b->mor_secs%60);
          q=sappend(q, " - HP "); q=sapp_int(q, b->mor_hp); *q=0;
          draw_text_center(x+w/2, y+48, buf, 1, COL(240,244,250)); }
        static const char *ROWS[3] = { "RESUME THE RUN", "NEW RUN", "LEAVE (THE RUN WAITS)" };
        for (int r=0;r<3;r++){
            int ry = mor_panel_row_y(y, r);
            const char *nm = (r==1 && S->mor_confirm_t>0)? "NEW RUN? AGAIN TO SCRAP IT" : ROWS[r];
            if (S->mor_psel==r){ rect_fill(x+10, ry-4, w-20, 16, COL(60,48,16)); draw_text(x+16, ry, ">", 1, COL(255,220,90)); }
            draw_text(x+30, ry, nm, 1, S->mor_psel==r? COL(255,244,210) : COL(170,180,190));
        }
        // v1.30 (audit F11): the old text told pad players "START -
        // LEAVE" (START opens the Start menu; leaving is the Back
        // button) and touch players "TAP - LEAVE" (tap-outside is inert
        // here — a tap must land on the LEAVE row).
        if (hint_dev()==DEV_TOUCH)
            draw_text_center(x+w/2, y+h-16, "TAP A ROW TO CHOOSE", 1, COL(120,132,150));
        else { char buf[64]; char *q=buf; q=sappend(q, lbla(H_CONFIRM)); q=sappend(q, " - CHOOSE    "); q=sappend(q, hint_dev()==DEV_PAD? "BACK" : "ESC"); q=sappend(q, " - LEAVE"); *q=0;
          draw_text_center(x+w/2, y+h-16, buf, 1, COL(120,132,150)); }
    }
    if (S->note_t>0 && !S->mor_panel){
        rect_fill(vx, vy+vh2-22, vw2, 22, COL(14,14,20));
        draw_text_center(vx+vw2/2, vy+vh2-15, S->note, 1, COL(255,240,200));
    }
}
