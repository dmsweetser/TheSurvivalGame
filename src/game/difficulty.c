// THE SURVIVAL GAME — game/difficulty.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 1272-1435 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// Difficulty — EASY is the game exactly as it has always played; every
// step up only makes it harder (Daniel's rule). Picked on the title
// screen, locked into the run at start (diff_run), saved with the run.
// ---------------------------------------------------------------------------
static const char *DIFF_NAME[4] = { "EASY", "MEDIUM", "HARD", "BUGAGGEDON" };
// LIVE TUNING (v1.26 Item 3): mutable — the registry in data/livedata.c
// points here; the literals are the stock defaults.
static float diff_hp[4]  = { 1.0f, 1.35f, 1.8f, 2.6f };
static float diff_dmg[4] = { 1.0f, 1.25f, 1.6f, 2.2f };
static float diff_spd[4] = { 1.0f, 1.08f, 1.16f, 1.25f };
static float diff_pop[4] = { 1.0f, 1.3f, 1.7f, 2.4f };
static float diff_rate[4]= { 1.0f, 1.25f, 1.6f, 2.2f };
static float diff_w[4]   = { 1.0f, 1.15f, 1.3f, 1.6f };
static float d_hp(void){ return diff_hp[G.diff_run&3]; }
static float d_dmg(void){ return diff_dmg[G.diff_run&3]; }
static float d_spd(void){ return diff_spd[G.diff_run&3]; }
static float d_pop(void){ return diff_pop[G.diff_run&3]; }
static float d_rate(void){ return diff_rate[G.diff_run&3]; }
static float d_w(void){ return diff_w[G.diff_run&3]; }

