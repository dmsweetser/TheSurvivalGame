// THE SURVIVAL GAME — render/mall_draw.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 2658-3011 in the Nov 2026 fine-grained refactor; code unchanged.

static void draw_buddy(Buddy *b);   // Banzai Buddies, drawn further down
// the Bug Mall interior: plank floors, a red carpet up the middle, four
// counters with a keeper bug behind each, and NO hostile anything

// the mall keepers (Daniel, Nov 2): five variations on a certain
// bearded, bespectacled co-founder of a fruit company — round face,
// full beard, glasses, big friendly smile — each in his station's
// outfit, head and shoulders clear above the counter with a warm
// lantern glow behind him so he actually reads at game scale.
static void draw_keeper(int st, float wx, float wy){
    float cz = G.zoom; if (cz<0.6f) cz=0.6f;
    int x = SX(wx), y = SY(wy) + (int)(fsin(G.anim_t*1.3f + st*1.7f)*cz);
    u32 skin = COL(240,200,164), beard = COL(128,106,88), hair = COL(160,148,134);
    // (the lantern-glow discs behind the head are GONE — Nov 5,
    // Daniel: "get rid of the halos behind the shopkeepers". The
    // warmth moved to light pools on the floor instead.)
    // shoulders and the station outfit (the counter covers the rest)
    u32 shirt = st==0? COL(206,74,60) : st==1? COL(238,236,230)
              : st==2? COL(238,236,230) : st==3? COL(186,164,110) : COL(230,230,234);
    ellipse_fill(x, y + (int)(17*cz), (int)(17*cz), (int)(10*cz), shirt);
    if (st==0){          // Hawaiian: little flowers on the chest band
        disc_fill(x-(int)(9*cz), y+(int)(14*cz), (int)(1.6*cz), COL(255,240,200));
        disc_fill(x+(int)(2*cz), y+(int)(15*cz), (int)(1.6*cz), COL(255,214,90));
        disc_fill(x+(int)(11*cz), y+(int)(13*cz), (int)(1.6*cz), COL(255,240,200));
        tri_fill(x-(int)(3*cz), y+(int)(10*cz), x+(int)(3*cz), y+(int)(10*cz), x, y+(int)(15*cz), skin);   // open collar
    } else if (st==1){   // whales only: black tux panels, a red bow tie, a gold whale pin
        rect_fill(x-(int)(16*cz), y+(int)(11*cz), (int)(7*cz), (int)(9*cz), COL(28,26,34));
        rect_fill(x+(int)(9*cz), y+(int)(11*cz), (int)(7*cz), (int)(9*cz), COL(28,26,34));
        tri_fill(x-(int)(4*cz), y+(int)(11*cz), x, y+(int)(13*cz), x-(int)(4*cz), y+(int)(15*cz), COL(160,30,40));
        tri_fill(x+(int)(4*cz), y+(int)(11*cz), x, y+(int)(13*cz), x+(int)(4*cz), y+(int)(15*cz), COL(160,30,40));
        disc_fill(x+(int)(6*cz), y+(int)(16*cz), (int)(2*cz), COL(255,214,90));
    } else if (st==2){   // banker: green vest + a gold tie
        rect_fill(x-(int)(16*cz), y+(int)(11*cz), (int)(7*cz), (int)(9*cz), COL(56,116,78));
        rect_fill(x+(int)(9*cz), y+(int)(11*cz), (int)(7*cz), (int)(9*cz), COL(56,116,78));
        rect_fill(x-(int)(1.5*cz), y+(int)(10*cz), (int)(3*cz), (int)(9*cz), COL(226,178,60));
    } else if (st==3){   // buddies: khaki collar + a paw-print badge
        rect_fill(x-(int)(8*cz), y+(int)(11*cz), (int)(16*cz), (int)(3*cz), COL(150,128,84));
        disc_fill(x+(int)(7*cz), y+(int)(15*cz), (int)(2*cz), COL(120,84,50));
        disc_fill(x+(int)(4*cz), y+(int)(12.5*cz), 1, COL(120,84,50));
        disc_fill(x+(int)(10*cz), y+(int)(12.5*cz), 1, COL(120,84,50));
    } else {             // bowling: blue shoulders + a red neckerchief
        ellipse_fill(x-(int)(11*cz), y+(int)(14*cz), (int)(7*cz), (int)(5*cz), COL(70,130,220));
        ellipse_fill(x+(int)(11*cz), y+(int)(14*cz), (int)(7*cz), (int)(5*cz), COL(70,130,220));
        tri_fill(x-(int)(3*cz), y+(int)(10*cz), x+(int)(3*cz), y+(int)(10*cz), x, y+(int)(15*cz), COL(200,50,50));
    }
    // neck, beard mass, then the face over it so the beard wraps
    // the chin and cheeks
    rect_fill(x-(int)(3*cz), y+(int)(8*cz), (int)(6*cz), (int)(5*cz), col_shade(skin, 0.92f));
    ellipse_fill(x, y+(int)(4*cz), (int)(10.5*cz), (int)(9.5*cz), beard);
    circle_fill(x, y, (int)(11*cz), OUTLINE);
    circle_shade(x, y-(int)(1*cz), (int)(10*cz), skin);
    ellipse_fill(x, y-(int)(9*cz), (int)(8.5*cz), (int)(3.5*cz), hair);   // thin grey cap, forehead bare
    ellipse_fill(x, y+(int)(5.5*cz), (int)(4.5*cz), (int)(3*cz), skin);   // mouth patch over the beard
    line_thick(x-(int)(2.5*cz), y+(int)(5*cz), x, y+(int)(7*cz), 1, OUTLINE);   // the smile
    line_thick(x, y+(int)(7*cz), x+(int)(2.5*cz), y+(int)(5*cz), 1, OUTLINE);
    disc_fill(x, y+(int)(2*cz), (int)(1.6*cz)+1, col_shade(skin, 0.9f));        // nose
    // the glasses: thin dark frames, eyes inside, brows over the top
    { int gy2 = y-(int)(2*cz), lw2 = (int)(7.5*cz), lh2 = (int)(6*cz);
      rect_line(x-lw2-(int)(1*cz), gy2, lw2, lh2, OUTLINE);
      rect_line(x+(int)(1*cz), gy2, lw2, lh2, OUTLINE);
      rect_fill(x-(int)(1*cz), gy2+(int)(2*cz), (int)(2*cz), 1, OUTLINE);
      disc_fill(x-(int)(4.5*cz), gy2+(int)(3*cz), 1, OUTLINE);
      disc_fill(x+(int)(4.5*cz), gy2+(int)(3*cz), 1, OUTLINE);
      line_thick(x-(int)(7*cz), gy2-(int)(2*cz), x-(int)(2*cz), gy2-(int)(2.5*cz), 1, beard);
      line_thick(x+(int)(2*cz), gy2-(int)(2.5*cz), x+(int)(7*cz), gy2-(int)(2*cz), 1, beard); }
}
static void draw_mall(void){
    rect_fill(0, 0, VW, VH, COL(22,18,32));
    float cz = G.zoom>0.01f? G.zoom : 1.0f;
    int tlx = (int)(G.camx - (VW/2)/cz) - 64, tly = (int)(G.camy - (VH/2)/cz) - 64;
    int gx0 = tlx/64 - (tlx<0?1:0), gy0 = tly/64 - (tly<0?1:0);
    int gw = (int)(VW/cz/64) + 4, gh = (int)(VH/cz/64) + 4, ts = (int)(64*cz) + 1;
    for (int gy=gy0; gy<gy0+gh; gy++) for (int gx=gx0; gx<gx0+gw; gx++){
        float wx = gx*64+32, wy = gy*64+32;
        if (wx<-450 || wx>450 || wy<MALL_CY-270 || wy>MALL_CY+270) continue;
        u32 c = col_shade(((gx+gy)&1)? COL(124,94,62) : COL(116,88,58),
                          0.96f + hash_f(gx, gy, 91)*0.07f);
        { Player *vp = &G.p[G.view_pi>=0? G.view_pi : 0];
          if (vp->whale_on & 512){   // WHALES ONLY: DISCO MALL — the floor cycles for the disco owner
              static const u32 DISCO[4] = { COL(150,80,140), COL(70,130,170), COL(170,130,60), COL(80,150,110) };
              c = col_shade(DISCO[(gx+gy+(int)(G.time*2.0f))&3], 0.96f + hash_f(gx, gy, 91)*0.07f);
          } }
        int tx = SX(gx*64), tyy = SY(gy*64);
        rect_fill(tx, tyy, ts, ts, c);
        rect_fill(tx, tyy+ts-2, ts, 1, col_shade(c, 0.88f));   // plank seam
    }
    // the red carpet: entrance to counters
    { int cx0=SX(-37), cy0=SY(MALL_CY-158), cw=(int)(74*cz), ch=(int)(352*cz);
      rect_fill(cx0, cy0, cw, ch, COL(148,46,50));
      rect_line(cx0, cy0, cw, ch, COL(198,164,84));
      rect_line(cx0+3, cy0+3, cw-6, ch-6, COL(120,36,40)); }
    // the mall's name, painted on the floor inside the entrance. It
    // used to hang on the north wall — where the HUD's zone pill slid
    // in front of it every time you walked north. Down here, under
    // your feet as you come in, nothing can cover it.
    draw_text_center(SX(0), SY(MALL_CY+78), "THE BUG MALL", 2, COL(255,220,130));
    // room walls: wood panelling with a gold chair rail, door gap south
    { u32 wallc = COL(84,58,38);
      rect_fill(SX(-408), SY(MALL_CY-232), (int)(816*cz), (int)(26*cz), wallc);
      rect_fill(SX(-408), SY(MALL_CY-210), (int)(816*cz), 2, COL(198,164,84));
      rect_fill(SX(-408), SY(MALL_CY+202), (int)(368*cz), (int)(26*cz), wallc);
      rect_fill(SX(40), SY(MALL_CY+202), (int)(368*cz), (int)(26*cz), wallc);
      rect_fill(SX(-408), SY(MALL_CY+202), (int)(816*cz), 2, COL(198,164,84));
      rect_fill(SX(-408), SY(MALL_CY-232), (int)(24*cz), (int)(460*cz), wallc);
      rect_fill(SX(384), SY(MALL_CY-232), (int)(24*cz), (int)(460*cz), wallc); }
    // v1.32 dressing: a string of party lights along the north wall,
    // under the chair rail — mall money well spent. Each bulb twinkles
    // on its own phase; the run is clipped by the view like everything.
    { static const u32 STRING_COL[4] = { COL(255,190,90), COL(255,120,140), COL(140,220,255), COL(160,255,170) };
      for (int bx2 = -384; bx2 <= 384; bx2 += 48){
          int sx2 = SX(bx2), sy2 = SY(MALL_CY-198);
          if (sx2 < -8 || sx2 > VW+8) continue;
          int bi = (bx2 + 384) / 48;
          float tw = 0.6f + 0.4f*fsin(G.anim_t*2.4f + (float)bi*1.7f);
          glow_ellipse(sx2, sy2, 7, 7, STRING_COL[bi&3], (int)(26.0f*tw));
          disc_fill(sx2, sy2, 2, STRING_COL[bi&3]);
          pset(sx2-1, sy2-1, COL(255,255,255));
      } }
    // exit mat
    { int mx0=SX(-38), my0=SY(MALL_CY+172), mw=(int)(76*cz), mh=(int)(34*cz);
      rect_fill(mx0, my0, mw, mh, COL(52,96,60)); rect_line(mx0, my0, mw, mh, COL(198,164,84));
      draw_text_center(SX(0), SY(MALL_CY+183), "EXIT", 2, COL(240,255,240)); }
    // the Banzai World gate, south-east: a grass patch, a red torii,
    // and a sign. Through it, you play as your buddy.
    { int cx = SX(BW_SX);
      int gx0=SX(BW_SX-56), gy0=SY(MALL_CY+150), gw=(int)(112*cz), gh=(int)(48*cz);
      rect_fill(gx0, gy0, gw, gh, COL(86,150,66)); rect_line(gx0, gy0, gw, gh, COL(60,110,48));
      for (int gt=0; gt<7; gt++){ int tx2 = SX(BW_SX-48 + (gt*37)%96), ty2 = SY(MALL_CY+158 + (gt*23)%34);
          line_thick(tx2, ty2, tx2-1, ty2-4, 1, COL(60,120,45)); line_thick(tx2+3, ty2, tx2+4, ty2-3, 1, COL(60,120,45)); }
      // torii: two posts and two lintels, vermilion
      int px1=SX(BW_SX-26), px2=SX(BW_SX+26), py0=SY(MALL_CY+158), py1=SY(MALL_CY+192);
      rect_fill(px1-3, py0, 6, py1-py0, COL(196,64,44)); rect_line(px1-3, py0, 6, py1-py0, OUTLINE);
      rect_fill(px2-3, py0, 6, py1-py0, COL(196,64,44)); rect_line(px2-3, py0, 6, py1-py0, OUTLINE);
      rect_fill(px1-10, py0-4, (px2-px1)+20, 6, COL(196,64,44)); rect_line(px1-10, py0-4, (px2-px1)+20, 6, OUTLINE);
      rect_fill(px1-6, py0+8, (px2-px1)+12, 4, COL(170,52,38));
      int sx0=SX(BW_SX-52), sy0=SY(MALL_CY+136), sw=(int)(104*cz), sh=(int)(13*cz);
      rect_fill(sx0, sy0, sw, sh, COL(46,40,34)); rect_line(sx0, sy0, sw, sh, COL(198,164,84));
      draw_text_center(cx, sy0+3, "BANZAI WORLD", 1, COL(255,240,210)); }
    // the Arcade door, east wall: a dark opening in the panelling, a
    // violet mat, and a buzzing cyan sign. The games live through here.
    { int dx0=SX(386), dy0=SY(MALL_CY+2), dw=(int)(18*cz), dh=(int)(44*cz);
      rect_fill(dx0, dy0, dw, dh, COL(16,12,26)); rect_line(dx0, dy0, dw, dh, COL(120,240,255));
      int mx0=SX(334), my0=SY(MALL_CY+8), mw=(int)(50*cz), mh=(int)(34*cz);
      rect_fill(mx0, my0, mw, mh, COL(74,40,110)); rect_line(mx0, my0, mw, mh, COL(120,240,255));
      int sx0=SX(316), sy0=SY(MALL_CY-26), sw=(int)(76*cz), sh=(int)(13*cz);
      rect_fill(sx0, sy0, sw, sh, COL(24,20,36)); rect_line(sx0, sy0, sw, sh, COL(120,240,255));
      draw_text_center(SX(354), sy0+3, "ARCADE", 1, COL(160,255,255)); }
    // THE SOURCE door, north wall (v1.26): a steel slab in the gap
    // between counters 0 and 1. Sealed, it wears a plaque with the
    // viewing player's live trophy count; open, its seams glow green.
    { Player *vp = &G.p[G.view_pi>=0? G.view_pi : 0];
      int open = src_gate_open(vp-G.p);
      int dx0=SX(SRC_DOOR_X-30), dy0=SY(MALL_CY-234), dw=(int)(60*cz), dh=(int)(30*cz);
      rect_fill(dx0, dy0, dw, dh, open? COL(18,40,28) : COL(26,24,34));
      rect_line(dx0, dy0, dw, dh, open? COL(120,255,170) : COL(110,104,128));
      rect_fill(dx0+dw/2-1, dy0+3, 2, dh-6, open? COL(120,255,170) : COL(60,56,74));   // the centre seam
      draw_text_center_fit(SX(SRC_DOOR_X), dy0+4, dw-4, "THE SOURCE", 1, open? COL(170,255,200) : COL(190,186,205));
      { char b[24]; char *q=b;
        if (open){ q=sappend(q, "OPEN"); }
        else { q=sappend(q, "SEALED "); q=sapp_int(q, trophy_earned_count(vp)); q=sappend(q, "/"); q=sapp_int(q, SRC_DOOR_TROPHIES); }
        *q=0;
        int px0=SX(SRC_DOOR_X-40), py0=SY(MALL_CY-196), pw=(int)(80*cz), ph=(int)(13*cz);
        rect_fill(px0, py0, pw, ph, COL(46,40,34)); rect_line(px0, py0, pw, ph, COL(198,164,84));
        draw_text_center(SX(SRC_DOOR_X), py0+3, b, 1, open? COL(170,255,200) : COL(255,200,150)); } }
    // (two potted shrubs stood here while the Gallery slot was empty;
    // Whales Only took the counter over on Nov 2 and the shrubs went)
    // hanging lanterns down the carpet
    for (int i=0;i<3;i++){
        int lx=SX(-200 + i*200), ly=SY(MALL_CY-30);
        glow_ellipse(lx, ly, (int)(30*cz), (int)(20*cz), COL(255,190,100), 52);   // v1.22: the lanterns cast light now
        disc_fill(lx, ly, 13, COL(255,196,96));
        disc_fill(lx, ly, 8, COL(255,226,150));
        rect_fill(lx-3, ly-6, 6, 8, COL(94,60,32));
    }
    // the four counters, each with its keeper behind it
    static const u32 SBAND[4] = { COL(190,60,50), COL(198,164,84), COL(70,170,90), COL(240,140,50) };
    for (int s=0;s<4;s++){
        int cx = SX(MALL_SX[s]);
        // warm light pooled on the floorboards in front of the
        // counter (the glow-up's answer to the old head halos)
        { int py2 = SY(MALL_CY-152);
          ellipse_fill(cx, py2, (int)(58*cz), (int)(17*cz), COL(140,110,70));
          ellipse_fill(cx, py2, (int)(40*cz), (int)(11*cz), COL(152,122,78)); }
        // keeper first, so the counter hides his lower half; his
        // head rides well clear of the counter top
        draw_keeper(s, MALL_SX[s], MALL_CY-216);
        int x0=SX(MALL_SX[s]-60), y0=SY(MALL_CY-198), cw=(int)(120*cz), ch=(int)(34*cz);
        shadow_ellipse(cx, y0+ch+2, cw/2, 4);
        rect_fill(x0, y0, cw, ch, COL(122,80,44)); rect_line(x0, y0, cw, ch, OUTLINE);
        rect_fill(x0, y0, cw, 6, COL(150,104,58));
        rect_fill(x0, y0+8, cw, 4, SBAND[s]);               // the station's colour stripe
        // the station's name, lettered on the counter front where the
        // shopper actually stands. (There used to be hanging signs at
        // double-size letters, but the boards were wider than the
        // counters' spacing — the names ran into each other — and from
        // anywhere but the counter itself they sat off-screen or
        // behind the HUD. The plate is always readable.)
        draw_text_center(cx, y0+20, MALL_STATION[s], 1, COL(255,240,210));
    }
    // the bowling counter, south-west: a keeper, three pins, and a
    // little lane rug pointing at the real lanes (behind the counter)
    { int cx = SX(BOWL_SX);
      { int rx0=SX(BOWL_SX-24), ry0=SY(MALL_CY+66), rw=(int)(48*cz), rh=(int)(82*cz);
        rect_fill(rx0, ry0, rw, rh, COL(216,186,130)); rect_line(rx0, ry0, rw, rh, COL(150,120,80));
        for (int pn=0;pn<3;pn++){ int px2 = SX(BOWL_SX + (pn-1)*13), py2 = SY(MALL_CY+80);
            ellipse_fill(px2, py2, 3, 5, COL(245,242,238)); rect_fill(px2-3, py2-2, 6, 2, COL(200,60,50)); } }
      draw_keeper(4, BOWL_SX, MALL_CY+140);
      int x0=SX(BOWL_SX-52), y0=SY(MALL_CY+158), cw=(int)(104*cz), ch=(int)(34*cz);
      shadow_ellipse(cx, y0+ch+2, cw/2, 4);
      rect_fill(x0, y0, cw, ch, COL(122,80,44)); rect_line(x0, y0, cw, ch, OUTLINE);
      rect_fill(x0, y0+ch-6, cw, 6, COL(150,104,58));
      rect_fill(x0, y0+ch-10, cw, 4, COL(80,130,220));         // bowling blue stripe
      draw_carcass(cx-30, y0+16, 8, 1, 0.6f);                  // the house ball: a carcass, of course
      { int sw2 = text_w(MALL_STATION[4], 2) + 16;
        int sx0 = cx - sw2/2, sy0 = SY(MALL_CY+108);
        rect_fill(sx0, sy0, sw2, 20, COL(94,60,32)); rect_line(sx0, sy0, sw2, 20, OUTLINE);
        rect_line(sx0+2, sy0+2, sw2-4, 16, COL(198,164,84));
        draw_text_center(cx, sy0+3, MALL_STATION[4], 2, COL(255,220,130)); } }
    // the TROPHY CASE (v1.25 feat 4), at the south wall's east
    // end, past the Banzai gate: a glass case on a wooden base. No
    // keeper — nobody sells anything here; the plaques are the
    // staff. Three little medals sit on the shelf whether or not
    // the viewer has earned any: they are the shape of the promise.
    // (The name board is size 1, like the gate's sign: at size 2 it
    // would run past the east wall from this corner spot.)
    { int cx = SX(CASE_SX);
      { int py2 = SY(MALL_CY+152);
        ellipse_fill(cx, py2, (int)(46*cz), (int)(14*cz), COL(140,110,70));
        ellipse_fill(cx, py2, (int)(32*cz), (int)(9*cz), COL(152,122,78)); }
      int x0=SX(CASE_SX-38), y0=SY(MALL_CY+158), cw=(int)(76*cz), ch=(int)(34*cz);
      shadow_ellipse(cx, y0+ch+2, cw/2, 4);
      rect_fill(x0, y0, cw, ch, COL(122,80,44)); rect_line(x0, y0, cw, ch, OUTLINE);
      rect_fill(x0, y0+ch-6, cw, 6, COL(150,104,58));
      rect_fill(x0, y0+8, cw, 4, COL(198,164,84));          // the case's gold stripe
      // a tiny gold cup lettered on the base front
      { int ux=cx, uy=y0+19;
        rect_fill(ux-4, uy, 8, 6, COL(250,200,50)); rect_line(ux-4, uy, 8, 6, COL(150,110,20));
        rect_fill(ux-6, uy+1, 2, 3, COL(250,200,50)); rect_fill(ux+4, uy+1, 2, 3, COL(250,200,50));
        rect_fill(ux-1, uy+6, 2, 3, COL(250,200,50)); rect_fill(ux-3, uy+9, 6, 2, COL(250,200,50)); }
      // the glass cabinet on the base's north edge: pale glass, a
      // bright frame, one shelf, three medal discs, a travelling glint
      { int gx0=SX(CASE_SX-32), gy0=SY(MALL_CY+138), gw=(int)(64*cz), gh=(int)(22*cz);
        rect_fill(gx0, gy0, gw, gh, COL(196,228,238)); rect_line(gx0, gy0, gw, gh, COL(240,250,255));
        rect_line(gx0+2, gy0+2, gw-4, gh-4, COL(150,190,205));
        rect_fill(gx0+3, gy0+gh-8, gw-6, 2, COL(122,80,44));
        static const u32 MCC[3] = { COL(250,200,50), COL(205,212,220), COL(198,124,64) };
        for (int mi=0; mi<3; mi++){ int mx = gx0 + gw*(mi+1)/4, my = gy0+gh-11;
            disc_fill(mx, my, 4, COL(90,80,60)); disc_fill(mx, my, 3, MCC[mi]); }
        { int glx = gx0 + (int)(G.anim_t*14.0f) % (gw>8? gw-8 : 8);
          line_thick(glx, gy0+3, glx+5, gy0+gh-4, 1, COL(255,255,255)); } }
      { int sw2 = text_w(MALL_STATION[5], 1) + 16;
        int sx0 = cx - sw2/2, sy0 = SY(MALL_CY+108);
        rect_fill(sx0, sy0, sw2, 15, COL(94,60,32)); rect_line(sx0, sy0, sw2, 15, OUTLINE);
        rect_line(sx0+2, sy0+2, sw2-4, 11, COL(198,164,84));
        draw_text_center(cx, sy0+3, MALL_STATION[5], 1, COL(255,220,130)); } }
    // the CHALLENGE BOARD (v1.25 feat 6), at the south wall's
    // west end — the case's mirror bookend: a cork notice board
    // on a wooden base. This week's card is pinned centre; the
    // pin is amber while the week is open and green once it is
    // done. Last week's card hangs askew beside it.
    { int cx = SX(BOARD_SX);
      { int py2 = SY(MALL_CY+152);
        ellipse_fill(cx, py2, (int)(46*cz), (int)(14*cz), COL(140,110,70));
        ellipse_fill(cx, py2, (int)(32*cz), (int)(9*cz), COL(152,122,78)); }
      int x0=SX(BOARD_SX-38), y0=SY(MALL_CY+158), cw=(int)(76*cz), ch=(int)(34*cz);
      shadow_ellipse(cx, y0+ch+2, cw/2, 4);
      rect_fill(x0, y0, cw, ch, COL(122,80,44)); rect_line(x0, y0, cw, ch, OUTLINE);
      rect_fill(x0, y0+ch-6, cw, 6, COL(150,104,58));
      rect_fill(x0, y0+8, cw, 4, COL(64,96,120));          // the board's steel-blue stripe
      // the board itself on the base's north edge: cork, a dark
      // top rail lettered with its name (a hanging sign here
      // would run into Bug Bowl's — the board is wide and flat,
      // so the name rides the rail instead), this week's paper
      // card, last week's askew beside it
      { int gx0=SX(BOARD_SX-32), gy0=SY(MALL_CY+132), gw=(int)(64*cz), gh=(int)(28*cz);
        rect_fill(gx0, gy0, gw, gh, COL(166,124,82)); rect_line(gx0, gy0, gw, gh, COL(94,60,32));
        rect_fill(gx0, gy0, gw, 9, COL(48,40,34));
        draw_text_center(gx0+gw/2, gy0+1, MALL_STATION[6], 1, COL(255,220,130));
        rect_fill(gx0+gw/2-11, gy0+12, 22, 13, COL(238,230,205)); rect_line(gx0+gw/2-11, gy0+12, 22, 13, COL(190,180,150));
        rect_fill(gx0+gw/2-8, gy0+18, 16, 2, COL(150,140,120));
        rect_fill(gx0+gw/2-8, gy0+22, 11, 2, COL(150,140,120));
        rect_fill(gx0+5, gy0+14, 12, 9, COL(210,200,168));
        { int done = G.wk_week!=0 && G.wk_done_week==G.wk_week;
          disc_fill(gx0+gw/2, gy0+13, 3, done? COL(90,190,110) : COL(230,160,40)); } } }
    // the BOUNTY BOARD (v1.25 feat 8), hanging on the WEST wall —
    // the one wall with nothing on it (south and north are full,
    // the east carries the Arcade). A hanging sign in the Arcade
    // sign's idiom: no floor footprint, no collision box, nothing
    // in any corridor. The poster names the current bounty in-world;
    // its pin is amber while the elite is at large, red while loose.
    { int cx = SX(BOUNTY_WX), cy = SY(BOUNTY_WY-30);
      { int py2 = SY(BOUNTY_WY+8);
        ellipse_fill(cx, py2, (int)(44*cz), (int)(13*cz), COL(140,110,70));
        ellipse_fill(cx, py2, (int)(30*cz), (int)(8*cz), COL(152,122,78)); }
      int bw2=(int)(118*cz), bh2=(int)(44*cz);
      int x0=cx-bw2/2, y0=cy-bh2/2;
      shadow_ellipse(cx, y0+bh2+3, bw2/2, 4);
      rect_fill(x0, y0, bw2, bh2, COL(94,60,32)); rect_line(x0, y0, bw2, bh2, OUTLINE);
      rect_fill(x0+3, y0+3, bw2-6, 11, COL(46,40,34));
      draw_text_center(cx, y0+5, "WANTED", 1, COL(255,220,130));
      rect_fill(x0+5, y0+17, bw2-10, bh2-22, COL(238,230,205)); rect_line(x0+5, y0+17, bw2-10, bh2-22, COL(190,180,150));
      { const char *nm = (G.net_mode==NET_CLIENT)? "" : ELITE_NAME[bn_name_idx(G.bn_serial)];
        if (nm[0]) draw_text_center(cx, y0+24, nm, 1, COL(150,50,40)); }
      disc_fill(x0+bw2-9, y0+8, 3, (G.net_mode!=NET_CLIENT && G.bn_live)? COL(220,60,50) : COL(230,160,40)); }
    // the PHOTO BOOTH (feat 9, given its own fixture by the v1.25
    // MALL ENLARGEMENT): a freestanding curtained booth standing
    // island-style west-centre — the Arcade's island-row precedent.
    // A plum curtain in a dark frame, a gold valance, a hanging
    // sign, and a little camera glyph on the front so the fixture
    // reads at a glance. The interact disc sits just south of the
    // body, where the shopper stands.
    { int cx = SX(BOOTH_SX);
      { int py2 = SY(BOOTH_DY);
        ellipse_fill(cx, py2, (int)(46*cz), (int)(14*cz), COL(140,110,70));
        ellipse_fill(cx, py2, (int)(32*cz), (int)(9*cz), COL(152,122,78)); }
      int x0=SX(BOOTH_SX-34), y0=SY(BOOTH_CY-34), cw=(int)(68*cz), ch=(int)(64*cz);
      shadow_ellipse(cx, y0+ch+2, cw/2, 4);
      rect_fill(x0, y0, cw, ch, COL(58,30,52)); rect_line(x0, y0, cw, ch, OUTLINE);
      for (int cs2=0; cs2<6; cs2++){   // curtain folds: plum stripes with a darker seam
          int fx0 = x0 + (int)((6+cs2*10)*cz);
          rect_fill(fx0, y0+8, (int)(5*cz), ch-8, (cs2&1)? COL(96,44,84) : COL(112,54,98));
      }
      rect_fill(x0, y0, cw, 8, COL(198,164,84)); rect_line(x0, y0, cw, 8, OUTLINE);   // the gold valance
      // the camera glyph on the curtain: a dark body, a gold lens
      { int kx=cx, ky=y0+(int)(34*cz);
        rect_fill(kx-9, ky-6, 18, 12, COL(24,20,36)); rect_line(kx-9, ky-6, 18, 12, COL(198,164,84));
        rect_fill(kx-3, ky-9, 6, 3, COL(24,20,36));
        disc_fill(kx, ky, 4, COL(150,200,230)); disc_fill(kx, ky, 2, COL(240,250,255)); }
      { int sw2 = text_w(MALL_STATION[8], 1) + 16;
        int sx0 = cx - sw2/2, sy0 = SY(BOOTH_CY-58);
        rect_fill(sx0, sy0, sw2, 15, COL(94,60,32)); rect_line(sx0, sy0, sw2, 15, OUTLINE);
        rect_line(sx0+2, sy0+2, sw2-4, 11, COL(198,164,84));
        draw_text_center(cx, sy0+3, MALL_STATION[8], 1, COL(255,220,130)); } }
    // (the Gallery's display pedestals — "yesterday's legends" —
    // went out with the Gallery itself on Nov 2)
    // the bank's vault door, gleaming behind the counter, beside
    // the keeper (it used to sit dead centre — where he now stands)
    { int vx=SX(MALL_SX[2]+44), vy=SY(MALL_CY-216);
      circle_fill(vx, vy, 15, COL(120,124,134)); circle_out(vx, vy, 15, COL(198,164,84));
      circle_out(vx, vy, 9, COL(90,94,104));
      for (int k=0;k<4;k++){ float a=k*TAU/4 + G.anim_t*0.1f;
          line_thick(vx, vy, vx+(int)(fcos(a)*7), vy+(int)(fsin(a)*7), 2, COL(198,164,84)); } }
    // the buddy pen: a little fence in front of the Banzai counter
    for (int fx2=MALL_SX[3]-56; fx2<=MALL_SX[3]+56; fx2+=14){
        int px=SX(fx2), py=SY(MALL_CY-132);
        rect_fill(px-2, py-10, 4, 12, COL(150,110,64)); rect_line(px-2, py-10, 4, 12, OUTLINE);
    }
    rect_fill(SX(MALL_SX[3]-58), SY(MALL_CY-140), (int)(116*cz), 3, COL(150,110,64));
    ui_vignette(clip_x0, clip_y0, clip_x1-clip_x0, clip_y1-clip_y0);
    // entities: buddies, then shoppers, then the usual sparkle
    for (int i=0;i<MAXP;i++) if (G.bud[i].active){
        float bx=G.bud[i].x, by=G.bud[i].y;
        if (!G.bud[i].in_bw && by>MALL_CY-300 && by<MALL_CY+300) draw_buddy(&G.bud[i]);
    }
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].in_mall) draw_player_sprite(&G.p[i]);
    for (int i=0;i<MAXPART;i++) if (G.parts[i].alive){
        Particle *q=&G.parts[i];
        if (q->y>MALL_CY-300 && q->y<MALL_CY+300) rect_fill(SX(q->x), SY(q->y), q->size, q->size, q->col);
    }
    for (int i=0;i<MAXFLOAT;i++) if (G.floats[i].alive){
        Floater *q=&G.floats[i];
        if (q->y>MALL_CY-300 && q->y<MALL_CY+300) draw_text_center(SX(q->x), SY(q->y), q->text, 1, q->col);
    }
    // v1.32 dressing: dust motes drifting in the mall's warm air —
    // the Entrances lamp-mote pattern, mall-wide and slow
    for (int m2=0; m2<14; m2++){
        u32 hh = hash2(m2*23+5, 11, 0xD057u);
        float ph = G.anim_t*(0.09f + (float)(hh%5)*0.02f) + (float)(hh>>8);
        int mx2 = SX(-380 + (int)(hh % 760)) + (int)(fsin(ph)*10.0f);
        int my2 = SY(MALL_CY-170 + (int)((hh>>6) % 320)) + (int)(fcos(ph*0.8f)*7.0f);
        if (mx2>=clip_x0 && mx2<clip_x1 && my2>=clip_y0 && my2<clip_y1)
            pset_blend(mx2, my2, COL(255,232,180), 60 + (int)(hh>>11)%50);
    }
    // WALK OF FAME marquee: flashes through the confetti colours while
    // the viewed player is mid-crawl under a heavy hat
    { Player *vp=&G.p[G.view_pi>=0? G.view_pi : 0];
      if (vp->wof>0){
          // v1.24: the marquee gets its bulbs — a chasing row of
          // confetti-coloured lamps over a soft glow, so the parade
          // moment reads as a marquee and not a caption
          int mcx = (clip_x0+clip_x1)/2, myy = clip_y0+150;
          glow_ellipse(mcx, myy+12, 170, 30, COL(255,190,80), 40);
          for (int i=0;i<9;i++){
              int bx = mcx-128+i*32, by = myy-13;
              u32 bc = WOF_COL[((int)(G.anim_t*8)+i)%6];
              disc_fill(bx, by, 5, COL(10,9,18));
              disc_fill(bx, by, 4, bc);
              pset(bx-1, by-1, COL(255,255,255));
          }
          draw_text_center(mcx, myy, "WALK OF FAME", 3,
                           WOF_COL[(int)(G.anim_t*8)%6]); } }
}
// THE ARCADE room (Nov 10): dark carpet, neon trim, and four cabinets
// against the north wall — one per game. West wall leads back to the mall.
