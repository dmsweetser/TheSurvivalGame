// THE SURVIVAL GAME — modes/run/run_levels.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from run.c lines 1-163 in the Nov 2026 fine-grained refactor; code unchanged.

// ===========================================================================
// SUPER BUDDY RUN (Banzai World mode 3, Nov 5 — Daniel asked for a
// side-scrolling platformer in the game's own clothes): run right,
// stomp bugs, bonk mystery crates, kick shells, grab bits, and plant
// the DEPLOY flag. Levels are generated per (seed, level); finishing
// one unlocks the next for good (the Buddy's pd record).
// ===========================================================================
#define PL_TILE 24
#define PL_ROWS 15
#define PL_MAXW 240
#define PL_GROUND 13
// MEDAL TIMES (v1.25 feat 3): {gold, silver, bronze} thresholds in
// deciseconds for levels 1..16, DERIVED from the runner's own physics
// (spec: v125-feat3-spec.md): level distance D = (W-5.5)*24 px with
// W = min(110+14*lvl, 236); tier target average speeds gold ~109,
// silver ~94, bronze ~77 u/s (top speed is 124, walk 100), tapering
// ~0.5 u/s per level for the critter-speed creep. Medals derive from
// the stored per-level best (Buddy.plat_tbest) — retune here only.
// LIVE TUNING (v1.26 Item 3): mutable — data/livedata.c holds a tentative
// declaration and the registry points here; literals are stock defaults.
static int plat_medal[16][3] = {
    {261,303,369}, {293,340,415}, {326,378,461}, {358,416,508},
    {391,454,555}, {425,493,603}, {458,532,651}, {493,572,700},
    {527,612,750}, {529,615,754}, {532,618,758}, {534,621,762},
    {537,624,766}, {540,628,770}, {542,631,775}, {545,634,779},
};
// TROPHY CASE (v1.25 feat 4): how many of this buddy's levels 1..16
// derive to a given medal tier (0 = a best exists but earned no
// medal). Twin of cart.c's crt_medal_count — the case counts through
// here so PLAT_MEDAL keeps exactly one reader per piece.
static int plat_medal_count(Buddy *b, int tier){
    int n=0;
    for (int t=0;t<16;t++) if (medal_tier(b->plat_tbest[t], plat_medal[t])==tier && (tier>0 || b->plat_tbest[t]>0)) n++;
    return n;
}
// append a decisecond time as m:ss.d (mirrors cart.c's helper)
static char *sapp_time_ds(char *q, int tds){
    int sec = (tds / 10) % 60;
    q = sapp_int(q, tds / 600); q = sappend(q, ":");
    if (sec < 10) q = sappend(q, "0");
    q = sapp_int(q, sec); q = sappend(q, "."); q = sapp_int(q, tds % 10);
    return q;
}
// one sky per region — the run wears the country it is in
static const u32 PLAT_SKY[NREGIONS][3] = {
    { COL(104,178,238), COL(122,190,242), COL(140,200,245) },   // Clean Code Meadows
    { COL(116,182,226), COL(136,194,232), COL(156,206,238) },   // Spaghetti Fields
    { COL(128,162,202), COL(146,176,212), COL(164,190,220) },   // Legacy Lands (dusty)
    { COL(198,152,102), COL(210,168,118), COL(222,184,136) },   // Kernel Badlands (haze)
    { COL(172,206,230), COL(190,218,238), COL(206,230,245) },   // Frozen Cache Fields
    { COL(38,34,90),    COL(52,46,114),   COL(70,62,138) },     // Neon RAM Flats (dusk)
    { COL(86,152,132),  COL(102,168,146), COL(120,184,160) },   // Cable Jungle
    { COL(94,90,114),   COL(108,104,128), COL(122,118,142) },   // The Unmaintained Depths
};
static int plat_tile(BwSim *S, int tx, int ty){
    if (tx<0 || ty<0 || tx>=S->plat_w || ty>=PL_ROWS) return 0;
    return S->plat_grid[ty*PL_MAXW + tx];
}
static void plat_settile(BwSim *S, int tx, int ty, int v){
    if (tx>=0 && ty>=0 && tx<S->plat_w && ty<PL_ROWS) S->plat_grid[ty*PL_MAXW + tx] = (unsigned char)v;
}
static int plat_solidt(int t){ return t==1 || t==2 || t==3 || t==4 || t==5 || t==6 || t==7 || t==9; }
static int plat_solid_px(BwSim *S, float px, float py){
    return plat_solidt(plat_tile(S, (int)(px/PL_TILE), (int)(py/PL_TILE)));
}
// the buddy's collision box: 16 wide, 22 tall, origin at the feet
static int plat_box_hit(BwSim *S, float cx, float feet){
    return plat_solid_px(S, cx-8, feet-21) || plat_solid_px(S, cx+7, feet-21)
        || plat_solid_px(S, cx-8, feet-11) || plat_solid_px(S, cx+7, feet-11)
        || plat_solid_px(S, cx-8, feet-1)  || plat_solid_px(S, cx+7, feet-1);
}
static void plat_puff(BwSim *S, float x, float y){
    for (int i=0;i<10;i++) if (!S->plat_puff_a[i]){
        S->plat_puff_a[i]=1; S->plat_puff_x[i]=x; S->plat_puff_y[i]=y; S->plat_puff_t[i]=0.4f; return;
    }
}
static void plat_en_add(BwSim *S, int kind, float x, float y){
    if (S->plat_en_n>=24) return;
    PlatEn *e=&S->plat_en[S->plat_en_n++];
    e->x=x; e->y=y; e->vx=0; e->vy=0; e->kind=kind; e->state=0; e->t=0;
    if (kind==0) e->vx=-38; if (kind==1) e->vx=-30; if (kind==5) e->vx=44;
    // regional pressure: the further country, the quicker its
    // critters — the same scaling the walk out from the epicenter
    // applies in the main game, plus a lap bonus per full tour
    if (kind==0 || kind==1 || kind==4)
        e->vx *= 1.0f + 0.05f*(float)S->plat_region + 0.06f*(float)((S->plat_lvl-1)/8);
}
static void plat_load_level(int pi, int lvl){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    for (int i=0;i<PL_MAXW*PL_ROWS;i++) S->plat_grid[i]=0;
    S->plat_lvl=lvl; S->plat_en_n=0; S->plat_cleart=0; S->plat_deadt=0; S->plat_grace=0;
    S->plat_region = (lvl-1) % NREGIONS; S->plat_sub = 0; S->plat_vault_tx = -1; S->plat_chain = 0;
    for (int i=0;i<8;i++) S->plat_fa[i]=0;
    int W = 110 + lvl*14; if (W>PL_MAXW-4) W=PL_MAXW-4; S->plat_w=W;
    u32 seed = (u32)b->bw_seed ^ (0x9E37u + (u32)lvl*7919u);
    for (int x=0;x<W;x++){ plat_settile(S, x, 13, 1); plat_settile(S, x, 14, 1); }
    int x=10, cpdone=0;
    while (x < W-18){
        u32 h = hash2(x, lvl*3+1, seed);
        int seg = (int)(hash2(x*3+1, lvl*7, seed>>3) % 6);
        if (!cpdone && x > W/2){ cpdone=1; S->plat_cpx = x*PL_TILE; x += 6; continue; }
        if (seg==0){            // a gap, sometimes with a floating platform
            int gw = 2 + (int)(h%3);
            for (int g2=0;g2<gw;g2++){ plat_settile(S, x+g2, 13, 0); plat_settile(S, x+g2, 14, 0); }
            if (h&8){ for (int p2=0;p2<3;p2++){ plat_settile(S, x+p2, 8, 5); plat_settile(S, x+p2, 7, 10); } }
            x += gw + 2;
        } else if (seg==1){     // a server pipe
            int ph = 2 + (int)(h%3);
            for (int r=0;r<ph;r++) for (int c2=0;c2<2;c2++) plat_settile(S, x+c2, 12-r, r==ph-1? 7 : 6);
            if (lvl>=2 && (h&16)) plat_en_add(S, 3, (x+1)*PL_TILE, (13-ph)*PL_TILE);
            if (ph>=4) S->plat_vault_tx = x;   // the tallest pipe is the vault pipe (the last one wins)
            x += 4;
        } else if (seg==2){     // a crate row: bricks and mystery crates
            int n = 3 + (int)(h%3);
            for (int i2=0;i2<n;i2++) plat_settile(S, x+i2, 9, (i2==n/2 || (h>>i2)&1)? 3 : 2);
            if (h&32) for (int i2=0;i2<n;i2++) plat_settile(S, x+i2, 8, 10);
            x += n + 3;
        } else if (seg==3){     // stairs up (and maybe down)
            int sh = 2 + (int)(h%3);
            for (int s2=0;s2<sh;s2++) for (int r=0;r<=s2;r++) plat_settile(S, x+s2, 12-r, 5);
            x += sh + 2;
            if (h&64){ for (int s2=0;s2<sh;s2++) for (int r=0;r<sh-s2;r++) plat_settile(S, x+s2, 12-r, 5); x += sh + 1; }
        } else if (seg==4){     // an enemy stretch, bits arcing over it
            int ne = 1 + (int)(h%3);
            for (int e2=0;e2<ne;e2++){
                int kind = 0;
                u32 eh = hash2(x+e2*7, lvl, seed^0xE11u);
                if (lvl>=3 && eh%5==0) kind=4; else if (lvl>=2 && eh%3==0) kind=1;
                plat_en_add(S, kind, (x+2+e2*3)*PL_TILE, kind==4? 9*PL_TILE : PL_GROUND*PL_TILE);
            }
            for (int b2=0;b2<4;b2++) plat_settile(S, x+1+b2, 11, 10);
            x += 6 + ne*2;
        } else {                // platforms with bits
            for (int p2=0;p2<3;p2++){ plat_settile(S, x+1+p2, 9, 5); plat_settile(S, x+1+p2, 8, 10); }
            if (h&128) for (int p2=0;p2<3;p2++){ plat_settile(S, x+3+p2, 5, 5); plat_settile(S, x+3+p2, 4, 10); }
            x += 7;
        }
    }
    // a clean runway: nothing solid overhead and no critters for the
    // first nine tiles (the runner could spawn inside scenery —
    // Daniel), and the same courtesy around the checkpoint post,
    // whose ground the pit-respawn trusts
    for (int cx2=0;cx2<9;cx2++) for (int ry=0;ry<PL_GROUND;ry++) plat_settile(S, cx2, ry, 0);
    { int ct = (int)(S->plat_cpx/PL_TILE);
      for (int cx2=ct-1;cx2<=ct+2;cx2++) for (int ry=0;ry<PL_GROUND;ry++) plat_settile(S, cx2, ry, 0); }
    { int ct = S->plat_w/2;   // the runtime checkpoint target gets the same courtesy
      for (int cx2=ct-1;cx2<=ct+2;cx2++) for (int ry=0;ry<PL_GROUND;ry++) plat_settile(S, cx2, ry, 0); }
    for (int e2=0;e2<S->plat_en_n;e2++) if (S->plat_en[e2].x < 9*PL_TILE) S->plat_en[e2].kind=-1;
    // the run home: a tall staircase, a hop, and the DEPLOY flag
    int sx = W-13;
    for (int s2=0;s2<8;s2++) for (int r=0;r<=s2;r++) plat_settile(S, sx+s2, 12-r, 5);
    plat_settile(S, W-3, 12, 9);
    for (int r=4;r<=11;r++) plat_settile(S, W-3, r, 8);
    b->x = 2.5f*PL_TILE; b->y = PL_GROUND*PL_TILE;
    S->plat_vx=0; S->plat_vy=0; S->plat_face=1; S->plat_cpx = 2.5f*PL_TILE;
    S->plat_run_t=0; S->plat_skid=0; S->plat_land_t=0; S->plat_jumpheld=0; S->plat_dust_t=0;
    S->plat_lvl_t=0; S->plat_show_tds=0; S->plat_show_tier=0; S->plat_show_newbest=0;   // MEDAL TIMES: a fresh level, a fresh clock
    for (int i=0;i<10;i++) S->plat_puff_a[i]=0;
}