static float bug_max_weight(float d){
    float t = d/3000.0f; if (t>1.3f) t=1.3f;
    float m = 2.0f + 133.0f * t*t*(0.35f + 0.65f*t);
    float cap = 150.0f * (1.0f + (G.threat-1.0f)*(0.35f/3.0f));   // threat lifts the ceiling (0.6 -> 0.35, Nov 2; slope /3, Nov 18 size stretch)
    return m>cap? cap : m;
}
static void spawn_bug(float x, float y){
    for (int i=0;i<MAXBUGS;i++) if (!G.bugs[i].alive){
        Bug *b=&G.bugs[i];
        float d = fsqrt(x*x + y*y);
        // THREAT: an upgraded player makes the whole map act farther-out
        // than it is — the bugs near home grow teeth too. Retuned Nov 2
        // (Daniel: pacing too aggressive, too many big bugs): the
        // inflation is 1100 -> 650 and the weight roll below is
        // quartic, so high THREAT mostly means MORE small bugs, not a
        // screen full of giants on the doorstep.
        float de = d + (G.threat-1.0f)*650.0f;
        float maxW = bug_max_weight(de) * d_w();
        float r1 = rng_f();
        float w = 0.6f + r1*r1*r1*r1*maxW;               // quartic skew: mostly tiny, giants rare
        // SIZE scales with THREAT, indefinitely (Daniel): the stronger
        // you get, the bigger everything that crawls out gets — weight,
        // radius, HP and bite all ride on w, and there is no ceiling.
        // The camera gives ground in steps to compensate (zoom_target_of).
        // Nov 18 (Daniel): SIZE takes the scenic route — this slope is
        // the Nov 5 slope /3 (0.30 -> 0.10; the bug_max_weight ceiling
        // lift is /3 to match), so the giants still arrive, just three
        // times later. Still linear, still no cap; every other scaling
        // term (population, spawn rate, distance teeth) is untouched.
        w *= 1.0f + (G.threat-1.0f)*0.10f;
        b->alive=1; b->kind=(int)(rng_f()*4); b->aggro=-1; b->target_wall=-1; b->btarget=-1;
        // regional strains (Nov 9): each far country has a dominant
        // kind — rustbacks in the Badlands, frostbites in the Cache
        // Fields, neons in the RAM Flats, mossbacks in the Jungle.
        // Half the locals wear the regional colors; the rest stay a
        // fair mix. This flavors the spawn AFTER all the threat and
        // distance scaling above, so it composes with that curve
        // instead of bypassing it.
        { int rg = region_at(d);
          if (REGION_BUG[rg]>=0 && rng_f()<0.5f) b->kind = REGION_BUG[rg]; }
        b->x=x; b->y=y; b->vx=0; b->vy=0; b->w=w;
        b->maxhp = b->hp = (4.0f + w*2.4f) * d_hp();
        // speed: near the Epicenter everything crawls, big or small. Far
        // out the whole swarm speeds up — and the little ones most of all.
        float dfac = de/2600.0f; if (dfac>1) dfac=1;
        float smallness = 1.0f - (w/60.0f>1?1:w/60.0f);
        b->speed = (20.0f + 38.0f*dfac + 34.0f*dfac*smallness) * rng_range(0.85f,1.15f) * d_spd();
        b->dmg = (1.5f + w*0.10f) * d_dmg();
        // the Depths keep ancients: a rare few that were old when the
        // codebase was young, and noticeably heavier for it
        if (region_at(d)==NREGIONS-1 && rng_f()<0.08f){
            b->w *= 1.35f;
            b->maxhp = b->hp = (4.0f + b->w*2.4f) * d_hp();
            b->dmg = (1.5f + b->w*0.10f) * d_dmg();
        }
        if (G.zen_run){   // zen mode: every bug gigantic, every bug slow
            b->w = w*8.0f + 6.0f;
            b->maxhp = b->hp = (4.0f + b->w*2.4f) * d_hp();
            b->dmg = (1.5f + b->w*0.10f) * d_dmg();
            b->speed *= 0.30f;
        }
        // NAMED ELITE (v1.25 feat 8): if the bounty roll (or the test
        // seam) marked this spawn, it crawls out as the posted
        // bounty's elite. The elevation gate lives here so the
        // spawner and the seam obey one rule: never for a LAN client
        // (the host's bugs are the only real ones), never during the
        // post-claim cooldown, never while another elite lives (one
        // name at a time). It lands AFTER every flavour above — the
        // distance curve, the threat multiplier on Daniel's /3 size
        // stretch, the Depths ancients, Zen's x8 — and multiplies
        // what they produced, so it composes with all of them and
        // bypasses none. HP and bite are recomputed from the final
        // weight by the standard formulas: an elite is exactly a bug
        // of its weight in every system that reads weight.
        if (bn_force){
            bn_force = 0;
            if (G.net_mode!=NET_CLIENT && G.bn_cool<=0 && bn_any_live()<0){
                b->elite = bn_name_idx(G.bn_serial) + 1;
                b->kind = bn_kind_of(G.bn_serial);
                b->w *= 3.0f;
                if (b->w > BN_WCAP) b->w = BN_WCAP;   // a fight, not a wall
                b->maxhp = b->hp = (4.0f + b->w*2.4f) * d_hp();
                b->dmg = (1.5f + b->w*0.10f) * d_dmg();
                b->speed *= 0.8f;                     // it is heavy; it lumbers
                sfx(24);   // feat 15: THE BOUNTY STIRS — the elite announces itself, off-screen and all
            }
        }
        b->leg_ph = rng_f()*TAU; b->wobble = rng_f()*TAU;
        b->hit_t=0; b->bp_t=0; b->target_wall=-1; b->chew_t=0;
        // the seed is a trig PHASE (pupil drift, Eddie's fuzz), not a
        // number to keep: a full-range u32 (up to 4.3e9) blows past
        // wrap_pi's float range, the Taylor sin explodes into the
        // thousands, and the bug's pupils land half a screen away
        // (Daniel, Nov 6). 0..6282 milli-radians is plenty of variety.
        b->seed = rng_u32()%6283;
        return;
    }
}
static void spawner(float dt){
    compute_threat();
    G.spawn_t -= dt;
    if (G.spawn_t>0) return;
    int alive=0; for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive) alive++;
    float far=0; int npl=0;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].alive && !G.p[i].in_mall){
        npl++;
        float d=fsqrt(G.p[i].x*G.p[i].x + G.p[i].y*G.p[i].y); if (d>far) far=d;
    }
    // population target: a sparse trickle near home, a swarm far out.
    int time_term = (int)(G.time*0.15f); if (time_term>10) time_term=10;
    int target = 6 + (int)(far*0.017f) + time_term + 3*(npl>0? npl-1:0);
    target = (int)(target * (1.0f + (G.threat-1.0f)*0.18f));   // power draws a crowd (0.30 -> 0.18, Nov 2)
    target = (int)(target * d_pop());                          // difficulty draws a bigger one
    if (target>200) target=200;
    if (G.zen_run && target>8) target = 8 + (target-8)/2;   // giants need elbow room
    G.spawn_t = (far<600? 1.0f : 0.55f) / (1.0f + (G.threat-1.0f)*0.08f) / d_rate();   // slow drip at home
    if (alive>=target || npl==0) return;
    // spawn near a random connected player, just off-screen. Anywhere is
    // fair game now — including the Epicenter plaza itself.
    int pi=-1, tries=0;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && !G.p[i].in_mall) pi=i;
    if (pi<0) return;
    pi = (int)(rng_f()*(pi+1)); if (!G.p[pi].connected) pi=0;
    // NAMED ELITE roll (v1.25 feat 8): the posted bounty's bug crawls
    // out through the ordinary spawner — one roll per tick, only
    // while no elite is live, the cooldown has run out, and the run
    // is 90 seconds old (a brand-new player never meets a named bug
    // before their first hat). More likely far from home: the board
    // rewards expeditions. A won roll just marks the next spawn;
    // spawn_bug's gate has the final say.
    if (G.net_mode!=NET_CLIENT && G.bn_cool<=0 && G.time>=90.0f && bn_any_live()<0){
        float ch = far<600.0f? 0.01f : far<2600.0f? 0.02f : 0.04f;
        if (rng_f()<ch) bn_force = 1;
    }
    while (tries<6){
        tries++;
        float a = rng_f()*TAU, rad = rng_range(620.0f, 860.0f);
        float x = G.p[pi].x + fcos(a)*rad, y = G.p[pi].y + fsin(a)*rad;
        if (x>-110 && x<110 && y>-135 && y<-25) continue; // not inside the hut
        spawn_bug(x, y);
        if (far>2000 && alive<target-25) spawn_bug(x + rng_range(-60,60), y + rng_range(-60,60));
        return;
    }
}

