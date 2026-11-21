// THE SURVIVAL GAME — data/palette.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 706-741 in the Nov 2026 fine-grained refactor; code unchanged.

static const u32 HOODIE[MAXP] = { COL(64,140,240), COL(240,90,90), COL(90,200,110), COL(240,180,60) };
static const u32 BUG_COL[4] = { COL(150,90,200), COL(90,170,80), COL(220,120,50), COL(100,180,220) };
static const u32 BUG_BELLY[4] = { COL(190,150,230), COL(140,210,120), COL(245,170,100), COL(160,220,245) };

// regions by distance from the epicenter (stretched: the good stuff
// is FAR out). The first three bands are the original country, kept
// exactly as they were; beyond the Legacy Lands the world keeps
// changing its mind — scorched kernel badlands, a frozen cache field,
// neon RAM flats, an overgrown cable jungle, and at the end of the
// world the Depths, pushed far out to where they belong (Daniel,
// Nov 9: "add more regions as you go further from the epicenter").
// Everything regional is a pure function of distance, so the ground,
// the prop tables and the bug strains all agree with each other —
// and with the host in co-op — with nothing stored anywhere.
#define NREGIONS 8
static const char *REGION_NAME[NREGIONS] = {
    "CLEAN CODE MEADOWS", "SPAGHETTI FIELDS", "LEGACY LANDS",
    "KERNEL BADLANDS", "FROZEN CACHE FIELDS", "NEON RAM FLATS",
    "CABLE JUNGLE", "THE UNMAINTAINED DEPTHS"
};
static const float REGION_EDGE[NREGIONS-1] = { 500, 1400, 2600, 4200, 6200, 8800, 12000 };
static const u32 REGION_GA[NREGIONS] = {
    COL(116,184,96),  COL(150,164,84),  COL(128,116,92),  COL(112,70,48),
    COL(176,204,220), COL(56,50,104),   COL(56,102,60),   COL(88,80,104)
};
static const u32 REGION_GB[NREGIONS] = {
    COL(108,176,90),  COL(142,156,78),  COL(120,108,86),  COL(102,62,42),
    COL(166,196,212), COL(48,43,94),    COL(48,94,54),    COL(80,72,96)
};
// ground sprinkle tint per region (the little grass-tuft strokes)
static const u32 REGION_TUFT[NREGIONS] = {
    COL(80,140,70),  COL(96,132,74),  COL(120,124,88),  COL(158,102,58),
    COL(214,234,244), COL(110,235,255), COL(74,156,84), COL(140,128,140)
};
// each far region's dominant bug strain (BUG_COL order); -1 = fair mix
static const int REGION_BUG[NREGIONS] = { -1, -1, -1, 2, 3, 0, 1, -1 };
