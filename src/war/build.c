// THE SURVIVAL GAME — war/build.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 1618-1806 in the Nov 2026 fine-grained refactor; code unchanged.

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
                           acts[n]=10; costs[n]=WAR_BUILD_COST[5]; names[n]="ASSEMBLY"; n++;
                           acts[n]=14; costs[n]=WAR_BUILD_COST[7]; names[n]="BIT MINER"; n++; }   // v1.33: the trickle rig (Section A)
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
    if (act==6){ S->war_place=-1; S->war_bcat=-1; S->war_wdrag=0; sfx(6); return; }   // v1.33: the bar's CANCEL also drops a wall run's preview
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
    if (act==14){ S->war_place = WAR_S_MINER; S->war_bcat=-1; sfx(6); { char nb[128]; char *q=nb; q=sappend(q, war_place_verb()); q=sappend(q, " TO PLACE THE BIT MINER - IT MUST SIT NEAR THE CORE. +1 BIT A SECOND, FOREVER."); *q=0; war_note(S, nb); } return; }   // v1.33
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
    if (kind==WAR_S_MINER){   // v1.33: the miner runs near the base it feeds — within 340u of the Core
        int hq = war_find_hq(S);
        if (hq<0){ sfx(16); war_note(S, "NO CORE TO FEED!"); return; }
        float dx=wx-S->war_sx[hq], dy=wy-S->war_sy[hq];
        if (dx*dx+dy*dy > 340.0f*340.0f){ sfx(16); war_note(S, "BIT MINERS MUST SIT NEAR THE CORE!"); return; }
    }
    S->war_bits -= cost;
    int s = war_spawn_struct(S, kind, 0, wx, wy);
    S->war_place = -1; S->war_bcat=-1;
    for (int i=0;i<WAR_MAXU;i++) S->war_usel[i]=0;
    S->war_selstruct = s;
    sfx(19);
    war_note(S, kind==1? "PATCH BAY ONLINE! IT TRAINS FIGHTERS." : kind==2? "ZAPPER TOWER ONLINE!" : kind==5? "ASSEMBLY PLANT ONLINE! IT BUILDS VEHICLES." : kind==6? "GATE UP! YOUR UNITS PASS - BUGS DON'T." : kind==7? "BIT MINER ONLINE! THE BITS TRICKLE IN." : "FIREWALL UP!");
}
// -------------------------------------------------------------------
// v1.33 (Section G): DRAG-TO-BUILD WALLS. With FIREWALL armed, a
// field drag (finger or mouse) no longer pans — it draws a RUN of
// segments along the dominant axis on the 24u grid, previewed live
// with its segment count and total cost. Releasing a run of 2+
// leaves the preview standing for an explicit APPROVE / CANCEL
// (tappable chips, Enter, or the pad's A / B); approving builds
// every individually placeable segment in drag order until the
// purse can't cover the next placeable one — ONE charge for the
// run, never a charge for a segment that didn't build. A one-
// segment drag (or a plain tap) places singly, exactly as before.
// The pad's equivalent: A marks the start, the cursor draws the
// run, A locks the preview, A again approves, B peels it away.
// The preview is transient UI state — it is never snapshotted.
// -------------------------------------------------------------------
static int war_wall_run(BwSim *S, float *xs, float *ys, int maxn){   // the run's segment centres, in drag order; returns the count
    float x0 = war_snap24(S->war_wdx0, WAR_OX), y0 = war_snap24(S->war_wdy0, WAR_OY);
    float x1 = war_snap24(S->war_wdx1, WAR_OX), y1 = war_snap24(S->war_wdy1, WAR_OY);
    float dx = x1-x0, dy = y1-y0;
    float adx = dx<0? -dx : dx, ady = dy<0? -dy : dy;
    int n = 0;
    if (adx >= ady){   // horizontal run at y0
        float step = dx>=0? 24.0f : -24.0f;
        for (float x=x0; ; x+=step){
            if (n<maxn){ xs[n]=x; ys[n]=y0; }
            n++;
            if ((step>0 && x>=x1) || (step<0 && x<=x1)) break;
            if (n>200) break;   // a runaway run is a bug, not a wall: bounded, always
        }
    } else {   // vertical run at x0
        float step = dy>=0? 24.0f : -24.0f;
        for (float y=y0; ; y+=step){
            if (n<maxn){ xs[n]=x0; ys[n]=y; }
            n++;
            if ((step>0 && y>=y1) || (step<0 && y<=y1)) break;
            if (n>200) break;
        }
    }
    return n;
}
static void war_wall_cancel(BwSim *S){
    S->war_wdrag = 0;
    war_note(S, "WALL RUN CANCELLED.");
    sfx(6);
}
static void war_wall_commit(int pi){
    BwSim *S = war_bs(pi);
    float xs[200], ys[200];
    int n = war_wall_run(S, xs, ys, 200);
    int cost = WAR_BUILD_COST[WAR_S_WALL], built = 0, spent = 0, last = -1;
    for (int k=0;k<n;k++){
        if (!war_place_ok(S, WAR_S_WALL, xs[k], ys[k])) continue;   // an unplaceable segment is skipped, never charged
        if (S->war_bits < cost) break;   // the purse ran dry mid-run: what stands, stands
        int s = war_spawn_struct(S, WAR_S_WALL, 0, xs[k], ys[k]);
        if (s<0) break;   // the field is full of buildings
        S->war_bits -= cost; spent += cost; built++; last = s;
    }
    if (built==0){
        war_note(S, S->war_bits < cost? "NOT ENOUGH BITS FOR THAT RUN!" : "NO ROOM FOR THAT RUN!");
        sfx(16);
        return;   // the preview stands — the commander can pan, rethink, or cancel it
    }
    S->war_wdrag = 0; S->war_place = -1; S->war_bcat = -1;
    for (int i=0;i<WAR_MAXU;i++) S->war_usel[i]=0;
    S->war_selstruct = last;
    { char nb[64]; char *q=nb; q=sapp_int(q, built); q=sappend(q, built>1? " FIREWALLS UP! -" : " FIREWALL UP! -"); q=sapp_int(q, spent); q=sappend(q, " BITS."); *q=0; war_note(S, nb); }
    sfx(19);
}
// the APPROVE / CANCEL chips (screen rects, shared by the renderer
// and the tap handler): two 84x26 chips above the command bar's
// right end, drawn only while a run awaits approval
static void war_wall_chip_rects(int vx, int vy, int vw2, int vh2, int *ax, int *ay, int *cx, int *cy){
    *ay = *cy = vy + vh2 - WAR_BAR_H - 34;
    *ax = vx + vw2 - 186; *cx = vx + vw2 - 94;
}
