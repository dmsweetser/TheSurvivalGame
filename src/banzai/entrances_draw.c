// THE SURVIVAL GAME — banzai/entrances_draw.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from banzai.c lines 2560-2870 in the Nov 2026 fine-grained refactor; code unchanged.

// --- ENTRANCES drawing ---
static void ent_draw_door(int x, int y, const char *plate, int fake, int duet){
    rect_fill(x+6, y+2, BW_TILE-12, BW_TILE-2, COL(104,72,40));
    rect_line(x+6, y+2, BW_TILE-12, BW_TILE-2, COL(56,36,20));
    rect_fill(x+10, y+20, BW_TILE-20, 3, COL(84,56,30));
    disc_fill(x+BW_TILE-13, y+26, 2, COL(230,196,90));
    if (duet){   // DEEPER DOORS (feat 11): a double door — centre seam, a knob for each friend
        rect_fill(x+BW_TILE/2-1, y+3, 2, BW_TILE-5, COL(56,36,20));
        disc_fill(x+BW_TILE/2-6, y+26, 2, COL(230,196,90));
    }
    rect_fill(x+13, y+6, BW_TILE-26, 11, COL(198,164,84));
    draw_text_center(x+BW_TILE/2, y+8, plate, 1, COL(40,30,10));
    // painted doors used to wear a red plate and a crack — a free
    // tell. Now a dupe is drawn exactly like the real thing (Daniel,
    // Nov 6: less obvious); only walking into it gives it away.
    (void)fake;
}
static void bw_render_ent(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    G.camx=b->x; G.camy=b->y;
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    rect_fill(vx, vy, vw2, vh2, COL(16,12,20));
    int tx0=bw_tileof(b->x - vw2/2 - BW_TILE), tx1=bw_tileof(b->x + vw2/2 + BW_TILE);
    int ty0=bw_tileof(b->y - vh2/2 - BW_TILE), ty1=bw_tileof(b->y + vh2/2 + BW_TILE);
    // tiles + floor-level dressing
    for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
        int x=SX(tx*BW_TILE), y=SY(ty*BW_TILE);
        if (x+ BW_TILE<vx || x>vx+vw2 || y+BW_TILE<vy || y>vy+vh2) continue;
        int t = ent_tile(pi, tx, ty);
        int k = tx>=0? tx/ENT_STRIDE + 1 : 1, lx = tx>=0? tx%ENT_STRIDE : 0;
        u32 h = hash2(tx, ty, (u32)b->bw_seed ^ 0x7A55u);
        if (t==1){ if (S->ent_sub){ rect_fill(x, y, BW_TILE+1, BW_TILE+1, COL(50,50,56)); rect_fill(x, y, BW_TILE+1, 7, COL(72,72,78)); rect_fill(x, y+BW_TILE-5, BW_TILE+1, 5, COL(36,36,40)); }
                   else if (S->ent_chap==2){ rect_fill(x, y, BW_TILE+1, BW_TILE+1, COL(24,40,48)); rect_fill(x, y, BW_TILE+1, 7, COL(40,62,70)); rect_fill(x, y+BW_TILE-5, BW_TILE+1, 5, COL(17,29,35)); }   // DEEPER DOORS (feat 11): the same hotel, one floor of mood down
                   else { rect_fill(x, y, BW_TILE+1, BW_TILE+1, COL(47,37,54)); rect_fill(x, y, BW_TILE+1, 7, COL(68,54,72)); rect_fill(x, y+BW_TILE-5, BW_TILE+1, 5, COL(33,26,39)); }
                   // a panel seam, wandering per tile (glow-up)
                   rect_fill(x + 6 + (int)((h>>8)%28), y+8, 1, BW_TILE-14, S->ent_sub? COL(41,41,45) : S->ent_chap==2? COL(19,32,38) : COL(37,28,43));
                   continue; }
        // floor: lamplit wood, a warmer checker on landings, confetti past the end
        { u32 base = S->ent_sub? (((tx+ty)&1)==0? COL(72,72,68) : COL(65,65,62))
                              : S->ent_chap==2? ((k%10==0)? (((tx+ty)&1)==0? COL(74,74,52) : COL(67,67,47))
                                                           : (((tx+ty)&1)==0? COL(52,68,60) : COL(47,62,55)))
                              : (k%10==0)? (((tx+ty)&1)==0? COL(100,76,47) : COL(92,70,42))
                              : (((tx+ty)&1)==0? COL(84,62,41) : COL(77,57,37));
          if (!S->ent_sub && k==25 && S->ent_chap!=2) base = ((tx+ty)&1)==0? COL(80,55,45) : COL(74,50,42);
          rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(base, 0.94f + (float)((h>>20)%12)*0.01f));
          rect_fill(x, y+BW_TILE-2, BW_TILE+1, 2, COL(70,52,36));
          // a welcome mat in front of every door (glow-up): the hotel
          // is spooky, not rude
          if (t==0 && !S->ent_sub){
              int nd = ent_tile(pi, tx+1, ty)==2 || ent_tile(pi, tx-1, ty)==2 || ent_tile(pi, tx, ty+1)==2 || ent_tile(pi, tx, ty-1)==2
                    || ent_tile(pi, tx+1, ty)==7 || ent_tile(pi, tx-1, ty)==7 || ent_tile(pi, tx, ty+1)==8 || ent_tile(pi, tx, ty-1)==8;
              if (nd){ rect_fill(x+11, y+31, 26, 11, COL(112,44,44)); rect_line(x+11, y+31, 26, 11, COL(84,32,32)); }
          } }
        if (k==ent_final_room(S)){ static const u32 CONFC[4] = { COL(255,120,150), COL(120,220,255), COL(255,214,64), COL(150,255,150) };
            for (int c2=0;c2<3;c2++){ u32 hh = hash2(tx*3+c2, ty*5-c2, 0xC04Fu); pset(x + (int)(hh%40) + 4, y + (int)((hh>>6)%40) + 4, CONFC[hh%4]); } }
        if (t==2 || t==7 || t==8){
            int door = (lx==ENT_STRIDE-1)? k : k-1;
            int duet = (t==2 && lx==ENT_STRIDE-1 && ent_duet_door(pi, door));   // DEEPER DOORS (feat 11)
            char plate[8];
            if (t==8) scpy(plate, S->ent_sub? "UP" : "OUT", 8);
            else if (t==7) scpy(plate, "EXIT", 8);
            else if (!S->ent_sub && door+1==ent_final_room(S)) scpy(plate, "EXIT", 8);
            else if (duet) scpy(plate, "DUET", 8);
            else if (S->ent_sub){ char *q=plate; q=sappend(q, "B"); q=sapp_int(q, door+1); *q=0; }
            else if (S->ent_chap==2){ char *q=plate; q=sappend(q, "D"); q=sapp_int(q, door+1); *q=0; }
            else { char *q=plate; q=sapp_int(q, door+1); *q=0; }
            int fake = (t==2 && lx==ENT_STRIDE-1 && !S->ent_sub && ent_dupe_room(b, k) && (ty==ent_dupe_ly_i(b, k, 0) || (ent_dupe_n(b, k)>1 && ty==ent_dupe_ly_i(b, k, 1))));
            ent_draw_door(x, y, plate, fake, duet);
            if (t==2 && lx==ENT_STRIDE-1 && door>=1 && ty==ent_door_ly(b, door) && !S->ent_dupe_used){
                // the guiding moths hover at the REAL door — the hotel
                // is a kind place, if you look
                for (int m2=0;m2<2;m2++){ float ph = G.anim_t*2.2f + m2*2.4f;
                    int mx2 = x + 24 + (int)(fsin(ph)*13), my2 = y + 8 + (int)(fcos(ph*1.3f)*4);
                    disc_fill(mx2, my2, 2, COL(255,246,190)); }
            }
            continue;
        }
        if (t==3){ if (S->ent_sub){ rect_fill(x+6, y+10, BW_TILE-12, 28, COL(38,40,44)); rect_line(x+6, y+10, BW_TILE-12, 28, COL(90,94,100));
                for (int s2=0;s2<4;s2++) rect_fill(x+9, y+14+s2*6, BW_TILE-18, 3, COL(16,17,20)); continue; }
            int bx2 = x+24, by2 = y+26;
            disc_fill(bx2-6, by2+3, 6, COL(44,102,42)); disc_fill(bx2+6, by2+3, 6, COL(44,102,42));
            disc_fill(bx2, by2-2, 7, COL(52,116,48)); disc_fill(bx2-2, by2-5, 3, COL(86,150,70)); continue; }
        if (t==4){ int looted = ent_room_looted(S, S->ent_sub? k+1000 : k);
            rect_fill(x+7, y+12, BW_TILE-14, 24, S->ent_sub? COL(96,92,86) : COL(122,88,52)); rect_line(x+7, y+12, BW_TILE-14, 24, S->ent_sub? COL(52,50,46) : COL(70,48,26));
            rect_fill(x+10, y+ (looted? 10 : 15), BW_TILE-20, 4, S->ent_sub? COL(70,68,64) : COL(96,66,36));
            disc_fill(x+24, y+26, 2, COL(230,196,90)); continue; }
        if (t==5){ if (S->ent_sub){ rect_fill(x+4, y+2, BW_TILE-8, BW_TILE-2, COL(52,54,58));
                for (int p2=0;p2<3;p2++){ rect_fill(x+10+p2*12, y+4, 5, BW_TILE-8, COL(96,100,106)); rect_fill(x+10+p2*12, y+16, 5, 3, COL(60,62,66)); } continue; }
            rect_fill(x+3, y+2, BW_TILE-6, BW_TILE-2, COL(96,64,40)); rect_line(x+3, y+2, BW_TILE-6, BW_TILE-2, COL(56,36,20));
            for (int bk=0;bk<4;bk++){ u32 hh = hash2(tx*9+bk, ty*7, 0xB00Cu);
                rect_fill(x+8+bk*9, y+8 + (int)(hh%12), 5, 16, (hh&1)? COL(150,70,60) : COL(70,110,150)); } continue; }
        if (t==6){ bw_draw_tent(tx*BW_TILE + 24, ty*BW_TILE + 24); continue; }
        if (t==9){ rect_fill(x+3, y+1, BW_TILE-6, BW_TILE-1, COL(74,50,30)); rect_line(x+3, y+1, BW_TILE-6, BW_TILE-1, COL(40,26,14));
            for (int bk=0;bk<5;bk++){ u32 hh = hash2(tx*7+bk, ty*3, 0xB00Cu);
                rect_fill(x+7+bk*7, y+6 + (int)(hh%8), 4, 18, (hh&1)? COL(150,70,60) : COL(70,110,150)); }
            for (int bk=0;bk<5;bk++){ u32 hh = hash2(tx*11+bk, ty*5, 0xB00Du);
                rect_fill(x+7+bk*7, y+28 + (int)(hh%6), 4, 15, (hh&1)? COL(90,130,80) : COL(160,120,60)); } continue; }
    }
    int room = S->ent_room;
    // the key, waiting on its drawer
    if (!S->ent_key && room > (S->ent_sub? S->ent_door_sub : S->ent_door_upto) && ent_door_locked(pi, room)){
        float kx, ky; ent_key_pos(pi, room, &kx, &ky);
        int x=SX(kx), y=SY(ky) + (int)(fsin(G.anim_t*3)*2);
        disc_fill(x, y, 4, COL(255,214,64)); disc_fill(x, y, 2, COL(255,240,170));
        rect_fill(x+3, y-1, 7, 2, COL(255,214,64));
    }
    // library fuses
    if (room==25 && S->ent_ev==6){
        for (int i=0;i<5;i++){
            if (S->ent_fuse_mask>>i & 1) continue;
            int x=SX(ent_room_x0(25) + ENT_FUSE_TX[i]*BW_TILE + 24), y=SY(ENT_FUSE_TY[i]*BW_TILE + 24);
            disc_fill(x, y, 6, COL(60,120,60)); rect_fill(x-3, y-6, 6, 12, COL(120,255,140)); disc_fill(x, y-8, 2, COL(220,255,220));
        }
    }
    // DEEPER DOORS (feat 11): an unopened duet door's brass plates,
    // glowing under whoever is holding them, and the charge pip at
    // the door while the two of them stand together
    if (S->ent_chap==2 && ent_duet_door(pi, room) && room > S->ent_door_upto){
        for (int pl=0; pl<2; pl++){
            float wx, wy; ent_plate_pos(pi, room, pl, &wx, &wy);
            int x=SX(wx), y=SY(wy), held = 0;
            { float dx=b->x-wx, dy=b->y-wy; if (dx*dx+dy*dy < 22*22) held = 1; }
            if (!held){ float dx=S->ent_lux-wx, dy=S->ent_luy-wy; if (dx*dx+dy*dy < 16*16) held = 1; }
            if (held) glow_ellipse(x, y, 20, 12, COL(255,190,90), 60);
            disc_fill(x, y, 11, COL(150,116,52)); disc_fill(x, y, 8, held? COL(230,196,90) : COL(178,146,74));
            disc_fill(x, y, 3, COL(120,90,40));
        }
        if (S->ent_duet_t > 0){
            int dx2 = SX(ent_room_x0(room) + (ENT_STRIDE-1)*BW_TILE + 24), dy2 = SY(ent_doory(b, room));
            int rr = 3 + (int)(S->ent_duet_t/0.6f * 6);
            disc_fill(dx2, dy2, rr+3, COL(255,214,64)); disc_fill(dx2, dy2, rr, COL(104,72,40));
        }
    }
    // chase fires, licking at the carpet (only ever in Inspector rooms)
    if (S->ent_ev==5){
        for (int i=0;i<S->ent_fn;i++){
            int x=SX(S->ent_fx[i]), y=SY(S->ent_fy[i]);
            int fl = (int)(fsin(G.anim_t*11 + i*1.7f)*2);
            glow_ellipse(x, y+2, 21, 13, COL(255,120,40), 44);
            ellipse_fill(x, y+4, 13, 6, COL(110,38,14));
            line_thick(x-6, y+5, x-1, y-9-fl, 3, COL(255,110,30));
            line_thick(x-1, y-9-fl, x+3, y+5, 3, COL(255,110,30));
            line_thick(x+3, y+5, x+6, y-5+fl, 3, COL(255,165,55));
            disc_fill(x, y-1, 3, COL(255,224,130));
        }
    }
    // residents, split around the buddy by depth
    for (int pass=0; pass<2; pass++){
        if (pass==0){
            // (props already sit in the tile pass; entities sort by y)
        }
        // the Zip Bug: a hacker at full sprint, with speed lines
        if ((S->ent_ev==1 || S->ent_ev==2 || S->ent_ev==7) && S->ent_phase==2 && ((S->ent_zy < b->y) == (pass==0))){
            bw_draw_hacker(S->ent_zx, S->ent_zy, 3, 0, 0);
            if (S->ent_sub && S->ent_ev==2){   // the Multibug is a crowd pretending to be one bug
                bw_draw_hacker(S->ent_zx - S->ent_zdir*26, S->ent_zy - 15, 4, 0, 0);
                bw_draw_hacker(S->ent_zx - S->ent_zdir*20, S->ent_zy + 13, 2, 0, 0);
            }
            int zx=SX(S->ent_zx), zy=SY(S->ent_zy);
            for (int l2=1;l2<4;l2++) line_thick(zx - (int)S->ent_zdir*(14+l2*7), zy-4+l2*3, zx - (int)S->ent_zdir*(20+l2*7), zy-4+l2*3, 1, COL(220,230,240));
        }
        if (S->ent_ev==3 && S->ent_phase==2 && ((S->ent_tyy < b->y) == (pass==0))){
            int x=SX(S->ent_tyx), y=SY(S->ent_tyy);
            disc_fill(x, y, 6, COL(196,120,210));
            glow_ellipse(x, y-2, 8, 5, COL(80,160,255), 70);
            disc_fill(x-2, y-2, 2, COL(150,220,255)); disc_fill(x+2, y-2, 2, COL(150,220,255));
            draw_text_center(x, y-20, "psst", 1, COL(240,220,255));
        }
        if (S->ent_ev==4 && ((S->ent_rvy < b->y) == (pass==0))){
            int x=SX(S->ent_rvx), y=SY(S->ent_rvy) + (int)(fsin(G.anim_t*2)*2);
            disc_fill(x, y, 9, COL(240,240,235)); disc_fill(x-8, y-6, 5, COL(240,240,235)); disc_fill(x+8, y-6, 5, COL(240,240,235));
            disc_fill(x, y, 4, COL(90,170,255)); disc_fill(x-8, y-6, 3, COL(90,170,255)); disc_fill(x+8, y-6, 3, COL(90,170,255));
            disc_fill(x, y, 3, COL(30,30,60)); disc_fill(x-8, y-6, 2, COL(30,30,60)); disc_fill(x+8, y-6, 2, COL(30,30,60));
        }
        if (S->ent_ev==5 && S->ent_phase==1 && ((S->ent_iy < b->y) == (pass==0))){
            bw_draw_big_hacker(S->ent_ix, S->ent_iy, 0, 0);
            if (S->ent_istun>0) draw_text_center(SX(S->ent_ix), SY(S->ent_iy)-40, "* * *", 1, COL(255,230,120));
            // guiding moths light the way to the next door
            for (int m2=0;m2<3;m2++){ float fx2 = b->x + 60 + m2*46 + fsin(G.anim_t*3 + m2)*6;
                disc_fill(SX(fx2), SY(ent_doory(b, room)) - 14 + (int)(fsin(G.anim_t*4 + m2*2)*3), 2, COL(255,246,190)); }
        }
        if (S->ent_ev==6 && ((S->ent_bby < b->y) == (pass==0))){
            bw_draw_big_hacker(S->ent_bbx, S->ent_bby, 3, 0);
            if (S->ent_bbcharge>0) draw_text_center(SX(S->ent_bbx), SY(S->ent_bby)-40, "!!", 2, COL(255,92,80));
        }
        if (S->ent_chap==2 && ((S->ent_luy < b->y) == (pass==0))){
            // DEEPER DOORS (feat 11): LUMEN, the lantern buddy — a
            // small warm pal with beating wings; in a dark room his
            // halo swells, because there the light is his
            int x=SX(S->ent_lux), y=SY(S->ent_luy) + (int)(fsin(G.anim_t*2.6f)*3);
            int lmdark = S->ent_dark;
            glow_ellipse(x, y, lmdark? 32 : 20, lmdark? 21 : 13, COL(255,176,84), lmdark? 95 : 55);
            int flap = (int)(fsin(G.anim_t*18)*2);
            ellipse_fill(x-8, y-5-flap, 5, 3, COL(240,230,200));
            ellipse_fill(x+8, y-5-flap, 5, 3, COL(240,230,200));
            disc_fill(x, y, 8, COL(255,196,92));
            disc_fill(x, y-1, 5, COL(255,224,150));
            disc_fill(x-3, y-2, 1, COL(60,40,16)); disc_fill(x+3, y-2, 1, COL(60,40,16));
        }
        if (pass==0) draw_buddy(b);
    }
    ui_vignette(vx, vy, vw2, vh2);
    // the Zip Bug warning: the lamps flicker
    if ((S->ent_ev==1 || S->ent_ev==2) && S->ent_phase==1 && ((int)(G.anim_t*9)%2))
        blend_rect(vx, vy, vw2, vh2, COL(255,236,170), 22);
    // the whole hotel is dim now (Daniel, Nov 5 — darker); Typo rooms
    // and the Library are properly DARK, landings keep their lamplight,
    // and every lamp breathes a little — worse when something screams
    {
        int bx=SX(b->x), by=SY(b->y);
        int dark = S->ent_dark || (!S->ent_sub && room==25 && S->ent_chap!=2);
        int landing = S->ent_sub? (room%20==0) : (room%10==0);
        float fl = 1.0f + 0.045f*fsin(G.anim_t*13.7f)*fsin(G.anim_t*7.9f+1.3f);
        if ((S->ent_ev==1 || S->ent_ev==2 || S->ent_ev==7) && S->ent_phase==1) fl *= ((int)(G.anim_t*11)%2)? 0.70f : 1.12f;
        int rin, amax;
        if (landing){ rin = 148; amax = 92; }
        else if (dark){ rin = 56; amax = 208; }
        else if (S->ent_sub){ rin = 102; amax = 152; }
        else { rin = 118; amax = 130; }
        if (dark && S->ent_chap==2){ rin += 64; if (amax>168) amax = 168; }   // DEEPER DOORS (feat 11): Lumen's glow carries the dark rooms; the drawer lantern still stacks on top
        rin = (int)((rin + S->ent_lant*36) * fl); if (rin<28) rin=28;
        int rout = rin + (dark? 74 : 96);
        for (int cy=vy; cy<vy+vh2; cy+=16) for (int cx=vx; cx<vx+vw2; cx+=16){
            int ddx = cx+8-bx, ddy = cy+8-by; int d = (int)fsqrt((float)(ddx*ddx + ddy*ddy));
            int a = d<=rin? 0 : d>=rout? amax : (d-rin)*amax/(rout-rin);
            if (a>0) blend_rect(cx, cy, 16, 16, COL(8,8,18), a);
        }
    }
    // dust motes drifting through the lamplight (glow-up): slow and
    // warm, and only inside the lamp's reach — the dark stays clean
    { int bx=SX(b->x), by=SY(b->y);
      for (int m2=0;m2<12;m2++){
          u32 hh = hash2(m2*17+3, 7, 0xD057u);
          float ph = G.anim_t*(0.11f + (float)(hh%5)*0.02f) + (float)(hh>>8);
          int mx2 = bx + (int)(fsin(ph + (float)(hh%9))*70.0f) + (int)((hh>>4)%40) - 20;
          int my2 = by + (int)(fcos(ph*0.8f + (float)(hh%7))*44.0f);
          int ddx = mx2-bx, ddy = my2-by;
          if (ddx*ddx + ddy*ddy < 110*110 && mx2>=vx && mx2<vx+vw2 && my2>=vy && my2<vy+vh2)
              pset(mx2, my2, COL(255,236,190));
      } }
    // a glitch tears the picture while it tears the air
    if (S->ent_ev==7){
        for (int s2=0;s2<7;s2++){
            u32 hh = hash2(s2*31, (int)(G.anim_t*24), 0x6C17u);
            int gy = vy + (int)(hh % (vh2-14));
            int gx = vx + (int)((hh>>7) % 40) - 20;
            blend_rect(gx, gy, vw2, 3, (s2&1)? COL(255,40,90) : COL(60,240,255), 46 + (int)(hh>>11)%40);
            for (int n2=0;n2<8;n2++){ u32 hn = hash2(s2*57+n2*13, (int)(G.anim_t*31), 0x57A7u);
                pset(vx + (int)(hn % vw2), vy + (int)((hn>>8) % vh2), (hn&1)? COL(255,255,255) : COL(20,20,30)); }
        }
    }
    // the buddy card + the run card
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
    { char buf[72]; char *q=buf; if (S->ent_sub){ q=sappend(q, "FLOOR B-"); q=sapp_int(q, room); } else if (S->ent_chap==2){ q=sappend(q, "DEEPER "); q=sapp_int(q, room); } else { q=sappend(q, "ENTRANCE "); q=sapp_int(q, room); }
      rect_fill(vx+vw2-158, vy+8, 150, 46, COL(20,26,20)); rect_line(vx+vw2-158, vy+8, 150, 46, COL(198,164,84));
      draw_text(vx+vw2-150, vy+13, buf, 1, COL(255,244,210));
      q=buf; q=sappend(q, "BITS "); q=sapp_int(q, S->ent_bits); q=sappend(q, " - STICKS "); q=sapp_int(q, S->ent_sticks); *q=0;
      draw_text(vx+vw2-150, vy+25, buf, 1, COL(230,240,230));
      q=buf; if (S->ent_sub){ q=sappend(q, "BEST B-"); q=sapp_int(q, b->ent_sub_best); } else if (S->ent_chap==2){ q=sappend(q, "BEST "); q=sapp_int(q, b->ent2_best); } else { q=sappend(q, "BEST ROOM "); q=sapp_int(q, b->ent_best); }
      if (S->ent_key) q=sappend(q, " - KEY!");
      if (!S->ent_sub && S->ent_chap==2){ if (b->ent2_done>0){ q=sappend(q, " - OUT x"); q=sapp_int(q, b->ent2_done); } }
      else if (!S->ent_sub && b->ent_done>0){ q=sappend(q, " - OUT x"); q=sapp_int(q, b->ent_done); }
      *q=0;
      draw_text(vx+vw2-150, vy+37, buf, 1, S->ent_key? COL(255,214,64) : COL(170,200,170)); }
    if (S->ent_ev==6){
        int n=0; for (int j=0;j<5;j++) if (S->ent_fuse_mask>>j & 1) n++;
        char buf[40]; char *q=buf; q=sappend(q, "FUSES "); q=sapp_int(q, n); q=sappend(q, "/5   NOISE"); *q=0;
        draw_text(vx+14, vy+62, buf, 1, COL(255,244,210));
        rect_fill(vx+14, vy+74, 120, 4, COL(40,30,40));
        rect_fill(vx+14, vy+74, (int)(120 * S->ent_noise/100.0f), 4, S->ent_noise>66? COL(255,92,80) : COL(255,200,110)); }
    // context hint — dynamic situations only; the standing controls
    // line is gone (controls live in the Start-menu's CONTROLS panel),
    // and the prompts name the actual key/button for the device in
    // hand (Daniel, Nov 6)
    { const char *hint = 0; char ehb[72];
      float doorx = ent_room_x0(room) + (ENT_STRIDE-1)*BW_TILE + 24, doory = ent_doory(b, room);
      float dx=b->x-doorx, dy=b->y-doory;
      int hidden = ent_tile(pi, bw_tileof(b->x), bw_tileof(b->y))==3;
      if (S->ent_ev==5 && S->ent_phase==1) hint = "RUN! FOLLOW THE MOTHS!";
      else if (hidden) hint = S->ent_sub? "HIDING IN THE VENT. VERY SNEAKY." : "HIDING IN THE BUSH. VERY SNEAKY.";
      else if (S->ent_ev==6) hint = (room==25 && !S->ent_libdone)? "STAND STILL TO BE QUIET. FUSES GLOW GREEN." : (room==25? "THE LIBRARY DOOR IS OPEN." : "IT HEARS FOOTSTEPS. STAND STILL TO BE QUIET.");
      else if (S->ent_chap==2 && ent_duet_door(pi, room) && room > S->ent_door_upto){
          // DEEPER DOORS (feat 11): the duet hints outrank the plain
          // OPEN THE DOOR line below — Interact does not open these
          float pax, pay, pbx, pby; ent_plate_pos(pi, room, 0, &pax, &pay); ent_plate_pos(pi, room, 1, &pbx, &pby);
          float dax=b->x-pax, day=b->y-pay, dbx=b->x-pbx, dby=b->y-pby;
          if (dax*dax+day*day < 22*22 || dbx*dbx+dby*dby < 22*22) hint = "HOLD STILL - TWO FRIENDS, ONE DOOR";
          else if (dx*dx+dy*dy < 90*90) hint = "STAND ON A PLATE - LUMEN TAKES THE OTHER";
      }
      else if (!S->ent_sub && room==25 && S->ent_chap!=2 && (b->x-(ent_room_x0(25)+(ENT_STRIDE-2)*BW_TILE+24))*(b->x-(ent_room_x0(25)+(ENT_STRIDE-2)*BW_TILE+24)) + (b->y-((ENT_ROWS-2)*BW_TILE+24))*(b->y-((ENT_ROWS-2)*BW_TILE+24)) < 58*58)
          hint = S->ent_libdone? key_prompt(ehb, lbl_int(), "BEHIND THE BOOKCASE...") : "A TALL BOOKCASE. IT WILL NOT BUDGE... YET.";
      else if (ent_door_locked(pi, room) && dx*dx+dy*dy < 54*54) hint = S->ent_key? key_prompt(ehb, lbl_int(), "UNLOCK IT!") : "LOCKED - THE KEY IS IN THIS ROOM";
      else if (dx*dx+dy*dy < 54*54 && room > (S->ent_sub? S->ent_door_sub : S->ent_door_upto)) hint = key_prompt(ehb, lbl_int(), "OPEN THE DOOR");
      else if (!S->ent_sub && room==ent_final_room(S)) hint = "THE EXIT DOOR IS IN THE SOUTH WALL";
      else if (room==1 && b->x < 90) hint = key_prompt(ehb, lbl_int(), S->ent_sub? "UP THE STAIRS" : "BACK TO THE MALL");
      if (hint){ int hw2 = text_w(hint, 1);
      rect_fill(vx + vw2/2 - hw2/2 - 6, vy + vh2-22, hw2+12, 14, COL(20,26,20));
      draw_text_center(vx + vw2/2, vy + vh2-19, hint, 1, COL(240,248,240)); } }
    if (S->sleep_t>0) draw_text_center(vx + vw2/2, vy + vh2/2-4, "Z Z Z", 2, COL(200,210,255));
    bw_draw_note(pi);
}

