// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: war.c — WAR! (bw_mode 8), the tap-first RTS.
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]
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
// --- WAR! LOCAL CO-OP COMMAND (v1.25 feat 13) -----------------------
// One battle, one army, one purse, two commanders. The battle lives
// in the OWNER's BwSim (whoever launched it); the other local player
// is the GUEST, with full command over the shared army. What is
// per-player is only the command VIEW — camera, pad cursor,
// selection, bar capture — and the view lives in each player's own
// BwSim fields. For the guest's command turn and render, the view
// set is EXCHANGED with the owner's sim, so the whole command layer
// (tap path, bar, pad) runs untouched against the battle; the
// exchange is swapped back at the end of the turn. war_msg and
// war_grace are NOT view state: the message line and the input
// grace belong to the battle, shared by both commanders.
typedef struct {
    float camx, camy, curx, cury; int curon;
    int barhl, barhl_s, barcap, barhl_row, capnx, capny;
    int selstruct, place, bcat, demoarm;
    int drag_id, dragx, dragy; float drag_cx, drag_cy;
    float ltap_t; int ltap_x, ltap_y, ltap_unit, ltap_n;
    float ordx, ordy, ordt;
    int usel[WAR_MAXU];
} WarView;
static void war_view_save(BwSim *S, WarView *v){
    v->camx=S->war_camx; v->camy=S->war_camy; v->curx=S->war_curx; v->cury=S->war_cury; v->curon=S->war_curon;
    v->barhl=S->war_barhl; v->barhl_s=S->war_barhl_s; v->barcap=S->war_barcap; v->barhl_row=S->war_barhl_row;
    v->capnx=S->war_capnx; v->capny=S->war_capny;
    v->selstruct=S->war_selstruct; v->place=S->war_place; v->bcat=S->war_bcat; v->demoarm=S->war_demoarm;
    v->drag_id=S->war_drag_id; v->dragx=S->war_dragx; v->dragy=S->war_dragy; v->drag_cx=S->war_drag_cx; v->drag_cy=S->war_drag_cy;
    v->ltap_t=S->war_ltap_t; v->ltap_x=S->war_ltap_x; v->ltap_y=S->war_ltap_y; v->ltap_unit=S->war_ltap_unit; v->ltap_n=S->war_ltap_n;
    v->ordx=S->war_ordx; v->ordy=S->war_ordy; v->ordt=S->war_ordt;
    for (int i=0;i<WAR_MAXU;i++) v->usel[i]=S->war_usel[i];
}
static void war_view_load(BwSim *S, const WarView *v){
    S->war_camx=v->camx; S->war_camy=v->camy; S->war_curx=v->curx; S->war_cury=v->cury; S->war_curon=v->curon;
    S->war_barhl=v->barhl; S->war_barhl_s=v->barhl_s; S->war_barcap=v->barcap; S->war_barhl_row=v->barhl_row;
    S->war_capnx=v->capnx; S->war_capny=v->capny;
    S->war_selstruct=v->selstruct; S->war_place=v->place; S->war_bcat=v->bcat; S->war_demoarm=v->demoarm;
    S->war_drag_id=v->drag_id; S->war_dragx=v->dragx; S->war_dragy=v->dragy; S->war_drag_cx=v->drag_cx; S->war_drag_cy=v->drag_cy;
    S->war_ltap_t=v->ltap_t; S->war_ltap_x=v->ltap_x; S->war_ltap_y=v->ltap_y; S->war_ltap_unit=v->ltap_unit; S->war_ltap_n=v->ltap_n;
    S->war_ordx=v->ordx; S->war_ordy=v->ordy; S->war_ordt=v->ordt;
    for (int i=0;i<WAR_MAXU;i++) S->war_usel[i]=v->usel[i];
}
static void war_view_exchange(BwSim *A, BwSim *B){
    WarView t; war_view_save(A, &t); WarView u; war_view_save(B, &u);
    war_view_load(A, &u); war_view_load(B, &t);
}
// the battle sim a player commands: their own, or the owner's when
// they are its guest. The command layer addresses the battle
// through this, never through &BWS[pi] directly.
static BwSim *war_bs(int pi){
    BwSim *S = &BWS[pi];
    return S->war_guest? &BWS[S->war_guest-1] : S;
}
static int war_other_local(int pi){
    if (pi==G.my_id) return G.p2_slot;
    if (pi==G.p2_slot) return G.my_id;
    return -1;
}
static int war_net_ok(void){   // Cart's link condition, verbatim in spirit: local couch only
    return G.net_mode==NET_OFF || (G.net_mode==NET_HOST && !any_remote_connected());
}
// a joining guest starts with a clean view standing on the owner's
// camera — the war_clear_field defaults for every view field
static void war_view_fresh(BwSim *GS, BwSim *HS){
    GS->war_camx=HS->war_camx; GS->war_camy=HS->war_camy;
    GS->war_curx=HS->war_camx; GS->war_cury=HS->war_camy; GS->war_curon=0;
    GS->war_barhl=-1; GS->war_barhl_s=-1; GS->war_barcap=0; GS->war_barhl_row=0; GS->war_capnx=0; GS->war_capny=0;
    GS->war_selstruct=-1; GS->war_place=-1; GS->war_bcat=-1; GS->war_demoarm=-1;
    GS->war_drag_id=-1; GS->war_dragx=0; GS->war_dragy=0; GS->war_drag_cx=0; GS->war_drag_cy=0;
    GS->war_ltap_t=-10.0f; GS->war_ltap_x=0; GS->war_ltap_y=0; GS->war_ltap_unit=-1; GS->war_ltap_n=0;
    GS->war_ordx=0; GS->war_ordy=0; GS->war_ordt=0;
    for (int i=0;i<WAR_MAXU;i++) GS->war_usel[i]=0;
}
static void war_link_form(int owner, int guest){
    BwSim *HS = &BWS[owner], *GS = &BWS[guest];
    HS->war_duo = guest; GS->war_guest = owner + 1;
    GS->war_state = HS->war_state;
    war_view_fresh(GS, HS);
    war_note(HS, guest==G.p2_slot? "P2 JOINED THE COMMAND!" : "P1 JOINED THE COMMAND!");
    sfx(19);
}
static void war_link_break(int owner){
    BwSim *HS = &BWS[owner];
    int g = HS->war_duo;
    if (g>=0 && g<MAXP){
        BwSim *GS = &BWS[g];
        if (GS->war_guest==owner+1) GS->war_guest = 0;
    }
    HS->war_duo = -1;
}
// a battle launch pulls the other local commander in, if they are
// standing at their own WAR! front door (map or setup panel)
static void war_link_pull(int pi){
    if (!war_net_ok()) return;
    int o = war_other_local(pi);
    if (o<0) return;
    BwSim *OS = &BWS[o];
    if (!G.bud[o].active || !G.bud[o].in_bw || G.bud[o].bw_mode!=8) return;
    if (OS->war_guest || OS->war_duo==pi) return;
    if (OS->war_state!=0 && OS->war_state!=4) return;
    war_link_form(pi, o);
}
// the acting player's pad sources: pad 1's globals for P1, the new
// pad-2 channels for P2 (feat 13's plumbing)
static void war_pad_src(int pi, const float **ax, u32 *pe, u32 *held){
    if (pi==G.my_id){ *ax = IN.pad_ax; *pe = IN.pad_edges; *held = IN.act_held; }
    else { *ax = IN.pad2_ax; *pe = IN.pad2_edges; *held = IN.act2_held; }
}
static int war_pad_live(int pi){ return pi==G.my_id? pad_mode() : (IN.pad_count>=2); }
// --- fog of war ------------------------------------------------------
// Cells are 40u over the 1600x1200 arena. Explored is sticky: a cell
// that has ever been visible stays explored for the whole battle.
static int war_fog_at(BwSim *S, float wx, float wy){
    int cx = (int)((wx - WAR_OX)/40.0f), cy = (int)((wy - WAR_OY)/40.0f);
    if (cx<0 || cy<0 || cx>=S->war_gw || cy>=S->war_gh) return 2;   // off-arena (or off a SMALL skirmish field): don't hide the frame
    return S->war_fog[cy*WAR_FOG_W + cx];
}
static void war_fog_mark(BwSim *S, float wx, float wy, float r){
    int cx0 = (int)((wx - r - WAR_OX)/40.0f), cx1 = (int)((wx + r - WAR_OX)/40.0f);
    int cy0 = (int)((wy - r - WAR_OY)/40.0f), cy1 = (int)((wy + r - WAR_OY)/40.0f);
    if (cx0<0) cx0=0; if (cy0<0) cy0=0;
    if (cx1>=S->war_gw) cx1=S->war_gw-1; if (cy1>=S->war_gh) cy1=S->war_gh-1;
    for (int cy=cy0; cy<=cy1; cy++) for (int cx=cx0; cx<=cx1; cx++){
        float px = WAR_OX + cx*40.0f + 20.0f, py = WAR_OY + cy*40.0f + 20.0f;
        float dx = px-wx, dy = py-wy;
        if (dx*dx + dy*dy <= r*r) S->war_fog[cy*WAR_FOG_W + cx] = 2;
    }
}
static void war_fog_update(BwSim *S){
    for (int i=0;i<WAR_FOG_W*WAR_FOG_H;i++) if (S->war_fog[i]==2) S->war_fog[i]=1;   // visible lapses into explored, never back to black
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0 || war_u_side(S->war_utype[i])!=0) continue;
        war_fog_mark(S, S->war_ux[i], S->war_uy[i], war_u_sight(S->war_utype[i]));
    }
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0 || S->war_sside[s]!=0) continue;
        war_fog_mark(S, S->war_sx[s], S->war_sy[s], S->war_stype[s]==WAR_S_HQ? 200.0f : 170.0f);
    }
}
// --- terrain ---------------------------------------------------------
// v1.18 (Daniel: "novel every time you play"): the scatter is rolled
// from the battle's own seed (bseed), with the region mixed in for
// flavour — the same region never grows the same woods twice. Hive
// clearances below read the ACTUAL hive structs, which
// war_start_battle has already placed from the same seed.
static void war_gen_terrain(BwSim *S, u32 bseed){
    int region = S->war_region;
    S->war_tn = 0;
    u32 seed = bseed ^ (0x7E44u + (u32)region * 977u);
    int want = 52 + (int)(hash2(region, 7, seed) % 17u);   // 52..68 features
    for (int t=0; t<600 && S->war_tn<want; t++){
        float x = WAR_OX + 44.0f + hash_f(t, 1, seed) * (S->war_aw - 88.0f);
        float y = WAR_OY + 44.0f + hash_f(t, 2, seed) * (S->war_ah - 88.0f);
        float dx, dy;
        dx = x-(WAR_OX+280); dy = y-(WAR_OY+S->war_ah*0.5f); if (dx*dx+dy*dy < 104.0f*104.0f) continue;   // the Core's clearing (the Core sits at mid-height on any field)
        int bad = 0;
        for (int s=0;s<WAR_MAXS && !bad;s++){
            if (S->war_shp[s]<=0 || S->war_stype[s]!=WAR_S_HIVE) continue;
            dx = x-S->war_sx[s]; dy = y-S->war_sy[s]; if (dx*dx+dy*dy < 84.0f*84.0f) bad = 1;
        }
        for (int n=0;n<S->war_nn && !bad;n++){
            dx = x-S->war_nx[n]; dy = y-S->war_ny[n]; if (dx*dx+dy*dy < 54.0f*54.0f) bad = 1;
        }
        for (int j=0;j<S->war_tn && !bad;j++){
            dx = x-S->war_tx[j]; dy = y-S->war_ty[j]; if (dx*dx+dy*dy < 30.0f*30.0f) bad = 1;
        }
        if (bad) continue;
        float r = hash_f(t, 3, seed);
        int kind = r < 0.50f? 0 : r < 0.75f? 1 : 2 + (int)(hash2(t, 4, seed) % 3u);
        int i = S->war_tn++;
        S->war_tx[i]=x; S->war_ty[i]=y; S->war_ttype[i]=kind;
        S->war_trad[i] = 10 + (int)(hash2(t, 5, seed) % 5u);   // 10..14
        S->war_tmaxhp[i] = WAR_THP[kind]; S->war_thp[i] = (float)WAR_THP[kind]; S->war_tdeadt[i] = 0;
    }
    // v1.18: BARRIER LINES (terrain kind 5) — vertical runs of
    // indestructible slabs with a gap rolled into every line, so
    // each battle has walls to think around AND a path through
    // every one of them. Lines never span the full arena height,
    // so a Core->hive route always exists even ignoring the gaps.
    // Segments ride the same arrays as debris: collision
    // (war_separate), the A* blocked grid and the snapshot all key
    // off thp>0, and war_hurt_terrain refuses kind 5 — they can
    // never be removed, only routed around.
    { int debris_n = S->war_tn;   // the 30u spacing test below runs against DEBRIS only — a line's own segments stand 24u apart by design
      int lines = S->war_lines>0? S->war_lines : (region>=2? 3 : 2);   // v1.19: a skirmish overrides the line count
      for (int k=0;k<lines;k++){
          // v1.19: geometry is proportional to the live field —
          // at the campaign's 1600x1200 these are the same numbers
          // as before (x 560 + k*230 + roll*120, ylen 420..740,
          // y0 base 60, gap centre y0+90 + roll*(ylen-180)).
          float xk = S->war_aw*0.35f + (float)k*(S->war_aw*0.14375f) + hash_f(k, 21, bseed)*(S->war_aw*0.075f);
          float ylen = S->war_ah*0.35f + hash_f(k, 22, bseed)*(S->war_ah*0.2667f);
          float y0 = S->war_ah*0.05f + hash_f(k, 23, bseed)*((S->war_ah-S->war_ah*0.1f)-ylen);
          float gy = y0 + S->war_ah*0.075f + hash_f(k, 24, bseed)*(ylen-S->war_ah*0.15f);
          for (float y=y0; y<=y0+ylen; y+=24.0f){
              if (S->war_tn >= WAR_MAXT-2) break;
              float gdy = y-gy; if (gdy<0) gdy=-gdy;
              if (gdy < 55.0f) continue;   // the gap: a ~110u path through the line
              float wx = WAR_OX + xk, wy = WAR_OY + y;
              float dx, dy;
              dx = wx-(WAR_OX+280); dy = wy-(WAR_OY+S->war_ah*0.5f); if (dx*dx+dy*dy < 130.0f*130.0f) continue;   // the Core's clearing, wider than debris' — a wall must never pen the base in
              int bad = 0;
              for (int s=0;s<WAR_MAXS && !bad;s++){
                  if (S->war_shp[s]<=0 || S->war_stype[s]!=WAR_S_HIVE) continue;
                  dx = wx-S->war_sx[s]; dy = wy-S->war_sy[s]; if (dx*dx+dy*dy < 115.0f*115.0f) bad = 1;
              }
              for (int n=0;n<S->war_nn && !bad;n++){
                  dx = wx-S->war_nx[n]; dy = wy-S->war_ny[n]; if (dx*dx+dy*dy < 75.0f*75.0f) bad = 1;
              }
              for (int j=0;j<debris_n && !bad;j++){
                  dx = wx-S->war_tx[j]; dy = wy-S->war_ty[j]; if (dx*dx+dy*dy < 30.0f*30.0f) bad = 1;
              }
              if (bad) continue;
              int i = S->war_tn++;
              S->war_tx[i]=wx; S->war_ty[i]=wy; S->war_ttype[i]=5;
              S->war_trad[i]=15;
              S->war_tmaxhp[i]=WAR_THP[5]; S->war_thp[i]=(float)WAR_THP[5]; S->war_tdeadt[i]=0;
          }
      } }
}
static int war_spawn_unit(BwSim *S, int type, float x, float y){
    for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]<=0){
        S->war_utype[i]=type; S->war_ux[i]=x; S->war_uy[i]=y;
        S->war_umaxhp[i]=war_u_hp(type, S->war_region); S->war_uhp[i]=(float)S->war_umaxhp[i];
        S->war_ustate[i]=0; S->war_utarget[i]=0; S->war_ucarry[i]=0; S->war_usel[i]=0;
        S->war_uatk[i]=0; S->war_uharv[i]=0; S->war_utx[i]=x; S->war_uty[i]=y;
        S->war_urepair[i]=0; S->war_urepacc[i]=0; S->war_urepscan[i]=0;
        S->war_wpn[i]=0; S->war_wpi[i]=0; S->war_stallt[i]=0; S->war_repath[i]=0;
        S->war_stallx[i]=x; S->war_stally[i]=y; S->war_ptgx[i]=x; S->war_ptgy[i]=y;
        return i;
    }
    return -1;
}
static int war_spawn_struct(BwSim *S, int type, int side, float x, float y){
    for (int i=0;i<WAR_MAXS;i++) if (S->war_shp[i]<=0){
        int hp = type==WAR_S_HIVE? 220 + 60*S->war_region : WAR_SHP[type];
        S->war_stype[i]=type; S->war_sside[i]=side; S->war_sx[i]=x; S->war_sy[i]=y;
        S->war_smaxhp[i]=hp; S->war_shp[i]=(float)hp; S->war_scd[i]=0; S->war_qt[i]=0; S->war_qlen[i]=0; S->war_rallyx[i]=0; S->war_rallyy[i]=0; S->war_hbits[i]=0;
        return i;
    }
    return -1;
}
// the wave composition rule, shared by free spawns (the starting
// garrison) and bought ones (v1.21 waves): from region 2, every
// third bug off the line is a spitter
static int war_bug_type_next(BwSim *S){
    if (S->war_region>=2 && (war_count_units(S, 1) % 3)==2) return WAR_U_SPIT;
    return WAR_U_CRAWL;
}
static int war_spawn_bug_t(BwSim *S, int hive, int wave, int type){   // returns the unit slot, -1 when the swarm is capped or the field is full
    if (war_count_units(S, 1) >= 40) return -1;
    float a = (float)(war_count_units(S, 1) % 8) * 0.8f + (wave? 2.0f : 0.0f);
    int i = war_spawn_unit(S, type, S->war_sx[hive] + fcos(a)*(WAR_SRAD[WAR_S_HIVE]+16),
                                   S->war_sy[hive] + fsin(a)*(WAR_SRAD[WAR_S_HIVE]+16));
    if (i>=0){
        S->war_ustate[i] = wave? 1 : 0;
        S->war_utx[i] = S->war_sx[hive]; S->war_uty[i] = S->war_sy[hive];   // a guard's home
    }
    return i;
}
static void war_spawn_bug(BwSim *S, int hive, int wave){
    war_spawn_bug_t(S, hive, wave, war_bug_type_next(S));
}
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
        war_note(S, "SKIRMISH LOST.");   // v1.19: ad-hoc — no snapshot clear, no progress, nothing to save
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
// --- pathfinding (v1.17, Daniel: units must conceive of going
// AROUND a wall) ------------------------------------------------------
// war_move_to used to steer in a straight line and let war_separate
// push the unit out of obstacle circles; with the target behind a
// firewall line or a building the two forces reach a stable
// equilibrium and the unit pins forever. Now: grid A* over the fog
// grid's geometry (40x30 cells of 40u), requested ONLY when a unit
// stalls (0.6s of trying to step with <3u of net movement), so idle
// behaviour and cost are unchanged. The grid and the LOS shortcut are
// SIDE-AWARE (addendum 1): a gate is an obstacle only to side 1 —
// your harvesters route through your gates; bugs route around them
// or chew through. All scratch is fixed static arrays; every loop is
// bounded (expansion cap, ring cap, sample cap) — no wasm hangs.
static unsigned char war_pf_block[WAR_FOG_W*WAR_FOG_H];
static int war_pf_gw = WAR_FOG_W, war_pf_gh = WAR_FOG_H;   // v1.19: the live field's grid dims, set by war_pf_find — a SMALL skirmish field's grid is a subgrid of these same arrays (cells outside it are filled blocked)
static int war_pf_g[WAR_FOG_W*WAR_FOG_H];
static int war_pf_from[WAR_FOG_W*WAR_FOG_H];
static unsigned char war_pf_closed[WAR_FOG_W*WAR_FOG_H];
static int war_pf_path[WAR_FOG_W*WAR_FOG_H];
static float war_pf_tpx[128], war_pf_tpy[128];
static int war_pf_blocked_at(BwSim *S, int side, float wx, float wy){
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0) continue;
        if (side==0 && S->war_stype[s]==WAR_S_GATE) continue;   // your gates are doors — to you
        float rr = (float)WAR_SRAD[S->war_stype[s]] + 13.0f;
        float dx=wx-S->war_sx[s], dy=wy-S->war_sy[s];
        if (dx*dx+dy*dy < rr*rr) return 1;
    }
    for (int t=0;t<S->war_tn;t++){
        if (S->war_thp[t]<=0) continue;
        float rr = (float)S->war_trad[t] + 13.0f;
        float dx=wx-S->war_tx[t], dy=wy-S->war_ty[t];
        if (dx*dx+dy*dy < rr*rr) return 1;
    }
    return 0;
}
static int war_pf_seg_blocked(BwSim *S, int side, float x0, float y0, float x1, float y1){
    float dx=x1-x0, dy=y1-y0, d=fsqrt(dx*dx+dy*dy);
    int steps = (int)(d/10.0f) + 1; if (steps>200) steps=200;
    for (int k=0;k<=steps;k++){
        float t = (float)k/(float)steps;
        if (war_pf_blocked_at(S, side, x0+dx*t, y0+dy*t)) return 1;
    }
    return 0;
}
static int war_pf_nearest_free(int cx, int cy, float wx, float wy){
    // the free cell whose CENTRE is closest to (wx,wy), searching
    // rings outward from (cx,cy); the first ring with any free cell
    // wins. Distance matters, not just ring order: a unit pressing a
    // wall stands in a blocked cell, and the first free cell in ring
    // order can be the one BEHIND the wall — its centre then becomes
    // the path's first waypoint, unreachable, and the unit jams
    // against the wall forever (found by wartest check 5: a crawler
    // frozen at the wall face for 45s). The nearest free centre is
    // on the unit's own side of the obstacle, where it can be
    // steered to, and the grid path leads on from there.
    for (int r=0;r<=4;r++){
        int best=-1; float bd=1e18f;
        for (int dy=-r; dy<=r; dy++) for (int dx=-r; dx<=r; dx++){
            if (r>0 && dx>-r && dx<r && dy>-r && dy<r) continue;   // the ring only, never the filled square
            int nx=cx+dx, ny=cy+dy;
            if (nx<0 || ny<0 || nx>=war_pf_gw || ny>=war_pf_gh) continue;
            if (war_pf_block[ny*WAR_FOG_W+nx]) continue;
            float ccx = WAR_OX + nx*40.0f + 20.0f, ccy = WAR_OY + ny*40.0f + 20.0f;
            float ddx = ccx-wx, ddy = ccy-wy, d2 = ddx*ddx + ddy*ddy;
            if (d2<bd){ bd=d2; best = ny*WAR_FOG_W+nx; }
        }
        if (best>=0) return best;
    }
    return -1;
}
static int war_pf_find(BwSim *S, int i, float tx, float ty){
    int side = war_u_side(S->war_utype[i]);
    war_pf_gw = S->war_gw; war_pf_gh = S->war_gh;
    for (int cy=0; cy<WAR_FOG_H; cy++) for (int cx=0; cx<WAR_FOG_W; cx++)
        war_pf_block[cy*WAR_FOG_W+cx] = (cx>=war_pf_gw || cy>=war_pf_gh)? 1 : (unsigned char)war_pf_blocked_at(S, side, WAR_OX + cx*40.0f + 20.0f, WAR_OY + cy*40.0f + 20.0f);
    int start = war_pf_nearest_free((int)((S->war_ux[i]-WAR_OX)/40.0f), (int)((S->war_uy[i]-WAR_OY)/40.0f), S->war_ux[i], S->war_uy[i]);
    int goal = war_pf_nearest_free((int)((tx-WAR_OX)/40.0f), (int)((ty-WAR_OY)/40.0f), tx, ty);
    if (start<0 || goal<0) return 0;
    int gx = goal%WAR_FOG_W, gy = goal/WAR_FOG_W;
    for (int c=0;c<WAR_FOG_W*WAR_FOG_H;c++){ war_pf_g[c]=-1; war_pf_from[c]=-1; war_pf_closed[c]=0; }
    war_pf_g[start]=0;
    int found = (start==goal), expansions = 0;
    while (!found){
        int best=-1, bf=0;
        for (int c=0;c<WAR_FOG_W*WAR_FOG_H;c++){
            if (war_pf_g[c]<0 || war_pf_closed[c]) continue;
            int cx=c%WAR_FOG_W, cy=c/WAR_FOG_W;
            int adx = cx>gx? cx-gx : gx-cx, ady = cy>gy? cy-gy : gy-cy;
            int f = war_pf_g[c] + 10*(adx>ady? adx:ady) + 4*(adx<ady? adx:ady);   // octile heuristic
            if (best<0 || f<bf){ best=c; bf=f; }
        }
        if (best<0) break;                    // the frontier ran dry: unreachable
        if (++expansions > 4000) break;       // bounded, always
        war_pf_closed[best]=1;
        if (best==goal){ found=1; break; }
        int bx=best%WAR_FOG_W, by=best/WAR_FOG_W;
        for (int dy=-1; dy<=1; dy++) for (int dx=-1; dx<=1; dx++){
            if (!dx && !dy) continue;
            int nx=bx+dx, ny=by+dy;
            if (nx<0 || ny<0 || nx>=war_pf_gw || ny>=war_pf_gh) continue;
            int nc = ny*WAR_FOG_W+nx;
            if (war_pf_block[nc] || war_pf_closed[nc]) continue;
            if (dx && dy && (war_pf_block[by*WAR_FOG_W+nx] || war_pf_block[ny*WAR_FOG_W+bx])) continue;   // no corner cutting
            int ng = war_pf_g[best] + ((dx && dy)? 14 : 10);
            if (war_pf_g[nc]<0 || ng<war_pf_g[nc]){ war_pf_g[nc]=ng; war_pf_from[nc]=best; }
        }
    }
    if (!found) return 0;
    // the came-from chain, goal-first; every step strictly descends
    // the g-score, so the walk is bounded by the chain itself
    int pn=0, c=goal;
    while (c>=0 && pn<WAR_FOG_W*WAR_FOG_H){ war_pf_path[pn++]=c; if (c==start) break; c=war_pf_from[c]; }
    if (pn<1 || war_pf_path[pn-1]!=start) return 0;
    // turning points only: walk in travel order, keep a cell when the
    // direction of travel changes (plus the first and last cells)
    int tn=0, pdx=0, pdy=0;
    for (int k=pn-1; k>=0; k--){
        int cx=war_pf_path[k]%WAR_FOG_W, cy=war_pf_path[k]/WAR_FOG_W;
        int dx=0, dy=0;
        if (k>0){ dx = war_pf_path[k-1]%WAR_FOG_W - cx; dy = war_pf_path[k-1]/WAR_FOG_W - cy; }
        if ((k==pn-1 || k==0 || dx!=pdx || dy!=pdy) && tn<128){
            war_pf_tpx[tn]=WAR_OX+cx*40.0f+20.0f; war_pf_tpy[tn]=WAR_OY+cy*40.0f+20.0f; tn++;
        }
        pdx=dx; pdy=dy;
    }
    // LOS shortcut: from wherever we stand, jump to the farthest
    // turning point with a clear segment (attempts capped)
    int wn=0, idx=0; float ax=S->war_ux[i], ay=S->war_uy[i];
    while (idx<tn && wn<31){
        int jump=idx, tries=0;
        for (int k=tn-1; k>idx && tries<24; k--, tries++)
            if (!war_pf_seg_blocked(S, side, ax, ay, war_pf_tpx[k], war_pf_tpy[k])){ jump=k; break; }
        S->war_wpx[i][wn]=war_pf_tpx[jump]; S->war_wpy[i][wn]=war_pf_tpy[jump]; wn++;
        ax=war_pf_tpx[jump]; ay=war_pf_tpy[jump]; idx=jump+1;
    }
    if (wn<1) return 0;
    S->war_wpn[i]=(unsigned char)wn; S->war_wpi[i]=0;
    S->war_ptgx[i]=tx; S->war_ptgy[i]=ty;
    return 1;
}
// one stall window: 1 when a full 0.6s passed with <3u of net
// movement — the unit is pressing against the world, not travelling
static int war_pf_stall_tick(BwSim *S, int i, float dt){
    S->war_stallt[i] += dt;
    if (S->war_stallt[i] < 0.6f) return 0;
    float mdx=S->war_ux[i]-S->war_stallx[i], mdy=S->war_uy[i]-S->war_stally[i];
    int stuck = (mdx*mdx + mdy*mdy < 9.0f);
    S->war_stallt[i]=0; S->war_stallx[i]=S->war_ux[i]; S->war_stally[i]=S->war_uy[i];
    return stuck;
}
static float war_move_to(BwSim *S, int i, float tx, float ty, float stopd, float dt){
    float dx=tx-S->war_ux[i], dy=ty-S->war_uy[i];
    float d=fsqrt(dx*dx+dy*dy);
    if (S->war_repath[i] > 0) S->war_repath[i] -= dt;
    if (S->war_wpn[i]>0){
        float pdx=tx-S->war_ptgx[i], pdy=ty-S->war_ptgy[i];
        if (pdx*pdx+pdy*pdy > 24.0f*24.0f) S->war_wpn[i]=0;   // the target JUMPED since last frame (a new order, a dead target replaced): the path is stale intent
        else { S->war_ptgx[i]=tx; S->war_ptgy[i]=ty; }        // a MOVING target (a unit under pursuit) re-anchors every frame: its step-by-step motion is the same intent, not staleness — a cumulative-drift rule here killed every pursuit path within a second (wartest check 5: the bait's ordinary charge-and-return oscillation, ±30u, dropped the bug's path mid-lunge three times in a row)
    }
    if (S->war_wpn[i]>0){
        if (war_pf_stall_tick(S, i, dt)) S->war_wpn[i]=0;   // the path itself jammed (a new wall, a crowd): drop it and re-think below
        else {
            while (S->war_wpi[i] < S->war_wpn[i]){
                float wx=S->war_wpx[i][S->war_wpi[i]], wy=S->war_wpy[i][S->war_wpi[i]];
                float wdx=wx-S->war_ux[i], wdy=wy-S->war_uy[i];
                if (wdx*wdx+wdy*wdy <= 100.0f){ S->war_wpi[i]++; continue; }
                war_move_step(S, i, wx, wy, 6.0f, dt);
                return d;
            }
            S->war_wpn[i]=0;   // waypoints exhausted: the last leg is a straight line at the real target, caller's stopd intact
        }
    }
    if (d > stopd + 1.0f){
        if (war_pf_stall_tick(S, i, dt) && S->war_repath[i] <= 0.0f)
            S->war_repath[i] = war_pf_find(S, i, tx, ty)? 1.5f : 2.0f;   // no route (a walled-in Core): keep pressing, ask again later
    } else { S->war_stallt[i]=0; S->war_stallx[i]=S->war_ux[i]; S->war_stally[i]=S->war_uy[i]; }   // standing to fire never accumulates stall
    return war_move_step(S, i, tx, ty, stopd, dt);
}
static void war_harv_update(int pi, int i, float dt){
    BwSim *S = &BWS[pi];
    int st = S->war_ustate[i];
    float x = S->war_ux[i], y = S->war_uy[i];
    if (st==1){   // a manual move order: go there, then think again
        if (war_move_to(S, i, S->war_utx[i], S->war_uty[i], 4.0f, dt) <= 6.0f) S->war_ustate[i]=0;
        return;
    }
    if (st==3){   // parked on a node, scooping bits
        S->war_uharv[i] -= dt;
        if (S->war_uharv[i] <= 0){
            int n = S->war_utarget[i]-1;   // harvesters store the node index+1 in utarget
            if (n>=0 && n<S->war_nn && S->war_namt[n]>0){
                int take = WAR_CARRY - S->war_ucarry[i];
                if (take > S->war_namt[n]) take = S->war_namt[n];
                S->war_namt[n] -= take; S->war_ucarry[i] += take;
                if (take>0) war_fx_spawn(S, x, y-6, 1);   // cosmetic: the scoop sparkle
            }
            S->war_ustate[i] = 0;
        }
        return;
    }
    if (S->war_ucarry[i] >= WAR_CARRY || st==4){   // full (or already homeward): dump it at the Core
        int hq = war_find_hq(S);
        if (hq<0 || !war_struct_alive(S, hq)){ S->war_ustate[i]=0; return; }
        float d = war_move_to(S, i, S->war_sx[hq], S->war_sy[hq], WAR_SRAD[WAR_S_HQ]+8.0f, dt);
        if (d <= WAR_SRAD[WAR_S_HQ]+16.0f){
            if (S->war_ucarry[i]>0){ S->war_bits += S->war_ucarry[i]; S->war_ucarry[i]=0; sfx(6); war_fx_spawn(S, x, y-6, 1); }   // cosmetic: the deposit sparkle
            S->war_ustate[i]=0;
        } else S->war_ustate[i]=4;
        return;
    }
    // find the nearest node with bits left and go scoop
    { int bn=-1; float bd=1e18f;
      for (int n=0;n<S->war_nn;n++){
          if (S->war_namt[n]<=0) continue;
          float dx=S->war_nx[n]-x, dy=S->war_ny[n]-y, d2=dx*dx+dy*dy;
          if (d2<bd){ bd=d2; bn=n; }
      }
      if (bn<0){   // the field is dry: hand over what's carried, then wait by the Core
          if (S->war_ucarry[i]>0){ S->war_ustate[i]=4; return; }
          int hq = war_find_hq(S);
          if (hq>=0) war_move_to(S, i, S->war_sx[hq]+48, S->war_sy[hq]+34, 6.0f, dt);
          return;
      }
      if (war_move_to(S, i, S->war_nx[bn], S->war_ny[bn], 14.0f, dt) <= 20.0f){
          S->war_ustate[i]=3; S->war_uharv[i]=2.0f; S->war_utarget[i]=bn+1;
      } }
}
// v1.21: the BUG HARVESTER — the hive's half of the economy, the
// player's harvester loop mirrored exactly: same shared nodes, same
// 2s scoop, same WAR_CARRY load, hauled home and deposited — into
// its own hive's purse instead of yours. It has no weapon and never
// will: its states are the harvester's own (0 think / 3 scooping /
// 4 homeward), and what it carries can be raided out of it only by
// killing it, exactly like yours.
static void war_bharv_update(int pi, int i, float dt){
    BwSim *S = &BWS[pi];
    int st = S->war_ustate[i];
    float x = S->war_ux[i], y = S->war_uy[i];
    if (st==3){   // parked on a node, scooping bits
        S->war_uharv[i] -= dt;
        if (S->war_uharv[i] <= 0){
            int n = S->war_utarget[i]-1;   // the node index+1 rides in utarget, as with yours
            if (n>=0 && n<S->war_nn && S->war_namt[n]>0){
                int take = WAR_CARRY - S->war_ucarry[i];
                if (take > S->war_namt[n]) take = S->war_namt[n];
                S->war_namt[n] -= take; S->war_ucarry[i] += take;
                if (take>0) war_fx_spawn(S, x, y-6, 1);   // cosmetic: the scoop sparkle
            }
            S->war_ustate[i] = 0;
        }
        return;
    }
    if (S->war_ucarry[i] >= WAR_CARRY || st==4){   // full (or already homeward): haul it to the home hive
        int h = war_home_hive(S, x, y);
        if (h<0){ S->war_ustate[i]=0; return; }   // no hive left to pay — the battle is already decided
        float d = war_move_to(S, i, S->war_sx[h], S->war_sy[h], WAR_SRAD[WAR_S_HIVE]+8.0f, dt);
        if (d <= WAR_SRAD[WAR_S_HIVE]+16.0f){
            if (S->war_ucarry[i]>0){ S->war_hbits[h] += S->war_ucarry[i]; S->war_ucarry[i]=0; war_fx_spawn(S, x, y-6, 1); }   // cosmetic: the deposit sparkle (no sfx — the hive works in silence)
            S->war_ustate[i]=0;
        } else S->war_ustate[i]=4;
        return;
    }
    // find the nearest node with bits left and go scoop — the SAME
    // pool the player draws from: a node the hive drains is gone
    { int bn=-1; float bd=1e18f;
      for (int n=0;n<S->war_nn;n++){
          if (S->war_namt[n]<=0) continue;
          float dx=S->war_nx[n]-x, dy=S->war_ny[n]-y, d2=dx*dx+dy*dy;
          if (d2<bd){ bd=d2; bn=n; }
      }
      if (bn<0){   // the field is dry: hand over what's carried, then wait by the hive
          if (S->war_ucarry[i]>0){ S->war_ustate[i]=4; return; }
          int h = war_home_hive(S, x, y);
          if (h>=0) war_move_to(S, i, S->war_sx[h]+52, S->war_sy[h]+38, 6.0f, dt);
          return;
      }
      if (war_move_to(S, i, S->war_nx[bn], S->war_ny[bn], 14.0f, dt) <= 20.0f){
          S->war_ustate[i]=3; S->war_uharv[i]=2.0f; S->war_utarget[i]=bn+1;
      } }
}
// v1.19 addendum: the PATCHER — repair infantry. It has no
// weapon and never will: its whole update is walk-to-work and
// channel. A player order owns it while it runs (a move order
// walks, a repair order fixes the named structure); with no
// order in progress it AUTOSEEKS — about twice a second it looks
// for the nearest damaged friendly structure anywhere on the
// field and goes to fix it (Daniel: "I want it to autoseek").
// Channel: +12 HP/s, billed at 1 bit per 4 HP — fractional bits
// accumulate in war_urepacc and only whole bits leave the purse.
// It stops when the structure is full, when the purse runs dry
// mid-bill (the OUT OF BITS note fires once per emptying), when
// another order lands, or when it dies.
static void war_patch_update(int pi, int i, float dt){
    BwSim *S = &BWS[pi];
    (void)pi;   // the channel bills the shared purse and heals through S directly — no per-player call-outs
    if (S->war_bits > 0) S->war_repnoted = 0;   // the purse recovered: the next emptying may call out again
    if (S->war_ustate[i]==1){   // a plain move order owns the unit until it arrives; autoseek resumes after
        if (war_move_to(S, i, S->war_utx[i], S->war_uty[i], 4.0f, dt) <= 6.0f) S->war_ustate[i]=0;
        return;
    }
    int s = S->war_urepair[i]-1;
    if (s>=0 && (!war_struct_alive(S, s) || S->war_sside[s]!=0 || S->war_shp[s] >= (float)S->war_smaxhp[s])){
        S->war_urepair[i]=0; s=-1;   // the work is gone or done — fall back to autoseek
    }
    if (s<0){
        S->war_urepscan[i] -= dt;
        if (S->war_urepscan[i] <= 0){
            S->war_urepscan[i] = 0.5f;
            if (S->war_bits > 0){   // an empty purse means wait, not wander (the note already fired at the halt)
                float bd = 1e18f; int bs = -1;
                for (int t=0;t<WAR_MAXS;t++){
                    if (S->war_shp[t]<=0 || S->war_sside[t]!=0) continue;
                    if (S->war_shp[t] >= (float)S->war_smaxhp[t]) continue;
                    float dx=S->war_sx[t]-S->war_ux[i], dy=S->war_sy[t]-S->war_uy[i], d2=dx*dx+dy*dy;
                    if (d2<bd){ bd=d2; bs=t; }
                }
                if (bs>=0){ S->war_urepair[i]=bs+1; s=bs; }
            }
        }
        if (s<0) return;
    }
    float rr = (float)WAR_SRAD[S->war_stype[s]] + 14.0f;
    if (war_move_to(S, i, S->war_sx[s], S->war_sy[s], rr, dt) > rr + 2.0f) return;   // still walking to the work
    float room = (float)S->war_smaxhp[s] - S->war_shp[s];
    float heal = 12.0f*dt; if (heal > room) heal = room;
    S->war_urepacc[i] += heal*0.25f;
    int pay = (int)S->war_urepacc[i];
    if (pay > S->war_bits) pay = S->war_bits;
    if (pay>0){ S->war_bits -= pay; S->war_urepacc[i] -= (float)pay; }
    if (S->war_bits<=0 && S->war_urepacc[i] >= 1.0f){
        // a whole bit is owed and the purse is empty: halt here
        S->war_urepair[i]=0; S->war_urepacc[i]=0;
        if (!S->war_repnoted){ S->war_repnoted=1; war_note(S, "OUT OF BITS - REPAIRS HALTED."); }
        return;
    }
    S->war_shp[s] += heal;
    if (S->war_shp[s] >= (float)S->war_smaxhp[s]){ S->war_shp[s] = (float)S->war_smaxhp[s]; S->war_urepair[i]=0; }
}
static void war_unit_update(int pi, int i, float dt){
    BwSim *S = &BWS[pi];
    int type = S->war_utype[i], side = war_u_side(type);
    if (S->war_uatk[i] > 0) S->war_uatk[i] -= dt;
    if (type == WAR_U_HARV){ war_harv_update(pi, i, dt); return; }
    if (type == WAR_U_BHARV){ war_bharv_update(pi, i, dt); return; }
    if (type == WAR_U_PATCH){ war_patch_update(pi, i, dt); return; }
    float x = S->war_ux[i], y = S->war_uy[i];
    int state = S->war_ustate[i];
    if (state==2){   // an attack order whose target died is just an order again
        float tx, ty;
        if (!war_ref_alive(S, S->war_utarget[i], &tx, &ty)){ S->war_ustate[i]=0; S->war_utarget[i]=0; state=0; }
    }
    int tref = 0;
    if (state==2) tref = S->war_utarget[i];
    else if (side==0){   // a buddy on guard: bugs first, a hive only if it is right there
        float aggro = state==1? 80.0f : 150.0f;
        tref = war_nearest_enemy(S, 0, x, y, aggro, 1, 0);
        if (!tref && state==0) tref = war_nearest_enemy(S, 0, x, y, 70.0f, 0, 1);
    } else if (state==1){   // a wave bug: whatever it meets, then the base
        tref = war_nearest_enemy(S, 1, x, y, 170.0f, 1, 0);
        if (!tref) tref = war_nearest_enemy(S, 1, x, y, 210.0f, 0, 1);
    } else {   // a hive guard: anything of yours that wanders close
        tref = war_nearest_enemy(S, 1, x, y, 180.0f, 1, 1);
    }
    if (tref){
        float tx, ty;
        war_ref_alive(S, tref, &tx, &ty);
        float dx=tx-x, dy=ty-y, dist=fsqrt(dx*dx+dy*dy);
        if (side==0 && state==0 && dist > 260.0f) tref = 0;   // guards don't chase across the map
        else {
            float rng = war_u_range(type) + war_ref_radius(S, tref);
            float d = war_move_to(S, i, tx, ty, rng*0.92f, dt);
            if (d <= rng + 2.0f && S->war_uatk[i] <= 0){
                S->war_uatk[i] = war_u_rate(type);
                int dmg = war_u_dmg(type, S->war_region);
                int ft = war_ref_feat(tref);
                // the siege gun is built to crack STRUCTURES — hives
                // above all. Debris is not a structure: trees, rocks
                // and rubble take the plain rate, never the x2.
                if (type==WAR_U_SIEGE && tref<0 && ft<0) dmg *= 2;
                if (type==WAR_U_MELEE || type==WAR_U_CRAWL){
                    if (tref>0) war_hurt_unit(pi, tref-1, dmg);
                    else if (ft>=0) war_hurt_terrain(pi, ft, dmg, side);
                    else war_hurt_struct(pi, -tref-1, dmg);
                } else war_fire(S, side, x, y, tref, dmg);
            }
            return;
        }
    }
    if (state==1 && side==0){   // a plain move order
        if (war_move_to(S, i, S->war_utx[i], S->war_uty[i], 4.0f, dt) <= 6.0f) S->war_ustate[i]=0;
        return;
    }
    if (side==1 && state==1){   // wave bug, nothing in the way: march on the Core
        int hq = war_find_hq(S);
        if (hq>=0 && war_struct_alive(S, hq)) war_move_to(S, i, S->war_sx[hq], S->war_sy[hq], WAR_SRAD[WAR_S_HQ]+10.0f, dt);
        return;
    }
    if (side==1 && state==0){   // guard bug: drift back home if it strayed
        float dx=S->war_utx[i]-x, dy=S->war_uty[i]-y;
        if (dx*dx+dy*dy > 60.0f*60.0f) war_move_to(S, i, S->war_utx[i], S->war_uty[i], 42.0f, dt);
        return;
    }
}
static void war_struct_update(int pi, int s, float dt){
    BwSim *S = &BWS[pi];
    if (S->war_scd[s] > 0) S->war_scd[s] -= dt;
    int type = S->war_stype[s];
    if (type==WAR_S_ZAP){
        if (S->war_scd[s] <= 0){
            int tref = war_nearest_enemy(S, 0, S->war_sx[s], S->war_sy[s], 118.0f, 1, 0);
            if (tref){ war_fire(S, 0, S->war_sx[s], S->war_sy[s]-10, tref, 8); S->war_scd[s]=0.55f; }
        }
    } else if (type==WAR_S_HIVE){
        // v1.21: the hive spends NOTHING between waves. The old
        // free defender drip is gone — guards are bought at wave
        // time out of the purse, from what's left after the wave
        // buy, like everything else the hive fields. (The starting
        // garrison still arrives free at battle build.)
    }
    if (S->war_qlen[s] > 0){   // the training QUEUE (addendum 2): the front item cooks on war_qt, the rest wait their turn
        S->war_qt[s] -= dt;
        if (S->war_qt[s] <= 0){
            if (war_count_units(S, 0) < 24){
                int ut = S->war_queue[s][0];
                float a = (float)((s*47) % 6) * 1.1f;
                int ni = war_spawn_unit(S, ut, S->war_sx[s] + fcos(a)*(WAR_SRAD[type]+16),
                                              S->war_sy[s] + fsin(a)*(WAR_SRAD[type]+16));
                if (ni>=0 && S->war_rallyx[s]!=0){   // rally (addendum 3): fresh units walk to the flag — harvesters too; on arrival they think for themselves
                    S->war_ustate[ni]=1; S->war_utarget[ni]=0;
                    S->war_utx[ni]=S->war_rallyx[s]; S->war_uty[ni]=S->war_rallyy[s];
                    S->war_wpn[ni]=0;
                }
                for (int k=1;k<S->war_qlen[s];k++) S->war_queue[s][k-1]=S->war_queue[s][k];
                S->war_qlen[s]--;
                if (S->war_qlen[s]>0) S->war_qt[s]=WAR_TRAIN_TIME[S->war_queue[s][0]];
                if (pi==G.my_id) sfx(19);
            } else S->war_qt[s] = 0.5f;   // army full: the front item waits at the door, and the queue behind it waits too
        }
    }
}
static void war_separate(BwSim *S){
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0) continue;
        for (int j=i+1;j<WAR_MAXU;j++){
            if (S->war_uhp[j]<=0) continue;
            float dx=S->war_ux[j]-S->war_ux[i], dy=S->war_uy[j]-S->war_uy[i];
            float d2=dx*dx+dy*dy;
            if (d2 < 15.0f*15.0f && d2 > 0.01f){
                float d=fsqrt(d2), push=(15.0f-d)*0.5f, px=dx/d*push, py=dy/d*push;
                S->war_ux[i]-=px; S->war_uy[i]-=py; S->war_ux[j]+=px; S->war_uy[j]+=py;
            } else if (d2 <= 0.01f) S->war_ux[j] += 1.5f;
        }
        for (int s=0;s<WAR_MAXS;s++){
            if (S->war_shp[s]<=0) continue;
            if (S->war_stype[s]==WAR_S_GATE && war_u_side(S->war_utype[i])==0) continue;   // gates (addendum 1): YOUR units walk straight through; bugs still hit a wall
            float rad = (float)WAR_SRAD[S->war_stype[s]] + 9.0f;
            float dx=S->war_ux[i]-S->war_sx[s], dy=S->war_uy[i]-S->war_sy[s];
            float d2=dx*dx+dy*dy;
            if (d2 < rad*rad){
                float d=fsqrt(d2);
                if (d<0.5f) S->war_ux[i] = S->war_sx[s]+rad;
                else { S->war_ux[i] = S->war_sx[s]+dx/d*rad; S->war_uy[i] = S->war_sy[s]+dy/d*rad; }
            }
        }
        for (int t=0;t<S->war_tn;t++){   // terrain blocks feet and wheels, never shots — and felled debris blocks nothing
            if (S->war_thp[t]<=0) continue;
            float rad = (float)S->war_trad[t] + 9.0f;
            float dx=S->war_ux[i]-S->war_tx[t], dy=S->war_uy[i]-S->war_ty[t];
            float d2=dx*dx+dy*dy;
            if (d2 < rad*rad){
                float d=fsqrt(d2);
                if (d<0.5f) S->war_ux[i] = S->war_tx[t]+rad;
                else { S->war_ux[i] = S->war_tx[t]+dx/d*rad; S->war_uy[i] = S->war_ty[t]+dy/d*rad; }
            }
        }
        if (S->war_ux[i] < WAR_OX+12) S->war_ux[i] = WAR_OX+12;
        if (S->war_ux[i] > WAR_OX+S->war_aw-12) S->war_ux[i] = WAR_OX+S->war_aw-12;
        if (S->war_uy[i] < WAR_OY+12) S->war_uy[i] = WAR_OY+12;
        if (S->war_uy[i] > WAR_OY+S->war_ah-12) S->war_uy[i] = WAR_OY+S->war_ah-12;
    }
}
static void war_proj_update(int pi, float dt){
    BwSim *S = &BWS[pi];
    for (int i=0;i<WAR_MAXP;i++){
        if (S->war_pt[i]<=0) continue;
        S->war_pt[i] -= dt;
        float tx, ty;
        if (!war_ref_alive(S, S->war_ptar[i], &tx, &ty)){ S->war_pt[i]=0; continue; }
        float dx=tx-S->war_px[i], dy=ty-S->war_py[i], d=fsqrt(dx*dx+dy*dy);
        float sp = S->war_pside[i]==0? 320.0f : 250.0f;
        if (d <= sp*dt + 7.0f){
            int ft = war_ref_feat(S->war_ptar[i]);
            if (S->war_ptar[i]>0) war_hurt_unit(pi, S->war_ptar[i]-1, S->war_pdmg[i]);
            else if (ft>=0) war_hurt_terrain(pi, ft, S->war_pdmg[i], S->war_pside[i]);
            else war_hurt_struct(pi, -S->war_ptar[i]-1, S->war_pdmg[i]);
            S->war_pt[i]=0;
        } else { S->war_px[i] += dx/d*sp*dt; S->war_py[i] += dy/d*sp*dt; }
    }
}
static float war_wave_dt(int region){ float v = 30.0f - 2.0f*region; return v<16.0f? 16.0f : v; }
// v1.19: eleven nodes, 8,200 bits on a campaign field (was nine /
// 4,800) — (560,600) and (1180,600) join the midfield: the centre
// is contested ground now, not just a road between two halves.
// The skirmish sizes draw from this same table (SMALL takes a
// scaled subset; LARGE adds two corner nodes of its own).
static const float WAR_ND[11][2] = { {430,430},{430,770},{800,300},{800,900},{800,600},{1180,350},{1180,850},{620,120},{1010,600},{560,600},{1180,600} };
static const int WAR_NA[11] = { 550, 550, 700, 700, 850, 850, 850, 700, 950, 600, 900 };
static void war_place_nodes(BwSim *S, const float (*nd)[2], const int *na, int nn, float sx, float sy){
    S->war_nn = nn;
    for (int n=0;n<nn;n++){ S->war_nx[n]=WAR_OX+nd[n][0]*sx; S->war_ny[n]=WAR_OY+nd[n][1]*sy; S->war_namt[n]=na[n]; }
}
// the shared battle build: seed, purse, Core, starting units,
// hives (banded), terrain, fog, camera, banner. The caller has
// already cleared the field, set the field geometry (war_aw /
// war_ah / gw / gh / lines / skirmish) and placed the bit nodes —
// terrain generation tests against the nodes and hives, so the
// nodes must be down before this runs.
static void war_build_battle(int pi, int region, int bits, int nh){
    BwSim *S = &BWS[pi];
    // v1.18 (Daniel: "novel every time you play"): this battle's
    // seed mixes the region, the run clock and a per-process
    // battle counter — RESTARTs included — through the unsigned
    // hash, so no two starts ever lay out the same field. The seed
    // itself is never saved: hives are structs and the terrain
    // rides the battle snapshot, so a parked battle restores its
    // own rolled field exactly.
    { static u32 war_battle_ctr = 0;
      war_battle_ctr += 1u;
      u32 tbits = (u32)(int)(G.time * 4.0f);
      S->war_bseed = hash2((i32)(tbits + (u32)region * 977u), (i32)(war_battle_ctr * 131u + (u32)region), 0x5EEDu ^ (war_battle_ctr * 2654435761u)); }
    if (S->war_seedforce) S->war_bseed = (u32)S->war_seedforce;   // test hook (dbg_warseed): a pinned seed lays out the same field every battle — wartest only; zero in shipped play, so the roll above is untouched there
    S->war_state=1; S->war_region=region; S->war_bits=bits; S->war_t=0;
    // v1.19 addendum (Daniel): the FIRST wave is paced — at least
    // 45s of breathing room before the hive sends anything, so a
    // fresh commander can get the economy standing. Only the
    // first: the wave-spawn reset keeps the normal interval, and a
    // restored parked battle keeps its saved war_wavet.
    { float fw = war_wave_dt(region)*1.5f; S->war_wavet = fw<45.0f? 45.0f : fw; }
    S->war_grace=0.35f; S->war_ordt=0; S->war_msgt=0;
    S->war_ltap_t=-10.0f; S->war_ltap_unit=-1; S->war_drag_id=-1;
    float corey = S->war_ah*0.5f;   // the Core anchors the west edge at mid-height on any field
    war_spawn_struct(S, WAR_S_HQ, 0, WAR_OX+280, WAR_OY+corey);       // struct 0 is always the Core
    war_spawn_unit(S, WAR_U_HARV, WAR_OX+336, WAR_OY+corey+52.0f);
    war_spawn_unit(S, WAR_U_MELEE, WAR_OX+336, WAR_OY+corey-52.0f);
    // v1.18: the hives land along the FAR side, banded-random — the
    // field height is cut into nh equal bands and each hive rolls
    // its spot inside its own band (x deep in the east, y anywhere
    // in the band), so the swarm is always spread out but never
    // where it stood last time. No more "just go to the far right".
    // (At the campaign's 1600x1200: x 1240..1390, margin 130.)
    { float band_h = S->war_ah / (float)nh;
      float hmargin = S->war_ah*(130.0f/1200.0f);
      for (int h=0;h<nh;h++){
          float hx = (S->war_aw-360.0f) + hash_f(h, 11, S->war_bseed) * 150.0f;
          float hy = (float)h * band_h + hmargin + hash_f(h, 12, S->war_bseed) * (band_h - 2.0f*hmargin);
          int hi = war_spawn_struct(S, WAR_S_HIVE, 1, WAR_OX+hx, WAR_OY+hy);
          war_spawn_bug(S, hi, 0); war_spawn_bug(S, hi, 0);
          if (region>=2) war_spawn_bug(S, hi, 0);
          if (hi>=0){   // v1.21: every hive opens its purse and fields its workforce — two bug harvesters, free at the start like the garrison
              S->war_hbits[hi] = war_hive_start_bits(S);
              war_spawn_bharv(S, hi); war_spawn_bharv(S, hi);
          }
      } }
    war_gen_terrain(S, S->war_bseed);
    for (int i=0;i<WAR_FOG_W*WAR_FOG_H;i++) S->war_fog[i]=0;   // a fresh battle starts in the dark
    war_fog_update(S);
    S->war_camx = WAR_OX+280; S->war_camy = WAR_OY+corey;
    war_clamp_cam(S);
    if (S->war_skirmish){
        static const char *SKN[4] = { "", "SMALL", "STANDARD", "LARGE" };
        static const char *SKD[4] = { "", "EASY", "NORMAL", "HARD" };
        char buf[72]; char *q=buf; q=sappend(q, "SKIRMISH - "); q=sappend(q, SKN[S->war_size]); q=sappend(q, " FIELD, "); q=sappend(q, SKD[S->war_diff]); q=sappend(q, " ENEMY. KILL THE HIVE"); if (nh>1) q=sappend(q, "S"); q=sappend(q, "."); *q=0;
        war_note(S, buf);
    } else {
        char buf[72]; char *q=buf; q=sappend(q, "CONQUER "); q=sappend(q, REGION_NAME[region]);
          q=sappend(q, "! HARVEST BITS. KILL THE HIVE"); if (nh>1) q=sappend(q, "S"); q=sappend(q, "."); *q=0;
          war_note(S, buf);
    }
    sfx(19);
}
static void war_start_battle(int pi, int region){
    BwSim *S = &BWS[pi];
    if (region<0) region=0; if (region>7) region=7;
    war_clear_field(S);   // resets to campaign geometry, war_skirmish=0
    war_place_nodes(S, WAR_ND, WAR_NA, 11, 1.0f, 1.0f);
    int nh = 1 + (region>=3? 1:0) + (region>=6? 1:0);
    war_build_battle(pi, region, 300, nh);   // v1.19: 300 starting bits (was 250)
    war_link_pull(pi);   // feat 13: a launch pulls the other local commander in
}
// v1.19: a SKIRMISH — an ad-hoc battle on the player's picked
// size and difficulty. It never touches war_prog, is never
// parked, and never disturbs a parked campaign battle (nothing
// here serializes or clears a snapshot). The difficulty doubles
// as the battle's region equivalent: every region-keyed strength
// (unit/hive HP, wave strength and pace, hive count banding)
// reads it exactly as it would a campaign region.
static void war_start_skirmish(int pi){
    BwSim *S = &BWS[pi];
    int size = (S->war_size>=1 && S->war_size<=3)? S->war_size : 2;
    int diff = (S->war_diff>=1 && S->war_diff<=3)? S->war_diff : 2;
    S->war_size = size; S->war_diff = diff;
    war_clear_field(S);
    S->war_skirmish = 1;
    if (size==1){ S->war_aw=1200.0f; S->war_ah=900.0f; S->war_lines=2; }
    else if (size==3){ S->war_lines=4; }
    else { S->war_lines=3; }
    S->war_gw = (int)(S->war_aw/40.0f); S->war_gh = (int)(S->war_ah/40.0f);
    int region = diff==1? 1 : diff==3? 6 : 3;   // EASY / NORMAL / HARD as region equivalents
    int bits = size==1? 250 : size==3? 400 : 300;
    if (size==1){
        // a subset of the standard table, scaled onto the small
        // field: both west nodes, the centre, both east nodes and
        // the east-midfield node — 4,250 bits
        static const int IDX[6] = { 0, 1, 4, 5, 6, 10 };
        float nd[6][2]; int na[6];
        for (int k=0;k<6;k++){ nd[k][0]=WAR_ND[IDX[k]][0]; nd[k][1]=WAR_ND[IDX[k]][1]; na[k]=WAR_NA[IDX[k]]; }
        war_place_nodes(S, (const float (*)[2])nd, na, 6, S->war_aw/1600.0f, S->war_ah/1200.0f);
    } else if (size==3){
        // the standard eleven plus two corner nodes — 9,500 bits,
        // a dense field
        float nd[13][2]; int na[13];
        for (int k=0;k<11;k++){ nd[k][0]=WAR_ND[k][0]; nd[k][1]=WAR_ND[k][1]; na[k]=WAR_NA[k]; }
        nd[11][0]=300.0f; nd[11][1]=150.0f; na[11]=500;
        nd[12][0]=1450.0f; nd[12][1]=900.0f; na[12]=800;
        war_place_nodes(S, (const float (*)[2])nd, na, 13, 1.0f, 1.0f);
    } else {
        war_place_nodes(S, WAR_ND, WAR_NA, 11, 1.0f, 1.0f);
    }
    int nh = diff;              // EASY 1 / NORMAL 2 / HARD 3 hives
    if (size==1 && nh>2) nh=2;  // a SMALL field caps at 2
    war_build_battle(pi, region, bits, nh);
    war_link_pull(pi);   // feat 13: a launch pulls the other local commander in
}
static float war_axis(float a);   // defined with the pad code below; the pause menu's pad handler needs it early
// --- battle snapshots (v1.17, Daniel: leaving must SAVE the battle
// and pause it) -------------------------------------------------------
// The canonical text lives in war_snap[pi] (save section): keys wb /
// wu / ws / wg / wt / wf / wh in the "cw" idiom — ',' inside a record,
// ';' between records, one space between keys. Coordinates are
// arena-relative ints EXCEPT the rally (absolute, so 0 stays "unset").
// The save writer emits the text verbatim; the loader rebuilds it;
// restore parses it back into the BwSim. Projectiles, attack
// cooldowns, selection and scorch marks are transient — never saved.
static void war_snap_serialize(int pi){
    BwSim *S = &BWS[pi];
    char *p = war_snap[pi];
    p = put_key(p, "wb");
    p = put_int(p, S->war_region); *p++=','; *p=0;
    p = put_int(p, S->war_bits); *p++=','; *p=0;
    p = put_int(p, (int)S->war_t); *p++=','; *p=0;
    p = put_int(p, (int)(S->war_wavet*10.0f + 0.5f)); *p++=','; *p=0;
    p = put_int(p, (int)(S->war_camx - WAR_OX)); *p++=','; *p=0;
    p = put_int(p, (int)(S->war_camy - WAR_OY));
    { int first=1;   // wu: ALIVE units only, slot-stable (dead slots stay dead — refs in utarget must land)
      for (int i=0;i<WAR_MAXU;i++){
          if (S->war_uhp[i]<=0) continue;
          if (first){ *p++=' '; *p=0; p = put_key(p, "wu"); first=0; } else { *p++=';'; *p=0; }
          p = put_int(p, i); *p++=','; *p=0;
          p = put_int(p, S->war_utype[i]); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_ux[i]-WAR_OX)); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_uy[i]-WAR_OY)); *p++=','; *p=0;
          p = put_int(p, (int)S->war_uhp[i]); *p++=','; *p=0;
          p = put_int(p, S->war_umaxhp[i]); *p++=','; *p=0;
          p = put_int(p, S->war_ustate[i]); *p++=','; *p=0;
          p = put_int(p, S->war_utarget[i]); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_utx[i]-WAR_OX)); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_uty[i]-WAR_OY)); *p++=','; *p=0;
          p = put_int(p, S->war_ucarry[i]); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_uharv[i]*10.0f + 0.5f));
      } }
    { int first=1;   // ws: ALIVE structs — queue rides as qlen + the queued types (addendum 2), rally last (addendum 3)
      for (int s=0;s<WAR_MAXS;s++){
          if (S->war_shp[s]<=0) continue;
          if (first){ *p++=' '; *p=0; p = put_key(p, "ws"); first=0; } else { *p++=';'; *p=0; }
          p = put_int(p, s); *p++=','; *p=0;
          p = put_int(p, S->war_stype[s]); *p++=','; *p=0;
          p = put_int(p, S->war_sside[s]); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_sx[s]-WAR_OX)); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_sy[s]-WAR_OY)); *p++=','; *p=0;
          p = put_int(p, (int)S->war_shp[s]); *p++=','; *p=0;
          p = put_int(p, S->war_smaxhp[s]); *p++=','; *p=0;
          p = put_int(p, S->war_qlen[s]); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_qt[s]*10.0f + 0.5f)); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_scd[s]*10.0f + 0.5f));
          for (int k=0;k<S->war_qlen[s];k++){ *p++=','; *p=0; p = put_int(p, S->war_queue[s][k]); }
          *p++=','; *p=0; p = put_int(p, (int)S->war_rallyx[s]);
          *p++=','; *p=0; p = put_int(p, (int)S->war_rallyy[s]);
      } }
    *p++=' '; *p=0; p = put_key(p, "wg");   // bit nodes: every slot, amounts may be 0 — indices matter
    for (int n=0;n<S->war_nn;n++){
        if (n){ *p++=';'; *p=0; }
        p = put_int(p, (int)(S->war_nx[n]-WAR_OX)); *p++=','; *p=0;
        p = put_int(p, (int)(S->war_ny[n]-WAR_OY)); *p++=','; *p=0;
        p = put_int(p, S->war_namt[n]);
    }
    *p++=' '; *p=0; p = put_key(p, "wt");   // terrain: ALL entries — WAR_TREF aims at these indices
    for (int t=0;t<S->war_tn;t++){
        if (t){ *p++=';'; *p=0; }
        p = put_int(p, (int)(S->war_tx[t]-WAR_OX)); *p++=','; *p=0;
        p = put_int(p, (int)(S->war_ty[t]-WAR_OY)); *p++=','; *p=0;
        p = put_int(p, S->war_ttype[t]); *p++=','; *p=0;
        p = put_int(p, S->war_trad[t]); *p++=','; *p=0;
        p = put_int(p, (int)S->war_thp[t]); *p++=','; *p=0;
        p = put_int(p, S->war_tmaxhp[t]);
    }
    *p++=' '; *p=0; p = put_key(p, "wf");   // fog: explored-or-visible as a 1200-bit set in 38 u32 words
    for (int w=0;w<38;w++){
        if (w){ *p++=','; *p=0; }
        u32 word=0;
        for (int b=0;b<32;b++){ int c=w*32+b; if (c<WAR_FOG_W*WAR_FOG_H && S->war_fog[c]) word |= (1u<<b); }
        p = put_int(p, (int)word);
    }
    { int first=1;   // wh (v1.21): each living hive's purse — slot, bits. The economy rides the park.
      for (int s=0;s<WAR_MAXS;s++){
          if (S->war_shp[s]<=0 || S->war_stype[s]!=WAR_S_HIVE) continue;
          if (first){ *p++=' '; *p=0; p = put_key(p, "wh"); first=0; } else { *p++=';'; *p=0; }
          p = put_int(p, s); *p++=','; *p=0;
          p = put_int(p, S->war_hbits[s]);
      } }
    *p=0;
    war_snap_on[pi]=1;
    war_snap_reg[pi]=S->war_region;
}
static const char *war_snap_for_save(int pi){   // the save writer's door: refresh from a live battle, then hand over the text (0 = nothing parked)
    // v1.18: a live battle in a DIFFERENT region than the parked one
    // must NOT re-serialize over the park — the autosave would
    // silently evict the saved battle from the file (Daniel's
    // playtest: SAVE & QUIT, tap a region, fresh fight, saved
    // fight gone). The parked text is returned as-is instead.
    // v1.19: a live SKIRMISH is never serialized at all — it is
    // ad-hoc by definition; whatever campaign battle is parked
    // (if any) is returned untouched.
    // feat 13: a GUEST's war_state mirrors the owner's battle for
    // rendering, but their sim holds no battle — serializing it
    // would park an empty field over the guest's own saved battle.
    if (BWS[pi].war_state==1 && !BWS[pi].war_guest && !BWS[pi].war_skirmish && !(war_snap_on[pi] && war_snap_reg[pi]!=BWS[pi].war_region)) war_snap_serialize(pi);
    return war_snap_on[pi]? war_snap[pi] : 0;
}
static const char *war_snap_val(const char *snap, const char *key){
    int kl = slen(key);
    const char *s = snap;
    while (*s){
        if ((s==snap || s[-1]==' ') && s[0]==key[0]){
            int ok=1; for (int i=0;i<kl;i++) if (s[i]!=key[i]){ ok=0; break; }
            if (ok && s[kl]=='=') return s+kl+1;
        }
        while (*s && *s!=' ') s++;
        if (*s==' ') s++;
    }
    return 0;
}
static int war_rd_int(const char **qp){   // one int, then step over exactly one separator (',' / ';' / ' ')
    const char *q = *qp;
    int v = parse_int(q);
    while (*q && *q!=',' && *q!=';' && *q!=' ') q++;
    if (*q==',' || *q==';') q++;
    *qp = q;
    return v;
}
static void war_snap_restore(int pi){
    BwSim *S = &BWS[pi];
    const char *snap = war_snap[pi];
    const char *v;
    war_clear_field(S);   // v1.19: also restores campaign geometry (war_skirmish=0, war_aw/war_ah = WAR_AW/WAR_AH) — a snapshot is always a campaign battle
    if ((v = war_snap_val(snap, "wb"))){
        S->war_region = war_rd_int(&v);
        S->war_bits = war_rd_int(&v);
        S->war_t = (float)war_rd_int(&v);
        S->war_wavet = war_rd_int(&v) * 0.1f;
        S->war_camx = WAR_OX + (float)war_rd_int(&v);
        S->war_camy = WAR_OY + (float)war_rd_int(&v);
    }
    if (S->war_region<0) S->war_region=0; if (S->war_region>7) S->war_region=7;
    if ((v = war_snap_val(snap, "wu"))){
        while (*v && *v!=' '){
            int idx = war_rd_int(&v);
            int type = war_rd_int(&v);
            int x = war_rd_int(&v), y = war_rd_int(&v);
            int hp = war_rd_int(&v), maxhp = war_rd_int(&v);
            int state = war_rd_int(&v), target = war_rd_int(&v);
            int tx = war_rd_int(&v), ty = war_rd_int(&v);
            int carry = war_rd_int(&v), harv = war_rd_int(&v);
            if (idx>=0 && idx<WAR_MAXU){
                S->war_utype[idx]=type; S->war_ux[idx]=WAR_OX+(float)x; S->war_uy[idx]=WAR_OY+(float)y;
                S->war_uhp[idx]=(float)hp; S->war_umaxhp[idx]=maxhp;
                S->war_ustate[idx]=state; S->war_utarget[idx]=target;
                S->war_utx[idx]=WAR_OX+(float)tx; S->war_uty[idx]=WAR_OY+(float)ty;
                S->war_ucarry[idx]=carry; S->war_uharv[idx]=harv*0.1f;
                S->war_uatk[idx]=0; S->war_usel[idx]=0;
                S->war_wpn[idx]=0; S->war_wpi[idx]=0; S->war_stallt[idx]=0; S->war_repath[idx]=0;
                S->war_stallx[idx]=S->war_ux[idx]; S->war_stally[idx]=S->war_uy[idx];
            }
        }
    }
    if ((v = war_snap_val(snap, "ws"))){
        while (*v && *v!=' '){
            int idx = war_rd_int(&v);
            int type = war_rd_int(&v), side = war_rd_int(&v);
            int x = war_rd_int(&v), y = war_rd_int(&v);
            int hp = war_rd_int(&v), maxhp = war_rd_int(&v);
            int qlen = war_rd_int(&v);
            int qt = war_rd_int(&v), scd = war_rd_int(&v);
            int qn = qlen<0? 0 : qlen>9? 9 : qlen;
            int qt_[9];
            for (int k=0;k<qn;k++) qt_[k] = war_rd_int(&v);
            int rx = war_rd_int(&v), ry = war_rd_int(&v);
            if (idx>=0 && idx<WAR_MAXS){
                S->war_stype[idx]=type; S->war_sside[idx]=side;
                S->war_sx[idx]=WAR_OX+(float)x; S->war_sy[idx]=WAR_OY+(float)y;
                S->war_shp[idx]=(float)hp; S->war_smaxhp[idx]=maxhp;
                S->war_qlen[idx]=qn;
                for (int k=0;k<qn;k++) S->war_queue[idx][k]=qt_[k];
                S->war_qt[idx]=qt*0.1f; S->war_scd[idx]=scd*0.1f;
                S->war_rallyx[idx]=(float)rx; S->war_rallyy[idx]=(float)ry;
            }
        }
    }
    // v1.21: hive purses. A v1.20 park carries no wh key — every
    // living hive then opens at the campaign starting purse (150),
    // exactly as if the battle had just been built; a v1.21 park
    // restores the purse it saved.
    for (int s=0;s<WAR_MAXS;s++)
        if (S->war_shp[s]>0 && S->war_stype[s]==WAR_S_HIVE) S->war_hbits[s]=150;
    if ((v = war_snap_val(snap, "wh"))){
        while (*v && *v!=' '){
            int idx = war_rd_int(&v);
            int purse = war_rd_int(&v);
            if (idx>=0 && idx<WAR_MAXS && S->war_shp[idx]>0 && S->war_stype[idx]==WAR_S_HIVE) S->war_hbits[idx]=purse;
        }
    }
    if ((v = war_snap_val(snap, "wg"))){
        int n=0;
        while (*v && *v!=' ' && n<16){   // v1.19: an 11-node field must restore whole (was capped at 10)
            S->war_nx[n]=WAR_OX+(float)war_rd_int(&v);
            S->war_ny[n]=WAR_OY+(float)war_rd_int(&v);
            S->war_namt[n]=war_rd_int(&v);
            n++;
        }
        S->war_nn=n;
    }
    if ((v = war_snap_val(snap, "wt"))){
        int t=0;
        while (*v && *v!=' ' && t<WAR_MAXT){
            S->war_tx[t]=WAR_OX+(float)war_rd_int(&v);
            S->war_ty[t]=WAR_OY+(float)war_rd_int(&v);
            S->war_ttype[t]=war_rd_int(&v);
            S->war_trad[t]=war_rd_int(&v);
            S->war_thp[t]=(float)war_rd_int(&v);
            S->war_tmaxhp[t]=war_rd_int(&v);
            S->war_tdeadt[t]=0;
            t++;
        }
        S->war_tn=t;
    }
    for (int c=0;c<WAR_FOG_W*WAR_FOG_H;c++) S->war_fog[c]=0;
    if ((v = war_snap_val(snap, "wf"))){
        for (int w=0;w<38;w++){
            if (!*v || *v==' ') break;
            u32 word = (u32)war_rd_int(&v);
            for (int b=0;b<32;b++){ int c=w*32+b; if (c<WAR_FOG_W*WAR_FOG_H && ((word>>b)&1u)) S->war_fog[c]=1; }
        }
    }
    S->war_state=1; S->war_pause=0; S->war_bcat=-1;
    S->war_grace=0.35f; S->war_ordt=0; S->war_msgt=0; S->war_endt=0;
    S->war_ltap_t=-10.0f; S->war_ltap_unit=-1; S->war_drag_id=-1;
    war_clamp_cam(S);
    war_fog_update(S);   // stored cells come back explored; this re-marks the visible ones
    war_note(S, "BATTLE RESTORED. WELCOME BACK, COMMANDER.");
}
// --- the pause menu (v1.17) ------------------------------------------
// Esc in a battle no longer abandons it: the battle freezes behind a
// three-choice panel — RESUME, RESTART BATTLE, SAVE & QUIT. One set
// of rects, shared by the renderer and every input path (tap, pad
// cursor, keys 1-3), so they can never disagree about the buttons.
static void war_pause_rects(int vx, int vy, int vw2, int vh2, int *px, int *py, int *pw, int *ph){
    *pw = vw2<420? vw2-24 : 396; *ph = 196;
    *px = vx+(vw2-*pw)/2; *py = vy+(vh2-*ph)/2 - 10;
}
static int war_pause_btn_y(int py, int n){ return py + 56 + n*44; }   // three 36px buttons, 8px apart
static void war_pause_pick(int pi, int n){
    if (BWS[pi].war_guest) pi = BWS[pi].war_guest-1;   // feat 13: a guest pauser's picks act on the battle (the owner's sim)
    BwSim *S = &BWS[pi];
    if (n==0){ S->war_pause=0; S->war_grace=0.3f; sfx(6); }   // RESUME (the grace keeps the closing press from leaking an order)
    else if (S->war_skirmish && n==1){   // v1.19: RESTART a skirmish — re-roll the same size/difficulty; NO snapshot clear (a campaign park is never this fight's to discard)
        S->war_pause=0;
        war_start_skirmish(pi);
    }
    else if (S->war_skirmish && n==2){   // v1.19: QUIT BATTLE — the skirmish is ad-hoc: nothing serialized, nothing saved, back to the map
        S->war_pause=0;
        war_clear_field(S);
        S->war_state=0; S->war_grace=0.3f;
        war_note(S, "SKIRMISH ABANDONED.");
        if (S->war_duo>=0){ int g=S->war_duo; war_link_break(pi); BWS[g].war_state=0; BWS[g].war_grace=0.3f; war_note(&BWS[g], "THE COMMANDER LEFT THE BATTLE."); }   // feat 13: the guest goes back to their own map too
        sfx(6);
    }
    else if (n==1){   // RESTART BATTLE: this region, fresh, and no snapshot of the old fight survives — unless the park belongs to another region (v1.18 scoping, as in war_win/war_lose)
        if (!war_snap_on[pi] || war_snap_reg[pi]==S->war_region) war_snap_clear(pi);
        S->war_pause=0;
        war_start_battle(pi, S->war_region);
    } else if (n==2){   // SAVE & QUIT: park the battle, persist, back to the map
        war_snap_serialize(pi);
        do_save();
        S->war_pause=0;
        war_clear_field(S);
        S->war_state=0; S->war_grace=0.3f;
        war_note(S, "BATTLE SAVED. IT WILL WAIT FOR YOU.");
        if (S->war_duo>=0){ int g=S->war_duo; war_link_break(pi); BWS[g].war_state=0; BWS[g].war_grace=0.3f; war_note(&BWS[g], "THE COMMANDER SAVED THE BATTLE."); }   // feat 13: the guest goes back to their own map too
        sfx(19);
    }
}
static void war_pause_tap(int pi, int sx, int sy){
    int vx, vy, vw2, vh2; pi_view_rect(pi, &vx, &vy, &vw2, &vh2);
    int px, py, pw, ph; war_pause_rects(vx, vy, vw2, vh2, &px, &py, &pw, &ph);
    for (int n=0;n<3;n++){
        int by = war_pause_btn_y(py, n);
        if (sx>=px+40 && sx<px+pw-40 && sy>=by && sy<by+36){ war_pause_pick(pi, n); return; }
    }
    // a tap outside the panel does nothing: the menu is modal
}
// the pad in the pause menu: the cursor flies the frozen field, A
// presses through war_pause_tap, B resumes — war_pad_update's peel
// logic stays out of it
static void war_pad_pause(int pi, const float *ax, u32 pe, float dt){
    BwSim *S = war_bs(pi);   // feat 13: either commander can drive the pause with their own pad
    int vx, vy, vw2, vh2; pi_view_rect(pi, &vx, &vy, &vw2, &vh2);
    float lx = war_axis(ax[0]), ly = war_axis(ax[1]);
    int stick = (ax[0]>0.18f || ax[0]<-0.18f || ax[1]>0.18f || ax[1]<-0.18f);
    u32 btns = (1u<<ACT_FIRE)|(1u<<ACT_INTERACT);
    if (!S->war_curon){
        if (!war_pad_live(pi) || !(stick || (pe & btns))) return;
        S->war_curon = 1;
        S->war_curx = S->war_camx; S->war_cury = S->war_camy + WAR_BAR_H*0.5f;   // wake at the camera's centre, under the panel
        if (!stick) return;   // a button wake spends the press on waking
        pe = 0;
    }
    if (lx || ly){ S->war_curx += lx * 540.0f * dt; S->war_cury += ly * 540.0f * dt; }
    if (S->war_curx < WAR_OX) S->war_curx = WAR_OX;
    if (S->war_curx > WAR_OX+S->war_aw) S->war_curx = WAR_OX+S->war_aw;
    if (S->war_cury < WAR_OY) S->war_cury = WAR_OY;
    if (S->war_cury > WAR_OY+S->war_ah) S->war_cury = WAR_OY+S->war_ah;
    { float csx = (float)vx + vw2/2 + (S->war_curx - S->war_camx);   // no edge-follow: the camera holds still, the cursor stays on screen
      float csy = (float)vy + vh2/2 + (S->war_cury - S->war_camy - WAR_BAR_H*0.5f);
      if (csx < vx) S->war_curx += (vx - csx);
      if (csx > vx+vw2) S->war_curx -= (csx - (vx+vw2));
      if (csy < vy) S->war_cury += (vy - csy);
      if (csy > vy+vh2) S->war_cury -= (csy - (vy+vh2));
    }
    if (pe & (1u<<ACT_INTERACT)){ war_pause_pick(pi, 0); return; }   // B = RESUME
    if ((pe & (1u<<ACT_FIRE)) && S->war_grace<=0){
        int csx = vx + vw2/2 + (int)(S->war_curx - S->war_camx);
        int csy = vy + vh2/2 + (int)(S->war_cury - S->war_camy - WAR_BAR_H*0.5f);
        war_pause_tap(pi, csx, csy);
    }
}
static void war_enter(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    if (S->war_size<1 || S->war_size>3) S->war_size = 2;   // v1.19: the skirmish setup defaults — STANDARD / NORMAL, kept per player from then on
    if (S->war_diff<1 || S->war_diff>3) S->war_diff = 2;
    // feat 13: if the other local commander already owns a live
    // battle, join IT as guest co-commander instead of opening a
    // separate front door (your own parked battle stays parked)
    if (!S->war_guest && war_net_ok()){
        int o = war_other_local(pi);
        if (o>=0 && o!=pi){
            BwSim *OS = &BWS[o];
            if (!OS->war_guest && OS->war_state==1 && G.bud[o].active && G.bud[o].in_bw && G.bud[o].bw_mode==8){
                war_link_form(o, pi);
                b->x = WAR_OX+800; b->y = WAR_OY+600;   // parked mid-arena; this mode never draws him
                b->bw_x = b->x; b->bw_y = b->y;
                return;
            }
        }
    }
    if (war_snap_on[pi]){   // v1.17: a parked battle resumes instead of the bare map
        war_snap_restore(pi);
        war_snap_clear(pi);   // consumed — the live BwSim is authoritative now; later saves re-serialize from it
        war_link_pull(pi);   // feat 13: a resumed battle pulls a waiting commander in
        b->x = WAR_OX+800; b->y = WAR_OY+600;   // parked mid-arena; this mode never draws him
        b->bw_x = b->x; b->bw_y = b->y;
        return;
    }
    war_clear_field(S);
    S->war_state=0;
    S->war_map_sel = b->war_prog<8? b->war_prog : 7;
    S->war_grace=0.35f; S->war_msgt=0; S->war_region=0; S->war_bits=0;
    b->x = WAR_OX+800; b->y = WAR_OY+600;   // parked mid-arena; this mode never draws him
    b->bw_x = b->x; b->bw_y = b->y;
}
// the command bar: one list of actions, shared by the renderer and
// the tap handler so they can never disagree about what's on offer.
// acts: 0 train harvester, 1-3 place patch/zapper/firewall,
// 4 train melee, 5 train ranged, 6 cancel placement, 7 hold position,
// 8 train scout, 9 train siege tank, 10 place assembly plant,
// 11 place gate, 12 train patcher (v1.19 addendum),
// 13 demolish (v1.21: sell a selected building back for half its
// build cost, two-step arm + confirm; the Core is never offered it),
// 20/21/22 the TRAIN / BUILD / DEFENSE categories.
// v1.17 (addendum 1): structure contexts are TWO-LEVEL — the bar
// shows categories; opening one puts its options in a strip directly
// above the bar. war_bar_list stays the single source: it returns
// the CURRENT row (the open category's options, else the top level).
// A building with a single category (Patch Bay, Assembly Plant)
// starts OPEN — no two-tap tax for a menu of one.
static int war_cat_items(int st, int cat, int acts[5], int costs[5], const char *names[5]){
    int n=0;
    if (cat==0){   // TRAIN
        if (st==WAR_S_HQ){ acts[n]=0; costs[n]=WAR_TRAIN_COST[0]; names[n]="HARVESTER"; n++; }
        else if (st==WAR_S_PATCH){ acts[n]=4; costs[n]=WAR_TRAIN_COST[1]; names[n]="WRENCH"; n++;
                                   acts[n]=5; costs[n]=WAR_TRAIN_COST[2]; names[n]="BOLT"; n++;
                                   acts[n]=12; costs[n]=WAR_TRAIN_COST[7]; names[n]="PATCHER"; n++; }
        else if (st==WAR_S_ASM){ acts[n]=8; costs[n]=WAR_TRAIN_COST[5]; names[n]="SCOUT BUGGY"; n++;
                                 acts[n]=9; costs[n]=WAR_TRAIN_COST[6]; names[n]="SIEGE TANK"; n++; }
    } else if (cat==1){   // BUILD
        if (st==WAR_S_HQ){ acts[n]=1; costs[n]=WAR_BUILD_COST[1]; names[n]="PATCH BAY"; n++;
                           acts[n]=10; costs[n]=WAR_BUILD_COST[5]; names[n]="ASSEMBLY"; n++; }
    } else {   // DEFENSE
        if (st==WAR_S_HQ){ acts[n]=2; costs[n]=WAR_BUILD_COST[2]; names[n]="ZAPPER"; n++;
                           acts[n]=3; costs[n]=WAR_BUILD_COST[3]; names[n]="FIREWALL"; n++;
                           acts[n]=11; costs[n]=WAR_BUILD_COST[6]; names[n]="GATE"; n++; }
    }
    return n;
}
// the category row itself: what the main bar shows while a category
// is open in the strip above it
static int war_bar_cats(BwSim *S, int acts[5], int costs[5], const char *names[5]){
    int n=0;
    if (S->war_selstruct>=0 && war_struct_alive(S, S->war_selstruct)){
        int st = S->war_stype[S->war_selstruct];
        int ta[5], tc[5]; const char *tn[5];
        for (int c=0;c<3;c++) if (war_cat_items(st, c, ta, tc, tn)>0){
            acts[n]=20+c; costs[n]=0; names[n]= c==0? "TRAIN" : c==1? "BUILD" : "DEFENSE"; n++;
        }
    }
    return n;
}
static int war_bar_list(BwSim *S, int acts[5], int costs[5], const char *names[5]){
    if (S->war_place>=0){ acts[0]=6; costs[0]=0; names[0]="CANCEL"; return 1; }
    int n=0;
    if (S->war_selstruct>=0 && war_struct_alive(S, S->war_selstruct)){
        int st = S->war_stype[S->war_selstruct];
        // v1.21: DEMOLISH rides at the end of a selected building's
        // own options — any building you own except the Compiler
        // Core itself. For a building with no categories at all
        // (Zapper, Firewall, Gate) it is the whole menu.
        int demo = (S->war_sside[S->war_selstruct]==0 && st!=WAR_S_HQ);
        if (S->war_bcat>=0){
            int m = war_cat_items(st, S->war_bcat, acts, costs, names);
            if (m>0){
                if (demo && m<5){ acts[m]=13; costs[m]=0; names[m]= (S->war_demoarm==S->war_selstruct)? "CONFIRM?" : "DEMOLISH"; m++; }
                return m;
            }
            S->war_bcat=-1;   // the open category has nothing for this building: fall back to the top level
        }
        int nc = war_bar_cats(S, acts, costs, names);
        if (nc==1){   // a menu of one opens itself
            S->war_bcat = acts[0]-20;
            int m = war_cat_items(st, S->war_bcat, acts, costs, names);
            if (demo && m<5){ acts[m]=13; costs[m]=0; names[m]= (S->war_demoarm==S->war_selstruct)? "CONFIRM?" : "DEMOLISH"; m++; }
            return m;
        }
        if (nc==0 && demo){ acts[0]=13; costs[0]=0; names[0]= (S->war_demoarm==S->war_selstruct)? "CONFIRM?" : "DEMOLISH"; return 1; }
        return nc;
    }
    if (war_sel_count(S)>0){ acts[n]=7; costs[n]=0; names[n]="HOLD"; n++; }
    return n;
}
static void war_bar_activate(int pi, int slot){
    BwSim *S = war_bs(pi);   // feat 13: a guest activates the OWNER's bar — shared bits, shared queues
    int acts[5], costs[5]; const char *names[5];
    int n = war_bar_list(S, acts, costs, names);
    if (slot<0 || slot>=n) return;
    int act = acts[slot], cost = costs[slot];
    if (act!=13) S->war_demoarm=-1;   // v1.21: doing anything else with the bar disarms a pending DEMOLISH
    if (act==13){   // DEMOLISH: two-step — the first activation arms, the second pays out and tears it down
        int s = S->war_selstruct;
        if (s<0 || !war_struct_alive(S, s) || S->war_sside[s]!=0 || S->war_stype[s]==WAR_S_HQ) return;
        if (S->war_demoarm != s){
            S->war_demoarm = s;
            war_note(S, "DEMOLISH? ACTIVATE AGAIN TO CONFIRM");
            sfx(6); return;
        }
        int refund = WAR_BUILD_COST[S->war_stype[s]]/2;   // half the build cost, floored: Patch Bay 75, Assembly 100, Zapper 50, Firewall 12, Gate 17
        S->war_bits += refund;
        S->war_shp[s]=0; S->war_qlen[s]=0; S->war_qt[s]=0; S->war_rallyx[s]=0; S->war_rallyy[s]=0;   // the normal removal path: footprint freed, queue cancelled (no refund for queued units), rally cleared
        if (S->war_selstruct==s) S->war_selstruct=-1;
        S->war_demoarm=-1; S->war_bcat=-1;
        char nb[48]; char *nq=nb; nq=sappend(nq, "DEMOLISHED! +"); nq=sapp_int(nq, refund); nq=sappend(nq, " BITS BACK."); *nq=0;
        war_note(S, nb);
        sfx(17); return;
    }
    if (act==6){ S->war_place=-1; S->war_bcat=-1; sfx(6); return; }
    if (act==7){
        for (int i=0;i<WAR_MAXU;i++)
            if (S->war_uhp[i]>0 && S->war_usel[i] && S->war_utype[i]!=WAR_U_HARV){ S->war_ustate[i]=0; S->war_utarget[i]=0; S->war_urepair[i]=0; }
        sfx(6); return;
    }
    if (act>=20 && act<=22){ S->war_bcat = act-20; sfx(6); return; }   // opening a category costs nothing, trains nothing, builds nothing
    if (act==0 || act==4 || act==5 || act==8 || act==9 || act==12){   // TRAIN: pay-to-queue (addendum 2)
        int s = S->war_selstruct;
        if (s<0 || !war_struct_alive(S, s)) return;
        if (S->war_qlen[s]>=9){ sfx(16); war_note(S, "QUEUE FULL (9)."); return; }
        if (S->war_bits < cost){ sfx(16); war_note(S, "NOT ENOUGH BITS TO QUEUE THAT."); return; }
        int ut = act==0? WAR_U_HARV : act==4? WAR_U_MELEE : act==5? WAR_U_RANGED : act==8? WAR_U_SCOUT : act==9? WAR_U_SIEGE : WAR_U_PATCH;
        S->war_bits -= cost;
        if (S->war_qlen[s]==0) S->war_qt[s]=WAR_TRAIN_TIME[ut];   // an idle building starts its front item NOW — exactly the old feel
        S->war_queue[s][S->war_qlen[s]++]=ut;
        sfx(6); return;   // the category stays open: queue the next one
    }
    if (S->war_bits < cost){ sfx(16); war_note(S, "NOT ENOUGH BITS! SEND THE HARVESTER OUT."); return; }
    if (act==10){ S->war_place = WAR_S_ASM; S->war_bcat=-1; sfx(6); { char nb[96]; char *q=nb; q=sappend(q, war_place_verb()); q=sappend(q, " TO PLACE THE ASSEMBLY PLANT."); *q=0; war_note(S, nb); } return; }
    S->war_place = act==11? WAR_S_GATE : act;   // acts 1..3 are the struct kinds 1..3; act 11 arms the gate
    S->war_bcat=-1;   // arming a placement hands the bar to [CANCEL]
    sfx(6);
    { char nb[128]; char *q=nb; q=sappend(q, war_place_verb());
      q=sappend(q, act==1? " TO PLACE THE PATCH BAY." : act==2? " TO PLACE THE ZAPPER TOWER." : act==11? " TO PLACE THE GATE - UNITS PASS THROUGH IT. BUGS DON'T." : " TO PLACE THE FIREWALL."); *q=0; war_note(S, nb); }
}
// the property rule (Daniel, Nov 16): you build on your own land.
// Every friendly structure radiates a property zone — its radius + 76
// for buildings, + 52 for a firewall segment. Round 3 (Daniel, later
// that day: "make the build region more generous, and require only
// that the new build be adjacent to the current area"): a placement
// is legal when the new structure's FOOTPRINT touches the property —
// its centre within (zone radius + its own radius) of some friendly
// structure's centre. The build may sit mostly outside the zone as
// long as it touches it, and every placed structure extends the
// property, so a firewall line (or a chain of buildings) can grow
// outward step by step.
static float war_zone_rad(int kind){
    return (kind==WAR_S_WALL || kind==WAR_S_GATE)? (float)WAR_SRAD[kind] + 52.0f : (float)WAR_SRAD[kind] + 76.0f;   // a gate is wall family: it radiates the wall's +52 zone, not a building's +76
}
static int war_in_property(BwSim *S, int kind, float x, float y){
    float own = (float)WAR_SRAD[kind];
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0 || S->war_sside[s]!=0) continue;
        float dx=x-S->war_sx[s], dy=y-S->war_sy[s], rr = war_zone_rad(S->war_stype[s]) + own;
        if (dx*dx+dy*dy <= rr*rr) return 1;
    }
    return 0;
}
// physical room only: inside the arena, not on top of anything, not
// on a bit node. (The property rule is separate, above.)
static int war_place_space(BwSim *S, int kind, float x, float y){
    if (x < WAR_OX+36 || x > WAR_OX+S->war_aw-36 || y < WAR_OY+36 || y > WAR_OY+S->war_ah-36) return 0;
    float rad = (float)WAR_SRAD[kind];
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0) continue;
        float dx=x-S->war_sx[s], dy=y-S->war_sy[s];
        // firewall segments are meant to stand shoulder to shoulder on
        // the 24u grid: wall-family on wall-family (walls AND gates,
        // either way round) only needs its own centre clear — the
        // full radii sum (13+13+8 = 34) would reject the 24u spacing
        // a gate in a wall line depends on
        int wallfam_new = (kind==WAR_S_WALL || kind==WAR_S_GATE);
        int wallfam_old = (S->war_stype[s]==WAR_S_WALL || S->war_stype[s]==WAR_S_GATE);
        float rr = (wallfam_new && wallfam_old)? 20.0f
                 : rad + WAR_SRAD[S->war_stype[s]] + 8.0f;
        if (dx*dx+dy*dy < rr*rr) return 0;
    }
    for (int n=0;n<S->war_nn;n++){
        if (S->war_namt[n]<=0) continue;
        float dx=x-S->war_nx[n], dy=y-S->war_ny[n], rr = rad + 26.0f;
        if (dx*dx+dy*dy < rr*rr) return 0;
    }
    for (int t=0;t<S->war_tn;t++){
        if (S->war_thp[t]<=0) continue;   // cleared ground can be built on
        float dx=x-S->war_tx[t], dy=y-S->war_ty[t], rr = rad + (float)S->war_trad[t] + 4.0f;
        if (dx*dx+dy*dy < rr*rr) return 0;
    }
    return 1;
}
static int war_place_ok(BwSim *S, int kind, float x, float y){
    return war_place_space(S, kind, x, y) && war_in_property(S, kind, x, y);
}
static float war_snap24(float w, float origin){
    float l = w - origin;
    return origin + (float)((int)(l/24.0f + 0.5f)) * 24.0f;
}
static void war_try_place(int pi, float wx, float wy){
    BwSim *S = &BWS[pi];
    int kind = S->war_place, cost = WAR_BUILD_COST[kind];
    if (S->war_bits < cost){ sfx(16); war_note(S, "NOT ENOUGH BITS!"); S->war_place=-1; S->war_bcat=-1; return; }
    if (kind==WAR_S_WALL || kind==WAR_S_GATE){ wx = war_snap24(wx, WAR_OX); wy = war_snap24(wy, WAR_OY); }   // wall-family segments align on the grid, so lines (and the doors in them) join up
    if (!war_place_space(S, kind, wx, wy)){ sfx(16); war_note(S, "NO ROOM THERE!"); return; }
    if (!war_in_property(S, kind, wx, wy)){ sfx(16); war_note(S, "TOO FAR FROM YOUR BASE - BUILD ON YOUR OWN LAND!"); return; }
    S->war_bits -= cost;
    int s = war_spawn_struct(S, kind, 0, wx, wy);
    S->war_place = -1; S->war_bcat=-1;
    for (int i=0;i<WAR_MAXU;i++) S->war_usel[i]=0;
    S->war_selstruct = s;
    sfx(19);
    war_note(S, kind==1? "PATCH BAY ONLINE! IT TRAINS FIGHTERS." : kind==2? "ZAPPER TOWER ONLINE!" : kind==5? "ASSEMBLY PLANT ONLINE! IT BUILDS VEHICLES." : kind==6? "GATE UP! YOUR UNITS PASS - BUGS DON'T." : "FIREWALL UP!");
}
static void war_select_unit(BwSim *S, int i, int all_of_type){
    for (int j=0;j<WAR_MAXU;j++) S->war_usel[j]=0;
    S->war_selstruct = -1; S->war_bcat = -1;   // a selection change closes any open category
    if (all_of_type){
        int t = S->war_utype[i];
        for (int j=0;j<WAR_MAXU;j++){
            if (S->war_uhp[j]<=0 || S->war_utype[j]!=t) continue;
            float dx=S->war_ux[j]-S->war_camx, dy=S->war_uy[j]-S->war_camy;
            if (dx>-360 && dx<360 && dy>-220 && dy<220) S->war_usel[j]=1;   // on screen, with a margin
        }
        if (!S->war_usel[i]) S->war_usel[i]=1;
    } else S->war_usel[i]=1;
    sfx(6);
}
// the third tap of a streak: EVERY combat unit you own — melee,
// ranged and both vehicles, map-wide. Harvesters stay out of it.
static void war_select_all_combat(BwSim *S){
    for (int j=0;j<WAR_MAXU;j++)
        S->war_usel[j] = (S->war_uhp[j]>0 && war_u_side(S->war_utype[j])==0 && war_u_combat(S->war_utype[j]))? 1 : 0;
    S->war_selstruct = -1; S->war_bcat = -1;
    sfx(6);
}
static void war_order_move(BwSim *S, float wx, float wy){
    int ord = 0;
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0 || !S->war_usel[i]) continue;
        float ox = (float)(ord%4 - 1.5f) * 16.0f, oy = (float)((ord/4)%3 - 1) * 16.0f;
        ord++;
        S->war_ustate[i]=1; S->war_utarget[i]=0;
        S->war_utx[i]=wx+ox; S->war_uty[i]=wy+oy;
        S->war_urepair[i]=0;   // a fresh order supersedes repair work (v1.19 addendum)
        S->war_wpn[i]=0;   // a fresh order drops any old path (v1.17)
    }
}
static void war_order_attack(BwSim *S, int eref){
    float tx, ty;
    if (!war_ref_alive(S, eref, &tx, &ty)) return;
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0 || !S->war_usel[i]) continue;
        if (S->war_utype[i]==WAR_U_HARV || S->war_utype[i]==WAR_U_PATCH){ S->war_ustate[i]=1; S->war_utarget[i]=0; S->war_utx[i]=tx; S->war_uty[i]=ty; }   // the weaponless just walk there — the barrier-tap precedent: an enemy/debris tap is a move order to a patcher, never an attack
        else { S->war_ustate[i]=2; S->war_utarget[i]=eref; }
        S->war_urepair[i]=0;   // a fresh order supersedes repair work (v1.19 addendum)
        S->war_wpn[i]=0;   // a fresh order drops any old path (v1.17)
    }
}
// the heart of every tap: a point on the battlefield (world coords)
static void war_tap_world(int pi, float wx, float wy){
    BwSim *S = war_bs(pi);   // feat 13: orders land on the shared army, whoever gives them
    if (S->war_place>=0){ war_try_place(pi, wx, wy); return; }
    int bu=-1; float bd=20.0f*20.0f;
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0 || war_u_side(S->war_utype[i])!=0) continue;
        float dx=wx-S->war_ux[i], dy=wy-S->war_uy[i], d2=dx*dx+dy*dy;
        if (d2<bd){ bd=d2; bu=i; }
    }
    if (bu>=0){
        // the tap streak (Daniel's ladder): one tap selects the unit,
        // a second inside the window selects its whole type on screen,
        // a third escalates to every combat unit you own — and further
        // taps inside the same streak hold that full selection.
        int streak = (S->war_ltap_unit==bu && S->war_t - S->war_ltap_t < 0.35f)? S->war_ltap_n + 1 : 1;
        S->war_ltap_unit=bu; S->war_ltap_t=S->war_t; S->war_ltap_n=streak;
        if (streak>=3) war_select_all_combat(S);
        else war_select_unit(S, bu, streak==2);
        return;
    }
    S->war_ltap_unit=-1; S->war_ltap_n=0;   // any other tap ends the streak
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0 || S->war_sside[s]!=0) continue;
        float dx=wx-S->war_sx[s], dy=wy-S->war_sy[s], rr=(float)WAR_SRAD[S->war_stype[s]]+8.0f;
        if (dx*dx+dy*dy < rr*rr){
            // v1.19 addendum: patchers in the selection + a DAMAGED
            // friendly structure = a REPAIR order, not a selection
            // change. Other selected units keep their orders,
            // exactly as a friendly-structure tap always left them.
            // A full-health structure falls through to the selection
            // behaviour below — there is nothing to fix.
            if (S->war_shp[s] < (float)S->war_smaxhp[s]){
                int anyp = 0;
                for (int i=0;i<WAR_MAXU;i++)
                    if (S->war_uhp[i]>0 && S->war_usel[i] && S->war_utype[i]==WAR_U_PATCH){
                        S->war_urepair[i]=s+1; S->war_ustate[i]=0; S->war_utarget[i]=0; S->war_wpn[i]=0; anyp = 1;
                    }
                if (anyp){ S->war_ordx=wx; S->war_ordy=wy; S->war_ordt=0.5f; sfx(6); return; }
            }
            if (s == S->war_selstruct){
                // v1.19 addendum (Daniel): tapping the selected
                // building itself toggles it OFF — the same cleanup
                // as every other deselect path (the pad retires its
                // bar highlight on the selection change by itself)
                S->war_selstruct=-1; S->war_bcat=-1; sfx(6); return;
            }
            for (int i=0;i<WAR_MAXU;i++) S->war_usel[i]=0;
            S->war_selstruct=s; S->war_bcat=-1; sfx(6); return;   // a selection change closes any open category (a lone category reopens itself in the bar)
        }
    }
    int eref=0; float ed=1e18f, edist=1e18f;
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0 || war_u_side(S->war_utype[i])!=1) continue;
        float dx=wx-S->war_ux[i], dy=wy-S->war_uy[i], d2=dx*dx+dy*dy;
        if (d2<18.0f*18.0f && d2<ed){ ed=d2; edist=fsqrt(d2); eref=i+1; }
    }
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0 || S->war_sside[s]!=1) continue;
        float dx=wx-S->war_sx[s], dy=wy-S->war_sy[s], rr=(float)WAR_SRAD[S->war_stype[s]]+8.0f;
        if (dx*dx+dy*dy < rr*rr){ eref=-(s+1); edist=fsqrt(dx*dx+dy*dy); break; }
    }
    // debris (Daniel, Nov 16): a tap on a terrain feature is an attack
    // order on it — your combat units can clear the field — unless an
    // enemy unit or structure sits even closer to the tap point.
    // Harvesters in the selection just walk there, as for any tap.
    { int fref=0; float fdist=1e18f;
      for (int t=0;t<S->war_tn;t++){
          if (S->war_thp[t]<=0) continue;
          if (S->war_ttype[t]==5) continue;   // v1.18: barriers are NOT attack targets — a tap on one falls through to a plain move order below
          float dx=wx-S->war_tx[t], dy=wy-S->war_ty[t], rr=(float)S->war_trad[t]+6.0f, d2=dx*dx+dy*dy;
          if (d2 < rr*rr){ float d=fsqrt(d2); if (d<fdist){ fdist=d; fref=WAR_TREF(t); } }
      }
      if (fref && (!eref || fdist < edist)){
          if (war_sel_count(S)>0){
              war_order_attack(S, fref);
              S->war_ordx=wx; S->war_ordy=wy; S->war_ordt=0.5f; sfx(6);
          }
          return;
      } }
    if (eref){
        if (war_sel_count(S)>0){
            war_order_attack(S, eref);
            S->war_ordx=wx; S->war_ordy=wy; S->war_ordt=0.5f; sfx(6);
        }
        return;
    }
    if (war_sel_count(S)>0){
        war_order_move(S, wx, wy);
        S->war_ordx=wx; S->war_ordy=wy; S->war_ordt=0.5f; sfx(6);
    } else if (S->war_selstruct>=0){
        int ss = S->war_selstruct;
        int st = war_struct_alive(S, ss)? S->war_stype[ss] : -1;
        if (st==WAR_S_HQ || st==WAR_S_PATCH || st==WAR_S_ASM){
            // rally point (addendum 3): a field tap with a PRODUCER
            // selected plants its flag instead of deselecting — the
            // building stays selected, fresh units walk to the flag
            S->war_rallyx[ss]=wx; S->war_rallyy[ss]=wy;
            S->war_ordx=wx; S->war_ordy=wy; S->war_ordt=0.5f;   // the order-marker flash, reused
            sfx(6);
        } else { S->war_selstruct=-1; S->war_bcat=-1; sfx(6); }   // a non-producer just deselects, as always
    }
}
static void war_handle_tap(int pi, int sx, int sy){
    BwSim *S = war_bs(pi);   // feat 13: the tap acts on the shared battle, in the TAPPER's view
    if (S->war_pause){ war_pause_tap(pi, sx, sy); return; }   // v1.17: the pause menu is modal — the tap never reaches the field
    int vx, vy, vw2, vh2; pi_view_rect(pi, &vx, &vy, &vw2, &vh2);
    if (S->war_bcat>=0 && sy >= vy+vh2-WAR_BAR_H-WAR_SUB_H && sy < vy+vh2-WAR_BAR_H){
        // the options strip (addendum 1): it holds the row war_bar_list
        // describes. WAR_BAR_H is untouched — the world view never
        // resizes; the strip simply owns these taps while it is drawn.
        if (sx>=vx && sx<vx+vw2){
            int rel = sx - vx - 6;
            int slot = rel/90;
            if (rel>=0 && slot<5 && (rel%90)<84) war_bar_activate(pi, slot);
        }
        return;
    }
    if (sy >= vy+vh2-WAR_BAR_H){   // the command bar owns the bottom strip
        if (sx>=vx && sx<vx+vw2){
            int rel = sx - vx - 6;
            int slot = rel/90;
            if (rel>=0 && slot<5 && (rel%90)<84){
                if (S->war_bcat>=0){
                    // a category is open above: the bar row itself IS
                    // the categories — tapping one switches the strip
                    int ca[5], cc[5]; const char *cn[5];
                    int cn2 = war_bar_cats(S, ca, cc, cn);
                    if (slot<cn2){ S->war_bcat = ca[slot]-20; S->war_barhl=-1; sfx(6); }
                } else war_bar_activate(pi, slot);
            }
        }
        return;
    }
    float wx = S->war_camx + (float)(sx - vx - vw2/2);
    float wy = S->war_camy + WAR_BAR_H*0.5f + (float)(sy - vy - vh2/2);
    war_tap_world(pi, wx, wy);
}
// Grab-the-world panning (reworked Nov 16, Daniel: the old pan
// "didn't expect what was actually happening"). The old code anchored
// on wherever the finger happened to be when the touch crossed the
// 14px "moved" threshold, so the world trailed the finger by that
// much on every drag — and on the STICK scheme it never ran at all,
// leaving the field stick to shove the camera at a fixed 430 u/s
// through BwIn keys. Now the anchor is the DOWN point (tp x0/y0) and
// the camera as it stood when the finger landed: cam = anchor -
// (finger - down), so the world point grabbed stays under the finger
// from the first pixel, on touch and mouse alike (same channel).
// Returns 1 while a pan drag owns the field (the caller then skips
// the key/stick camera pan, so the two never fight).
static int war_drag_pan(int pi){
    BwSim *S = war_bs(pi);   // feat 13: under the guest swap this is the battle, wearing the guest's view
    int found=-1;
    for (int i=0;i<6;i++){
        if (!IN.tp[i].active) continue;
        if (S->war_drag_id>=0 && IN.tp[i].id != S->war_drag_id) continue;   // one drag pointer at a time
        if (IN.tp[i].zone!=0 && IN.tp[i].zone!=8) continue;                 // field touches only — buttons never pan
        found=i; break;
    }
    if (found<0){ S->war_drag_id=-1; return 0; }
    int vx, vy, vw2, vh2; pi_view_rect(pi, &vx, &vy, &vw2, &vh2);
    if (IN.tp[found].y0 >= vy+vh2-WAR_BAR_H){ S->war_drag_id=-1; return 0; }   // drags off the bar don't pan
    // v1.25: a drag is the finger/mouse taking the field back (last
    // device wins, the same rule a tap follows) — it retires the pad
    // cursor and any bar capture, so the pad's centred camera (see
    // war_pad_update) never fights a drag for the view.
    if (S->war_curon || S->war_barcap){ S->war_curon=0; S->war_barcap=0; S->war_barhl=-1; S->war_barhl_row=0; }
    if (S->war_drag_id != IN.tp[found].id){
        S->war_drag_id = IN.tp[found].id;
        S->war_drag_cx = S->war_camx; S->war_drag_cy = S->war_camy;
    }
    S->war_camx = S->war_drag_cx - (float)(IN.tp[found].x - IN.tp[found].x0);
    S->war_camy = S->war_drag_cy - (float)(IN.tp[found].y - IN.tp[found].y0);
    war_clamp_cam(S);
    return 1;
}
static void war_node_pos(int vx, int vy, int vw2, int vh2, int r, int *nx, int *ny){
    float cx = (float)(vx + vw2/2), cy = (float)(vy + vh2/2 + 14);
    float a = -PI/2 + (float)r * (PI/4);
    *nx = (int)(cx + fcos(a) * (float)vw2 * 0.36f);
    *ny = (int)(cy + fsin(a) * (float)vh2 * 0.295f);
}
// v1.18: the ONE door into a battle from the map. A region with a
// battle parked in it (SAVE & QUIT) RESUMES that battle — before
// this helper existed, both confirm paths called war_start_battle
// unconditionally, so the natural player move (walk to the map, tap
// the region you were fighting) silently started a fresh fight and
// the autosave then overwrote the parked one in the save file.
static void war_map_launch(int pi, int r){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    if (war_snap_on[pi] && war_snap_reg[pi]==r){
        war_snap_restore(pi);   // sets war_state=1 and posts the RESTORED note itself
        war_snap_clear(pi);     // consumed — the live BwSim is authoritative now
        war_link_pull(pi);      // feat 13: a resumed battle pulls a waiting commander in
        return;
    }
    if (r <= b->war_prog){ war_start_battle(pi, r); }
    else { sfx(16); war_note(S, "THAT REGION IS STILL WILD. CONQUER THE ONES BEFORE IT!"); }
}
// v1.19: the SKIRMISH button on the world map's bottom strip —
// one rect shared by the tap handler and the renderer.
static void war_skirmish_btn(int vx, int vy, int vw2, int vh2, int *bx, int *by){
    (void)vy;
    *bx = vx+vw2-140; *by = vy+vh2-68;
}
// v1.25 fix: open the skirmish setup panel — one body for every
// door in ('S', the button tap, and the map selection's 9th stop).
static void war_open_setup(BwSim *S, u32 keys){
    S->war_state = 4; S->war_setup_sel = 0; S->war_setup_prevk = (int)keys; sfx(6);
}
static void war_map_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi];
    (void)dt;
    if (in->exit_){ bw_exit(pi, 0); return; }
    if (S->war_grace > 0) return;
    if (pi==G.my_id){   // v1.19: S opens the skirmish setup panel
        for (int t=0;t<IN.typed_n;t++) if (IN.typed[t]=='s' || IN.typed[t]=='S'){
            war_open_setup(S, in->keys);
            return;
        }
    }
    // v1.25 fix: the selection walks NINE stops — the eight regions
    // plus the SKIRMISH button as the last stop. Before this, the
    // pad had no door into skirmish at all: the panel opened only
    // on a typed 'S' or a tap, the pad cursor never runs on the
    // map, and A/B here are the region launch — so a pad player
    // could invade every region and never once skirmish. Now the
    // stick/dpad/arrows land on the button like any other stop
    // and A confirms it, the same verb as everywhere else.
    if ((in->keys&K_LEFT) || in->up) { S->war_map_sel = (S->war_map_sel+8)%9; sfx(6); }
    if ((in->keys&K_RIGHT) || in->down) { S->war_map_sel = (S->war_map_sel+1)%9; sfx(6); }
    if (in->sel || in->interact){
        if (S->war_map_sel==8){ war_open_setup(S, in->keys); return; }
        war_map_launch(pi, S->war_map_sel);
        return;
    }
    if (pi==G.my_id && IN.tapped){
        IN.tapped = 0;
        int vx, vy, vw2, vh2; p1_view_rect(&vx, &vy, &vw2, &vh2);
        { int bx, by; war_skirmish_btn(vx, vy, vw2, vh2, &bx, &by);
          if (IN.tap_x>=bx && IN.tap_x<bx+128 && IN.tap_y>=by && IN.tap_y<by+28){
              war_open_setup(S, in->keys);
              return;
          } }
        for (int r=0;r<8;r++){
            int nx, ny; war_node_pos(vx, vy, vw2, vh2, r, &nx, &ny);
            int dx=IN.tap_x-nx, dy=IN.tap_y-ny;
            if (dx*dx+dy*dy < 34*34){
                S->war_map_sel = r;
                war_map_launch(pi, r);
                return;
            }
        }
    }
}
// v1.19: the SKIRMISH SETUP panel (war_state 4) — the map shows
// through, dimmed, and four rows do everything: MAP SIZE, ENEMY,
// LAUNCH, BACK. Same idiom as the pause panel: one rect set
// shared by the tap handler and the renderer, keyboard edges for
// the arrows, the pad's bumpers walk a value.
static void war_render_map(int pi, int vx, int vy, int vw2, int vh2);   // defined below; the setup panel draws it as its backdrop
static void war_setup_rects(int vx, int vy, int vw2, int vh2, int *px, int *py, int *pw, int *ph){
    *pw = vw2<420? vw2-24 : 396; *ph = 224;
    *px = vx+(vw2-*pw)/2; *py = vy+(vh2-*ph)/2 - 10;
}
static void war_setup_pick(int pi, int n){
    BwSim *S = &BWS[pi];
    if (n==0){ S->war_size = S->war_size%3 + 1; sfx(6); }
    else if (n==1){ S->war_diff = S->war_diff%3 + 1; sfx(6); }
    else if (n==2){ war_start_skirmish(pi); }
    else { S->war_state = 0; S->war_grace = 0.3f; sfx(6); }
}
static void war_setup_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi];
    (void)dt;
    if (in->exit_){ S->war_state = 0; S->war_grace = 0.3f; sfx(6); return; }
    if (S->war_grace > 0) return;
    if (in->up) { S->war_setup_sel = (S->war_setup_sel+3)%4; sfx(6); }
    if (in->down) { S->war_setup_sel = (S->war_setup_sel+1)%4; sfx(6); }
    u32 kd = in->keys & ~(u32)S->war_setup_prevk;   // arrow EDGES: BwIn's keys are levels, so a held arrow must not spin the value
    if (kd & K_LEFT){ if (S->war_setup_sel==0) S->war_size = (S->war_size+1)%3 + 1; else if (S->war_setup_sel==1) S->war_diff = (S->war_diff+1)%3 + 1; sfx(6); }
    if (kd & K_RIGHT){ if (S->war_setup_sel==0) S->war_size = S->war_size%3 + 1; else if (S->war_setup_sel==1) S->war_diff = S->war_diff%3 + 1; sfx(6); }
    S->war_setup_prevk = (int)in->keys;
    // v1.25 fix: the pad's B backs out of this panel, exactly as
    // its own hint line promises — but the same ACT_INTERACT edge
    // also arrives as in->interact, and the row pick below spent
    // it first, so B CONFIRMED the row (on LAUNCH, the opposite
    // of backing out). Read the pad edge here, before the pick;
    // the keyboard's interact key still picks, Esc still backs
    // out — only the pad's B changes, to what the hint says.
    if (pi==G.my_id && (IN.pad_edges & (1u<<ACT_INTERACT))){ S->war_state = 0; S->war_grace = 0.3f; sfx(6); return; }
    if (in->sel || in->interact){ war_setup_pick(pi, S->war_setup_sel); return; }
    if (pi==G.my_id){
        if (IN.tapped){
            IN.tapped = 0;
            int vx, vy, vw2, vh2; p1_view_rect(&vx, &vy, &vw2, &vh2);
            int px, py, pw, ph; war_setup_rects(vx, vy, vw2, vh2, &px, &py, &pw, &ph);
            for (int n=0;n<4;n++){
                int ry = py+58+n*38;
                if (IN.tap_x>=px+16 && IN.tap_x<px+pw-16 && IN.tap_y>=ry && IN.tap_y<ry+32){
                    S->war_setup_sel = n;
                    if (n<2){   // a value row: the left third steps back, the rest steps forward
                        int back = IN.tap_x < px+pw/3;
                        if (n==0) S->war_size = back? (S->war_size+1)%3 + 1 : S->war_size%3 + 1;
                        else S->war_diff = back? (S->war_diff+1)%3 + 1 : S->war_diff%3 + 1;
                        sfx(6);
                    } else war_setup_pick(pi, n);
                    return;
                }
            }
        }
        u32 pe = IN.pad_edges;
        if (pe & ((1u<<ACT_WPREV)|(1u<<ACT_WNEXT))){
            int d = (pe & (1u<<ACT_WNEXT))? 1 : -1;
            if (S->war_setup_sel==0) S->war_size = ((S->war_size-1 + d + 3)%3) + 1;
            else if (S->war_setup_sel==1) S->war_diff = ((S->war_diff-1 + d + 3)%3) + 1;
            sfx(6);
        }
        if (pe & (1u<<ACT_FIRE)){ war_setup_pick(pi, S->war_setup_sel); return; }
        // (ACT_INTERACT is handled above, before the row pick —
        // here it would be dead code: in->interact fires first.)
    }
}
static void war_render_setup(int pi, int vx, int vy, int vw2, int vh2){
    BwSim *S = &BWS[pi];
    war_render_map(pi, vx, vy, vw2, vh2);
    blend_rect(vx, vy, vw2, vh2, COL(8,8,14), 150);
    int px, py, pw, ph; war_setup_rects(vx, vy, vw2, vh2, &px, &py, &pw, &ph);
    rect_fill(px, py, pw, ph, COL(16,18,28)); rect_line(px, py, pw, ph, COL(255,170,90));
    draw_text_center(px+pw/2, py+14, "SKIRMISH SETUP", 2, COL(255,210,150));
    static const char *SZ[4] = { "", "SMALL", "STANDARD", "LARGE" };
    static const char *DF[4] = { "", "EASY", "NORMAL", "HARD" };
    for (int n=0;n<4;n++){
        int ry = py+58+n*38;
        int sel = (S->war_setup_sel==n);
        rect_fill(px+16, ry, pw-32, 32, sel? COL(56,64,88) : COL(38,44,60));
        rect_line(px+16, ry, pw-32, 32, sel? COL(255,220,120) : COL(198,164,84));
        char buf[64]; char *q=buf;
        if (n==0){ q=sappend(q, "MAP SIZE  < "); q=sappend(q, SZ[S->war_size]); q=sappend(q, " >"); }
        else if (n==1){ q=sappend(q, "ENEMY  < "); q=sappend(q, DF[S->war_diff]); q=sappend(q, " >"); }
        else if (n==2){ q=sappend(q, "LAUNCH"); }
        else { q=sappend(q, "BACK"); }
        *q=0;
        draw_text_center(px+pw/2, ry+10, buf, 2, COL(240,244,250));
    }
    draw_text_center(px+pw/2, py+ph-16, hint_dev()==DEV_PAD? "STICK + A - B BACKS OUT" : (hint_dev()==DEV_TOUCH? "TAP A ROW" : "ARROWS + SPACE - ESC BACKS OUT"), 1, COL(150,160,185));
}
// --- WAR! on a gamepad (v1.16, Daniel) --------------------------------
// The pad never had a tap channel, so every WAR! verb — select,
// order, build, the command bar — was touch/mouse only; a pad could
// pan and nothing more. The fix is a cursor that IS a finger: the
// left stick flies it, A presses it through war_handle_tap itself
// (so the double/triple-tap muster ladder falls out of the existing
// streak code), B peels one layer back, LB/RB walk the command bar.
// Last device wins, as everywhere in this game: a real tap retires
// the cursor, and any pad input summons it again. Since v1.25 feat
// 13 this path serves BOTH local commanders: it is parameterized by
// the acting player's pad sources (war_pad_src), and P2's old
// centre-tap is gone — the second pad has a real cursor now.
static float war_axis(float a){   // deadzone 0.18, rescaled past it so a gentle push is a gentle fly
    float m = a<0? -a : a;
    if (m<=0.18f) return 0;
    float r = (m-0.18f)/0.82f; if (r>1.0f) r=1.0f;
    return a<0? -r : r;
}
static void war_pad_update(int pi, const float *ax, u32 pe, u32 held, float dt){
    BwSim *S = war_bs(pi);   // feat 13: the acting commander's battle (under the guest swap, wearing their view)
    int vx, vy, vw2, vh2; pi_view_rect(pi, &vx, &vy, &vw2, &vh2);
    float lx = war_axis(ax[0]), ly = war_axis(ax[1]);
    float rx = war_axis(ax[2]), ry = war_axis(ax[3]);
    int stick = (ax[0]>0.18f || ax[0]<-0.18f || ax[1]>0.18f || ax[1]<-0.18f
              || ax[2]>0.18f || ax[2]<-0.18f || ax[3]>0.18f || ax[3]<-0.18f);
    u32 btns = (1u<<ACT_FIRE)|(1u<<ACT_INTERACT)|(1u<<ACT_WPREV)|(1u<<ACT_WNEXT);
    // v1.25 fix (Daniel, Nov 19): the D-pad is a pad steering method
    // too — the game's own hints name it ("STICK/DPAD") — but it only
    // ever fed IN.keys (the camera pan), never the cursor: a D-pad
    // player could wake the cursor and then not aim it anywhere.
    // D-pad edges and holds now wake and fly it like the stick does.
    u32 dirm = (1u<<ACT_UP)|(1u<<ACT_DOWN)|(1u<<ACT_LEFT)|(1u<<ACT_RIGHT);
    int dirwake = (pe & dirm) || (held & dirm);
    if (!S->war_curon){
        if (!war_pad_live(pi) || !(stick || dirwake || (pe & btns))) return;
        S->war_curon = 1;
        // spawn on the first selected unit, else at the camera's centre
        int si=-1;
        for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]>0 && S->war_usel[i]){ si=i; break; }
        if (si>=0){ S->war_curx = S->war_ux[si]; S->war_cury = S->war_uy[si]; }
        else { S->war_curx = S->war_camx; S->war_cury = S->war_camy; }   // v1.22 fix: wake AT the camera centre — the point dbg_warcam and P2's centre-tap already treat as centre. The old +WAR_BAR_H/2 spawn sat 23u below it, a hair outside a small building's tap disc (Zapper 22u), so a wake-press A missed until the stick was nudged
        if (!stick && !dirwake) return;   // a button wake spends the press on waking; it does not also tap
        pe = 0;               // a stick/D-pad wake still flies this frame, but its buttons wait
    }
    // the bar highlight belongs to one structure's selection; any
    // change of selection — from any device — retires it
    if (S->war_barhl_s != S->war_selstruct){ S->war_barhl = -1; S->war_barhl_s = S->war_selstruct; }
    // v1.20: command-bar CAPTURE (Daniel's design — the cursor is
    // NOT flown down to the bar; the bar takes the pad over). A
    // building selected through the pad's own A-tap (the entry is at
    // the A press below) captures: the highlight lands on the first
    // option of the current row, the field cursor freezes and hides
    // (war_render), the camera holds still, and the stick walks the
    // menu — moving it no longer retires the highlight. Capture is
    // validated every frame: it lives only while its building does.
    if (S->war_barcap && (S->war_selstruct<0 || !war_struct_alive(S, S->war_selstruct))){
        S->war_barcap = 0; S->war_barhl = -1; S->war_barhl_row = 0;
    }
    if (S->war_barcap){
        int acts[5], costs[5]; const char *names[5];
        int ca[5], cc[5]; const char *cn[5];
        // stick edges: a fresh deflection steps once — holding the
        // stick does not spin the highlight
        int navx = (ax[0]>0.18f)? 1 : (ax[0]<-0.18f)? -1 : 0;
        int navy = (ax[1]>0.18f)? 1 : (ax[1]<-0.18f)? -1 : 0;
        int ex = (navx!=0 && navx!=S->war_capnx)? navx : 0;
        int ey = (navy!=0 && navy!=S->war_capny)? navy : 0;
        S->war_capnx = navx; S->war_capny = navy;
        if ((pe & (1u<<ACT_WNEXT)) && !ex) ex = 1;   // the bumpers walk the same row
        if ((pe & (1u<<ACT_WPREV)) && !ex) ex = -1;
        // v1.25 fix: D-pad edges walk the captured menu too (a D-pad
        // player could not navigate it at all). The axis guards keep
        // the shell's stick-to-digital fold from double-stepping: a
        // stick push past 0.45 raises an ACT edge the same frame as
        // its analog edge, so D-pad steps count only while the stick
        // itself is centred on that axis.
        if ((pe & (1u<<ACT_RIGHT)) && !ex && ax[0]<=0.18f) ex = 1;
        if ((pe & (1u<<ACT_LEFT)) && !ex && ax[0]>=-0.18f) ex = -1;
        if ((pe & (1u<<ACT_DOWN)) && !ey && ax[1]<=0.18f) ey = 1;
        if ((pe & (1u<<ACT_UP)) && !ey && ax[1]>=-0.18f) ey = -1;
        if (ex){
            int n = S->war_barhl_row==1? war_bar_cats(S, ca, cc, cn) : war_bar_list(S, acts, costs, names);
            if (n>0){ S->war_barhl = S->war_barhl<0? (ex>0? 0 : n-1) : (S->war_barhl + ex + n) % n; sfx(6); }
        }
        if (ey && S->war_bcat>=0){   // two rows are up: up/down hops between the options row and the category row
            if (ey>0 && S->war_barhl_row==0){
                int cn2 = war_bar_cats(S, ca, cc, cn);
                S->war_barhl_row = 1; S->war_barhl = 0;
                for (int i=0;i<cn2;i++) if (ca[i]==20+S->war_bcat){ S->war_barhl = i; break; }   // land on the open category
                sfx(6);
            } else if (ey<0 && S->war_barhl_row==1){
                S->war_barhl_row = 0; S->war_barhl = 0; sfx(6);
            }
        }
        // A works the menu: on the category row it switches the
        // category and hands back its options row; on the options
        // row it activates through war_bar_activate — a TRAIN option
        // queues and KEEPS capture (queue several), while arming a
        // placement hands the field cursor back for the placement
        if ((pe & (1u<<ACT_FIRE)) && S->war_grace<=0 && S->war_barhl>=0){
            if (S->war_barhl_row==1){
                int cn2 = war_bar_cats(S, ca, cc, cn);
                if (S->war_barhl<cn2){ S->war_bcat = ca[S->war_barhl]-20; S->war_barhl_row = 0; S->war_barhl = 0; sfx(6); }
            } else {
                int n = war_bar_list(S, acts, costs, names);
                if (S->war_barhl<n){
                    int act = acts[S->war_barhl];
                    war_bar_activate(pi, S->war_barhl);
                    if (S->war_place>=0){ S->war_barcap = 0; S->war_barhl = -1; S->war_barhl_row = 0; }
                    else if (act>=20 && act<=22) S->war_barhl = 0;   // a category opened from the top level: its options row starts at the first option
                }
            }
        }
        // B peels, captured-style: an open category on a
        // multi-category building closes back to the top-level row,
        // still captured; anything else exits capture AND clears the
        // selection — Daniel's "cancel to get out of the selection"
        if (pe & (1u<<ACT_INTERACT)){
            S->war_demoarm = -1;   // v1.21: B disarms a pending DEMOLISH
            int cn2 = war_bar_cats(S, ca, cc, cn);
            if (S->war_bcat>=0 && cn2>1){
                int old = S->war_bcat;
                S->war_bcat = -1; S->war_barhl_row = 0; S->war_barhl = 0;
                for (int i=0;i<cn2;i++) if (ca[i]==20+old){ S->war_barhl = i; break; }
                sfx(6);
            } else {
                S->war_barcap = 0; S->war_barhl = -1; S->war_barhl_row = 0;
                for (int i=0;i<WAR_MAXU;i++) S->war_usel[i]=0;
                S->war_selstruct = -1; S->war_bcat = -1; sfx(6);
            }
        }
        return;   // captured: no cursor flight, no pan, no edge-follow this frame
    }
    // the sticks fly the cursor (and retire the bar highlight — A
    // goes back to pressing the field). The D-pad flies it too, at
    // full deflection, on any axis the analog stick leaves centred
    // (v1.25 fix: the D-pad only ever panned the camera before, so
    // it could not aim the cursor at all). The RIGHT stick flies it
    // as well — its old job, panning the camera while the cursor
    // kept its screen spot, is gone under the centred model below:
    // both sticks move the cursor, the camera follows the cursor.
    float fx = lx, fy = ly;
    if (fx==0) fx = ((held>>ACT_RIGHT)&1)? 1.0f : ((held>>ACT_LEFT)&1)? -1.0f : 0;
    if (fy==0) fy = ((held>>ACT_DOWN)&1)? 1.0f : ((held>>ACT_UP)&1)? -1.0f : 0;
    fx += rx; fy += ry;
    { float fm = fsqrt(fx*fx + fy*fy); if (fm>1.0f){ fx/=fm; fy/=fm; } }
    if (fx || fy){
        S->war_curx += fx * 540.0f * dt;
        S->war_cury += fy * 540.0f * dt;
        S->war_barhl = -1;
    }
    // v1.25 (Daniel, Nov 19): THE CURSOR IS THE CAMERA. The old model
    // flew the cursor inside a clamped camera — and war_update's key
    // pan, which the pad's own stick feeds through IN.keys, shoved
    // the camera at 430 u/s while the cursor flew at 540, so the
    // difference piled up as a screen offset; the edge-follow then
    // pinned the cursor 28u inside the edge, the camera clamp at the
    // map's rim stranded it there, and once it was offset it stayed
    // offset in every direction. Now the pad cursor sits at the
    // centre of the field view, always: the cursor clamps to the
    // arena, the camera centres on it exactly, and at the rim the
    // view simply scrolls off the map — war_render already draws the
    // ground beyond the fence darker, so the off-map space reads as
    // compensated void, not as a broken clamp. war_clamp_cam still
    // governs every other camera driver (keys, drags, restores);
    // while the pad cursor is on, this re-centring runs after them
    // every frame, so no other pan can strand the cursor either.
    if (S->war_curx < WAR_OX) S->war_curx = WAR_OX;
    if (S->war_curx > WAR_OX+S->war_aw) S->war_curx = WAR_OX+S->war_aw;
    if (S->war_cury < WAR_OY) S->war_cury = WAR_OY;
    if (S->war_cury > WAR_OY+S->war_ah) S->war_cury = WAR_OY+S->war_ah;
    S->war_camx = S->war_curx; S->war_camy = S->war_cury;
    // LB / RB walk the selected structure's command bar
    if (S->war_selstruct>=0 && (pe & ((1u<<ACT_WPREV)|(1u<<ACT_WNEXT)))){
        int acts[5], costs[5]; const char *names[5];
        int n = war_bar_list(S, acts, costs, names);
        if (n>0){
            int dir = (pe & (1u<<ACT_WNEXT))? 1 : -1;
            S->war_barhl = S->war_barhl<0? (dir>0? 0 : n-1) : (S->war_barhl + dir + n) % n;
            sfx(6);
        }
    }
    // B peels one layer: a pending placement, then an open
    // category (v1.17), then the bar highlight, then the selection
    // itself. A single-category building's menu reopens itself, so
    // there B skips the category step and deselects as it always did.
    if (pe & (1u<<ACT_INTERACT)){
        S->war_demoarm = -1;   // v1.21: B disarms a pending DEMOLISH
        int ca[5], cc[5]; const char *cn[5];
        if (S->war_place>=0){ S->war_place = -1; S->war_bcat = -1; sfx(6); }
        else if (S->war_bcat>=0 && war_bar_cats(S, ca, cc, cn)>1){ S->war_bcat = -1; S->war_barhl = -1; sfx(6); }
        else if (S->war_barhl>=0){ S->war_barhl = -1; }
        else { for (int i=0;i<WAR_MAXU;i++) S->war_usel[i]=0; S->war_selstruct = -1; S->war_bcat = -1; sfx(6); }
    }
    // A: the finger comes down — on the lit bar button if one is
    // lit, else on the field at the cursor, through the very same
    // door a real tap uses (same grace gate, same streak ladder)
    if ((pe & (1u<<ACT_FIRE)) && S->war_grace<=0){
        if (S->war_barhl>=0){ int hl = S->war_barhl; S->war_barhl = -1; war_bar_activate(pi, hl); }
        else {
            int csx = vx + vw2/2 + (int)(S->war_curx - S->war_camx);
            int csy = vy + vh2/2 + (int)(S->war_cury - S->war_camy - WAR_BAR_H*0.5f);
            int ssb = S->war_selstruct;
            war_handle_tap(pi, csx, csy);
            // v1.20: a building that became selected through THIS
            // pad tap captures the command bar (the capture block
            // above) — if it has a menu at all. Selections of units,
            // deselects and rally taps never capture.
            if (S->war_selstruct>=0 && S->war_selstruct!=ssb){
                int ta[5], tc[5]; const char *tn[5];
                if (war_bar_list(S, ta, tc, tn)>0){
                    S->war_barcap = 1; S->war_barhl = 0; S->war_barhl_row = 0;
                    S->war_barhl_s = S->war_selstruct;
                    // v1.25 fix (Daniel, Nov 19): seed the captured
                    // stick-edge state from the stick's CURRENT sign.
                    // It used to reset to 0, so a player who pressed A
                    // while still pushing the stick toward the building
                    // — the natural way to select mid-flight — had the
                    // still-held stick read as a FRESH deflection on
                    // the next frame, and the highlight stepped once
                    // for free: capture opened on the wrong button.
                    S->war_capnx = (ax[0]>0.18f)? 1 : (ax[0]<-0.18f)? -1 : 0;
                    S->war_capny = (ax[1]>0.18f)? 1 : (ax[1]<-0.18f)? -1 : 0;
                }
            }
        }
    }
}
// the pause menu's input, for whoever opened it (feat 13: the
// pauser drives the shared pause — owner or guest — with their own
// devices; the other commander's inputs wait it out)
static void war_pause_input(int pi, float dt){
    if (pi==G.my_id){
        if (IN.tapped){ IN.tapped=0; war_pause_tap(pi, IN.tap_x, IN.tap_y); }
        if (IN.weapon_key>=1 && IN.weapon_key<=3){ int k=IN.weapon_key; IN.weapon_key=0; war_pause_pick(pi, k-1); }
    }
    { const float *ax; u32 pe, held; war_pad_src(pi, &ax, &pe, &held); war_pad_pause(pi, ax, pe, dt); }
}
// the battle's command-input section, extracted verbatim from
// war_update (feat 13): the owner runs it in place, exactly where
// it always ran; a guest co-commander runs it under the view swap.
static void war_command_input(int pi, float dt, const BwIn *in){
    BwSim *S = war_bs(pi);
    int dragging = (pi==G.my_id)? war_drag_pan(pi) : 0;
    if (!dragging){   // a field drag owns the camera; keys/stick pan only when no drag does
        int kx=0, ky=0;
        if (in->keys&K_LEFT) kx-=1; if (in->keys&K_RIGHT) kx+=1;
        if (in->keys&K_UP) ky-=1; if (in->keys&K_DOWN) ky+=1;
        if (kx || ky){ S->war_camx += (float)kx*430.0f*dt; S->war_camy += (float)ky*430.0f*dt; war_clamp_cam(S); }
    }
    if (pi==G.my_id && IN.tapped){
        S->war_curon=0;   // a real tap: the finger/mouse takes the field back (last device wins)
        if (S->war_barcap){ S->war_barcap=0; S->war_barhl=-1; S->war_barhl_row=0; }   // v1.20: ...and it ends a bar capture the same way
        if (S->war_grace<=0){ IN.tapped=0; war_handle_tap(pi, IN.tap_x, IN.tap_y); }
    }
    // feat 13: the old P2 centre-tap (in->interact taps at the
    // camera centre) is GONE — pad 2 has a real cursor now, and B
    // on that pad is the pad path's peel verb; a centre-tap on the
    // same edge would fight it. P2's WAR! verbs are P1's verbs.
    if (pi==G.my_id && IN.weapon_key>=1 && IN.weapon_key<=5){ war_bar_activate(pi, IN.weapon_key-1); IN.weapon_key=0; }
    { const float *ax; u32 pe, held; war_pad_src(pi, &ax, &pe, &held); war_pad_update(pi, ax, pe, held, dt); }
}
// a guest co-commander's frame (feat 13): the battle NEVER advances
// here — it validates the link, mirrors the state for rendering,
// and runs only the command layer, under the view swap.
static void war_guest_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    int owner = S->war_guest-1;
    BwSim *HS = &BWS[owner];
    int valid = owner==war_other_local(pi) && HS->war_duo==pi
        && G.bud[owner].active && G.bud[owner].in_bw && G.bud[owner].bw_mode==8
        && (HS->war_state==1 || HS->war_state==2 || HS->war_state==3);
    if (!valid){
        // the command is over (battle decided, commander saved &
        // quit, or the link broke from the other side): back to
        // your own map, with a fresh field's view defaults
        S->war_guest = 0;
        if (HS->war_duo==pi) HS->war_duo = -1;
        war_view_fresh(S, S);
        S->war_state = 0; S->war_map_sel = b->war_prog<8? b->war_prog : 7; S->war_grace = 0.3f;
        return;
    }
    S->war_state = HS->war_state;   // mirror, for the renderer and the state readers
    if (HS->war_state==2 || HS->war_state==3) return;   // the end screen runs on the owner's clock; the guest watches it
    if (S->war_ordt > 0) S->war_ordt -= dt;
    if (HS->war_pause){
        if (HS->war_pause_by==pi){
            if (in->exit_){ HS->war_pause=0; HS->war_grace=0.3f; sfx(6); return; }
            war_view_exchange(HS, S);
            war_pause_input(pi, dt);
            war_view_exchange(HS, S);
        }
        return;
    }
    if (in->exit_){ HS->war_pause=1; HS->war_pause_by=pi; sfx(6); return; }
    war_view_exchange(HS, S);
    war_command_input(pi, dt, in);
    war_view_exchange(HS, S);
}
static void war_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    if (S->war_msgt > 0) S->war_msgt -= dt;
    if (S->war_grace > 0) S->war_grace -= dt;
    if (S->war_guest){ war_guest_update(pi, dt, in); return; }   // feat 13: a guest commands the owner's battle, never their own
    if (S->war_duo>=0){   // feat 13: link upkeep — a guest who left WAR! (any door) dissolves the link within a frame
        int g = S->war_duo;
        BwSim *GS = (g>=0 && g<MAXP)? &BWS[g] : 0;
        if (!GS || g!=war_other_local(pi) || !G.bud[g].active || !G.bud[g].in_bw || G.bud[g].bw_mode!=8 || GS->war_guest!=pi+1){
            if (S->war_pause && S->war_pause_by==g) S->war_pause_by = pi;   // a departed pauser hands the menu to the owner
            war_link_break(pi);
        }
    }
    if (S->war_state==0){ war_map_update(pi, dt, in); return; }
    if (S->war_state==4){ war_setup_update(pi, dt, in); return; }   // v1.19: the skirmish setup panel — the battle clock does not run here
    if (S->war_state==2 || S->war_state==3){
        S->war_endt -= dt;
        if (pi==G.my_id && IN.tapped){ IN.tapped=0; if (S->war_endt > 0.4f) S->war_endt = 0.4f; }
        if (S->war_endt <= 0){
            war_clear_field(S);
            S->war_state=0; S->war_map_sel = b->war_prog<8? b->war_prog : 7; S->war_grace=0.3f;
            if (S->war_duo>=0){ int g=S->war_duo; war_link_break(pi); BWS[g].war_state=0; BWS[g].war_grace=0.3f; }   // feat 13: a decided battle releases the guest to their own map
        }
        return;
    }
    if (S->war_pause){   // v1.17: the battle holds its breath — only the pause menu's inputs run; war_t, waves, units and structures all stay frozen
        int linked = S->war_duo>=0;   // feat 13: shared pause — only the pauser resumes it or drives it; the other commander waits
        if (in->exit_ && (!linked || S->war_pause_by==pi)){ S->war_pause=0; S->war_grace=0.3f; sfx(6); return; }   // Esc again = RESUME
        if (!linked || S->war_pause_by==pi) war_pause_input(pi, dt);
        else if (pi==G.my_id){ IN.tapped=0; IN.weapon_key=0; }   // feat 13: the waiting commander's taps/keys are eaten, not queued into the resume frame
        return;
    }
    S->war_t += dt;
    if (S->war_demoarm>=0 && S->war_demoarm != S->war_selstruct) S->war_demoarm=-1;   // v1.21: an armed DEMOLISH lives only while its building stays selected
    for (int i=0;i<WAR_MAXU;i++) if (S->war_uflash[i]>0) S->war_uflash[i] -= dt;   // cosmetic timers (hit flashes + particles): they only ever feed the renderer
    for (int s=0;s<WAR_MAXS;s++) if (S->war_sflash[s]>0) S->war_sflash[s] -= dt;
    for (int i=0;i<24;i++) if (S->war_ft[i]>0) S->war_ft[i] -= dt;
    for (int t=0;t<S->war_tn;t++) if (S->war_tdeadt[t] > 0) S->war_tdeadt[t] -= dt;   // scorch marks fade
    if (in->exit_){   // Esc in a battle (v1.17): the pause menu — RESUME / RESTART / SAVE & QUIT — not an abandonment
        S->war_pause=1; S->war_pause_by=pi; sfx(6);   // feat 13: the pauser is recorded — the pause is theirs to drive
        return;
    }
    war_command_input(pi, dt, in);   // feat 13: the drag / key-pan / tap / weapon-key / pad section, extracted verbatim — see war_command_input above (the old P2 centre-tap line is gone there, deliberately)
    S->war_wavet -= dt;
    if (S->war_wavet <= 0){
        S->war_wavet = war_wave_dt(S->war_region);
        // v1.21: a wave is BOUGHT, not conjured — each hive spends
        // its own purse, in this order: (1) replace dead bug
        // harvesters first, because a hive that cannot harvest
        // cannot fight for long; (2) buy the wave, bug by bug,
        // stopping the moment the purse can't cover the next one —
        // a partial wave is a real wave, and a hive that buys
        // nothing sends nothing and says nothing; (3) refill the
        // guard post only from real surplus (>= 60 left over).
        int sent = 0;
        for (int s=0;s<WAR_MAXS;s++)
            if (S->war_shp[s]>0 && S->war_stype[s]==WAR_S_HIVE){
                while (war_hive_bharv(S, s) < 2 && S->war_hbits[s] >= WAR_COST_BHARV){
                    if (war_spawn_bharv(S, s) < 0) break;
                    S->war_hbits[s] -= WAR_COST_BHARV;
                }
                int k = 2 + (S->war_region>=4? 1:0);
                for (int j=0;j<k;j++){
                    int type = war_bug_type_next(S);
                    int price = (type==WAR_U_SPIT)? WAR_COST_SPIT : WAR_COST_CRAWL;
                    if (S->war_hbits[s] < price) break;
                    if (war_spawn_bug_t(S, s, 1, type) < 0) break;   // capped/full: nothing bought, nothing charged
                    S->war_hbits[s] -= price;
                    sent = 1;
                }
                int gar = S->war_region>=2? 3 : 2;
                while (war_hive_guards(S, s) < gar && S->war_hbits[s] >= 60){
                    if (war_spawn_bug_t(S, s, 0, WAR_U_CRAWL) < 0) break;
                    S->war_hbits[s] -= WAR_COST_CRAWL;
                }
            }
        if (sent) war_note(S, "THE HIVE SENDS A WAVE!");
    }
    for (int s=0;s<WAR_MAXS;s++) if (S->war_shp[s]>0) war_struct_update(pi, s, dt);
    for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]>0) war_unit_update(pi, i, dt);
    war_separate(S);
    war_proj_update(pi, dt);
    war_fog_update(S);
    if (S->war_ordt > 0) S->war_ordt -= dt;
}
static void war_hp_bar(int x, int y, int w, float f, int enemy){
    if (f<0) f=0; if (f>1) f=1;
    rect_fill(x, y, w, 3, COL(24,20,28));
    rect_fill(x, y, (int)(w*f), 3, enemy? COL(255,110,90) : COL(120,230,90));
}
static void war_render_map(int pi, int vx, int vy, int vw2, int vh2){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    rect_fill(vx, vy, vw2, vh2, COL(12,10,24));
    for (int i=0;i<40;i++){
        u32 h = hash2(i*7+1, i*13+5, 0x5EEDu);
        int sx = vx + (int)(h % (u32)(vw2>0? vw2 : 1)), sy = vy + (int)((h>>9) % (u32)(vh2>0? vh2 : 1));
        pset(sx, sy, COL(70,66,100));
    }
    draw_text_center(vx+vw2/2, vy+14, "WAR!", 3, COL(255,120,90));
    draw_text_center(vx+vw2/2, vy+42, "CONQUER THE EIGHT REGIONS", 1, COL(200,210,230));
    int cx = vx+vw2/2, cy = vy+vh2/2+8;
    for (int r=0;r<8;r++){
        int nx, ny; war_node_pos(vx, vy, vw2, vh2, r, &nx, &ny);
        line_thick(cx, cy, nx, ny, 1, r < b->war_prog? REGION_GA[r] : COL(52,48,78));
    }
    disc_fill(cx, cy, 10, COL(255,90,70)); disc_fill(cx, cy, 5, COL(255,200,120));
    draw_text_center(cx, cy+13, "EPICENTER", 1, COL(255,170,150));
    for (int r=0;r<8;r++){
        int nx, ny; war_node_pos(vx, vy, vw2, vh2, r, &nx, &ny);
        int done = r < b->war_prog, frontier = r == b->war_prog;
        if (frontier){
            int pr = 27 + (int)(fsin(G.anim_t*4.0f)*2.5f);
            circle_out(nx, ny, pr, COL(255,240,200));
        }
        if (war_snap_on[pi] && war_snap_reg[pi]==r){   // v1.18: a battle is parked HERE — tapping this node resumes it, and the player must be able to see that
            int pr2 = 34 + (int)(fsin(G.anim_t*4.0f + 1.5f)*3.0f);
            circle_out(nx, ny, pr2, COL(140,255,170));
            draw_text_center(nx, ny+44, "IN PROGRESS", 1, COL(160,255,190));
        }
        if (r == S->war_map_sel) circle_out(nx, ny, 31, COL(255,220,90));
        disc_fill(nx, ny, 24, done? REGION_GA[r] : frontier? col_shade(REGION_GA[r], 0.75f) : COL(52,50,66));
        circle_out(nx, ny, 24, done || frontier? col_shade(REGION_GA[r], 0.6f) : COL(84,80,104));
        if (done){
            line_thick(nx-8, ny, nx-2, ny+7, 2, COL(20,30,20));
            line_thick(nx-2, ny+7, nx+9, ny-8, 2, COL(20,30,20));
        } else if (!frontier){
            rect_fill(nx-6, ny-2, 12, 9, COL(120,116,140));
            rect_fill(nx-4, ny-8, 8, 6, COL(120,116,140));
            disc_fill(nx, ny+2, 2, COL(40,38,54));
        } else {
            char nb[4]; char *q=nb; q=sapp_int(q, r+1); *q=0;
            draw_text_center(nx, ny-7, nb, 2, COL(16,16,24));
        }
        static const char *SHORT[8] = { "MEADOWS", "SPAGHETTI", "LEGACY", "BADLANDS", "CACHE FIELDS", "RAM FLATS", "CABLE JUNGLE", "THE DEPTHS" };
        draw_text_center_fit(nx, r==4? ny-40 : ny+30, 150, SHORT[r], 1,
                             done? col_shade(REGION_GA[r], 1.25f) : frontier? COL(240,244,250) : COL(110,106,130));
    }
    { char buf[48]; char *q=buf; q=sappend(q, "REGIONS CONQUERED: "); q=sapp_int(q, b->war_prog); q=sappend(q, " / 8"); *q=0;
      draw_text_center(vx+vw2/2, vy+vh2-34, buf, 1, COL(255,220,120)); }
    { int bx, by; war_skirmish_btn(vx, vy, vw2, vh2, &bx, &by);   // v1.19: the skirmish door, bottom-right of the map
      int selb = (S->war_map_sel==8);   // v1.25: the selection's 9th stop lights the button like a picked node
      rect_fill(bx, by, 128, 28, selb? COL(56,64,88) : COL(38,44,60));
      rect_line(bx, by, 128, 28, selb? COL(255,220,120) : COL(255,170,90));
      draw_text_center(bx+64, by+9, "SKIRMISH", 2, COL(255,210,150)); }
    if (b->war_prog>=8){ char b2[96]; char *q=b2; q=sappend(q, "ALL REGIONS CONQUERED! ");
        q=sappend(q, hint_dev()==DEV_PAD? "A ON ANY REGION" : (hint_dev()==DEV_TOUCH? "TAP ANY REGION" : "CLICK ANY REGION"));
        q=sappend(q, " TO FIGHT IT AGAIN."); *q=0;
        draw_text_center(vx+vw2/2, vy+vh2-19, b2, 1, COL(200,230,200)); }
    else draw_text_center(vx+vw2/2, vy+vh2-19, hint_dev()==DEV_PAD? "STICK + A - INVADE - LAST STOP IS SKIRMISH" : (hint_dev()==DEV_TOUCH? "TAP A REGION TO INVADE IT" : "CLICK A REGION - ARROWS + SPACE - S: SKIRMISH"), 1, COL(150,160,185));
    if (S->war_msgt>0) draw_text_center(vx+vw2/2, vy+58, S->war_msg, 1, COL(255,170,150));
}
static void war_draw_bar_btn(BwSim *S, int bx, int by, int act, int cost, const char *name, int hl){
    int afford = S->war_bits >= cost;
    if ((act==0 || act==4 || act==5 || act==8 || act==9 || act==12) && S->war_selstruct>=0 && S->war_qlen[S->war_selstruct]>=9) afford = 0;   // a full queue greys its train buttons out (addendum 2)
    u32 fill = afford? COL(38,44,60) : COL(26,26,34);
    u32 edge = afford? COL(198,164,84) : COL(70,66,84);
    if (act>=20 && act<=22 && act-20==S->war_bcat){ fill = COL(56,64,88); edge = COL(255,220,120); }   // the open category, lit
    if (act==13){   // v1.21 cosmetic: DEMOLISH wears a warning red — and once ARMED, its edge pulses so the state can't be missed
        int armed = S->war_selstruct>=0 && S->war_demoarm==S->war_selstruct;
        edge = armed? (((int)(G.anim_t*5.0f)&1)? COL(255,70,55) : COL(255,170,60)) : COL(205,95,80);
        if (armed) fill = COL(64,34,34);
    }
    rect_fill(bx, by+6, 84, 34, fill);
    rect_line(bx, by+6, 84, 34, edge);
    draw_text_center(bx+42, by+11, name, 1, afford? COL(240,244,250) : COL(130,126,150));
    if (cost>0){
        char cb[16]; char *q=cb; q=sapp_int(q, cost); q=sappend(q, " BITS"); *q=0;
        draw_text_center(bx+42, by+24, cb, 1, afford? COL(255,205,40) : COL(120,110,90));
    } else if (act>=20 && act<=22) draw_text_center(bx+42, by+24, act-20==S->war_bcat? "OPEN" : "PICK", 1, COL(160,170,190));
    else if (act==13 && S->war_selstruct>=0){   // v1.21: DEMOLISH advertises its refund, not "FREE"
        char cb[16]; char *q=cb; q=sappend(q, "+"); q=sapp_int(q, WAR_BUILD_COST[S->war_stype[S->war_selstruct]]/2); q=sappend(q, " BITS"); *q=0;
        draw_text_center(bx+42, by+24, cb, 1, COL(255,205,40));
    }
    else draw_text_center(bx+42, by+24, act==6? "STOP" : "FREE", 1, COL(160,170,190));
    // a train button carries the building's QUEUE (addendum 2): the
    // front item's progress tick plus the depth in the corner
    if ((act==0 || act==4 || act==5 || act==8 || act==9 || act==12) && S->war_selstruct>=0 && S->war_qlen[S->war_selstruct]>0){
        int ut = S->war_queue[S->war_selstruct][0];
        float f = 1.0f - S->war_qt[S->war_selstruct] / WAR_TRAIN_TIME[ut];
        if (f<0) f=0; if (f>1) f=1;
        rect_fill(bx+4, by+36, 76, 3, COL(20,18,30));
        rect_fill(bx+4, by+36, (int)(76*f), 3, COL(120,230,90));
        char qb[8]; char *q=qb; *q++='x'; q=sapp_int(q, S->war_qlen[S->war_selstruct]); *q=0;
        draw_text(bx+84-text_w(qb,1)-2, by+8, qb, 1, COL(160,255,190));
    }
    if (hl){   // the pad's LB/RB highlight (v1.16): a bright double frame — A presses THIS button
        rect_line(bx-2, by+4, 88, 38, COL(255,240,160));
        rect_line(bx, by+6, 84, 34, COL(255,240,160));
    }
}
static void war_render_bar(int pi, int vx, int vy, int vw2, int vh2){
    BwSim *S = &BWS[pi];
    int by = vy + vh2 - WAR_BAR_H;
    rect_fill(vx, by, vw2, WAR_BAR_H, COL(14,13,22));
    rect_fill(vx, by, vw2, 2, COL(52,48,74));
    rect_fill(vx, by+2, vw2, 1, COL(120,100,50));   // v1.21 cosmetic: a brass pinstripe under the panel's top edge (styling only — the bar's geometry is untouched)
    int acts[5], costs[5]; const char *names[5];
    int n = war_bar_list(S, acts, costs, names);
    if (S->war_bcat>=0 && S->war_place<0){
        // two rows (addendum 1): the open category's options in the
        // strip directly above the bar, the categories in the bar
        // itself. The pad highlight walks the OPTIONS (the current
        // row), so it is drawn up there; WAR_BAR_H never changes.
        // v1.20: under bar capture the highlight may walk the
        // category row instead — each row draws it only when it is
        // the walked row (war_barhl_row); outside capture the row
        // is always 0 and this renders exactly as before.
        int sy2 = by - WAR_SUB_H;
        rect_fill(vx, sy2, vw2, WAR_SUB_H, COL(18,17,28));
        rect_fill(vx, sy2, vw2, 2, COL(52,48,74));
        rect_fill(vx, sy2+2, vw2, 1, COL(120,100,50));   // v1.21 cosmetic: the strip wears the same pinstripe
        for (int i=0;i<n;i++) war_draw_bar_btn(S, vx + 6 + i*90, sy2, acts[i], costs[i], names[i], i==S->war_barhl && S->war_barhl_row==0);
        { const char *cl = S->war_bcat==0? "TRAIN" : S->war_bcat==1? "BUILD" : "DEFENSE";
          int ix = vx + 6 + n*90 + 10;
          if (ix + text_w(cl,1) < vx+vw2) draw_text(ix, sy2+17, cl, 1, COL(255,220,120)); }
        int ca[5], cc[5]; const char *cn[5];
        int cn2 = war_bar_cats(S, ca, cc, cn);
        for (int i=0;i<cn2;i++) war_draw_bar_btn(S, vx + 6 + i*90, by, ca[i], cc[i], cn[i], i==S->war_barhl && S->war_barhl_row==1);
    } else {
        for (int i=0;i<n;i++) war_draw_bar_btn(S, vx + 6 + i*90, by, acts[i], costs[i], names[i], i==S->war_barhl && S->war_barhl_row==0);
    }
    if (vw2 >= VW){   // the info zone only earns its keep at full width
        int ix = vx + 6 + n*90 + 10;
        rect_fill(ix-6, by+7, 1, WAR_BAR_H-14, COL(52,48,74));   // v1.21 cosmetic: a divider between the buttons and the readout
        if (S->war_place>=0) draw_text(ix, by+17, hint_dev()==DEV_PAD? "A PLACES IT WHERE THE CURSOR SITS" : (hint_dev()==DEV_TOUCH? "TAP GROUND TO PLACE IT" : "CLICK THE GROUND TO PLACE IT"), 1, COL(255,220,150));
        else if (S->war_selstruct>=0 && war_struct_alive(S, S->war_selstruct)){
            int s = S->war_selstruct;
            static const char *SN[7] = { "COMPILER CORE", "PATCH BAY", "ZAPPER TOWER", "FIREWALL", "BUG HIVE", "ASSEMBLY PLANT", "GATE" };
            char buf[56]; char *q=buf; q=sappend(q, SN[S->war_stype[s]]); q=sappend(q, "  ");
            q=sapp_int(q, (int)S->war_shp[s]); q=sappend(q, "/"); q=sapp_int(q, S->war_smaxhp[s]);
            if (S->war_qlen[s]>0){ q=sappend(q, "  TRAINING x"); q=sapp_int(q, S->war_qlen[s]); }
            *q=0;
            draw_text(ix, by+17, buf, 1, COL(200,214,220));
        }
        else if (war_sel_count(S)>0){
            char buf[64]; char *q=buf; q=sapp_int(q, war_sel_count(S));
            q=sappend(q, hint_dev()==DEV_PAD? " SELECTED - A ON GROUND: MOVE, A ON A BUG: ATTACK" : (hint_dev()==DEV_TOUCH? " SELECTED - TAP GROUND: MOVE, TAP A BUG: ATTACK" : " SELECTED - CLICK GROUND: MOVE, CLICK A BUG: ATTACK")); *q=0;
            draw_text(ix, by+17, buf, 1, COL(200,230,200));
        }
        else draw_text(ix, by+17, hint_dev()==DEV_PAD? "A ON A BUDDY OR A BUILDING" : (hint_dev()==DEV_TOUCH? "TAP A BUDDY OR A BUILDING" : "CLICK A BUDDY OR A BUILDING"), 1, COL(150,160,185));
    }
}
static void war_draw_struct(BwSim *S, int s){
    float x = S->war_sx[s], y = S->war_sy[s];
    int type = S->war_stype[s];
    int sx = SX(x), sy = SY(y);
    int rad = WAR_SRAD[type];
    ellipse_fill(sx, sy + rad*3/4, rad, rad/3, COL(16,14,24));
    if (S->war_selstruct == s)   // a selection disc BEHIND the body: a green rim (circle_out fills, so it must go first)
        disc_fill(sx, sy, rad + 5 + (int)(fsin(G.anim_t*5.0f)*1.5f), COL(140,255,140));
    if (type==WAR_S_HQ){
        rect_fill(sx-30, sy-26, 60, 52, COL(52,58,78)); rect_line(sx-30, sy-26, 60, 52, COL(120,140,180));
        rect_fill(sx-30, sy-26, 60, 8, COL(70,80,110));
        int pr = 10 + (int)(fsin(G.anim_t*2.4f)*1.5f);
        war_blend_disc(sx, sy+2, pr+11, COL(110,235,255), 34);   // v1.21 cosmetic: the Core's soft halo
        disc_fill(sx, sy+2, pr+4, COL(30,90,110));
        disc_fill(sx, sy+2, pr, COL(110,235,255));
        disc_fill(sx-3, sy-1, 3, COL(240,255,255));
        rect_fill(sx-2, sy-40, 3, 14, COL(120,140,180));
        disc_fill(sx, sy-42, 3, COL(255,120,90));
        draw_text_center(sx, sy+32, "COMPILER CORE", 1, COL(200,230,255));
    } else if (type==WAR_S_PATCH){
        rect_fill(sx-24, sy-16, 48, 34, COL(88,98,118)); rect_line(sx-24, sy-16, 48, 34, COL(160,170,200));
        rect_fill(sx-24, sy-16, 48, 7, COL(240,170,60));
        line_thick(sx-8, sy+8, sx+6, sy-6, 3, COL(220,226,235));
        disc_fill(sx+9, sy-9, 4, COL(220,226,235));
        // v1.21 cosmetic: the work light — blinking green while the queue cooks, a dim ember while idle
        disc_fill(sx+19, sy-12, 2, S->war_qlen[s]>0? (((int)(G.anim_t*2.0f)&1)? COL(140,255,160) : COL(60,140,80)) : COL(52,72,58));
        draw_text_center(sx, sy+24, "PATCH BAY", 1, COL(220,228,240));
    } else if (type==WAR_S_ZAP){
        rect_fill(sx-11, sy+2, 22, 10, COL(70,76,96)); rect_line(sx-11, sy+2, 22, 10, COL(140,150,180));
        rect_fill(sx-2, sy-16, 4, 18, COL(120,130,160));
        u32 orb = S->war_scd[s] > 0.4f? COL(255,255,255) : COL(120,220,255);
        war_blend_disc(sx, sy-20, 11 + (int)(fsin(G.anim_t*3.0f + (float)s)*1.5f), COL(120,220,255), S->war_scd[s] > 0.4f? 64 : 30);   // v1.21 cosmetic: the charge glow, hottest just after a shot
        disc_fill(sx, sy-20, 7, orb); disc_fill(sx, sy-20, 3, COL(230,250,255));
        draw_text_center(sx, sy+18, "ZAPPER", 1, COL(200,230,255));
    } else if (type==WAR_S_WALL){
        // segments on the 24u grid join into ONE barrier (Daniel,
        // Nov 16): a centre tile plus an arm toward every friendly
        // firewall segment a grid step away, so runs, columns and
        // corners all read as a single wall.
        int armE=0, armW=0, armN=0, armS=0;
        for (int o=0;o<WAR_MAXS;o++){
            if (o==s || S->war_shp[o]<=0 || S->war_stype[o]!=WAR_S_WALL) continue;
            float dx=S->war_sx[o]-x, dy=S->war_sy[o]-y;
            if (dy>-4.0f && dy<4.0f){ if (dx>20.0f && dx<28.0f) armE=1; else if (dx<-20.0f && dx>-28.0f) armW=1; }
            else if (dx>-4.0f && dx<4.0f){ if (dy>20.0f && dy<28.0f) armS=1; else if (dy<-20.0f && dy>-28.0f) armN=1; }
        }
        if (armW) rect_fill(sx-24, sy-7, 24, 14, COL(196,92,44));
        if (armE) rect_fill(sx, sy-7, 24, 14, COL(196,92,44));
        if (armN) rect_fill(sx-7, sy-24, 14, 24, COL(196,92,44));
        if (armS) rect_fill(sx-7, sy, 14, 24, COL(196,92,44));
        rect_fill(sx-13, sy-7, 26, 14, COL(196,92,44)); rect_line(sx-13, sy-7, 26, 14, COL(255,170,80));
        for (int f=0; f<3; f++){
            int fx = sx-11 + f*10, fh = 5 + (int)(fsin(G.anim_t*9.0f + (float)(s*7+f*3))*2.0f);
            tri_fill(fx-4, sy-7, fx+4, sy-7, fx, sy-7-fh, COL(255,170,60));
        }
        if (!armE && !armW && !armN && !armS) draw_text_center(sx, sy+14, "FIREWALL", 1, COL(255,200,150));
    } else if (type==WAR_S_GATE){
        // the gate (addendum 1): two gold posts and the energy panel
        // between them. The panel brightens while any of YOUR units
        // stands close — the door is open — and dims when the field
        // is clear. Bugs only ever see it shut.
        int open = 0;
        for (int i=0;i<WAR_MAXU;i++){
            if (S->war_uhp[i]<=0 || war_u_side(S->war_utype[i])!=0) continue;
            float dx=S->war_ux[i]-x, dy=S->war_uy[i]-y;
            if (dx*dx+dy*dy < 30.0f*30.0f){ open=1; break; }
        }
        blend_rect(sx-11, sy-9, 22, 18, COL(255,205,80), open? 170 : 90);
        rect_fill(sx-13, sy-11, 4, 22, COL(120,90,20)); rect_line(sx-13, sy-11, 4, 22, COL(255,205,80));
        rect_fill(sx+9, sy-11, 4, 22, COL(120,90,20)); rect_line(sx+9, sy-11, 4, 22, COL(255,205,80));
        disc_fill(sx-11, sy-13, 3, COL(255,205,80)); disc_fill(sx+11, sy-13, 3, COL(255,205,80));
        draw_text_center(sx, sy+16, "GATE", 1, COL(255,225,150));
    } else if (type==WAR_S_ASM){
        rect_fill(sx-26, sy-14, 52, 32, COL(96,88,110)); rect_line(sx-26, sy-14, 52, 32, COL(170,160,190));
        rect_fill(sx-26, sy-14, 52, 7, COL(90,160,220));
        rect_fill(sx+14, sy-26, 8, 12, COL(70,64,88)); rect_line(sx+14, sy-26, 8, 12, COL(140,132,160));
        disc_fill(sx-8, sy+3, 8, COL(52,48,66)); disc_fill(sx-8, sy+3, 4, COL(200,205,220));
        line_thick(sx-8, sy-5, sx-8, sy+11, 2, COL(200,205,220)); line_thick(sx-16, sy+3, sx, sy+3, 2, COL(200,205,220));
        rect_fill(sx+6, sy-2, 12, 10, COL(255,205,40)); rect_line(sx+6, sy-2, 12, 10, COL(120,90,20));
        // v1.21 cosmetic: the work light — amber and pulsing while the plant is building, dark when it isn't
        if (S->war_qlen[s]>0) disc_fill(sx+18, sy-20, 2, ((int)(G.anim_t*3.0f)&1)? COL(255,190,80) : COL(150,100,40));
        else disc_fill(sx+18, sy-20, 2, COL(70,58,40));
        draw_text_center(sx, sy+26, "ASSEMBLY", 1, COL(220,228,240));
    } else {   // the HIVE
        int strain = REGION_BUG[S->war_region]>=0? REGION_BUG[S->war_region] : 0;
        u32 hc = col_shade(BUG_COL[strain], 0.8f);
        int breathe = (int)(fsin(G.anim_t*1.8f + (float)s*2.0f)*1.5f);   // v1.21 cosmetic: the hive breathes
        ellipse_fill(sx, sy+6, 30 + breathe, 20, hc);
        ellipse_fill(sx, sy-4, 22, 16, col_shade(BUG_COL[strain], 0.95f));
        for (int k=0;k<5;k++){
            int kx = sx-20 + k*10;
            tri_fill(kx-4, sy-14, kx+4, sy-14, kx, sy-26-((k%2)? 4:0), col_shade(BUG_COL[strain], 0.6f));
        }
        int pr = 6 + (int)(fsin(G.anim_t*3.0f + (float)s)*1.5f);
        disc_fill(sx, sy-2, pr, BUG_BELLY[strain]);
        disc_fill(sx-9, sy-8, 3, COL(24,16,30)); disc_fill(sx+9, sy-8, 3, COL(24,16,30));
        draw_text_center(sx, sy+32, "BUG HIVE", 1, COL(255,190,170));
    }
    if (S->war_sflash[s] > 0)   // v1.21 cosmetic: the hit flash
        war_blend_disc(sx, sy, rad+3, COL(255,236,220), (int)(115.0f * (S->war_sflash[s]/0.12f)));
    if (S->war_shp[s] < S->war_smaxhp[s])
        war_hp_bar(sx - rad, sy - rad - 12, rad*2, S->war_shp[s] / (float)S->war_smaxhp[s], S->war_sside[s]);
    if (type==WAR_S_HIVE){   // v1.21: the hive's purse, printed under its name — you can watch the enemy economy breathe (this draw is already fog-gated by the caller)
        char pb[24]; char *q=pb; q=sapp_int(q, S->war_hbits[s]); q=sappend(q, " BITS"); *q=0;
        draw_text_center(sx, sy+44, pb, 1, COL(255,205,40));
    }
}
static void war_draw_unit(BwSim *S, Buddy *b, int i){
    float x = S->war_ux[i], y = S->war_uy[i];
    int type = S->war_utype[i];
    int sx = SX(x), sy = SY(y);
    int veh = (type==WAR_U_SCOUT || type==WAR_U_SIEGE);
    if (S->war_usel[i]) circle_out(sx, sy+4, type==WAR_U_SIEGE? 20 : type==WAR_U_SCOUT? 15 : 12, COL(140,255,140));
    war_blend_disc(sx, sy+6, veh? (type==WAR_U_SIEGE? 19:15) : 11, COL(10,8,18), 75);   // v1.21 cosmetic: a soft drop shadow under every unit
    ellipse_fill(sx, sy+6, veh? (type==WAR_U_SIEGE? 17:13) : 8, 3, COL(16,14,24));
    if (war_u_side(type)==0){
        u32 coat = bud_coat(b, WAR_SPCOL[b->species % BUDN]);
        int bob = (S->war_ustate[i]!=0 || S->war_urepair[i]!=0)? ((int)(G.anim_t*9.0f + (float)i*1.7f) & 1) : 0;   // repair work bobs too — a patcher under autoseek has no move state set
        if (type==WAR_U_SCOUT){
            // the scout buggy: light frame, four wheels, the driver's
            // head up top and a little pop-gun out front
            disc_fill(sx-8, sy+5, 3, COL(24,22,30)); disc_fill(sx+8, sy+5, 3, COL(24,22,30));
            disc_fill(sx-8, sy-5, 3, COL(24,22,30)); disc_fill(sx+8, sy-5, 3, COL(24,22,30));
            rect_fill(sx-11, sy-5, 22, 11, col_shade(coat, 0.9f)); rect_line(sx-11, sy-5, 22, 11, col_shade(coat, 0.6f));
            disc_fill(sx-2, sy-3-bob, 5, coat);
            disc_fill(sx-3, sy-4-bob, 1, COL(24,20,28)); disc_fill(sx, sy-4-bob, 1, COL(24,20,28));
            line_thick(sx+8, sy-1, sx+15, sy-1, 2, COL(220,226,235));
        } else if (type==WAR_U_SIEGE){
            // the siege tank: treads, a heavy hull, turret + long gun,
            // and the driver peeking out of the hatch
            rect_fill(sx-17, sy+3, 34, 6, COL(30,28,38)); rect_fill(sx-17, sy-10, 34, 6, COL(30,28,38));
            for (int w=0;w<4;w++){ disc_fill(sx-13+w*9, sy+6, 2, COL(90,86,100)); disc_fill(sx-13+w*9, sy-7, 2, COL(90,86,100)); }
            rect_fill(sx-15, sy-7, 30, 14, col_shade(coat, 0.85f)); rect_line(sx-15, sy-7, 30, 14, col_shade(coat, 0.55f));
            disc_fill(sx-3, sy-1, 7, col_shade(coat, 0.7f)); disc_fill(sx-3, sy-1, 4, col_shade(coat, 0.5f));
            line_thick(sx+2, sy-1, sx+21, sy-1, 3, COL(52,48,66));
            disc_fill(sx-10, sy-6-bob, 4, coat);
            disc_fill(sx-11, sy-7-bob, 1, COL(24,20,28)); disc_fill(sx-8, sy-7-bob, 1, COL(24,20,28));
        } else {
        disc_fill(sx, sy - bob, 8, coat);
        disc_fill(sx-3, sy-3-bob, 2, COL(24,20,28)); disc_fill(sx+3, sy-3-bob, 2, COL(24,20,28));
        if (type==WAR_U_HARV){
            rect_fill(sx-6, sy-10-bob, 12, 4, COL(255,205,40));   // the hard hat
            if (S->war_ucarry[i]>0) disc_fill(sx+8, sy+2-bob, 3, COL(255,205,40));
            for (int p=0;p<3;p++)   // v1.21 cosmetic: load pips — one lit pip per third of a full load, so a loaded harvester reads at a glance
                if (S->war_ucarry[i] > p*10) disc_fill(sx-9, sy+3-bob-p*4, 1, COL(255,205,40));
        } else if (type==WAR_U_MELEE){
            line_thick(sx+5, sy+4-bob, sx+11, sy-4-bob, 2, COL(220,226,235));   // the wrench
            disc_fill(sx+12, sy-5-bob, 2, COL(220,226,235));
        } else if (type==WAR_U_PATCH){
            // v1.21 (Daniel): a FIELD MEDIC, not a nurse — an olive
            // patrol helmet carrying the red cross on a white disc,
            // and a medkit satchel on its back with a cross of its
            // own. Draw code only; the patcher underneath is untouched.
            rect_fill(sx-11, sy-1-bob, 5, 8, COL(240,244,250)); rect_line(sx-11, sy-1-bob, 5, 8, COL(150,60,55));   // the medkit satchel
            rect_fill(sx-10, sy+2-bob, 3, 2, COL(255,90,80));
            rect_fill(sx-9, sy+1-bob, 1, 4, COL(255,90,80));
            ellipse_fill(sx, sy-8-bob, 8, 4, COL(104,116,72));   // the helmet
            disc_fill(sx, sy-8-bob, 3, COL(240,244,250));
            rect_fill(sx-1, sy-10-bob, 2, 5, COL(255,90,80));
            rect_fill(sx-2, sy-9-bob, 4, 2, COL(255,90,80));
        } else {
            disc_fill(sx, sy-9-bob, 3, COL(120,235,255));   // the bolt buddy's charged cap
            rect_fill(sx-1, sy-14-bob, 2, 4, COL(120,235,255));
        }
        }
    } else {
        int strain = REGION_BUG[S->war_region]>=0? REGION_BUG[S->war_region] : (i % 4);
        int legph = (int)(G.anim_t*11.0f + (float)i*2.1f) & 1;
        for (int l=0;l<3;l++){
            int lx = sx-6 + l*6;
            line_thick(lx, sy+3, lx-2, sy+8, 1, col_shade(BUG_COL[strain], 0.55f));
            line_thick(lx, sy+3, lx+3, sy+8 - (legph? 2:0), 1, col_shade(BUG_COL[strain], 0.55f));
        }
        ellipse_fill(sx, sy, type==WAR_U_SPIT? 10:9, 7, BUG_COL[strain]);
        ellipse_fill(sx, sy+2, 6, 4, type==WAR_U_BHARV? COL(168,124,52) : BUG_BELLY[strain]);   // v1.21 cosmetic: the bug harvester's bronze work-belly sets it apart from the fighters
        if (type==WAR_U_SPIT) rect_fill(sx+6, sy-2, 6, 4, col_shade(BUG_COL[strain], 0.8f));
        if (type==WAR_U_BHARV){   // v1.21: the bug harvester wears a gold-rimmed bit pack — the same gold your harvester hauls, and the same gold dot when it's carrying
            rect_fill(sx-5, sy-8, 10, 5, COL(150,110,40)); rect_line(sx-5, sy-8, 10, 5, COL(255,205,40));
            if (S->war_ucarry[i]>0) disc_fill(sx+8, sy+2, 3, COL(255,205,40));
            for (int p=0;p<3;p++)   // load pips beside the pack, same language as your harvester's
                if (S->war_ucarry[i] > p*10) disc_fill(sx+9, sy-6+p*4, 1, COL(255,205,40));
        }
        disc_fill(sx-3, sy-3, 2, COL(255,250,240)); disc_fill(sx+3, sy-3, 2, COL(255,250,240));
        disc_fill(sx-3, sy-3, 1, COL(16,12,20)); disc_fill(sx+3, sy-3, 1, COL(16,12,20));
    }
    if (S->war_uflash[i] > 0)   // v1.21 cosmetic: the hit flash
        war_blend_disc(sx, sy-2, veh? 17:11, COL(255,236,220), (int)(115.0f * (S->war_uflash[i]/0.12f)));
    if (S->war_uhp[i] < S->war_umaxhp[i])
        war_hp_bar(sx - (veh? 13:9), sy - (type==WAR_U_SIEGE? 22:16), veh? 26:18, S->war_uhp[i] / (float)S->war_umaxhp[i], war_u_side(type));
}
static void war_blend_disc(int cx, int cy, int r, u32 c, int alpha){
    for (int dy=-r; dy<=r; dy++){ int dx=(int)fsqrt((float)(r*r-dy*dy)); blend_rect(cx-dx, cy+dy, dx*2, 1, c, alpha); }
}
static void war_draw_terrain(BwSim *S, int t){
    float x = S->war_tx[t], y = S->war_ty[t];
    int sx = SX(x), sy = SY(y), kind = S->war_ttype[t], rad = S->war_trad[t];
    u32 vary = hash2(t, 11, 0x7E44u);
    if (S->war_thp[t]<=0){   // destroyed: only the fading scorch remains
        if (S->war_tdeadt[t]>0){
            ellipse_fill(sx, sy + rad*3/4, rad, rad/3, COL(28,24,22));
            disc_fill(sx - rad/3, sy + rad/2, 2, COL(44,38,32));
            disc_fill(sx + rad/4, sy + rad*2/3, 2, COL(38,32,28));
        }
        return;
    }
    ellipse_fill(sx, sy + rad*3/4, rad, rad/3, COL(16,14,24));
    if (kind==0){   // a tree: trunk, then a two-tone canopy in the region's tint
        u32 can = WAR_TREE_COL[S->war_region];
        rect_fill(sx-2, sy, 4, rad, COL(90,64,40));
        disc_fill(sx, sy - rad/2, rad + 2, col_shade(can, 0.75f));
        disc_fill(sx - rad/3, sy - rad/2 - 3, rad - 2, can);
        disc_fill(sx + rad/4, sy - rad/3, 3, col_shade(can, 1.3f));
    } else if (kind==1){   // rocks: grey lumps
        u32 rk = col_shade(COL(128,124,118), 0.85f + (float)(vary % 30u) * 0.01f);
        ellipse_fill(sx, sy, rad, rad*2/3, rk);
        ellipse_fill(sx - rad/2, sy + 2, rad/2, rad/3, col_shade(rk, 0.8f));
        disc_fill(sx + rad/4, sy - rad/3, 2, col_shade(rk, 1.25f));
    } else if (kind==2){   // a broken terminal, left by whoever debugged here last
        rect_fill(sx-9, sy-8, 18, 15, COL(40,42,54)); rect_line(sx-9, sy-8, 18, 15, COL(90,96,120));
        rect_fill(sx-6, sy-5, 12, 8, COL(24,60,66));
        line_thick(sx-5, sy-4, sx+1, sy+1, 1, COL(120,235,255)); line_thick(sx+1, sy+1, sx+5, sy-3, 1, COL(120,235,255));
        rect_fill(sx-3, sy+7, 6, 3, COL(52,54,68));
    } else if (kind==3){   // a cable spool
        ellipse_fill(sx, sy, rad, rad*3/4, COL(110,82,50));
        ellipse_fill(sx, sy, rad*2/3, rad/2, COL(84,60,36));
        disc_fill(sx, sy, 3, COL(52,40,26));
        line_thick(sx - rad*2/3, sy - rad/4, sx + rad*2/3, sy + rad/4, 2, COL(38,34,48));
    } else if (kind==4){   // rubble
        tri_fill(sx - rad, sy + 4, sx - rad/3, sy - rad/2, sx + 1, sy + 4, COL(104,96,88));
        tri_fill(sx - 2, sy + 4, sx + rad/3, sy - rad/3, sx + rad, sy + 4, COL(88,80,74));
        disc_fill(sx + 2, sy + 1, 2, COL(130,120,108));
    } else {   // kind 5 (v1.18): a barrier slab — dark slate, a lit top edge, one cyan seam. It will outlive us all.
        u32 sl = col_shade(COL(52,58,72), 0.9f + (float)(vary % 20u) * 0.01f);
        rect_fill(sx-rad, sy-rad, rad*2, rad*2, sl);
        rect_fill(sx-rad, sy-rad, rad*2, 3, col_shade(sl, 1.45f));
        rect_fill(sx-1, sy-rad+4, 2, rad*2-8, COL(110,225,235));
        rect_line(sx-rad, sy-rad, rad*2, rad*2, col_shade(sl, 0.6f));
    }
    if (S->war_thp[t] < (float)S->war_tmaxhp[t])   // the house health bar, same as units and buildings
        war_hp_bar(sx - rad, sy - rad - 12, rad*2, S->war_thp[t] / (float)S->war_tmaxhp[t], 0);
}
// the property tint, drawn while a build is in hand: your land glows
// softly, so you can see exactly how far your building rights reach.
// The faint outer ring is the adjacency reach for the build in hand:
// its footprint may poke that far past the zone edge and still count
// as touching your land (the ghost is the exact judge either way).
static void war_render_zones(BwSim *S){
    if (S->war_place<0) return;
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0 || S->war_sside[s]!=0) continue;
        int sx = SX(S->war_sx[s]), sy = SY(S->war_sy[s]);
        int zr = (int)war_zone_rad(S->war_stype[s]);
        int reach = zr + WAR_SRAD[S->war_place];
        war_blend_disc(sx, sy, reach, COL(120,220,255), 12);
        war_blend_disc(sx, sy, zr, COL(120,220,255), 34);
        circle_out(sx, sy, zr, COL(120,220,255));
        circle_out(sx, sy, reach, COL(90,160,190));
    }
}
// the placement ghost: green where the build is legal, red where
// it is not — drawn at a world point, whoever's finger is asking.
static void war_ghost_at(BwSim *S, float wx, float wy){
    int kind = S->war_place;
    if (kind==WAR_S_WALL){ wx = war_snap24(wx, WAR_OX); wy = war_snap24(wy, WAR_OY); }
    int okp = war_place_ok(S, kind, wx, wy);
    int sx = SX(wx), sy = SY(wy), rad = WAR_SRAD[kind];
    int ga = 95 + (int)(fsin(G.anim_t*6.0f)*25.0f);   // v1.21 cosmetic: the ghost breathes, and its footprint is squared so the fit is judgeable
    war_blend_disc(sx, sy, rad, okp? COL(120,255,120) : COL(255,90,80), ga);
    circle_out(sx, sy, rad, okp? COL(140,255,140) : COL(255,110,90));
    rect_line(sx-rad, sy-rad, rad*2, rad*2, okp? COL(140,255,140) : COL(255,110,90));
}
// the placement ghost under your finger. (Touch gives no hover, so
// the ghost lives while a finger or mouse button is down.) The pad
// cursor is a finger too (v1.16): while it is on and no touch point
// is asking, the ghost hovers at the cursor instead, so a pad player
// can judge a placement before pressing A.
static int war_view_pi = -1;   // feat 13: the player whose VIEW war_render is drawing right now (-1 outside a render)
static void war_render_ghost(int pi, int vx, int vy, int vw2, int vh2){
    BwSim *S = &BWS[pi];
    if (S->war_place<0) return;
    if (war_view_pi==G.my_id){   // the touch points belong to the local player — only their own view ghosts them (feat 13: the viewer, not the battle owner)
        for (int i=0;i<6;i++){
            if (!IN.tp[i].active || IN.tp[i].y >= vy+vh2-WAR_BAR_H) continue;
            float wx = S->war_camx + (float)(IN.tp[i].x - vx - vw2/2);
            float wy = S->war_camy + WAR_BAR_H*0.5f + (float)(IN.tp[i].y - vy - vh2/2);
            war_ghost_at(S, wx, wy);
            return;
        }
    }
    if (S->war_curon) war_ghost_at(S, S->war_curx, S->war_cury);
}
static void war_render(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    int vx = G.view_x0, vy = G.view_y0, vw2 = G.view_w>0? G.view_w : VW, vh2 = G.view_h>0? G.view_h : VH;
    if (S->war_state==0){ war_render_map(pi, vx, vy, vw2, vh2); return; }
    if (S->war_state==4){ war_render_setup(pi, vx, vy, vw2, vh2); return; }   // v1.19: the setup panel over the map
    G.zoom = 1.0f;
    G.camx = S->war_camx; G.camy = S->war_camy + WAR_BAR_H*0.5f;
    int rg = S->war_region;
    rect_fill(vx, vy, vw2, vh2, col_shade(REGION_GB[rg], 0.5f));
    { float wx0 = S->war_camx - vw2/2 - 80, wy0 = S->war_camy - vh2/2 - 80;
      float wx1 = S->war_camx + vw2/2 + 80, wy1 = S->war_camy + vh2/2 + 80;
      int gx0 = (int)(wx0/80.0f), gy0 = (int)(wy0/80.0f), gx1 = (int)(wx1/80.0f), gy1 = (int)(wy1/80.0f);
      for (int gy=gy0; gy<=gy1; gy++) for (int gx=gx0; gx<=gx1; gx++){
          float cxp = gx*80.0f+40, cyp = gy*80.0f+40;
          int inside = cxp>WAR_OX && cxp<WAR_OX+S->war_aw && cyp>WAR_OY && cyp<WAR_OY+S->war_ah;
          u32 c = inside? (((gx+gy)&1)? REGION_GA[rg] : REGION_GB[rg]) : col_shade(REGION_GB[rg], 0.45f);
          c = col_shade(c, 0.95f + hash_f(gx, gy, 0xB17u)*0.09f);
          rect_fill(SX(gx*80.0f), SY(gy*80.0f), 81, 81, c);
          if (inside){
              // v1.21 cosmetic: a faint survey grid + speckle, so
              // the ground has grain and movement reads against it
              int csx = SX(gx*80.0f), csy = SY(gy*80.0f);
              rect_fill(csx, csy, 81, 1, col_shade(c, 0.9f)); rect_fill(csx, csy, 1, 81, col_shade(c, 0.9f));
              disc_fill(csx + 12 + (int)(hash2(gx, gy, 21u) % 56), csy + 14 + (int)(hash2(gx, gy, 23u) % 52), 1, col_shade(c, 0.86f));
              disc_fill(csx + 10 + (int)(hash2(gx, gy, 27u) % 60), csy + 12 + (int)(hash2(gx, gy, 29u) % 56), 1, col_shade(c, 1.1f));
              if (hash2(gx, gy, 0x7A55u) % 5 == 0)
                  disc_fill(SX(gx*80.0f + 20 + (int)(hash2(gx, gy, 3u) % 36)), SY(gy*80.0f + 22 + (int)(hash2(gx, gy, 9u) % 34)), 3, REGION_TUFT[rg]);
          }
      }
      // the arena fence
      int ax0 = SX(WAR_OX), ay0 = SY(WAR_OY), ax1 = SX(WAR_OX+S->war_aw), ay1 = SY(WAR_OY+S->war_ah);
      rect_fill(ax0-5, ay0-5, (int)S->war_aw+10, 5, COL(70,44,44)); rect_fill(ax0-5, ay1, (int)S->war_aw+10, 5, COL(70,44,44));
      rect_fill(ax0-5, ay0-5, 5, (int)S->war_ah+10, COL(70,44,44)); rect_fill(ax1, ay0-5, 5, (int)S->war_ah+10, COL(70,44,44));
    }
    war_render_zones(S);
    for (int t=0;t<S->war_tn;t++)   // terrain is scenery: remembered once explored
        if (war_fog_at(S, S->war_tx[t], S->war_ty[t])>=1) war_draw_terrain(S, t);
    for (int n=0;n<S->war_nn;n++){
        if (S->war_namt[n]<=0) continue;
        if (war_fog_at(S, S->war_nx[n], S->war_ny[n])<1) continue;   // an unmapped node is just a rumour
        int sx = SX(S->war_nx[n]), sy = SY(S->war_ny[n]);
        // v1.21 cosmetic: the node is a crystal cluster with a soft
        // breathing glow — and it VISIBLY DEPLETES: the crystals
        // shrink and dim as the bits drain, so both economies read
        // at a glance (full > 500, working > 200, nearly dry below)
        float dsc = S->war_namt[n] > 500? 1.0f : S->war_namt[n] > 200? 0.72f : 0.45f;
        u32 cc1 = col_shade(COL(255,205,40), 0.6f + 0.4f*dsc);
        u32 cc2 = col_shade(COL(255,190,30), 0.6f + 0.4f*dsc);
        u32 cc3 = col_shade(COL(240,180,40), 0.6f + 0.4f*dsc);
        int pulse = (int)(fsin(G.anim_t*2.2f + (float)n*1.3f)*3.0f);
        war_blend_disc(sx, sy, 17 + pulse, COL(255,205,40), 20 + pulse*2);
        ellipse_fill(sx, sy+8, 16, 5, COL(90,74,30));
        tri_fill(sx-12, sy+8, sx-5, sy+8 - (int)(20.0f*dsc), sx+1, sy+8, cc1);
        tri_fill(sx-2, sy+8, sx+4, sy+8 - (int)(24.0f*dsc), sx+10, sy+8, cc2);
        tri_fill(sx+4, sy+8, sx+10, sy+8 - (int)(16.0f*dsc), sx+15, sy+8, cc3);
        disc_fill(sx+3, sy-6, 2, COL(255,250,220));
        { char nb[8]; char *q=nb; q=sapp_int(q, S->war_namt[n]); *q=0;
          draw_text_center(sx, sy+14, nb, 1, COL(255,230,150)); }
    }
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0) continue;
        if (S->war_sside[s]==1 && war_fog_at(S, S->war_sx[s], S->war_sy[s])<1) continue;   // a hive shows once scouted, and stays on the map
        war_draw_struct(S, s);
    }
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0) continue;
        if (war_u_side(S->war_utype[i])==1 && war_fog_at(S, S->war_ux[i], S->war_uy[i])<2) continue;   // bugs only while watched
        war_draw_unit(S, b, i);
    }
    for (int i=0;i<24;i++){   // v1.21 cosmetic: particles — death poofs and harvest sparkles, at the units' own layer
        if (S->war_ft[i]<=0) continue;
        if (war_fog_at(S, S->war_fx[i], S->war_fy[i])<1) continue;
        int sx = SX(S->war_fx[i]), sy = SY(S->war_fy[i]);
        float dur = S->war_fk[i]==2? 0.7f : S->war_fk[i]==1? 0.5f : 0.45f;
        float a = 1.0f - S->war_ft[i]/dur; if (a<0) a=0; if (a>1) a=1;
        if (S->war_fk[i]==1){   // the harvest sparkle: a small gold plus that rises and shrinks
            int arm = (int)(5.0f*(1.0f-a)) + 1, yy = sy - (int)(a*10.0f);
            line_thick(sx-arm, yy, sx+arm, yy, 1, COL(255,220,90));
            line_thick(sx, yy-arm, sx, yy+arm, 1, COL(255,220,90));
            disc_fill(sx, yy, 1, COL(255,250,220));
        } else {   // the poof: a soft expanding smoke disc (bigger for a building)
            int r = (S->war_fk[i]==2? 10 : 5) + (int)(a * (S->war_fk[i]==2? 20.0f : 11.0f));
            war_blend_disc(sx, sy, r, S->war_fk[i]==2? COL(130,120,115) : COL(205,198,190), (int)(120.0f*(1.0f-a)));
            war_blend_disc(sx - r/2, sy - r/3, r/2, COL(90,84,80), (int)(80.0f*(1.0f-a)));
        }
    }
    for (int i=0;i<WAR_MAXP;i++){
        if (S->war_pt[i]<=0) continue;
        if (S->war_pside[i]==1 && war_fog_at(S, S->war_px[i], S->war_py[i])<2) continue;
        int sx = SX(S->war_px[i]), sy = SY(S->war_py[i]);
        { float tx, ty;   // v1.21 cosmetic: a tracer streak behind the shot, aimed by its target
          if (war_ref_alive(S, S->war_ptar[i], &tx, &ty)){
              float dx = tx - S->war_px[i], dy = ty - S->war_py[i], d = fsqrt(dx*dx+dy*dy);
              if (d > 4.0f) line_thick(sx - (int)(dx/d*11.0f), sy - (int)(dy/d*11.0f), sx, sy, 2, S->war_pside[i]==0? COL(70,140,170) : COL(90,150,70));
          } }
        disc_fill(sx, sy, 3, S->war_pside[i]==0? COL(140,240,255) : COL(170,255,120));
        disc_fill(sx, sy, 1, COL(255,255,255));
    }
    // the fog itself: unexplored is solid dark, explored-but-unwatched
    // is dimmed. It sits above the field and below every marker.
    { float wx0 = S->war_camx - vw2/2, wy0 = S->war_camy - vh2/2;
      float wx1 = S->war_camx + vw2/2, wy1 = S->war_camy + vh2/2;
      int cx0 = (int)((wx0 - WAR_OX)/40.0f), cx1 = (int)((wx1 - WAR_OX)/40.0f);
      int cy0 = (int)((wy0 - WAR_OY)/40.0f), cy1 = (int)((wy1 - WAR_OY)/40.0f);
      if (cx0<0) cx0=0; if (cy0<0) cy0=0;
      if (cx1>=S->war_gw) cx1=S->war_gw-1; if (cy1>=S->war_gh) cy1=S->war_gh-1;   // v1.19: fog is drawn only over the live field's grid
      for (int cy=cy0; cy<=cy1; cy++) for (int cx=cx0; cx<=cx1; cx++){
          int f = S->war_fog[cy*WAR_FOG_W + cx];
          if (f==2) continue;
          int sx = SX(WAR_OX + cx*40.0f), sy = SY(WAR_OY + cy*40.0f);
          if (f==0){
              // v1.21 cosmetic: a STEPPED soft edge — an unexplored
              // cell bordering watched (or merely explored) ground
              // fades in two steps instead of ending at a hard
              // tile line
              int near2 = 0, near1 = 0;
              for (int oy=-1; oy<=1; oy++) for (int ox=-1; ox<=1; ox++){
                  int nx = cx+ox, ny = cy+oy;
                  if (nx<0 || ny<0 || nx>=S->war_gw || ny>=S->war_gh) continue;
                  int nf = S->war_fog[ny*WAR_FOG_W + nx];
                  if (nf==2) near2 = 1; else if (nf==1) near1 = 1;
              }
              if (near2) blend_rect(sx, sy, 41, 41, COL(6,6,12), 150);
              else if (near1) blend_rect(sx, sy, 41, 41, COL(6,6,12), 205);
              else rect_fill(sx, sy, 41, 41, COL(6,6,12));
          }
          else blend_rect(sx, sy, 41, 41, COL(4,4,10), 145);
      } }
    for (int i=0;i<WAR_MAXU;i++){   // selected units redraw above the fog, marker disc and all —
        if (S->war_uhp[i]<=0 || !S->war_usel[i]) continue;   // a bare circle_out here would bury the buddy under its own marker
        war_draw_unit(S, b, i);
    }
    war_render_ghost(pi, vx, vy, vw2, vh2);
    if (S->war_curon && S->war_state==1 && !S->war_barcap){   // the pad cursor rides above the fog, like the selected units do (v1.16). v1.20: while the command bar has captured pad control, the field cursor hides — the highlight is the pointer now. feat 13: no pi gate — war_render only ever runs for a local view (the owner's own, or a guest's by exchange), and the loaded view's curon is the VIEWER's cursor
        int csx = SX(S->war_curx), csy = SY(S->war_cury);
        int cr = 10 + (int)(fsin(G.anim_t*5.0f)*1.5f);
        circle_out(csx, csy, cr+3, COL(120,100,40));
        circle_out(csx, csy, cr, COL(255,240,160));
        line_thick(csx-cr-5, csy, csx-cr+2, csy, 2, COL(255,240,160));
        line_thick(csx+cr-2, csy, csx+cr+5, csy, 2, COL(255,240,160));
        line_thick(csx, csy-cr-5, csx, csy-cr+2, 2, COL(255,240,160));
        line_thick(csx, csy+cr-2, csx, csy+cr+5, 2, COL(255,240,160));
        disc_fill(csx, csy, 2, COL(255,240,160));
    }
    if (S->war_ordt > 0){
        int sx = SX(S->war_ordx), sy = SY(S->war_ordy);
        int r = 6 + (int)((0.5f - S->war_ordt) * 14.0f);
        circle_out(sx, sy, r, COL(140,255,140));
        disc_fill(sx, sy, 2, COL(140,255,140));
    }
    if (S->war_selstruct>=0 && war_struct_alive(S, S->war_selstruct) && S->war_rallyx[S->war_selstruct]!=0){
        // the rally flag (addendum 3): while a producer with a rally
        // set is selected, a thin line runs from the building to the
        // flag. Like the order marker, it ignores fog.
        int ss = S->war_selstruct;
        int bx = SX(S->war_sx[ss]), by2 = SY(S->war_sy[ss]);
        int fx = SX(S->war_rallyx[ss]), fy = SY(S->war_rallyy[ss]);
        line_thick(bx, by2, fx, fy, 1, COL(140,255,140));
        line_thick(fx, fy, fx, fy-14, 2, COL(200,220,200));
        tri_fill(fx, fy-14, fx, fy-7, fx+9, fy-10, COL(140,255,140));
        disc_fill(fx, fy, 2, COL(140,255,140));
    }
    // the top strip: bits, region, what's left to kill
    rect_fill(vx, vy, vw2, 20, COL(10,10,18));
    disc_fill(vx+12, vy+10, 5, COL(255,205,40));
    { char buf[64]; char *q=buf; q=sappend(q, "BITS "); q=sapp_int(q, S->war_bits); *q=0;
      draw_text(vx+22, vy+6, buf, 1, COL(255,230,150)); }
    draw_text_center(vx+vw2/2, vy+6, S->war_skirmish? "SKIRMISH" : REGION_NAME[rg], 1, COL(240,244,250));
    { char buf[48]; char *q=buf; q=sappend(q, "HIVES "); q=sapp_int(q, war_hives_alive(S));
      q=sappend(q, "   ARMY "); q=sapp_int(q, war_count_units(S, 0)); q=sappend(q, "/24"); *q=0;
      draw_text(vx+vw2-text_w(buf, 1)-8, vy+6, buf, 1, COL(255,190,170)); }
    if (S->war_msgt > 0){
        int tw = text_w(S->war_msg, 1);
        rect_fill(vx+vw2/2 - tw/2 - 6, vy+25, tw+12, 15, COL(16,14,24));
        rect_line(vx+vw2/2 - tw/2 - 6, vy+25, tw+12, 15, COL(198,164,84));   // v1.21 cosmetic: the banner wears a gold frame
        draw_text_center(vx+vw2/2, vy+28, S->war_msg, 1, COL(255,240,200));
    }
    war_render_bar(pi, vx, vy, vw2, vh2);
    if (S->war_state==2 || S->war_state==3){
        blend_rect(vx, vy, vw2, vh2, COL(8,8,14), 110);
        int pw = vw2<420? vw2-24 : 396, ph = 108;
        int px = vx+(vw2-pw)/2, py = vy+(vh2-ph)/2 - 20;
        rect_fill(px, py, pw, ph, COL(16,18,28)); rect_line(px, py, pw, ph, S->war_state==2? COL(255,205,40) : COL(255,110,90));
        if (S->war_state==2){
            draw_text_center(px+pw/2, py+16, S->war_skirmish? "SKIRMISH WON!" : "REGION CONQUERED!", 2, COL(255,220,120));
            draw_text_center(px+pw/2, py+44, S->war_skirmish? "SKIRMISH" : REGION_NAME[rg], 1, COL(240,244,250));
            draw_text_center(px+pw/2, py+62, S->war_skirmish? "THE FIELD IS YOURS... THIS TIME." : "THE SWARM RETREATS... FOR NOW.", 1, COL(200,220,200));
        } else {
            draw_text_center(px+pw/2, py+16, "THE CORE HAS FALLEN", 2, COL(255,140,120));
            draw_text_center(px+pw/2, py+44, S->war_skirmish? "SKIRMISH" : REGION_NAME[rg], 1, COL(240,244,250));
            draw_text_center(px+pw/2, py+62, S->war_skirmish? "THE FIELD KEEPS ITS SECRETS. TRY AGAIN!" : "THE REGION STAYS WILD. REBUILD AND TRY AGAIN!", 1, COL(220,200,200));
        }
        { char b2[48]; char *q=b2; q=sappend(q, lbla(H_POINT)); q=sappend(q, " TO RETURN TO THE MAP"); *q=0;
          draw_text_center(px+pw/2, py+ph-16, b2, 1, COL(150,160,185)); }
    }
    if (S->war_pause){   // the pause menu (v1.17): same rects the tap handler and pad use
        blend_rect(vx, vy, vw2, vh2, COL(8,8,14), 150);
        int px, py, pw, ph; war_pause_rects(vx, vy, vw2, vh2, &px, &py, &pw, &ph);
        rect_fill(px, py, pw, ph, COL(16,18,28)); rect_line(px, py, pw, ph, COL(255,205,40));
        draw_text_center(px+pw/2, py+14, "PAUSED", 2, COL(255,220,120));
        { char buf[48]; char *q=buf; q=sappend(q, S->war_skirmish? "SKIRMISH" : REGION_NAME[rg]); q=sappend(q, " - THE BATTLE WAITS"); *q=0;
          draw_text_center(px+pw/2, py+36, buf, 1, COL(200,210,230)); }
        const char *PLBL[3] = { "RESUME", "RESTART BATTLE", S->war_skirmish? "QUIT BATTLE" : "SAVE & QUIT" };   // v1.19: a skirmish is never parked — its third choice quits outright
        for (int n=0;n<3;n++){
            int by = war_pause_btn_y(py, n);
            rect_fill(px+40, by, pw-80, 36, COL(38,44,60));
            rect_line(px+40, by, pw-80, 36, COL(198,164,84));
            draw_text_center(px+pw/2, by+12, PLBL[n], 2, COL(240,244,250));
        }
        draw_text_center(px+pw/2, py+ph-16, hint_dev()==DEV_PAD? "A - PICK A CHOICE - B RESUMES" : (hint_dev()==DEV_TOUCH? "TAP A CHOICE" : "KEYS 1 / 2 / 3 - ESC RESUMES"), 1, COL(150,160,185));
    }
}

