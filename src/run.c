// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: run.c — SUPER BUDDY RUN (Banzai World mode 3).
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]
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
static const int PLAT_MEDAL[16][3] = {
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
    for (int t=0;t<16;t++) if (medal_tier(b->plat_tbest[t], PLAT_MEDAL[t])==tier && (tier>0 || b->plat_tbest[t]>0)) n++;
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
static void plat_enter(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    S->plat_score=0; S->plat_bits=0; S->plat_hearts=3; S->plat_power=0; S->plat_t=0;
    plat_load_level(pi, b->plat_done+1);
    // MEDAL TIMES (feat 3): a player who has finished a level gets the
    // level select (the replay hook — medals live there); a brand-new
    // player's entry flow is exactly as before.
    S->plat_sel = (b->plat_done >= 1)? 1 : 0;
    S->plat_pick = b->plat_done + 1;
    S->plat_selt = 0;
}
static void plat_hurt(int pi, float fromx){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->plat_grace>0 || S->plat_cleart>0 || S->plat_deadt>0) return;
    if (S->plat_power>0){
        S->plat_power--; S->plat_grace=2.0f; sfx(16);
        scpy(S->note, "THE PATCH WORE OFF!", 72); S->note_t=1.8f;
    } else {
        S->plat_hearts--; S->plat_grace=2.0f; sfx(16);
        if (S->plat_hearts<=0){ S->plat_deadt=1.4f; return; }
        scpy(S->note, "OUCH! MIND THE BUGS.", 72); S->note_t=1.8f;
    }
    S->plat_vx = (b->x < fromx? -150 : 150); S->plat_vy = -190;
}
static void plat_bonk_tile(int pi, int tx, int ty){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    int t = plat_tile(S, tx, ty);
    if (t==3){
        plat_settile(S, tx, ty, 4); sfx(11);
        u32 h = hash2(tx*5+1, ty*9+2, (u32)b->bw_seed ^ (u32)S->plat_lvl*131u);
        int roll = (int)(h%100);
        if (roll<55){ S->plat_bits += 5; S->plat_score += 250; scpy(S->note, "A BURST OF BITS! +5", 72); S->note_t=1.6f; }
        else if (roll<82){ plat_en_add(S, 5, tx*PL_TILE+12, ty*PL_TILE); scpy(S->note, "A PATCH SHROOM! IT FIXES WHAT AILS YOU.", 72); S->note_t=2.0f; }
        else { plat_en_add(S, 6, tx*PL_TILE+12, ty*PL_TILE); { char *q=S->note; q=sappend(q, "A HOTFIX FLOWER! "); q=sappend(q, lbl_int()); q=sappend(q, " THROWS HOTFIXES."); *q=0; } S->note_t=2.0f; }
    } else if (t==2 && S->plat_power>=1){
        plat_settile(S, tx, ty, 0); S->plat_score += 50; sfx(4);
        splat(tx*PL_TILE+12, ty*PL_TILE+12, COL(150,100,60), 6, 90);
    } else if (t==2 || t==3) sfx(5);
}
// the BONUS VAULT under the level's tallest pipe: a small quiet
// room of bits and a heart, reached by pressing DOWN on the gold-
// banded pipe. The whole overworld (grid, critters, checkpoint) is
// parked in the spare buffers and restored on the way out.
static void plat_vault_enter(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    for (int i=0;i<PL_MAXW*PL_ROWS;i++) S->plat_grid2[i] = S->plat_grid[i];
    S->plat_savew = S->plat_w; S->plat_savex = b->x; S->plat_savey = b->y; S->plat_savecpx = S->plat_cpx;
    S->plat_saven = S->plat_en_n;
    for (int i=0;i<S->plat_en_n;i++) S->plat_saveen[i] = S->plat_en[i];
    for (int i=0;i<PL_MAXW*PL_ROWS;i++) S->plat_grid[i]=0;
    S->plat_en_n = 0; S->plat_w = 36; S->plat_sub = 1;
    for (int x=0;x<36;x++){ plat_settile(S, x, 13, 1); plat_settile(S, x, 14, 1); plat_settile(S, x, 0, 5); }
    for (int y=1;y<13;y++){ plat_settile(S, 0, y, 5); plat_settile(S, 35, y, 5); }
    // the haul: two arcs and a shelf of bits, and a heart on a plinth
    for (int i2=0;i2<7;i2++) plat_settile(S, 5+i2, 11 - (i2<3? i2 : 6-i2)/1, 10);
    for (int i2=0;i2<8;i2++) plat_settile(S, 14+i2, 10, 10);
    for (int i2=0;i2<3;i2++){ plat_settile(S, 24+i2, 9, 5); plat_settile(S, 24+i2, 8, 10); }
    plat_en_add(S, 7, 17*PL_TILE, PL_GROUND*PL_TILE);   // the heart (kind 7)
    // the way out: a pipe at the far end
    for (int r=0;r<3;r++) for (int c2=0;c2<2;c2++) plat_settile(S, 32+c2, 12-r, r==2? 7 : 6);
    b->x = 3.5f*PL_TILE; b->y = PL_GROUND*PL_TILE;
    S->plat_vx=0; S->plat_vy=0; S->plat_cpx = 3.5f*PL_TILE;
    scpy(S->note, "THE BONUS VAULT! HELP YOURSELF.", 72); S->note_t=2.4f; sfx(19);
}
static void plat_vault_exit(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    for (int i=0;i<PL_MAXW*PL_ROWS;i++) S->plat_grid[i] = S->plat_grid2[i];
    S->plat_w = S->plat_savew; S->plat_en_n = S->plat_saven;
    for (int i=0;i<S->plat_saven;i++) S->plat_en[i] = S->plat_saveen[i];
    b->x = S->plat_savex; b->y = S->plat_savey; S->plat_cpx = S->plat_savecpx;
    S->plat_vx=0; S->plat_vy=0; S->plat_sub = 0;
    scpy(S->note, "BACK TO THE RUN!", 72); S->note_t=1.6f; sfx(19);
}
static void plat_update(int pi, float dt, const BwIn *in){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (in->exit_){ bw_exit(pi, 0); return; }
    if (S->plat_sel){
        // MEDAL TIMES (feat 3): the level select — LEFT/RIGHT pick any
        // unlocked level, FIRE runs it (after a grace beat so the
        // cabinet-entry press can't confirm by accident), EXIT left
        // above. Nothing else in the level moves while you pick.
        int l_edge = (in->keys & K_LEFT) && !(S->plat_prevk & K_LEFT);
        int r_edge = (in->keys & K_RIGHT) && !(S->plat_prevk & K_RIGHT);
        S->plat_prevk = in->keys;
        S->plat_selt += dt;
        if (l_edge && S->plat_pick > 1){ S->plat_pick--; sfx(1); }
        if (r_edge && S->plat_pick < b->plat_done + 1){ S->plat_pick++; sfx(1); }
        if (in->sel && S->plat_selt > 0.35f){
            S->plat_sel = 0;
            plat_load_level(pi, S->plat_pick);
            { char *q=S->note; q=sappend(q, "LEVEL "); q=sapp_int(q, S->plat_lvl); q=sappend(q, " - "); q=sappend(q, REGION_NAME[S->plat_region]); q=sappend(q, ". ");
              if (S->plat_lvl<=16 && b->plat_tbest[S->plat_lvl-1]){ q=sappend(q, "BEST "); q=sapp_time_ds(q, b->plat_tbest[S->plat_lvl-1]); q=sappend(q, " - "); q=sappend(q, medal_name(medal_tier(b->plat_tbest[S->plat_lvl-1], PLAT_MEDAL[S->plat_lvl-1]))); q=sappend(q, ". "); }
              else if (S->plat_lvl<=16){ q=sappend(q, "NO TIME YET - GOLD IS "); q=sapp_time_ds(q, PLAT_MEDAL[S->plat_lvl-1][0]); q=sappend(q, ". "); }
              q=sappend(q, lbl_fire()); q=sappend(q, " JUMPS. STOMP BUGS!"); *q=0; }
            S->note_t=3.4f;
        }
        return;
    }
    int dn_edge = (in->keys & K_DOWN) && !(S->plat_prevk & K_DOWN);
    S->plat_prevk = in->keys;
    S->plat_t += dt;
    if (S->plat_cleart<=0) S->plat_lvl_t += dt;   // MEDAL TIMES: the level clock freezes at the flag touch
    if (S->plat_grace>0) S->plat_grace -= dt;
    if (b->atk_cd>0) b->atk_cd -= dt;
    if (S->plat_deadt>0){
        S->plat_deadt -= dt; b->y += 130*dt;
        if (S->plat_deadt<=0){ b->hp=0; b->fainted=1; bw_exit(pi, 1); }
        return;
    }
    if (S->plat_cleart>0){
        S->plat_cleart += dt;
        // victory hops while the flag slides down the pole
        float hop = ffabs(fsin(S->plat_cleart*6.2f)) * 9.0f * (1.0f - S->plat_cleart/1.9f);
        if (hop>0) b->y = S->plat_cleary - hop;
        if (S->plat_cleart>1.9f){
            if (S->plat_lvl > b->plat_done){ b->plat_done = S->plat_lvl; }
            buddy_award_xp(pi, 25);   // v1.19: a finish pays the buddy
            do_save();
            plat_load_level(pi, S->plat_lvl+1);
            { char *q=S->note; q=sappend(q, "DEPLOYED! LEVEL "); q=sapp_int(q, S->plat_lvl); q=sappend(q, " - "); q=sappend(q, REGION_NAME[S->plat_region]); q=sappend(q, "."); if (b->active) q=sappend(q, " +25 BUDDY XP"); *q=0; }
            S->note_t=2.6f;
        }
        return;
    }
    // --- the runner ---
    b->bw_x = b->x; b->bw_y = b->y;   // keep the stored BW spot in step (dbg_bwx reads it)
    int dir = 0;
    if (in->keys & K_LEFT) dir -= 1;
    if (in->keys & K_RIGHT) dir += 1;
    // momentum (Daniel: the running felt rigid): acceleration and
    // friction are gentler, air control is weaker than ground
    // control, a held run builds to a faster top speed, a jump
    // carries your speed with it — and letting go of the button
    // early cuts the jump short. In the air, momentum just carries.
    int grounded = S->plat_ground;
    if (dir!=0 && grounded && dir==S->plat_face) S->plat_run_t += dt;
    else if (grounded && dir==0) S->plat_run_t = 0;
    else if (grounded && dir!=S->plat_face) S->plat_run_t = 0;
    // Nov 15 (Daniel): still too fast and too slidey — top speed
    // down ~25%, friction way up so a stop is a stop, snappier
    // ground accel to reach the (lower) top sooner, overspeed
    // decays fast, weaker air drift. Jump/gravity untouched: the
    // level geometry (pipes, stairs, gaps) is tuned to them.
    // Nov 16 (Daniel): a little slower even — 100 base / 124 top
    // (was 110/136). Jump distance at full run is still ~100px,
    // clear of the widest generated gap (4 tiles) with a run-up.
    float maxs = 100.0f + 24.0f*fclamp(S->plat_run_t/0.9f, 0.0f, 1.0f);
    if (dir!=0){
        float acc = grounded? 1240.0f : 540.0f;
        if (grounded && ((dir>0 && S->plat_vx<-92) || (dir<0 && S->plat_vx>92))){
            acc = 2400.0f; S->plat_skid = 0.16f;   // skidding against your own momentum
            if (S->plat_dust_t<=0){ plat_puff(S, b->x - S->plat_face*8, b->y-2); S->plat_dust_t = 0.05f; }
        }
        // the cap has to be a cap: above it, the 560/s bleed must
        // WIN, so never feed accel into an overspeed push. Playtest
        // find (Nov 19): accel 1240 vs bleed 560 netted +680/s for as
        // long as the key was held — a plain held run rocketed past
        // 600 u/s inside two seconds, which is also why the runner
        // "felt too fast" no matter where the cap was tuned. With
        // the guard, a push at/above the cap just holds it while a
        // knockback's overspeed still bleeds off at 560/s as meant.
        if (!((dir>0 && S->plat_vx>=maxs) || (dir<0 && S->plat_vx<=-maxs))) S->plat_vx += dir*acc*dt;
        if (dir>0 && S->plat_vx>maxs){ float d2 = S->plat_vx-560*dt; S->plat_vx = d2>maxs? d2 : maxs; }
        if (dir<0 && S->plat_vx<-maxs){ float d2 = S->plat_vx+560*dt; S->plat_vx = d2<-maxs? d2 : -maxs; }
        S->plat_face = dir;
    } else if (grounded){
        float fr = 1750.0f*dt;
        if (S->plat_vx > 0) S->plat_vx = S->plat_vx>fr? S->plat_vx-fr : 0;
        else if (S->plat_vx < 0) S->plat_vx = S->plat_vx<-fr? S->plat_vx+fr : 0;
    }
    if (S->plat_skid>0) S->plat_skid -= dt;
    if (S->plat_land_t>0) S->plat_land_t -= dt;
    if (S->plat_dust_t>0) S->plat_dust_t -= dt;
    if (grounded && (S->plat_vx>150 || S->plat_vx<-150) && S->plat_dust_t<=0){
        plat_puff(S, b->x - S->plat_face*9, b->y-1); S->plat_dust_t = 0.13f;   // sprint dust
    }
    if (grounded && S->plat_vx!=0){ b->walk_ph += dt * (5.0f + (S->plat_vx<0? -S->plat_vx : S->plat_vx) * 0.05f); b->mv_t = 0.15f; }
    b->aim = S->plat_face>0? 0 : PI;
    // jumping: Space, or an up-tap (the touch stick's jump)
    int upnow = (in->keys & K_UP)? 1:0;
    if (in->sel || (upnow && !S->plat_up_prev)) S->plat_jbuf = 0.12f; else if (S->plat_jbuf>0) S->plat_jbuf -= dt;
    S->plat_up_prev = upnow;
    if (S->plat_ground) S->plat_coyote = 0.10f; else if (S->plat_coyote>0) S->plat_coyote -= dt;
    if (S->plat_jbuf>0 && S->plat_coyote>0){
        float sp2 = S->plat_vx<0? -S->plat_vx : S->plat_vx;
        S->plat_vy = -560 - (sp2*0.35f>95? 95 : sp2*0.35f);
        S->plat_jbuf = 0; S->plat_coyote = 0; S->plat_ground = 0; S->plat_jumpheld = 1; sfx(6);
    }
    if (S->plat_jumpheld){
        if (S->plat_vy >= -60) S->plat_jumpheld = 0;
        else if (!in->sel_held){ S->plat_vy = -210; S->plat_jumpheld = 0; }   // released early: a short hop
    }
    S->plat_vy += 1500.0f*dt;
    if ((in->keys & K_DOWN) && !S->plat_ground) S->plat_vy += 2200.0f*dt;   // stomp-dive
    if (S->plat_vy > 640) S->plat_vy = 640;
    float vy_pre = S->plat_vy; int was_ground = S->plat_ground;
    // integrate in substeps so fast falls never tunnel
    {
        float dx = S->plat_vx*dt, dy = S->plat_vy*dt;
        float adx2 = dx<0? -dx:dx, ady2 = dy<0? -dy:dy;
        int steps = (int)((adx2>ady2? adx2 : ady2)/4.0f) + 1;
        if (steps>8) steps=8;
        float sx = dx/steps, sy = dy/steps;
        for (int s2=0;s2<steps;s2++){
            b->x += sx;
            if (plat_box_hit(S, b->x, b->y)){ b->x -= sx; if (sx!=0){ /* step back to the wall face */ int guard=0; while (!plat_box_hit(S, b->x + (sx>0? 1:-1), b->y) && guard++<8) b->x += (sx>0? 1:-1); } S->plat_vx = 0; }
            if (b->x < 10) b->x = 10;
            float oldy = b->y;
            b->y += sy;
            if (plat_box_hit(S, b->x, b->y)){
                b->y = oldy;
                if (sy>0){ int g2=0; while (!plat_box_hit(S, b->x, b->y+1.0f) && g2++<8) b->y += 1.0f;
                           S->plat_ground = 1; S->plat_vy = 0; }
                else if (sy<0){
                    // head bonk: the tile at head-centre gets it
                    int tx = (int)(b->x/PL_TILE), ty = (int)((b->y-23)/PL_TILE);
                    if (!plat_solidt(plat_tile(S, tx, ty))){ tx = (int)((b->x + (S->plat_face>0? 7:-7))/PL_TILE); }
                    plat_bonk_tile(pi, tx, ty);
                    S->plat_vy = 0;
                }
                sy = 0;
            }
        }
    }
    S->plat_ground = plat_box_hit(S, b->x, b->y+1.5f)? 1:0;
    if (S->plat_ground && S->plat_vy>0) S->plat_vy = 0;
    if (!was_ground && S->plat_ground && vy_pre>420){
        S->plat_land_t = 0.14f;   // a hard landing: crouch + a puff of dust
        plat_puff(S, b->x-6, b->y-1); plat_puff(S, b->x+6, b->y-1);
    }
    // checkpoint + flag. The midpoint post arms only when the runner
    // REACHES it: arming it a few tiles out of spawn (the old rule)
    // teleported every early pit fall to the middle of the level,
    // skipping the whole first half — and banked medal times for
    // courses the runner never ran (playtest find, Nov 19: a level-1
    // pit dive banked a 141ds gold against a 261ds threshold whose
    // derivation assumes the full course). Falls before the post
    // respawn at the start post, as the white flag says.
    if (S->plat_cpx < (S->plat_w/2)*PL_TILE && b->x > (S->plat_w/2)*PL_TILE) S->plat_cpx = (S->plat_w/2)*PL_TILE;
    {
        int tx = (int)(b->x/PL_TILE);
        for (int ty=0;ty<PL_ROWS;ty++) if (plat_tile(S, tx, ty)==8){
            S->plat_cleart = 0.001f; S->plat_cleary = b->y; S->plat_score += 500 + S->plat_hearts*100; sfx(11);
            scpy(S->note, "FLAG PLANTED! LEVEL CLEAR!", 72); S->note_t=2.6f;
            // MEDAL TIMES (feat 3): the finish is THIS moment — bank
            // the level time now (not after the celebration), Zen
            // excluded by the standing records rule
            { int tds = (int)(S->plat_lvl_t * 10.0f);
              S->plat_show_tds = tds;
              S->plat_show_tier = (S->plat_lvl<=16)? medal_tier(tds, PLAT_MEDAL[S->plat_lvl-1]) : 0;
              S->plat_show_newbest = 0;
              if (!G.zen_run && S->plat_lvl>=1 && S->plat_lvl<=16){
                  int *slot = &b->plat_tbest[S->plat_lvl-1];
                  if (!*slot || tds < *slot){ *slot = tds; S->plat_show_newbest = 1; }
              } }
            break;
        }
    }
    // bits underfoot
    {
        int tx0=(int)((b->x-8)/PL_TILE), tx1=(int)((b->x+7)/PL_TILE), ty0=(int)((b->y-22)/PL_TILE), ty1=(int)((b->y-1)/PL_TILE);
        for (int ty=ty0;ty<=ty1;ty++) for (int tx=tx0;tx<=tx1;tx++) if (plat_tile(S, tx, ty)==10){
            plat_settile(S, tx, ty, 0); S->plat_bits++; S->plat_score += 50; sfx(6);
            if (S->plat_bits%25==0 && S->plat_hearts<5){ S->plat_hearts++; scpy(S->note, "25 BITS! +1 HEART", 72); S->note_t=2.0f; }
        }
    }
    // the vault pipe: stand on the gold-banded pipe and press DOWN
    if (S->plat_ground){
        int btx = (int)(b->x/PL_TILE);
        int ontop = plat_tile(S, btx, (int)((b->y+2)/PL_TILE))==7;
        if (!S->plat_sub && S->plat_vault_tx>=0 && ontop && (btx==S->plat_vault_tx || btx==S->plat_vault_tx+1)){
            if (dn_edge) plat_vault_enter(pi);
            else if (S->note_t<=0){ scpy(S->note, "DOWN - INTO THE VAULT!", 72); S->note_t=1.4f; }
        } else if (S->plat_sub && ontop && (btx==32 || btx==33)){
            if (dn_edge) plat_vault_exit(pi);
            else if (S->note_t<=0){ scpy(S->note, "DOWN - BACK TO THE RUN!", 72); S->note_t=1.4f; }
        }
    }
    if (S->plat_ground) S->plat_chain = 0;   // a stomp chain only lives while you stay airborne
    // hotfix fireballs
    if (in->interact && S->plat_power>=2 && b->atk_cd<=0){
        b->atk_cd = 0.34f;
        for (int i=0;i<8;i++) if (!S->plat_fa[i]){
            S->plat_fa[i]=1; S->plat_ft[i]=2.2f;
            S->plat_fx[i]=b->x + S->plat_face*12; S->plat_fy[i]=b->y-12;
            S->plat_fvx[i]=S->plat_face*235; S->plat_fvy[i]=40; sfx(12);
            break;
        }
    }
    for (int i=0;i<8;i++) if (S->plat_fa[i]){
        S->plat_ft[i] -= dt; if (S->plat_ft[i]<=0){ S->plat_fa[i]=0; continue; }
        S->plat_fvy[i] += 900*dt; if (S->plat_fvy[i]>420) S->plat_fvy[i]=420;
        S->plat_fx[i] += S->plat_fvx[i]*dt; S->plat_fy[i] += S->plat_fvy[i]*dt;
        if (plat_solid_px(S, S->plat_fx[i], S->plat_fy[i]+5)){
            S->plat_fy[i] -= 2; S->plat_fvy[i] = -250;
            if (plat_solid_px(S, S->plat_fx[i], S->plat_fy[i]-6)){ S->plat_fa[i]=0; }
        }
        if (plat_solid_px(S, S->plat_fx[i] + (S->plat_fvx[i]>0? 5:-5), S->plat_fy[i])) S->plat_fa[i]=0;
        for (int e2=0;e2<S->plat_en_n && S->plat_fa[i];e2++){
            PlatEn *e=&S->plat_en[e2];
            if (e->kind>=5) continue;
            float dx=S->plat_fx[i]-e->x, dy=S->plat_fy[i]-(e->y-8);
            if (dx*dx+dy*dy < 15*15){ e->kind=-1; S->plat_fa[i]=0; S->plat_score += 100; sfx(4); splat(e->x, e->y-6, COL(255,150,60), 8, 100); }
        }
    }
    // dust puffs drift up and thin out
    for (int i=0;i<10;i++) if (S->plat_puff_a[i]){
        S->plat_puff_t[i] -= dt; S->plat_puff_y[i] -= 13*dt;
        if (S->plat_puff_t[i]<=0) S->plat_puff_a[i]=0;
    }
    // --- the critters ---
    for (int i=0;i<S->plat_en_n;i++){
        PlatEn *e=&S->plat_en[i];
        if (e->kind<0) continue;
        e->t += dt;
        if (e->kind==3){   // the piranha bug rides its pipe
            float ph = e->t - (int)(e->t/3.4f)*3.4f;
            float rise = ph<0.8f? ph/0.8f : ph<1.9f? 1.0f : ph<2.7f? (2.7f-ph)/0.8f : 0.0f;
            e->vy = rise;   // vy doubles as the rise fraction for the drawing
            continue;
        }
        if (e->kind==4){   // the fly drifts and bobs
            e->x -= 22*dt;
            if (e->x < b->x - 700) e->kind=-1;
            continue;
        }
        // walkers (and the shell, and the power-ups) get gravity
        if (e->kind!=2 || e->state==1) e->x += e->vx*dt;
        e->vy += 1500*dt; if (e->vy>600) e->vy=600;
        {
            float ny = e->y + e->vy*dt;
            // land on the first solid under the feet
            int txa=(int)((e->x-6)/PL_TILE), txb=(int)((e->x+6)/PL_TILE), tyy=(int)(ny/PL_TILE);
            if (plat_solidt(plat_tile(S, txa, tyy)) || plat_solidt(plat_tile(S, txb, tyy))){
                e->y = tyy*PL_TILE; e->vy = 0;
            } else e->y = ny;
            if (e->y > PL_ROWS*PL_TILE + 60){ e->kind=-1; continue; }
            // walls turn walkers around; a sliding shell bounces off
            if (e->vx!=0){
                float ahead = e->x + (e->vx>0? 9:-9);
                if (plat_solid_px(S, ahead, e->y-6)){
                    e->vx = -e->vx;
                    if (e->kind==2 && e->state==1) sfx(5);
                }
                // ledges turn walkers around too (shells and power-ups roll on)
                if (e->kind==0 || e->kind==1){
                    float below = e->y + 4;
                    if (!plat_solid_px(S, ahead, below)){ e->vx = -e->vx; }
                }
            }
        }
        // contact with the runner
        if (S->plat_cleart>0) continue;
        float dx = b->x - e->x, dy = (b->y-11) - (e->y-8);
        if (dx*dx > 17*17 || dy*dy > 20*20) continue;
        if (e->kind==7){   // the vault heart
            if (S->plat_hearts<5) S->plat_hearts++;
            e->kind=-1; S->plat_score += 200; sfx(19);
            scpy(S->note, "A SPARE HEART! +1.", 72); S->note_t=1.8f;
            continue;
        }
        if (e->kind==5 || e->kind==6){
            S->plat_power = e->kind==5? 1 : 2; e->kind=-1; S->plat_score += 200; sfx(19);
            if (S->plat_power==1){ scpy(S->note, "PATCHED UP! BRICKS BREAK NOW.", 72); }
            else { char *q=S->note; q=sappend(q, "HOTFIX POWER! "); q=sappend(q, lbl_int()); q=sappend(q, " THROWS FIRE."); *q=0; }
            S->note_t=2.0f;
            continue;
        }
        if (e->kind==3){ if (e->vy>0.4f) plat_hurt(pi, e->x); continue; }
        int stomp = S->plat_vy > 60 && (b->y - (e->y-16)) < 13;
        if (e->kind==2 && e->state==0){
            // a resting shell: any touch kicks it away from the runner
            e->state=1; e->vx = (e->x >= b->x? 205 : -205); sfx(5); S->plat_grace = 0.35f;
            if (stomp) S->plat_vy = -300;
            continue;
        }
        if (stomp){
            if (e->kind==0 || e->kind==4){
                // the stomp chain: each airborne stomp doubles the
                // last — 100, 200, 400, 800, 1600 — until you land
                e->kind=-1;
                int pts = 100 << (S->plat_chain<4? S->plat_chain : 4);
                S->plat_score += pts; S->plat_chain++;
                if (S->plat_chain>=2){
                    char buf[48]; char *q=buf;
                    q=sappend(q, "STOMP CHAIN X"); q=sapp_int(q, S->plat_chain); q=sappend(q, "! +"); q=sapp_int(q, pts); *q=0;
                    scpy(S->note, buf, 72); S->note_t=1.2f;
                }
                sfx(4); splat(e->x, e->y-8, COL(120,90,160), 7, 95);
            }
            else if (e->kind==1){ e->kind=2; e->state=0; e->vx=0; sfx(4); }
            else if (e->kind==2){ e->state=0; e->vx=0; sfx(4); }
            S->plat_vy = -350;
            continue;
        }
        if (e->kind==2 && e->state==1 && S->plat_grace<=0){ plat_hurt(pi, e->x); continue; }
        if (e->kind==0 || e->kind==1 || e->kind==4) plat_hurt(pi, e->x);
    }
    // a sliding shell clears out whatever it touches
    for (int i=0;i<S->plat_en_n;i++){
        PlatEn *sh=&S->plat_en[i];
        if (sh->kind!=2 || sh->state!=1) continue;
        for (int j=0;j<S->plat_en_n;j++){
            if (j==i) continue; PlatEn *e=&S->plat_en[j];
            if (e->kind<0 || e->kind>=5 || e->kind==3) continue;
            float dx=sh->x-e->x, dy=sh->y-e->y;
            if (dx*dx<18*18 && dy*dy<18*18){ e->kind=-1; S->plat_score += 100; sfx(4); splat(e->x, e->y-8, COL(120,200,120), 7, 95); }
        }
    }
    // the long drop
    if (b->y > PL_ROWS*PL_TILE + 26){
        S->plat_hearts--;
        if (S->plat_hearts<=0){ S->plat_deadt=1.4f; b->y = PL_ROWS*PL_TILE; }
        else { // back to the checkpoint — but never inside scenery:
               // if the post's own column is blocked, scan a few
               // columns either way for a free box (the checkpoint
               // respawn could embed him in a rock — Daniel)
               float rx = S->plat_cpx;
               if (plat_box_hit(S, rx, PL_GROUND*PL_TILE - 2)){
                   int found = 0;
                   for (int d2=1; d2<=4 && !found; d2++){
                       if (!plat_box_hit(S, S->plat_cpx + d2*PL_TILE, PL_GROUND*PL_TILE - 2)){ rx = S->plat_cpx + d2*PL_TILE; found = 1; }
                       else if (!plat_box_hit(S, S->plat_cpx - d2*PL_TILE, PL_GROUND*PL_TILE - 2)){ rx = S->plat_cpx - d2*PL_TILE; found = 1; }
                   }
               }
               b->x = rx; b->y = PL_GROUND*PL_TILE - 2; S->plat_vx=0; S->plat_vy=0; S->plat_grace=2.0f;
               scpy(S->note, "DOWN A PIT! BACK TO THE CHECKPOINT.", 72); S->note_t=2.2f; sfx(16); }
    }
    // compact the critter list now and then
    if (S->plat_en_n>0 && S->plat_en[0].kind<0){
        int w=0; for (int i=0;i<S->plat_en_n;i++) if (S->plat_en[i].kind>=0) S->plat_en[w++]=S->plat_en[i];
        S->plat_en_n=w;
    }
}
static void plat_draw_enemy(PlatEn *e, int bob, int strain){
    int x = SX(e->x), y = SY(e->y);
    // v1.22: the grounded kinds sit ON the ground now — the fly and
    // the pipe piranha ride above their ground line, so they skip it
    if (e->kind==0 || e->kind==1 || e->kind==2 || e->kind==5 || e->kind==6)
        shadow_ellipse(x, y+1, 9, 2);
    if (e->kind==7){   // the vault heart, gently bobbing
        int hy = y - 12 + (int)(fsin(e->t*3.0f)*2.0f);
        disc_fill(x-4, hy-3, 5, COL(240,80,90)); disc_fill(x+4, hy-3, 5, COL(240,80,90));
        rect_fill(x-8, hy-3, 17, 6, COL(240,80,90));
        tri_fill(x-8, hy+2, x+9, hy+2, x, hy+11, COL(240,80,90));
        disc_fill(x-3, hy-4, 2, COL(255,190,195));
        return;
    }
    if ((e->kind==0 || e->kind==1) && G.eddie){   // EDDIE MODE (v1.24): the crawler is a puffball here
                                     // too — same footprint, orange fuzz, big red nose
        ellipse_fill(x-5+(bob?1:0), y-1, 4, 2, OUTLINE);
        ellipse_fill(x+5-(bob?1:0), y-1, 4, 2, OUTLINE);
        for (int fz=0; fz<5; fz++) circle_fill(x-8+fz*4, y-12+(fz%2), 3, COL(243,146,44));
        ellipse_fill(x, y-7, 11, 8, OUTLINE);
        ellipse_shade(x, y-7, 10, 7, COL(243,146,44));
        ellipse_shade(x, y-4, 6, 3, COL(255,224,168));
        disc_fill(x-4, y-9, 2, COL(255,255,255)); disc_fill(x+4, y-9, 2, COL(255,255,255));
        disc_fill(x-4, y-9, 1, OUTLINE); disc_fill(x+4, y-9, 1, OUTLINE);
        circle_fill(x, y-6, 4, COL(228,54,46)); pset(x-1, y-7, COL(255,255,255));
    } else if (e->kind==0 || e->kind==1){   // crawlers: a bug with a dome (green for the shell-bug;
                                     // the region's dominant strain colours the plain crawler)
        ellipse_fill(x, y-7, 10, 7, e->kind==1? COL(64,150,70) : (strain>=0? BUG_COL[strain] : COL(96,74,140)));
        if (e->kind==1){ rect_fill(x-10, y-9, 20, 3, COL(50,120,56)); }
        disc_fill(x + (e->vx<0? -5:5), y-8, 2, COL(255,255,255));
        disc_fill(x + (e->vx<0? -5:5), y-8, 1, COL(20,16,24));
        for (int l=0;l<3;l++){ int lx2 = x-6+l*6; line_thick(lx2, y-2, lx2-2, y+ (bob&&l%2? 2:0), 1, COL(40,30,50)); }
    } else if (e->kind==2){          // the shell, at rest or at speed
        ellipse_fill(x, y-5, 10, 6, COL(64,150,70));
        rect_fill(x-10, y-3, 20, 3, COL(220,230,220));
        if (e->state==1){   // spinning: the highlights chase each other around the shell
            int ph = ((int)(e->x*0.45f) % 4 + 4) % 4;
            static const int SPO[4][2] = { {-5,-2}, {0,-4}, {5,-2}, {0,0} };
            disc_fill(x+SPO[ph][0], y-5+SPO[ph][1], 2, COL(255,255,255));
            disc_fill(x-SPO[ph][0], y-5+SPO[(ph+2)&3][1], 2, COL(210,240,210)); }
    } else if (e->kind==3){          // the piranha bug, mid-rise
        int top = SY(e->y - e->vy*34.0f);   // v1.17 fix: this was raw WORLD y used as a screen y — fine only where camy made SY the identity (solo desktop); on touch (deck 84) and split-screen the piranha drew ~3 tiles too low, buried in its pipe, while its bite used the true position
        if (e->vy>0.03f){
            ellipse_fill(x, top+8, 9, 8, COL(196,64,54));
            disc_fill(x-3, top+5, 2, COL(255,255,255)); disc_fill(x+3, top+5, 2, COL(255,255,255));
            int chomp = (e->vy>0.85f && ((int)(e->t*10)&1))? 2 : 0;   // the teeth chomp as it tops out
            for (int t2=0;t2<3;t2++){ rect_fill(x-5+t2*5, top+12, 3, 4+chomp, COL(255,240,220)); }
        }
    } else if (e->kind==4){          // the fly
        int fy2 = y-8 + (int)(fsin(e->t*7)*3);
        ellipse_fill(x, fy2, 8, 6, COL(150,90,60));
        int wf = (int)(fsin(e->t*30)*3);
        ellipse_fill(x-5, fy2-7+wf, 5, 2, COL(230,240,250));
        ellipse_fill(x+5, fy2-7-wf, 5, 2, COL(230,240,250));
        disc_fill(x-4, fy2-1, 2, COL(255,220,80));
    } else if (e->kind==5){          // the patch shroom ambles by
        rect_fill(x-3, y-7, 6, 7, COL(240,235,220));
        ellipse_fill(x, y-8, 9, 6, COL(210,70,60));
        rect_fill(x-1, y-12, 3, 7, COL(255,255,255)); rect_fill(x-3, y-10, 7, 3, COL(255,255,255));
    } else if (e->kind==6){          // the hotfix flower stands its ground
        line_thick(x, y, x, y-10, 2, COL(70,150,70));
        for (int p2=0;p2<5;p2++){ float a = p2*1.2566f + e->t; disc_fill(x + (int)(fcos(a)*5), y-13 + (int)(fsin(a)*5), 3, COL(255,140,40)); }
        disc_fill(x, y-13, 3, COL(255,220,90));
    }
}
static void plat_render(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    float camx = b->x; float half = vw2/2.0f;
    if (camx < half) camx = half;
    if (camx > S->plat_w*PL_TILE - half) camx = S->plat_w*PL_TILE - half;
    G.camx = camx;
    // Nov 15 (Daniel): on the touch stick scheme the joystick and
    // JUMP button sit right on the ground line and swallow the
    // runner. Give the level an 84px deck: the world rides up by
    // that much and a dark stage fills the strip under it, so the
    // controls live on the deck, not on the game. Only the local
    // player's full-width view — split co-op draws no stick HUD.
    int plat_deck = (pi==G.my_id && mobile_ui() && !pad_mode() && G.touch_ctl==1 && vw2>=VW)? 84 : 0;
    G.camy = 180 + plat_deck;
    // sky, then two layers of server skyline drifting behind — the
    // sky wears the level's REGION (the vault borrows the Depths)
    int rg = S->plat_sub? 7 : S->plat_region;
    // THE LIVING SKY (v1.25 feat 12): the sky wears the hour too —
    // bands, skyline base and clouds through tod_sky; the tiles,
    // the runner and the HUD keep their daylight colours.
    rect_fill(vx, vy, vw2, vh2, tod_sky(PLAT_SKY[rg][0]));
    rect_fill(vx, vy+vh2/3, vw2, vh2/3, tod_sky(PLAT_SKY[rg][1]));   // gradient step (glow-up)
    rect_fill(vx, vy+vh2*2/3, vw2, vh2/3, tod_sky(PLAT_SKY[rg][2]));
    // the sun, high and pleased with itself (glow-up) — a pale moon
    // over the dark countries, and over every country at night
    { int sunx = vx + vw2 - 84, suny = vy + 44;
      int dark = (rg==5 || rg==7) || tod_darkf() > 0.5f;
      glow_ellipse(sunx, suny, 34, 34, dark? COL(200,204,225) : COL(255,236,160), 60);
      disc_fill(sunx, suny, 15, dark? COL(226,230,242) : COL(255,240,170));
      disc_fill(sunx-4, suny-4, 6, dark? COL(240,242,250) : COL(255,250,215)); }
    // lazy clouds on the slowest parallax (glow-up)
    { int coff = (int)(camx*0.12f + G.anim_t*3.0f);
      u32 cloudc = tod_sky((rg==5 || rg==7)? COL(126,122,158) : COL(235,246,255));
      for (int sx = vx - (coff % 150) - 150; sx < vx+vw2+150; sx += 150){
          int idx = (coff + sx - vx)/150;
          u32 hh = hash2(idx, 3, 0xC10Du);
          int cy2 = vy + 18 + (int)(hh % 44), cs = 12 + (int)((hh>>6) % 10);
          ellipse_fill(sx+30, cy2, cs+10, cs*2/3, cloudc);
          ellipse_fill(sx+52, cy2-4, cs+4, cs/2, col_shade(cloudc, 1.05f));
          ellipse_fill(sx+12, cy2+2, cs, cs/2, col_shade(cloudc, 0.96f));
      } }
    for (int layer=0; layer<2; layer++){
        float par = layer? 0.65f : 0.35f;
        u32 lc = col_shade(tod_sky(PLAT_SKY[rg][2]), layer? 0.60f : 0.76f);
        int off = (int)(camx*par) % 92;
        for (int sx = vx - off - 92; sx < vx+vw2+92; sx += 92){
            int wx = sx + (int)(camx*par) - vx;
            int hh = 46 + (int)(hash2(wx/92, layer*31+7, 0x51DEu)%54) + layer*16;
            rect_fill(sx, vy+vh2-hh-52, 54, hh, lc);
            for (int wnd=0; wnd<4; wnd++) if ((hash2(wx/92, wnd*3+layer, 0xBEEFu)&3)==0) rect_fill(sx+8+wnd*11, vy+vh2-hh-38, 5, 7, COL(250,230,150));
        }
    }
    int tx0 = (int)((camx-half)/PL_TILE)-1, tx1 = (int)((camx+half)/PL_TILE)+1;
    if (tx0<0) tx0=0; if (tx1>=S->plat_w) tx1=S->plat_w-1;
    for (int ty=0;ty<PL_ROWS;ty++) for (int tx=tx0;tx<=tx1;tx++){
        int t = plat_tile(S, tx, ty); if (!t) continue;
        int x = SX(tx*PL_TILE), y = SY(ty*PL_TILE);
        if (t==1){
            // the earth itself is regional: topsoil in the region's
            // own green (or frost, or neon loam), dirt a dark shade
            // of its second ground tone
            int above = plat_tile(S, tx, ty-1);
            rect_fill(x, y, PL_TILE, PL_TILE, col_shade(REGION_GB[rg], 0.52f));
            rect_fill(x, y, PL_TILE, 4, col_shade(REGION_GB[rg], 0.68f));
            if (above!=1){ rect_fill(x, y, PL_TILE, 8, REGION_GA[rg]); rect_fill(x, y+6, PL_TILE, 3, REGION_GB[rg]); }
        } else if (t==2){
            rect_fill(x+1, y+1, PL_TILE-2, PL_TILE-2, COL(168,102,58));
            rect_line(x+1, y+1, PL_TILE-2, PL_TILE-2, COL(110,62,34));
            line_thick(x+2, y+11, x+PL_TILE-2, y+11, 1, COL(110,62,34));
        } else if (t==3){
            float pulse = 0.9f + 0.1f*fsin(S->plat_t*5 + tx);
            rect_fill(x+1, y+1, PL_TILE-2, PL_TILE-2, col_shade(COL(238,186,54), pulse));
            rect_line(x+1, y+1, PL_TILE-2, PL_TILE-2, COL(160,116,26));
            disc_fill(x+12, y+12, 5, COL(255,240,180)); disc_fill(x+12, y+12, 2, COL(160,116,26));
        } else if (t==4){
            rect_fill(x+1, y+1, PL_TILE-2, PL_TILE-2, COL(146,110,70));
            rect_line(x+1, y+1, PL_TILE-2, PL_TILE-2, COL(100,74,44));
        } else if (t==5){
            rect_fill(x, y, PL_TILE, PL_TILE, col_shade(REGION_GA[rg], 0.78f));
            rect_fill(x, y, PL_TILE, 3, col_shade(REGION_GA[rg], 0.97f));
            disc_fill(x+4, y+4, 1, col_shade(REGION_GA[rg], 0.5f)); disc_fill(x+PL_TILE-5, y+PL_TILE-5, 1, col_shade(REGION_GA[rg], 0.5f));
        } else if (t==6 || t==7){
            rect_fill(x+2, y, PL_TILE-4, PL_TILE, COL(38,148,96));
            rect_fill(x+5, y, 4, PL_TILE, COL(70,180,120));
            if (t==7){
                // the vault pipes (in and out) wear a gold band
                int gold = (S->plat_sub && (tx==32 || tx==33)) || (!S->plat_sub && (tx==S->plat_vault_tx || tx==S->plat_vault_tx+1));
                if (gold){ float pu = 0.9f + 0.1f*fsin(S->plat_t*5);
                    rect_fill(x-1, y, PL_TILE+2, 7, col_shade(COL(238,186,54), pu));
                    rect_fill(x-1, y+5, PL_TILE+2, 2, COL(160,116,26)); }
                else rect_fill(x-1, y, PL_TILE+2, 7, COL(30,128,84));
            }
        } else if (t==8){
            rect_fill(x+10, y, 3, PL_TILE, COL(190,196,204));
            if (ty==4){
                // the flag ripples in the wind, and once the level is
                // cleared it SLIDES down the pole (it used to teleport)
                int slide = S->plat_cleart>0? (int)(fclamp(S->plat_cleart/0.8f, 0.0f, 1.0f)*118) : 0;
                int flagy = y + 2 + slide;
                for (int r=0;r<9;r++){
                    int xo = (int)(fsin(S->plat_t*9 + r*0.85f)*2.0f);
                    u32 fc = ((r + (int)(S->plat_t*6))%3==0)? COL(104,214,110) : COL(84,196,90);
                    rect_fill(x+13+xo, flagy+r, 15-r, 1, fc);
                }
                draw_text(x-8, flagy+12, "DEPLOY", 1, COL(30,90,40)); }
        } else if (t==9){
            rect_fill(x, y, PL_TILE, PL_TILE, COL(110,116,124));
            rect_fill(x, y, PL_TILE, 4, COL(150,156,164));
        } else if (t==10){
            int sq = (int)(fsin(S->plat_t*6 + tx*1.7f)*2.5f);
            ellipse_fill(x+12, y+12, 5-(sq<0?-sq:sq)/2+2, 6, COL(250,205,70));
            disc_fill(x+12, y+12, 2, COL(200,150,40));
        }
    }
    // the deck under the world (touch stick scheme, see above):
    // a dark stage strip the controls can sit on
    if (plat_deck){
        rect_fill(vx, vy + vh2 - plat_deck, vw2, plat_deck, COL(14, 13, 22));
        rect_fill(vx, vy + vh2 - plat_deck, vw2, 2, COL(52, 48, 74));
    }
    // the checkpoint post
    {
        int x = SX(S->plat_cpx), y = SY(PL_GROUND*PL_TILE);
        rect_fill(x, y-34, 3, 34, COL(200,204,210));
        int passed = b->x > S->plat_cpx;
        for (int r=0;r<7;r++) rect_fill(x+3, y-34+r, 11-r, 1, passed? COL(84,196,90) : COL(240,240,240));
    }
    for (int i=0;i<S->plat_en_n;i++) if (S->plat_en[i].kind>=0) plat_draw_enemy(&S->plat_en[i], (int)(S->plat_t*8)&1, REGION_BUG[S->plat_region]);
    for (int i=0;i<8;i++) if (S->plat_fa[i]){
        int x = SX(S->plat_fx[i]), y = SY(S->plat_fy[i]);
        disc_fill(x, y, 5, COL(255,130,40)); disc_fill(x, y, 3, COL(255,200,80));
    }
    // dust puffs
    for (int i=0;i<10;i++) if (S->plat_puff_a[i]){
        int x = SX(S->plat_puff_x[i]), y = SY(S->plat_puff_y[i]);
        int rr = 3 + (int)((0.4f - S->plat_puff_t[i])*16);
        glow_ellipse(x, y, rr, rr*2/3, COL(232,220,196), (int)(S->plat_puff_t[i]/0.4f*120));
    }
    // a ground shadow that shrinks as he flies — reads the height at a glance
    { int gy = -1; int tx2 = (int)(b->x/PL_TILE);
      for (int ty2=(int)(b->y/PL_TILE); ty2<PL_ROWS; ty2++) if (plat_solidt(plat_tile(S, tx2, ty2))){ gy = ty2*PL_TILE; break; }
      if (gy>=0){ float hgt = gy - b->y; int sr = (int)(9 - hgt/18); if (sr<3) sr=3; if (sr>9) sr=9;
          shadow_ellipse(SX(b->x), SY(gy)+2, sr, 2); } }
    // the runner, drawn by the shared buddy brush (feet -> centre)
    if (!(S->plat_grace>0 && ((int)(S->plat_t*14)&1))){
        float savey = b->y; b->y -= 12 - (S->plat_land_t>0? 2 : 0); draw_buddy(b); b->y = savey;   // a landing crouch
        if (S->plat_power==1){ int x=SX(b->x), y=SY(b->y-12); rect_line(x-14, y-14, 28, 28, COL(250,205,70)); }
        if (S->plat_power==2){ int x=SX(b->x), y=SY(b->y-12); rect_line(x-14, y-14, 28, 28, COL(255,130,40)); }
    }
    // HUD
    rect_fill(vx, vy, vw2, 20, COL(16,20,28));
    { char buf[96]; char *q=buf;
      q=sappend(q, "LEVEL "); q=sapp_int(q, S->plat_lvl);
      q=sappend(q, " - "); q=sappend(q, S->plat_sub? "BONUS VAULT" : REGION_NAME[S->plat_region]);
      q=sappend(q, "   SCORE "); q=sapp_int(q, S->plat_score);
      q=sappend(q, "   BITS "); q=sapp_int(q, S->plat_bits);
      if (S->plat_power==1) q=sappend(q, "   PATCHED");
      if (S->plat_power==2) q=sappend(q, "   HOTFIX!");
      *q=0; draw_text(vx+8, vy+6, buf, 1, COL(240,244,250)); }
    for (int hh2=0;hh2<S->plat_hearts;hh2++){
        int hx = vx+vw2-16-hh2*16, hy = vy+10;
        disc_fill(hx-3, hy-2, 4, COL(240,80,90)); disc_fill(hx+3, hy-2, 4, COL(240,80,90));
        rect_fill(hx-6, hy-2, 13, 5, COL(240,80,90));
    }
    if (S->plat_cleart>0){
        draw_text_center(vx+vw2/2, vy+vh2/3, "LEVEL CLEAR!", 2, COL(255,244,180));
        // MEDAL TIMES (feat 3): the run's time and the medal it earns —
        // in Zen the time shows but no medal is earned or banked
        char buf[96]; char *q=buf;
        q=sappend(q, "TIME "); q=sapp_time_ds(q, S->plat_show_tds);
        if (!G.zen_run && S->plat_show_tier){ q=sappend(q, " - "); q=sappend(q, medal_name(S->plat_show_tier)); q=sappend(q, " MEDAL!"); if (S->plat_show_newbest) q=sappend(q, "  NEW BEST!"); }
        else if (!G.zen_run && S->plat_lvl<=16){ q=sappend(q, " - NO MEDAL (BRONZE "); q=sapp_time_ds(q, PLAT_MEDAL[S->plat_lvl-1][2]); q=sappend(q, ")"); }
        else if (G.zen_run) q=sappend(q, " - ZEN: NO MEDALS");
        *q=0;
        draw_text_center(vx+vw2/2, vy+vh2/3 + 22, buf, 1, (!G.zen_run && S->plat_show_tier)? medal_col(S->plat_show_tier) : COL(220,224,230));
    }
    if (S->plat_sel){
        // MEDAL TIMES (feat 3): the level select panel, over the
        // (loaded, frozen) frontier level
        rect_fill(vx+vw2/2-190, vy+vh2/2-64, 380, 128, COL(12,14,22));
        rect_line(vx+vw2/2-190, vy+vh2/2-64, 380, 128, COL(250,200,50));
        draw_text_center(vx+vw2/2, vy+vh2/2-52, "SUPER BUDDY RUN", 2, COL(250,200,50));
        draw_text_center(vx+vw2/2, vy+vh2/2-30, "PICK YOUR LEVEL", 1, COL(255,255,255));
        { char buf[64]; char *q=buf;
          q=sappend(q, "<  LEVEL "); q=sapp_int(q, S->plat_pick); q=sappend(q, "  >"); *q=0;
          draw_text_center(vx+vw2/2, vy+vh2/2-10, buf, 2, COL(255,240,200)); }
        draw_text_center(vx+vw2/2, vy+vh2/2+10, REGION_NAME[(S->plat_pick-1)%8], 1, COL(170,200,190));
        { char buf[96]; char *q=buf; int pk = S->plat_pick;
          if (pk<=16 && b->plat_tbest[pk-1]){ q=sappend(q, "BEST "); q=sapp_time_ds(q, b->plat_tbest[pk-1]); q=sappend(q, " - "); q=sappend(q, medal_name(medal_tier(b->plat_tbest[pk-1], PLAT_MEDAL[pk-1]))); q=sappend(q, " MEDAL"); }
          else if (pk<=16){ q=sappend(q, "NO TIME YET - GOLD IS "); q=sapp_time_ds(q, PLAT_MEDAL[pk-1][0]); }
          else q=sappend(q, "BEYOND THE MEDAL BOOK (LEVEL 16+)");
          if (pk == b->plat_done+1) q=sappend(q, "   -   NEW!");
          *q=0;
          draw_text_center(vx+vw2/2, vy+vh2/2+28, buf, 1, (pk<=16 && b->plat_tbest[pk-1])? medal_col(medal_tier(b->plat_tbest[pk-1], PLAT_MEDAL[pk-1])) : COL(200,214,200)); }
        { char b[80]; char *q=b; q=sappend(q, "LEFT / RIGHT - LEVEL      "); q=sappend(q, lbla(H_FIRE)); q=sappend(q, " - RUN IT"); *q=0;
          draw_text_center(vx+vw2/2, vy+vh2/2+48, b, 1, COL(200,230,200)); }
    }
    // (no standing hint line here any more — controls live in the
    // Start-menu's CONTROLS panel; Daniel, Nov 6)
    if (S->note_t>0){
        rect_fill(vx, vy+vh2-22, vw2, 22, COL(14,14,20));
        draw_text_center(vx+vw2/2, vy+vh2-15, S->note, 1, COL(255,240,200));
    }
}

