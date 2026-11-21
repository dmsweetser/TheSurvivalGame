// THE SURVIVAL GAME — render/hats.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 1734-1914 in the Nov 2026 fine-grained refactor; code unchanged.

static void hat_shape_c(int n, int cap, int rowcap, int *rows, int *max_across){
    int vis = n>cap? cap : n;
    if (vis<=0){ if (rows) *rows=0; if (max_across) *max_across=0; return; }
    int r = vis<=HAT_ROWS? vis : HAT_ROWS + (vis-HAT_ROWS)/24;
    if (r>rowcap) r=rowcap;
    int base = vis/r, extra = vis%r;
    if (rows) *rows = r;
    if (max_across) *max_across = base + (extra? 1:0);
}
static void hat_shape(int n, int *rows, int *max_across){ hat_shape_c(n, HAT_VIS_MAX, 30, rows, max_across); }
static int hat_across_c(int n, int row, int cap, int rowcap){
    // carcasses across on a given row (row 0 = bottom); bottom rows widen first
    int vis = n>cap? cap : n;
    if (vis<=0) return 0;
    int r = vis<=HAT_ROWS? vis : HAT_ROWS + (vis-HAT_ROWS)/24;
    if (r>rowcap) r=rowcap;
    if (row>=r) return 0;
    int base = vis/r, extra = vis%r;
    return base + (row<extra? 1:0);
}
static int hat_across(int n, int row){ return hat_across_c(n, row, HAT_VIS_MAX, 30); }
// HAT TOPPERS (v1.25 feat 5): derive the earned set from the same
// records the Trophy Case reads, so a plaque and its topper can
// never disagree. Returns the newly-earned mask. The BOWL PIN is
// the one non-derivable topper (the lanes keep only a house best,
// so nobody can be credited after the fact); mall.c awards it at
// the bowl finalize site. Everything else derives here.
static int topper_derive(Buddy *b, Player *p){
    int m = 0;
    if (b->cas_best > 0) m |= TOP_BIT(TOP_CROWN);                    // escaped CASTLE once
    if (b->war_prog >= 8) m |= TOP_BIT(TOP_CAP);                     // WAR! campaign conquered
    { int home = b->crt_cupbest % 8, far = b->crt_cupbest / 8;       // a Grand Prix cup won
      if (home==1 || far==1) m |= TOP_BIT(TOP_FLAG); }
    if (b->story_bits & (1<<17)) m |= TOP_BIT(TOP_MEDAL);            // ZERO-DAY ZED defeated
    if (crt_medal_count(b,3) + plat_medal_count(b,3) > 0) m |= TOP_BIT(TOP_GOLD);   // any gold medal
    if (!G.zen_run && p->heaviest >= TOP_ANVIL_KG) m |= TOP_BIT(TOP_ANVIL);         // carried a Legacy-System hat (records rule: no Zen)
    if (G.fs_state==2) m |= TOP_BIT(TOP_HARDHAT);                    // FIRST SHIFT complete (slot-level, like the case's row)
    return m;
}
static int topper_refresh(int pi, int announce){
    Player *p=&G.p[pi]; Buddy *b=&G.bud[pi];
    if (!b->active){ p->top_worn = 0; return 0; }
    int before = b->top_earned;
    b->top_earned |= topper_derive(b, p);
    int fresh = b->top_earned & ~before;
    if (b->top_worn<1 || b->top_worn>TOP_N || !(b->top_earned & TOP_BIT(b->top_worn))) b->top_worn = 0;
    p->top_worn = b->top_worn;   // the render mirror draw_hat reads
    if (fresh && announce && (pi==G.my_id || pi==G.p2_slot)){
        int slot = 0;
        for (int id=1; id<=TOP_N; id++) if (fresh & TOP_BIT(id)){
            char buf[24]; char *q=buf; q=sappend(q, TOP_NAME[id]); q=sappend(q, " EARNED!"); *q=0;
            spawn_floater(p->x, p->y-46-(slot++)*10, buf, COL(255,205,40));
        }
        sfx(11);
    }
    return fresh;
}
// one topper, drawn upward from (cx, base): chunky pixel art in the
// game's palette language, ~16 px wide at reading zoom. Eddie Mode
// restyles the stack UNDER it; the trophy itself never changes.
static void draw_topper(int id, int cx, int base, float z){
    int u = (z>=0.9f)? 2 : 1;
    switch (id){
    case TOP_CROWN:   // gold band, three points, red gem
        rect_fill(cx-4*u, base-2*u, 8*u, 2*u, COL(255,205,40));
        rect_fill(cx-4*u, base-2*u, 8*u, u, COL(255,225,110));
        tri_fill(cx-4*u, base-2*u, cx-3*u, base-5*u, cx-2*u, base-2*u, COL(255,205,40));
        tri_fill(cx-u, base-2*u, cx, base-6*u, cx+u, base-2*u, COL(255,205,40));
        tri_fill(cx+2*u, base-2*u, cx+3*u, base-5*u, cx+4*u, base-2*u, COL(255,205,40));
        circle_fill(cx, base-u, u, COL(230,60,70));
        break;
    case TOP_CAP:     // a general's peaked cap: olive dome, gold band, black brim
        ellipse_fill(cx, base-2*u, 4*u, 2*u, COL(88,96,52));
        rect_fill(cx-4*u, base-2*u, 8*u, u, COL(255,205,40));
        rect_fill(cx-4*u, base-u, 8*u, u, COL(30,30,34));
        circle_fill(cx, base-3*u, u, COL(255,205,40));
        break;
    case TOP_FLAG:    // a pole with a waving checker flag
        rect_fill(cx-u/2, base-7*u, u, 7*u, COL(140,140,150));
        for (int fx=0; fx<3; fx++) for (int fy=0; fy<2; fy++){
            u32 c = ((fx+fy)&1)? COL(30,30,34) : COL(240,240,245);
            rect_fill(cx + u/2 + fx*2*u - (fy? u/2 : 0), base-7*u + fy*2*u, 2*u, 2*u, c);
        }
        break;
    case TOP_MEDAL:   // a medal: ribbon V over a gold disc
        line_thick(cx-2*u, base-6*u, cx, base-2*u, u, COL(200,50,60));
        line_thick(cx+2*u, base-6*u, cx, base-2*u, u, COL(60,90,200));
        circle_fill(cx, base-2*u, 2*u, COL(255,205,40));
        circle_fill(cx, base-2*u, u, COL(255,225,110));
        break;
    case TOP_PIN:     // a bowling pin: white silhouette, red neck stripe
        circle_fill(cx, base-2*u, 2*u, COL(245,245,248));
        rect_fill(cx-u, base-5*u, 2*u, 3*u, COL(245,245,248));
        circle_fill(cx, base-5*u, u, COL(245,245,248));
        rect_fill(cx-u, base-4*u, 2*u, u, COL(220,50,60));
        break;
    case TOP_GOLD:    // a gold star: two triangles and a bright core
        tri_fill(cx-3*u, base-u, cx, base-6*u, cx+3*u, base-u, COL(255,205,40));
        tri_fill(cx-3*u, base-4*u, cx, base+u, cx+3*u, base-4*u, COL(255,205,40));
        circle_fill(cx, base-2*u, u, COL(255,225,110));
        break;
    case TOP_ANVIL:   // dark iron: horn, waist, base, one highlight
        rect_fill(cx-4*u, base-5*u, 7*u, 2*u, COL(105,105,120));
        tri_fill(cx+3*u, base-5*u, cx+5*u, base-4*u, cx+3*u, base-3*u, COL(105,105,120));
        rect_fill(cx-u, base-3*u, 2*u, u, COL(85,85,100));
        rect_fill(cx-3*u, base-2*u, 6*u, 2*u, COL(95,95,110));
        rect_fill(cx-4*u, base-5*u, 3*u, u, COL(170,170,185));
        break;
    case TOP_HARDHAT: // a builder's helmet: yellow dome, wide brim
        ellipse_fill(cx, base-u, 4*u, 3*u, COL(255,190,40));
        rect_fill(cx-5*u, base-u, 10*u, u, COL(240,170,30));
        rect_fill(cx-u, base-4*u, 2*u, u, COL(255,205,70));
        break;
    }
}
static void draw_hat(Player *p, int x, int y){
    // the carcass stack on the player's head, growing through tier shapes
    int n = p->stack_n;
    int worn = (p->top_worn>=1 && p->top_worn<=TOP_N)? p->top_worn : 0;   // HAT TOPPERS (feat 5)
    if (n<=0){
        // a worn topper sits on the bare head: banking the whole
        // hat must not un-wear the trophy
        if (worn) draw_topper(worn, x, y-13, G.zoom>0.01f? G.zoom : 1.0f);
        return;
    }
    g_gold_carcass = (p->whale_on & 1) != 0;   // Whales Only: golden hat
    // (The Psychic Hat Stand that once floated the pile beside its
    // owner — bonus height and all — was removed with the Gallery
    // on Nov 2. The hat is worn, and shows what you carry.)
    int vis_n = n;
    int cap = HAT_VIS_MAX;
    int rowcap = 30;
    int hx2 = x, hy2 = y;
    if (0){
        ellipse_fill(hx2, hy2+1, 10, 3, COL(190,150,255));
        ellipse_fill(hx2, hy2, 7, 2, COL(230,210,255));
    }
    int rows, maxa; hat_shape_c(vis_n, cap, rowcap, &rows, &maxa);
    int wide = maxa>1;
    float sway_amp = 0.6f + (wide? (maxa-1)*0.55f : 0.0f);   // width = wobble
    // the hat lives in the world, so it shrinks with the camera zoom
    float z = G.zoom>0.01f? G.zoom : 1.0f;
    int stepx = (int)(10*z+0.5f); if (stepx<5) stepx=5;
    int stepy = (int)(7*z+0.5f);  if (stepy<4) stepy=4;
    int cs = (int)(6*z+0.5f);     if (cs<3) cs=3; if (maxa>10 && cs>5) cs=5;
    int idx = 0;
    int top_sway = 0;
    for (int i=0;i<rows;i++){
        int a = hat_across_c(vis_n, i, cap, rowcap);
        int cy = hy2 - 16 - i*stepy;
        // each row sways a little more than the one below; a wide stack
        // also leans slowly side to side like it's about to go over
        float sway = fsin(G.anim_t*2.2f + p->walk_ph + i*0.35f) * (sway_amp + i*0.30f);
        float lean = wide? fsin(G.anim_t*0.8f + p->walk_ph*0.5f) * (maxa-1) * ((float)i/rows) * 1.6f : 0.0f;
        int rowx = hx2 + (int)(sway + lean);
        if (i==rows-1) top_sway = (int)(sway+lean);
        for (int k=0;k<a;k++){
            int off = (int)((k - (a-1)*0.5f)*stepx);
            draw_carcass(rowx + off + ((idx%2)? 1 : -1), cy, cs,
                         (idx < p->stack_n? (int)(p->stack[idx]) : idx*7)%4, fsin(idx*2.3f));
            idx++;
        }
    }
    int top_y = hy2 - 16 - rows*stepy;
    if (vis_n>=19 && !worn){ // CROWN: a golden bug reigns (a worn topper takes the perch instead)
        draw_carcass(hx2+top_sway, top_y-4, 7, 1, 0);
        tri_fill(hx2-6+top_sway, top_y-12, hx2+top_sway, top_y-18, hx2+6+top_sway, top_y-12, COL(255,205,40));
    }
    if (vis_n>=36){ // LEGACY SYSTEM: scaffolding + warning light, sized to the stack
        int hw = maxa*stepx/2 + 8;
        line_thick(hx2-hw, hy2-14, hx2-hw, top_y, 1, COL(120,120,130));
        line_thick(hx2+hw, hy2-14, hx2+hw, top_y, 1, COL(120,120,130));
        if (((int)(G.anim_t*2))%2) circle_fill(hx2+top_sway, top_y-14 - (worn? 16:0), 3, COL(255,60,60));
    }
    if (worn) draw_topper(worn, hx2+top_sway, top_y+2, z);   // HAT TOPPERS: the trophy rides the sway
    if (n>=4){ // weight label (the REAL weight, stand or no stand)
        char b[20]; fmt_int(b, "", (int)p->carried, "KG");
        draw_text_center(hx2, top_y - (vis_n>=19? 26:12) - (worn? 15:0), b, 1, COL(255,240,180));
    }
    g_gold_carcass = 0;
}