static void bw_render_modes(int pi){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    if (S->bat.active){ bw_render_battle(pi); return; }
    if (b->bw_mode==2){ bw_render_ent(pi); return; }
    if (b->bw_mode==3){ plat_render(pi); return; }
    if (b->bw_mode==4){ craft_render(pi); return; }
    if (b->bw_mode==5){ tank_render(pi); return; }
    if (b->bw_mode==6){ cas_render(pi); return; }
    if (b->bw_mode==7){ if (S->crt_guest) crt_render_view(S->crt_guest - 1, pi); else crt_render(pi); return; }
    if (b->bw_mode==8){
        if (S->war_guest){   // feat 13: a guest views the OWNER's battle wearing their own view (Cart's crt_render_view, by exchange)
            BwSim *HS = &BWS[S->war_guest-1];
            war_view_exchange(HS, S);
            war_view_pi = pi;
            war_render(S->war_guest-1);
            war_view_exchange(HS, S);
            war_view_pi = -1;
        } else { war_view_pi = pi; war_render(pi); war_view_pi = -1; }
        return;
    }
    u32 seed=(u32)b->bw_seed;
    G.camx=b->x; G.camy=b->y;
    // the view: full screen solo, or this player's split half in co-op
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    rect_fill(vx, vy, vw2, vh2, COL(84,156,62));
    int tx0=bw_tileof(b->x - vw2/2 - BW_TILE), tx1=bw_tileof(b->x + vw2/2 + BW_TILE);
    int ty0=bw_tileof(b->y - vh2/2 - BW_TILE), ty1=bw_tileof(b->y + vh2/2 + BW_TILE);
    for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
        int x=SX(tx*BW_TILE), y=SY(ty*BW_TILE);
        u32 h = hash2(tx, ty, seed ^ 0x7A55u);
        // palette lift (Nov 3, Daniel: "kinda drab"): a checker you can
        // actually see, per-tile shade weather, and sunlit patches
        int site = bw_site_at_xy(b, tx*BW_TILE+24, ty*BW_TILE+24);
        int ocean = bw_outside_world(tx*BW_TILE+24, ty*BW_TILE+24);
        { u32 base;
          if (ocean){
              // the sea: two blues in a checker, lighter in the
              // shallows within a tile or so of the shore
              float wx2 = tx*BW_TILE+24, wy2 = ty*BW_TILE+24;
              int shallow = wx2 > BW_WX0-84 && wx2 < BW_WX1+84 && wy2 > BW_WY0-84 && wy2 < BW_WY1+84;
              base = shallow? (((tx+ty)&1)==0? COL(74,146,196) : COL(66,136,186))
                            : (((tx+ty)&1)==0? COL(36,88,146) : COL(32,80,136));
              rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(base, 0.94f + (float)((h>>20)%14)*0.01f));
              if (h % 7 == 0){ int wxp = x + (int)((h>>6) % 30) + 6 + (int)(G.anim_t*6 + (h%40)) % 13 - 6,
                                  wyp = y + (int)((h>>11) % 32) + 8 + (int)(fsin(G.anim_t*1.3f + (float)(h%9))*1.5f);
                  line_thick(wxp, wyp, wxp+4, wyp-2, 1, COL(150,200,235)); line_thick(wxp+4, wyp-2, wxp+8, wyp, 1, COL(150,200,235)); }
              continue;
          }
          float road_d = (b->bw_mode==1 && site<0)? bw_road_dist(tx*BW_TILE+24, ty*BW_TILE+24) : 1e9f;
          if (road_d < 42)  base = road_d<26? (((tx+ty)&1)==0? COL(170,150,98) : COL(160,140,90))    // the worn road itself
                                            : (((tx+ty)&1)==0? COL(138,166,86) : COL(128,156,78));   // its trampled edge
          else if (site==0) base = ((tx+ty)&1)==0? COL(182,156,102) : COL(172,146,94);
          else if (site==1) base = ((tx+ty)&1)==0? COL(132,134,132) : COL(122,124,122);
          else if (site==2) base = ((tx+ty)&1)==0? COL(178,216,236) : COL(166,204,226);
          else if (site==3) base = ((tx+ty)&1)==0? COL(102,98,104) : COL(92,88,94);
          else if (site==4) base = ((tx+ty)&1)==0? COL(126,136,100) : COL(116,126,92);
          else { base = ((tx+ty)&1)==0? COL(104,178,80) : COL(84,156,62); if (h % 19 == 0) base = COL(126,192,86); }
          rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(base, 0.94f + (float)((h>>20)%14)*0.01f)); }
        // terrain grain on the open grass (Daniel, Nov 6): worn dirt
        // patches and speckle, so the meadow reads as GROUND, not as
        // one flat green sheet — kept quiet on purpose
        if (!ocean && site<0){
            if ((h>>3)%11==0){ int px2 = x + (int)((h>>6)%26)+8, py2 = y + (int)((h>>11)%24)+10;
                disc_fill(px2, py2, 6, COL(156,138,92)); disc_fill(px2+7, py2+2, 4, COL(148,130,86)); disc_fill(px2-6, py2+3, 4, COL(150,134,90)); }
            if ((h>>5)%3==0){ pset(x + (int)((h>>7)%40)+4, y + (int)((h>>13)%40)+4, COL(70,138,52));
                pset(x + (int)((h>>17)%40)+4, y + (int)((h>>23)%40)+4, COL(140,205,105)); }
        }
        int lush = bw_lush_at(tx, ty, seed);
        if (!ocean && site!=1 && site!=2 && site!=3){ // grass tufts, thick where it's lush
            int tufts = lush==2? (h%5==0? 0 : 4) : lush==1? (h%3==0? 3 : (h%3==1? 2 : 0)) : (h%6==0? 2 : 0);
            for (int g2=0; g2<tufts; g2++){ u32 hh = hash2(tx*7+g2*13, ty*7-g2*11, seed ^ 0x6A11u);
                int gx = x + (int)(hh % 38) + 5, gy = y + (int)((hh>>8) % 36) + 7;
                line_thick(gx, gy, gx-1, gy-5, 1, COL(58,124,46)); line_thick(gx+3, gy, gx+4, gy-4, 1, COL(134,202,100)); } }
        if (!ocean && (site<0 || site==0) && lush>=1 && h % (lush==2? 9 : 16) == 0){ // a bush: clustered dark discs, one lit top
            int bx2 = x + (int)((h>>7) % 30) + 9, by2 = y + (int)((h>>13) % 28) + 10;
            disc_fill(bx2-5, by2+2, 5, COL(44,102,42)); disc_fill(bx2+5, by2+2, 5, COL(44,102,42));
            disc_fill(bx2, by2-2, 6, COL(52,116,48)); disc_fill(bx2-2, by2-4, 3, COL(86,150,70)); }
        if (!ocean && site<0 && lush==2 && (h>>5) % 14 == 0){ // mushrooms in the deep green only
            int mx2 = x + (int)((h>>9) % 32) + 8, my2 = y + (int)((h>>15) % 30) + 9;
            pset(mx2, my2, COL(240,238,228)); pset(mx2, my2-1, COL(240,238,228));
            disc_fill(mx2, my2-3, 2, COL(206,64,54)); pset(mx2-1, my2-3, COL(255,240,230));
            pset(mx2+6, my2+2, COL(240,238,228)); disc_fill(mx2+6, my2, 1, COL(206,64,54)); }
        if (!ocean && h % (lush==2? 6 : lush==1? 11 : 33) == 0 && (site<0 || site==0)){ // flower clusters, not pinpricks: three petals + a heart
            static const u32 FLW[4] = { COL(255,216,90), COL(255,150,190), COL(190,160,255), COL(250,250,246) };
            u32 fc = FLW[(h>>16)&3];
            int fx2 = x + (int)((h>>6) % 34) + 6, fy2 = y + (int)((h>>11) % 34) + 7;
            disc_fill(fx2-2, fy2, 1, fc); disc_fill(fx2+2, fy2, 1, fc); disc_fill(fx2, fy2-2, 1, fc);
            pset(fx2, fy2, COL(255,240,190)); }
    }
    // the shoreline (glow-up): a dashed foam line where meadow
    // meets sea, crawling slowly so the water feels alive
    { int ph = (int)(G.anim_t*2.0f);
      int fx0 = SX(BW_WX0), fx1 = SX(BW_WX1), fy0 = SY(BW_WY0), fy1 = SY(BW_WY1);
      int vy0 = vy, vy1 = vy+vh2, vx0 = vx, vx1 = vx+vw2;
      if (fx0>=vx0 && fx0<vx1) for (int yy=vy0; yy<vy1; yy+=7) if ((((yy/7)+ph)&1)==0) rect_fill(fx0-1, yy, 2, 4, COL(228,244,252));
      if (fx1>=vx0 && fx1<vx1) for (int yy=vy0; yy<vy1; yy+=7) if ((((yy/7)+ph)&1)==0) rect_fill(fx1-1, yy, 2, 4, COL(228,244,252));
      if (fy0>=vy0 && fy0<vy1) for (int xx=vx0; xx<vx1; xx+=7) if ((((xx/7)+ph)&1)==0) rect_fill(xx, fy0-1, 4, 2, COL(228,244,252));
      if (fy1>=vy0 && fy1<vy1) for (int xx=vx0; xx<vx1; xx+=7) if ((((xx/7)+ph)&1)==0) rect_fill(xx, fy1-1, 4, 2, COL(228,244,252)); }
    bw_draw_gate();
    bw_draw_objects(pi, 0);
    draw_buddy(b);
    bw_draw_objects(pi, 1);
    ui_vignette(vx, vy, vw2, vh2);
    // THE LIVING SKY (v1.25 feat 12): the same tint the overworld
    // wears, laid over the meadow but under the buddy card, the
    // prompt, the minimap and the note — BW's UI never sits under
    // the sky. Only this outdoor fall-through reaches here: every
    // cabinet and interior mode returned above, Entrances included.
    tod_apply_tint();
    // the buddy card
    rect_fill(vx+8, vy+8, 168, 46, COL(20,26,20)); rect_line(vx+8, vy+8, 168, 46, COL(198,164,84));
    { char buf[64]; char *q=buf; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[b->species%BUDN]); *q=0;
      draw_text(vx+14, vy+13, buf, 1, COL(255,244,210)); }
    { float f = b->maxhp>0? b->hp/b->maxhp : 0; if (f<0) f=0;
      rect_fill(vx+14, vy+26, 120, 5, COL(40,30,40)); rect_fill(vx+14, vy+26, (int)(120*f), 5, f>0.35f? COL(120,230,90) : COL(255,170,60));
      char hb[24]; char *q=hb; q=sapp_int(q, (int)b->hp); q=sappend(q, "/"); q=sapp_int(q, (int)b->maxhp); *q=0;
      draw_text(vx+138, vy+25, hb, 1, COL(230,240,230)); }
    { float f = b->energy/BUD_ENERGY_MAX; if (f<0) f=0;
      rect_fill(vx+14, vy+36, 120, 4, COL(40,30,40)); rect_fill(vx+14, vy+36, (int)(120*f), 4, COL(120,200,255));
      draw_text(vx+138, vy+35, "ENERGY", 1, COL(170,220,255)); }
    // the context prompt (Daniel, Nov 6): ONE line, only when there is
    // something to interact with, naming the actual key/button for
    // the device in hand — never the whole control list
    { const char *verb = 0;
      float dx=b->x, dy=b->y+6;
      if (bw_npc_near(b, 46)>=0) verb = "TALK";
      else if (bw_term_near(b, 44)>=0) verb = "PRESS THE TERMINAL";
      else if (bw_sign_near(b, 44)) verb = "READ THE SCRATCHES";
      else if (bw_hsign_near(b, 44)>=0) verb = "READ THE SIGN";
      else if (bw_bitling_near(b, 44)) verb = "SAY HI";
      else if (bw_tool_near(b, 44)>=0) verb = "PICK IT UP";
      else if (dx*dx+dy*dy < 46*46) verb = "BACK TO THE BUG MALL";
      else { float tx, ty; if (bw_tent_near(b, 42, &tx, &ty)) verb = "SLEEP (FULL HP + ENERGY)"; }
      if (verb){ char hb[72]; const char *hint = key_prompt(hb, lbl_int(), verb);
      int hw2 = text_w(hint, 1);
      rect_fill(vx + vw2/2 - hw2/2 - 6, vy + vh2-22, hw2+12, 14, COL(20,26,20));
      draw_text_center(vx + vw2/2, vy + vh2-19, hint, 1, COL(240,248,240)); } }
    bw_draw_minimap(pi);
    bw_draw_note(pi);
    bw_draw_ch2_hud(pi);   // THE REBUILDING (feat 14): the cider run's countdown, when a run is live
    if (S->log_open){
        static const char *QN[10] = { "MEET ELDER PATCHINA", "CLEAR THE SERVER FARM", "HOUSE CALL FOR A HERMIT", "ZERO-DAY FUNERAL", "MOP'S LOST TOOLS", "THE SUNKEN CACHE", "COLD STORAGE", "SPECIAL DELIVERY", "THE STRAY PROCESS", "READ THE SIGNS" };
        // a guiding line per quest (Nov 4, Daniel: "more detailed,
        // clearer — don't give it all away"): shown while ACTIVE only
        static const char *QD[10] = {
            "THE ELDER WAITS IN PATCH VILLAGE, NORTH-WEST.",
            "BONK 3 HACKERS AT THE FARM. THEN MIND THE TERMINALS.",
            "GLITCH LIVES ALONE AT THE FROZEN LAKE, FAR SOUTH.",
            "ZED WAITS PAST HIS FENCE. GATES OPEN FOR CRANKS.",
            "TOOLS GLINT GOLD OUT THERE. THE DIAMOND FINDS ONE.",
            "THE HOARDER SITS IN THE WESTERN RUINS. 4TH HITS HEAL IT.",
            "BONK 2 HACKERS ON THE FROZEN LAKE ITSELF.",
            "CARRY MOP'S PARCEL TO GLITCH AT THE LAKE.",
            "A STRAY PROCESS WANDERS SOUTH OF PATCH VILLAGE.",
            "READ SCRATCHED SIGNS BY ROADS AND PASSES. ANY THREE." };
        int w = vw2<420? vw2-16 : 404;
        int need = 26 + 4 + 14 + 4*13 + 16;
        if (b->bw_mode!=1) need = 26 + 14 + 4 + 14 + 4*13 + 16;
        else for (int q2=0; q2<10; q2++){ int st = bw_qstate_g(b, q2); if (st) need += st==1? 23 : 12; }
        int h = need; if (h > vh2-8) h = vh2-8; if (h < 150) h = 150;
        int x = vx + (vw2-w)/2, y = vy + (vh2-h)/2; if (y < vy+4) y = vy+4;
        rect_fill(x, y, w, h, COL(16,20,18)); rect_line(x, y, w, h, COL(198,164,84));
        draw_text(x+12, y+8, "QUEST LOG", 1, COL(255,214,64));
        draw_text(x+w-104, y+8, "F / X - CLOSE", 1, COL(150,170,190));
        int ry = y + 26;
        if (b->bw_mode!=1){
            draw_text(x+12, ry, "NO STORY HERE. JUST MEADOW, TEA, AND VIOLENCE.", 1, COL(200,214,200)); ry += 14;
        } else for (int q2=0; q2<10; q2++){
            int st = bw_qstate_g(b, q2);
            if (st==0) continue;
            if (ry > y+h-86) break;   // keep room for the backpack below
            draw_text(x+12, ry, QN[q2], 1, st==2? COL(140,160,140) : COL(255,244,210));
            char tb[28]; char *tq=tb;
            if (st==2){ tq=sappend(tq, "DONE"); }
            else if (q2==1){ tq=sappend(tq, "ACTIVE "); tq=sapp_int(tq, bw_farmkills(b)); tq=sappend(tq, "/3"); }
            else if (q2==4){ tq=sappend(tq, "ACTIVE "); tq=sapp_int(tq, bw_tools_found(b)); tq=sappend(tq, "/3"); }
            else if (q2==6){ tq=sappend(tq, "ACTIVE "); tq=sapp_int(tq, bw_lakekills(b)); tq=sappend(tq, "/2"); }
            else if (q2==9){ tq=sappend(tq, "ACTIVE "); tq=sapp_int(tq, bw_signs_read(b)); tq=sappend(tq, "/3"); }
            else tq=sappend(tq, "ACTIVE");
            *tq=0;
            draw_text(x+w-12-text_w(tb, 1), ry, tb, 1, st==2? COL(120,200,120) : COL(255,200,110));
            ry += 12;
            if (st==1){ draw_text(x+24, ry, QD[q2], 1, COL(168,190,205)); ry += 11; }
        }
        ry += 4;
        draw_text(x+12, ry, "BACKPACK (BW ONLY)", 1, COL(255,214,64)); ry += 14;
        for (int it=0; it<4; it++){
            if (it==S->log_sel){ rect_fill(x+8, ry-2, w-16, 13, COL(70,58,30)); draw_text(x+12, ry, ">", 1, COL(255,220,90)); }
            draw_text(x+24, ry, BW_ITEM_NAME[it], 1, b->items[it]>0? COL(240,248,240) : COL(110,110,120));
            char cb[12]; char *cq=cb; cq=sappend(cq, "x"); cq=sapp_int(cq, b->items[it]); *cq=0;
            draw_text(x+w-12-text_w(cb, 1), ry, cb, 1, COL(255,210,130));
            ry += 13;
        }
        { char b2[64]; char *q=b2; q=sappend(q, lbla(H_FIRE)); q=sappend(q, " - USE.  ITEMS ALSO WORK MID-BATTLE."); *q=0;
          draw_text_center(x+w/2, y+h-14, b2, 1, COL(150,170,190)); }
    }
    if (pi==G.my_id && mobile_ui() && !pad_mode() && !S->log_open){
        const char *hint = G.touch_ctl? "STICK MOVE - TALK TALKS - PICK FOR BATTLES" : "DRAG TO MOVE - TAP TO TALK";
        int hw2 = text_w(hint, 1);
        rect_fill(vx + vw2/2 - hw2/2 - 6, vy + vh2-22, hw2+12, 14, COL(20,26,20));
        draw_text_center(vx + vw2/2, vy + vh2-19, hint, 1, COL(240,248,240));
    }
    if (S->sleep_t>0){
        float d = 1.0f - (S->sleep_t>0.7f? (S->sleep_t-0.7f)/0.7f : S->sleep_t/0.7f);
        blend_rect(vx, vy, vw2, vh2, COL(6,6,16), (int)(210*d));
        if (d>0.75f) draw_text_center(vx + vw2/2, vy + vh2/2-4, "Z Z Z", 2, COL(200,210,255));
    }
}
// --- touch controls inside Banzai World (Daniel, Nov 5) ----------------
// The main game's HUD drew the stick and buttons; the minigames never
// did, so on a phone they were invisible — and an invisible control
// scheme reads as a broken one. Same fixed spots as the meadow, with
// the button captions naming what they DO in this mode.
static void bw_draw_touch(int pi){
    if (pi!=G.my_id || !mobile_ui() || pad_mode()) return;
    Buddy *b=&G.bud[pi]; BwSim *S=&BWS[pi];
    if (S->bat.active) return;                    // battles run on taps + menus
    if (b->bw_mode==8) return;                    // WAR! is tap-only: its command bar owns the bottom strip
    int vw2 = G.view_w>0? G.view_w : VW;
    if (vw2<VW) return;                           // split view: fixed buttons are full-screen furniture
    const char *wl = (b->bw_mode==4)? "CRAFT" : (b->bw_mode==2)? "PATCH" : "WALL";
    if (G.touch_ctl==0){                          // DRAG: the finger steers; one button is furniture enough
        if (b->bw_mode==3 || b->bw_mode==5 || S->craft_menu) return;
        tbtn_base(TP_WALL_CX, TP_WALL_CY, 24, COL(62,56,86));
        { u32 br = COL(232,186,108);
          rect_fill(TP_WALL_CX-10, TP_WALL_CY-11, 9, 4, br); rect_fill(TP_WALL_CX+1, TP_WALL_CY-11, 9, 4, br);
          rect_fill(TP_WALL_CX-10, TP_WALL_CY-6, 6, 4, br);  rect_fill(TP_WALL_CX-3, TP_WALL_CY-6, 6, 4, br);
          rect_fill(TP_WALL_CX+4, TP_WALL_CY-6, 6, 4, br); }
        draw_text(TP_WALL_CX-text_w(wl,1)/2, TP_WALL_CY+4, wl, 1, COL(255,244,200));
        return;
    }
    // STICK: FIRE + USE + WALL, captioned per mode, plus the stick itself
    const char *fl = "PICK", *ul = "TALK";
    int have_wall = 1;
    if (b->bw_mode==2){ fl="THROW"; ul="OPEN"; }
    else if (b->bw_mode==3){ fl="JUMP"; ul=0; have_wall=0; }
    else if (b->bw_mode==4){ fl="SWING"; ul="USE"; }
    else if (b->bw_mode==5){ fl="FIRE"; ul=0; have_wall=0; }
    if (!S->craft_menu){
        tbtn_base(TP_FIRE_CX, TP_FIRE_CY, 34, (IN.tp_held&32)? COL(224,88,62):COL(158,54,46));
        { u32 lc = COL(255,240,230); int gy = TP_FIRE_CY-6;
          rect_fill(TP_FIRE_CX-1, gy-10, 3, 6, lc); rect_fill(TP_FIRE_CX-1, gy+5, 3, 6, lc);
          rect_fill(TP_FIRE_CX-10, gy-1, 6, 3, lc); rect_fill(TP_FIRE_CX+5, gy-1, 6, 3, lc);
          disc_fill(TP_FIRE_CX, gy, 2, lc); }
        draw_text(TP_FIRE_CX-text_w(fl,1)/2, TP_FIRE_CY+14, fl, 1, COL(255,240,230));
        if (ul){
            tbtn_base(TP_ACT_CX, TP_ACT_CY, 22, COL(56,92,62));
            { u32 lc = COL(230,255,235); int gy = TP_ACT_CY-3;
              tri_fill(TP_ACT_CX, gy-8, TP_ACT_CX-6, gy+1, TP_ACT_CX+6, gy+1, lc);
              rect_fill(TP_ACT_CX-2, gy+1, 5, 5, lc); }
            draw_text(TP_ACT_CX-text_w(ul,1)/2, TP_ACT_CY+9, ul, 1, COL(230,255,235));
        }
        if (have_wall){
            tbtn_base(TP_WALL_CX, TP_WALL_CY, 24, COL(62,56,86));
            { u32 br = COL(232,186,108);
              rect_fill(TP_WALL_CX-10, TP_WALL_CY-11, 9, 4, br); rect_fill(TP_WALL_CX+1, TP_WALL_CY-11, 9, 4, br);
              rect_fill(TP_WALL_CX-10, TP_WALL_CY-6, 6, 4, br);  rect_fill(TP_WALL_CX-3, TP_WALL_CY-6, 6, 4, br);
              rect_fill(TP_WALL_CX+4, TP_WALL_CY-6, 6, 4, br); }
            draw_text(TP_WALL_CX-text_w(wl,1)/2, TP_WALL_CY+4, wl, 1, COL(255,244,200));
        }
    }
    if (IN.joy_on){
        circle_out(IN.joy_ax, IN.joy_ay, 42, COL(34,31,52));
        circle_shade(IN.joy_ax, IN.joy_ay, 39, COL(40,36,58));
        disc_fill(IN.joy_ax, IN.joy_ay, 25, COL(30,27,46));
        circle_out(IN.joy_kx, IN.joy_ky, 17, COL(198,204,220));
        circle_shade(IN.joy_kx, IN.joy_ky, 14, COL(198,204,220));
        disc_fill(IN.joy_kx-5, IN.joy_ky-5, 4, COL(244,246,250));
    } else if (!S->craft_menu){
        disc_fill(TP_DPAD_CX, TP_DPAD_CY, 42, COL(24,22,38));
        circle_out(TP_DPAD_CX, TP_DPAD_CY, 15, COL(52,48,72));
    }
}
static void bw_render(int pi){
    g_dev_ctx = dev_of(pi);   // v1.25: this view's hints name this player's device
    bw_render_modes(pi);
    bw_draw_touch(pi);
    // the Start-menu and its CONTROLS panel, over this player's view
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    if (S->bw_menu){
        int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
        if (S->bw_menu==2){ draw_controls_panel(vx, vy, vw2, vh2, b->bw_mode<2? 1 : (b->bw_mode==5? 6 : b->bw_mode)); return; }
        int pw=300, ph=140, px=vx+(vw2-pw)/2, py=vy+(vh2-ph)/2;
        blend_rect(vx, vy, vw2, vh2, COL(8,8,14), 120);
        rect_fill(px, py, pw, ph, COL(16,18,28)); rect_line(px, py, pw, ph, COL(198,164,84));
        draw_text_center(px+pw/2, py+12, "PAUSED", 2, COL(255,214,64));
        { static const char *rows[3] = { "RESUME", "CONTROLS", "LEAVE" };
          for (int i=0;i<3;i++){
              int ry = py+48+i*26;
              if (i==S->bw_msel){ rect_fill(px+10, ry-4, pw-20, 19, COL(255,205,40)); draw_text_center(px+pw/2, ry, rows[i], 2, COL(30,26,44)); }
              else draw_text_center(px+pw/2, ry, rows[i], 2, COL(235,238,245));
          } }
        draw_text_center(px+pw/2, py+ph-16, pad_mode()? "STICK/DPAD + A" : (mobile_ui()? "TAP A ROW" : "ARROWS + SPACE"), 1, COL(140,150,170));
    }
}
static void bw_render_battle(int pi){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    int band = vh2<300? vh2/2 : 150;
    rect_fill(vx, vy, vw2, vh2, COL(22,40,28));
    rect_fill(vx, vy+vh2-band, vw2, band, COL(30,56,34));
    ellipse_fill(vx+vw2/2, vy+vh2*58/100, vw2*39/100, vh2*12/100, COL(52,92,50));
    ellipse_fill(vx+vw2/2, vy+vh2*58/100, vw2*26/100, vh2*8/100, COL(66,110,58));
    // fighters on a stage far from any real map
    float sx = b->x, sy = b->y, sa = b->aim;
    G.camx = 20000; G.camy = 20000;
    // spotlight pools under the fighters (glow-up)
    { int bxp = SX(20000-112), byp = SY(20000+34), exp2 = SX(20000+118), eyp = SY(20000-24);
      ellipse_fill(bxp, byp+9, 46, 13, COL(84,128,66)); ellipse_fill(bxp, byp+9, 30, 8, COL(102,148,80));
      ellipse_fill(exp2, eyp+17, 46, 13, COL(84,128,66)); ellipse_fill(exp2, eyp+17, 30, 8, COL(102,148,80)); }
    b->x = 20000-112; b->y = 20000+34; b->aim = 0;
    draw_buddy(b);
    b->x = sx; b->y = sy; b->aim = sa;
    if (S->bat.boss) bw_draw_big_hacker(20000+118, 20000-14, S->bat.boss, 0);
    else bw_draw_hacker(20000+118, 20000-24, S->bat.lvl, (S->bat.phase==2 && S->bat.t>0.85f)? 1.0f : 0.0f, 0);
    // foe plate
    rect_fill(vx+14, vy+12, 212, 34, COL(16,20,16)); rect_line(vx+14, vy+12, 212, 34, COL(255,120,110));
    { char buf[64]; char *q=buf; q=sappend(q, bw_foe_name(S)); q=sappend(q, "  LV "); q=sapp_int(q, S->bat.lvl); *q=0;
      draw_text(vx+20, vy+17, buf, 1, COL(255,210,200)); }
    { float f = S->bat.hmax>0? S->bat.hhp/S->bat.hmax : 0; if (f<0) f=0;
      rect_fill(vx+20, vy+31, 150, 5, COL(40,30,40)); rect_fill(vx+20, vy+31, (int)(150*f), 5, COL(255,120,90));
      char hb[24]; char *q=hb; q=sapp_int(q, (int)S->bat.hhp); q=sappend(q, "/"); q=sapp_int(q, (int)S->bat.hmax); *q=0;
      draw_text(vx+174, vy+30, hb, 1, COL(255,225,215)); }
    // buddy plate (bottom-right of the view, above the command box)
    { int px0 = vx+vw2-224, py0 = vy+vh2-128-((S->bat.natt>6)? ((S->bat.natt+1)/2-3)*15 : 0); if (py0 < vy+52) py0 = vy+52;
    rect_fill(px0, py0, 210, 40, COL(16,20,16)); rect_line(px0, py0, 210, 40, COL(198,164,84));
    { char buf[64]; char *q=buf; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[b->species%BUDN]); *q=0;
      draw_text(px0+6, py0+5, buf, 1, COL(255,244,210)); }
    { float f = b->maxhp>0? b->hp/b->maxhp : 0; if (f<0) f=0;
      rect_fill(px0+6, py0+17, 130, 5, COL(40,30,40)); rect_fill(px0+6, py0+17, (int)(130*f), 5, f>0.35f? COL(120,230,90) : COL(255,170,60));
      char hb[24]; char *q=hb; q=sapp_int(q, (int)b->hp); q=sappend(q, "/"); q=sapp_int(q, (int)b->maxhp); *q=0;
      draw_text(px0+140, py0+16, hb, 1, COL(230,240,230));
      float ef = b->energy/BUD_ENERGY_MAX; if (ef<0) ef=0;
      rect_fill(px0+6, py0+26, 130, 4, COL(40,30,40)); rect_fill(px0+6, py0+26, (int)(130*ef), 4, COL(120,200,255));
      char eb[24]; char *q2=eb; q2=sapp_int(q2, (int)b->energy); q2=sappend(q2, " EN"); *q2=0;
      draw_text(px0+140, py0+25, eb, 1, COL(170,220,255)); } }
    // the bottom box: attack menu or message (taller once items join)
    int rows = (S->bat.natt+1)/2, bh = 84 + (rows>3? (rows-3)*15 : 0), rstep = rows>3? 15 : 18;
    rect_fill(vx, vy+vh2-bh, vw2, bh, COL(14,16,22)); rect_line(vx, vy+vh2-bh, vw2, bh, COL(198,164,84));
    if (S->bat.phase==0){
        draw_text(vx+16, vy+vh2-bh+6, "CHOOSE AN ATTACK:", 1, COL(255,244,210));
        for (int i=0;i<S->bat.natt;i++){
            int mv = S->bat.attacks[i];
            const char *nm; float cost, mult; bw_attack_info(b, mv, &nm, &cost, &mult);
            int cxp = vx+16 + (i%2)*(vw2/2-20), cyp = vy+vh2-bh+22 + (i/2)*rstep;
            int afford = b->energy >= cost;
            if (i==S->bat.sel) draw_text(cxp, cyp, ">", 1, COL(255,220,90));
            draw_text(cxp+12, cyp, nm, 1, afford? COL(240,248,240) : COL(110,110,120));
            if (mv>=6){ char cb[20]; char *q=cb; q=sappend(q, "x"); q=sapp_int(q, b->items[mv-6]); *q=0;
                draw_text(cxp + vw2/2-130, cyp, cb, 1, COL(255,210,130)); }
            else if (cost>0){ char cb[20]; char *q=cb; q=sapp_int(q, (int)cost); q=sappend(q, " EN"); *q=0;
                draw_text(cxp + vw2/2-130, cyp, cb, 1, afford? COL(170,220,255) : COL(110,110,120)); }
            else if (mv!=5) draw_text(cxp + vw2/2-130, cyp, "FREE", 1, COL(160,230,160));
        }
    } else {
        draw_text(vx+16, vy+vh2-bh+14, S->bat.msg, 1, COL(255,250,235));
        if (S->bat.msg2[0]) draw_text(vx+16, vy+vh2-bh+32, S->bat.msg2, 1, COL(200,214,235));
    }
}

