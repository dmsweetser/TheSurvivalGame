// THE SURVIVAL GAME — data/hat_tiers.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 754-774 in the Nov 2026 fine-grained refactor; code unchanged.

static const char *hat_tier_name(int n, float kg){
    if (n<=0)        return "BALD";
    if (n==1)        return "SINGLE ROACH";
    if (n<=3)        return "BUG CAP";
    if (n<=6)        return "BUG BEANIE";
    if (n<=11)       return "STACK OVERFLOW";
    if (n<=18)       return "TOWER OF TECH DEBT";
    if (kg<160.0f)   return "CROWN OF CARCASSES";
    return "THE LEGACY SYSTEM";
}
// Espresso Legs. Nov 9 rebalance (Daniel): "cap the unburdened
// speed upgrade max so I'm not zooming everywhere — it should
// continue to have a benefit burdened, and make the burdened
// benefit more substantial per level." So: the bare-speed bonus
// now CAPS at +40% (207 px/s — where a level-10 build already
// cruises) no matter how Endless the levels get, and the
// upgrade's endless value moved into the load curve instead:
// each level raises by 50% (was 35%) the weight the curve
// shrugs off, so every level claws back a bigger share of the
// hat's penalty than it used to, forever. Level 0 is untouched:
// same base, same curve as always.
