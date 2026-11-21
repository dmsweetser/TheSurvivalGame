// THE SURVIVAL GAME — render/eddie.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 1552-1619 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
static void draw_eddie_carcass(int x, int y, int s, float tilt){
    int o = (int)(tilt*3);
    ellipse_fill(x+o, y, s+6, s+1, OUTLINE);
    ellipse_shade(x+o, y, s+4, s-1>2? s-1:2, COL(243,146,44));
    for (int i=0;i<5;i++){   // flattened fuzz bumps along the top
        int fx = x-s+2 + i*(s/2) + o;
        circle_fill(fx, y-s+1, 2, COL(243,146,44));
    }
    ellipse_fill(x+o, y, s/2+2, 2, OUTLINE);                 // squished nose
    ellipse_shade(x+o, y, s/2, 1, COL(228,54,46));
    line_thick(x-5+o, y-2, x-2+o, y+1, 1, OUTLINE); line_thick(x-5+o, y+1, x-2+o, y-2, 1, OUTLINE);
    line_thick(x+3+o, y-2, x+6+o, y+1, 1, OUTLINE); line_thick(x+3+o, y+1, x+6+o, y-2, 1, OUTLINE);
    line_thick(x-3+o, y-s+1, x-3+o, y-s-3, 1, OUTLINE);      // feet in the air
    line_thick(x+4+o, y-s+1, x+4+o, y-s-3, 1, OUTLINE);
}
static void draw_eddie_full(Bug *b, int x, int y, int r){
    int flash = b->hit_t>0;
    u32 fur  = flash? COL(255,255,255) : COL(243,146,44);
    u32 fsh  = flash? COL(230,230,230) : COL(214,116,32);
    u32 belly= flash? COL(255,255,255) : COL(255,224,168);
    u32 nose = flash? COL(255,255,255) : COL(228,54,46);
    shadow_ellipse(x, y+r-1, r+3, r/3+1);
    // two little feet, shuffling with the leg phase
    int fo = (int)(fsin(b->leg_ph)*2.0f);
    ellipse_fill(x-r/2+fo, y+r-2, r/3+2, 3, OUTLINE);
    ellipse_fill(x+r/2-fo, y+r-2, r/3+2, 3, OUTLINE);
    ellipse_shade(x-r/2+fo, y+r-2, r/3, 2, COL(146,84,30));
    ellipse_shade(x+r/2-fo, y+r-2, r/3, 2, COL(146,84,30));
    // fuzz: bumps all around the rim, jittered by the bug's own seed
    for (int i=0;i<12;i++){
        float a = i*0.5236f + b->seed*3.0f;
        int bx = x + (int)(fcos(a)*(r+1)), by = y + (int)(fsin(a)*(r*3/4+1));
        circle_fill(bx, by, r/6+2, fur);
    }
    // round body + cream belly
    ellipse_fill(x, y, r+4, r*3/4+4, OUTLINE);
    ellipse_shade(x, y, r+2, r*3/4+2, fur);
    ellipse_shade(x, y, r, r*3/4, fsh);
    ellipse_shade(x, y+r/3, r*3/5, r/3, belly);
    gloss(x, y, r+2);
    // googly eyes above the nose, tracking like any bug's
    int look = (int)(fsin(G.anim_t*2 + b->seed)*1.5f);
    int ey = y - r/3, ex1 = x - r/3, ex2 = x + r/3, er = r/5+2;
    circle_fill(ex1, ey, er+1, OUTLINE); circle_fill(ex2, ey, er+1, OUTLINE);
    circle_fill(ex1, ey, er, COL(255,255,255)); circle_fill(ex2, ey, er, COL(255,255,255));
    circle_fill(ex1+look, ey, er/2+1, OUTLINE); circle_fill(ex2+look, ey, er/2+1, OUTLINE);
    pset(ex1+look-1, ey-1, COL(255,255,255)); pset(ex2+look-1, ey-1, COL(255,255,255));
    // THE nose: one big shiny red button right in the middle
    int nr = r*55/100; if (nr<3) nr=3;
    circle_fill(x, y+2, nr+2, OUTLINE);
    circle_shade(x, y+2, nr, nose);
    pset_blend(x-nr/3, y+2-nr/3, COL(255,255,255), 200);
    pset_blend(x-nr/3+1, y+2-nr/3, COL(255,255,255), 140);
    pset_blend(x-nr/3, y+2-nr/3+1, COL(255,255,255), 140);
    // hp bar on big bugs + breakpoint symbol, same as any bug
    if (b->maxhp>=40 && b->hp<b->maxhp){
        int wpx = r*2;
        rect_fill(x-r, y-r-10, wpx, 3, COL(40,30,40));
        rect_fill(x-r, y-r-10, (int)(wpx*b->hp/b->maxhp), 3, COL(120,230,90));
    }
    if (b->bp_t>0){
        circle_out(x+r-2, y-r-4, 7, COL(70,140,255));
        rect_fill(x+r-5, y-r-8, 2, 7, COL(255,255,255));
        rect_fill(x+r-1, y-r-8, 2, 7, COL(255,255,255));
    }
}
static int g_gold_carcass = 0;   // Whales Only: the hat being drawn is golden
