// THE SURVIVAL GAME — enemies/draw_bugs.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 1620-1733 in the Nov 2026 fine-grained refactor; code unchanged.

static void draw_carcass(int x, int y, int s, int kind, float tilt){
    if (G.eddie){ draw_eddie_carcass(x, y, s, tilt); return; }
    // a dead bug, belly-up: squished body, legs in the air, X eyes
    u32 col = g_gold_carcass? COL(226,168,44) : BUG_COL[kind&3];
    int o = (int)(tilt*3);
    ellipse_fill(x+o, y, s+5, s+2, OUTLINE);
    ellipse_shade(x+o, y, s+3, s, g_gold_carcass? COL(255,224,130) : BUG_BELLY[kind&3]);
    ellipse_shade(x+o, y-1, s, s-2>2? s-2:2, col);   // a patch of shell color
    pset_blend(x-2+o, y-3, COL(255,255,255), 120);   // waxy sheen
    pset_blend(x-1+o, y-3, COL(255,255,255), 90);
    for (int l=0;l<3;l++){
        int lx = x - s + l*(s) + o;
        line_thick(lx, y-2, lx-3, y-7-(l&1)*2, 1, OUTLINE);
        line_thick(lx+4, y-2, lx+7, y-7-((l+1)&1)*2, 1, OUTLINE);
    }
    // X eyes
    line_thick(x-4+o, y-3, x-1+o, y, 1, OUTLINE); line_thick(x-4+o, y, x-1+o, y-3, 1, OUTLINE);
    line_thick(x+2+o, y-3, x+5+o, y, 1, OUTLINE); line_thick(x+2+o, y, x+5+o, y-3, 1, OUTLINE);
}
static void draw_bug_sprite(Bug *b){
    int x=SX(b->x), y=SY(b->y);
    if (x<-70||x>VW+70||y<-70||y>VH+70) return;
    int r=(int)(bug_radius(b->w)*(G.zoom>0.01f? G.zoom : 1.0f)); if (r<3) r=3;
    if (G.eddie){ draw_eddie_full(b, x, y, r); return; }
    u32 col = b->hit_t>0? COL(255,255,255) : BUG_COL[b->kind];
    u32 dark = col_shade(col, 0.72f);
    shadow_ellipse(x, y+r-1, r+3, r/3+1);
    // legs: 6 (spiders 8), two-segment with a knee, scuttling with leg_ph
    int legs = b->kind==2? 8 : 6;
    for (int l=0;l<legs;l++){
        float side = (l%2==0)? -1.0f: 1.0f;
        float row = (float)(l/2)/(legs/2-1) - 0.5f;      // -0.5..0.5 along body
        float wig = fsin(b->leg_ph + l*1.7f)*3.0f;
        int lx = x + (int)(row*r*1.2f);
        int ly = y + (int)(side*r*0.55f);
        int kx = lx + (int)(wig*0.6f + row*3), ky = ly + (int)(side*(r*0.38f));
        int fx2 = lx + (int)(wig + row*5), fy2 = ly + (int)(side*(r*0.68f));
        line_thick(lx, ly, kx, ky, 1, OUTLINE);
        line_thick(kx, ky, fx2, fy2, 1, OUTLINE);
        pset(fx2, fy2, dark);
    }
    if (b->kind==3){ // moth wings: shaded, spotted, gently translucent tips
        int flap = (int)(fsin(b->leg_ph*1.6f)*4);
        ellipse_fill(x-r-4, y-4-flap, r/2+5, r/3+4, OUTLINE);
        ellipse_fill(x+r+4, y-4-flap, r/2+5, r/3+4, OUTLINE);
        ellipse_shade(x-r-4, y-4-flap, r/2+3, r/3+2, BUG_BELLY[3]);
        ellipse_shade(x+r+4, y-4-flap, r/2+3, r/3+2, BUG_BELLY[3]);
        circle_fill(x-r-6, y-6-flap, 2, col_shade(BUG_BELLY[3],0.8f));
        circle_fill(x+r+2, y-6-flap, 2, col_shade(BUG_BELLY[3],0.8f));
    }
    if (b->kind==2){ // spider abdomen bulging out the back
        circle_fill(x, y+r/2, r*3/4+2, OUTLINE);
        circle_shade(x, y+r/2, r*3/4, dark);
        circle_fill(x-3, y+r/2-2, 2, col); circle_fill(x+4, y+r/2+3, 2, col);
    }
    // body: outlined, sphere-shaded, glossed
    ellipse_fill(x, y, r+4, r*3/4+4, OUTLINE);
    ellipse_shade(x, y, r+2, r*3/4+2, col);
    gloss(x, y, r+2);
    // shell markings, per kind
    if (b->kind==0){ // roach: two darker bands across the shell
        line_thick(x-r*2/3, y-r/4, x+r*2/3, y-r/4, 2, dark);
        line_thick(x-r*3/4, y+r/6, x+r*3/4, y+r/6, 2, dark);
    } else if (b->kind==1){ // beetle: dorsal seam + rivet dots
        line_thick(x, y-r*2/3, x, y+r*2/3, 1, dark);
        circle_fill(x-r/3, y-r/5, 1, dark); circle_fill(x+r/3, y-r/5, 1, dark);
        circle_fill(x-r/3, y+r/4, 1, dark); circle_fill(x+r/3, y+r/4, 1, dark);
    } else if (b->kind==2){ // spider: warning chevrons
        line_thick(x-4, y-3, x, y+1, 1, COL(255,220,90));
        line_thick(x, y+1, x+4, y-3, 1, COL(255,220,90));
    }
    ellipse_shade(x, y+2, r*3/5, r/2, BUG_BELLY[b->kind]);
    // head + googly eyes that track the wobble
    int hx = x + (int)(fcos(b->wobble)*2), hy = y - r/3;
    circle_fill(hx, hy, r/2+3, OUTLINE);
    circle_shade(hx, hy, r/2+1, col);
    int look = (int)(fsin(G.anim_t*2 + b->seed)*1.5f);
    circle_fill(hx-r/4, hy-1, r/4+1, COL(255,255,255));
    circle_fill(hx+r/4, hy-1, r/4+1, COL(255,255,255));
    circle_fill(hx-r/4+look, hy-1, r/8+1, OUTLINE);
    circle_fill(hx+r/4+look, hy-1, r/8+1, OUTLINE);
    pset(hx-r/4+look-1, hy-2, COL(255,255,255));
    pset(hx+r/4+look-1, hy-2, COL(255,255,255));
    // antennae: jointed, with shaded tips
    line_thick(hx-3, hy-r/3, hx-5, hy-r/3-3, 1, OUTLINE);
    line_thick(hx-5, hy-r/3-3, hx-7, hy-r/3-6, 1, OUTLINE);
    line_thick(hx+3, hy-r/3, hx+5, hy-r/3-3, 1, OUTLINE);
    line_thick(hx+5, hy-r/3-3, hx+7, hy-r/3-6, 1, OUTLINE);
    circle_fill(hx-7, hy-r/3-6, 2, dark); circle_fill(hx+7, hy-r/3-6, 2, dark);
    gloss(hx, hy, r/2+1);   // shiny little head
    // a determined little mouth
    line_thick(hx-3, hy+r/4+2, hx, hy+r/4, 1, dark);
    line_thick(hx, hy+r/4, hx+3, hy+r/4+2, 1, dark);
    // hp bar on big bugs — and on elites always (feat 8: the bar is
    // the hunt's progress, visible from the first sighting)
    if ((b->maxhp>=40 && b->hp<b->maxhp) || (b->elite && b->hp>0)){
        int wpx = r*2;
        rect_fill(x-r, y-r-10, wpx, 3, COL(40,30,40));
        rect_fill(x-r, y-r-10, (int)(wpx*b->hp/b->maxhp), 3, COL(120,230,90));
    }
    if (b->bp_t>0){ // breakpoint: paused, with the universal symbol
        circle_out(x+r-2, y-r-4, 7, COL(70,140,255));
        rect_fill(x+r-5, y-r-8, 2, 7, COL(255,255,255));
        rect_fill(x+r-1, y-r-8, 2, 7, COL(255,255,255));
    }
}
// Hat shape: the stack grows UP to HAT_ROWS rows — then it stops getting
// taller and starts getting WIDER, spreading sideways row by row until it
// is four carcasses across. A wide hat sways much more: properly unwieldy.
#define HAT_ROWS 14
#define HAT_VIS_MAX MAXSTACK
// the hat grows UP to HAT_ROWS first — then it stops being polite and
// spreads SIDEWAYS, wider and wider, rows creeping up only slowly.
// At 420 carcasses it is a 14-wide, 30-row monument.
