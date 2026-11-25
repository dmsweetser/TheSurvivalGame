// THE SURVIVAL GAME — modes/castle/castle_render.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from castle.c lines 342-520 in the Nov 2026 fine-grained refactor; code unchanged.

static void cas_render(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    int vx = G.view_x0, vy = G.view_y0, vw2 = G.view_w>0? G.view_w : VW, vh2 = G.view_h>0? G.view_h : VH;
    float cs = (float)vw2 / 640.0f; { float csy = (float)vh2 / 360.0f; if (csy < cs) cs = csy; }
    int ox = vx + (int)((vw2 - 640*cs) / 2), oy = vy + (int)((vh2 - 360*cs) / 2);
    int room = S->cas_room;
#define CRX(lx) (ox + (int)((lx) * cs))
#define CRY(ly) (oy + (int)((ly) * cs))
#define CRS(v) ((int)((v) * cs) < 1 ? 1 : (int)((v) * cs))
    rect_fill(vx, vy, vw2, vh2, COL(6, 8, 6));
    // floor: a dim checker, text-mode green — v1.22: every flagstone
    // wears its own shade now (hashed, never stored) with a sparse
    // grit speckle, so the courtyard reads as stone, not graph paper
    for (int ty = 0; ty < 12; ty++) for (int tx = 0; tx < 20; tx++){
        u32 fb = ((tx + ty) & 1)? COL(13, 19, 13) : COL(10, 15, 10);
        u32 fc = col_shade(fb, 0.90f + hash_f(tx, ty, 7)*0.20f);
        rect_fill(CRX(tx*32), CRY(ty*32), CRS(32) + 1, CRS(32) + 1, fc);
        float sp = hash_f(tx, ty, 21);
        if (sp > 0.70f){
            u32 sc = col_shade(fb, 1.35f);
            pset(CRX(tx*32 + (int)(sp*83.0f)%26 + 3), CRY(ty*32 + (int)(sp*47.0f)%26 + 3), sc);
            pset(CRX(tx*32 + (int)(sp*29.0f)%22 + 6), CRY(ty*32 + (int)(sp*61.0f)%22 + 6), sc);
        }
    }
    // the stone walls, with mortar lines
    { u32 wc = COL(74, 88, 74), wl = COL(52, 64, 52);
      rect_fill(CRX(0), CRY(0), CRS(640), CRS(CAS_Y0), wc);
      rect_fill(CRX(0), CRY(CAS_Y1), CRS(640), CRS(360 - CAS_Y1), wc);
      rect_fill(CRX(0), CRY(0), CRS(CAS_X0), CRS(360), wc);
      rect_fill(CRX(CAS_X1), CRY(0), CRS(640 - CAS_X1), CRS(360), wc);
      for (int bx = 0; bx < 640; bx += 32){ rect_fill(CRX(bx), CRY(0), 1, CRS(CAS_Y0), wl); rect_fill(CRX(bx), CRY(CAS_Y1), 1, CRS(360 - CAS_Y1), wl); }
      for (int by = 0; by < 360; by += 32){ rect_fill(CRX(0), CRY(by), CRS(CAS_X0), 1, wl); rect_fill(CRX(CAS_X1), CRY(by), CRS(640 - CAS_X1), 1, wl); }
      rect_fill(CRX(0), CRY(CAS_Y0), CRS(640), 2, COL(120, 220, 130));
      // doors: dark openings with a wooden leaf swung in
      if (CAS_DOORS[room]&1){ rect_fill(CRX(288), CRY(0), CRS(64), CRS(CAS_Y0), COL(4, 4, 4)); rect_fill(CRX(288), CRY(CAS_Y0 - 12), CRS(10), CRS(12), COL(120, 84, 44)); }
      if (CAS_DOORS[room]&4){ rect_fill(CRX(288), CRY(CAS_Y1), CRS(64), CRS(360 - CAS_Y1), COL(4, 4, 4)); rect_fill(CRX(342), CRY(CAS_Y1), CRS(10), CRS(12), COL(120, 84, 44)); }
      if (CAS_DOORS[room]&8){ rect_fill(CRX(0), CRY(162), CRS(CAS_X0), CRS(64), COL(4, 4, 4)); rect_fill(CRX(CAS_X0 - 12), CRY(162), CRS(12), CRS(10), COL(120, 84, 44)); }
      if (CAS_DOORS[room]&2){ rect_fill(CRX(CAS_X1), CRY(162), CRS(640 - CAS_X1), CRS(64), COL(4, 4, 4)); rect_fill(CRX(CAS_X1), CRY(216), CRS(12), CRS(10), COL(120, 84, 44)); }
      // the courtyard gate: iron bars on the south wall (never a door)
      if (room == 17){
          int lift = S->cas_won? CRS(16) : 0;
          rect_fill(CRX(288), CRY(CAS_Y1), CRS(64), CRS(360 - CAS_Y1), COL(4, 4, 4));
          for (int bx = 292; bx < 350; bx += 8) rect_fill(CRX(bx), CRY(CAS_Y1) - lift, CRS(3), CRS(360 - CAS_Y1), COL(150, 150, 160));
          rect_fill(CRX(288), CRY(CAS_Y1 + 10) - lift, CRS(64), CRS(3), COL(150, 150, 160));
          if (!S->cas_key) disc_fill(CRX(320), CRY(346), CRS(5), COL(255, 205, 40));
      }
    }
    // v1.22: the walls fall off into shade where they meet the floor,
    // and every doorway wears a pair of flickering sconces — the
    // castle is lit now, not just drawn
    blend_rect(CRX(CAS_X0), CRY(CAS_Y0), CRS(CAS_X1 - CAS_X0), CRS(9), COL(0, 0, 0), 56);
    blend_rect(CRX(CAS_X0), CRY(CAS_Y1) - CRS(9), CRS(CAS_X1 - CAS_X0), CRS(9), COL(0, 0, 0), 56);
    blend_rect(CRX(CAS_X0), CRY(CAS_Y0), CRS(9), CRS(CAS_Y1 - CAS_Y0), COL(0, 0, 0), 56);
    blend_rect(CRX(CAS_X1) - CRS(9), CRY(CAS_Y0), CRS(9), CRS(CAS_Y1 - CAS_Y0), COL(0, 0, 0), 56);
    { static const short SCN[8][3] = { {1, 272, 28}, {1, 368, 28}, {4, 272, 346}, {4, 368, 346},
                                       {8, 14, 146}, {8, 14, 242}, {2, 626, 146}, {2, 626, 242} };
      for (int si = 0; si < 8; si++){
          if (!(CAS_DOORS[room] & SCN[si][0])) continue;
          int sx2 = CRX(SCN[si][1]), sy2 = CRY(SCN[si][2]);
          int fl = 82 + (int)(fsin(G.anim_t*11.0f + (float)si*2.3f)*16.0f) + (int)(hash2(SCN[si][1], SCN[si][2], 5u)%12u);
          glow_ellipse(sx2, sy2, CRS(20), CRS(13), COL(255, 170, 70), fl);
          disc_fill(sx2, sy2, CRS(3), COL(255, 214, 120));
          disc_fill(sx2, sy2 - CRS(2), CRS(1) + 1, COL(255, 240, 200));
      } }
    // the room's stone block
    { const short *P = CAS_PIL[room];
      if (P[2] > 0){ rect_fill(CRX(P[0]), CRY(P[1]), CRS(P[2]), CRS(P[3]), COL(64, 76, 64)); rect_line(CRX(P[0]), CRY(P[1]), CRS(P[2]), CRS(P[3]), COL(96, 110, 96)); } }
    // items
    for (int id = 0; id < 17; id++){
        if (S->cas_taken[id] || S->cas_iroom[id] != room) continue;
        float fx, fy; cas_item_pos(pi, id, &fx, &fy);
        int cx = CRX(fx - CAS_OX), cy = CRY(fy - CAS_OY);
        int bob = (int)(fsin(G.anim_t * 3.0f + (float)id * 1.7f) * 2.0f);
        cy += CRS(bob);
        if (CAS_ITEM_KIND[id] == 0){           // a treasure: gold and glinting
            // v1.22: a slow breathing halo so the gold calls across the room
            glow_ellipse(cx, cy, CRS(14), CRS(10), COL(255, 205, 40), 58 + (int)(fsin(G.anim_t*3.0f + (float)id*1.7f)*18.0f));
            disc_fill(cx, cy, CRS(8), COL(255, 205, 40));
            tri_fill(cx - CRS(8), cy, cx, cy - CRS(10), cx + CRS(8), cy, COL(255, 230, 130));
            disc_fill(cx - CRS(3), cy - CRS(3), CRS(2), COL(255, 250, 220));
        } else if (CAS_ITEM_KIND[id] == 1){    // the sword
            rect_fill(cx - CRS(2), cy - CRS(13), CRS(4), CRS(18), COL(215, 220, 230));
            rect_fill(cx - CRS(6), cy + CRS(5), CRS(12), CRS(3), COL(255, 205, 40));
            rect_fill(cx - CRS(2), cy + CRS(8), CRS(4), CRS(6), COL(120, 84, 44));
        } else if (CAS_ITEM_KIND[id] == 2){    // the gate key
            disc_fill(cx - CRS(4), cy - CRS(4), CRS(5), COL(255, 205, 40));
            rect_fill(cx, cy - CRS(1), CRS(13), CRS(3), COL(255, 205, 40));
            rect_fill(cx + CRS(9), cy + CRS(2), CRS(3), CRS(5), COL(255, 205, 40));
        } else {                               // a tonic
            rect_fill(cx - CRS(5), cy - CRS(6), CRS(10), CRS(14), COL(200, 70, 70));
            rect_fill(cx - CRS(2), cy - CRS(10), CRS(4), CRS(4), COL(200, 200, 210));
            rect_fill(cx - CRS(5), cy - CRS(1), CRS(10), CRS(2), COL(240, 150, 150));
        }
    }
    // monsters
    for (int i = 0; i < 15; i++){
        if (!S->cas_malive[i] || S->cas_mroom[i] != room) continue;
        int cx = CRX(S->cas_mx[i] - CAS_OX), cy = CRY(S->cas_my[i] - CAS_OY);
        int ty = S->cas_mtype[i];
        // v1.22: the grounded monsters cast a shadow now; the bat flies
        if (ty == 1) shadow_ellipse(cx, cy + CRS(7), CRS(11), CRS(3));
        else if (ty == 2) shadow_ellipse(cx, cy + CRS(15), CRS(13), CRS(4));
        else if (ty == 3) shadow_ellipse(cx, cy + CRS(15), CRS(12), CRS(4));
        if (ty == 0){           // bat
            int flap = (int)(fsin(G.anim_t * 10.0f + (float)i * 2.0f) * CRS(5));
            tri_fill(cx, cy, cx - CRS(16), cy - CRS(6) - flap, cx - CRS(13), cy + CRS(5), COL(96, 84, 150));
            tri_fill(cx, cy, cx + CRS(16), cy - CRS(6) - flap, cx + CRS(13), cy + CRS(5), COL(96, 84, 150));
            disc_fill(cx, cy, CRS(6), COL(56, 48, 84));
            disc_fill(cx - CRS(2), cy - CRS(2), CRS(1) + 1, COL(255, 90, 90)); disc_fill(cx + CRS(2), cy - CRS(2), CRS(1) + 1, COL(255, 90, 90));
        } else if (ty == 1){    // snake
            for (int sg = 0; sg < 3; sg++){
                int sx2 = cx + (sg - 1) * CRS(9), sy2 = cy + (int)(fsin(G.anim_t * 6.0f + (float)(i + sg) * 1.3f) * CRS(3));
                disc_fill(sx2, sy2, CRS(6), COL(80, 180, 80));
            }
            disc_fill(cx + CRS(8), cy - CRS(2), CRS(1) + 1, COL(255, 220, 120));
        } else if (ty == 2){    // ogre
            rect_fill(cx - CRS(11), cy - CRS(6), CRS(22), CRS(20), COL(122, 142, 70));
            disc_fill(cx, cy - CRS(12), CRS(8), COL(150, 168, 96));
            rect_fill(cx - CRS(16), cy - CRS(4), CRS(5), CRS(14), COL(122, 142, 70));
            rect_fill(cx + CRS(11), cy - CRS(4), CRS(5), CRS(14), COL(122, 142, 70));
            disc_fill(cx - CRS(3), cy - CRS(13), CRS(1) + 1, COL(30, 26, 20)); disc_fill(cx + CRS(3), cy - CRS(13), CRS(1) + 1, COL(30, 26, 20));
        } else {                // demon
            rect_fill(cx - CRS(10), cy - CRS(8), CRS(20), CRS(22), COL(172, 62, 52));
            tri_fill(cx - CRS(10), cy - CRS(8), cx - CRS(13), cy - CRS(18), cx - CRS(4), cy - CRS(11), COL(240, 220, 180));
            tri_fill(cx + CRS(10), cy - CRS(8), cx + CRS(13), cy - CRS(18), cx + CRS(4), cy - CRS(11), COL(240, 220, 180));
            disc_fill(cx - CRS(3), cy - CRS(3), CRS(1) + 1, COL(255, 220, 90)); disc_fill(cx + CRS(3), cy - CRS(3), CRS(1) + 1, COL(255, 220, 90));
        }
        if (S->cas_mhp[i] < CAS_MHP[ty])   // hurt pips
            for (int hp2 = 0; hp2 < S->cas_mhp[i]; hp2++)
                rect_fill(cx - CRS(8) + hp2 * CRS(6), cy - CRS(24), CRS(4), CRS(3), COL(255, 120, 120));
    }
    // the adventurer (the buddy, in a green cloak — a nod to a clover)
    { int cx = CRX(b->x - CAS_OX), cy = CRY(b->y - CAS_OY) + (int)(fsin(b->walk_ph) * cs * 1.5f);
      ellipse_fill(cx, cy + CRS(13), CRS(11), CRS(4), COL(0, 0, 0));
      tri_fill(cx - CRS(10), cy + CRS(14), cx, cy - CRS(4), cx + CRS(10), cy + CRS(14), COL(52, 142, 72));
      rect_fill(cx - CRS(7), cy - CRS(2), CRS(14), CRS(14), COL(52, 142, 72));
      disc_fill(cx, cy - CRS(8), CRS(6), COL(240, 200, 164));
      tri_fill(cx - CRS(7), cy - CRS(9), cx, cy - CRS(17), cx + CRS(7), cy - CRS(9), COL(38, 110, 54));
      if (S->cas_sword){
          int hx = cx + (int)(fcos(b->aim) * CRS(13)), hy = cy + (int)(fsin(b->aim) * CRS(13));
          line_thick(cx, cy, hx, hy, CRS(3), COL(215, 220, 230));
          disc_fill(cx + (int)(fcos(b->aim) * CRS(8)), cy + (int)(fsin(b->aim) * CRS(8)), CRS(2), COL(255, 205, 40));
      }
      if (S->cas_hitcd > 0.35f){ rect_line(cx - CRS(13), cy - CRS(20), CRS(26), CRS(36), COL(255, 90, 90)); }   // hurt flash frame
    }
    // HUD, written on the top wall
    { char buf[80]; char *q;
      // v1.32: the left column takes the START-chip offset like the
      // other Banzai HUDs — the chip ate the CASTLE title whole (only
      // its last letter peeked out) and clipped the HP bar's left end.
      int cdx = (int)(bw_chip_dx(pi) / cs);
      draw_text(CRX(40 + cdx), CRY(10), S->cas_shuffle? "CASTLE - SHUFFLE" : "CASTLE", 1, COL(255, 205, 40));
      draw_text_center(CRX(320), CRY(10), CAS_NAMES[room], 1, COL(190, 235, 190));
      q = buf; q = sappend(q, "SCORE "); q = sapp_int(q, S->cas_score); *q = 0;
      draw_text(CRX(600) - (int)(slen(buf) * 8 * cs), CRY(10), buf, 1, COL(235, 238, 245));
      draw_text(CRX(40 + cdx), CRY(30), "HP", 1, COL(190, 235, 190));
      rect_fill(CRX(66 + cdx), CRY(30), CRS(110), CRS(8), COL(40, 44, 40));
      { int hw = b->maxhp > 0? (int)(110.0f * b->hp / b->maxhp) : 0; if (hw < 0) hw = 0;
        if (hw > 0) rect_fill(CRX(66 + cdx), CRY(30), CRS(hw), CRS(8), (b->hp * 3 < b->maxhp)? COL(230, 80, 80) : COL(110, 220, 120)); }
      q = buf; q = sappend(q, "TREASURES "); q = sapp_int(q, S->cas_treasures); q = sappend(q, "/13"); *q = 0;
      draw_text_center(CRX(320), CRY(30), buf, 1, COL(255, 220, 120));
      { int ix = CRX(470);
        if (S->cas_sword){ draw_text(ix, CRY(30), "SWORD", 1, COL(215, 220, 230)); ix += (int)(48 * cs); }
        if (S->cas_key) draw_text(ix, CRY(30), "KEY", 1, COL(255, 205, 40)); }
    }
    if (S->cas_won){
        rect_fill(CRX(120), CRY(120), CRS(400), CRS(110), COL(16, 20, 16));
        rect_line(CRX(120), CRY(120), CRS(400), CRS(110), COL(255, 205, 40));
        draw_text_center(CRX(320), CRY(142), "YOU ESCAPED THE CASTLE!", 2, COL(255, 220, 120));
        { char buf[64]; char *q = buf;
          q = sappend(q, "SCORE "); q = sapp_int(q, S->cas_score);
          q = sappend(q, "   BEST "); q = sapp_int(q, S->cas_score > b->cas_best? S->cas_score : b->cas_best); *q = 0;
          draw_text_center(CRX(320), CRY(182), buf, 1, COL(235, 238, 245)); }
    }
    if (S->note_t > 0){
        rect_fill(vx, vy + vh2 - 22, vw2, 22, COL(10, 12, 10));
        draw_text_center(vx + vw2/2, vy + vh2 - 15, S->note, 1, COL(220, 255, 220));
    }
#undef CRX
#undef CRY
#undef CRS
}

