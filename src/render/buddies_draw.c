// THE SURVIVAL GAME — render/buddies_draw.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 1915-2203 in the Nov 2026 fine-grained refactor; code unchanged.

static const u32 HAIR_COL[MAXP] = { COL(96,64,40), COL(38,34,38), COL(158,116,62), COL(72,50,34) };
static const u32 SKIN = COL(245,205,160);
static int g_bud_monocle = 0;   // WHALES ONLY: this buddy's owner bought the monocle
static int g_bud_crown = 0;     // WHALES ONLY: ROYAL BUDDY
static int g_bud_space = 0;     // WHALES ONLY: SPACE BUDDY
// buddies wear the same comically big nerd glasses as their DEV
// (Daniel, Nov 2): pale sheen, chunky frames, wider than the face.
static void draw_bud_glasses(int cx, int cy, int lw, int lh){
    int gl = cx - lw - 1, gr = cx + 1, gy = cy - lh/2;
    blend_rect(gl, gy, lw, lh, COL(200,230,255), 60);
    blend_rect(gr, gy, lw, lh, COL(200,230,255), 60);
    for (int s2=0;s2<2;s2++){
        rect_line(gl-s2, gy-s2, lw+2*s2, lh+2*s2, OUTLINE);
        rect_line(gr-s2, gy-s2, lw+2*s2, lh+2*s2, OUTLINE);
    }
    rect_fill(gl+lw, cy-1, gr-(gl+lw), 2, OUTLINE);
    pset_blend(gl+2, gy+lh-2, COL(255,255,255), 170);
    pset_blend(gl+3, gy+lh-3, COL(255,255,255), 140);
    pset_blend(gr+2, gy+lh-2, COL(255,255,255), 170);
    pset_blend(gr+3, gy+lh-3, COL(255,255,255), 140);
    if (g_bud_space){   // WHALES ONLY: SPACE BUDDY — a bubble over the whole head
        int br = lw + 8;
        for (int a2=0;a2<28;a2++){
            float an = (float)a2 * (6.2831853f/28.0f);
            pset_blend(cx + (int)(fcos(an)*(float)br), cy - 2 + (int)(fsin(an)*(float)(br*3/4)), COL(200,235,255), 190);
        }
        line_thick(cx-br+4, cy-2-br/2, cx-br+9, cy-2-br/4, 1, COL(240,250,255));   // the shine
    }
    if (g_bud_crown){   // WHALES ONLY: ROYAL BUDDY — three gold points on a band
        int cw2 = lw + 4, cx0 = cx - cw2/2, cy0 = cy - lh - 5;
        rect_fill(cx0, cy0+4, cw2, 4, COL(232,184,60));
        tri_fill(cx0, cy0+4, cx0+5, cy0+4, cx0+2, cy0-3, COL(255,205,60));
        tri_fill(cx0+cw2/2-3, cy0+4, cx0+cw2/2+3, cy0+4, cx0+cw2/2, cy0-4, COL(255,205,60));
        tri_fill(cx0+cw2-5, cy0+4, cx0+cw2, cy0+4, cx0+cw2-2, cy0-3, COL(255,205,60));
        pset(cx0+cw2/2, cy0+5, COL(230,60,90));
    }
    if (g_bud_monocle){   // WHALES ONLY: THE FORBIDDEN MONOCLE, on a chain
        int mx2 = gr + lw/2, my2 = gy + lh/2;
        for (int a2=0;a2<20;a2++){
            float an = (float)a2 * (6.2831853f/20.0f);
            pset(mx2 + (int)(fcos(an)*(float)(lw/2+2)), my2 + (int)(fsin(an)*(float)(lh/2+2)), COL(232,184,60));
        }
        line_thick(mx2+lw/2, my2+lh/2, mx2+lw/2+3, gy+lh+7, 1, COL(232,184,60));
    }
}
static void draw_buddy(Buddy *b){
    int owner = (int)(b - G.bud);
    g_bud_monocle = (owner>=0 && owner<MAXP && (G.p[owner].whale_on & 16) && !b->fainted);
    g_bud_crown   = (owner>=0 && owner<MAXP && (G.p[owner].whale_on & 128) && !b->fainted);
    g_bud_space   = (owner>=0 && owner<MAXP && (G.p[owner].whale_on & 4096) && !b->fainted);
    int x=SX(b->x), y=SY(b->y);
    if (x<-50||x>VW+50||y<-60||y>VH+50) return;
    u32 collar = (owner>=0 && owner<MAXP)? HOODIE[G.p[owner].color&3] : COL(200,60,60);
    float fx = fcos(b->aim), fy = fsin(b->aim);
    int hx = (int)(fx*8), hy = (int)(fy*5);            // head leads toward facing
    int lx = (int)(fx * b->lunge_t * 34), ly = (int)(fy * b->lunge_t * 34);   // the lunge
    int flash = (b->hurt_t>0 && ((int)(G.anim_t*20))%2);
    shadow_ellipse(x, y+10, 12, 3);
    if (b->on_wall>=0) y-=8;          // up on top of an interlocked wall run, like his DEV
    if (b->fainted){
        // flat out, seeing stars — fainted, not gone. The Banzai
        // Buddies shop revives it; until then it naps right here.
        if (b->species==4){   // a fainted rock is a flat cracked slab
            rect_fill(x-13, y+2, 26, 5, COL(120,110,110));
            rect_fill(x-10, y-1, 19, 3, COL(120,110,110));
            rect_fill(x-5, y-3, 8, 2, COL(120,110,110));
            line_thick(x-7, y+1, x-1, y+5, 1, OUTLINE);
            line_thick(x-1, y+5, x+5, y+2, 1, OUTLINE);
            line_thick(x+2, y+2, x+4, y+7, 1, OUTLINE);
        } else {
            ellipse_fill(x, y+4, 13, 6, COL(120,110,110));
            circle_fill(x-8, y+2, 5, COL(120,110,110));
        }
        for (int s=0;s<3;s++){
            float a = G.anim_t*2.4f + s*(TAU/3);
            disc_fill(x + (int)(fcos(a)*10), y-8 + (int)(fsin(a)*3), 1, COL(255,230,120));
        }
        draw_text_center(x, y-18, "FAINTED", 1, COL(255,150,150));
        return;
    }
    int bob = (int)(fsin(G.anim_t*3.0f + owner*1.7f)*1.0f);          // breathing
    int step = (int)(fsin(b->walk_ph)*3);
    // THE WALK (Daniel, Nov 6): no wiggle. While actually moving,
    // the body F L O P S — a rigid square-wave lean locked to the
    // same phase that steps the feet: the head tips the full flop,
    // the body half of it, the shadow stays planted. Breathing
    // pauses mid-stride. The pet rock is exempt: it already flops
    // end over end on its own roll.
    int walking = (b->mv_t>0 && b->species!=4);
    int wflop = walking? ((fsin(b->walk_ph)>=0)? 3 : -3) : 0;
    if (walking){ bob = 0; hx += wflop; lx += wflop/2; }
    if (b->species==0){          // PUPPY: tan, floppy ears, eternal tail
        u32 body = flash? COL(255,255,255) : bud_coat(b, COL(199,152,94));
        u32 dark = col_shade(body, 0.7f);
        // tail: wagging is a full-time job
        float wag = fsin(G.anim_t*11.0f + owner)*0.6f;
        int tx2 = x - hx - (int)(fx*6), ty2 = y - hy + 2;
        line_thick(tx2, ty2, tx2 + (int)(-fx*7 - fy*wag*7), ty2 + (int)(-fy*7 + fx*wag*7) - 3, 2, dark);
        ellipse_shade(x+lx, y+ly+bob, 12, 9, body);
        for (int lg=0;lg<4;lg++){   // trotters
            int ox = (lg%2? 6:-6), oy = (lg<2? 4:8);
            disc_fill(x+lx+ox, y+ly+oy + ((lg+ (step>0?0:1))%2? 0:1), 2, dark);
        }
        rect_fill(x+lx-5, y+ly-3+bob, 10, 4, collar);                // collar band
        int hxx = x+lx+hx, hyy = y+ly+hy+bob;
        circle_fill(hxx, hyy, 8, OUTLINE); circle_shade(hxx, hyy, 7, body); gloss(hxx, hyy, 7);
        // floppy ears, mid-flop (at a walk they ride the head rigidly)
        int flop = walking? 0 : (int)(fsin(b->walk_ph*1.4f)*1.5f);
        ellipse_fill(hxx-6, hyy-3+flop, 3, 5, dark);
        ellipse_fill(hxx+6, hyy-3-flop, 3, 5, dark);
        circle_fill(hxx + (int)(fx*5), hyy + (int)(fy*3) + 2, 2, COL(60,40,36));   // snout nose
        circle_fill(hxx-3, hyy-2, 1, OUTLINE); circle_fill(hxx+3, hyy-2, 1, OUTLINE);
        draw_bud_glasses(hxx + (int)(fx*3), hyy-2, 8, 8);
        if (b->lunge_t>0){   // jaw snap
            line_thick(hxx + (int)(fx*6), hyy+2, hxx + (int)(fx*10), hyy+4, 1, OUTLINE);
            line_thick(hxx + (int)(fx*6), hyy+4, hxx + (int)(fx*10), hyy+2, 1, OUTLINE);
        }
    } else if (b->species==1){   // KITTY: orange, striped, unbothered
        u32 body = flash? COL(255,255,255) : bud_coat(b, COL(228,152,72));
        u32 dark = col_shade(body, 0.72f);
        // tail: a slow question mark
        float curl = fsin(G.anim_t*2.0f + owner*2.0f)*3.0f;
        int tx2 = x - hx - (int)(fx*7), ty2 = y - hy + 3;
        line_thick(tx2, ty2, tx2 - (int)(fx*6), ty2 - 5, 2, dark);
        line_thick(tx2 - (int)(fx*6), ty2 - 5, tx2 - (int)(fx*6) + (int)curl, ty2 - 10, 2, dark);
        ellipse_shade(x+lx, y+ly+bob, 11, 8, body);
        line_thick(x+lx-6, y+ly-4+bob, x+lx-4, y+ly+5+bob, 1, dark);   // tabby stripes
        line_thick(x+lx+1, y+ly-5+bob, x+lx+3, y+ly+5+bob, 1, dark);
        for (int lg=0;lg<4;lg++){
            int ox = (lg%2? 5:-5), oy = (lg<2? 3:7);
            disc_fill(x+lx+ox, y+ly+oy + ((lg + (step>0?0:1))%2? 0:1), 2, dark);
        }
        rect_fill(x+lx-5, y+ly-3+bob, 10, 4, collar);
        int hxx = x+lx+hx, hyy = y+ly+hy+bob;
        circle_fill(hxx, hyy, 7, OUTLINE); circle_shade(hxx, hyy, 6, body); gloss(hxx, hyy, 6);
        tri_fill(hxx-6, hyy-4, hxx-4, hyy-10, hxx-1, hyy-5, dark);     // ears: triangles, obviously
        tri_fill(hxx+6, hyy-4, hxx+4, hyy-10, hxx+1, hyy-5, dark);
        circle_fill(hxx + (int)(fx*4), hyy + (int)(fy*2) + 1, 1, COL(200,90,90));
        circle_fill(hxx-2, hyy-1, 1, OUTLINE); circle_fill(hxx+2, hyy-1, 1, OUTLINE);
        draw_bud_glasses(hxx + (int)(fx*2), hyy-1, 7, 7);
        line_thick(hxx-8, hyy+1, hxx-4, hyy+1, 1, dark);               // whiskers
        line_thick(hxx+4, hyy+1, hxx+8, hyy+1, 1, dark);
        if (b->lunge_t>0){   // paw swipe, alternating
            int sw = ((int)(b->walk_ph*2))%2? 1 : -1;
            int sxp = hxx + (int)(fx*8) - (int)(fy*sw*5), syp = hyy + (int)(fy*8) + (int)(fx*sw*5);
            line_thick(sxp-3, syp-3, sxp+3, syp+3, 1, COL(255,255,255));
            line_thick(sxp-3, syp+1, sxp+3, syp-3, 1, COL(255,255,255));
        }
    } else if (b->species==2){   // MOOSE: a walking wall with antlers
        u32 body = flash? COL(255,255,255) : bud_coat(b, COL(143,106,68));
        u32 dark = col_shade(body, 0.7f);
        int sway = walking? 0 : (int)(fsin(G.anim_t*1.1f + owner)*1.2f);   // the great sway (idle only — walking is the flop)
        ellipse_shade(x+lx, y+ly+bob, 15, 11, body);
        for (int lg=0;lg<4;lg++){   // sturdy legs
            int ox = (lg%2? 8:-8), oy = (lg<2? 5:10);
            rect_fill(x+lx+ox-2, y+ly+oy-2, 4, 5, dark);
        }
        rect_fill(x+lx-6, y+ly-4+bob, 12, 4, collar);
        int hxx = x+lx+hx+sway, hyy = y+ly+hy-2+bob;
        ellipse_fill(hxx, hyy, 9, 7, OUTLINE); ellipse_shade(hxx, hyy, 8, 6, body);
        ellipse_fill(hxx + (int)(fx*6), hyy + (int)(fy*3) + 2, 5, 3, dark);    // the great snout
        circle_fill(hxx-2, hyy-1, 1, OUTLINE); circle_fill(hxx+3, hyy-1, 1, OUTLINE);
        draw_bud_glasses(hxx + (int)(fx*3), hyy-1, 9, 7);
        // antlers: two racks of pure punctuation
        u32 ant = COL(226,204,160);
        for (int s=-1;s<=1;s+=2){
            int ax = hxx + s*7, ay = hyy - 5;
            line_thick(ax, ay, ax + s*5, ay - 6, 2, ant);
            line_thick(ax + s*2, ay - 3, ax + s*2, ay - 8, 1, ant);
            line_thick(ax + s*4, ay - 5, ax + s*7, ay - 9, 1, ant);
        }
        if (b->lunge_t>0){   // the head swing behind every swipe
            int ax = hxx + (int)(fx*8), ay = hyy + (int)(fy*6);
            line_thick(ax-6, ay, ax+6, ay, 2, ant);
        }
    }
    else if (b->species==3){   // PLATYPUS: brown, billed, gloriously odd
        u32 body = flash? COL(255,255,255) : bud_coat(b, COL(128,92,58));
        u32 dark = col_shade(body, 0.7f);
        int sway = walking? 0 : (int)(fsin(G.anim_t*4.0f + owner*1.3f)*1.0f);   // a swimmy wiggle (idle only — walking is the flop)
        // the flat paddle tail, behind everything
        int tx2 = x - hx - (int)(fx*9), ty2 = y - hy + 3;
        ellipse_fill(tx2, ty2, 8, 4, dark); ellipse_fill(tx2, ty2, 7, 3, body);
        ellipse_shade(x+lx, y+ly+bob+sway/2, 12, 8, body);
        for (int lg=0;lg<4;lg++){   // webbed feet, roughly
            int ox = (lg%2? 6:-6), oy = (lg<2? 4:8);
            disc_fill(x+lx+ox, y+ly+oy + ((lg + (step>0?0:1))%2? 0:1), 2, dark);
        }
        rect_fill(x+lx-5, y+ly-3+bob, 10, 4, collar);
        int hxx = x+lx+hx, hyy = y+ly+hy+bob;
        circle_fill(hxx, hyy, 7, OUTLINE); circle_shade(hxx, hyy, 6, body); gloss(hxx, hyy, 6);
        // THE bill: wide, flat, faintly smug
        int bx2 = hxx + (int)(fx*7), by2 = hyy + (int)(fy*3) + 2;
        ellipse_fill(bx2, by2, 7, 3, OUTLINE); ellipse_fill(bx2, by2, 6, 2, COL(216,186,128));
        circle_fill(hxx-3, hyy-2, 1, OUTLINE); circle_fill(hxx+3, hyy-2, 1, OUTLINE);
        draw_bud_glasses(hxx + (int)(fx*2), hyy-2, 8, 7);
        if (b->lunge_t>0){   // the spur flashes
            int sxp = x+lx - (int)(fx*10), syp = y+ly + 6;
            line_thick(sxp, syp, sxp - (int)(fx*5), syp+3, 1, COL(190,255,200));
        }
    } else if (b->species==4){   // PET ROCK: a flat-bottomed crag
        u32 body = flash? COL(255,255,255) : bud_coat(b, COL(148,144,138));
        u32 dark = col_shade(body, 0.7f);
        // Daniel's spec (Nov 2): flat bottom, jagged hilly top, and
        // it FLOPS over as it travels — the slab tips end over end,
        // its peaks shearing across the base, with a little hop at
        // each flop. tip runs -1..1 over the roll cycle.
        float ph = b->roll; ph -= (int)(ph/6.28318f)*6.28318f; if (ph<0) ph+=6.28318f;
        float tip = fsin(ph);
        int lift = -(int)((tip>0? tip : -tip)*3.0f);
        int cx = x+lx + (int)(tip*2.0f);
        int base = y+ly+bob+lift + 9;                 // the flat bottom line
        int shear = (int)(tip*5.0f);                  // top leans into the flop
        // the silhouette in horizontal slices, bottom to top: full
        // width at the flat base, stepping in jaggedly to two humps
        static const int PROF[9] = { 15, 15, 14, 13, 12, 12, 9, 8, 5 };
        for (int r2=0;r2<9;r2++){
            int hw = PROF[r2];
            int off = (shear * r2) / 8;               // upper slices lean most
            int yy = base - r2*2;
            rect_fill(cx - hw + off, yy-1, hw*2, 2, r2==0? dark : body);
            if (r2==6) rect_fill(cx - hw + off - 3, yy-1, 4, 2, body);   // the left shoulder juts
            if (r2==8) rect_fill(cx + off + 1, yy-3, 5, 2, body);        // the tall peak
        }
        // speckles riding the slab as it flops
        for (int s2=0;s2<5;s2++){
            float a = b->roll + s2*2.4f + (float)(s2%3)*0.7f;
            int sx2 = cx + (int)(fcos(a)*9.0f) + (shear * (s2%3)) / 2;
            int sy2 = base - 3 - (s2%4)*3;
            disc_fill(sx2, sy2, 1, dark);
        }
        // googly eyes up on the ridge, glasses over them, both
        // shearing with the flop
        int ey = base - 15 + (int)(tip*1.0f), exx = (int)(fx*2), eoff = (shear * 7) / 8;
        for (int s2=-1;s2<=1;s2+=2){
            circle_fill(cx+eoff+s2*5, ey, 3, OUTLINE);
            circle_fill(cx+eoff+s2*5, ey, 2, COL(246,246,246));
            disc_fill(cx+eoff+s2*5+exx, ey + (int)(fy*1), 1, OUTLINE);
        }
        draw_bud_glasses(cx+eoff, ey, 9, 8);
        line_thick(cx-2, base-3, cx+2, base-3, 1, dark);   // a determined mouth, low on the slab
        if (b->steady_t>0) circle_out(cx, base-8, 19, COL(255,214,64));   // ROCK STEADY ring
        if (b->lunge_t>0) circle_out(cx, base-8, 17, COL(230,226,218));
    } else if (b->species==5){   // KITCHEN SINK: porcelain, plumbed, present
        u32 body = flash? COL(255,255,255) : bud_coat(b, COL(238,240,242));
        u32 dark = col_shade(body, 0.8f);
        u32 chrome = COL(168,174,182);
        int hop = walking? 0 : (int)(fsin(b->walk_ph*1.4f)*1.2f);   // no hopping at a walk — the whole sink tips instead
        // stubby pipe legs
        rect_fill(x+lx-9, y+ly+8, 4, 5, chrome); rect_fill(x+lx+5, y+ly+8, 4, 5, chrome);
        // the basin
        rect_fill(x+lx-14, y+ly-1+bob+hop, 28, 11, body); rect_line(x+lx-14, y+ly-1+bob+hop, 28, 11, dark);
        ellipse_fill(x+lx, y+ly-2+bob+hop, 14, 5, body); ellipse_fill(x+lx, y+ly-2+bob+hop, 13, 4, dark);
        // water, sloshing as it goes
        int slosh = walking? wflop : (int)(fsin(b->walk_ph*1.6f)*2.0f);   // at a walk the water tips rigidly with the basin
        ellipse_fill(x+lx+slosh, y+ly-2+bob+hop, 10, 3, COL(126,192,242));
        disc_fill(x+lx+slosh+3, y+ly-3+bob+hop, 1, COL(220,245,255));
        // backsplash with twin faucets and cross handles
        rect_fill(x+lx-11, y+ly-11+bob+hop, 22, 6, body); rect_line(x+lx-11, y+ly-11+bob+hop, 22, 6, dark);
        for (int s2=-1;s2<=1;s2+=2){
            rect_fill(x+lx+s2*6-1, y+ly-14+bob+hop, 2, 4, chrome);
            line_thick(x+lx+s2*6-3, y+ly-14+bob+hop, x+lx+s2*6+3, y+ly-14+bob+hop, 1, chrome);
            line_thick(x+lx+s2*6, y+ly-16+bob+hop, x+lx+s2*6, y+ly-12+bob+hop, 1, chrome);
        }
        // centre spout + the occasional drip of destiny
        rect_fill(x+lx-1, y+ly-13+bob+hop, 2, 3, chrome);
        if (((int)(G.anim_t*0.9f + owner))%3==0) disc_fill(x+lx, y+ly-6+bob+hop, 1, COL(126,192,242));
        // a face on the porcelain front, drain for a nose
        circle_fill(x+lx-5, y+ly+3+bob+hop, 1, OUTLINE); circle_fill(x+lx+5, y+ly+3+bob+hop, 1, OUTLINE);
        circle_fill(x+lx, y+ly+5+bob+hop, 2, dark);
        draw_bud_glasses(x+lx, y+ly+3+bob+hop, 8, 7);
        if (b->lunge_t>0){   // it splashes toward the fight
            for (int s2=0;s2<3;s2++)
                disc_fill(x+lx + (int)(fx*14) + s2*2 - 2, y+ly + (int)(fy*10) + (s2%2)*3 - 1, 1, COL(126,192,242));
        }
    }
    draw_text_center(x, y+16, BUD_ELF[b->name_idx%BUD_NAMES_N], 1, COL(255,244,210));
    if (b->hp < b->maxhp){
        float frac = b->maxhp>0? b->hp/b->maxhp : 0; if (frac<0) frac=0;
        rect_fill(x-12, y-24, 24, 3, COL(40,30,40));
        rect_fill(x-12, y-24, (int)(24*frac), 3, frac>0.35f? COL(120,230,90) : COL(255,170,60));
    }
    if (b->energy < BUD_ENERGY_MAX - 0.5f){   // the blue thread under the health
        float ef = b->energy/BUD_ENERGY_MAX; if (ef<0) ef=0;
        rect_fill(x-12, y-20, 24, 2, COL(40,30,40));
        rect_fill(x-12, y-20, (int)(24*ef), 2, COL(120,200,255));
    }
}

