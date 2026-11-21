// THE SURVIVAL GAME — enemies/bugs.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 1178-1226 in the Nov 2026 fine-grained refactor; code unchanged.

// Bugs — stats scale with distance from the Epicenter, per the design:
// further out = bigger max size, more bugs, and the small ones get faster.
// ---------------------------------------------------------------------------
static float bug_radius(float w){ return 7.0f + fsqrt(w)*2.3f; }
// max bug weight at distance d from the Epicenter. The curve stays nearly
// flat for a long way out (pocket-change roaches) and only rears up in the
// far country — you have to travel to find the big ones.
// --- player power and world threat -----------------------------------
// Your capability score: best-weapon DPS plus effective health. A fresh
// DEV scores ~152, and world THREAT is your score relative to that — so
// it starts at x1.0 and climbs as you upgrade, feeding you bigger, faster,
// more numerous bugs even on the Epicenter's doorstep.
static float player_dps(Player *p){
    float best = 0;
    for (int w=0;w<NWEAP;w++) if (p->owned[w]){
        float d;
        if (w==4) d = (8.0f/0.07f) * p->dmg_mul
                    * ((BEAM_EXT+BEAM_SPIN)/(BEAM_EXT+BEAM_SPIN+BEAM_RET+BEAM_REST)); // beam dps x sweep duty cycle
        else d = weap_dmg(w) * (float)p->owned[w] / weap_cd(w) * p->dmg_mul * p->rate_mul;
        if (d>best) best=d;
    }
    return best;
}
static float player_power(Player *p){
    return player_dps(p) + (float)p->maxhp + (float)p->shield_max*2.0f;
}
static float threat_override = -1;   // debug/test hook only; -1 = use the real threat
// THREAT RATIO (Daniel, Nov 9): "it doesn't need to tip more in the
// user's favor, just change the general ratio." The world's answer
// to your power is scaled by this one constant: the curve keeps its
// shape at every upgrade level, but only 3/4 of the excess over a
// fresh DEV's x1.0 ever arrives. A new run is untouched (power 152
// still means threat x1.0), and every threat channel downstream —
// bug size, the far-country inflation, the crowd a powerful player
// draws — reads G.threat, so the whole composite eases by the same
// proportion everywhere.
// LIVE TUNING (v1.26 Item 3): was a #define; its only use is the float
// multiply in compute_threat below (no constant-expression contexts —
// audited), so it is a variable now and the registry points at it.
static float THREAT_RATIO = 0.75f;
static void compute_threat(void){
    float pw = 152.0f;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        float v = player_power(&G.p[i]); if (v>pw) pw=v;
    }
    G.threat = 1.0f + (pw/152.0f - 1.0f) * THREAT_RATIO; if (G.threat<1.0f) G.threat=1.0f;
    if (threat_override>=0) G.threat = threat_override;
}
// hauling a downed partner back up costs carcasses off YOUR hat,
// priced off the DOWNED player's capability (Daniel): a fresh
// sidekick is cheap to save, a stacked one takes a real haul. Half health and a grace ring, like
// the Rollback Token, and the swarm on them gets shoved back.
