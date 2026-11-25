// THE SURVIVAL GAME — war/modes_render.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 3244-3452 in the Nov 2026 fine-grained refactor; code unchanged.

static void bw_render_modes(int pi){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    if (S->bat.active){ bw_render_battle(pi); return; }
    if (b->bw_mode==2){ bw_render_ent(pi); return; }
    if (b->bw_mode==3){ plat_render(pi); return; }
    if (b->bw_mode==4){ craft_render(pi); return; }
    if (b->bw_mode==5){ tank_render(pi); return; }
    if (b->bw_mode==6){ cas_render(pi); return; }
    if (b->bw_mode==7){ if (S->crt_guest) crt_render_view(S->crt_guest - 1, pi); else crt_render(pi); return; }
    if (b->bw_mode==9){ mor_render(pi); return; }
    if (b->bw_mode==10){ scr_render(pi); return; }
    if (b->bw_mode==8){
        if (S->war_guest){   // feat 13: a guest views the OWNER's battle wearing their own view (Cart's crt_render_view, by exchange)
            BwSim *HS = &BWS[S->war_guest-1];
            war_view_exchange(HS, S);
            war_view_pi = pi;
            war_render(S->war_guest-1);
            war_view_exchange(HS, S);
            war_view_pi = -1;
        } else { war_view_pi = pi; war_render(pi); war_view_pi = -1; }
        return;
    }
    u32 seed=(u32)b->bw_seed;
    G.camx=b->x; G.camy=b->y;
    // the view: full screen solo, or this player's split half in co-op
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    rect_fill(vx, vy, vw2, vh2, COL(84,156,62));
    int tx0=bw_tileof(b->x - vw2/2 - BW_TILE), tx1=bw_tileof(b->x + vw2/2 + BW_TILE);
    int ty0=bw_tileof(b->y - vh2/2 - BW_TILE), ty1=bw_tileof(b->y + vh2/2 + BW_TILE);
    for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
        int x=SX(tx*BW_TILE), y=SY(ty*BW_TILE);
        u32 h = hash2(tx, ty, seed ^ 0x7A55u);
        // palette lift (Nov 3, Daniel: "kinda drab"): a checker you can
        // actually see, per-tile shade weather, and sunlit patches
        int site = bw_site_at_xy(b, tx*BW_TILE+24, ty*BW_TILE+24);
        int ocean = bw_outside_world(tx*BW_TILE+24, ty*BW_TILE+24);
        { u32 base;
          if (ocean){
              // the sea: two blues in a checker, lighter in the
              // shallows within a tile or so of the shore
              float wx2 = tx*BW_TILE+24, wy2 = ty*BW_TILE+24;
              int shallow = wx2 > BW_WX0-84 && wx2 < BW_WX1+84 && wy2 > BW_WY0-84 && wy2 < BW_WY1+84;
              base = shallow? (((tx+ty)&1)==0? COL(74,146,196) : COL(66,136,186))
                            : (((tx+ty)&1)==0? COL(36,88,146) : COL(32,80,136));
              rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(base, 0.94f + (float)((h>>20)%14)*0.01f));
              if (h % 7 == 0){ int wxp = x + (int)((h>>6) % 30) + 6 + (int)(G.anim_t*6 + (h%40)) % 13 - 6,
                                  wyp = y + (int)((h>>11) % 32) + 8 + (int)(fsin(G.anim_t*1.3f + (float)(h%9))*1.5f);
                  line_thick(wxp, wyp, wxp+4, wyp-2, 1, COL(150,200,235)); line_thick(wxp+4, wyp-2, wxp+8, wyp, 1, COL(150,200,235)); }
              continue;
          }
          float road_d = (b->bw_mode==1 && site<0)? bw_road_dist(tx*BW_TILE+24, ty*BW_TILE+24) : 1e9f;
          if (road_d < 42)  base = road_d<26? (((tx+ty)&1)==0? COL(170,150,98) : COL(160,140,90))    // the worn road itself
                                            : (((tx+ty)&1)==0? COL(138,166,86) : COL(128,156,78));   // its trampled edge
          else if (site==0) base = ((tx+ty)&1)==0? COL(182,156,102) : COL(172,146,94);
          else if (site==1) base = ((tx+ty)&1)==0? COL(132,134,132) : COL(122,124,122);
          else if (site==2) base = ((tx+ty)&1)==0? COL(178,216,236) : COL(166,204,226);
          else if (site==3) base = ((tx+ty)&1)==0? COL(102,98,104) : COL(92,88,94);
          else if (site==4) base = ((tx+ty)&1)==0? COL(126,136,100) : COL(116,126,92);
          else { base = ((tx+ty)&1)==0? COL(104,178,80) : COL(84,156,62); if (h % 19 == 0) base = COL(126,192,86); }
          rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(base, 0.94f + (float)((h>>20)%14)*0.01f)); }
        // terrain grain on the open grass (Daniel, Nov 6): worn dirt
        // patches and speckle, so the meadow reads as GROUND, not as
        // one flat green sheet — kept quiet on purpose
        if (!ocean && site<0){
            if ((h>>3)%11==0){ int px2 = x + (int)((h>>6)%26)+8, py2 = y + (int)((h>>11)%24)+10;
                disc_fill(px2, py2, 6, COL(156,138,92)); disc_fill(px2+7, py2+2, 4, COL(148,130,86)); disc_fill(px2-6, py2+3, 4, COL(150,134,90)); }
            if ((h>>5)%3==0){ pset(x + (int)((h>>7)%40)+4, y + (int)((h>>13)%40)+4, COL(70,138,52));
                pset(x + (int)((h>>17)%40)+4, y + (int)((h>>23)%40)+4, COL(140,205,105)); }
        }
        int lush = bw_lush_at(tx, ty, seed);
        if (!ocean && site!=1 && site!=2 && site!=3){ // grass tufts, thick where it's lush
            int tufts = lush==2? (h%5==0? 0 : 4) : lush==1? (h%3==0? 3 : (h%3==1? 2 : 0)) : (h%6==0? 2 : 0);
            for (int g2=0; g2<tufts; g2++){ u32 hh = hash2(tx*7+g2*13, ty*7-g2*11, seed ^ 0x6A11u);
                int gx = x + (int)(hh % 38) + 5, gy = y + (int)((hh>>8) % 36) + 7;
                line_thick(gx, gy, gx-1, gy-5, 1, COL(58,124,46)); line_thick(gx+3, gy, gx+4, gy-4, 1, COL(134,202,100)); } }
        if (!ocean && (site<0 || site==0) && lush>=1 && h % (lush==2? 9 : 16) == 0){ // a bush: clustered dark discs, one lit top
            int bx2 = x + (int)((h>>7) % 30) + 9, by2 = y + (int)((h>>13) % 28) + 10;
            disc_fill(bx2-5, by2+2, 5, COL(44,102,42)); disc_fill(bx2+5, by2+2, 5, COL(44,102,42));
            disc_fill(bx2, by2-2, 6, COL(52,116,48)); disc_fill(bx2-2, by2-4, 3, COL(86,150,70)); }
        if (!ocean && site<0 && lush==2 && (h>>5) % 14 == 0){ // mushrooms in the deep green only
            int mx2 = x + (int)((h>>9) % 32) + 8, my2 = y + (int)((h>>15) % 30) + 9;
            pset(mx2, my2, COL(240,238,228)); pset(mx2, my2-1, COL(240,238,228));
            disc_fill(mx2, my2-3, 2, COL(206,64,54)); pset(mx2-1, my2-3, COL(255,240,230));
            pset(mx2+6, my2+2, COL(240,238,228)); disc_fill(mx2+6, my2, 1, COL(206,64,54)); }
        if (!ocean && h % (lush==2? 6 : lush==1? 11 : 33) == 0 && (site<0 || site==0)){ // flower clusters, not pinpricks: three petals + a heart
            static const u32 FLW[4] = { COL(255,216,90), COL(255,150,190), COL(190,160,255), COL(250,250,246) };
            u32 fc = FLW[(h>>16)&3];
            int fx2 = x + (int)((h>>6) % 34) + 6, fy2 = y + (int)((h>>11) % 34) + 7;
            disc_fill(fx2-2, fy2, 1, fc); disc_fill(fx2+2, fy2, 1, fc); disc_fill(fx2, fy2-2, 1, fc);
            pset(fx2, fy2, COL(255,240,190)); }
    }
    // the shoreline (glow-up): a dashed foam line where meadow
    // meets sea, crawling slowly so the water feels alive
    { int ph = (int)(G.anim_t*2.0f);
      int fx0 = SX(BW_WX0), fx1 = SX(BW_WX1), fy0 = SY(BW_WY0), fy1 = SY(BW_WY1);
      int vy0 = vy, vy1 = vy+vh2, vx0 = vx, vx1 = vx+vw2;
      if (fx0>=vx0 && fx0<vx1) for (int yy=vy0; yy<vy1; yy+=7) if ((((yy/7)+ph)&1)==0) rect_fill(fx0-1, yy, 2, 4, COL(228,244,252));
      if (fx1>=vx0 && fx1<vx1) for (int yy=vy0; yy<vy1; yy+=7) if ((((yy/7)+ph)&1)==0) rect_fill(fx1-1, yy, 2, 4, COL(228,244,252));
      if (fy0>=vy0 && fy0<vy1) for (int xx=vx0; xx<vx1; xx+=7) if ((((xx/7)+ph)&1)==0) rect_fill(xx, fy0-1, 4, 2, COL(228,244,252));
      if (fy1>=vy0 && fy1<vy1) for (int xx=vx0; xx<vx1; xx+=7) if ((((xx/7)+ph)&1)==0) rect_fill(xx, fy1-1, 4, 2, COL(228,244,252)); }
    bw_draw_gate();
    bw_draw_objects(pi, 0);
    draw_buddy(b);
    bw_draw_objects(pi, 1);
    ui_vignette(vx, vy, vw2, vh2);
    // THE LIVING SKY (v1.25 feat 12): the same tint the overworld
    // wears, laid over the meadow but under the buddy card, the
    // prompt, the minimap and the note — BW's UI never sits under
    // the sky. Only this outdoor fall-through reaches here: every
    // cabinet and interior mode returned above, Entrances included.
    tod_apply_tint();
    // the buddy card
    rect_fill(vx+8, vy+8, 168, 46, COL(20,26,20)); rect_line(vx+8, vy+8, 168, 46, COL(198,164,84));
    { char buf[64]; char *q=buf; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[b->species%BUDN]); *q=0;
      draw_text(vx+14, vy+13, buf, 1, COL(255,244,210)); }
    { float f = b->maxhp>0? b->hp/b->maxhp : 0; if (f<0) f=0;
      rect_fill(vx+14, vy+26, 120, 5, COL(40,30,40)); rect_fill(vx+14, vy+26, (int)(120*f), 5, f>0.35f? COL(120,230,90) : COL(255,170,60));
      char hb[24]; char *q=hb; q=sapp_int(q, (int)b->hp); q=sappend(q, "/"); q=sapp_int(q, (int)b->maxhp); *q=0;
      draw_text(vx+138, vy+25, hb, 1, COL(230,240,230)); }
    { float f = b->energy/BUD_ENERGY_MAX; if (f<0) f=0;
      rect_fill(vx+14, vy+36, 120, 4, COL(40,30,40)); rect_fill(vx+14, vy+36, (int)(120*f), 4, COL(120,200,255));
      draw_text(vx+138, vy+35, "ENERGY", 1, COL(170,220,255)); }
    // the context prompt (Daniel, Nov 6): ONE line, only when there is
    // something to interact with, naming the actual key/button for
    // the device in hand — never the whole control list
    { const char *verb = 0;
      float dx=b->x, dy=b->y+6;
      if (bw_npc_near(b, 46)>=0) verb = "TALK";
      else if (bw_term_near(b, 44)>=0) verb = "PRESS THE TERMINAL";
      else if (bw_sign_near(b, 44)) verb = "READ THE SCRATCHES";
      else if (bw_hsign_near(b, 44)>=0) verb = "READ THE SIGN";
      else if (bw_bitling_near(b, 44)) verb = "SAY HI";
      else if (bw_tool_near(b, 44)>=0) verb = "PICK IT UP";
      else if (dx*dx+dy*dy < 46*46) verb = "BACK TO THE BUG MALL";
      else { float tx, ty; if (bw_tent_near(b, 42, &tx, &ty)) verb = "SLEEP (FULL HP + ENERGY)"; }
      if (verb){ char hb[72]; const char *hint = key_prompt(hb, lbl_int(), verb);
      int hw2 = text_w(hint, 1);
      rect_fill(vx + vw2/2 - hw2/2 - 6, vy + vh2-22, hw2+12, 14, COL(20,26,20));
      draw_text_center(vx + vw2/2, vy + vh2-19, hint, 1, COL(240,248,240)); } }
    bw_draw_minimap(pi);
    bw_draw_note(pi);
    bw_draw_ch2_hud(pi);   // THE REBUILDING (feat 14): the cider run's countdown, when a run is live
    if (S->log_open){
        static const char *QN[10] = { "MEET ELDER PATCHINA", "CLEAR THE SERVER FARM", "HOUSE CALL FOR A HERMIT", "ZERO-DAY FUNERAL", "MOP'S LOST TOOLS", "THE SUNKEN CACHE", "COLD STORAGE", "SPECIAL DELIVERY", "THE STRAY PROCESS", "READ THE SIGNS" };
        // a guiding line per quest (Nov 4, Daniel: "more detailed,
        // clearer — don't give it all away"): shown while ACTIVE only
        static const char *QD[10] = {
            "THE ELDER WAITS IN PATCH VILLAGE, NORTH-WEST.",
            "BONK 3 HACKERS AT THE FARM. THEN MIND THE TERMINALS.",
            "GLITCH LIVES ALONE AT THE FROZEN LAKE, FAR SOUTH.",
            "ZED WAITS PAST HIS FENCE. GATES OPEN FOR CRANKS.",
            "TOOLS GLINT GOLD OUT THERE. THE DIAMOND FINDS ONE.",
            "THE HOARDER SITS IN THE WESTERN RUINS. 4TH HITS HEAL IT.",
            "BONK 2 HACKERS ON THE FROZEN LAKE ITSELF.",
            "CARRY MOP'S PARCEL TO GLITCH AT THE LAKE.",
            "A STRAY PROCESS WANDERS SOUTH OF PATCH VILLAGE.",
            "READ SCRATCHED SIGNS BY ROADS AND PASSES. ANY THREE." };
        int w = vw2<420? vw2-16 : 404;
        int need = 26 + 4 + 14 + 4*13 + 16;
        if (b->bw_mode!=1) need = 26 + 14 + 4 + 14 + 4*13 + 16;
        else for (int q2=0; q2<10; q2++){ int st = bw_qstate_g(b, q2); if (st) need += st==1? 23 : 12; }
        int h = need; if (h > vh2-8) h = vh2-8; if (h < 150) h = 150;
        int x = vx + (vw2-w)/2, y = vy + (vh2-h)/2; if (y < vy+4) y = vy+4;
        rect_fill(x, y, w, h, COL(16,20,18)); rect_line(x, y, w, h, COL(198,164,84));
        draw_text(x+12, y+8, "QUEST LOG", 1, COL(255,214,64));
        { char hb[40]; char *q=hb; q=sappend(q, lbla(H_WALL)); q=sappend(q, " - CLOSE"); *q=0;
          draw_text(x+w-text_w(hb,1), y+8, hb, 1, COL(150,170,190)); }   // v1.30 (audit F12): was a hard-coded "F / X" for every device
        int ry = y + 26;
        if (b->bw_mode!=1){
            draw_text(x+12, ry, "NO STORY HERE. JUST MEADOW, TEA, AND VIOLENCE.", 1, COL(200,214,200)); ry += 14;
        } else for (int q2=0; q2<10; q2++){
            int st = bw_qstate_g(b, q2);
            if (st==0) continue;
            if (ry > y+h-86) break;   // keep room for the backpack below
            draw_text(x+12, ry, QN[q2], 1, st==2? COL(140,160,140) : COL(255,244,210));
            char tb[28]; char *tq=tb;
            if (st==2){ tq=sappend(tq, "DONE"); }
            else if (q2==1){ tq=sappend(tq, "ACTIVE "); tq=sapp_int(tq, bw_farmkills(b)); tq=sappend(tq, "/3"); }
            else if (q2==4){ tq=sappend(tq, "ACTIVE "); tq=sapp_int(tq, bw_tools_found(b)); tq=sappend(tq, "/3"); }
            else if (q2==6){ tq=sappend(tq, "ACTIVE "); tq=sapp_int(tq, bw_lakekills(b)); tq=sappend(tq, "/2"); }
            else if (q2==9){ tq=sappend(tq, "ACTIVE "); tq=sapp_int(tq, bw_signs_read(b)); tq=sappend(tq, "/3"); }
            else tq=sappend(tq, "ACTIVE");
            *tq=0;
            draw_text(x+w-12-text_w(tb, 1), ry, tb, 1, st==2? COL(120,200,120) : COL(255,200,110));
            ry += 12;
            if (st==1){ draw_text(x+24, ry, QD[q2], 1, COL(168,190,205)); ry += 11; }
        }
        ry += 4;
        draw_text(x+12, ry, "BACKPACK (BW ONLY)", 1, COL(255,214,64)); ry += 14;
        for (int it=0; it<4; it++){
            if (it==S->log_sel){ rect_fill(x+8, ry-2, w-16, 13, COL(70,58,30)); draw_text(x+12, ry, ">", 1, COL(255,220,90)); }
            draw_text(x+24, ry, BW_ITEM_NAME[it], 1, b->items[it]>0? COL(240,248,240) : COL(110,110,120));
            char cb[12]; char *cq=cb; cq=sappend(cq, "x"); cq=sapp_int(cq, b->items[it]); *cq=0;
            draw_text(x+w-12-text_w(cb, 1), ry, cb, 1, COL(255,210,130));
            ry += 13;
        }
        { char b2[64]; char *q=b2; q=sappend(q, lbla(H_FIRE)); q=sappend(q, " - USE.  ITEMS ALSO WORK MID-BATTLE."); *q=0;
          draw_text_center(x+w/2, y+h-14, b2, 1, COL(150,170,190)); }
    }
    if (pi==G.my_id && mobile_ui() && !pad_mode() && !S->log_open){
        const char *hint = "STICK MOVE - USE TALKS - PICK FOR BATTLES";
        int hw2 = text_w(hint, 1);
        rect_fill(vx + vw2/2 - hw2/2 - 6, vy + vh2-22, hw2+12, 14, COL(20,26,20));
        draw_text_center(vx + vw2/2, vy + vh2-19, hint, 1, COL(240,248,240));
    }
    if (S->sleep_t>0){
        float d = 1.0f - (S->sleep_t>0.7f? (S->sleep_t-0.7f)/0.7f : S->sleep_t/0.7f);
        blend_rect(vx, vy, vw2, vh2, COL(6,6,16), (int)(210*d));
        if (d>0.75f) draw_text_center(vx + vw2/2, vy + vh2/2-4, "Z Z Z", 2, COL(200,210,255));
    }
}
