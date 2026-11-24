// THE SURVIVAL GAME — modes/run/run_render.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from run.c lines 598-868 in the Nov 2026 fine-grained refactor; code unchanged.

static void plat_draw_enemy(PlatEn *e, int bob, int strain){
    int x = SX(e->x), y = SY(e->y);
    // v1.22: the grounded kinds sit ON the ground now — the fly and
    // the pipe piranha ride above their ground line, so they skip it
    if (e->kind==0 || e->kind==1 || e->kind==2 || e->kind==5 || e->kind==6)
        shadow_ellipse(x, y+1, 9, 2);
    if (e->kind==7){   // the vault heart, gently bobbing
        int hy = y - 12 + (int)(fsin(e->t*3.0f)*2.0f);
        disc_fill(x-4, hy-3, 5, COL(240,80,90)); disc_fill(x+4, hy-3, 5, COL(240,80,90));
        rect_fill(x-8, hy-3, 17, 6, COL(240,80,90));
        tri_fill(x-8, hy+2, x+9, hy+2, x, hy+11, COL(240,80,90));
        disc_fill(x-3, hy-4, 2, COL(255,190,195));
        return;
    }
    if ((e->kind==0 || e->kind==1) && G.eddie){   // EDDIE MODE (v1.24): the crawler is a puffball here
                                     // too — same footprint, orange fuzz, big red nose
        ellipse_fill(x-5+(bob?1:0), y-1, 4, 2, OUTLINE);
        ellipse_fill(x+5-(bob?1:0), y-1, 4, 2, OUTLINE);
        for (int fz=0; fz<5; fz++) circle_fill(x-8+fz*4, y-12+(fz%2), 3, COL(243,146,44));
        ellipse_fill(x, y-7, 11, 8, OUTLINE);
        ellipse_shade(x, y-7, 10, 7, COL(243,146,44));
        ellipse_shade(x, y-4, 6, 3, COL(255,224,168));
        disc_fill(x-4, y-9, 2, COL(255,255,255)); disc_fill(x+4, y-9, 2, COL(255,255,255));
        disc_fill(x-4, y-9, 1, OUTLINE); disc_fill(x+4, y-9, 1, OUTLINE);
        circle_fill(x, y-6, 4, COL(228,54,46)); pset(x-1, y-7, COL(255,255,255));
    } else if (e->kind==0 || e->kind==1){   // crawlers: a bug with a dome (green for the shell-bug;
                                     // the region's dominant strain colours the plain crawler)
        ellipse_fill(x, y-7, 10, 7, e->kind==1? COL(64,150,70) : (strain>=0? BUG_COL[strain] : COL(96,74,140)));
        if (e->kind==1){ rect_fill(x-10, y-9, 20, 3, COL(50,120,56)); }
        disc_fill(x + (e->vx<0? -5:5), y-8, 2, COL(255,255,255));
        disc_fill(x + (e->vx<0? -5:5), y-8, 1, COL(20,16,24));
        for (int l=0;l<3;l++){ int lx2 = x-6+l*6; line_thick(lx2, y-2, lx2-2, y+ (bob&&l%2? 2:0), 1, COL(40,30,50)); }
    } else if (e->kind==2){          // the shell, at rest or at speed
        ellipse_fill(x, y-5, 10, 6, COL(64,150,70));
        rect_fill(x-10, y-3, 20, 3, COL(220,230,220));
        if (e->state==1){   // spinning: the highlights chase each other around the shell
            int ph = ((int)(e->x*0.45f) % 4 + 4) % 4;
            static const int SPO[4][2] = { {-5,-2}, {0,-4}, {5,-2}, {0,0} };
            disc_fill(x+SPO[ph][0], y-5+SPO[ph][1], 2, COL(255,255,255));
            disc_fill(x-SPO[ph][0], y-5+SPO[(ph+2)&3][1], 2, COL(210,240,210)); }
    } else if (e->kind==3){          // the piranha bug, mid-rise
        int top = SY(e->y - e->vy*34.0f);   // v1.17 fix: this was raw WORLD y used as a screen y — fine only where camy made SY the identity (solo desktop); on touch (deck 84) and split-screen the piranha drew ~3 tiles too low, buried in its pipe, while its bite used the true position
        if (e->vy>0.03f){
            ellipse_fill(x, top+8, 9, 8, COL(196,64,54));
            disc_fill(x-3, top+5, 2, COL(255,255,255)); disc_fill(x+3, top+5, 2, COL(255,255,255));
            int chomp = (e->vy>0.85f && ((int)(e->t*10)&1))? 2 : 0;   // the teeth chomp as it tops out
            for (int t2=0;t2<3;t2++){ rect_fill(x-5+t2*5, top+12, 3, 4+chomp, COL(255,240,220)); }
        }
    } else if (e->kind==4){          // the fly
        int fy2 = y-8 + (int)(fsin(e->t*7)*3);
        ellipse_fill(x, fy2, 8, 6, COL(150,90,60));
        int wf = (int)(fsin(e->t*30)*3);
        ellipse_fill(x-5, fy2-7+wf, 5, 2, COL(230,240,250));
        ellipse_fill(x+5, fy2-7-wf, 5, 2, COL(230,240,250));
        disc_fill(x-4, fy2-1, 2, COL(255,220,80));
    } else if (e->kind==5){          // the patch shroom ambles by
        rect_fill(x-3, y-7, 6, 7, COL(240,235,220));
        ellipse_fill(x, y-8, 9, 6, COL(210,70,60));
        rect_fill(x-1, y-12, 3, 7, COL(255,255,255)); rect_fill(x-3, y-10, 7, 3, COL(255,255,255));
    } else if (e->kind==6){          // the hotfix flower stands its ground
        line_thick(x, y, x, y-10, 2, COL(70,150,70));
        for (int p2=0;p2<5;p2++){ float a = p2*1.2566f + e->t; disc_fill(x + (int)(fcos(a)*5), y-13 + (int)(fsin(a)*5), 3, COL(255,140,40)); }
        disc_fill(x, y-13, 3, COL(255,220,90));
    }
}
static void plat_render(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    float camx = b->x; float half = vw2/2.0f;
    if (camx < half) camx = half;
    if (camx > S->plat_w*PL_TILE - half) camx = S->plat_w*PL_TILE - half;
    G.camx = camx;
    // Nov 15 (Daniel): on the touch stick scheme the joystick and
    // JUMP button sit right on the ground line and swallow the
    // runner. Give the level an 84px deck: the world rides up by
    // that much and a dark stage fills the strip under it, so the
    // controls live on the deck, not on the game. Only the local
    // player's full-width view — split co-op draws no stick HUD.
    int plat_deck = (pi==G.my_id && mobile_ui() && !pad_mode() && G.touch_ctl==1 && vw2>=VW)? 84 : 0;
    G.camy = 180 + plat_deck;
    // sky, then two layers of server skyline drifting behind — the
    // sky wears the level's REGION (the vault borrows the Depths)
    int rg = S->plat_sub? 7 : S->plat_region;
    // THE LIVING SKY (v1.25 feat 12): the sky wears the hour too —
    // bands, skyline base and clouds through tod_sky; the tiles,
    // the runner and the HUD keep their daylight colours.
    rect_fill(vx, vy, vw2, vh2, tod_sky(PLAT_SKY[rg][0]));
    rect_fill(vx, vy+vh2/3, vw2, vh2/3, tod_sky(PLAT_SKY[rg][1]));   // gradient step (glow-up)
    rect_fill(vx, vy+vh2*2/3, vw2, vh2/3, tod_sky(PLAT_SKY[rg][2]));
    // the sun, high and pleased with itself (glow-up) — a pale moon
    // over the dark countries, and over every country at night
    { int sunx = vx + vw2 - 84, suny = vy + 44;
      int dark = (rg==5 || rg==7) || tod_darkf() > 0.5f;
      glow_ellipse(sunx, suny, 34, 34, dark? COL(200,204,225) : COL(255,236,160), 60);
      disc_fill(sunx, suny, 15, dark? COL(226,230,242) : COL(255,240,170));
      disc_fill(sunx-4, suny-4, 6, dark? COL(240,242,250) : COL(255,250,215)); }
    // lazy clouds on the slowest parallax (glow-up)
    { int coff = (int)(camx*0.12f + G.anim_t*3.0f);
      u32 cloudc = tod_sky((rg==5 || rg==7)? COL(126,122,158) : COL(235,246,255));
      for (int sx = vx - (coff % 150) - 150; sx < vx+vw2+150; sx += 150){
          int idx = (coff + sx - vx)/150;
          u32 hh = hash2(idx, 3, 0xC10Du);
          int cy2 = vy + 18 + (int)(hh % 44), cs = 12 + (int)((hh>>6) % 10);
          ellipse_fill(sx+30, cy2, cs+10, cs*2/3, cloudc);
          ellipse_fill(sx+52, cy2-4, cs+4, cs/2, col_shade(cloudc, 1.05f));
          ellipse_fill(sx+12, cy2+2, cs, cs/2, col_shade(cloudc, 0.96f));
      } }
    for (int layer=0; layer<2; layer++){
        float par = layer? 0.65f : 0.35f;
        u32 lc = col_shade(tod_sky(PLAT_SKY[rg][2]), layer? 0.60f : 0.76f);
        int off = (int)(camx*par) % 92;
        for (int sx = vx - off - 92; sx < vx+vw2+92; sx += 92){
            int wx = sx + (int)(camx*par) - vx;
            int hh = 46 + (int)(hash2(wx/92, layer*31+7, 0x51DEu)%54) + layer*16;
            rect_fill(sx, vy+vh2-hh-52, 54, hh, lc);
            for (int wnd=0; wnd<4; wnd++) if ((hash2(wx/92, wnd*3+layer, 0xBEEFu)&3)==0) rect_fill(sx+8+wnd*11, vy+vh2-hh-38, 5, 7, COL(250,230,150));
        }
    }
    int tx0 = (int)((camx-half)/PL_TILE)-1, tx1 = (int)((camx+half)/PL_TILE)+1;
    if (tx0<0) tx0=0; if (tx1>=S->plat_w) tx1=S->plat_w-1;
    for (int ty=0;ty<PL_ROWS;ty++) for (int tx=tx0;tx<=tx1;tx++){
        int t = plat_tile(S, tx, ty); if (!t) continue;
        int x = SX(tx*PL_TILE), y = SY(ty*PL_TILE);
        if (t==1){
            // the earth itself is regional: topsoil in the region's
            // own green (or frost, or neon loam), dirt a dark shade
            // of its second ground tone
            int above = plat_tile(S, tx, ty-1);
            rect_fill(x, y, PL_TILE, PL_TILE, col_shade(REGION_GB[rg], 0.52f));
            rect_fill(x, y, PL_TILE, 4, col_shade(REGION_GB[rg], 0.68f));
            if (above!=1){ rect_fill(x, y, PL_TILE, 8, REGION_GA[rg]); rect_fill(x, y+6, PL_TILE, 3, REGION_GB[rg]); }
        } else if (t==2){
            rect_fill(x+1, y+1, PL_TILE-2, PL_TILE-2, COL(168,102,58));
            rect_line(x+1, y+1, PL_TILE-2, PL_TILE-2, COL(110,62,34));
            line_thick(x+2, y+11, x+PL_TILE-2, y+11, 1, COL(110,62,34));
        } else if (t==3){
            float pulse = 0.9f + 0.1f*fsin(S->plat_t*5 + tx);
            rect_fill(x+1, y+1, PL_TILE-2, PL_TILE-2, col_shade(COL(238,186,54), pulse));
            rect_line(x+1, y+1, PL_TILE-2, PL_TILE-2, COL(160,116,26));
            disc_fill(x+12, y+12, 5, COL(255,240,180)); disc_fill(x+12, y+12, 2, COL(160,116,26));
        } else if (t==4){
            rect_fill(x+1, y+1, PL_TILE-2, PL_TILE-2, COL(146,110,70));
            rect_line(x+1, y+1, PL_TILE-2, PL_TILE-2, COL(100,74,44));
        } else if (t==5){
            rect_fill(x, y, PL_TILE, PL_TILE, col_shade(REGION_GA[rg], 0.78f));
            rect_fill(x, y, PL_TILE, 3, col_shade(REGION_GA[rg], 0.97f));
            disc_fill(x+4, y+4, 1, col_shade(REGION_GA[rg], 0.5f)); disc_fill(x+PL_TILE-5, y+PL_TILE-5, 1, col_shade(REGION_GA[rg], 0.5f));
        } else if (t==6 || t==7){
            rect_fill(x+2, y, PL_TILE-4, PL_TILE, COL(38,148,96));
            rect_fill(x+5, y, 4, PL_TILE, COL(70,180,120));
            if (t==7){
                // the vault pipes (in and out) wear a gold band
                int gold = (S->plat_sub && (tx==32 || tx==33)) || (!S->plat_sub && (tx==S->plat_vault_tx || tx==S->plat_vault_tx+1));
                if (gold){ float pu = 0.9f + 0.1f*fsin(S->plat_t*5);
                    rect_fill(x-1, y, PL_TILE+2, 7, col_shade(COL(238,186,54), pu));
                    rect_fill(x-1, y+5, PL_TILE+2, 2, COL(160,116,26)); }
                else rect_fill(x-1, y, PL_TILE+2, 7, COL(30,128,84));
            }
        } else if (t==8){
            rect_fill(x+10, y, 3, PL_TILE, COL(190,196,204));
            if (ty==4){
                // the flag ripples in the wind, and once the level is
                // cleared it SLIDES down the pole (it used to teleport)
                int slide = S->plat_cleart>0? (int)(fclamp(S->plat_cleart/0.8f, 0.0f, 1.0f)*118) : 0;
                int flagy = y + 2 + slide;
                for (int r=0;r<9;r++){
                    int xo = (int)(fsin(S->plat_t*9 + r*0.85f)*2.0f);
                    u32 fc = ((r + (int)(S->plat_t*6))%3==0)? COL(104,214,110) : COL(84,196,90);
                    rect_fill(x+13+xo, flagy+r, 15-r, 1, fc);
                }
                draw_text(x-8, flagy+12, "DEPLOY", 1, COL(30,90,40)); }
        } else if (t==9){
            rect_fill(x, y, PL_TILE, PL_TILE, COL(110,116,124));
            rect_fill(x, y, PL_TILE, 4, COL(150,156,164));
        } else if (t==10){
            int sq = (int)(fsin(S->plat_t*6 + tx*1.7f)*2.5f);
            ellipse_fill(x+12, y+12, 5-(sq<0?-sq:sq)/2+2, 6, COL(250,205,70));
            disc_fill(x+12, y+12, 2, COL(200,150,40));
        }
    }
    // the deck under the world (touch stick scheme, see above):
    // a dark stage strip the controls can sit on
    if (plat_deck){
        rect_fill(vx, vy + vh2 - plat_deck, vw2, plat_deck, COL(14, 13, 22));
        rect_fill(vx, vy + vh2 - plat_deck, vw2, 2, COL(52, 48, 74));
    }
    // the checkpoint post
    {
        int x = SX(S->plat_cpx), y = SY(PL_GROUND*PL_TILE);
        rect_fill(x, y-34, 3, 34, COL(200,204,210));
        int passed = b->x > S->plat_cpx;
        for (int r=0;r<7;r++) rect_fill(x+3, y-34+r, 11-r, 1, passed? COL(84,196,90) : COL(240,240,240));
    }
    for (int i=0;i<S->plat_en_n;i++) if (S->plat_en[i].kind>=0) plat_draw_enemy(&S->plat_en[i], (int)(S->plat_t*8)&1, REGION_BUG[S->plat_region]);
    for (int i=0;i<8;i++) if (S->plat_fa[i]){
        int x = SX(S->plat_fx[i]), y = SY(S->plat_fy[i]);
        disc_fill(x, y, 5, COL(255,130,40)); disc_fill(x, y, 3, COL(255,200,80));
    }
    // dust puffs
    for (int i=0;i<10;i++) if (S->plat_puff_a[i]){
        int x = SX(S->plat_puff_x[i]), y = SY(S->plat_puff_y[i]);
        int rr = 3 + (int)((0.4f - S->plat_puff_t[i])*16);
        glow_ellipse(x, y, rr, rr*2/3, COL(232,220,196), (int)(S->plat_puff_t[i]/0.4f*120));
    }
    // a ground shadow that shrinks as he flies — reads the height at a glance
    { int gy = -1; int tx2 = (int)(b->x/PL_TILE);
      for (int ty2=(int)(b->y/PL_TILE); ty2<PL_ROWS; ty2++) if (plat_solidt(plat_tile(S, tx2, ty2))){ gy = ty2*PL_TILE; break; }
      if (gy>=0){ float hgt = gy - b->y; int sr = (int)(9 - hgt/18); if (sr<3) sr=3; if (sr>9) sr=9;
          shadow_ellipse(SX(b->x), SY(gy)+2, sr, 2); } }
    // the runner, drawn by the shared buddy brush (feet -> centre)
    if (!(S->plat_grace>0 && ((int)(S->plat_t*14)&1))){
        float savey = b->y; b->y -= 12 - (S->plat_land_t>0? 2 : 0); draw_buddy(b); b->y = savey;   // a landing crouch
        if (S->plat_power==1){ int x=SX(b->x), y=SY(b->y-12); rect_line(x-14, y-14, 28, 28, COL(250,205,70)); }
        if (S->plat_power==2){ int x=SX(b->x), y=SY(b->y-12); rect_line(x-14, y-14, 28, 28, COL(255,130,40)); }
    }
    // HUD
    rect_fill(vx, vy, vw2, 20, COL(16,20,28));
    { char buf[96]; char *q=buf;
      q=sappend(q, "LEVEL "); q=sapp_int(q, S->plat_lvl);
      q=sappend(q, " - "); q=sappend(q, S->plat_sub? "BONUS VAULT" : REGION_NAME[S->plat_region]);
      q=sappend(q, "   SCORE "); q=sapp_int(q, S->plat_score);
      q=sappend(q, "   BITS "); q=sapp_int(q, S->plat_bits);
      if (S->plat_power==1) q=sappend(q, "   PATCHED");
      if (S->plat_power==2) q=sappend(q, "   HOTFIX!");
      *q=0; draw_text(vx+8+bw_chip_dx(pi), vy+6, buf, 1, COL(240,244,250)); }   // v1.30 (audit C3): clear of the START chip
    for (int hh2=0;hh2<S->plat_hearts;hh2++){
        int hx = vx+vw2-16-hh2*16, hy = vy+10;
        disc_fill(hx-3, hy-2, 4, COL(240,80,90)); disc_fill(hx+3, hy-2, 4, COL(240,80,90));
        rect_fill(hx-6, hy-2, 13, 5, COL(240,80,90));
    }
    if (S->plat_cleart>0){
        draw_text_center(vx+vw2/2, vy+vh2/3, "LEVEL CLEAR!", 2, COL(255,244,180));
        // MEDAL TIMES (feat 3): the run's time and the medal it earns —
        // in Zen the time shows but no medal is earned or banked
        char buf[96]; char *q=buf;
        q=sappend(q, "TIME "); q=sapp_time_ds(q, S->plat_show_tds);
        if (!G.zen_run && S->plat_show_tier){ q=sappend(q, " - "); q=sappend(q, medal_name(S->plat_show_tier)); q=sappend(q, " MEDAL!"); if (S->plat_show_newbest) q=sappend(q, "  NEW BEST!"); }
        else if (!G.zen_run && S->plat_lvl<=16){ q=sappend(q, " - NO MEDAL (BRONZE "); q=sapp_time_ds(q, plat_medal[S->plat_lvl-1][2]); q=sappend(q, ")"); }
        else if (G.zen_run) q=sappend(q, " - ZEN: NO MEDALS");
        *q=0;
        draw_text_center(vx+vw2/2, vy+vh2/3 + 22, buf, 1, (!G.zen_run && S->plat_show_tier)? medal_col(S->plat_show_tier) : COL(220,224,230));
    }
    if (S->plat_sel){
        // MEDAL TIMES (feat 3): the level select panel, over the
        // (loaded, frozen) frontier level
        rect_fill(vx+vw2/2-190, vy+vh2/2-64, 380, 128, COL(12,14,22));
        rect_line(vx+vw2/2-190, vy+vh2/2-64, 380, 128, COL(250,200,50));
        draw_text_center(vx+vw2/2, vy+vh2/2-52, "SUPER BUDDY RUN", 2, COL(250,200,50));
        draw_text_center(vx+vw2/2, vy+vh2/2-30, "PICK YOUR LEVEL", 1, COL(255,255,255));
        { char buf[64]; char *q=buf;
          q=sappend(q, "<  LEVEL "); q=sapp_int(q, S->plat_pick); q=sappend(q, "  >"); *q=0;
          draw_text_center(vx+vw2/2, vy+vh2/2-10, buf, 2, COL(255,240,200)); }
        draw_text_center(vx+vw2/2, vy+vh2/2+10, REGION_NAME[(S->plat_pick-1)%8], 1, COL(170,200,190));
        { char buf[96]; char *q=buf; int pk = S->plat_pick;
          if (pk<=16 && b->plat_tbest[pk-1]){ q=sappend(q, "BEST "); q=sapp_time_ds(q, b->plat_tbest[pk-1]); q=sappend(q, " - "); q=sappend(q, medal_name(medal_tier(b->plat_tbest[pk-1], plat_medal[pk-1]))); q=sappend(q, " MEDAL"); }
          else if (pk<=16){ q=sappend(q, "NO TIME YET - GOLD IS "); q=sapp_time_ds(q, plat_medal[pk-1][0]); }
          else q=sappend(q, "BEYOND THE MEDAL BOOK (LEVEL 16+)");
          if (pk == b->plat_done+1) q=sappend(q, "   -   NEW!");
          *q=0;
          draw_text_center(vx+vw2/2, vy+vh2/2+28, buf, 1, (pk<=16 && b->plat_tbest[pk-1])? medal_col(medal_tier(b->plat_tbest[pk-1], plat_medal[pk-1])) : COL(200,214,200)); }
        { char b[80]; char *q=b; q=sappend(q, "LEFT / RIGHT - LEVEL      "); q=sappend(q, lbla(H_FIRE)); q=sappend(q, " - RUN IT"); *q=0;
          draw_text_center(vx+vw2/2, vy+vh2/2+48, b, 1, COL(200,230,200)); }
    }
    // (no standing hint line here any more — controls live in the
    // Start-menu's CONTROLS panel; Daniel, Nov 6)
    if (S->note_t>0){
        rect_fill(vx, vy+vh2-22, vw2, 22, COL(14,14,20));
        draw_text_center(vx+vw2/2, vy+vh2-15, S->note, 1, COL(255,240,200));
    }
}

