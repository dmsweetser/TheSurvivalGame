// THE SURVIVAL GAME — war/coop.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 189-296 in the Nov 2026 fine-grained refactor; code unchanged.

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
