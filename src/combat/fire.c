// THE SURVIVAL GAME — combat/fire.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 517-657 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// COMFORT (v1.25 feature 2): screen shake. Big hits raise G.shake
// through this one gate — with the player's shake toggle off, the
// value never rises and nothing anywhere shakes. render_world turns
// a live value into a camera jitter (see shk_off_x below).
// ---------------------------------------------------------------------------
static void fx_shake(float v){ if (G.shake_on && v>G.shake) G.shake=v; }

static void player_fire(Player *p, int pi, float dt){
    if (p->in_mall){ p->fire_cd = 0; return; }   // house rules: no shooting in the mall
    p->fire_cd -= dt;
    if (!(p->cur_keys & K_FIRE)) return;
    int w = p->weapon;
    if (!p->owned[w]) return;
    float mx = p->x + fcos(p->aim)*16, my = p->y + fsin(p->aim)*16;
    float dm = p->dmg_mul, rm = p->rate_mul;
    if (w==4){ // THE DEBUGGER: one pull = one sweep. All your beams
        // extend together, spin one revolution together, then close
        // together (the sweep itself lives in update_beam). Stacked
        // copies don't widen one beam — they add beams to the ring:
        // 2 = front + back, 3 = every 120°, and so on, each one also
        // reaching a little farther.
        if (p->fire_cd<=0 && beam_phase[pi]==0){
            int copies = p->owned[4]; if (copies<1) copies=1;
            beam_n[pi] = copies>MAXBEAMS? MAXBEAMS : copies;
            beam_len[pi] = 105 + 10*(copies-1);
            beam_base[pi] = p->aim;
            beam_phase[pi] = 1; beam_t[pi] = 0; beam_tick[pi] = 0;
            beam_frac[pi] = 0;
            p->fire_cd = (BEAM_EXT + BEAM_SPIN + BEAM_RET + BEAM_REST)/rm;
        }
        return;
    }
    float cd = (w==0?0.115f : w==1?0.38f : w==2?0.30f : 1.05f);
    if (p->fire_cd>0) return;
    p->fire_cd = cd/rm;
    float spread = (w==0?0.055f : w==1?0.03f : w==2?0.045f : 0.02f);
    float dmg = (w==0?6.0f : w==1?24.0f : w==2?15.0f : 65.0f) * dm;
    // stacked copies fan out: one copy fires straight ahead, two fire
    // at two angles, three at three — a row of shots across your front.
    // Stacks are uncapped; the fan itself tops out at 40 projectiles
    // per volley (the projectile pool is only so deep).
    int n = p->owned[w]; if (n<1) n=1; if (n>40) n=40;
    for (int i=0;i<n;i++){
        float off = ((float)i - (float)(n-1)/2.0f) * 0.11f;
        float ang = p->aim + off + rng_range(-spread, spread);
        fire_proj(pi, w, mx, my, ang, dmg, -1);
    }
    sfx(w==0?1 : w==1?2 : w==2?3 : 4);
    if (w==3) fx_shake(2.0f);
}

// one damage tick of every live beam: same cone test the held beam used
// (nearest bugs inside reach and ±0.55 rad of the beam's current angle
// get breakpoint-frozen and burned)
static void beam_hit_tick(int pi){
    Player *p=&G.p[pi];
    int nb = beam_n[pi]; if (nb<1) nb=1;
    float bl = beam_len[pi] * beam_frac[pi];
    float dm = p->dmg_mul;
    for (int k=0;k<nb;k++){
        float ba = beam_ang[pi][k];
        for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
            float dx=G.bugs[i].x-p->x, dy=G.bugs[i].y-p->y;
            float d2=dx*dx+dy*dy;
            if (d2 < bl*bl){
                float da = wrap_pi(fatan2(dy,dx) - ba);
                if (ffabs(da) < 0.55f){
                    G.bugs[i].bp_t = 0.35f;          // BREAKPOINT: frozen
                    hurt_bug(i, 8.0f*dm, pi, fcos(ba)*30, fsin(ba)*30);
                }
            }
        }
    }
    sfx(14);
}

