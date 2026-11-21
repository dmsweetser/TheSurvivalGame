// THE SURVIVAL GAME — banzai/tiles.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from banzai.c lines 1-51 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// BANZAI WORLD (Daniel, Nov 3): through the torii gate in the mall's
// south-east corner, you ARE your Banzai Buddy. A procedurally generated
// open world of grass, rocks and trees — generated from the buddy's own
// seed, so it is the same world every visit, and the buddy's saved
// position puts it exactly where it left off. Hackers roam it; walking
// into one starts a turn-based battle (choose an attack, attacks cost
// energy, the hacker hits back). Winning earns buddy XP. Sleeping at a
// tent restores HP and energy in full. Fainting wakes you in the mall.
// Solo trip: no split-screen and no LAN guests in there (v1).
// ---------------------------------------------------------------------------
static void bw_render(int pi);
static void bw_render_battle(int pi);
static void bw_exit(int pi, int fainted);
#define BW_TILE 48
#define ENT_ROWS 10          // ENTRANCES room height in tiles (was 8)
#define ENT_DOOR_LY 4        // the ENTRANCES doorway row in every room
#define ENT_DOORY (ENT_DOOR_LY*BW_TILE + 24)
#define BW_CELL 264
#define BW_ROAMN 20
#define TANK_W 26            // TANK RESCUE field width, tiles
#define TANK_LEN 110         // TANK RESCUE field length, tiles (ty 0 = the top; you fight north)
#define TANK_MAXEN 90
#define TANK_GATE_TY 8       // the barrier row under the boss arena: shut while the core lives
#define TANK_MID_TY 46       // the blockade barrier row, mid-field: shut while the blockade gun lives
static int bw_tileof(float v){ int t=(int)(v/BW_TILE); if (v<0 && v!=(float)t*BW_TILE) t--; return t; }
static int bw_cellof(float v){ int t=(int)(v/BW_CELL); if (v<0 && v!=(float)t*BW_CELL) t--; return t; }
// regional density in 5x5-tile cells: 0 sparse, 1 normal, 2 lush — so
// the meadow breathes: thick woods, then open ground (Nov 3, Daniel:
// "filling the screen with some sparse areas")
static int bw_lush_at(int tx, int ty, u32 seed){
    int cx = tx>=0? tx/5 : (tx-4)/5, cy = ty>=0? ty/5 : (ty-4)/5;
    int r = (int)(hash2(cx, cy, seed ^ 0xD3A7u) % 100);
    return r < 42? 2 : (r < 78? 1 : 0);
}
static int bw_tile_at(int tx, int ty, u32 seed){   // 0 grass, 1 tree, 2 rock
    if (tx>=-2 && tx<=2 && ty>=-3 && ty<=2) return 0;          // the gate clearing
    int lush = bw_lush_at(tx, ty, seed);
    int rockp = lush==2? 3 : lush==1? 2 : 1;
    int treep = lush==2? 17 : lush==1? 8 : 2;
    int r = (int)(hash2(tx, ty, seed ^ 0xB17u) % 100);
    if (r < rockp) return 2;
    if (r < rockp + treep) return 1;
    return 0;
}
