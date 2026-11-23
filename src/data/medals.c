// THE SURVIVAL GAME — data/medals.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 219-232 in the Nov 2026 fine-grained refactor; code unchanged.

static int medal_tier(int best_ds, const int thr[3]){
    if (best_ds<=0) return 0;
    if (best_ds<=thr[0]) return 3;
    if (best_ds<=thr[1]) return 2;
    if (best_ds<=thr[2]) return 1;
    return 0;
}
// CART MEDALS, PODIUM ONLY (v1.27, Daniel: "Cart medals should only
// be for first second and third. Right now you get medals when you
// shouldn't."): a Cart finish earns the time tier CAPPED by its
// placement — 1st can earn up to gold, 2nd up to silver, 3rd up to
// bronze, and 4th or lower earns NOTHING however fast the clock was.
// The time thresholds themselves are untouched. (Super Buddy Run is
// a solo time trial — no field to place in — and keeps medal_tier.)
static int medal_tier_finish(int tds, const int thr[3], int place){
    int cap = (place>=1 && place<=3)? 4-place : 0;
    int t = medal_tier(tds, thr);
    return t<cap? t : cap;
}
static const char *medal_name(int tier){
    return tier>=3? "GOLD" : tier==2? "SILVER" : tier==1? "BRONZE" : "NONE";
}
static u32 medal_col(int tier){
    return tier>=3? COL(250,200,50) : tier==2? COL(205,212,220) : tier==1? COL(198,124,64) : COL(140,140,148);
}

