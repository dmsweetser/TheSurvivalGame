// THE SURVIVAL GAME — mall/booth.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 1534-1769 in the Nov 2026 fine-grained refactor; code unchanged.

static void booth_frame_rect(int *fx, int *fy, int *fw, int *fh){
    if (G.ui_compact){
        int x = G.view_x0 + (G.view_w - 308)/2;
        *fx = x+12; *fy = STORE_Y+40; *fw = 138; *fh = 196;
    } else {
        *fx = STORE_X+16; *fy = STORE_Y+42; *fw = 208; *fh = 260;
    }
}
static void booth_speckle(int x, int y, int w, int h, int n, int seed, u32 c){
    for (int i=0;i<n;i++){
        int px = x + (int)(hash_f(i, seed, 7)*(float)(w-1));
        int py = y + (int)(hash_f(i, seed, 13)*(float)(h-1));
        pset(px, py, c);
    }
}
static void draw_booth_backdrop(int bd, int x, int y, int w, int h){
    int fl = y + h*2/3;   // the floor line: subjects stand on it
    if (bd==0){           // PLAZA DAWN
        for (int i=0;i<5;i++) rect_fill(x, y+i*(fl-y)/5, w, (fl-y)/5+1, col_shade(COL(250,196,148), 1.0f - i*0.05f));
        disc_fill(x+w-34, y+26, 11, COL(255,240,190)); disc_fill(x+w-34, y+26, 7, COL(255,248,215));
        rect_fill(x, fl, w, y+h-fl, COL(178,150,104));
        booth_speckle(x, fl+3, w, h-(fl-y)-5, 26, 41, COL(150,124,84));
    } else if (bd==1){    // FROZEN CACHE
        rect_fill(x, y, w, fl-y, COL(198,226,246));
        rect_fill(x, fl, w, y+h-fl, COL(238,246,252));
        ellipse_fill(x+26, fl+8, 30, 7, COL(248,252,255)); ellipse_fill(x+w-30, fl+10, 36, 8, COL(248,252,255));
        booth_speckle(x, y+2, w, fl-y-4, 46, 97, COL(255,255,255));
        booth_speckle(x, fl+2, w, h-(fl-y)-4, 16, 55, COL(214,232,244));
    } else if (bd==2){    // KERNEL BADLANDS
        rect_fill(x, y, w, fl-y, COL(226,124,66));
        rect_fill(x, y, w, (fl-y)/2, COL(240,160,84));
        disc_fill(x+36, y+30, 14, COL(232,74,52)); disc_fill(x+36, y+30, 10, COL(244,110,66));
        rect_fill(x, fl, w, y+h-fl, COL(122,62,44));
        for (int i=0;i<4;i++){ int cx0 = x + 10 + (int)(hash_f(i, 3, 71)*(float)(w-20));
            line_thick(cx0, fl+6+i*5, cx0+9, fl+8+i*5, 1, COL(84,40,30)); }
        booth_speckle(x, y+3, w, fl-y-6, 20, 23, COL(255,205,120));
    } else if (bd==3){    // CABLE JUNGLE
        rect_fill(x, y, w, fl-y, COL(34,86,54));
        rect_fill(x, y, w, (fl-y)/3, COL(24,64,40));
        rect_fill(x, fl, w, y+h-fl, COL(44,96,58));
        for (int i=0;i<6;i++){ int vx = x + 8 + (int)(hash_f(i, 8, 31)*(float)(w-16));
            line_thick(vx, y, vx+2, y + (fl-y)*2/3, 2, COL(22,58,36));
            disc_fill(vx+2, y + (fl-y)*2/3, 3, COL(72,150,84)); }
        for (int i=0;i<22;i++){ int rx = x + (int)(hash_f(i, 17, 43)*(float)(w-8)), ry = y + (int)(hash_f(i, 17, 59)*(float)(fl-y-10));
            line_thick(rx, ry, rx+2, ry+6, 1, COL(160,210,235)); }
    } else if (bd==4){    // THE BUG MALL
        rect_fill(x, y, w, fl-y, COL(84,58,38));
        rect_fill(x, y+10, w, 2, COL(198,164,84));
        rect_fill(x, fl, w, y+h-fl, COL(124,94,62));
        for (int sx2=x+8; sx2<x+w-4; sx2+=13) rect_fill(sx2, fl+2, 1, y+h-fl-4, COL(106,80,52));
        { int cw2 = w/2; rect_fill(x+(w-cw2)/2, fl, cw2, y+h-fl, COL(148,46,50));
          rect_line(x+(w-cw2)/2, fl, cw2, y+h-fl, COL(198,164,84)); }
        disc_fill(x+24, y+22, 6, COL(255,196,96)); disc_fill(x+w-24, y+22, 6, COL(255,196,96));
    } else {              // WALK OF FAME
        rect_fill(x, y, w, h, COL(16,14,28));
        glow_ellipse(x+w/2, fl-16, w/3, 26, COL(255,190,80), 44);
        rect_fill(x, fl, w, y+h-fl, COL(24,20,38));
        for (int i=0;i<30;i++){
            int px = x + (int)(hash_f(i, 29, 3)*(float)(w-2)), py = y + (int)(hash_f(i, 29, 9)*(float)(h-2));
            pset(px, py, WOF_COL[i%6]); pset(px+1, py, WOF_COL[i%6]);
        }
    }
}
// draw the pose (backdrop, subjects, plate) inside the frame's
// inner rect; snap=1 renders the frozen record, snap=0 the live crew
static void draw_booth_pose(int pi, int snap, int fx, int fy, int fw, int fh){
    Player *p=&G.p[pi]; Buddy *b=&G.bud[pi]; Portrait *pt2=&portrait[pi];
    int bd = snap? pt2->backdrop : port_bd[pi];
    int hat_on = snap? pt2->hat_on : port_hat[pi];
    int top = snap? pt2->top_worn : p->top_worn;
    int kg = snap? pt2->kg : (int)p->carried;
    int stack_n = snap? pt2->stack_n : p->stack_n;
    int name_idx = snap? pt2->name_idx : b->name_idx;
    int plate_h = G.ui_compact? 24 : 30;
    int ix = fx+4, iy = fy+4, iw = fw-8, ih = fh-8;
    draw_booth_backdrop(bd%PORT_BD_N, ix, iy, iw, ih);
    int feet_y = iy + ih - plate_h - 3;
    int vw2 = G.view_w>0? G.view_w : VW, vh2 = G.view_h>0? G.view_h : VH;
    float pz = G.zoom; G.zoom = 1.0f;
    // the DEV, wearing the hat (render_over's synthetic hero)
    { static Player hero;
      smemset(&hero, 0, sizeof(hero));
      hero.connected=1; hero.alive=1; hero.name[0]=0;
      hero.color = snap? pt2->pcolor : p->color;
      hero.weapon=0; hero.owned[0]=1; hero.hp=hero.maxhp=100;
      hero.aim=-0.5f; hero.walk_ph=0; hero.idle_t=1.0f;
      hero.whale_on = snap? pt2->whale_on : p->whale_on;
      hero.whale_own = p->whale_own;
      hero.top_worn = top;
      if (hat_on){
          hero.stack_n = stack_n;
          if (snap){ for (int i=0;i<stack_n;i++) hero.stack[i] = (float)pt2->kinds[i] + 0.5f; hero.carried = (float)kg; }
          else { for (int i=0;i<stack_n;i++) hero.stack[i] = p->stack[i]; hero.carried = p->carried; }
      }
      int hcx = ix + iw/2 - (b->active? 15 : 0);
      hero.x = G.camx + (float)(hcx - G.view_x0 - vw2/2);
      hero.y = G.camy + (float)(feet_y - 14 - G.view_y0 - vh2/2);
      draw_player_sprite(&hero); }
    // the buddy, on the real struct with borrowed pose fields
    if (b->active){
        float bx0=b->x, by0=b->y, ba0=b->aim, bw0=b->walk_ph, bm0=b->mv_t, bl0=b->lunge_t, bh0=b->hurt_t;
        int bo0=b->on_wall, bf0=b->fainted, bs0=b->species, bc0=b->color, wn0=p->whale_on;
        if (snap){ b->species = pt2->species; b->color = pt2->bud_color; p->whale_on = pt2->whale_on; }
        b->fainted=0; b->on_wall=-1; b->mv_t=0; b->lunge_t=0; b->hurt_t=0; b->walk_ph=0;
        b->aim = 1.35f;
        int bcx = ix + iw/2 + 31;
        b->x = G.camx + (float)(bcx - G.view_x0 - vw2/2);
        b->y = G.camy + (float)(feet_y - 8 - G.view_y0 - vh2/2);
        draw_buddy(b);
        b->x=bx0; b->y=by0; b->aim=ba0; b->walk_ph=bw0; b->mv_t=bm0; b->lunge_t=bl0; b->hurt_t=bh0;
        b->on_wall=bo0; b->fainted=bf0; b->species=bs0; b->color=bc0; p->whale_on=wn0;
    }
    G.zoom = pz;
    // the plate: name and hat weight, lettered on the frame
    rect_fill(ix, iy+ih-plate_h, iw, plate_h, COL(20,16,30));
    rect_fill(ix, iy+ih-plate_h, iw, 1, COL(198,164,84));
    if (G.ui_compact){
        draw_text_center(ix+iw/2, iy+ih-plate_h+3, BUD_ELF[name_idx%BUD_NAMES_N], 1, COL(255,214,90));
        if (!hat_on) draw_text_center(ix+iw/2, iy+ih-plate_h+13, "BARE-HEADED AND PROUD", 1, COL(240,236,220));
        else if (!stack_n) draw_text_center(ix+iw/2, iy+ih-plate_h+13, "NO HAT - YET", 1, COL(240,236,220));
        else { char bb[24]; char *q=bb; q=sapp_int(q, kg); q=sappend(q, " KG OF HAT"); *q=0;
               draw_text_center(ix+iw/2, iy+ih-plate_h+13, bb, 1, COL(240,236,220)); }
    } else {
        draw_text_center(ix+iw/2, iy+ih-plate_h+3, BUD_ELF[name_idx%BUD_NAMES_N], 2, COL(255,214,90));
        if (!hat_on) draw_text_center(ix+iw/2, iy+ih-plate_h+20, "BARE-HEADED AND PROUD", 1, COL(240,236,220));
        else if (!stack_n) draw_text_center(ix+iw/2, iy+ih-plate_h+20, "NO HAT - YET", 1, COL(240,236,220));
        else { char bb[24]; char *q=bb; q=sapp_int(q, kg); q=sappend(q, " KG OF HAT"); *q=0;
               draw_text_center(ix+iw/2, iy+ih-plate_h+20, bb, 1, COL(240,236,220)); }
    }
}
static void draw_booth_panel(void){
    Player *me=&G.p[G.store_pi];
    int snap = booth_stage[G.store_pi];
    int sel = (G.p2_slot>=0 && G.store_pi==G.p2_slot)? G.store_sel2 : G.store_sel;
    int y=STORE_Y, h=STORE_H;
    int w = G.ui_compact? 308 : STORE_W;
    int x = G.ui_compact? G.view_x0 + (G.view_w - w)/2 : STORE_X;
    if (!G.ui_compact) rect_fill(0, 0, VW, VH, COL(16,14,28));
    else rect_fill(G.view_x0, G.view_y0, G.view_w, G.view_h>0? G.view_h : VH, COL(16,14,28));
    rect_fill(x, y+1, w+8, h+8, COL(8,6,14));
    rect_fill(x-4, y-4, w+8, h+8, OUTLINE);
    rect_fill(x, y, w, h, COL(250,244,225));
    rect_line(x+3, y+3, w-6, h-6, COL(198,164,84));
    // pose first, words and chrome over it — the game-over rule:
    // a tall hat never hides a word, and never smears the frame
    int fx, fy, fw, fh; booth_frame_rect(&fx, &fy, &fw, &fh);
    rect_fill(fx, fy, fw, fh, COL(24,20,36));
    draw_booth_pose(G.store_pi, snap, fx, fy, fw, fh);
    rect_line(fx, fy, fw, fh, COL(198,164,84));
    rect_line(fx+2, fy+2, fw-4, fh-4, COL(150,118,52));
    { int nb2 = 7;   // the marquee bulbs, chasing like the Walk of Fame's own
      for (int i=0;i<nb2;i++){
          int bx = fx + (i+1)*fw/(nb2+1), by = fy+1;
          u32 bc = WOF_COL[((int)(G.anim_t*8)+i)%6];
          disc_fill(bx, by, G.ui_compact? 3 : 4, COL(10,9,18));
          disc_fill(bx, by, G.ui_compact? 2 : 3, bc);
          pset(bx-1, by-1, COL(255,255,255));
      } }
    { const char *bdn = PORT_BD_NAME[(snap? portrait[G.store_pi].backdrop : port_bd[G.store_pi])%PORT_BD_N];
      int cw2 = text_w(bdn, 1) + 8;
      rect_fill(fx+6, fy+6, cw2, 11, COL(20,16,30));
      draw_text(fx+10, fy+8, bdn, 1, COL(255,240,205)); }
    // the hand-off: this draw is showing the pose in THIS layout —
    // record the frame's true rect for the shell, exactly once
    if (!snap && port_arm[G.store_pi]){
        G.port_req_x=fx; G.port_req_y=fy; G.port_req_w=fw; G.port_req_h=fh;
        scpy(G.port_req_name, BUD_ELF[portrait[G.store_pi].name_idx%BUD_NAMES_N], 16);
        G.port_req_kg = portrait[G.store_pi].kg;
        G.port_req_serial++;
        port_arm[G.store_pi]=0;
    }
    // the band, over whatever hat spilled upward
    rect_fill(x, y, w, 32, COL(120,66,32));
    rect_fill(x, y+31, w, 2, COL(198,164,84));
    draw_text_center(x+w/2, y+3, MALL_STATION[8], 2, COL(255,226,150));
    draw_text_center(x+w/2, y+20, snap? "YOUR PORTRAIT - UNDER GLASS, NEVER FADING" : "THE PHOTO BOOTH - SAY 'BUGS!'", 1, COL(255,240,205));
    if (!G.ui_compact)
    { const char *sh = "SHIELD: UP";
      draw_text(x+w-14-text_w(sh,1), y+8, sh, 1, COL(150,220,255)); }
    // the control rows, in the standard row positions so the
    // standard input (keys, pad, y-band taps) needs no new math
    int rows = station_rows(8, me);
    int rx = fx+fw+14, rw = x+w-8-rx;
    char nb[48], rb[32];
    for (int i=0;i<rows;i++){
        int ry = MALL_ROW0 + i*STORE_ROWH;
        if (i%2==0) rect_fill(rx, ry-2, rw, STORE_ROWH, COL(243,236,210));
        rect_line(rx, ry-2, rw, STORE_ROWH, COL(228,219,192));
        if (i==sel){
            rect_fill(rx, ry-2, rw, STORE_ROWH, COL(255,224,138));
            rect_fill(rx, ry-2, 3, STORE_ROWH, COL(190,60,40));
            rect_line(rx, ry-2, rw, STORE_ROWH, COL(198,164,84));
        }
        const char *nm = station_row_name(8, i, me, nb);
        const char *rt = station_row_right(8, i, me, rb);
        if (G.ui_compact){ draw_text(rx+6, ry+5, nm, 1, COL(40,36,50)); draw_text(rx+rw-6-text_w(rt,1), ry+5, rt, 1, COL(150,95,30)); }
        else {
            // the booth's column is narrower than the full panel, and
            // the drawn advance runs wider than text_w admits — fit
            // the pair with a safety factor, stepping the value down
            // first and the name second, rather than collide
            int rsize = (text_w(rt,2)*23/20 > rw - text_w(nm,2)*23/20 - 20)? 1 : 2;
            int nsize = (text_w(nm,2)*23/20 + text_w(rt,rsize)*23/20 + 20 > rw)? 1 : 2;
            draw_text(rx+8, ry + (nsize==2? 2 : 5), nm, nsize, COL(40,36,50));
            draw_text(rx+rw-8-text_w(rt,rsize), ry + (rsize==2? 2 : 5), rt, rsize, COL(150,95,30));
        }
    }
    if (!snap && !G.ui_compact){
        // the booth's own little sign, under the controls
        draw_text_center(rx+rw/2, MALL_ROW0 + 5*STORE_ROWH + 26, "ONE POSE, KEPT FOREVER.", 1, COL(150,95,30));
        draw_text_center(rx+rw/2, MALL_ROW0 + 5*STORE_ROWH + 40, "THE CAMERA NEVER BLINKS.", 1, COL(150,95,30));
    }
    int by = y+h-40;
    int bh = G.ui_compact? 24 : 20;
    rect_fill(x+8, by-4, w-16, bh, COL(240,232,205));
    rect_line(x+8, by-4, w-16, bh, COL(198,164,84));
    rect_fill(x+8, by-4, 3, bh, COL(190,60,40));
    if (G.ui_compact){
        char bb[128]; scpy(bb, station_blurb(8, sel, me), 128);
        int cut=-1; for (int i=0; bb[i] && i<34; i++) if (bb[i]==' ') cut=i;
        if (cut>0 && bb[cut+1]){ bb[cut]=0;
            draw_text(x+14, by+1, bb, 1, COL(80,66,44));
            draw_text(x+14, by+12, bb+cut+1, 1, COL(80,66,44));
        } else draw_text(x+14, by+2, bb, 1, COL(80,66,44));
    }
    else draw_text(x+14, by+2, station_blurb(8, sel, me), 1, COL(80,66,44));
    if (pay_active) draw_text_center(x+w/2, y+h-13, "COUNTING CARCASSES...", 1, COL(190,60,40));
    else if (G.store_msg_t>0) draw_text_center(x+w/2, y+h-13, G.store_msg, 1, COL(60,140,60));
    else if (G.ui_compact){ if (hint_dev()==DEV_TOUCH) draw_text_center(x+w/2, y+h-13, "TAP A ROW - TAP AGAIN TO SELECT", 1, COL(88,76,54));
        else { char b[80]; char *q=b; q=sappend(q, "SHIELD UP - "); q=sappend(q, lbla(H_FIRE)); q=sappend(q, " SELECTS - "); q=sappend(q, lbla(H_BACK)); q=sappend(q, " LEAVES");
               draw_text_center(x+w/2, y+h-13, b, 1, COL(88,76,54)); } }
    else if (hint_dev()==DEV_TOUCH) draw_text_center(x+w/2, y+h-13, "TAP A ROW - TAP AGAIN TO SELECT - TAP OUTSIDE TO LEAVE", 1, COL(88,76,54));
    else { char b[96]; char *q=b; q=sappend(q, lbla(H_MOVE)); q=sappend(q, " CHOOSE - "); q=sappend(q, lbla(H_CONFIRM));
           q=sappend(q, " SELECTS - "); q=sappend(q, lbla(H_INT)); q=sappend(q, " LEAVES");
           draw_text_center(x+w/2, y+h-13, b, 1, COL(88,76,54)); }
}
