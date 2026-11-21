// THE SURVIVAL GAME — modes/run/run.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from run.c lines 164-597 in the Nov 2026 fine-grained refactor; code unchanged.

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
              if (S->plat_lvl<=16 && b->plat_tbest[S->plat_lvl-1]){ q=sappend(q, "BEST "); q=sapp_time_ds(q, b->plat_tbest[S->plat_lvl-1]); q=sappend(q, " - "); q=sappend(q, medal_name(medal_tier(b->plat_tbest[S->plat_lvl-1], plat_medal[S->plat_lvl-1]))); q=sappend(q, ". "); }
              else if (S->plat_lvl<=16){ q=sappend(q, "NO TIME YET - GOLD IS "); q=sapp_time_ds(q, plat_medal[S->plat_lvl-1][0]); q=sappend(q, ". "); }
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
              S->plat_show_tier = (S->plat_lvl<=16)? medal_tier(tds, plat_medal[S->plat_lvl-1]) : 0;
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