// advance one player's Debugger sweep: extend (beams grow, damage live),
// spin (one full revolution, damage live), retract (beams close, no
// damage), done. Writes the render state (angles, reach fraction, vis)
// every frame; the ring rotates as a whole from the captured aim.
static void update_beam(int pi, float dt){
    if (beam_phase[pi]==0){ beam_frac[pi]=1; return; }
    Player *p=&G.p[pi];
    if (!p->alive || !p->owned[4]){ beam_phase[pi]=0; beam_vis[pi]=0; beam_frac[pi]=1; return; }
    beam_t[pi]+=dt;
    int ph = beam_phase[pi];
    float dur = ph==1? BEAM_EXT : ph==2? BEAM_SPIN : BEAM_RET;
    if (beam_t[pi]>=dur){
        beam_t[pi]=0;
        beam_phase[pi] = ph==3? 0 : ph+1;
        if (beam_phase[pi]==0){ beam_vis[pi]=0; beam_frac[pi]=1; return; }
        ph = beam_phase[pi];
        dur = ph==1? BEAM_EXT : ph==2? BEAM_SPIN : BEAM_RET;
    }
    float f = beam_t[pi]/dur;
    float rot = 0;
    if (ph==1) beam_frac[pi]=f;
    else if (ph==2){ beam_frac[pi]=1; rot = TAU*f; }
    else { beam_frac[pi]=1-f; rot = TAU; }
    int nb = beam_n[pi]; if (nb<1) nb=1;
    for (int k=0;k<nb;k++) beam_ang[pi][k] = beam_base[pi] + rot + TAU*(float)k/(float)nb;
    beam_vis[pi]=0.09f;
    if (ph==1 || ph==2){
        beam_tick[pi]+=dt;
        while (beam_tick[pi]>=0.07f){ beam_tick[pi]-=0.07f; beam_hit_tick(pi); }
    }
}

// THE BACKFIRE BEAN (Krista's idea, Daniel's build, Nov 5): a small
// green fellow with goggles and a jetpack he fuels entirely himself —
// beans are famous for exactly one thing, and he has weaponized it,
// tastefully, in little green puffs. Squeeze him (key 6, or tap BEAN
// in the status card) anywhere out in the wild and he flies you —
// and your hat, and your buddy — straight home to the Epicenter.
// One bean, one ride. He regrets nothing.
//
// INSIDE the epicenter there is no home to fly to, so the bean does
// the other thing he knows (Daniel, Nov 10): he becomes a JETPACK.
// You fly where you steer, over props, walls and swarms alike, for
// as long as you keep your nerve — the ride ends the moment you
// FIRE, or the moment anything lands ONE HIT on you, and he drops
// you right where you are. A launch pad for the far country.
#define BEAN_EPI_R 280.0f
static void bean_use(int pi){
    Player *p=&G.p[pi];
    if (G.state!=GS_PLAY || p->beans<=0 || p->bean_t>0 || p->bean_jet>0 || !p->alive) return;
    if (p->in_mall || p->station>=0 || G.bud[pi].in_bw) return;
    float dx=p->x, dy=p->y-70;
    if (dx*dx+dy*dy < BEAN_EPI_R*BEAN_EPI_R){
        p->beans--; p->bean_jet=1; p->jet_fall=0;
        spawn_floater(p->x, p->y-30, "JETPACK AWAY!", COL(150,220,120));
        sfx(23);
        if (pi==G.my_id) do_save();
        return;
    }
    p->beans--; p->bean_t=1.6f;
    spawn_floater(p->x, p->y-30, "BEAN POWER!", COL(150,220,120));
    sfx(23);
    if (pi==G.my_id) do_save();
}
