// THE SURVIVAL GAME — war/hive.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 451-613 in the Nov 2026 fine-grained refactor; code unchanged.

// --- the hive economy (v1.21) ---------------------------------------
// A bug harvester's home is the NEAREST living hive, read fresh each
// time it matters: the unit snapshot carries no spare field for a
// fixed assignment, and a worker whose hive falls re-homes to the
// next one instead of idling. Hives stand banded apart, so the
// nearest hive is the one it hatched from in every normal fight.
static int war_home_hive(BwSim *S, float x, float y){
    int best=-1; float bd=1e18f;
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0 || S->war_stype[s]!=WAR_S_HIVE) continue;
        float dx=S->war_sx[s]-x, dy=S->war_sy[s]-y, d2=dx*dx+dy*dy;
        if (d2<bd){ bd=d2; best=s; }
    }
    return best;
}
static int war_spawn_bharv(BwSim *S, int hive){
    if (war_count_units(S, 1) >= 40) return -1;
    float a = 2.6f + (float)(war_count_units(S, 1) % 3) * 1.1f;
    return war_spawn_unit(S, WAR_U_BHARV, S->war_sx[hive] + fcos(a)*(WAR_SRAD[WAR_S_HIVE]+16),
                                          S->war_sy[hive] + fsin(a)*(WAR_SRAD[WAR_S_HIVE]+16));
}
static int war_hive_bharv(BwSim *S, int hive){   // this hive's workforce: bug harvesters whose home it is
    int n=0;
    for (int i=0;i<WAR_MAXU;i++)
        if (S->war_uhp[i]>0 && S->war_utype[i]==WAR_U_BHARV && war_home_hive(S, S->war_ux[i], S->war_uy[i])==hive) n++;
    return n;
}
static int war_hive_guards(BwSim *S, int hive){   // combat bugs on guard duty around this hive (the old drip's own metric: state 0, within 130)
    int n=0;
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0 || war_u_side(S->war_utype[i])!=1 || S->war_utype[i]==WAR_U_BHARV || S->war_ustate[i]!=0) continue;
        float dx=S->war_ux[i]-S->war_sx[hive], dy=S->war_uy[i]-S->war_sy[hive];
        if (dx*dx+dy*dy < 130.0f*130.0f) n++;
    }
    return n;
}
static int war_hive_start_bits(BwSim *S){   // a fresh hive's purse: 150 on campaign; a skirmish scales with its difficulty
    return S->war_skirmish? (S->war_diff==1? 100 : S->war_diff==3? 250 : 150) : 150;
}
static void war_win(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    S->war_state = 2; S->war_endt = 3.2f; S->war_pause = 0;
    if (S->war_skirmish){
        // v1.19: a skirmish is ad-hoc — no progress, and NEVER a
        // snapshot clear: a campaign battle may be parked under
        // this same region-equivalent number, and it is not this
        // fight's to discard. The win pays buddy XP by difficulty
        // and saves (the award has to persist like a campaign's).
        // v1.29: the SKIRMISH park is this fight's own to discard —
        // a decided battle leaves nothing to resume, cleared BEFORE
        // the save below so the file agrees.
        war_snap_sk_clear(pi);
        static const int SKXP[4] = { 0, 30, 50, 80 };   // by war_diff: EASY / NORMAL / HARD
        int amt = SKXP[S->war_diff>=1 && S->war_diff<=3? S->war_diff : 2];
        buddy_award_xp(pi, amt);
        if (S->war_duo>=0) buddy_award_xp(S->war_duo, amt);   // feat 13: a co-commander fought it too — the XP pays both (progress stays the owner's)
        do_save();
        if (b->active){ char buf[72]; char *q=buf; q=sappend(q, "SKIRMISH WON! +"); q=sapp_int(q, amt); q=sappend(q, " BUDDY XP"); *q=0; war_note(S, buf); }
        else war_note(S, "SKIRMISH WON!");
    } else {
        if (!war_snap_on[pi] || war_snap_reg[pi]==S->war_region) war_snap_clear(pi);   // the battle is decided: ITS snapshot does not survive it (cleared BEFORE the save below) — but a battle parked in ANOTHER region is not this fight's to discard (v1.18)
        // v1.19: a campaign win pays the buddy — a first conquest
        // pays 60 + 10 per region, a replay of a taken region 25.
        int first = (S->war_region == b->war_prog);
        int amt = first? 60 + 10*S->war_region : 25;
        buddy_award_xp(pi, amt);
        if (S->war_duo>=0) buddy_award_xp(S->war_duo, amt);   // feat 13: a co-commander fought it too — the XP pays both (war_prog stays the owner's)
        if (first && b->war_prog < 8) b->war_prog++;
        do_save();   // the award (and any advance) persists with the battle's own save
        if (b->active){ char buf[72]; char *q=buf; q=sappend(q, "REGION CONQUERED! +"); q=sapp_int(q, amt); q=sappend(q, " BUDDY XP"); *q=0; war_note(S, buf); }
        else war_note(S, "REGION CONQUERED!");
    }
    qnote(0, 0, 60, 80, 6); qnote(0.12f, 0, 64, 80, 6); qnote(0.24f, 0, 67, 80, 6); qnote(0.36f, 0, 72, 90, 10);
}
static void war_lose(int pi){
    BwSim *S = &BWS[pi];
    S->war_state = 3; S->war_endt = 3.2f; S->war_pause = 0;
    if (S->war_skirmish){
        war_snap_sk_clear(pi);   // v1.29: a decided skirmish leaves nothing to resume (its own park is this fight's to discard; the campaign park still stands)
        war_note(S, "SKIRMISH LOST.");   // v1.19: no campaign snapshot clear, no progress
    } else {
        if (!war_snap_on[pi] || war_snap_reg[pi]==S->war_region) war_snap_clear(pi);   // a fallen Core leaves nothing of THIS battle to resume — a park in another region still stands (v1.18)
        war_note(S, "THE COMPILER CORE HAS FALLEN!");
    }
    qnote(0, 0, 48, 80, 8); qnote(0.2f, 0, 43, 80, 8); qnote(0.4f, 0, 36, 90, 12);
}
static void war_hurt_unit(int pi, int i, int dmg){
    BwSim *S = &BWS[pi];
    if (!war_unit_alive(S, i)) return;
    S->war_uflash[i] = 0.12f;   // cosmetic: the hit flash (draw only)
    S->war_uhp[i] -= dmg;
    if (S->war_uhp[i] <= 0){
        war_fx_spawn(S, S->war_ux[i], S->war_uy[i], 0);   // cosmetic: a death poof where it fell
        S->war_uhp[i] = 0; S->war_usel[i] = 0; S->war_utarget[i] = 0; S->war_wpn[i] = 0;   // the dead drop their paths
        if (S->war_utype[i]==WAR_U_HARV && war_count_type(S, WAR_U_HARV)==0
            && S->war_bits < WAR_TRAIN_COST[0] && S->war_state==1){
            // never soft-lock the economy: the Core prints one
            // harvester's worth of bits, once, in an emergency
            S->war_bits = WAR_TRAIN_COST[0];
            war_note(S, "EMERGENCY BIT GRANT: TRAIN A NEW HARVESTER!");
        }
    }
}
static void war_hurt_struct(int pi, int i, int dmg){
    BwSim *S = &BWS[pi];
    if (!war_struct_alive(S, i)) return;
    S->war_sflash[i] = 0.12f;   // cosmetic: the hit flash (draw only)
    S->war_shp[i] -= dmg;
    if (S->war_shp[i] <= 0){
        war_fx_spawn(S, S->war_sx[i], S->war_sy[i], 2);   // cosmetic: a bigger poof for a building
        S->war_shp[i] = 0; S->war_qlen[i] = 0;
        if (S->war_selstruct == i) S->war_selstruct = -1;
        if (S->war_state == 1 && S->war_stype[i]==WAR_S_HQ) war_lose(pi);
        else if (S->war_state == 1 && S->war_stype[i]==WAR_S_HIVE && war_hives_alive(S)==0) war_win(pi);
    }
}
// debris damage (Daniel, Nov 16): terrain features take plain damage
// from your combat units — never the siege gun's structure bonus —
// and at 0 hp they are gone for the rest of the battle: movement
// frees up at once and all that remains is a scorch mark that fades.
// v1.17: felling debris with PLAYER-side damage (side 0) also pays a
// salvage bounty by kind (WAR_TBITS) — silent income, no note spam.
// Bug damage (side 1) pays nothing.
static void war_hurt_terrain(int pi, int t, int dmg, int side){
    BwSim *S = &BWS[pi];
    if (t<0 || t>=S->war_tn || S->war_thp[t]<=0) return;
    if (S->war_ttype[t]==5) return;   // v1.18: barrier slabs are truly indestructible — collision and the A* grid key off thp>0, so this hp must never reach 0
    S->war_thp[t] -= (float)dmg;
    if (S->war_thp[t] <= 0){
        S->war_thp[t] = 0;
        S->war_tdeadt[t] = 6.0f;
        if (side==0) S->war_bits += WAR_TBITS[S->war_ttype[t]];
        if (pi==G.my_id) sfx(17);   // the wall-crunch: debris gives way
    }
}
static void war_fire(BwSim *S, int side, float x, float y, int target_ref, int dmg){
    for (int i=0;i<WAR_MAXP;i++) if (S->war_pt[i]<=0){
        S->war_px[i]=x; S->war_py[i]=y; S->war_ptar[i]=target_ref;
        S->war_pdmg[i]=dmg; S->war_pside[i]=side; S->war_pt[i]=2.0f;
        return;
    }
}
static int war_nearest_enemy(BwSim *S, int side, float x, float y, float maxd, int want_units, int want_structs){
    int best = 0; float bd = maxd*maxd;
    if (want_units)
        for (int i=0;i<WAR_MAXU;i++){
            if (S->war_uhp[i]<=0 || war_u_side(S->war_utype[i])==side) continue;
            float dx=S->war_ux[i]-x, dy=S->war_uy[i]-y, d2=dx*dx+dy*dy;
            if (d2<bd){ bd=d2; best=i+1; }
        }
    if (want_structs)
        for (int i=0;i<WAR_MAXS;i++){
            if (S->war_shp[i]<=0 || S->war_sside[i]==side) continue;
            float dx=S->war_sx[i]-x, dy=S->war_sy[i]-y, d2=dx*dx+dy*dy;
            if (d2<bd){ bd=d2; best=-(i+1); }
        }
    return best;
}
static float war_move_step(BwSim *S, int i, float tx, float ty, float stopd, float dt){
    float dx=tx-S->war_ux[i], dy=ty-S->war_uy[i];
    float d=fsqrt(dx*dx+dy*dy);
    if (d>stopd && d>0.01f){
        float step = war_u_speed(S->war_utype[i], S->war_region) * dt;
        if (step > d-stopd) step = d-stopd;
        S->war_ux[i] += dx/d*step; S->war_uy[i] += dy/d*step;
    }
    return d;
}
