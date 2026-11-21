// THE SURVIVAL GAME — war/units.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 1-188 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// WAR! (bw_mode 8, Nov 15 2026, Daniel): a tap-first RTS in the Arcade.
// Buddies vs the bug swarm, one region at a time: a world map of the
// eight regions radiating from the epicenter; win a region's battle
// (harvest BITS, raise an army, raze every HIVE) and the next region
// unlocks. Progress lives on the Buddy (war_prog, saved as wr); a
// battle itself is per-trip scratch state in the BwSim.
//
// Controls are Daniel's spec, tap-first: tap a unit to select it,
// double-tap to select every unit of its type on screen, tap ground
// to move, tap an enemy to attack it, tap a structure for its build
// bar along the bottom (a reserved strip — the camera view sits
// above it, the way RUN's deck keeps the joystick off the field).
// Drags pan the camera (DRAG scheme / mouse), arrow keys pan too.
// ---------------------------------------------------------------------------
#define WAR_OX 40000.0f
#define WAR_OY 40000.0f
#define WAR_AW 1600.0f
#define WAR_AH 1200.0f
#define WAR_MAXU 80
#define WAR_MAXS 28
#define WAR_MAXP 64
#define WAR_BAR_H 46
#define WAR_U_HARV 0
#define WAR_U_MELEE 1
#define WAR_U_RANGED 2
#define WAR_U_CRAWL 3
#define WAR_U_SPIT 4
#define WAR_U_SCOUT 5
#define WAR_U_SIEGE 6
#define WAR_U_PATCH 7
#define WAR_U_BHARV 8
#define WAR_S_HQ 0
#define WAR_S_PATCH 1
#define WAR_S_ZAP 2
#define WAR_S_WALL 3
#define WAR_S_HIVE 4
#define WAR_S_ASM 5
#define WAR_S_GATE 6
// WAR_MAXT lives up by BwSim (it sizes the struct's terrain arrays)
#define WAR_FOG_W 40
#define WAR_FOG_H 30
#define WAR_CARRY 30          // bits a harvester hauls per trip (v1.19: 25 -> 30 — richer fields)
// v1.21 (Daniel: balanced economics): what the HIVE pays for its own
// bugs, out of its own purse — set against your train costs (melee
// 50 / ranged 75 / harvester 80) and the same 30-bit hauls both
// sides make. A crawler is a cheaper, weaker body than a melee
// buddy, so it is priced like one: 20. The spitter is the ranged
// bug: 35. A replacement bug harvester is half your harvester's
// price for a slower, frailer worker: 40.
#define WAR_COST_CRAWL 20
#define WAR_COST_SPIT 35
#define WAR_COST_BHARV 40
#define WAR_SUB_H 46          // the categorized bar's options strip, drawn above the bar (WAR_BAR_H itself is load-bearing — tap math is coupled to it)
static const int WAR_SRAD[7] = { 34, 26, 14, 13, 30, 28, 13 };        // structure radii (gate last: a firewall-footprint segment)
static const int WAR_SHP[7] = { 600, 300, 160, 120, 0, 450, 120 };     // structure hp (hive scales instead)
static const int WAR_TRAIN_COST[8] = { 80, 50, 75, 0, 0, 90, 160, 45 };   // by UNIT type: harvester / melee / ranged / - / - / scout / siege / patcher (v1.19 addendum: the patcher is the cheapest body on the field — fragile and weaponless, priced just under the melee's 50)
static const float WAR_TRAIN_TIME[8] = { 6.0f, 4.0f, 5.0f, 0, 0, 5.0f, 9.0f, 4.0f };
static const int WAR_BUILD_COST[7] = { 0, 150, 100, 25, 0, 200, 35 }; // patch bay / zapper / firewall / - / assembly plant / gate
// canopy tints for WAR terrain trees, one per region (Meadows green,
// Spaghetti olive, Legacy dry, Badlands scorched, Cache frost-pale,
// RAM Flats neon, Cable Jungle deep green, the Depths ashen violet)
static const u32 WAR_TREE_COL[8] = { COL(46,120,52), COL(96,124,44), COL(110,96,58), COL(120,74,44),
                                     COL(190,220,230), COL(64,240,190), COL(34,110,60), COL(96,80,140) };
