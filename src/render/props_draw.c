// THE SURVIVAL GAME — render/props_draw.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 2422-2657 in the Nov 2026 fine-grained refactor; code unchanged.

static void draw_store_building(void){
    int x0=SX(-84), y0=SY(-108), x1=SX(84), y1=SY(-38);
    if (x1<-48||x0>VW+48||y1<-48||y0>VH+48) return;
    int cx=(x0+x1)/2, w=x1-x0;
    u32 wood = COL(122,80,44), wood_d = COL(94,60,32), plank = COL(238,220,180);
    shadow_ellipse(cx, y1+3, w/2, 6);
    // back wall: warm planks with seams
    rect_fill(x0, y0+18, w, y1-y0-18, plank);
    for (int py=y0+27; py<y1-4; py+=9) rect_fill(x0+2, py, w-4, 1, COL(216,194,150));
    rect_fill(x0, y1-6, w, 6, COL(226,204,162));
    // a little lit window high on the left wall, clear of the board
    rect_fill(x0+9, y0+27, 20, 14, wood_d); rect_line(x0+9, y0+27, 20, 14, OUTLINE);
    rect_fill(x0+12, y0+30, 14, 8, COL(255,214,120));
    rect_fill(x0+18, y0+30, 2, 8, wood_d); rect_fill(x0+12, y0+33, 14, 2, wood_d);
    // corner posts + top beam (the stall frame)
    rect_fill(x0-2, y0+14, 7, y1-y0-6, wood);  rect_line(x0-2, y0+14, 7, y1-y0-6, OUTLINE);
    rect_fill(x1-5, y0+14, 7, y1-y0-6, wood);  rect_line(x1-5, y0+14, 7, y1-y0-6, OUTLINE);
    rect_fill(x0-3, y0+16, w+6, 8, wood);      rect_line(x0-3, y0+16, w+6, 8, OUTLINE);
    // lanterns hanging off the posts
    for (int s=0;s<2;s++){
        int lx = s? x1-10 : x0+10;
        disc_fill(lx, y0+33, 6, COL(255,196,96));
        rect_fill(lx-3, y0+28, 7, 9, wood_d); rect_line(lx-3, y0+28, 7, 9, OUTLINE);
        disc_fill(lx, y0+32, 2, COL(255,240,190));
    }
    // striped awning with a scalloped hem, over the beam
    for (int i=0;i<8;i++){
        int ax = x0 + i*w/8; u32 c = (i%2)? COL(220,70,70) : COL(245,245,245);
        rect_fill(ax, y0+4, w/8, 13, c);
        disc_fill(ax + w/16, y0+17, w/16, c);
    }
    rect_line(x0, y0+4, w, 13, OUTLINE);
    rect_fill(x0, y0+2, w, 2, COL(198,164,84));
    // bunting strung between the posts
    { static const u32 flags[5] = { COL(220,70,70), COL(255,205,40), COL(110,190,255), COL(120,220,120), COL(180,120,255) };
      int bx0=x0+6, by0=y0+26, bx1=x1-6, sag=9;
      for (int i=0;i<14;i++){
          float t0=i/14.0f, t1=(i+1)/14.0f;
          int ax=bx0+(int)((bx1-bx0)*t0), ay=by0+(int)(sag*4*t0*(1-t0));
          int ex=bx0+(int)((bx1-bx0)*t1), ey=by0+(int)(sag*4*t1*(1-t1));
          line_thick(ax, ay, ex, ey, 1, OUTLINE);
          if (i%2==0) tri_fill(ax, ay, ax+8, ay, ax+4, ay+8, flags[(i/2)%5]);
      } }
    // the hanging sign (sways a touch) on two stubs off the awning
    { int sway = (int)(fsin(G.anim_t*0.8f)*1.5f);
      int sx0=x0+18+sway, sw=x1-x0-36;
      rect_fill(sx0+8, y0-14, 4, 18, wood_d); rect_fill(sx0+sw-12, y0-14, 4, 18, wood_d);
      rect_fill(sx0, y0-16, sw, 22, wood_d); rect_line(sx0, y0-16, sw, 22, OUTLINE);
      rect_line(sx0+2, y0-14, sw-4, 18, COL(198,164,84));
      rect_fill(sx0+22, y0-11, sw-44, 1, COL(122,88,52));
      draw_text_center(cx+sway, y0-9, "BUG MALL", 2, COL(255,220,90)); }
    // "we buy" board on the wall — two lines, so the whole phrase fits
    rect_fill(cx-52, y0+42, 104, 24, COL(250,242,218)); rect_line(cx-52, y0+42, 104, 24, wood_d);
    draw_text_center(cx, y0+45, "WE BUY BUGS", 1, COL(90,60,30));
    draw_text_center(cx, y0+55, "MALL: 10 STOPS", 1, COL(90,60,30));
    // counter, crates of stock, a sack, and the golden scale
    int cy=SY(-26);
    rect_fill(cx-62, cy-8, 17, 17, wood); rect_line(cx-62, cy-8, 17, 17, OUTLINE);
    rect_fill(cx-60, cy-17, 14, 9, wood);  rect_line(cx-60, cy-17, 14, 9, OUTLINE);
    ellipse_fill(cx-53, cy-19, 6, 3, COL(234,130,46));          // today's catch, on top
    rect_fill(cx+48, cy-9, 13, 9, COL(206,182,138)); rect_line(cx+48, cy-9, 13, 9, OUTLINE);
    rect_fill(cx+52, cy-12, 5, 3, wood_d);
    rect_fill(cx-32, cy, 64, 10, COL(140,95,55)); rect_line(cx-32, cy, 64, 10, OUTLINE);
    rect_fill(cx-32, cy, 64, 2, COL(172,124,78));
    { int gx=cx+19;   // the scale: post, beam, two pans — a bug being weighed
      rect_fill(gx, cy-13, 2, 13, COL(198,164,84));
      rect_fill(gx-11, cy-14, 24, 2, COL(198,164,84));
      line_thick(gx-9, cy-12, gx-13, cy-6, 1, COL(198,164,84)); line_thick(gx-9, cy-12, gx-5, cy-6, 1, COL(198,164,84));
      line_thick(gx+11, cy-12, gx+7, cy-6, 1, COL(198,164,84)); line_thick(gx+11, cy-12, gx+15, cy-6, 1, COL(198,164,84));
      ellipse_fill(gx-9, cy-5, 6, 2, COL(216,180,80)); ellipse_fill(gx+11, cy-5, 6, 2, COL(216,180,80));
      disc_fill(gx-9, cy-8, 3, COL(98,192,92)); }
    draw_text_center(cx-14, cy-6, lbla(H_INT), 1, COL(255,255,255));
    // welcome rug in front of the counter
    int ry=SY(-14);
    rect_fill(cx-34, ry, 68, 9, COL(178,60,54)); rect_line(cx-34, ry, 68, 9, COL(120,40,38));
    rect_line(cx-30, ry+2, 60, 5, COL(240,200,120));
}
static void draw_prop(int type, int x, int y, u32 seed){
    u32 edge = COL(52,50,48);
    int j1 = (int)(seed%5) - 2, j2 = (int)((seed>>3)%5) - 2;   // per-prop jitter
    switch (type){
    case 1: // shattered PCB + parts spill — flat debris, walkable
        ellipse_fill(x, y+5, 18, 6, COL(96,80,60));                 // scuffed dirt patch
        tri_fill(x-14, y+6, x+2, y+7, x-6, y-4, COL(38,118,60));    // main board shard
        tri_fill(x-14, y+6, x-6, y-4, x-13, y-3, COL(48,140,72));
        rect_fill(x-4, y+1, 12, 6, COL(42,128,64)); rect_line(x-4, y+1, 12, 6, COL(24,74,38));
        rect_fill(x-3, y+5, 10, 1, COL(216,178,64));                 // gold edge pins
        rect_fill(x-1, y-2, 5, 3, COL(28,28,34));                   // a loose chip
        rect_fill(x+5, y+2, 4, 2, COL(28,28,34));
        circle_fill(x-11+j1, y+2, 3, COL(178,52,44)); circle_out(x-11+j1, y+2, 3, edge);  // capacitor
        circle_out(x+11, y+3, 6, COL(52,102,200));                   // coiled CAT6
        circle_out(x+11, y+3, 4, COL(52,102,200));
        rect_fill(x+15, y+1, 4, 3, COL(196,202,210));                // its RJ45 plug
        break;
    case 2: // a crate with a CRT monitor in it — solid
        shadow_ellipse(x, y+13, 22, 5);
        rect_fill(x-11, y-17, 22, 15, COL(206,198,178)); rect_line(x-11, y-17, 22, 15, edge);  // CRT body
        rect_fill(x-8, y-14, 13, 9, COL(58,76,86));                  // screen
        tri_fill(x-8, y-14, x+1, y-14, x-8, y-7, COL(112,140,150));  // screen glare
        rect_fill(x+6, y-13, 2, 7, COL(160,152,134));                // side vents
        line_thick(x+8, y-3, x+12, y+3, 1, COL(30,30,34));           // power cord into the crate
        rect_fill(x-17, y-2, 34, 16, COL(146,104,58)); rect_line(x-17, y-2, 34, 16, COL(84,58,32));  // crate
        line_thick(x-17, y-2, x+17, y+14, 2, COL(120,84,44));        // cross planks
        line_thick(x+17, y-2, x-17, y+14, 2, COL(120,84,44));
        rect_fill(x-17, y-2, 34, 3, COL(164,120,68));
        break;
    case 3: // DRAM sticks + a coax coil, lying flat — walkable
        for (int k=0;k<2;k++){                                       // two RAM sticks
            int sx = x-13+k*7, sy = y-3+k*7;
            rect_fill(sx, sy, 20, 5, COL(34,110,58)); rect_line(sx, sy, 20, 5, COL(20,64,34));
            rect_fill(sx, sy+4, 20, 1, COL(216,178,64));             // gold contacts
            rect_fill(sx+2, sy+1, 4, 3, COL(26,26,32));
            rect_fill(sx+8, sy+1, 4, 3, COL(26,26,32));
            rect_fill(sx+14, sy+1, 4, 3, COL(26,26,32));
        }
        circle_out(x+11+j2, y-4, 7, COL(26,26,30));                  // coax coil
        circle_out(x+11+j2, y-4, 4, COL(26,26,30));
        rect_fill(x+16+j2, y-8, 4, 4, COL(178,182,188));             // F-connector
        rect_fill(x+17+j2, y-7, 2, 2, COL(120,124,130));
        circle_fill(x-9+j1, y+6, 2, COL(216,178,64));                // a stray gold pin
        break;
    case 4: // fax machine with a modem perched on top — solid
        shadow_ellipse(x, y+12, 20, 5);
        rect_fill(x-7, y-13, 12, 10, COL(238,236,228));              // paper sticking out the top
        rect_fill(x-5, y-11, 8, 1, COL(168,166,158));
        rect_fill(x-5, y-8, 8, 1, COL(168,166,158));
        rect_fill(x-15, y-3, 30, 14, COL(202,194,176)); rect_line(x-15, y-3, 30, 14, edge);   // fax body
        rect_fill(x-15, y-3, 30, 3, COL(178,170,152));               // feed slot shadow
        rect_fill(x-13, y+3, 6, 6, COL(160,152,134));                // handset rest
        for (int r2=0;r2<2;r2++) for (int c2=0;c2<3;c2++)            // keypad
            rect_fill(x+4+c2*4, y+3+r2*4, 2, 2, COL(90,88,84));
        rect_fill(x+3, y-8, 13, 6, COL(40,42,50)); rect_line(x+3, y-8, 13, 6, edge);          // modem
        line_thick(x+13, y-8, x+16, y-15, 1, COL(30,30,34));         // its antenna
        circle_fill(x+6, y-5, 1, COL(90,220,110));                   // modem LEDs
        circle_fill(x+10, y-5, 1, COL(240,180,60));
        break;
    case 5: // a game console tipped into the dirt, cables spilled — walkable
        ellipse_fill(x-2, y+8, 19, 5, COL(96,78,54));               // dirt mound it sank into
        tri_fill(x-15, y+7, x+11, y+7, x+15, y-6, COL(58,54,74));   // console, leaning
        tri_fill(x-15, y+7, x+15, y-6, x+5, y-9, COL(76,70,96));     // its top face
        rect_fill(x-2, y-9, 7, 6, COL(150,148,158));                 // cartridge, half out
        rect_fill(x-2, y-9, 7, 2, COL(196,84,60));                   // cartridge label stripe
        line_thick(x-13, y+4, x+12, y+4, 1, COL(38,36,48));          // slot shadow line
        circle_fill(x+9, y+1, 1, COL(232,64,54));                    // power LED, still on somehow
        line_thick(x+13, y+3, x+20, y+7, 2, COL(52,102,200));        // spilled CAT6...
        line_thick(x+20, y+7, x+26, y+4, 2, COL(52,102,200));
        rect_fill(x+25, y+1, 5, 4, COL(196,202,210));                // ...and its plug
        line_thick(x+11, y+6, x+19, y+11, 3, COL(24,24,28));         // spilled coax...
        rect_fill(x+18, y+10, 5, 4, COL(178,182,188));               // ...and its F-connector
        break;
    case 6: // KERNEL BADLANDS: a burnt server husk — solid
        shadow_ellipse(x, y+13, 20, 5);
        rect_fill(x-12, y-26, 24, 38, COL(46,40,38)); rect_line(x-12, y-26, 24, 38, COL(22,19,18));
        rect_fill(x-12, y-26, 24, 4, COL(64,54,48));                 // scorched crown
        for (int r2=0;r2<4;r2++) rect_fill(x-8, y-18+r2*8, 16, 3, COL(28,24,23));  // dead drive bays
        line_thick(x-12, y-26, x+3, y-12, 1, COL(16,14,13));         // the crack that killed it
        line_thick(x+3, y-12, x+1, y-2, 1, COL(16,14,13));
        circle_fill(x-6+j1, y+6, 1, COL(255,120,40));                // embers that never got the memo
        circle_fill(x+5, y+2, 1, COL(255,160,60));
        circle_fill(x+8+j2, y+8, 1, COL(230,80,30));
        break;
    case 7: // KERNEL BADLANDS: a cracked heat vent — walkable
        ellipse_fill(x, y+4, 17, 6, COL(60,38,28));                 // scorch ring
        ellipse_fill(x, y+4, 11, 4, COL(30,20,16));                  // the vent mouth
        rect_fill(x-8, y-6, 4, 8, COL(52,46,44)); rect_fill(x-8, y-7, 4, 2, COL(255,120,40));
        rect_fill(x+4, y-9, 4, 11, COL(52,46,44)); rect_fill(x+4, y-10, 4, 2, COL(255,150,50));
        circle_fill(x+j1, y+3, 1, COL(255,170,70)); circle_fill(x-4+j2, y+6, 1, COL(255,120,40));
        break;
    case 8: // FROZEN CACHE FIELDS: a frost-locked drive bank — solid
        shadow_ellipse(x, y+12, 21, 5);
        rect_fill(x-15, y-14, 30, 26, COL(150,172,190)); rect_line(x-15, y-14, 30, 26, COL(96,118,140));
        rect_fill(x-15, y-14, 30, 5, COL(232,242,250));              // snow cap
        tri_fill(x-10, y-9, x-6, y-9, x-8, y-3, COL(214,232,244));   // icicles
        tri_fill(x+2, y-9, x+6, y-9, x+4, y-4, COL(214,232,244));
        for (int r2=0;r2<3;r2++){ rect_fill(x-11, y-5+r2*7, 22, 4, COL(120,142,164)); rect_line(x-11, y-5+r2*7, 22, 4, COL(88,108,130)); }
        circle_fill(x+9, y+8, 1, COL(120,255,230));                  // one teal LED, frozen mid-blink
        break;
    case 9: // FROZEN CACHE FIELDS: RAM sticks in a snow drift — walkable
        ellipse_fill(x, y+6, 18, 6, COL(226,238,248));
        ellipse_fill(x-4, y+4, 10, 4, COL(240,248,252));
        rect_fill(x-12, y-2, 17, 4, COL(120,160,150)); rect_fill(x-12, y+2, 17, 1, COL(216,178,64));  // a stick, half out
        rect_fill(x-10, y-1, 3, 2, COL(60,90,86)); rect_fill(x-5, y-1, 3, 2, COL(60,90,86));
        rect_fill(x+2, y+3, 15, 4, COL(108,148,140)); rect_fill(x+2, y+7, 15, 1, COL(216,178,64));
        pset(x-6+j1, y-3, COL(255,255,255)); pset(x+9, y+1, COL(255,255,255)); pset(x+3+j2, y-2, COL(200,230,245));
        break;
    case 10: // NEON RAM FLATS: a memory tower, still lit — solid
        shadow_ellipse(x, y+12, 17, 5);
        rect_fill(x-8, y-30, 16, 42, COL(30,26,58)); rect_line(x-8, y-30, 16, 42, COL(70,64,140));
        line_thick(x-4, y-26, x-4, y+8, 2, COL(64,240,255));         // glowing traces up the spine
        line_thick(x+3, y-26, x+3, y+8, 2, COL(255,84,200));
        for (int r2=0;r2<4;r2++){ rect_fill(x-6, y-22+r2*9, 3, 3, COL(64,240,255)); rect_fill(x+3, y-18+r2*9, 3, 3, COL(255,84,200)); }
        circle_fill(x, y-33, 2, COL(190,250,255));                   // the beacon on top
        break;
    case 11: // NEON RAM FLATS: a circuit bloom on the ground — walkable
        ellipse_fill(x, y+3, 16, 6, COL(36,32,74));
        line_thick(x-12, y+3, x+12, y+3, 1, COL(64,240,255));
        line_thick(x, y-4, x, y+9, 1, COL(64,240,255));
        line_thick(x-8, y-2, x+7, y+8, 1, COL(255,84,200));
        circle_fill(x-12, y+3, 2, COL(64,240,255)); circle_fill(x+12, y+3, 2, COL(255,84,200));
        circle_fill(x, y-4, 2, COL(190,250,255)); circle_fill(x+j1, y+9, 1, COL(64,240,255));
        break;
    case 12: // CABLE JUNGLE: a cable tree — solid
        shadow_ellipse(x, y+12, 19, 5);
        rect_fill(x-4, y-16, 8, 28, COL(94,70,46)); rect_line(x-4, y-16, 8, 28, COL(60,44,28));
        disc_fill(x, y-24, 15, COL(44,92,48)); disc_fill(x-11, y-18, 10, COL(52,104,54)); disc_fill(x+11, y-19, 10, COL(40,84,44));
        disc_fill(x+j1, y-27, 7, COL(64,120,60));
        line_thick(x-9, y-20, x-11, y-2, 2, COL(26,26,30));          // hanging cables...
        rect_fill(x-13, y-1, 4, 4, COL(178,182,188));                // ...with the plugs still on
        line_thick(x+8, y-22, x+12, y-6, 2, COL(52,102,200));
        rect_fill(x+11, y-5, 4, 3, COL(196,202,210));
        break;
    case 13: // CABLE JUNGLE: a mossy keyboard slab — walkable
        rect_fill(x-16, y-4, 32, 12, COL(120,118,110)); rect_line(x-16, y-4, 32, 12, COL(80,78,70));
        for (int r2=0;r2<2;r2++) for (int c2=0;c2<6;c2++)
            rect_fill(x-13+c2*5, y-1+r2*5, 3, 3, COL(168,164,152));
        disc_fill(x-9, y-2, 4, COL(70,130,66)); disc_fill(x+11, y+6, 5, COL(62,120,60)); disc_fill(x+3+j1, y-4, 3, COL(80,140,72));
        break;
    case 14: // THE DEPTHS: a half-deleted file monolith — solid
        shadow_ellipse(x, y+12, 18, 5);
        rect_fill(x-11, y-28, 22, 40, COL(96,90,112)); rect_line(x-11, y-28, 22, 40, COL(58,54,74));
        rect_fill(x-11, y-28, 22, 6, COL(120,112,138));              // the header that survived
        rect_fill(x+2, y-14, 9, 10, COL(80,72,96));                  // a chunk that didn't (hole)
        rect_fill(x-11, y+2, 8, 8, COL(80,72,96));
        for (int r2=0;r2<3;r2++) rect_fill(x-7, y-18+r2*7, 10, 2, COL(140,132,158));   // faded file lines
        rect_fill(x-13, y-6, 4, 12, COL(110,90,140));                // a glitch slice, offset from the body
        pset(x+7+j1, y-24, COL(255,84,200)); pset(x-5, y+9, COL(64,240,255));
        break;
    case 15: // THE DEPTHS: glitch shards — walkable
        rect_fill(x-10+j1, y-2, 8, 4, COL(255,84,200));
        rect_fill(x-2, y+3, 10, 3, COL(64,240,255));
        rect_fill(x+4+j2, y-6, 6, 4, COL(150,144,168));
        rect_fill(x-13, y+5, 5, 3, COL(110,104,130));
        pset(x+11, y, COL(255,84,200)); pset(x-6, y-6, COL(64,240,255)); pset(x+1, y+8, COL(190,250,255));
        break;
    }
}
