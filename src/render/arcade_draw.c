// THE SURVIVAL GAME — render/arcade_draw.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 3012-3134 in the Nov 2026 fine-grained refactor; code unchanged.

static void draw_arcade(void){
    rect_fill(0, 0, VW, VH, COL(10,8,20));
    float cz = G.zoom>0.01f? G.zoom : 1.0f;
    int tlx = (int)(G.camx - (VW/2)/cz) - 64, tly = (int)(G.camy - (VH/2)/cz) - 64;
    int gx0 = tlx/64 - (tlx<0?1:0), gy0 = tly/64 - (tly<0?1:0);
    int gw = (int)(VW/cz/64) + 4, gh = (int)(VH/cz/64) + 4, ts = (int)(64*cz) + 1;
    for (int gy=gy0; gy<gy0+gh; gy++) for (int gx=gx0; gx<gx0+gw; gx++){
        float wx = gx*64+32, wy = gy*64+32;
        if (wx<-410 || wx>410 || wy<ARC_CY-230 || wy>ARC_CY+360) continue;   // v1.27: south cull rose 70px with the room (+430 -> +360)
        u32 c = col_shade(((gx+gy)&1)? COL(52,40,84) : COL(46,36,74), 0.96f + hash_f(gx, gy, 91)*0.07f);
        int tx = SX(gx*64), tyy = SY(gy*64);
        rect_fill(tx, tyy, ts, ts, c);
        rect_fill(tx, tyy+ts-2, ts, 1, col_shade(c, 0.85f));
        // v1.32 dressing: arcade-carpet flecks — dim neon confetti in
        // the pile, hashed per tile, the way every good arcade was
        { float fl2 = hash_f(gx, gy, 133);
          if (fl2 > 0.55f){ int fx2 = tx + (int)(fl2*50) + 4, fy2 = tyy + (int)(fl2*34) + 8;
              u32 fc2 = fl2 > 0.85f? COL(120,72,150) : fl2 > 0.70f? COL(60,120,138) : COL(138,118,60);
              pset(fx2, fy2, fc2); pset(fx2+2, fy2+1, fc2); if (fl2 > 0.8f) pset(fx2+1, fy2-1, fc2); } }
    }
    draw_text_center(SX(0), SY(ARC_CY+163), "THE ARCADE", 2, COL(140,255,255));   // v1.27: floor text stays in the band between the island row and the raised WAR! row (was +210, north of the old +254 row)
    { u32 wallc = COL(30,24,48);
      rect_fill(SX(-386), SY(ARC_CY-192), (int)(772*cz), (int)(26*cz), wallc);
      rect_fill(SX(-386), SY(ARC_CY-170), (int)(772*cz), 2, COL(120,240,255));
      rect_fill(SX(-386), SY(ARC_CY+262), (int)(772*cz), (int)(26*cz), wallc);   // v1.27: south wall rose 70px with the clamp (+332 -> +262, just past the +258 clamp)
      rect_fill(SX(-386), SY(ARC_CY+262), (int)(772*cz), 2, COL(240,120,220));
      rect_fill(SX(-386), SY(ARC_CY-192), (int)(24*cz), (int)(160*cz), wallc);
      rect_fill(SX(-386), SY(ARC_CY+52), (int)(24*cz), (int)(236*cz), wallc);   // v1.27: west wall's south run shortened 70px to meet the raised south wall (306 -> 236)
      rect_fill(SX(362), SY(ARC_CY-192), (int)(24*cz), (int)(480*cz), wallc); }   // v1.27: east wall likewise (550 -> 480)
    { int mx0=SX(-360), my0=SY(ARC_CY+6), mw=(int)(40*cz), mh=(int)(38*cz);
      rect_fill(mx0, my0, mw, mh, COL(52,96,60)); rect_line(mx0, my0, mw, mh, COL(120,240,255));
      draw_text_center(SX(-340), SY(ARC_CY+20), "EXIT", 1, COL(240,255,240)); }
    draw_text_center(SX(0), SY(ARC_CY-186), "ARCADE", 2, COL(255,140,230));
    // v1.32 (Item 4): EIGHT cabinets, eight colours — this array had
    // seven entries for eight cabinets, so MORDOR FUN RUN (s==7) read
    // past the end and its marquee band, border and chase lights all
    // came out black: the near-black name text on a black band was
    // flatly unreadable (Daniel: "you can't read it"). Mordor gets
    // pale ash — black lettering reads crisply on it, and ash is the
    // colour of the place.
    static const u32 CABCOL[8] = { COL(240,140,40), COL(84,190,84), COL(70,130,230), COL(200,70,60), COL(120,220,130), COL(250,200,50), COL(216,64,54), COL(178,170,160) };
    for (int s=0;s<8;s++){
        int cx = SX(ARC_SX[s]);
        { int py2 = SY(ARC_CY + ARC_ROWY[s] + 54);
          ellipse_fill(cx, py2, (int)(68*cz), (int)(17*cz), COL(60,50,96));
          ellipse_fill(cx, py2, (int)(48*cz), (int)(11*cz), COL(72,60,110)); }
        int x0=SX(ARC_SX[s]-64), y0=SY(ARC_CY + ARC_ROWY[s]), cw=(int)(128*cz), ch=(int)(98*cz);
        shadow_ellipse(cx, y0+ch+2, cw/2, 4);
        rect_fill(x0, y0, cw, ch, COL(24,22,38)); rect_line(x0, y0, cw, ch, CABCOL[s]);
        rect_fill(x0, y0, cw, (int)(16*cz), CABCOL[s]);
        draw_text_center(cx, y0+4, ARC_NAME[s], 1, COL(16,14,24));
        for (int li=0; li<6; li++){ int lx = x0 + (int)((8+li*17)*cz), ly = y0 + (int)(19*cz);
            disc_fill(lx, ly, 2, ((li + (int)(G.anim_t*3)) & 1)? COL(255,230,140) : CABCOL[s]); }
        int sx0 = x0+(int)(12*cz), sy0 = y0+(int)(26*cz), sw = (int)(104*cz), sh = (int)(44*cz);
        rect_fill(sx0, sy0, sw, sh, COL(10,14,20)); rect_line(sx0, sy0, sw, sh, COL(90,100,120));
        if (s==0){
            rect_fill(sx0+sw/2-(int)(9*cz), sy0+(int)(6*cz), (int)(18*cz), (int)(32*cz), COL(120,84,44));
            rect_fill(sx0+sw/2+(int)(2*cz), sy0+(int)(20*cz), (int)(3*cz), (int)(3*cz), COL(255,220,120));
        } else if (s==1){
            rect_fill(sx0, sy0+sh-(int)(8*cz), sw, (int)(8*cz), COL(70,150,70));
            rect_fill(sx0+sw-(int)(14*cz), sy0+(int)(8*cz), (int)(2*cz), (int)(28*cz), COL(220,220,220));
            tri_fill(sx0+sw-(int)(12*cz), sy0+(int)(8*cz), sx0+sw-(int)(12*cz), sy0+(int)(16*cz), sx0+sw-(int)(3*cz), sy0+(int)(12*cz), COL(240,90,80));
            disc_fill(sx0+(int)(16*cz), sy0+sh-(int)(14*cz), (int)(5*cz), COL(255,220,120));
        } else if (s==2){
            rect_fill(sx0+(int)(12*cz), sy0+(int)(20*cz), (int)(6*cz), (int)(16*cz), COL(120,84,44));
            disc_fill(sx0+(int)(15*cz), sy0+(int)(14*cz), (int)(10*cz), COL(70,160,80));
            rect_fill(sx0+sw-(int)(24*cz), sy0+sh-(int)(14*cz), (int)(13*cz), (int)(13*cz), COL(150,104,58)); rect_line(sx0+sw-(int)(24*cz), sy0+sh-(int)(14*cz), (int)(13*cz), (int)(13*cz), OUTLINE);
        } else if (s==3) {
            rect_fill(sx0+sw/2-(int)(13*cz), sy0+(int)(16*cz), (int)(26*cz), (int)(16*cz), COL(96,110,60));
            rect_fill(sx0+sw/2-(int)(2*cz), sy0+(int)(4*cz), (int)(4*cz), (int)(14*cz), COL(70,80,44));
            disc_fill(sx0+sw/2, sy0+(int)(20*cz), (int)(6*cz), COL(70,80,44));
            rect_fill(sx0+sw/2-(int)(17*cz), sy0+(int)(14*cz), (int)(4*cz), (int)(20*cz), COL(50,54,40));
            rect_fill(sx0+sw/2+(int)(13*cz), sy0+(int)(14*cz), (int)(4*cz), (int)(20*cz), COL(50,54,40));
        } else if (s==4) {
            // CASTLE: a little stone tower with a flag, DOS-green
            rect_fill(sx0+sw/2-(int)(11*cz), sy0+(int)(16*cz), (int)(22*cz), (int)(22*cz), COL(96,110,96));
            rect_fill(sx0+sw/2-(int)(13*cz), sy0+(int)(12*cz), (int)(5*cz), (int)(6*cz), COL(96,110,96));
            rect_fill(sx0+sw/2-(int)(3*cz), sy0+(int)(12*cz), (int)(5*cz), (int)(6*cz), COL(96,110,96));
            rect_fill(sx0+sw/2+(int)(8*cz), sy0+(int)(12*cz), (int)(5*cz), (int)(6*cz), COL(96,110,96));
            rect_fill(sx0+sw/2-(int)(1*cz), sy0+(int)(24*cz), (int)(3*cz), (int)(14*cz), COL(30,36,30));
            rect_fill(sx0+sw/2-(int)(1*cz), sy0+(int)(2*cz), (int)(2*cz), (int)(10*cz), COL(200,200,210));
            tri_fill(sx0+sw/2+(int)(1*cz), sy0+(int)(2*cz), sx0+sw/2+(int)(1*cz), sy0+(int)(8*cz), sx0+sw/2+(int)(11*cz), sy0+(int)(5*cz), COL(120,220,130));
        } else if (s==5) {
            // SUPER BUDDY CART: a little kart under a checkered flag
            rect_fill(sx0, sy0+sh-(int)(9*cz), sw, (int)(9*cz), COL(52,52,60));
            rect_fill(sx0, sy0+sh-(int)(5*cz), sw, (int)(2*cz), COL(240,240,240));
            rect_fill(sx0+sw/2-(int)(16*cz), sy0+(int)(18*cz), (int)(32*cz), (int)(13*cz), COL(230,60,50));
            disc_fill(sx0+sw/2-(int)(13*cz), sy0+(int)(20*cz), (int)(4*cz), COL(16,14,24));
            disc_fill(sx0+sw/2+(int)(13*cz), sy0+(int)(20*cz), (int)(4*cz), COL(16,14,24));
            disc_fill(sx0+sw/2-(int)(13*cz), sy0+(int)(32*cz), (int)(4*cz), COL(16,14,24));
            disc_fill(sx0+sw/2+(int)(13*cz), sy0+(int)(32*cz), (int)(4*cz), COL(16,14,24));
            disc_fill(sx0+sw/2, sy0+(int)(22*cz), (int)(6*cz), COL(250,200,50));
            rect_fill(sx0+sw-(int)(10*cz), sy0+(int)(4*cz), (int)(2*cz), (int)(16*cz), COL(200,200,210));
            rect_fill(sx0+sw-(int)(8*cz), sy0+(int)(4*cz), (int)(4*cz), (int)(4*cz), COL(240,240,240));
            rect_fill(sx0+sw-(int)(4*cz), sy0+(int)(4*cz), (int)(4*cz), (int)(4*cz), COL(16,14,24));
            rect_fill(sx0+sw-(int)(8*cz), sy0+(int)(8*cz), (int)(4*cz), (int)(4*cz), COL(16,14,24));
            rect_fill(sx0+sw-(int)(4*cz), sy0+(int)(8*cz), (int)(4*cz), (int)(4*cz), COL(240,240,240));
        } else if (s==7) {
            // MORDOR FUN RUN (v1.32): the road east, a black mountain,
            // and the crack of doom glowing in its face. (Until now
            // this cabinet fell through to the WAR! scene below.)
            rect_fill(sx0, sy0+sh-(int)(6*cz), sw, (int)(6*cz), COL(120,100,70));   // the road's end: dirt
            tri_fill(sx0+(int)(58*cz), sy0+(int)(6*cz), sx0+(int)(76*cz), sy0+sh-(int)(6*cz), sx0+(int)(40*cz), sy0+sh-(int)(6*cz), COL(28,24,30));   // the mountain
            glow_ellipse(sx0+(int)(58*cz), sy0+(int)(16*cz), (int)(10*cz), (int)(7*cz), COL(255,110,40), 60);
            line_thick(sx0+(int)(58*cz), sy0+(int)(12*cz), sx0+(int)(56*cz), sy0+(int)(22*cz), (int)(2*cz), COL(255,110,40));   // the lava crack
            line_thick(sx0+(int)(56*cz), sy0+(int)(22*cz), sx0+(int)(60*cz), sy0+(int)(30*cz), (int)(2*cz), COL(230,84,30));
            line_thick(sx0+(int)(8*cz), sy0+sh-(int)(2*cz), sx0+(int)(48*cz), sy0+sh-(int)(8*cz), (int)(2*cz), COL(172,152,100));   // the road running in
            line_thick(sx0+(int)(30*cz), sy0+sh-(int)(2*cz), sx0+(int)(54*cz), sy0+sh-(int)(8*cz), (int)(2*cz), COL(172,152,100));
            disc_fill(sx0+(int)(16*cz), sy0+(int)(16*cz), (int)(3*cz), COL(240,235,220));   // a Shire moon, far behind
        } else {
            // WAR!: a gold bit-node, a buddy and a bug squaring off
            rect_fill(sx0, sy0+sh-(int)(7*cz), sw, (int)(7*cz), COL(120,108,86));
            tri_fill(sx0+(int)(16*cz), sy0+(int)(24*cz), sx0+(int)(22*cz), sy0+(int)(10*cz), sx0+(int)(28*cz), sy0+(int)(24*cz), COL(255,205,40));
            disc_fill(sx0+(int)(22*cz), sy0+(int)(22*cz), (int)(3*cz), COL(255,240,180));
            disc_fill(sx0+(int)(42*cz), sy0+(int)(24*cz), (int)(7*cz), COL(216,64,54));
            rect_fill(sx0+(int)(40*cz), sy0+(int)(12*cz), (int)(2*cz), (int)(5*cz), COL(216,64,54));
            rect_fill(sx0+(int)(46*cz), sy0+(int)(12*cz), (int)(2*cz), (int)(5*cz), COL(216,64,54));
            ellipse_fill(sx0+(int)(76*cz), sy0+(int)(24*cz), (int)(10*cz), (int)(7*cz), COL(150,90,200));
            disc_fill(sx0+(int)(70*cz), sy0+(int)(20*cz), (int)(2*cz), COL(255,240,240));
            disc_fill(sx0+(int)(82*cz), sy0+(int)(20*cz), (int)(2*cz), COL(255,240,240));
            rect_fill(sx0+(int)(64*cz), sy0+(int)(30*cz), (int)(3*cz), (int)(6*cz), COL(90,60,120));
            rect_fill(sx0+(int)(88*cz), sy0+(int)(30*cz), (int)(3*cz), (int)(6*cz), COL(90,60,120));
        }
        rect_fill(x0+(int)(8*cz), y0+(int)(76*cz), cw-(int)(16*cz), (int)(14*cz), COL(38,34,56)); rect_line(x0+(int)(8*cz), y0+(int)(76*cz), cw-(int)(16*cz), (int)(14*cz), OUTLINE);
        disc_fill(x0+(int)(24*cz), y0+(int)(83*cz), 3, COL(240,90,80));
        disc_fill(x0+(int)(38*cz), y0+(int)(83*cz), 3, COL(255,220,120));
        rect_fill(x0+cw-(int)(22*cz), y0+(int)(79*cz), (int)(6*cz), (int)(8*cz), COL(200,200,210));
    }
    ui_vignette(clip_x0, clip_y0, clip_x1-clip_x0, clip_y1-clip_y0);
    for (int i=0;i<MAXP;i++) if (G.bud[i].active){
        float by=G.bud[i].y;
        if (!G.bud[i].in_bw && by>ARC_CY-300 && by<ARC_CY+360) draw_buddy(&G.bud[i]);
    }
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].in_arcade) draw_player_sprite(&G.p[i]);
    for (int i=0;i<MAXPART;i++) if (G.parts[i].alive){
        Particle *q=&G.parts[i];
        if (q->y>ARC_CY-300 && q->y<ARC_CY+360) rect_fill(SX(q->x), SY(q->y), q->size, q->size, q->col);
    }
    for (int i=0;i<MAXFLOAT;i++) if (G.floats[i].alive){
        Floater *q=&G.floats[i];
        if (q->y>ARC_CY-300 && q->y<ARC_CY+360) draw_text_center(SX(q->x), SY(q->y), q->text, 1, q->col);
    }
}
// COMFORT (v1.25 feature 2): the shake consumer. render_world adds
// this jitter to the camera for one render and takes it back off
// before returning — gameplay, culling and the net snapshot never
// see it. Exported read-only for the test rig (dbg_shkoff).
static float shk_off_x=0, shk_off_y=0;
