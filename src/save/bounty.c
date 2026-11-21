// THE SURVIVAL GAME — save/bounty.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from save.c lines 666-790 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// NAMED ELITES + BOUNTY BOARD (v1.25 feature 8) — the bookkeeping
// behind Mall station 7. The elite itself lives in buddies.c (the
// tables, the spawn elevation, the kill hand-off); this is the
// contract: the claim settlement, the cooldown, the live tracking
// for the board's LAST SEEN row, and the panel's row model. Full
// design: v125-feat8-spec.md.
// ---------------------------------------------------------------------------
// settle a pending claim: the killer's bank gains the elite's weight
// again, every connected buddy gains BN_XP through the shared door
// (or the tuition pool), the ledger and the serial advance, and the
// cooldown starts. Called only from bn_update, never from combat.
static void bn_settle_claim(void){
    int pi = G.bn_claim_pi;
    if (pi<0 || pi>=MAXP || !G.p[pi].connected){
        pi = -1;
        for (int i=0;i<MAXP;i++) if (G.p[i].connected){ pi = i; break; }
    }
    if (pi>=0) G.p[pi].banked += G.bn_claim_w;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        if (G.bud[i].active) buddy_award_xp(i, BN_XP);
        else G.fs_pool[i] += BN_XP;
    }
    { char b[32]; char *q=b; q=sappend(q, "BOUNTY +"); q=sapp_int(q, (int)G.bn_claim_w); q=sappend(q, " KG BANKED"); *q=0;
      Player *me=&G.p[G.my_id>=0? G.my_id : 0];
      spawn_floater(me->x, me->y-30, b, COL(255,214,120)); }
    sfx(11);
    sfx(25);   // feat 15: BOUNTY CLAIMED — the sting beside the till-jingle
    bn_flash_name = G.bn_claim_name;
    bn_flash_t = 3.5f;
    G.bn_done++;
    G.bn_serial++;
    G.bn_cool = BN_COOLDOWN;
    G.bn_claim_pending = 0;
    G.bn_last_reg = -1;   // a new contract: no sighting of THIS one yet
    if (G.autosave_t>0.5f) G.autosave_t = 0.5f;   // persist promptly
}
static void bn_update(float dt){
    if (bn_flash_t>0 && G.state==GS_PLAY) bn_flash_t -= dt;
    if (G.net_mode==NET_CLIENT) return;   // the host owns the bounty (wk_update's rule)
    if (G.state!=GS_PLAY && G.state!=GS_STORE) return;
    if (G.bn_cool>0) G.bn_cool -= dt;
    int claimed = G.bn_claim_pending;
    if (claimed) bn_settle_claim();
    // live tracking for the board and the pause line: where is the
    // elite right now — and if it vanished without a claim, it
    // slipped away (the 1750 px left-behind cull), so remember the
    // region it was last seen in. The contract itself never moves.
    int li = bn_any_live();
    if (li>=0){
        Bug *b=&G.bugs[li];
        float d = fsqrt(b->x*b->x + b->y*b->y);
        G.bn_live = 1; G.bn_live_reg = region_at(d);
    } else {
        if (G.bn_live && !claimed) G.bn_last_reg = G.bn_live_reg;
        G.bn_live = 0;
    }
}
// the board's row model (Mall station 7): one helper feeds the row
// count, the names, the right-hand values and the blurbs, so the
// panel can never disagree with itself (the Trophy Case rule).
enum { BN_HDR_WANTED, BN_BOUNTY, BN_SEEN, BN_HDR_LEDGER, BN_CLAIMED };
static int bn_row_ids(int *ids){
    int n=0;
    ids[n++]=BN_HDR_WANTED;
    ids[n++]=BN_BOUNTY;
    if (G.net_mode==NET_CLIENT) return n;   // a guest reads the poster; the host keeps the book
    ids[n++]=BN_SEEN;
    ids[n++]=BN_HDR_LEDGER;
    ids[n++]=BN_CLAIMED;
    return n;
}
static int bn_row_id(int row){
    int ids[8]; int n=bn_row_ids(ids);
    return (row>=0 && row<n)? ids[row] : -1;
}
static int bn_rows(void){ int ids[8]; return bn_row_ids(ids); }
static const char *bn_row_name(int row, char *buf){
    switch (bn_row_id(row)){
        case BN_HDR_WANTED: return "WANTED";
        case BN_BOUNTY: return ELITE_NAME[bn_name_idx(G.bn_serial)];
        case BN_SEEN: return "LAST SEEN";
        case BN_HDR_LEDGER: return "THE LEDGER";
        case BN_CLAIMED: return "BOUNTIES CLAIMED";
        default: return "";
    }
    (void)buf;
}
static const char *bn_row_right(int row, char *buf){
    char *q=buf;
    switch (bn_row_id(row)){
        case BN_BOUNTY:
            if (G.net_mode==NET_CLIENT) return "HOST'S DESK";
            return G.bn_live? "LOOSE!" : "AT LARGE";
        case BN_SEEN:
            if (G.bn_live){ q=sappend(q, REGION_NAME[G.bn_live_reg]); *q=0; return buf; }
            if (G.bn_last_reg>=0){ q=sappend(q, REGION_NAME[G.bn_last_reg]); *q=0; return buf; }
            return "NOWHERE YET";
        case BN_CLAIMED: q=sapp_int(q, G.bn_done); *q=0; return buf;
        default: return "";
    }
}
static char bn_blurb_buf[192];
static const char *bn_row_blurb(int row){
    char *q;
    switch (bn_row_id(row)){
        case BN_HDR_WANTED: return "ONE NAME AT A TIME. IT CRAWLS OUT WHEN IT PLEASES - MORE OFTEN FAR FROM HOME.";
        case BN_BOUNTY: {
            if (G.net_mode==NET_CLIENT) return "THE HOST KEEPS THE BOUNTIES. THE ELITE, THOUGH, IS REAL - WATCH FOR ITS NAME OVERHEAD.";
            int ni = bn_name_idx(G.bn_serial);
            q=bn_blurb_buf;
            q=sappend(q, ELITE_TITLE[ni]);
            q=sappend(q, ". A "); q=sappend(q, ELITE_KINDNAME[bn_kind_of(G.bn_serial)]);
            q=sappend(q, " THREE TIMES TOO BIG FOR ITS BOOTS. BRING IT DOWN: ITS WEIGHT AGAIN IN THE BANK FOR THE KILLER, ");
            q=sapp_int(q, BN_XP); q=sappend(q, " XP FOR EVERY BUDDY.");
            *q=0; return bn_blurb_buf; }
        case BN_SEEN:
            if (G.bn_live) return "IT IS OUT THERE NOW - FOLLOW THE AMBER ARROW.";
            if (G.bn_last_reg>=0) return "IT SLIPPED AWAY. THE CONTRACT STANDS - IT WILL CRAWL OUT AGAIN.";
            return "NO SIGHTING YET. THE BOARD ONLY KNOWS THE NAME.";
        case BN_HDR_LEDGER: return "THE BOOK OF NAMES. IT ONLY EVER COUNTS THE FALLEN.";
        case BN_CLAIMED: return "EVERY NAMED ONE BROUGHT DOWN, COUNTED HERE. THE NEXT NAME IS ALREADY CHALKED UP.";
        default: return "";
    }
}
