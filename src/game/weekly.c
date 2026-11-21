// THE SURVIVAL GAME — game/weekly.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from save.c lines 459-665 in the Nov 2026 fine-grained refactor; code unchanged.

static void wk_test_set_day(int d){ wk_day_override = d; }
static int wk_week_now(void){
    int d = wk_day_override>=0? wk_day_override : plat_daynum();
    return (d + 3) / 7;   // epoch day 0 was a Thursday; +3 starts blocks on Mondays
}
static void wk_challenge(int week, int *type, int *target){
    static const int NVAR[WK_NTYPES] = { 3,3,3,2,2,2,1,2,2,2 };
    static const int VAR[WK_NTYPES][3] = {
        {30,50,80}, {40,60,100}, {50,100,200}, {1,2,0}, {1,2,0},
        {2,3,0}, {1,0,0}, {1,3,0}, {100,250,0}, {4,6,0}
    };
    // hash2, never the gameplay RNG: the derivation must be identical
    // on every install in the same week, and the RNG is clock-seeded.
    u32 h = hash2(week, 7919, 0x9E3779B9u);
    int t = (int)(h % (u32)WK_NTYPES);
    u32 h2 = hash2(week, 104729, 0x85EBCA6Bu);
    *type = t;
    *target = VAR[t][(int)(h2 % (u32)NVAR[t])];
}
// the current value of a delta source. Per-buddy sources read the
// crew best (max over connected players) — the First Shift rule that
// any player's doing it counts; run totals are shared by construction.
static int wk_src_val(int src){
    int best = 0;
    switch (src){
        case WK_S_KILLS: return G.total_kills;
        case WK_S_SHOP: return fs_fingerprint();
        case WK_S_CASH: return (int)G.total_earned;
        case WK_S_WAR:
            for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.bud[i].war_prog>best) best = G.bud[i].war_prog;
            return best;
        case WK_S_TANK:
            for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.bud[i].tank_done>best) best = G.bud[i].tank_done;
            return best;
        case WK_S_MEDAL:
            for (int i=0;i<MAXP;i++) if (G.p[i].connected){
                Buddy *b=&G.bud[i];
                int n = crt_medal_count(b,1)+crt_medal_count(b,2)+crt_medal_count(b,3)
                      + plat_medal_count(b,1)+plat_medal_count(b,2)+plat_medal_count(b,3);
                if (n>best) best = n;
            }
            return best;
        case WK_S_QUEST:
            // quests 0-13 (feat 14): the task says "FINISH A STORY
            // QUEST" and the Rebuilding's quests are story quests;
            // the week-start baseline keeps pre-finished ones honest
            for (int i=0;i<MAXP;i++) if (G.p[i].connected){
                int n=0; for (int q=0;q<14;q++) if (bw_qstate_g(&G.bud[i],q)==2) n++;
                if (n>best) best = n;
            }
            return best;
    }
    return 0;
}
static void wk_snapshot(void){
    for (int s=0;s<7;s++) G.wk_base[s] = wk_src_val(s);
}
static int wk_progress(void){
    int type, target; wk_challenge(G.wk_week, &type, &target);
    int v = 0;
    switch (type){
        case WK_T_KILLS: v = G.total_kills - G.wk_base[WK_S_KILLS]; break;
        case WK_T_WAR:   v = wk_src_val(WK_S_WAR)   - G.wk_base[WK_S_WAR]; break;
        case WK_T_MEDAL: v = wk_src_val(WK_S_MEDAL) - G.wk_base[WK_S_MEDAL]; break;
        case WK_T_TANK:  v = wk_src_val(WK_S_TANK)  - G.wk_base[WK_S_TANK]; break;
        case WK_T_QUEST: v = wk_src_val(WK_S_QUEST) - G.wk_base[WK_S_QUEST]; break;
        case WK_T_SHOP:  v = fs_fingerprint()       - G.wk_base[WK_S_SHOP]; break;
        case WK_T_CASH:  v = (int)G.total_earned    - G.wk_base[WK_S_CASH]; break;
        case WK_T_HAT:
            for (int i=0;i<MAXP;i++) if (G.p[i].connected && (int)G.p[i].carried>v) v = (int)G.p[i].carried;
            break;
        case WK_T_BANK:
            for (int i=0;i<MAXP;i++) if (G.p[i].connected && (int)G.p[i].banked>v) v = (int)G.p[i].banked;
            break;
        case WK_T_WALLS:
            for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1) v++;
            break;
    }
    return v<0? 0 : v;
}
// open the current week for this slot: the outgoing week becomes the
// board's PREVIOUS row (with its done flag), the new week starts with
// fresh baselines, and "done" is derived so the new week is unpaid.
static void wk_rollover(void){
    int now = wk_week_now();
    if (G.wk_week==now) return;
    if (G.wk_week!=0){
        G.wk_prev_week = G.wk_week;
        G.wk_prev_done = (G.wk_done_week==G.wk_week)? 1 : 0;
    }
    G.wk_week = now;
    wk_snapshot();
}
static float wk_roll_t = 0;
static void wk_update(float dt){
    if (wk_flash_t>0 && G.state==GS_PLAY) wk_flash_t -= dt;
    if (G.net_mode==NET_CLIENT || G.wk_week==0) return;   // the host owns the week (fs_update's rule)
    if (G.state!=GS_PLAY && G.state!=GS_STORE) return;
    wk_roll_t -= dt;
    if (wk_roll_t<=0){   // a session left running over midnight rolls too
        wk_roll_t = 1.0f;
        int before = G.wk_week;
        wk_rollover();
        if (G.wk_week!=before){
            wk_flash_kind = 2; wk_flash_t = 3.5f;
            if (G.autosave_t>0.5f) G.autosave_t = 0.5f;   // persist promptly
        }
    }
    if (G.wk_done_week==G.wk_week) return;
    int type, target; wk_challenge(G.wk_week, &type, &target);
    if (wk_progress() < target) return;
    // complete: pay every connected player's buddy through the one
    // shared door; a player with no buddy yet banks it in the First
    // Shift tuition pool, which buddy_adopt flushes on adoption.
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        if (G.bud[i].active) buddy_award_xp(i, WK_XP);
        else G.fs_pool[i] += WK_XP;
    }
    { char b[24]; char *q=b; q=sappend(q, "WEEKLY +"); q=sapp_int(q, WK_XP); q=sappend(q, " XP"); *q=0;
      Player *me=&G.p[G.my_id>=0? G.my_id : 0];
      spawn_floater(me->x, me->y-30, b, COL(150,255,190)); }
    sfx(11);
    G.wk_done_week = G.wk_week;
    G.wk_total++;
    wk_flash_kind = 1; wk_flash_t = 3.5f;
    if (G.autosave_t>0.5f) G.autosave_t = 0.5f;   // persist promptly
}
static char *wk_task(int type, int target, char *buf){
    char *q=buf;
    switch (type){
        case WK_T_KILLS: q=sappend(q, "SQUASH "); q=sapp_int(q, target); q=sappend(q, " BUGS THIS WEEK."); break;
        case WK_T_HAT:   q=sappend(q, "WEAR A HAT OF "); q=sapp_int(q, target); q=sappend(q, " KG OR MORE."); break;
        case WK_T_BANK:  q=sappend(q, "HOLD "); q=sapp_int(q, target); q=sappend(q, " KG IN THE BUG BANK."); break;
        case WK_T_WAR:   q=sappend(q, "CONQUER "); q=sapp_int(q, target); q=sappend(q, target==1? " WAR! REGION." : " WAR! REGIONS."); break;
        case WK_T_MEDAL: q=sappend(q, "EARN "); q=sapp_int(q, target); q=sappend(q, target==1? " NEW MEDAL IN CART OR RUN." : " NEW MEDALS IN CART OR RUN."); break;
        case WK_T_TANK:  q=sappend(q, "CLEAR "); q=sapp_int(q, target); q=sappend(q, " TANK RESCUE LEVELS."); break;
        case WK_T_QUEST: q=sappend(q, "FINISH A STORY QUEST IN BANZAI WORLD."); break;
        case WK_T_SHOP:  q=sappend(q, "BUY "); q=sapp_int(q, target); q=sappend(q, target==1? " THING AT THE MALL." : " THINGS AT THE MALL."); break;
        case WK_T_CASH:  q=sappend(q, "CASH IN "); q=sapp_int(q, target); q=sappend(q, " KG AT THE COUNTER."); break;
        case WK_T_WALLS: q=sappend(q, "STAND "); q=sapp_int(q, target); q=sappend(q, " CARCASS WALLS AT ONCE."); break;
        default: q=sappend(q, "..."); break;
    }
    *q=0; return q;   // the END pointer: wk_row_blurb appends the pay line
}
// the board's row model (Mall station 6): one helper feeds the row
// count, the names, the right-hand values and the blurbs, so the
// panel can never disagree with itself (the Trophy Case rule).
enum { WK_HDR_WEEK, WK_CHAL, WK_HDR_LEDGER, WK_TOTAL, WK_PREV };
static int wk_row_ids(int *ids){
    int n=0;
    ids[n++]=WK_HDR_WEEK;
    ids[n++]=WK_CHAL;
    if (G.net_mode==NET_CLIENT) return n;   // a guest reads the card; the host keeps the book
    ids[n++]=WK_HDR_LEDGER;
    ids[n++]=WK_TOTAL;
    if (G.wk_prev_week) ids[n++]=WK_PREV;
    return n;
}
static int wk_row_id(int row){
    int ids[8]; int n=wk_row_ids(ids);
    return (row>=0 && row<n)? ids[row] : -1;
}
static int wk_rows(void){ int ids[8]; return wk_row_ids(ids); }
static const char *wk_row_name(int row, char *buf){
    char *q=buf;
    switch (wk_row_id(row)){
        case WK_HDR_WEEK: return "THIS WEEK";
        case WK_CHAL: { int t,tg; wk_challenge(G.wk_week, &t, &tg); return WK_NAME[t]; }
        case WK_HDR_LEDGER: return "THE LEDGER";
        case WK_TOTAL: return "CHALLENGES DONE";
        case WK_PREV: { int t,tg; wk_challenge(G.wk_prev_week, &t, &tg);
                        q=sappend(q, "PREVIOUS: "); q=sappend(q, WK_NAME[t]); *q=0; return buf; }
        default: return "";
    }
}
static const char *wk_row_right(int row, char *buf){
    char *q=buf;
    switch (wk_row_id(row)){
        case WK_CHAL: {
            if (G.net_mode==NET_CLIENT) return "HOST'S BOARD";
            int t,tg; wk_challenge(G.wk_week, &t, &tg);
            if (G.wk_done_week==G.wk_week) return "DONE!";
            q=sapp_int(q, wk_progress()); q=sappend(q, "/"); q=sapp_int(q, tg); *q=0; return buf; }
        case WK_TOTAL: q=sapp_int(q, G.wk_total); *q=0; return buf;
        case WK_PREV: return G.wk_prev_done? "DONE" : "MISSED";
        default: return "";
    }
}
static char wk_blurb_buf[160];
static const char *wk_row_blurb(int row){
    char *q;
    switch (wk_row_id(row)){
        case WK_HDR_WEEK: return "ONE FOR EVERYBODY, EVERY WEEK. MISS ONE AND NOTHING IS LOST.";
        case WK_CHAL: {
            if (G.net_mode==NET_CLIENT) return "THE HOST KEEPS THIS WEEK'S BOARD. YOUR OWN SLOT HAS ONE TOO.";
            int t,tg; wk_challenge(G.wk_week, &t, &tg);
            q=wk_task(t, tg, wk_blurb_buf);
            if (G.wk_done_week==G.wk_week) q=sappend(q, " DONE - PAID AND POSTED.");
            else { q=sappend(q, " PAYS "); q=sapp_int(q, WK_XP); q=sappend(q, " BUDDY XP WHEN IT LANDS."); }
            *q=0; return wk_blurb_buf; }
        case WK_HDR_LEDGER: return "THE BOOK OF WEEKS. IT ONLY EVER COUNTS THE FINISHES.";
        case WK_TOTAL: return "EVERY WEEK YOU FINISH, COUNTED HERE. NO STREAKS. NO GUILT.";
        case WK_PREV: return G.wk_prev_done? "LAST TIME OUT, AND YOU DID IT. THE BOARD REMEMBERS KINDLY."
                                           : "UNFINISHED LAST WEEK. IT COST NOTHING. THERE IS ALWAYS THIS WEEK.";
        default: return "";
    }
}