// debris hit points by terrain kind: 0 tree / 1 rock / 2 terminal /
// 3 spool / 4 rubble — rubble crumbles fast, rock takes a beating.
// Kind 5 (v1.18) is a BARRIER segment: 99999 is a sentinel, not a
// real pool — war_hurt_terrain refuses kind 5 outright, so its hp
// never moves and the collision/A* tests that key off thp>0 hold
// forever.
static const int WAR_THP[6] = { 35, 90, 55, 45, 25, 99999 };
// debris bounties (v1.17): felling terrain with PLAYER-side damage pays
// bits by kind — tree / rock / terminal / spool / rubble. Bugs pay none.
// Barriers pay nothing (and can never be felled anyway).
static const int WAR_TBITS[6] = { 10, 25, 40, 30, 15, 0 };
static const u32 WAR_SPCOL[7] = { COL(240,180,90), COL(250,150,70), COL(150,105,70), COL(90,180,170), COL(150,150,160), COL(190,200,210), COL(255,205,80) };   // last: the gate's warm gold — "yours, walk through"
static int war_u_side(int type){ return (type==WAR_U_CRAWL || type==WAR_U_SPIT || type==WAR_U_BHARV)? 1 : 0; }
static int war_u_hp(int type, int region){
    switch (type){ case 0: return 45; case 1: return 90; case 2: return 55;
                   case 3: case 8: return 26 + 4*region; case 5: return 45; case 6: return 230;
                   case 7: return 45;   // the patcher: fragile, exactly a harvester's hide
                   default: return 20 + 3*region; }
}
static float war_u_speed(int type, int region){
    switch (type){ case 0: return 95.0f; case 1: return 105.0f; case 2: return 100.0f;
                   case 3: case 8: return 60.0f + 3.0f*region; case 5: return 95.0f; case 6: return 38.0f;
                   case 7: return 100.0f;   // the patcher: a medium pace — it has fires to get to
                   default: return 56.0f + 2.0f*region; }
}
static int war_u_dmg(int type, int region){
    switch (type){ case 1: return 12; case 2: return 9; case 5: return 5; case 6: return 26;
                   case 3: return 6 + region/2; case 4: return 7 + region/2; default: return 0; }
}
static float war_u_range(int type){
    switch (type){ case 1: return 18.0f; case 2: return 105.0f; case 5: return 85.0f; case 6: return 125.0f;
                   case 3: return 16.0f; case 4: return 95.0f; default: return 0.0f; }
}
static float war_u_rate(int type){
    switch (type){ case 1: return 0.7f; case 2: return 0.9f; case 5: return 1.0f; case 6: return 2.2f;
                   case 3: return 0.8f; case 4: return 1.4f; default: return 1.0f; }
}
static float war_u_sight(int type){   // fog of war: how far a friendly unit sees
    switch (type){ case WAR_U_SCOUT: return 230.0f; default: return 150.0f; }
}
static int war_u_combat(int type){ return type==WAR_U_MELEE || type==WAR_U_RANGED || type==WAR_U_SCOUT || type==WAR_U_SIEGE; }
static int war_unit_alive(BwSim *S, int i){ return i>=0 && i<WAR_MAXU && S->war_uhp[i]>0; }
static int war_struct_alive(BwSim *S, int i){ return i>=0 && i<WAR_MAXS && S->war_shp[i]>0; }
// Target refs: >0 is a unit (index+1), <0 a structure (-(index+1)),
// and <= -100 a terrain feature — debris — encoded -(100 + t + 1),
// clear of every structure ref (-1..-28).
#define WAR_TREF(t) (-(100 + (t) + 1))
static int war_ref_feat(int ref){ return ref <= -100? -ref - 101 : -1; }
static int war_ref_alive(BwSim *S, int ref, float *x, float *y){
    int ft = war_ref_feat(ref);
    if (ft>=0){ if (ft>=S->war_tn || S->war_thp[ft]<=0) return 0; *x=S->war_tx[ft]; *y=S->war_ty[ft]; return 1; }
    if (ref>0){ int i=ref-1; if (!war_unit_alive(S,i)) return 0; *x=S->war_ux[i]; *y=S->war_uy[i]; return 1; }
    if (ref<0){ int i=-ref-1; if (!war_struct_alive(S,i)) return 0; *x=S->war_sx[i]; *y=S->war_sy[i]; return 1; }
    return 0;
}
static float war_ref_radius(BwSim *S, int ref){
    int ft = war_ref_feat(ref);
    if (ft>=0) return ft<S->war_tn? (float)S->war_trad[ft] : 0.0f;
    if (ref>0){ int t = S->war_utype[ref-1]; return t==WAR_U_SIEGE? 14.0f : t==WAR_U_SCOUT? 11.0f : 9.0f; }
    if (ref<0) return (float)WAR_SRAD[S->war_stype[-ref-1]];
    return 0.0f;
}
static int war_count_units(BwSim *S, int side){
    int n=0; for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]>0 && war_u_side(S->war_utype[i])==side) n++;
    return n;
}
static int war_count_type(BwSim *S, int type){
    int n=0; for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]>0 && S->war_utype[i]==type) n++;
    return n;
}
static int war_hives_alive(BwSim *S){
    int n=0; for (int i=0;i<WAR_MAXS;i++) if (S->war_shp[i]>0 && S->war_stype[i]==WAR_S_HIVE) n++;
    return n;
}
static int war_find_hq(BwSim *S){
    for (int i=0;i<WAR_MAXS;i++) if (S->war_stype[i]==WAR_S_HQ && S->war_smaxhp[i]>0) return i;
    return -1;
}
static int war_sel_count(BwSim *S){
    int n=0; for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]>0 && S->war_usel[i]) n++;
    return n;
}
static void war_note(BwSim *S, const char *txt){ scpy(S->war_msg, txt, 72); S->war_msgt = 2.8f; }
// v1.25: the placement verb for the field hints, per device
static const char *war_place_verb(void){ return hint_dev()==DEV_PAD? "A ON GROUND" : (hint_dev()==DEV_TOUCH? "TAP GROUND" : "CLICK GROUND"); }
static void war_blend_disc(int cx, int cy, int r, u32 c, int alpha);   // defined with the renderers; declared here for the draw helpers above it
// v1.21 cosmetic pass: spawn a purely visual particle (0 poof /
// 1 harvest sparkle / 2 structure poof). The ring is write-only
// from the sim's point of view — nothing but the renderer reads it.
static void war_fx_spawn(BwSim *S, float x, float y, int kind){
    int slot = 0;
    for (int i=0;i<24;i++) if (S->war_ft[i]<=0){ slot=i; break; }
    S->war_fx[slot]=x; S->war_fy[slot]=y; S->war_fk[slot]=kind;
    S->war_ft[slot]= kind==2? 0.7f : kind==1? 0.5f : 0.45f;
}
static void war_clamp_cam(BwSim *S){
    if (S->war_camx < WAR_OX+150) S->war_camx = WAR_OX+150;
    if (S->war_camx > WAR_OX+S->war_aw-150) S->war_camx = WAR_OX+S->war_aw-150;
    if (S->war_camy < WAR_OY+110) S->war_camy = WAR_OY+110;
    if (S->war_camy > WAR_OY+S->war_ah-110) S->war_camy = WAR_OY+S->war_ah-110;
}
static void war_clear_field(BwSim *S){
    for (int i=0;i<WAR_MAXU;i++){ S->war_uhp[i]=0; S->war_usel[i]=0; S->war_wpn[i]=0; S->war_wpi[i]=0; S->war_stallt[i]=0; S->war_repath[i]=0; }
    for (int i=0;i<WAR_MAXS;i++){ S->war_shp[i]=0; S->war_smaxhp[i]=0; S->war_qlen[i]=0; S->war_rallyx[i]=0; S->war_rallyy[i]=0; S->war_hbits[i]=0; S->war_sflash[i]=0; }
    for (int i=0;i<WAR_MAXU;i++) S->war_uflash[i]=0;
    for (int i=0;i<24;i++) S->war_ft[i]=0;
    for (int i=0;i<WAR_MAXP;i++) S->war_pt[i]=0;
    S->war_selstruct=-1; S->war_place=-1; S->war_demoarm=-1;
    S->war_tn=0; S->war_ltap_n=0; S->war_ltap_unit=-1;
    S->war_curon=0; S->war_barhl=-1; S->war_barhl_s=-1; S->war_barcap=0; S->war_barhl_row=0;   // a fresh field has no pad cursor, no bar highlight and no bar capture (v1.20)
    S->war_pause=0; S->war_bcat=-1;   // v1.17: no pause menu, bar at its top level
    // v1.19: a cleared field is a CAMPAIGN field at full size —
    // war_setup_battle overrides these for a skirmish right after
    // clearing. (war_size/war_diff are NOT reset: they are the
    // player's kept setup picks, not battle state.)
    S->war_skirmish=0; S->war_aw=WAR_AW; S->war_ah=WAR_AH;
    S->war_gw=WAR_FOG_W; S->war_gh=WAR_FOG_H; S->war_lines=0;
}
