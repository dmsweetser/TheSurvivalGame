// THE SURVIVAL GAME — render/srcroom_draw.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
//
// THE SOURCE ROOM (v1.26 Item 1a): a quiet machine room off the Mall's
// north wall — dark slate floor with cable runs, two server racks with
// blinking lights, and the terminal counter against the north wall,
// its screen forever scrolling code. Modest on purpose: the terminal
// panel is the room's real content.
static void draw_srcroom(void){
    rect_fill(0, 0, VW, VH, COL(10,12,18));
    float cz = G.zoom>0.01f? G.zoom : 1.0f;
    int tlx = (int)(G.camx - (VW/2)/cz) - 64, tly = (int)(G.camy - (VH/2)/cz) - 64;
    int gx0 = tlx/64 - (tlx<0?1:0), gy0 = tly/64 - (tly<0?1:0);
    int gw = (int)(VW/cz/64) + 4, gh = (int)(VH/cz/64) + 4, ts = (int)(64*cz) + 1;
    for (int gy=gy0; gy<gy0+gh; gy++) for (int gx=gx0; gx<gx0+gw; gx++){
        float wx = gx*64+32, wy = gy*64+32;
        if (wx<-450 || wx>450 || wy<SRC_CY-270 || wy>SRC_CY+270) continue;
        u32 c = col_shade(((gx+gy)&1)? COL(38,46,58) : COL(34,42,52),
                          0.96f + hash_f(gx, gy, 77)*0.07f);
        int tx = SX(gx*64), tyy = SY(gy*64);
        rect_fill(tx, tyy, ts, ts, c);
        if (((gx%4)+4)%4==0) rect_fill(tx, tyy, 2, ts, COL(24,30,40));   // cable run seam
    }
    // room walls: dark steel with a green chair rail, door gap south
    { u32 wallc = COL(30,38,48);
      rect_fill(SX(-408), SY(SRC_CY-232), (int)(816*cz), (int)(26*cz), wallc);
      rect_fill(SX(-408), SY(SRC_CY-210), (int)(816*cz), 2, COL(90,220,150));
      rect_fill(SX(-408), SY(SRC_CY+202), (int)(368*cz), (int)(26*cz), wallc);
      rect_fill(SX(40), SY(SRC_CY+202), (int)(368*cz), (int)(26*cz), wallc);
      rect_fill(SX(-408), SY(SRC_CY+202), (int)(816*cz), 2, COL(90,220,150));
      rect_fill(SX(-408), SY(SRC_CY-232), (int)(24*cz), (int)(460*cz), wallc);
      rect_fill(SX(384), SY(SRC_CY-232), (int)(24*cz), (int)(460*cz), wallc); }
    // exit mat
    { int mx0=SX(-38), my0=SY(SRC_CY+172), mw=(int)(76*cz), mh=(int)(34*cz);
      rect_fill(mx0, my0, mw, mh, COL(24,52,36)); rect_line(mx0, my0, mw, mh, COL(90,220,150));
      draw_text_center(SX(0), SY(SRC_CY+183), "EXIT", 2, COL(220,255,230)); }
    draw_text_center(SX(0), SY(SRC_CY+78), "THE SOURCE ROOM", 2, COL(120,200,150));
    // v1.32 dressing: data motes — green specks drifting in the
    // terminal light, slow as dust in a server room
    for (int m2=0; m2<12; m2++){
        u32 hh = hash2(m2*29+3, 7, 0xDA7Au);
        float ph = G.anim_t*(0.10f + (float)(hh%4)*0.025f) + (float)(hh>>8);
        int mx2 = SX(-360 + (int)(hh % 720)) + (int)(fsin(ph)*9.0f);
        int my2 = SY(SRC_CY-180 + (int)((hh>>6) % 360)) + (int)(fcos(ph*0.7f)*6.0f);
        if (mx2>=0 && mx2<VW && my2>=0 && my2<VH)
            pset_blend(mx2, my2, COL(140,255,180), 44 + (int)(hh>>11)%44);
    }
    // the terminal counter, north wall: a dark desk, a green screen
    // scrolling code, and the station name on its front
    { int cx = SX(0);
      { int py2 = SY(SRC_CY-152);
        ellipse_fill(cx, py2, (int)(58*cz), (int)(17*cz), COL(52,74,62));
        ellipse_fill(cx, py2, (int)(40*cz), (int)(11*cz), COL(60,86,70)); }
      int x0=SX(-60), y0=SY(SRC_CY-198), cw=(int)(120*cz), ch=(int)(34*cz);
      shadow_ellipse(cx, y0+ch+2, cw/2, 4);
      rect_fill(x0, y0, cw, ch, COL(28,36,44)); rect_line(x0, y0, cw, ch, OUTLINE);
      rect_fill(x0, y0, cw, 6, COL(90,220,150));
      draw_text_center(cx, y0+20, MALL_STATION[9], 1, COL(220,255,230));
      // the screen above the counter, code scrolling forever
      int sx0=SX(-44), sy0=SY(SRC_CY-252), sw=(int)(88*cz), sh=(int)(44*cz);
      rect_fill(sx0, sy0, sw, sh, COL(8,18,12)); rect_line(sx0, sy0, sw, sh, COL(90,220,150));
      { int rows=5, off=(int)(G.anim_t*7)%8;
        for (int r=0;r<rows;r++){
            int seed=(r*8+off)*31+7;
            int lw=18 + (int)(hash_f(seed, 3, 55)*52);
            int lx=sx0+5, ly=sy0+5+r*8;
            rect_fill(lx, ly, (int)(lw*cz), 3, (r&1)? COL(70,170,110) : COL(100,220,150));
            rect_fill(lx+(int)((lw+6)*cz), ly, (int)(10*cz), 3, COL(50,110,80));
        } } }
    // two server racks, west and east: dark cabinets, blinking lights
    for (int rk=0; rk<2; rk++){
        int rx = rk? 262 : -262;
        int x0=SX(rx-30), y0=SY(SRC_CY-84), cw=(int)(60*cz), ch=(int)(88*cz);
        shadow_ellipse(SX(rx), y0+ch+2, cw/2, 4);
        rect_fill(x0, y0, cw, ch, COL(20,26,34)); rect_line(x0, y0, cw, ch, OUTLINE);
        for (int u2=0;u2<5;u2++){
            int uy = y0 + (int)((6+u2*16)*cz);
            rect_fill(x0+4, uy, cw-8, (int)(12*cz), COL(28,36,46));
            for (int l2=0;l2<3;l2++){
                int on = (int)(hash_f(rk*17+u2*5+l2, (int)(G.anim_t*2), 99)*2)>0;
                disc_fill(x0+8+l2*7, uy+5, 2, on? COL(120,255,170) : COL(40,70,52));
            }
        }
    }
    ui_vignette(clip_x0, clip_y0, clip_x1-clip_x0, clip_y1-clip_y0);
    // entities: buddies, then visitors, then the usual sparkle
    for (int i=0;i<MAXP;i++) if (G.bud[i].active){
        float by=G.bud[i].y;
        if (!G.bud[i].in_bw && by>SRC_CY-300 && by<SRC_CY+300) draw_buddy(&G.bud[i]);
    }
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && in_srcroom[i]) draw_player_sprite(&G.p[i]);
    for (int i=0;i<MAXPART;i++) if (G.parts[i].alive){
        Particle *q=&G.parts[i];
        if (q->y>SRC_CY-300 && q->y<SRC_CY+300) rect_fill(SX(q->x), SY(q->y), q->size, q->size, q->col);
    }
    for (int i=0;i<MAXFLOAT;i++) if (G.floats[i].alive){
        Floater *q=&G.floats[i];
        if (q->y>SRC_CY-300 && q->y<SRC_CY+300) draw_text_center(SX(q->x), SY(q->y), q->text, 1, q->col);
    }
}
