// THE SURVIVAL GAME — music/pieces.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from music.c lines 456-497 in the Nov 2026 fine-grained refactor; code unchanged.

static int bw_piece(int mode){
    switch (mode){
        case 2: return 5;    // ENTRANCES — In the Hall of the Mountain King
        case 3: return 6;    // SUPER BUDDY RUN — Flight of the Bumblebee
        case 5: return 7;    // TANK RESCUE — Infernal Galop (the Can-Can)
        case 6: return 8;    // CASTLE — Greensleeves
        case 7: return 9;    // SUPER BUDDY CART — William Tell finale
        case 8: return 10;   // WAR! — Ride of the Valkyries
        default: return 4;   // CRAFT + everywhere else in Banzai World — Sheep
    }
}
// (threat_override is declared up by compute_threat, which honors it too)
// Daniel's music rule: Beethoven, calm first, fiercer as THREAT climbs.
// The menu always gets the Moonlight; a zen run never leaves it.
static int mus_piece(void){
    if (G.state==GS_MENU || G.state==GS_HELP || G.state==GS_SETTINGS || G.state==GS_SLOTS || G.state==GS_PAUSE) return 0;
    // Banzai World has its own music (Daniel, Nov 5): Bach's Sheep May
    // Safely Graze, pastoral for a pastoral place — it wins over the
    // threat ladder and even over a zen run, but menus still open with
    // the Moonlight. Since v1.16 each arcade cabinet plays its own
    // piece instead (see bw_piece); Sheep keeps the plaza and Craft.
    if (G.state==GS_PLAY && G.bud[G.my_id].in_bw) return bw_piece(G.bud[G.my_id].bw_mode);
    if (G.state==GS_PLAY && G.p2_slot>=0 && G.bud[G.p2_slot].in_bw) return bw_piece(G.bud[G.p2_slot].bw_mode);
    if (G.zen_run) return 0;
    float t = threat_override>=0? threat_override : G.threat;
    if (t<2.0f) return 0;
    if (t<3.5f) return 1;
    if (t<6.0f) return 2;
    return 3;
}
// ---------------------------------------------------------------------------
// ADAPTIVE MUSIC INTENSITY (v1.25 feature 15): the v1.23 arrangements
// breathe. Two levels only — SPARSE (the lead alone) and FULL (the
// arrangement exactly as it has played since v1.23) — chosen by a
// danger meter and latched at bar boundaries (every 16 steps; every
// piece length is a multiple of 16, so bars align across the ladder's
// piece switches too). The gate is the bass (ch 1) and accompaniment
// (ch 2) channels: far from danger they rest; they fill in as bug
// density or an active chase rises. Comfort (feat 2) stays sovereign
// — intensity only decides WHICH notes are offered to mus_note,
// never their velocity. Music reads the game; the game never reads
// music. Nothing here is saved. Spec: v125-feat15-spec.md.
