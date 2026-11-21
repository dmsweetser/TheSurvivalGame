// THE SURVIVAL GAME — world/regions.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 742-753 in the Nov 2026 fine-grained refactor; code unchanged.

static int region_at(float d){
    int r = 0;
    while (r < NREGIONS-1 && d >= REGION_EDGE[r]) r++;
    return r;
}
static const char *zone_name(float d){ return REGION_NAME[region_at(d)]; }
static u32 zone_ground(float d, int checker){
    int r = region_at(d);
    return checker ? REGION_GA[r] : REGION_GB[r];
}

// hat tiers — predefined shapes the stack grows through
