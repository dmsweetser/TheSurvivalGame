// THE SURVIVAL GAME — game/first_shift.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from save.c lines 378-458 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// FIRST SHIFT (v1.25) — the guided first-session chain: six small
// objectives the first time a slot is started, each paying buddy XP
// through the one shared door (buddy_award_xp). Conditions poll live
// state every frame, so anything the player already did counts the
// moment its step comes up. Full design + save notes: v125-spec.md.
// ---------------------------------------------------------------------------
static int fs_fingerprint(void){
    // everything the shop till can grant, summed over the crew —
    // weapon copies past the free starter, upgrade levels, the hat
    // stand, beans, adopted buddies. A fresh crew sums to 0, and a
    // co-op partner dropping in with a starter keyboard adds 0 too.
    int fp = 0;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        Player *p=&G.p[i];
        fp += (p->owned[0]>0? p->owned[0]-1 : 0);
        for (int w=1;w<NWEAP;w++) fp += p->owned[w];
        for (int u=0;u<5;u++) fp += p->upg[u];
        fp += p->hatstand + p->beans + (G.bud[i].active? 1:0);
    }
    return fp;
}
static int fs_step_done(int step){
    switch (step){
        case 0: return G.total_kills>=1;
        case 1: for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].carried>=10.0f) return 1; return 0;
        case 2: for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1) return 1; return 0;
        case 3: return fs_fingerprint() > G.fs_fp0;
        case 4: for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].banked>0.0f) return 1; return 0;
        case 5: for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.bud[i].in_bw) return 1; return 0;
    }
    return 0;
}
static void fs_update(float dt){
    // the banner only burns down where it can be seen (GS_PLAY), so
    // a step finished inside a station panel still gets its moment
    if (fs_flash_t>0 && G.state==GS_PLAY) fs_flash_t -= dt;
    if (G.fs_state!=1 || G.net_mode==NET_CLIENT) return;
    if (G.state!=GS_PLAY && G.state!=GS_STORE) return;
    int step = G.fs_step;
    if (step<0 || step>5 || !fs_step_done(step)) return;
    int amt = FS_XP[step];
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        if (G.bud[i].active) buddy_award_xp(i, amt);
        else G.fs_pool[i] += amt;      // tuition: lands on adoption
    }
    { char b[24]; char *q=b; q=sappend(q, "FIRST SHIFT +"); q=sapp_int(q, amt); q=sappend(q, " XP"); *q=0;
      Player *me=&G.p[G.my_id>=0? G.my_id : 0];
      spawn_floater(me->x, me->y-30, b, COL(150,255,190)); }
    sfx(11);
    fs_flash_amt = amt;
    fs_flash_fin = (step==5);
    fs_flash_t = 3.5f;
    if (step>=5){ G.fs_state=2; G.fs_step=6; }
    else G.fs_step = step+1;
    if (G.autosave_t>0.5f) G.autosave_t = 0.5f;   // persist promptly
}
static void fs_skip(void){
    if (G.fs_state!=1) return;
    G.fs_state=3; fs_flash_t=0;
    do_save();
}
// ---------------------------------------------------------------------------
// WEEKLY CHALLENGE (v1.25 feature 6) — one goal a week, identical for
// every player in that week because it is a pure function of the week
// id, drawn from counters the game already keeps. The board (Mall
// station 6) is the card; this is the bookkeeping: week math, the
// challenge table, baseline snapshots for the delta sources, the
// completion poll and the payout. Full design: v125-feat6-spec.md.
// ---------------------------------------------------------------------------
#define WK_XP 50   // one week-sized job: First Shift pays 65 for six small steps
enum { WK_T_KILLS, WK_T_HAT, WK_T_BANK, WK_T_WAR, WK_T_MEDAL,
       WK_T_TANK, WK_T_QUEST, WK_T_SHOP, WK_T_CASH, WK_T_WALLS, WK_NTYPES };
enum { WK_S_KILLS, WK_S_WAR, WK_S_MEDAL, WK_S_TANK, WK_S_QUEST, WK_S_SHOP, WK_S_CASH };
static const char *WK_NAME[WK_NTYPES] = {
    "BUG SQUASHER", "BIG HAT WEEK", "BANK HOLIDAY", "WAR EFFORT", "MEDAL HUNT",
    "TANK TROOP", "STORY TIME", "MALL RATS", "CASH IN", "WALL BUILDERS"
};
// the perceived day, for tests only (dbg_wksetday): -1 = the device
// clock. Runtime state — it never reaches a save.
static int wk_day_override = -1;
