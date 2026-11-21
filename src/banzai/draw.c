// THE SURVIVAL GAME — banzai/draw.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from banzai.c lines 2871-3260 in the Nov 2026 fine-grained refactor; code unchanged.

// --- drawing -------------------------------------------------------------------
static void bw_draw_big_hacker(float wx, float wy, int boss, int named){
    int x=SX(wx), y=SY(wy); if (x<-70||x>clip_x1+70||y<-90||y>clip_y1+70) return;
    int raid = boss==1 || boss==4, hoard = boss==3;   // boss 4 (feat 14) is the RAID, reassembled — drawn as itself
    shadow_ellipse(x, y+14, 24, 5);
    circle_fill(x, y, 21, OUTLINE); circle_fill(x, y, 19, raid? COL(44,52,92) : hoard? COL(66,52,26) : COL(30,30,38));
    rect_fill(x-13, y-8, 26, 15, COL(16,20,30)); rect_line(x-13, y-8, 26, 15, OUTLINE);
    glow_ellipse(x, y-3, 13, 6, COL(80,160,255), 60);   // eyes burn blue in the dark, halo and all (Daniel, Nov 5)
    u32 eyec = raid? COL(140,220,255) : hoard? COL(255,204,80) : COL(150,220,255);
    pset(x-6, y-3, eyec); pset(x-5, y-3, eyec);
    pset(x+5, y-3, eyec); pset(x+6, y-3, eyec);
    line_thick(x-12, y-16, x-16, y-24, 2, OUTLINE); circle_fill(x-16, y-25, 3, raid? COL(90,200,255) : hoard? COL(255,214,64) : COL(255,160,60));
    line_thick(x+12, y-16, x+16, y-24, 2, OUTLINE); circle_fill(x+16, y-25, 3, raid? COL(90,200,255) : hoard? COL(255,214,64) : COL(255,160,60));
    if (!raid && !hoard){ for (int cw=-2; cw<=2; cw++) line_thick(x+cw*5, y-19, x+cw*5, y-26, 1, COL(70,70,80)); }
    if (hoard){ // the sack it sits on: everything it ever cached
        circle_fill(x+21, y+9, 10, OUTLINE); circle_fill(x+21, y+9, 8, COL(122,94,52));
        line_thick(x+18, y+1, x+24, y+1, 1, OUTLINE); pset(x+19, y+7, COL(255,224,130)); }
    if (named) draw_text_center(x, y+22, raid? "BROKEN RAID" : hoard? "CACHE HOARDER" : "ZERO-DAY ZED", 1, COL(255,190,170));
}
static void bw_draw_hacker(float wx, float wy, int lvl, float lunge, int named){
    int x=SX(wx), y=SY(wy);
    if (x<clip_x0-40||x>clip_x1+40||y<clip_y0-50||y>clip_y1+50) return;
    int bob = (int)(fsin(G.anim_t*3.2f + wx*0.1f)*1.0f);
    int lx = (int)(lunge*22);
    shadow_ellipse(x, y+11, 11, 3);
    rect_fill(x-8+lx, y-6+bob, 16, 15, COL(38,36,48)); rect_line(x-8+lx, y-6+bob, 16, 15, OUTLINE);   // hoodie body
    circle_fill(x+lx, y-12+bob, 8, COL(38,36,48)); circle_fill(x+lx, y-12+bob, 8, OUTLINE);
    circle_fill(x+lx, y-12+bob, 7, COL(24,22,34));                                                     // hood
    ellipse_fill(x+lx, y-11+bob, 5, 4, COL(16,14,24));                                                 // shadowed face
    glow_ellipse(x+lx, y-11+bob, 8, 5, COL(80,160,255), 65);
    disc_fill(x-2+lx, y-11+bob, 1, COL(160,225,255)); disc_fill(x+2+lx, y-11+bob, 1, COL(160,225,255)); // eyes in the dark, burning blue
    rect_fill(x-6+lx, y+2+bob, 13, 8, COL(70,74,84)); rect_line(x-6+lx, y+2+bob, 13, 8, OUTLINE);      // laptop
    rect_fill(x-4+lx, y+4+bob, 9, 4, COL(90,255,140));
    if (named) draw_text_center(x, y-26+bob, bw_hacker_name(lvl), 1, COL(200,255,200));
}
static void bw_draw_tree(int x, int y, int pine){
    rect_fill(x-2, y-2, 4, 9, COL(110,76,44)); rect_line(x-2, y-2, 4, 9, OUTLINE);
    if (pine){   // the pointed kind: three stacked skirts of deep green
        tri_fill(x-13, y-6, x, y-20, x+13, y-6, COL(30,92,40));
        tri_fill(x-11, y-13, x, y-26, x+11, y-13, COL(36,104,44));
        tri_fill(x-8, y-20, x, y-31, x+8, y-20, COL(48,122,52));
        line_thick(x-13, y-6, x, y-20, 1, OUTLINE); line_thick(x, y-20, x+13, y-6, 1, OUTLINE);
        disc_fill(x-3, y-16, 1, COL(120,205,100));
        return;
    }
    circle_fill(x, y-10, 13, OUTLINE); circle_fill(x, y-10, 12, COL(36,104,44));
    circle_fill(x-5, y-14, 7, COL(56,136,56)); circle_fill(x+4, y-7, 5, COL(56,136,56));
    disc_fill(x-3, y-13, 2, COL(94,186,82)); disc_fill(x+3, y-15, 1, COL(120,205,100));
}
static void bw_draw_rock(int x, int y){
    // a crag, not a cup: the old smooth dome on its saucer base read as
    // a random teacup in the land (Daniel, Nov 3). Angular faces + a crack.
    shadow_ellipse(x, y+7, 14, 3);
    tri_fill(x-14, y+7, x-10, y-4, x+14, y+7, OUTLINE);
    tri_fill(x-10, y-4, x-2, y-9, x+14, y+7, OUTLINE);
    tri_fill(x-12, y+6, x-9, y-3, x-1, y+6, COL(102,110,128));
    tri_fill(x-9, y-3, x-2, y-8, x-1, y+6, COL(126,136,156));
    tri_fill(x-2, y-8, x+8, y-4, x+12, y+6, COL(110,118,138));
    tri_fill(x-2, y-8, x+12, y+6, x-1, y+6, COL(96,104,122));
    line_thick(x+1, y-6, x-1, y+1, 1, OUTLINE); line_thick(x-1, y+1, x+2, y+5, 1, OUTLINE);
}
static void bw_draw_tent(float wx, float wy){
    int x=SX(wx), y=SY(wy);
    shadow_ellipse(x, y+8, 16, 3);
    tri_fill(x-16, y+8, x, y-14, x+16, y+8, COL(226,126,44));
    tri_fill(x, y-14, x+16, y+8, x, y+8, COL(240,142,54));        // sunlit east face (glow-up)
    tri_fill(x-16, y+8, x, y-14, x+16, y+8, OUTLINE);
    tri_fill(x-5, y+8, x, y-4, x+5, y+8, COL(60,36,24));
    line_thick(x, y-14, x, y-19, 1, OUTLINE);
    { int wv = (int)(fsin(G.anim_t*3.1f + wx*0.05f)*1.5f);        // a little pennant, waving
      tri_fill(x, y-19, x+7, y-17+wv, x, y-15, COL(216,52,52)); }
    draw_text_center(x, y-28, "CAMP", 1, COL(255,240,210));
}
static void bw_draw_gate(void){
    int x0=SX(0), y0=SY(-30);
    disc_fill(x0, y0+18, 30, COL(140,190,110));
    int px1=SX(-26), px2=SX(26), py0=SY(-58), py1=SY(-6);
    rect_fill(px1-3, py0, 6, py1-py0, COL(196,64,44)); rect_line(px1-3, py0, 6, py1-py0, OUTLINE);
    rect_fill(px2-3, py0, 6, py1-py0, COL(196,64,44)); rect_line(px2-3, py0, 6, py1-py0, OUTLINE);
    rect_fill(px1-10, py0-4, (px2-px1)+20, 6, COL(196,64,44)); rect_line(px1-10, py0-4, (px2-px1)+20, 6, OUTLINE);
    rect_fill(px1-10, py0-4, (px2-px1)+20, 2, COL(226,110,84));   // sunlit top edge (glow-up)
    rect_fill(px1-2, py0, 1, py1-py0, COL(226,110,84));
    rect_fill(px2-2, py0, 1, py1-py0, COL(226,110,84));
    rect_fill(px1-6, py0+8, (px2-px1)+12, 4, COL(170,52,38));
    draw_text_center(x0, py0-16, "BUG MALL", 1, COL(255,255,240));
}
static void bw_draw_objects(int pi, int pass){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi]; u32 seed=(u32)b->bw_seed;
    int tx0=bw_tileof(b->x - VW/2 - BW_TILE), tx1=bw_tileof(b->x + VW/2 + BW_TILE);
    int ty0=bw_tileof(b->y - VH/2 - BW_TILE), ty1=bw_tileof(b->y + VH/2 + BW_TILE);
    for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
        int t = bw_tile_at(tx, ty, seed); if (!t) continue;
        if (bw_outside_world(tx*BW_TILE + BW_TILE/2, ty*BW_TILE + BW_TILE/2)) continue;   // nothing grows at sea
        if (b->bw_mode==1 && bw_ridge_pass_zone(tx*BW_TILE + BW_TILE/2, ty*BW_TILE + BW_TILE/2)) continue;   // the pass is kept clear
        float wy = ty*BW_TILE + BW_TILE/2;
        if ((wy < b->y) != (pass==0)) continue;
        int x=SX(tx*BW_TILE + BW_TILE/2), y=SY(wy);
        if (t==1) bw_draw_tree(x, y, (int)(hash2(tx, ty, seed ^ 0x7E33u) % 3 == 0)); else bw_draw_rock(x, y);
    }
    int cx0=bw_cellof(b->x)-3, cy0=bw_cellof(b->y)-3;
    for (int cx=cx0; cx<=cx0+6; cx++) for (int cy=cy0; cy<=cy0+6; cy++){
        float ox, oy; if (bw_cell_feat(cx, cy, seed, &ox, &oy, b->bw_mode==1)!=1) continue;
        float wx=cx*BW_CELL+ox, wy=cy*BW_CELL+oy;
        float dx=wx-b->x, dy=wy-b->y; if (dx*dx+dy*dy > 700*700) continue;
        if ((wy < b->y) != (pass==0)) continue;
        bw_draw_tent(wx, wy);
    }
    for (int i=0;i<BW_ROAMN;i++){
        BWHack *h=&S->roam[i];
        if (h->cell==0 && h->x==0 && h->y==0) continue;
        if ((h->y < b->y) != (pass==0)) continue;
        bw_draw_hacker(h->x, h->y, h->lvl, 0, 1);
        if (h->alert > 0.4f) draw_text_center(SX(h->x), SY(h->y)-36, "!", 2, COL(255,92,80));
    }
    if (b->bw_mode==1){   // the hill ridges: grassy mounds where the wall runs
        for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
            float wx = tx*BW_TILE + BW_TILE/2, wy = ty*BW_TILE + BW_TILE/2;
            int on = 0;
            for (int ri=0; ri<4 && !on; ri++){
                float along; float d = bw_ridge_dist(wx, wy, BW_RIDGES[ri], &along);
                if (d >= 30) continue;
                float g = along - BW_RIDGES[ri][4]; if (g<0) g=-g;
                if (g <= BW_RIDGES[ri][5] + 26) continue;   // the pass stays visibly open
                on = 1;
            }
            if (!on) continue;
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy);
            u32 hh = hash2(tx, ty, seed ^ 0x4117u);
            int jx = (int)(hh % 13) - 6, jy = (int)((hh>>6) % 9) - 4;
            ellipse_fill(x+jx, y+8+jy, 23, 9, COL(88,116,54));
            ellipse_fill(x-3+jx, y+3+jy, 16, 8, COL(102,132,60));
            ellipse_fill(x+2+jx, y-1+jy, 9, 5, COL(122,152,74));
            disc_fill(x-6+jx, y+2+jy, 1, COL(150,178,102)); disc_fill(x+7+jx, y+5+jy, 1, COL(150,178,102));
        }
    }
    // --- story props: huts, racks, the fence, and people ---------------------
    if (b->bw_mode==1){
        static const float HUT[3][2] = { { -2700, -1480 }, { -2500, -1480 }, { -2600, -1300 } };
        for (int i=0;i<3;i++){
            float wx=HUT[i][0], wy=HUT[i][1]+10;
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-70||x>clip_x1+70||y<-70||y>clip_y1+70) continue;
            shadow_ellipse(x, y+12, 30, 5);
            rect_fill(x-28, y-16, 56, 28, COL(158,116,74)); rect_line(x-28, y-16, 56, 28, OUTLINE);
            tri_fill(x-34, y-16, x, y-38, x+34, y-16, COL(128,66,44));
            line_thick(x-34, y-16, x, y-38, 1, OUTLINE); line_thick(x, y-38, x+34, y-16, 1, OUTLINE);
            rect_fill(x-6, y-2, 12, 14, COL(70,44,26)); rect_line(x-6, y-2, 12, 14, OUTLINE);
            rect_fill(x+14, y-10, 9, 9, COL(255,214,110)); rect_line(x+14, y-10, 9, 9, OUTLINE);
        }
        for (int gx=-2; gx<=2; gx++) for (int gy=-1; gy<=1; gy++){
            float wx = BW_SITE_XY[1][0] + gx*92, wy = BW_SITE_XY[1][1] + gy*112 + 16;
            { float cdx = wx-BW_SITE_XY[1][0], cdy = wy-16-BW_SITE_XY[1][1];
              if (cdx*cdx+cdy*cdy < 105*105) continue; }
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-70||x>clip_x1+70||y<-70||y>clip_y1+70) continue;
            shadow_ellipse(x, y+2, 28, 4);
            rect_fill(x-27, y-34, 54, 36, COL(40,44,56)); rect_line(x-27, y-34, 54, 36, OUTLINE);
            rect_fill(x-27, y-34, 54, 6, COL(58,64,78));
            for (int d2=0; d2<4; d2++){
                int on = (int)(G.anim_t*2 + gx*3 + gy*7 + d2*2) % 5 != 0;
                pset(x-18 + d2*11, y-18, on? COL(96,255,150) : COL(40,80,50));
                pset(x-18 + d2*11, y-10, COL(90,100,120));
            }
        }
        // the ring around the inner aisle (south rack sealed until solved)
        for (int k=0;k<8;k++){
            float a = k*0.7854f;
            float wx = BW_SITE_XY[1][0] + fcos(a)*108, wy = BW_SITE_XY[1][1] + fsin(a)*108 + 14;
            int sealed = (k==2 && !bw_puz_done(b));
            if (k==2 && bw_puz_done(b)){
                if (pass==0){ int x=SX(wx), y=SY(wy);   // the open gate: a worn plate
                    if (x>-40 && x<clip_x1+40 && y>-30 && y<clip_y1+30){ rect_fill(x-22, y-8, 44, 12, COL(96,88,60)); rect_line(x-22, y-8, 44, 12, OUTLINE); } }
                continue;
            }
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-70||x>clip_x1+70||y<-70||y>clip_y1+70) continue;
            shadow_ellipse(x, y+2, 24, 4);
            rect_fill(x-23, y-28, 46, 30, sealed? COL(52,40,44) : COL(40,44,56)); rect_line(x-23, y-28, 46, 30, OUTLINE);
            if (sealed){ int on = (int)(G.anim_t*2.4f)%2==0; circle_fill(x, y-14, 3, on? COL(255,70,60) : COL(120,30,26)); }
            else { pset(x-12, y-18, COL(96,255,150)); pset(x+2, y-18, COL(90,100,120)); pset(x+12, y-10, COL(96,255,150)); }
        }
        // the three terminals, wearing their colors (lit once pressed in order)
        { int ord[3]; bw_term_order(b, ord);
          int prog = bw_puz_prog(b);
          for (int i=0;i<3;i++){
              float wx=BW_TERM_XY[i][0], wy=BW_TERM_XY[i][1]+8;
              if ((wy < b->y) != (pass==0)) continue;
              int x=SX(wx), y=SY(wy); if (x<-40||x>clip_x1+40||y<-50||y>clip_y1+40) continue;
              shadow_ellipse(x, y+4, 10, 3);
              rect_fill(x-3, y-4, 6, 8, COL(90,84,80));
              rect_fill(x-9, y-16, 18, 13, COL(30,34,44)); rect_line(x-9, y-16, 18, 13, OUTLINE);
              int posn = ord[0]==i? 0 : ord[1]==i? 1 : 2;
              int lit = bw_puz_done(b) || posn < prog;
              circle_fill(x, y-10, 4, lit? BW_TERM_COL[i] : col_shade(BW_TERM_COL[i], 0.4f));
              if (lit) pset(x, y-10, COL(255,255,255));
          }
          // the scratched sign by the south entrance
          { float wx=3400, wy=-2100+8;
            if ((wy < b->y) == (pass==0)){
                int x=SX(wx), y=SY(wy);
                if (x>-40 && x<clip_x1+40 && y>-50 && y<clip_y1+40){
                    rect_fill(x-2, y-2, 4, 10, COL(90,84,80));
                    rect_fill(x-11, y-14, 22, 13, COL(64,58,50)); rect_line(x-11, y-14, 22, 13, OUTLINE);
                    line_thick(x-7, y-10, x+6, y-10, 1, COL(200,190,170));
                    line_thick(x-7, y-6, x+2, y-6, 1, COL(200,190,170));
                }
            }
          }
        }
        // Zed's fence: posts and wire all the way around
        { float cx=BW_SITE_XY[3][0], cy=BW_SITE_XY[3][1], r=BW_SITE_XY[3][2];
          for (int k=0;k<48;k++){
              float a = k*0.1309f, wx = cx + fcos(a)*r, wy = cy + fsin(a)*r;
              if ((wy < b->y) != (pass==0)) continue;
              int x=SX(wx), y=SY(wy); if (x<-30||x>clip_x1+30||y<-40||y>clip_y1+30) continue;
              rect_fill(x-2, y-16, 4, 20, COL(70,60,66)); rect_line(x-2, y-16, 4, 20, OUTLINE);
              float a2 = (k+1)*0.1309f;
              int x2=SX(cx + fcos(a2)*r), y2=SY(cy + fsin(a2)*r);
              line_thick(x, y-13, x2, y2-13, 1, COL(150,140,150));
              line_thick(x, y-6, x2, y2-6, 1, COL(120,112,120));
          }
          if (pass==1){
              int x=SX(cx), y=SY(cy + r + 20);
              if (x>-90 && x<clip_x1+90 && y>-30 && y<clip_y1+30)
                  draw_text_center(x, y, "ZED'S PLACE. KNOCK IF YOU MUST.", 1, COL(255,190,150));
          }
        }
        // the Overgrown Ruins: a ring of standing stones and the cache itself
        for (int k=0;k<7;k++){
            float a = k*0.8976f, wx = BW_SITE_XY[4][0] + fcos(a)*120, wy = BW_SITE_XY[4][1] + fsin(a)*120 + 8;
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-40||x>clip_x1+40||y<-60||y>clip_y1+40) continue;
            int hh = 22 + (int)(hash2(k, 7, 99)%8);
            shadow_ellipse(x, y+10, 10, 3);
            rect_fill(x-5, y+10-hh, 10, hh, COL(122,126,118)); rect_line(x-5, y+10-hh, 10, hh, OUTLINE);
            rect_fill(x-5, y+10-hh, 4, hh, COL(140,144,134));
            pset(x-2, y+2-hh/2, COL(96,120,72)); pset(x+2, y+6-hh/2, COL(96,120,72));   // moss
        }
        { float wx=BW_SITE_XY[4][0], wy=BW_SITE_XY[4][1]+26;   // the sunken cache, under the Hoarder
          if ((wy < b->y) == (pass==0)){
              int x=SX(wx), y=SY(wy);
              if (x>-50 && x<clip_x1+50 && y>-40 && y<clip_y1+40){
                  int open = (b->story_bits>>22)&1;
                  shadow_ellipse(x, y+8, 17, 4);
                  rect_fill(x-15, y-4, 30, 13, COL(104,74,40)); rect_line(x-15, y-4, 30, 13, OUTLINE);
                  rect_fill(x-15, y-4, 30, 4, COL(255,196,64));
                  if (open){ rect_fill(x-15, y-12, 30, 7, COL(84,58,30)); rect_line(x-15, y-12, 30, 7, OUTLINE);
                      pset(x-6, y+2, COL(255,224,130)); pset(x+4, y+3, COL(255,224,130)); }
                  else { rect_fill(x-15, y-9, 30, 6, COL(122,88,48)); rect_line(x-15, y-9, 30, 6, OUTLINE); }
              } } }
        // Mop's lost tools, glinting where the meadow dropped them
        for (int t2=0;t2<3;t2++){
            if ((b->story_bits>>(24+t2))&1) continue;
            float wx=BW_TOOL_XY[t2][0], wy=BW_TOOL_XY[t2][1];
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-30||x>clip_x1+30||y<-30||y>clip_y1+30) continue;
            int bob = (int)(fsin(G.anim_t*3 + t2*2.1f)*1.5f);
            disc_fill(x, y+bob, 4, COL(255,214,64)); disc_fill(x, y+bob, 2, COL(255,244,190));
            line_thick(x-4, y+5+bob, x+4, y+5+bob, 1, OUTLINE);
            if ((int)(G.anim_t*2 + t2)%3==0) pset(x+5, y-5+bob, COL(255,255,255));
        }
        // the big residents, at home
        for (int bs=0; bs<3; bs++){
            int alive = bs==0? (bw_puz_done(b) && !((b->story_bits>>16)&1)) : bs==1? !((b->story_bits>>17)&1) : !((b->story_bits>>22)&1);
            if (!alive) continue;
            float wx = BW_SITE_XY[bs==0? 1 : bs==1? 3 : 4][0];
            float wy = BW_SITE_XY[bs==0? 1 : bs==1? 3 : 4][1] + 10;
            if ((wy < b->y) != (pass==0)) continue;
            bw_draw_big_hacker(wx, wy, bs==0? 1 : bs==1? 2 : 3, 1);
        }
        // THE REBUILDING (feat 14): the rematch, reassembled and
        // waiting at the farm for exactly as long as quest 12 is owed
        if (bw_qstate3(b,12)==1){
            float wx = BW_SITE_XY[1][0], wy = BW_SITE_XY[1][1] + 10;
            if ((wy < b->y) == (pass==0)) bw_draw_big_hacker(wx, wy, 4, 1);
        }
        // help signs: a post, a board, and something scratched on it
        for (int i=0;i<6;i++){
            float wx=BW_HSIGN_XY[i][0], wy=BW_HSIGN_XY[i][1];
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-40||x>clip_x1+40||y<-50||y>clip_y1+40) continue;
            shadow_ellipse(x, y+10, 10, 3);
            rect_fill(x-2, y-2, 4, 12, COL(110,76,44)); rect_line(x-2, y-2, 4, 12, OUTLINE);
            rect_fill(x-13, y-14, 26, 13, COL(196,158,102)); rect_line(x-13, y-14, 26, 13, OUTLINE);
            line_thick(x-9, y-10, x+6, y-10, 1, COL(120,86,48)); line_thick(x-9, y-6, x+2, y-6, 1, COL(120,86,48));
        }
        // Bitling, the stray process, on its slow happy drift
        { float wx, wy; bw_bitling_pos(&wx, &wy);
          if ((wy < b->y) == (pass==0)){
            int x=SX(wx), y=SY(wy);
            if (x>-40 && x<clip_x1+40 && y>-50 && y<clip_y1+40){
                int bob = (int)(fsin(G.anim_t*5.0f)*1.5f);
                shadow_ellipse(x, y+9, 9, 3);
                rect_fill(x-7, y-4+bob, 14, 12, COL(70,160,170)); rect_line(x-7, y-4+bob, 14, 12, OUTLINE);
                circle_fill(x, y-8+bob, 6, COL(70,160,170)); circle_fill(x, y-8+bob, 6, OUTLINE);
                disc_fill(x-2, y-8+bob, 1, COL(255,250,220)); disc_fill(x+2, y-8+bob, 1, COL(255,250,220));
                line_thick(x, y-14+bob, x+4, y-19+bob, 1, OUTLINE);
                disc_fill(x+4, y-19+bob, 2, COL(255,120,160));
            }
          } }
        // people
        static const char *NPC_NAME[3] = { "PATCHINA", "MOP", "GLITCH" };
        for (int i=0;i<3;i++){
            float wx=BW_NPC_XY[i][0], wy=BW_NPC_XY[i][1];
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-40||x>clip_x1+40||y<-60||y>clip_y1+40) continue;
            shadow_ellipse(x, y+10, 11, 3);
            tri_fill(x-9, y+10, x, y-12, x+9, y+10, BW_NPC_ROBE[i]);
            line_thick(x-9, y+10, x, y-12, 1, OUTLINE); line_thick(x, y-12, x+9, y+10, 1, OUTLINE);
            circle_fill(x, y-15, 6, OUTLINE); circle_fill(x, y-15, 5, COL(242,204,164));
            if (i!=1){ tri_fill(x-4, y-11, x, y-4, x+4, y-11, COL(238,238,240)); }   // the beard (Mop is beardless, the scandal)
            pset(x-2, y-16, OUTLINE); pset(x+2, y-16, OUTLINE);
            int questy = (i==0 && (bw_qstate(b,0)==1 || (bw_qstate(b,1)==2 && bw_qstate2(b,8)==0)
                      || (bw_story_complete(b) && bw_qstate3(b,10)==0) || (bw_qstate3(b,10)==2 && bw_qstate3(b,11)==0) || bw_qstate3(b,13)==1))
                      || (i==1 && ((bw_qstate_x(b,4)==1 && bw_tools_found(b)>=3) || (bw_qstate_x(b,4)==2 && bw_qstate2(b,7)==0)))
                      || (i==2 && (bw_qstate(b,2)==1 || (bw_qstate2(b,7)==1 && ((b->story_bits2>>10)&1)) || (bw_qstate2(b,6)==1 && bw_lakekills(b)>=2) || (bw_qstate(b,2)==2 && bw_qstate2(b,6)==0)));
            if (questy) draw_text_center(x, y-34 - (int)(fsin(G.anim_t*4)*1.5f), "!", 2, COL(255,214,64));
            draw_text_center(x, y+16, NPC_NAME[i], 1, COL(255,246,220));
        }
        // Duster, the sweep-bot (feat 14): at its post, or at the
        // buddy's heels, for exactly as long as the escort is owed —
        // Bitling's shape in work clothes, brush and all
        if (bw_qstate3(b,11)==1 && S->ch2_esc_on){
            float wx=S->ch2_esc_x, wy=S->ch2_esc_y;
            if ((wy < b->y) == (pass==0)){
                int x=SX(wx), y=SY(wy);
                if (x>-40 && x<clip_x1+40 && y>-50 && y<clip_y1+40){
                    int bob = (int)(fsin(G.anim_t*5.0f + 1.3f)*1.5f);
                    shadow_ellipse(x, y+9, 9, 3);
                    rect_fill(x-7, y-4+bob, 14, 12, COL(150,144,132)); rect_line(x-7, y-4+bob, 14, 12, OUTLINE);
                    circle_fill(x, y-8+bob, 6, COL(150,144,132)); circle_fill(x, y-8+bob, 6, OUTLINE);
                    disc_fill(x-2, y-8+bob, 1, COL(255,190,80)); disc_fill(x+2, y-8+bob, 1, COL(255,190,80));
                    line_thick(x-6, y+8+bob, x+6, y+8+bob, 2, COL(196,158,102));   // the brush, sweeping
                    draw_text_center(x, y+16, "DUSTER", 1, COL(255,246,220));
                }
            }
        }
        // PATCH DAY dressing (feat 14): bunting over the village
        // green — up while the festival runs, and up for good once
        // the chapter is done (the village keeps the day). Villagers
        // and confetti only while the celebration clock runs.
        if (S->ch2_fest_t>0 || bw_chapter_done(b)){
            static const float BUNT[2][4] = { { -2700, -1470, -2500, -1470 }, { -2500, -1470, -2600, -1290 } };
            static const u32 PENN[4] = { COL(255,120,150), COL(255,214,64), COL(120,220,255), COL(150,255,150) };
            if (pass==0) for (int s2=0;s2<2;s2++){
                float ax=BUNT[s2][0], ay=BUNT[s2][1], bx2=BUNT[s2][2], by2=BUNT[s2][3];
                int px0=SX(ax), py0=SY(ay);
                if (px0<-340||px0>clip_x1+340||py0<-80||py0>clip_y1+80) continue;
                for (int sg=0; sg<8; sg++){
                    float t0=sg/8.0f, t1=(sg+1)/8.0f;
                    int x0=SX(ax+(bx2-ax)*t0), y0=SY(ay+(by2-ay)*t0 + fsin(t0*3.14159f)*14);
                    int x1=SX(ax+(bx2-ax)*t1), y1=SY(ay+(by2-ay)*t1 + fsin(t1*3.14159f)*14);
                    line_thick(x0, y0, x1, y1, 1, COL(90,70,50));
                    if (sg%2==0){ float tm=(t0+t1)/2; int mx=SX(ax+(bx2-ax)*tm), my=SY(ay+(by2-ay)*tm + fsin(tm*3.14159f)*14);
                        tri_fill(mx-4, my, mx+4, my, mx, my+9, PENN[(sg/2 + s2)%4]); }
                }
            }
            if (S->ch2_fest_t>0){
                static const float VIL[4][2] = { { -2780, -1330 }, { -2420, -1440 }, { -2660, -1220 }, { -2560, -1580 } };
                static const u32 VROBE[4] = { COL(178,84,84), COL(84,140,178), COL(190,150,60), COL(120,170,110) };
                for (int i=0;i<4;i++){
                    float wx=VIL[i][0], wy=VIL[i][1];
                    if ((wy < b->y) != (pass==0)) continue;
                    int x=SX(wx), y=SY(wy); if (x<-40||x>clip_x1+40||y<-60||y>clip_y1+40) continue;
                    int bob = (int)(fsin(G.anim_t*3 + i*1.7f)*1.5f);
                    shadow_ellipse(x, y+10, 11, 3);
                    tri_fill(x-9, y+10+bob, x, y-12+bob, x+9, y+10+bob, VROBE[i]);
                    line_thick(x-9, y+10+bob, x, y-12+bob, 1, OUTLINE); line_thick(x, y-12+bob, x+9, y+10+bob, 1, OUTLINE);
                    circle_fill(x, y-15+bob, 6, OUTLINE); circle_fill(x, y-15+bob, 5, COL(242,204,164));
                    pset(x-2, y-16+bob, OUTLINE); pset(x+2, y-16+bob, OUTLINE);
                }
                if (pass==0){
                    // confetti over the green — deterministic pieces
                    // from the hash and the clock, no stored particles
                    static const u32 CONFC[4] = { COL(255,120,150), COL(120,220,255), COL(255,214,64), COL(150,255,150) };
                    for (int i=0;i<42;i++){
                        u32 h = hash2(i, 7, 0xFE57u);
                        int wx = -2850 + (int)(h % 500);
                        int wy = -1650 + (int)((G.anim_t*(26 + (float)((h>>8)%20)) + (float)(h>>16)) ) % 500;
                        int x = SX(wx) + (int)(fsin(G.anim_t*2 + i)*4), y = SY(wy);
                        if (x<0||x>clip_x1||y<0||y>clip_y1) continue;
                        rect_fill(x, y, 3, 2, CONFC[h%4]);
                    }
                }
            }
        }
    }
}
