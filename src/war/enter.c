// THE SURVIVAL GAME — war/enter.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 1569-1617 in the Nov 2026 fine-grained refactor; code unchanged.

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
