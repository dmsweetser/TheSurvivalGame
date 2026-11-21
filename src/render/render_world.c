// THE SURVIVAL GAME — render/render_world.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 3135-3541 in the Nov 2026 fine-grained refactor; code unchanged.

static void render_world(void){
    // screen shake: a live G.shake (raised by fx_shake, decayed by
    // the frame loop) becomes a small deterministic jitter, capped
    // at ~5 px — a thump, not a strobe. Play states only: the menu,
    // pause and game-over backdrops hold still. With the toggle off
    // fx_shake never raises the value, so this stays exactly 0.
    shk_off_x = 0; shk_off_y = 0;
    if (G.shake_on && G.shake>0.05f && (G.state==GS_PLAY || G.state==GS_STORE)){
        float m = G.shake>3? 3 : G.shake;
        shk_off_x = fsin(G.anim_t*47.0f)*m*1.6f;
        shk_off_y = fcos(G.anim_t*59.0f)*m*1.6f;
        G.camx += shk_off_x; G.camy += shk_off_y;
    }
    // camera zoom: the bigger your hat, the wider your view of the world
    // (in a split-screen pass the caller owns cam/zoom — cam_lock — and
    // view_pi says whose hat would set the zoom)
    if (!G.cam_lock)
    { Player *me=&G.p[G.view_pi>=0? G.view_pi : 0];
      float zt = zoom_target_of(me);
      if (G.zoom<0.01f) G.zoom=1.0f;
      G.zoom += (zt-G.zoom)*0.045f; }
    { Player *viewp=&G.p[G.view_pi>=0? G.view_pi : 0];
      if (in_srcroom[viewp-G.p]){ draw_srcroom(); G.camx-=shk_off_x; G.camy-=shk_off_y; return; }
      if (viewp->in_arcade){ draw_arcade(); G.camx-=shk_off_x; G.camy-=shk_off_y; return; }
      if (viewp->in_mall){ draw_mall(); G.camx-=shk_off_x; G.camy-=shk_off_y; return; } }
    float cz = G.zoom>0.01f? G.zoom : 1.0f;
    // ground: 64px checker tiles tinted by zone band
    int tlx = (int)(G.camx - (VW/2)/cz) - 64, tly = (int)(G.camy - (VH/2)/cz) - 64;
    int gx0 = tlx/64 - (tlx<0?1:0), gy0 = tly/64 - (tly<0?1:0);
    int gw = (int)(VW/cz/64) + 4, gh = (int)(VH/cz/64) + 4, ts = (int)(64*cz) + 1;
    for (int gy=gy0; gy<gy0+gh; gy++) for (int gx=gx0; gx<gx0+gw; gx++){
        float wx = gx*64+32, wy = gy*64+32;
        float d = fsqrt(wx*wx+wy*wy);
        int chk = ((gx+gy)&1);
        u32 c = col_shade(zone_ground(d, chk), 0.97f + hash_f(gx, gy, 77)*0.05f);
        // region borders dither instead of seaming (Nov 9): within
        // 70px of a boundary a hashed share of tiles already wears the
        // country across the line — half and half right on the line,
        // thinning to nothing at the band's edge
        { int r = region_at(d);
          float e = -1.0f; int other = r;
          if (r < NREGIONS-1 && REGION_EDGE[r] - d < 70.0f){ e = REGION_EDGE[r]; other = r+1; }
          else if (r > 0 && d - REGION_EDGE[r-1] < 70.0f){ e = REGION_EDGE[r-1]; other = r-1; }
          if (e > 0.0f){ float f = 0.5f - (d>e? d-e : e-d)/140.0f;
              if (hash_f(gx, gy, 314) < f)
                  c = col_shade(chk? REGION_GA[other] : REGION_GB[other], 0.97f + hash_f(gx, gy, 77)*0.05f); } }
        int tx = SX(gx*64), tyy = SY(gy*64);
        rect_fill(tx, tyy, ts, ts, c);
        // soft grout seams so the checker reads clean instead of harsh
        rect_fill(tx+64, tyy, 1, 65, col_shade(c, 0.93f));
        rect_fill(tx, tyy+64, 65, 1, col_shade(c, 0.93f));
        // faint deterministic speckles for texture
        float sp = hash_f(gx, gy, 43);
        if (sp>0.45f){
            u32 sc = col_shade(c, 0.90f);
            pset(tx+(int)(sp*57)+3, tyy+(int)(sp*41)+5, sc);
            pset(tx+(int)(sp*29)+11, tyy+(int)(sp*53)+7, sc);
            // regional sparkle: lit traces in the RAM Flats, glitch
            // flecks in the Depths
            int rg = region_at(d);
            if (rg==5 && sp>0.86f) pset(tx+(int)(sp*47)+7, tyy+(int)(sp*37)+9, COL(110,235,255));
            if (rg==7 && sp>0.88f) pset(tx+(int)(sp*43)+9, tyy+(int)(sp*31)+13, COL(255,84,200));
        }
        // sprinkle: deterministic pebbles/tufts, tinted by the region
        float r = hash_f(gx, gy, 42);
        if (r>0.6f){ int px=SX(gx*64+(int)(r*50)), py=SY(gy*64+(int)(r*40));
            u32 tc = REGION_TUFT[region_at(d)];
            line_thick(px, py, px-2, py-4, 1, tc); line_thick(px+3, py, px+4, py-5, 1, tc);
            // the Meadows bloom: two-petal flowers close to home
            if (d<500 && r>0.86f){
                u32 fc = ((gx*7+gy*13)&1)? COL(255,170,200) : COL(255,240,200);
                disc_fill(px+8, py-3, 1, fc); disc_fill(px+10, py-2, 1, fc); pset(px+9, py-2, COL(255,205,40));
            }
        }
    }
    // the Epicenter plaza — the firewall is gone; it's just pavement now,
    // and it shows the wear: scuffs, stains and cracks
    int pcx=SX(0), pcy=SY(0);
    circle_fill(pcx, pcy, 150, COL(196,190,178));
    circle_fill(pcx, pcy, 138, COL(208,201,186));
    circle_fill(pcx, pcy, 96, COL(219,211,193));   // sun-warmed heart of the plaza (glow-up)
    circle_out(pcx, pcy, 121, COL(206,172,92));   // a gold inlay ring, mall money well spent
    circle_out(pcx, pcy, 118, COL(196,190,178));
    ellipse_fill(pcx-52, pcy+38, 30, 10, COL(188,182,168));
    ellipse_fill(pcx+64, pcy-30, 22, 8, COL(190,184,170));
    ellipse_fill(pcx+18, pcy+74, 16, 6, COL(186,180,166));
    line_thick(pcx-96, pcy-58, pcx-70, pcy-38, 1, COL(170,164,152));
    line_thick(pcx-70, pcy-38, pcx-78, pcy-18, 1, COL(170,164,152));
    line_thick(pcx+84, pcy+52, pcx+104, pcy+66, 1, COL(170,164,152));
    draw_text_center(pcx, pcy+118, "THE EPICENTER", 1, COL(120,110,95));
    draw_store_building();
    // the Epicenter shield: a shimmering ring, up while anyone is in the
    // store. When the last shopper leaves it fades out over ~0.8s (visual
    // only — the barrier itself drops the moment the store empties).
    if (G.state==GS_PLAY || G.state==GS_STORE){
        static float shield_seen = -10.0f;
        int show_shield = (G.net_mode==NET_CLIENT)? (shield_remote || store_open_local)
                                                  : shield_active();
        if (show_shield) shield_seen = G.anim_t;
        float fade = show_shield? 1.0f : 1.0f - (G.anim_t - shield_seen)/0.8f;
        if (fade > 0.0f){
            if (fade > 1.0f) fade = 1.0f;
            float pulse = (0.72f + 0.28f*fsin(G.anim_t*3.0f)) * fade;
            for (int ring=0; ring<2; ring++){
                float rr = ring? 146.0f : 152.0f;
                int alpha = (int)((ring? 90:170)*pulse);
                for (float a=0; a<TAU; a+=TAU/110){
                    if (((int)(a*9 - G.anim_t*1.5f))%2==0) continue;   // rotating dashes
                    pset_blend(pcx+(int)(fcos(a)*rr), pcy+(int)(fsin(a)*rr), COL(150,215,255), alpha);
                }
            }
            if (fade > 0.5f)
                for (int k=0;k<4;k++){ // bright nodes drifting around the ring
                    float a = G.anim_t*0.5f + k*(TAU/4);
                    circle_fill(pcx+(int)(fcos(a)*152), pcy+(int)(fsin(a)*152), 2, COL(190,235,255));
                }
        }
    }
    // props
    int cx0=(int)((G.camx-(VW/2)/cz)/CHUNK)-1, cy0=(int)((G.camy-(VH/2)/cz)/CHUNK)-1;
    int cw = (int)(VW/cz/CHUNK) + 3, chh = (int)(VH/cz/CHUNK) + 3;
    for (int cy=cy0; cy<cy0+chh; cy++) for (int cx=cx0; cx<cx0+cw; cx++)
        for (int s=0;s<2;s++){
            float px,py; int t,sol; chunk_prop(cx, cy, s, &px,&py,&t,&sol);
            if (t) draw_prop(t, SX(px), SY(py), hash2(cx,cy,7+(u32)s));
        }
    // walls: a proper barricade of carcasses — shaded mound, five bugs
    // deep, and it visibly falls apart as it gets chewed through
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive){
        Wall *w=&G.walls[i]; int x=SX(w->x), y=SY(w->y);
        if (x<-40||x>VW+40||y<-40||y>VH+40) continue;
        int hw = w->horiz? 26:9, hh = w->horiz? 9:26;
        float frac = w->hp/w->maxhp;
        u32 col = frac>0.5f? COL(168,118,72) : (frac>0.25f? COL(150,100,60) : COL(128,82,50));
        shadow_ellipse(x, y+hh+1, hw+3, 3);
        rect_fill(x-hw-2, y-hh-2, hw*2+4, hh*2+4, OUTLINE);
        rect_fill(x-hw, y-hh, hw*2, hh*2, col_shade(col, 0.82f));            // base in shade
        if (w->horiz) rect_fill(x-hw, y-hh, hw*2, hh, col);                   // lit top half
        else          rect_fill(x-hw, y-hh, hw, hh*2, col);
        rect_fill(x-hw, y-hh, w->horiz? hw*2:2, w->horiz? 2:hh*2, col_shade(col, 1.18f)); // top/left highlight
        // the carcasses themselves: five across (or down), each different,
        // slumping lower as the wall's HP drains away
        int slump = frac>0.5f? 0 : (frac>0.25f? 1:2);
        for (int k=0;k<5;k++){
            float t = (k-2)/2.0f;                          // -1..1 along the wall
            int cx2 = w->horiz? x + (int)(t*(hw-6)) : x + ((k%2)? 2:-2);
            int cy2 = w->horiz? y + ((k%2)? 2:-2) : y + (int)(t*(hh-6));
            int kind = (i*3 + k*7)%4;
            float tilt = ((i*13 + k*29)%10 - 5)/8.0f + (k==4 && frac<0.25f? 0.9f:0.0f); // one slides off when nearly dead
            draw_carcass(cx2, cy2 + slump + (k==4 && frac<0.25f? 3:0), 4 + (k%2), kind, tilt);
        }
        // exposed legs poking out of the mound
        line_thick(x-hw+4, y-hh+1, x-hw+1, y-hh-4, 1, OUTLINE);
        line_thick(x+3, y-hh+1, x+5, y-hh-4, 1, OUTLINE);
        if (w->horiz) line_thick(x+hw-5, y+hh-1, x+hw-2, y+hh+3, 1, OUTLINE);
        // reinforcement made visible (restored Nov 7): every stacked
        // layer piles another course of carcasses on top — the heap
        // grows taller with each reinforcement, capped for the eye
        int pile_h = 0;
        { int st = w->stack>0? w->stack : 1;
          int layers = st-1; if (layers>4) layers=4;
          pile_h = layers*5;
          for (int L=1; L<=layers; L++){
              int ly = y - hh - L*5 + slump;
              int inset = L*2; if (inset>hw-8) inset=hw-8; if (inset<0) inset=0;
              // shaded under-course so the layer reads as sitting ON the heap
              if (w->horiz) rect_fill(x-hw+inset, ly+3, (hw-inset)*2, 3, col_shade(col, 0.55f));
              else          rect_fill(x-4, ly+3, 8, 3, col_shade(col, 0.55f));
              int nk = (L>=3)? 3 : 4;
              for (int k=0;k<nk;k++){
                  float t = (k-(nk-1)/2.0f)/((nk-1)/2.0f);   // -1..1 across the course
                  int cx2 = w->horiz? x + (int)(t*(hw-6-inset)) : x + ((k%2)? 2:-2);
                  int cy2 = w->horiz? ly + ((k%2)? 1:-1) : ly + (int)(t*3);
                  int kind = (i*5 + L*3 + k*7)%4;
                  float tilt = ((i*17 + L*11 + k*29)%10 - 5)/8.0f;
                  draw_carcass(cx2, cy2, 4 + (k%2), kind, tilt);
              }
          }
          if (st>5){ // a proper tower: say how deep the pile goes
              char nb[8]; char *nq=nb; nq=sappend(nq, "x"); nq=sapp_int(nq, st); *nq=0;
              draw_text_center(x, y-hh-pile_h-13, nb, 1, COL(255,230,150));
          }
        }
        // cracks as it gets chewed
        if (frac<0.75f){
            line_thick(x-6, y-hh+2, x-2, y, 1, OUTLINE);
            line_thick(x-2, y, x-5, y+hh-3, 1, OUTLINE);
        }
        if (frac<0.45f){
            line_thick(x+8, y-hh+3, x+4, y+1, 1, OUTLINE);
            line_thick(x+4, y+1, x+7, y+hh-2, 1, OUTLINE);
            line_thick(x-1, y-2, x+3, y+3, 1, OUTLINE);
        }
        if (frac<0.25f) // scorch tint: it's barely holding together
            blend_rect(x-hw, y-hh, hw*2, hh*2, COL(40,26,20), 60);
        // interlock tiers: the bigger the run, the more engineered it
        // looks — and at 5+ you can walk on top of it
        { int tier = (w->grp>=0)? grp_tier(grp_n[w->grp]) : 0;
          if (tier>=1){ // plank cap rail + bolts
              u32 wood2 = COL(140,96,52);
              if (w->horiz){ rect_fill(x-hw, y-hh-3, hw*2, 3, wood2); rect_fill(x-hw, y-hh-3, hw*2, 1, COL(172,128,76)); }
              else { rect_fill(x-3, y-hh, 3, hh*2, wood2); rect_fill(x-3, y-hh, 1, hh*2, COL(172,128,76)); }
              disc_fill(x-hw+4, y, 1, OUTLINE); disc_fill(x+hw-4, y, 1, OUTLINE);
          }
          if (tier>=2){ // iron straps with rivets
              u32 iron = COL(74,74,88);
              if (w->horiz){ for (int s2=-1;s2<=1;s2+=2){ int sx2=x+s2*hw/2; rect_fill(sx2-1, y-hh, 3, hh*2, iron); disc_fill(sx2, y-hh+3, 1, COL(150,150,165)); disc_fill(sx2, y+hh-3, 1, COL(150,150,165)); } }
              else { for (int s2=-1;s2<=1;s2+=2){ int sy2=y+s2*hh/2; rect_fill(x-hw, sy2-1, hw*2, 3, iron); disc_fill(x-hw+3, sy2, 1, COL(150,150,165)); disc_fill(x+hw-3, sy2, 1, COL(150,150,165)); } }
          }
          if (tier>=3){ // CASTLE WALL (Daniel): at 20+ segments the run is
              // faced in stone — block courses, joints, and battlements.
              u32 stone = COL(148,146,142), stone_hi = COL(172,170,164), stone_lo = COL(118,116,112);
              rect_fill(x-hw, y-hh, hw*2, hh*2, stone);
              if (w->horiz) rect_fill(x-hw, y-hh, hw*2, hh, stone_hi);
              else          rect_fill(x-hw, y-hh, hw, hh*2, stone_hi);
              rect_line(x-hw, y-hh, hw*2, hh*2, OUTLINE);
              // block courses: a mid seam plus staggered joints
              if (w->horiz){
                  rect_fill(x-hw, y-1, hw*2, 1, stone_lo);
                  for (int jx=-hw+9; jx<hw-2; jx+=13) rect_fill(x+jx, y-hh, 1, hh, stone_lo);
                  for (int jx=-hw+15; jx<hw-2; jx+=13) rect_fill(x+jx, y, 1, hh, stone_lo);
              } else {
                  rect_fill(x-1, y-hh, 1, hh*2, stone_lo);
                  for (int jy=-hh+9; jy<hh-2; jy+=13) rect_fill(x-hw, y+jy, hw, 1, stone_lo);
                  for (int jy=-hh+15; jy<hh-2; jy+=13) rect_fill(x, y+jy, hw, 1, stone_lo);
              }
              // battlements: stone teeth along the outer edge
              if (w->horiz){ for (int tx2=x-hw+2; tx2<x+hw-5; tx2+=11){
                  rect_fill(tx2, y-hh-5, 7, 5, stone); rect_fill(tx2, y-hh-5, 7, 1, stone_hi);
                  rect_line(tx2, y-hh-5, 7, 5, OUTLINE); } }
              else { for (int ty2=y-hh+2; ty2<y+hh-5; ty2+=11){
                  rect_fill(x-hw-5, ty2, 5, 7, stone); rect_fill(x-hw-5, ty2, 1, 7, stone_hi);
                  rect_line(x-hw-5, ty2, 5, 7, OUTLINE); } }
              if (frac<0.6f){ // even castles crack
                  line_thick(x-5, y-hh+2, x-1, y+1, 1, OUTLINE);
                  line_thick(x+6, y, x+3, y+hh-2, 1, OUTLINE);
              }
          }
          if (tier>=4){ // GRAND CASTLE: taller teeth, a cap course, gold trim
              u32 stone = COL(148,146,142), stone_hi = COL(178,176,170);
              if (w->horiz){ for (int tx2=x-hw+2; tx2<x+hw-5; tx2+=11) rect_fill(tx2, y-hh-7, 7, 2, stone_hi);
                             rect_fill(x-hw-2, y+hh, hw*2+4, 2, stone); }
              else { for (int ty2=y-hh+2; ty2<y+hh-5; ty2+=11) rect_fill(x-hw-7, ty2, 2, 7, stone_hi);
                     rect_fill(x+hw, y-hh-2, 2, hh*2+4, stone); }
              rect_line(x-hw-3, y-hh-3, hw*2+6, hh*2+6, COL(255,205,40));
          }
          if (tier>=1 && w->endpt){ // step stones at the end of a climbable run + a pennant
              int ndx=0, ndy=0;
              for (int j=0;j<MAXWALLS;j++) if (j!=i && G.walls[j].alive==1 && G.walls[j].grp==w->grp && wall_adjacent(w, &G.walls[j])){
                  ndx = G.walls[j].x > w->x+2? 1 : (G.walls[j].x < w->x-2? -1:0);
                  ndy = G.walls[j].y > w->y+2? 1 : (G.walls[j].y < w->y-2? -1:0);
                  break;
              }
              int sxp = x - ndx*(hw+5), syp = y - ndy*(hh+5);
              rect_fill(sxp-5, syp+1, 10, 5, COL(150,146,140)); rect_line(sxp-5, syp+1, 10, 5, OUTLINE);
              rect_fill(sxp-3, syp-3, 6, 4, COL(168,164,158)); rect_line(sxp-3, syp-3, 6, 4, OUTLINE);
              if (tier>=4){ // a round stone turret caps the run's end,
                  // and the pennant flies from its top
                  circle_fill(x, y, 11, OUTLINE);
                  circle_fill(x, y, 10, COL(148,146,142));
                  circle_fill(x-3, y-3, 3, COL(172,170,164));
                  for (int ts=0; ts<6; ts++){ float ta=ts*1.0472f;
                      disc_fill(x + (int)(fcos(ta)*9), y + (int)(fsin(ta)*9), 1, COL(118,116,112)); }
                  line_thick(x, y-9, x, y-22, 1, OUTLINE);
                  tri_fill(x, y-22, x+8, y-19, x, y-16, (i%2)? COL(220,70,70) : COL(255,205,40));
              }
          }
        }
        if (w->alive==2) draw_text_center(x, y-16, "BUILDING...", 1, COL(255,240,180));
        if (frac<0.99f){ rect_fill(x-14, y-hh-pile_h-8, 28, 3, COL(40,30,40));
            rect_fill(x-14, y-hh-pile_h-8, (int)(28*frac), 3, COL(255,170,80)); }
    }
    // bugs (feat 8: an elite wears its name over the sprite — drawn
    // here, after the sprite, so Eddie Mode's early return inside
    // draw_bug_sprite gets the same plate)
    for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
        Bug *b=&G.bugs[i];
        draw_bug_sprite(b);
        if (b->elite){
            int x=SX(b->x), y=SY(b->y);
            if (x>-70 && x<VW+70 && y>-70 && y<VH+70){
                int r=(int)(bug_radius(b->w)*(G.zoom>0.01f? G.zoom : 1.0f)); if (r<3) r=3;
                const char *nm = ELITE_NAME[(b->elite-1)%ELITE_N];
                int ny = y-r-24;
                // the plate: a dark slip with a gold frame and a pin,
                // the name in amber — above the sprite's own hp bar
                int tw = text_w(nm, 1);
                rect_fill(x-tw/2-5, ny-3, tw+10, 13, COL(24,18,28));
                rect_line(x-tw/2-5, ny-3, tw+10, 13, COL(198,164,84));
                draw_text_center(x, ny, nm, 1, COL(255,190,80));
                disc_fill(x, ny-6, 2, COL(250,200,50));
            }
        }
    }
    // Banzai Buddies trot with their owners
    for (int i=0;i<MAXP;i++) if (G.bud[i].active && !G.bud[i].in_bw) draw_buddy(&G.bud[i]);
    // players
    for (int i=0;i<MAXP;i++) if (G.p[i].connected) draw_player_sprite(&G.p[i]);
    // projectiles
    for (int i=0;i<MAXPROJ;i++) if (G.projs[i].alive){
        Proj *q=&G.projs[i]; int x=SX(q->x), y=SY(q->y);
        if (x<-20||x>VW+20||y<-20||y>VH+20) continue;
        if (q->kind==0){ rect_fill(x-3, y-2, 6, 4, OUTLINE); rect_fill(x-2, y-1, 4, 2, COL(230,230,240)); }
        else if (q->kind==1){ ellipse_out(x, y, 5, 4, COL(180,180,200)); }
        else if (q->kind==2){ circle_out(x, y, 5, COL(255,215,60)); circle_fill(x+3, y-3, 2, COL(255,150,40)); }
        else { // a tiny project manager, tumbling through the air, briefcase first
            circle_out(x, y-3, 4, COL(245,205,160));
            rect_fill(x-3, y+1, 7, 6, COL(50,50,90));
            rect_fill(x+3, y+2, 4, 3, COL(120,80,40));
        }
    }
    // carcass flights
    for (int i=0;i<(int)(sizeof(G.fx)/sizeof(G.fx[0]));i++) if (G.fx[i].alive){
        CarcassFx *f=&G.fx[i];
        float t=f->t; if (t>1) t=1;
        float mx=(f->x0+f->x1)/2, my=(f->y0+f->y1)/2 - 70;
        float ix=(1-t)*(1-t)*f->x0 + 2*(1-t)*t*mx + t*t*f->x1;
        float iy=(1-t)*(1-t)*f->y0 + 2*(1-t)*t*my + t*t*f->y1;
        draw_carcass(SX(ix), SY(iy), 5, (int)f->w%4, t*5);
    }
    // v1.24 ambient life: drifting motes in the daylight — world-
    // anchored hash specks bobbing on anim_t, so the meadow air
    // itself feels alive. Pure draw (no state, no particles, a few
    // dozen blended points), outdoors only. THE LIVING SKY (feat
    // 12): as the light goes the motes dim with it, and the bolder
    // specks turn into fireflies — same cells, same cost.
    if ((G.state==GS_PLAY || G.state==GS_STORE) && !G.p[G.view_pi>=0? G.view_pi:0].in_mall){
        float dk = tod_darkf();
        int cx0 = (int)(G.camx/110)-8, cy0 = (int)(G.camy/110)-5;
        for (int gy=cy0; gy<cy0+10; gy++) for (int gx=cx0; gx<cx0+16; gx++){
            for (int k=0;k<2;k++){
                float h1 = hash_f(gx*3+k, gy*5+k, 71), h2 = hash_f(gx*7+k, gy*3+k, 97);
                float mx = gx*110 + h1*104 + fsin(G.anim_t*0.6f + h2*9.0f)*6;
                float my = gy*110 + h2*104 + fcos(G.anim_t*0.45f + h1*7.0f)*5;
                int sx = SX(mx), sy = SY(my);
                if (sx<clip_x0 || sx>=clip_x1 || sy<clip_y0 || sy>=clip_y1) continue;
                if (dk > 0.45f && h1 > 0.6f){
                    int fa = (int)((46.0f + 64.0f*(0.5f + 0.5f*fsin(G.anim_t*2.2f + h2*20.0f))) * ((dk - 0.45f)/0.55f));
                    pset_blend(sx, sy, COL(255,220,130), fa);
                } else {
                    int al = 24 + (int)(h1*36) + (int)(fsin(G.anim_t*1.3f + h2*12.0f)*10);
                    al = (int)(al * (1.0f - 0.8f*dk));
                    pset_blend(sx, sy, COL(255,246,205), al<8? 8 : al);
                }
            }
        }
    }
    // THE LIVING SKY (feat 12): the tint, the lantern glow, and the
    // regional weather — all over the finished world, all under the
    // vignette, the particles, the floaters and the HUD, so nothing
    // a player must read ever sits under the sky. Interiors never
    // reach here (the mall/arcade early-outs above).
    tod_apply_tint();
    if (G.state==GS_PLAY || G.state==GS_STORE){
        float dk = tod_darkf();
        if (dk > 0.02f){
            // the store building's lanterns and lit window, glowing
            // warmer by contrast as the world cools (world +-74,-75)
            glow_ellipse(SX(-74), SY(-75), 30, 20, COL(255,190,100), (int)(52.0f*dk));
            glow_ellipse(SX( 74), SY(-75), 30, 20, COL(255,190,100), (int)(52.0f*dk));
            glow_ellipse(SX(-65), SY(-74), 18, 12, COL(255,205,110), (int)(40.0f*dk));
        }
        // signature weather, in the motes' hash-grid language: each
        // speck asks its own region what the sky is doing there
        { int epoch = wx_epoch_now();
          int cx0 = (int)(G.camx/110)-8, cy0 = (int)(G.camy/110)-5;
          for (int gy=cy0; gy<cy0+10; gy++) for (int gx=cx0; gx<cx0+16; gx++){
              for (int k=0;k<2;k++){
                  float h1 = hash_f(gx*5+k, gy*3+k, 131), h2 = hash_f(gx*3+k, gy*7+k, 149);
                  float wxp = gx*110 + h1*104, wyp = gy*110 + h2*104;
                  int rg = region_at(fsqrt(wxp*wxp + wyp*wyp));
                  int kind = WX_KIND[rg];
                  if (!kind) continue;
                  int st = wx_state(rg, epoch);
                  if (!st || k >= st) continue;   // light = one speck, steady = two
                  if (kind==1){        // CABLE JUNGLE rain: fast slanted streaks
                      float fall = h2*7.0f + G.anim_t*2.36f; fall -= (int)fall;
                      int sx = SX(wxp - fall*8.0f), sy = SY(gy*110 + fall*110.0f);
                      if (sx<clip_x0 || sx>=clip_x1 || sy<clip_y0 || sy>=clip_y1) continue;
                      pset_blend(sx, sy, COL(170,200,235), 110);
                      pset_blend(sx-1, sy-3, COL(170,200,235), 90);
                      pset_blend(sx-2, sy-6, COL(170,200,235), 70);
                  } else if (kind==2){ // FROZEN CACHE snow: slow dots, sine sway
                      float fall = h2*7.0f + G.anim_t*0.2f; fall -= (int)fall;
                      int sx = SX(wxp + fsin(G.anim_t*0.9f + h1*20.0f)*6), sy = SY(gy*110 + fall*110.0f);
                      if (sx<clip_x0 || sx>=clip_x1 || sy<clip_y0 || sy>=clip_y1) continue;
                      pset_blend(sx, sy, COL(240,248,255), 130);
                  } else {             // KERNEL BADLANDS shimmer: warm specks, fast wobble
                      int sx = SX(wxp + fsin(G.anim_t*3.0f + h1*30.0f)*1.5f), sy = SY(wyp + fcos(G.anim_t*2.3f + h2*30.0f)*1.5f);
                      if (sx<clip_x0 || sx>=clip_x1 || sy<clip_y0 || sy>=clip_y1) continue;
                      pset_blend(sx, sy, COL(255,214,150), 70);
                  }
              }
          } }
    }
    ui_vignette(clip_x0, clip_y0, clip_x1-clip_x0, clip_y1-clip_y0);
    // particles + floaters
    for (int i=0;i<MAXPART;i++) if (G.parts[i].alive){
        Particle *q=&G.parts[i];
        rect_fill(SX(q->x), SY(q->y), q->size, q->size, q->col);
    }
    for (int i=0;i<MAXFLOAT;i++) if (G.floats[i].alive){
        Floater *q=&G.floats[i];
        draw_text_center(SX(q->x), SY(q->y), q->text, 1, q->col);
    }
    G.camx-=shk_off_x; G.camy-=shk_off_y;   // hand the camera back unshaken
}

