// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: banzai.c — BANZAI WORLD — the hub and Entrances (bw_modes 0-2).
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]
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
// --- Story Mode: fixed sites on the procedural meadow ----------------------
// Free play never sees any of this. Story buddies get five hand-placed
// places with procedural travel between them (Daniel's Alphaman brief).
// The world WIDENED Nov 3 (Daniel: "Make the game wider"): the farm,
// lake and compound spread far out, and a fifth place — the Overgrown
// Ruins — rose in the far west.
static const float BW_SITE_XY[5][3] = {   // x, y, radius
    { -2600, -1400, 330 },   // 0 Patch Village
    {  3400, -2400, 350 },   // 1 the Ruined Server Farm
    { -1400,  4400, 350 },   // 2 the Frozen Lake of Deprecated Code
    {  5800,  3400, 310 },   // 3 Zed's Compound
    { -5200,  1800, 340 },   // 4 the Overgrown Ruins
};
static const char *BW_SITE_NAME[5] = { "PATCH VILLAGE", "THE RUINED SERVER FARM", "THE FROZEN LAKE", "ZED'S COMPOUND", "THE OVERGROWN RUINS" };
static int bw_site_at_xy(Buddy *b, float x, float y){
    if (b->bw_mode!=1) return -1;
    for (int i=0;i<5;i++){
        float dx=x-BW_SITE_XY[i][0], dy=y-BW_SITE_XY[i][1];
        if (dx*dx+dy*dy < BW_SITE_XY[i][2]*BW_SITE_XY[i][2]) return i;
    }
    return -1;
}
static const char *BW_ITEM_NAME[4] = { "PATCH NOTES", "ENERGY DRINK", "ADRENAL COOKIE", "CTRL-Z" };
static int bw_qstate(Buddy *b, int q){ return (b->story_bits >> (q*2)) & 3; }
static void bw_qset(Buddy *b, int q, int s){ b->story_bits = (b->story_bits & ~(3 << (q*2))) | ((s&3) << (q*2)); }
static int bw_qstate_x(Buddy *b, int q){ if (q<4) return bw_qstate(b,q); return (b->story_bits >> (18+(q-4)*2)) & 3; }
static int bw_qstate2(Buddy *b, int q){ return (b->story_bits2 >> ((q-6)*2)) & 3; }   // quests 6-9
static void bw_qset2(Buddy *b, int q, int s){ int sh=(q-6)*2; b->story_bits2 = (b->story_bits2 & ~(3<<sh)) | ((s&3)<<sh); }
static int bw_qstate_g(Buddy *b, int q){ return q<6? bw_qstate_x(b,q) : bw_qstate2(b,q); }   // any quest, 0-9
static int bw_lakekills(Buddy *b){ return (b->story_bits2>>8)&3; }
static void bw_lakekills_set(Buddy *b, int n){ b->story_bits2 = (b->story_bits2 & ~(3<<8)) | ((n&3)<<8); }
static int bw_signs_read(Buddy *b){ int n=0; for (int i=0;i<6;i++) n += (b->story_bits2>>(11+i))&1; return n; }
static void bw_qset_x(Buddy *b, int q, int s){ if (q<4){ bw_qset(b,q,s); return; } int sh = 18+(q-4)*2; b->story_bits = (b->story_bits & ~(3<<sh)) | ((s&3)<<sh); }
// story bit 22: the Cache Hoarder is down. bit 23: you carry the GATE
// CRANK. bits 24-26: Mop's lost tools, found one by one.
static int bw_has_crank(Buddy *b){ return (b->story_bits>>23)&1; }
static const float BW_TOOL_XY[3][2] = { { 1000, 800 }, { -3600, 400 }, { 2000, 3000 } };
static const char *BW_TOOL_NAME[3] = { "WRENCH", "HAMMER", "BRUSH" };
static int bw_tools_found(Buddy *b){ return ((b->story_bits>>24)&1) + ((b->story_bits>>25)&1) + ((b->story_bits>>26)&1); }
static int bw_tool_near(Buddy *b, float maxd){
    if (b->bw_mode!=1) return -1;
    for (int i=0;i<3;i++){
        if ((b->story_bits>>(24+i))&1) continue;
        float dx=BW_TOOL_XY[i][0]-b->x, dy=BW_TOOL_XY[i][1]-b->y;
        if (dx*dx+dy*dy < maxd*maxd) return i;
    }
    return -1;
}
static int bw_farmkills(Buddy *b){ return (b->story_bits >> 8) & 15; }
static void bw_farmkills_set(Buddy *b, int n){ b->story_bits = (b->story_bits & ~(15 << 8)) | ((n&15) << 8); }
// NPCs: 0 Elder Patchina (village), 1 Mop (village), 2 Glitch (the lake hermit)
static const float BW_NPC_XY[3][2] = { { -2620, -1440 }, { -2540, -1320 }, { -1400, 4340 } };
static const u32 BW_NPC_ROBE[3] = { COL(116,74,158), COL(96,128,92), COL(84,96,118) };
// the farm's terminal puzzle: three colored terminals, one right order,
// and the order is scratched on a rack by the farm's south entrance
static const float BW_TERM_XY[3][2] = { { 3230, -2460 }, { 3570, -2460 }, { 3400, -2210 } };
static const u32 BW_TERM_COL[3] = { COL(255,90,80), COL(110,255,140), COL(110,170,255) };
static const char *BW_TERM_NAME[3] = { "RED", "GREEN", "BLUE" };
static void bw_term_order(Buddy *b, int out[3]){
    static const int PERM[6][3] = { {0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0} };
    const int *p = PERM[(b->bw_seed/7) % 6];
    out[0]=p[0]; out[1]=p[1]; out[2]=p[2];
}
static int bw_puz_prog(Buddy *b){ return (b->story_bits >> 12) & 7; }
static void bw_puz_prog_set(Buddy *b, int n){ b->story_bits = (b->story_bits & ~(7 << 12)) | ((n&7) << 12); }
static int bw_puz_done(Buddy *b){ return (b->story_bits >> 15) & 1; }
static int bw_term_near(Buddy *b, float maxd){
    if (b->bw_mode!=1 || bw_puz_done(b)) return -1;
    for (int i=0;i<3;i++){
        float dx=BW_TERM_XY[i][0]-b->x, dy=BW_TERM_XY[i][1]-b->y;
        if (dx*dx+dy*dy < maxd*maxd) return i;
    }
    return -1;
}
static int bw_sign_near(Buddy *b, float maxd){
    if (b->bw_mode!=1) return 0;
    float dx=3400-b->x, dy=-2100-b->y;
    return dx*dx+dy*dy < maxd*maxd;
}
// Help signs (Nov 4, Daniel: "help signs, so the user can get more
// info about how to complete quests — don't give it all away, but make
// everything clearer"). Wooden signposts stand at the village edge, at
// every ridge pass, and on the south road. Each teaches one thing the
// meadow never explained: facing, ridges, the crank, scarce tents.
// Reading them is also Patchina's errand (quest 9).
static const float BW_HSIGN_XY[6][2] = {
    { -2470, -860 },   // 0 the village edge
    {  1762, -1900 },  // 1 the farm pass
    { -1700,  2082 },  // 2 the lake pass
    { -3762,  1900 },  // 3 the ruins pass
    {  5300,   382 },  // 4 the compound pass
    { -1500,   620 },  // 5 the south road
};
static const char *BW_HSIGN_TXT[6] = {
    "SIGN: HACKERS HUNT WHAT THEY SEE. MIND THEIR FACING.",
    "SIGN: THE PASS. RIDGES STOP EVERYONE - HACKERS TOO.",
    "SIGN: COLD LAKE. GLITCH LIKES COMPANY, RARELY GETS IT.",
    "SIGN: WEST STANDS THE RUINS. SOMETHING SITS ON A CRANK.",
    "SIGN: THE COMPOUND GATE OPENS FOR A CRANK, NOT ASKING.",
    "SIGN: TENTS ARE SCARCE NOW. SLEEP WHERE IT COUNTS.",
};
static int bw_hsign_near(Buddy *b, float maxd){
    if (b->bw_mode!=1) return -1;
    for (int i=0;i<6;i++){
        float dx=BW_HSIGN_XY[i][0]-b->x, dy=BW_HSIGN_XY[i][1]-b->y;
        if (dx*dx+dy*dy < maxd*maxd) return i;
    }
    return -1;
}
// Bitling, the stray process (quest 8): a tiny lost bot drifting in a
// slow lissajous south of Patch Village, perfectly happy about it.
static void bw_bitling_pos(float *x, float *y){
    float t = G.anim_t;
    *x = -1800 + fsin(t*0.10f)*170;
    *y =  700 + fsin(t*0.073f + 1.7f)*130;
}
static int bw_bitling_near(Buddy *b, float maxd){
    if (b->bw_mode!=1) return 0;
    float x, y; bw_bitling_pos(&x, &y);
    float dx=x-b->x, dy=y-b->y;
    return dx*dx+dy*dy < maxd*maxd;
}
static int bw_npc_near(Buddy *b, float maxd){
    if (b->bw_mode!=1) return -1;
    for (int i=0;i<3;i++){
        float dx=BW_NPC_XY[i][0]-b->x, dy=BW_NPC_XY[i][1]-b->y;
        if (dx*dx+dy*dy < maxd*maxd) return i;
    }
    return -1;
}
static int bw_site_solid(Buddy *b, float x, float y){
    if (b->bw_mode!=1) return 0;
    // village huts: three homes around the green
    static const float HUT[3][2] = { { -2700, -1480 }, { -2500, -1480 }, { -2600, -1300 } };
    for (int i=0;i<3;i++) if (x>HUT[i][0]-32 && x<HUT[i][0]+32 && y>HUT[i][1]-14 && y<HUT[i][1]+24) return 1;
    // the farm's rack rows: a grid of dead cabinets (none in the centre clearing)
    for (int gx=-2; gx<=2; gx++) for (int gy=-1; gy<=1; gy++){
        float rx = BW_SITE_XY[1][0] + gx*92, ry = BW_SITE_XY[1][1] + gy*112;
        float cdx = rx-BW_SITE_XY[1][0], cdy = ry-BW_SITE_XY[1][1];
        if (cdx*cdx+cdy*cdy < 105*105) continue;
        if (x>rx-29 && x<rx+29 && y>ry-16 && y<ry+16) return 1;
    }
    // the ring around the inner aisle; its south rack is SEALED until the
    // terminal puzzle is solved, then it is simply gone
    for (int k=0;k<8;k++){
        float a = k*0.7854f;
        float rx = BW_SITE_XY[1][0] + fcos(a)*108, ry = BW_SITE_XY[1][1] + fsin(a)*108;
        if (k==2 && bw_puz_done(b)) continue;
        if (x>rx-24 && x<rx+24 && y>ry-14 && y<ry+14) return 1;
    }
    // terminals and the scratched sign are furniture too
    for (int i=0;i<3;i++){
        float dx=BW_TERM_XY[i][0]-x, dy=BW_TERM_XY[i][1]-y;
        if (dx*dx+dy*dy < 12*12) return 1;
    }
    { float dx=3400-x, dy=-2100-y; if (dx*dx+dy*dy < 12*12) return 1; }
    // Zed's fence: a ring, firewalled — its south arc only gives for
    // someone carrying the GATE CRANK out of the Sunken Cache
    { float dx=x-BW_SITE_XY[3][0], dy=y-BW_SITE_XY[3][1]; float d=fsqrt(dx*dx+dy*dy);
      if (d>BW_SITE_XY[3][2]-14 && d<BW_SITE_XY[3][2]+14){
          float down = dy/d;   // 1.0 at the due-south point of the ring
          if (!(down>0.93f && bw_has_crank(b))) return 1;
      } }
    // the Ruins' standing stones are furniture too
    for (int k=0;k<7;k++){
        float a = k*0.8976f, dx = BW_SITE_XY[4][0] + fcos(a)*120 - x, dy = BW_SITE_XY[4][1] + fsin(a)*120 - y;
        if (dx*dx+dy*dy < 12*12) return 1;
    }
    // people are solid too
    for (int i=0;i<3;i++){
        float dx=BW_NPC_XY[i][0]-x, dy=BW_NPC_XY[i][1]-y;
        if (dx*dx+dy*dy < 13*13) return 1;
    }
    return 0;
}
static int bw_solid_xy(float x, float y, u32 seed){
    return bw_tile_at(bw_tileof(x), bw_tileof(y), seed) != 0;
}
// Story Mode hill ridges (Nov 4, Daniel: "impediments along your path
// so you can't just walk straight there"): long impassable hill lines
// across the direct routes between places, each with ONE pass. They
// block buddies and hackers alike (both move through bw_solid_b).
// { x1,y1, x2,y2, gap_s (distance along the ridge of the pass centre),
//   gap_half } — the pass is the only way through on foot.
static const float BW_RIDGES[4][6] = {
    {  1700,-3600,  1700, -600, 1700, 130 },   // the farm road wall
    { -3600, 2200,   800, 2200, 1900, 130 },   // the lake road wall
    { -3900, -800, -3900, 3200, 2700, 130 },   // the ruins wall
    {  3600,  500,  6600,  500, 1700, 130 },   // the compound wall
};
static float bw_ridge_dist(float x, float y, const float *r, float *along){
    float dx=r[2]-r[0], dy=r[3]-r[1];
    float len2 = dx*dx + dy*dy, len = fsqrt(len2);
    float t = len2>0? ((x-r[0])*dx + (y-r[1])*dy)/len2 : 0;
    if (t<0) t=0; if (t>1) t=1;
    float px = r[0]+dx*t, py = r[1]+dy*t;
    if (along) *along = t*len;
    float ex=x-px, ey=y-py;
    return fsqrt(ex*ex + ey*ey);
}
// dirt roads (Daniel, Nov 6): the story map was one flat green —
// now the routes between the sites are worn into the ground, each
// road bending through the ONE pass in its ridge. Purely visual:
// nothing collides with a road, but your feet can follow one.
static float bw_road_dist(float x, float y){
    static const float SEG[8][4] = {
        { -2600,-1400,  1700,-1900 }, {  1700,-1900,  3400,-2400 },   // village <-> the farm
        { -2600,-1400, -1700, 2200 }, { -1700, 2200, -1400, 4400 },   // village <-> the lake
        { -2600,-1400, -3900, 1900 }, { -3900, 1900, -5200, 1800 },   // village <-> the ruins
        {  3400,-2400,  5300,  500 }, {  5300,  500,  5800, 3400 },   // the farm <-> Zed's compound
    };
    float best = 1e9f;
    for (int i=0;i<8;i++){
        float dx=SEG[i][2]-SEG[i][0], dy=SEG[i][3]-SEG[i][1];
        float len2 = dx*dx + dy*dy;
        float t = len2>0? ((x-SEG[i][0])*dx + (y-SEG[i][1])*dy)/len2 : 0;
        if (t<0) t=0; if (t>1) t=1;
        float px = SEG[i][0]+dx*t - x, py = SEG[i][1]+dy*t - y;
        float d = fsqrt(px*px + py*py);
        if (d<best) best = d;
    }
    return best;
}
static int bw_ridge_solid(float x, float y){
    for (int i=0;i<4;i++){
        float along; float d = bw_ridge_dist(x, y, BW_RIDGES[i], &along);
        if (d >= 24) continue;
        float g = along - BW_RIDGES[i][4];
        if (g<0) g=-g;
        if (g <= BW_RIDGES[i][5]) continue;   // the pass: walkable
        return 1;
    }
    return 0;
}
// inside a ridge's pass (with a little apron): the way through is
// KEPT CLEAR — no tree or rock tile counts there, or a lucky hash
// could plug the only gap in the wall
static int bw_ridge_pass_zone(float x, float y){
    // the pass is a ROAD, not a doorway: the corridor through it —
    // the gap's width, 240 units of approach on both sides — grows
    // no trees and no rocks, in solidity or in drawing. (First cut
    // cleared only the ridge line itself and a tree one tile out
    // still plugged the way.)
    for (int i=0;i<4;i++){
        const float *r = BW_RIDGES[i];
        float dx=r[2]-r[0], dy=r[3]-r[1];
        float len = fsqrt(dx*dx + dy*dy); if (len<=0) continue;
        float ux=dx/len, uy=dy/len;
        float rx=x-r[0], ry=y-r[1];
        float along = rx*ux + ry*uy;
        float perp = rx*(-uy) + ry*ux; if (perp<0) perp=-perp;
        float g = along - r[4]; if (g<0) g=-g;
        if (g <= r[5] + 12 && perp <= 240) return 1;
    }
    return 0;
}
// The world has an edge (Nov 4, Daniel: "a hard boundary around the
// whole world that is ocean, so you can only wander so far past the
// relevant areas"). The bounds cover every site with room to roam;
// the sea beyond them is solid — to buddies and hackers alike. (The
// old fixed minimap window was drawn to exactly this extent.)
#define BW_WX0 (-6200)
#define BW_WY0 (-3200)
#define BW_WX1 ( 6800)
#define BW_WY1 ( 5400)
static int bw_outside_world(float x, float y){ return x<BW_WX0 || x>BW_WX1 || y<BW_WY0 || y>BW_WY1; }
static int bw_solid_b(Buddy *b, float x, float y){
    if (b->bw_mode==2) return ent_solid((int)(b - G.bud), x, y);
    if (b->bw_mode==6) return cas_solid((int)(b - G.bud), x, y);
    if (b->bw_mode==7) return 0;   // SUPER BUDDY CART: grass is the wall
    if (b->bw_mode==8) return 0;   // WAR!: the buddy is parked; the RTS keeps its own ground
    if (bw_outside_world(x, y)) return 1;
    if (b->bw_mode==1 && bw_ridge_pass_zone(x, y)) return bw_site_solid(b, x, y);
    return bw_solid_xy(x, y, (u32)b->bw_seed) || bw_site_solid(b, x, y)
        || (b->bw_mode==1 && bw_ridge_solid(x, y));
}
// one feature per encounter cell: 0 none, 1 tent, 2 hacker
static int bw_cell_feat(int cx, int cy, u32 seed, float *ox, float *oy, int story){
    if (cx>=-1 && cx<=1 && cy>=-1 && cy<=1){ *ox=0; *oy=0; return 0; }   // calm near the gate
    u32 h = hash2(cx, cy, seed ^ 0xCE11u);
    *ox = (float)((h>>8) % (BW_CELL-96)) + 48;
    *oy = (float)((h>>16) % (BW_CELL-96)) + 48;
    if (bw_outside_world(cx*BW_CELL + *ox, cy*BW_CELL + *oy)) return 0;   // no camps at sea (Daniel, Nov 5 — a Story Mode tent rendered in the ocean; roamers and props already had this check, cell features were missed)
    int r = (int)(h % 100);
    if (r < (story? 3 : 9)) return 1;   // Nov 4 (Daniel): fewer camps in Story Mode — 3% of cells, Free Play keeps 9%
    if (r < 40) return 2;
    return 0;
}
static int bw_cell_key(int cx, int cy){ return cx*73856093 ^ cy*19349663; }
// Banzai World foes scale with the BUDDY, not the map (Daniel): level
// is linearly commensurate with the buddy's capability — its damage
// output and its bulk — so a trained buddy keeps meeting real fights
// (and, since winnings pay 4 + 2*lvl XP, keeps being paid for them),
// while a fresh buddy still meets fresh-buddy fights wherever it goes.
static int bw_level_at(Buddy *b, int cell){
    float dl = 0.40f*buddy_dmg(b) - 1.3f + ((float)b->maxhp - 100.0f)/150.0f;
    int l = 1 + (dl>0? (int)dl : 0) + (int)(hash2(cell, 11, 5) % 2);
    return l<1? 1 : l;
}
typedef struct { int cell; float x, y, wt, dir; int lvl; float alert; } BWHack;   // alert: story-mode chase memory (Nov 4)
typedef struct { float x, y, vx, vy; int kind, state; float t; } PlatEn;   // SUPER BUDDY RUN critters: 0 crawl, 1 shell-bug, 2 shell, 3 piranha, 4 fly, 5 shroom, 6 flower
typedef struct { float x, y, aim, ft, lo, hi; int kind, hp, dir; } TankEn;   // TANK RESCUE foes: 0 turret, 1 patrol tank, 2 prison hut, 3 repair pickup, 4 fortress core, 5 blockade gun. Patrol dir: +-1 runs on x, +-2 on y (sign = way), lo/hi bound that axis
typedef struct {
    int active, phase; float t;         // phases: 5 intro, 0 choose, 1 player msg, 2 foe msg, 3 won, 4 lost, 6 fled
    int lvl, cell, sel, natt, site, boss, strikes, zcharge, zph;
    float hhp, hmax, lastdmg;
    int attacks[12];
    char msg[96], msg2[72];
} BwBattle;
// Banzai World is a PER-PLAYER mode now (Nov 3): each local player has
// their own sim — own roamers, own battle, own notes — so in split-screen
// co-op one player can be in Banzai World in their half while the other
// keeps playing the field. LAN play never enters (denied at the gate).
#define WAR_MAXT 160   // WAR! terrain capacity (debris + barrier segments): v1.18 raised it from 72 — worst case is 68 debris + ~81 barrier segments = 149 (defined here, before BwSim; the WAR! block below points back at this)
typedef struct {
    BwBattle bat;
    BWHack roam[BW_ROAMN];
    int defeated[64], defeated_n;
    float grace_t, sleep_t, note_t;
    int slept, log_open, log_sel, buff, site_cur;
    int bw_menu, bw_msel;   // the Start-menu inside Banzai World: 0 off, 1 menu, 2 controls panel
    int from_arcade, arc_cab;   // this trip began at an Arcade cabinet (Nov 10): exits return there
    char note[72];
    // ENTRANCES run state (Nov 5): a Doors-like hotel run inside BW —
    // per-trip only, never saved; the records live on the Buddy (er).
    int ent_room, ent_door_upto, ent_key, ent_bits, ent_sticks, ent_lant;
    int ent_ev, ent_phase, ent_dark, ent_dupe_used, ent_libdone, ent_fuse_mask;
    int ent_sub, ent_door_sub, ent_passes;   // SUBFLOOR: ent_sub=1 while downstairs; its own door counter and pass count
    float ent_t, ent_grace, ent_zx, ent_zy, ent_zdir, ent_zspd;
    float ent_tamb, ent_zt;                  // ambient dread timer; zip-roar retrigger timer
    int ent_zpass;
    float ent_tyx, ent_tyy, ent_tt, ent_twait; int ent_typs;
    float ent_rvx, ent_rvy;
    float ent_ix, ent_iy, ent_istun; int ent_chase;
    float ent_fx[4], ent_fy[4]; int ent_fn; float ent_mt; int ent_mstep;   // chase-room floor fires + the Inspector's scary-music pulse (Nov 6)
    float ent_bbx, ent_bby, ent_bbdir, ent_bbcharge, ent_noise;
    int ent_loot_rooms[96], ent_loot_n;
    int ent_finished;
    // SUPER BUDDY RUN session (mode 3, Nov 5): a side-scrolling
    // platformer. The level is generated into plat_grid on entry;
    // records (levels done) live on the Buddy as pd.
    int plat_lvl, plat_w, plat_score, plat_bits, plat_hearts, plat_power, plat_face, plat_ground, plat_up_prev;
    float plat_vx, plat_vy, plat_t, plat_deadt, plat_cleart, plat_cpx, plat_coyote, plat_jbuf, plat_grace;
    float plat_run_t, plat_skid, plat_land_t, plat_cleary, plat_dust_t; int plat_jumpheld;
    float plat_puff_x[10], plat_puff_y[10], plat_puff_t[10]; int plat_puff_a[10];
    int plat_en_n; PlatEn plat_en[24];
    float plat_fx[8], plat_fy[8], plat_fvx[8], plat_fvy[8], plat_ft[8]; int plat_fa[8];
    unsigned char plat_grid[240*15];
    // regional alignment (Nov 14): level L IS main-game region
    // (L-1)%8 — same name, ground palette, bug strain and difficulty
    // pressure as the country that far from the epicenter. Plus the
    // Mario grammar: stomp chains, 100 bits = an extra heart, and
    // one enterable vault pipe per level hiding a bonus room.
    int plat_region, plat_chain, plat_sub, plat_vault_tx, plat_prevk;
    int plat_savew, plat_saven; float plat_savex, plat_savey, plat_savecpx;
    PlatEn plat_saveen[24];
    unsigned char plat_grid2[240*15];
    // BUDDY CRAFT session (mode 4): the world derives from the buddy's
    // seed; materials, gear, builds and mined-out nodes all persist on
    // the Buddy itself (cm/cn/cg/cw/cd), so only bugs, bolts and other
    // mayflies live here.
    int craft_menu, craft_sel, craft_cat, craft_biome, craft_hitn, craft_bugn, craft_prevk;
    float craft_grace, craft_bt, craft_demolish, craft_swarm_t, craft_cache_t, craft_haste, craft_darkp, craft_beam_t;
    int craft_hk[24], craft_hv[24];
    float craft_bx[14], craft_by[14], craft_bph[14]; int craft_bhp[14], craft_bk[14]; float craft_btr[14];
    float craft_bolt_x[4], craft_bolt_y[4], craft_bolt_vx[4], craft_bolt_vy[4], craft_bolt_t[4]; int craft_bolt_a[4];
    float craft_cache_x, craft_cache_y; int craft_cache_on;
    float craft_beam_x1, craft_beam_y1, craft_beam_x2, craft_beam_y2;
    float craft_tz[48];   // per-build zapper-tower cooldowns (session)
    int craft_nap;        // the current rest is a bedroll nap, not a fire sleep
    int craft_scroll;     // crafting menu: first visible row of the scrolling list
    int craft_search;     // menu search mode: typed text owns the keyboard (soft keyboard up on touch)
    char craft_filter[13];   // the menu's item filter ("" = the full list)
    float craft_sh_t; int craft_shn;   // sharks: they own the water
    float craft_sh_x[6], craft_sh_y[6], craft_sh_ph[6], craft_sh_tr[6], craft_sh_hd[6]; int craft_sh_hp[6];
    float craft_bmr_x, craft_bmr_y, craft_bmr_vx, craft_bmr_vy, craft_bmr_t; int craft_bmr_a;   // the boomerang
    int craft_bolt_pc[4]; float craft_bolt_hc[4];   // storm-zapper pierce + per-bolt hit cooldown
    // TANK RESCUE session (mode 5): per-run only — the field derives
    // from (bw_seed, level) each load and the record (deepest level
    // cleared) lives on the Buddy as td.
    int tank_lvl, tank_hull, tank_lives, tank_pass, tank_score, tank_seed;
    int tank_boss_hp, tank_boss_max, tank_en_n;
    int tank_deliv, tank_quota, tank_mid_hp, tank_mid_max, tank_obj;   // the mission chain: deliveries, the quota, the blockade, the current objective phase
    float tank_obj_x, tank_obj_y;                                      // where the yellow arrow is pointing
    float tank_t, tank_fire_cd, tank_grace, tank_deadt, tank_cleart, tank_respawn_y, tank_fx, tank_fy;
    TankEn tank_en[TANK_MAXEN];
    float tank_cap_x[24], tank_cap_y[24]; int tank_cap_a[24];
    float tank_sh_x[24], tank_sh_y[24], tank_sh_vx[24], tank_sh_vy[24], tank_sh_t[24]; int tank_sh_a[24];
    float tank_es_x[32], tank_es_y[32], tank_es_vx[32], tank_es_vy[32], tank_es_t[32]; int tank_es_a[32];
    int tank_wep, tank_gren;   // 0 cannon, 1 grenades (Nov 14: switchable secondary)
    float tank_gr_x[10], tank_gr_y[10], tank_gr_vx[10], tank_gr_vy[10], tank_gr_t[10]; int tank_gr_a[10];
    float tank_boom_x[6], tank_boom_y[6], tank_boom_t[6];
    unsigned char tank_dead[TANK_W*TANK_LEN], tank_hit[TANK_W*TANK_LEN];   // shot-away tiles + sandbag wear
    // CASTLE (bw_mode 6, Nov 14 2026): the run's whole state
    int cas_room, cas_score, cas_treasures, cas_sword, cas_key, cas_won;
    float cas_hitcd, cas_notecd, cas_wont;
    float cas_mx[16], cas_my[16], cas_mwt[16];
    int cas_mtype[16], cas_mhp[16], cas_mroom[16], cas_malive[16];
    unsigned char cas_taken[20];   // item ids: 0-12 treasures, 13 sword, 14 key, 15-16 tonics
    // SUPER BUDDY CART (bw_mode 7, Nov 14 2026): one race's state.
    // Kart 0 is the player's buddy; 1-5 are AI rivals. Progress is
    // px along the centreline relative to the start line (negative
    // on the grid); places are 1-based, finplace 0 until finished.
    int crt_state;                 // 0 countdown, 1 racing, 2 finished (player home)
    float crt_t, crt_count, crt_fint;
    int crt_fincount, crt_player_place;
    int crt_duo;                 // head-to-head kart: the other player's index, -1 solo
    int crt_guest;               // 0 = not a race guest; else the host's pi + 1 (this sim only ferries input/view)
    int crt_hfin[2];             // human finish places: [0] host kart, [1] guest kart; 0 until home
    float crt_endt;              // after the first human finishes, seconds before the race closes (-1 = not armed)
    int crt_kcol[6];             // kart index -> palette index actually worn this race
    int crt_species[6], crt_dcolidx[6], crt_nameidx[6];   // the DRIVER in each kart: a real Banzai Buddy (species, coat colour, name)
    float crt_line[6], crt_slidet[6];                     // AI racing-line lateral (smoothed); human drift charge time
    float crt_puffx[20], crt_puffy[20], crt_puffl[20];    // exhaust puffs: position + remaining life (ring buffer)
    int crt_puffn, crt_prevplace[2], crt_prevlap[2];      // puff ring cursor; last place/lap per human (overtake + final-lap notes)
    // the GRAND PRIX (Nov 14): a cup is four races, one per region
    // of its half of the world — HOME (regions 0-3) or FAR (4-7).
    // Points bank per race; the final state ranks the whole field.
    int crt_cup, crt_race, crt_track;
    int crt_pts[6], crt_lastpl[6], crt_gprank[6];
    float crt_pufft;                                      // puff spawn timer
    int crt_colsel, crt_colsel2; // waiting-room colour picks (host's own, the guest's)
    int crt_ready;               // waiting-room ready bits: 1 host, 2 guest
    int crt_prevk, crt_gprevk;   // held-key history for waiting-room edge detection
    float crt_x[6], crt_y[6], crt_h[6], crt_v[6], crt_prog[6], crt_boost[6];
    int crt_seg[6], crt_place[6], crt_finplace[6];
    float crt_off[6], crt_skill[6];
    // WAR! (bw_mode 8, Nov 15 2026): a tap-first RTS. The campaign
    // (regions conquered) lives on the Buddy as war_prog (saved as
    // wr); everything here is one battle's scratch state. Units and
    // structures share two arrays across both sides: types 0-2 are
    // the player's units, 3-4 are bugs; struct types 0-3 are the
    // player's buildings, 4 is a bug hive. Dead entries are hp<=0
    // (never compacted — refs stay stable for the whole battle).
    int war_state;               // 0 world map, 1 battle, 2 won, 3 lost
    int war_region, war_bits, war_map_sel;
    u32 war_bseed;               // v1.18: this battle's novelty seed (hives + terrain rolled from it; never saved — structs and terrain ride the snapshot themselves)
    int war_seedforce;           // test-only (dbg_warseed): when nonzero, war_build_battle uses it as war_bseed, so wartest's layout-sensitive checks run on one fixed field; shipped play never sets it
    float war_camx, war_camy, war_t, war_wavet, war_endt, war_grace;
    int war_selstruct, war_place;
    float war_ltap_t; int war_ltap_x, war_ltap_y, war_ltap_unit, war_ltap_n;
    float war_ordx, war_ordy, war_ordt;
    int war_drag_id, war_dragx, war_dragy;
    float war_drag_cx, war_drag_cy;   // camera position when the drag grabbed the world
    // WAR! on a gamepad (v1.16): the pad cursor — a "finger" in world
    // coords that presses through the same tap path as touch/mouse.
    // P1 only. war_barhl is the LB/RB highlight over the selected
    // structure's command bar; war_barhl_s is the struct it belongs
    // to, so any change of selection retires the highlight.
    float war_curx, war_cury; int war_curon;
    int war_barhl, war_barhl_s;
    // v1.20: command-bar CAPTURE (Daniel's design). When a building
    // becomes selected through the pad's own A-tap, the bar takes
    // the pad over: war_barcap is set, the field cursor freezes and
    // hides, and the stick/bumpers walk war_barhl instead — moving
    // the stick no longer retires the highlight. war_barhl_row says
    // which row the highlight walks: 0 = the war_bar_list row (the
    // open category's options, or the top level itself), 1 = the
    // category row in the bar. war_capnx/war_capny hold the stick's
    // previous-frame signs, for edge detection while captured.
    int war_barcap, war_barhl_row, war_capnx, war_capny;
    // v1.17: the battle pause menu (Esc in a battle) and the
    // categorized command bar. war_bcat: -1 top level (categories),
    // 0 TRAIN, 1 BUILD, 2 DEFENSE. war_pause freezes the battle.
    int war_pause, war_bcat;
    // v1.19: SKIRMISH — ad-hoc WAR! battles off the world map.
    // war_skirmish marks the live battle as ad-hoc (never parked,
    // never touches war_prog); war_aw/war_ah are the LIVE field
    // size (campaign = WAR_AW/WAR_AH; the #defines stay the array
    // maxima); war_gw/war_gh are the fog/A* grid dims in 40u cells
    // (a SMALL field's grids are subgrids of the same arrays).
    // war_size/war_diff are the player's setup-panel picks, kept
    // per player, never saved: size 1 SMALL / 2 STANDARD / 3
    // LARGE, diff 1 EASY / 2 NORMAL / 3 HARD (0 = unset, the map
    // entry defaults it to STANDARD/NORMAL). war_lines overrides
    // the barrier-line count when >0 (campaign leaves it 0 and
    // keeps its region rule).
    int war_skirmish, war_size, war_diff, war_setup_sel, war_setup_prevk;
    float war_aw, war_ah;
    int war_gw, war_gh, war_lines;
    char war_msg[72]; float war_msgt;
    float war_ux[80], war_uy[80], war_uhp[80], war_uatk[80];
    float war_utx[80], war_uty[80], war_uharv[80];
    // v1.19 addendum: the PATCHER's repair state — TRANSIENT, like
    // an attack target: it is never written into the battle
    // snapshot, so a restored patcher simply idles (and its
    // autoseek re-engages from there). war_urepair is a structure
    // index + 1 (0 = no repair work), war_urepacc the fractional
    // bit accumulator (repairs bill 1 bit per 4 HP; only whole
    // bits leave the purse), war_urepscan the autoseek re-scan
    // timer. war_repnoted latches the OUT OF BITS note to once per
    // emptying of the purse.
    int war_urepair[80]; float war_urepacc[80], war_urepscan[80];
    int war_repnoted;
    int war_utype[80], war_umaxhp[80], war_ustate[80], war_utarget[80], war_ucarry[80], war_usel[80];
    float war_sx[28], war_sy[28], war_shp[28], war_scd[28], war_qt[28];
    int war_stype[28], war_sside[28], war_smaxhp[28];
    // v1.17 addendum 2: production is a QUEUE of up to 9 per building,
    // C&C-style — paid in full when queued, mixed types allowed, the
    // front item trains on war_qt and the rest roll out in order.
    int war_queue[28][9]; int war_qlen[28];
    // v1.17 addendum 3: rally points — where a producer's fresh units
    // walk to the moment they roll out. 0 = unset (arena coords are
    // ~40000+, so 0 is never a real point).
    float war_rallyx[28], war_rallyy[28];
    float war_nx[16], war_ny[16]; int war_namt[16], war_nn;   // v1.19: 11 campaign nodes now, 13 on a LARGE skirmish field (was [10])
    float war_px[64], war_py[64]; int war_ptar[64], war_pdmg[64], war_pside[64]; float war_pt[64];
    // WAR! round 2 (Nov 16 2026, Daniel's playtest): static terrain and
    // the fog of war. Terrain kinds: 0 tree, 1 rock, 2 broken terminal,
    // 3 cable spool, 4 rubble. Fog cells are 40u: 0 unexplored,
    // 1 explored (permanent for the battle), 2 currently visible.
    float war_tx[WAR_MAXT], war_ty[WAR_MAXT]; int war_ttype[WAR_MAXT], war_trad[WAR_MAXT], war_tn;
    // WAR! round 3 (Nov 16, Daniel's playtest): debris has hit points —
    // your combat units can shoot, smash and roll over it. war_tdeadt is
    // the fading scorch mark a destroyed feature leaves behind.
    // v1.18: kind 5 is a BARRIER segment — indestructible (see WAR_THP).
    float war_thp[WAR_MAXT]; int war_tmaxhp[WAR_MAXT]; float war_tdeadt[WAR_MAXT];
    unsigned char war_fog[40*30];
    // v1.17 pathfinding: per-unit waypoint runs (turning points of a
    // grid A* path, LOS-shortcut). war_ptg is the target the path was
    // computed for; if the caller's target drifts >24u from it the
    // path is dropped. Stall bookkeeping lives here too: a unit that
    // tries to step but barely moves for 0.6s earns a path request.
    float war_wpx[80][32], war_wpy[80][32];
    unsigned char war_wpn[80], war_wpi[80];
    float war_ptgx[80], war_ptgy[80];
    float war_stallt[80], war_repath[80], war_stallx[80], war_stally[80];
} BwSim;
static BwSim BWS[MAXP];
static int bw_defeated_has(BwSim *S, int key){ for (int i=0;i<S->defeated_n;i++) if (S->defeated[i]==key) return 1; return 0; }
static int bw_tent_near(Buddy *b, float maxd, float *tx, float *ty){
    u32 seed=(u32)b->bw_seed;
    int cx0=bw_cellof(b->x)-1, cy0=bw_cellof(b->y)-1;
    for (int cx=cx0; cx<=cx0+2; cx++) for (int cy=cy0; cy<=cy0+2; cy++){
        float ox, oy; if (bw_cell_feat(cx, cy, seed, &ox, &oy, b->bw_mode==1)!=1) continue;
        float wx=cx*BW_CELL+ox, wy=cy*BW_CELL+oy;
        float dx=wx-b->x, dy=wy-b->y;
        if (dx*dx+dy*dy < maxd*maxd){ *tx=wx; *ty=wy; return 1; }
    }
    return 0;
}
static int bw_feat_solid(Buddy *b, float x, float y){
    if (b->bw_mode==2) return 0;
    float tx, ty; Buddy tmp=*b; tmp.x=x; tmp.y=y;
    return bw_tent_near(&tmp, 20, &tx, &ty);
}
// --- the battle ------------------------------------------------------------
static const char *BW_HACK_NAME[6] = { "SCRIPT KIDDIE", "PHISHER", "BOT HERDER", "SPAM LORD", "RANSOMWARE RANDY", "ZERO-DAY ZED" };
static const char *BW_HACK_MOVE[6] = { "BRUTE FORCE", "PHISH SPAM", "DDOS FLOOD", "ZERO-DAY", "RANSOMWARE", "STACK SMASH" };
static const char *bw_hacker_name(int lvl){
    int i = lvl<=1? 0 : lvl<=3? 1 : lvl<=5? 2 : lvl<=6? 3 : lvl<=8? 4 : 5;
    return BW_HACK_NAME[i];
}
static void bw_attack_info(Buddy *b, int mv, const char **nm, float *cost, float *mult){
    switch (mv){
        case 1: *nm=BUD_MOVE2[b->species%BUDN]; *cost=BUD_EN_MOVE2; *mult=1.6f; break;
        case 2: *nm=BUD_SPEC[b->species%BUDN];  *cost=BUD_EN_SPEC;  *mult=2.3f; break;
        case 3: *nm=BUD_MOVE3[b->species%BUDN]; *cost=BUD_EN_MOVE3; *mult=2.2f; break;
        case 4: *nm=BUD_MOVE4[b->species%BUDN]; *cost=BUD_EN_MOVE4; *mult=2.9f; break;
        case 5: *nm="RUN AWAY"; *cost=0; *mult=0; break;
        case 6: case 7: case 8: case 9: *nm=BW_ITEM_NAME[mv-6]; *cost=0; *mult=0; break;
        default: *nm=BUD_BASIC[b->species%BUDN]; *cost=0; *mult=1.0f; break;
    }
}
static void bw_battle_start(Buddy *b, BWHack *h){
    BwSim *S = &BWS[(int)(b - G.bud)];
    S->bat.active=1; S->bat.phase=5; S->bat.t=1.0f; S->bat.lvl=h->lvl; S->bat.cell=h->cell; S->bat.sel=0;
    S->bat.boss=0; S->bat.strikes=0; S->bat.zcharge=0; S->bat.zph=0; S->bat.lastdmg=0;
    S->bat.site = bw_site_at_xy(b, b->x, b->y);
    S->bat.hmax = 26 + 20*h->lvl + (int)(hash2(h->cell, 7, 3) % 10);
    S->bat.hhp = S->bat.hmax;
    S->bat.natt=0; S->bat.attacks[S->bat.natt++]=0;
    if (b->skills&1) S->bat.attacks[S->bat.natt++]=1;
    if (b->skills&2) S->bat.attacks[S->bat.natt++]=2;
    if (b->skills&4) S->bat.attacks[S->bat.natt++]=3;
    if (b->skills&8) S->bat.attacks[S->bat.natt++]=4;
    for (int it=0; it<4; it++) if (b->items[it]>0) S->bat.attacks[S->bat.natt++]=6+it;
    S->bat.attacks[S->bat.natt++]=5;
    { char *q=S->bat.msg; q=sappend(q, "A WILD "); q=sappend(q, bw_hacker_name(S->bat.lvl));
      q=sappend(q, " APPEARED!"); *q=0; scpy(S->bat.msg2, "IT WANTS YOUR PASSWORD.", 72); }
    sfx(7);
}
static void bw_boss_start(int pi, int boss){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    S->bat.active=1; S->bat.phase=5; S->bat.t=1.2f; S->bat.sel=0; S->bat.cell=0;
    S->bat.boss=boss; S->bat.strikes=0; S->bat.zcharge=0; S->bat.zph=0; S->bat.lastdmg=0;
    S->bat.site = bw_site_at_xy(b, b->x, b->y);
    if (boss==1){
        S->bat.lvl = 7; S->bat.hmax = 330;
        scpy(S->bat.msg, "BROKEN RAID ROLLS OUT OF THE DARK!", 96);
        scpy(S->bat.msg2, "EVERY BAD SECTOR YOU EVER MET. IN A TRENCHCOAT.", 72);
    } else if (boss==3){
        S->bat.lvl = 9; S->bat.hmax = 430;
        scpy(S->bat.msg, "THE CACHE HOARDER WAKES ON ITS HOARD!", 96);
        scpy(S->bat.msg2, "IT HAS BEEN SITTING ON THE GATE CRANK FOR YEARS.", 72);
    } else {
        S->bat.lvl = 12; S->bat.hmax = 560;
        scpy(S->bat.msg, "ZERO-DAY ZED. UNPATCHED. UNBOTHERED.", 96);
        scpy(S->bat.msg2, "THE FINAL BUG. PROBABLY.", 72);
    }
    S->bat.hhp = S->bat.hmax;
    S->bat.natt=0; S->bat.attacks[S->bat.natt++]=0;
    if (b->skills&1) S->bat.attacks[S->bat.natt++]=1;
    if (b->skills&2) S->bat.attacks[S->bat.natt++]=2;
    if (b->skills&4) S->bat.attacks[S->bat.natt++]=3;
    if (b->skills&8) S->bat.attacks[S->bat.natt++]=4;
    for (int it=0; it<4; it++) if (b->items[it]>0) S->bat.attacks[S->bat.natt++]=6+it;
    S->bat.attacks[S->bat.natt++]=5;
    sfx(7);
}
static void bw_battle_end(int pi, int won){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    if (won && S->bat.boss==0 && S->defeated_n<64) S->defeated[S->defeated_n++] = S->bat.cell;
    S->bat.active=0; S->grace_t = won? 1.2f : 2.5f;
    if (won){
        int noted = 0;
        if (S->bat.boss==1){
            b->story_bits |= (1<<16); b->xp += 60;
            b->items[2] += 2; if (b->items[2]>9) b->items[2]=9;
            scpy(S->note, "BROKEN RAID SCRAPPED. +60 XP. THE AISLE GOES QUIET.", 72); S->note_t=4.2f; noted=1;
        } else if (S->bat.boss==3){
            b->story_bits |= (1<<22) | (1<<23);   // hoarder down, crank carried
            if (bw_qstate_x(b,5)==1) bw_qset_x(b,5,2);
            b->xp += 100;
            for (int it=0; it<4; it++){ b->items[it] += 2; if (b->items[it]>9) b->items[it]=9; }
            scpy(S->note, "THE HOARDER CRACKS OPEN. THE GATE CRANK IS YOURS.", 72); S->note_t=5.0f; noted=1;
        } else if (S->bat.boss==2){
            b->story_bits |= (1<<17);
            if (bw_qstate(b,3)==1) bw_qset(b,3,2);
            b->xp += 150;
            for (int it=0; it<4; it++){ b->items[it] += 2; if (b->items[it]>9) b->items[it]=9; }
            scpy(S->note, "ZERO-DAY ZED HAS BEEN FUNERALED. THE MEADOW IS PATCHED.", 72); S->note_t=5.0f; noted=1;
        }
        // story: clearing hackers AT the farm counts toward the quest
        if (b->bw_mode==1 && S->bat.site==1 && bw_qstate(b, 1)==1){
            int k = bw_farmkills(b) + 1; bw_farmkills_set(b, k);
            if (k>=3){
                bw_qset(b, 1, 2); b->xp += 30;
                scpy(S->note, "QUEST DONE: CLEAR THE SERVER FARM. REPORT TO PATCHINA.", 72); S->note_t = 4.5f; noted = 1;
            } else {
                char *q=S->note; q=sappend(q, "SERVER FARM CLEARED: "); q=sapp_int(q, k); q=sappend(q, "/3"); *q=0;
                S->note_t = 2.2f; noted = 1;
            }
        }
        // story: clearing hackers ON the frozen lake counts for Glitch
        if (b->bw_mode==1 && S->bat.site==2 && S->bat.boss==0 && bw_qstate2(b,6)==1){
            int k = bw_lakekills(b) + 1; if (k>2) k=2; bw_lakekills_set(b, k);
            if (k>=2){ scpy(S->note, "LAKE CLEARED: 2/2. GLITCH WILL WANT WORD.", 72); S->note_t = 3.6f; noted = 1; }
            else { char *q=S->note; q=sappend(q, "LAKE CLEARED: "); q=sapp_int(q, k); q=sappend(q, "/2"); *q=0; S->note_t = 2.2f; noted = 1; }
        }
        // hackers sometimes drop something useful (a BW-only consumable)
        if (rng_f()<0.25f){
            int it = (int)(rng_f()*3.99f); if (it>3) it=3;
            if (b->items[it]<9){
                b->items[it]++;
                if (!noted){ char *q=S->note; q=sappend(q, "THE HACKER DROPPED: "); q=sappend(q, BW_ITEM_NAME[it]); q=sappend(q, "!"); *q=0; S->note_t = 2.6f; }
            }
        }
    }
    if (b->hp<=0){ b->hp=0; b->fainted=1; bw_exit(pi, 1); }
}
static const char *bw_foe_name(BwSim *S){
    return S->bat.boss==1? "BROKEN RAID" : S->bat.boss==2? "ZERO-DAY ZED" : S->bat.boss==3? "CACHE HOARDER" : bw_hacker_name(S->bat.lvl);
}
static void bw_foe_strike(int pi){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    const char *mv = BW_HACK_MOVE[(S->bat.lvl + (int)(G.anim_t*7)) % 6];
    float f = (1.0f - 0.12f*b->up_def) * BUD_DEF[b->species%BUDN];
    S->bat.strikes++;
    if (S->bat.boss==1 && S->bat.strikes%3==0){
        float dmg = S->bat.lastdmg * 0.5f * f; if (dmg<3) dmg=3;
        b->hp -= dmg; if (b->hp<0) b->hp=0;
        scpy(S->bat.msg, "BROKEN RAID MIRRORS YOUR LAST MOVE!", 96);
        { char *q2=S->bat.msg2; q2=sappend(q2, BUD_ELF[b->name_idx%BUD_NAMES_N]); q2=sappend(q2, " TOOK ");
          q2=sapp_int(q2, (int)(dmg+0.5f)); q2=sappend(q2, " DAMAGE. RUDE."); *q2=0; }
        sfx(4); return;
    }
    if (S->bat.boss==3 && S->bat.strikes%4==0){
        // the Hoarder's trick: it HOARDS the blow — no damage, and it
        // feels better for it. Hit it three times between its hoards.
        S->bat.hhp += 30; if (S->bat.hhp > S->bat.hmax) S->bat.hhp = S->bat.hmax;
        scpy(S->bat.msg, "THE HOARDER HOARDS YOUR HIT. IT FEELS BETTER.", 96);
        scpy(S->bat.msg2, "SOMEWHERE, A CACHE SINGS.", 72);
        sfx(11); return;
    }
    if (S->bat.boss==2 && S->bat.hhp <= S->bat.hmax*0.34f && !S->bat.zcharge){
        S->bat.zcharge = 1;
        scpy(S->bat.msg, "ZERO-DAY ZED IS CHARGING SOMETHING AWFUL...", 96);
        scpy(S->bat.msg2, "BRACE. OR RUN. BOTH ARE VALID.", 72);
        sfx(16); return;
    }
    float mult2 = 1.0f;
    if (S->bat.boss==2){
        if (S->bat.zcharge){ mult2 = 2.5f; S->bat.zcharge = 0; }
        else if (S->bat.hhp <= S->bat.hmax*0.67f) mult2 = 2.0f;
    }
    float dmg = (4.0f + 3.2f*S->bat.lvl) * rng_range(0.85f, 1.15f) * f * mult2;
    b->hp -= dmg; if (b->hp<0) b->hp=0;
    { char *q=S->bat.msg; q=sappend(q, bw_foe_name(S)); q=sappend(q, " USED "); q=sappend(q, mv); q=sappend(q, "!"); *q=0;
      char *q2=S->bat.msg2; q2=sappend(q2, BUD_ELF[b->name_idx%BUD_NAMES_N]); q2=sappend(q2, " TOOK ");
      q2=sapp_int(q2, (int)(dmg+0.5f)); q2=sappend(q2, " DAMAGE."); *q2=0; }
    if (S->bat.boss==2){
        int ph = S->bat.hhp > S->bat.hmax*0.67f? 0 : S->bat.hhp > S->bat.hmax*0.34f? 1 : 2;
        if (ph > S->bat.zph){ S->bat.zph = ph;
            scpy(S->bat.msg2, ph==1? "ZED: FINE. BOTH HANDS, THEN." : "ZED: ZERO DAY. TOMORROW IS CANCELLED.", 72); }
    }
    sfx(4);
}
// execute the highlighted attack (shared by the input handler and tests)
static void bw_battle_pick(int pi){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    int mv = S->bat.attacks[S->bat.sel];
    const char *nm; float cost, mult; bw_attack_info(b, mv, &nm, &cost, &mult);
    if (mv>=6){   // a consumable: it spends the turn (Ctrl-Z spends the battle)
        int it = mv-6;
        if (b->items[it]<=0) return;
        b->items[it]--;
        if (mv==9){ scpy(S->bat.msg, "CTRL-Z! THE LAST TEN SECONDS UNDO THEMSELVES.", 96); scpy(S->bat.msg2, "", 72); S->bat.phase=6; S->bat.t=1.0f; sfx(11); return; }
        if (mv==6){ b->hp += b->maxhp*0.4f; if (b->hp>b->maxhp) b->hp=b->maxhp;
            char *q=S->bat.msg; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " USED PATCH NOTES."); *q=0;
            char *q2=S->bat.msg2; q2=sappend(q2, "BACK TO "); q2=sapp_int(q2, (int)b->hp); q2=sappend(q2, " HP."); *q2=0; }
        else if (mv==7){ b->energy += 50; if (b->energy>BUD_ENERGY_MAX) b->energy=BUD_ENERGY_MAX;
            char *q=S->bat.msg; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " GLUGGED AN ENERGY DRINK."); *q=0;
            scpy(S->bat.msg2, "THE WORLD VIBRATES SLIGHTLY FASTER.", 72); }
        else { S->buff=1;
            char *q=S->bat.msg; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " ATE THE ADRENAL COOKIE."); *q=0;
            scpy(S->bat.msg2, "NEXT HIT LANDS DOUBLE.", 72); }
        S->bat.phase=1; S->bat.t=1.15f; sfx(11);
        return;
    }
    if (mv==5){
        if (rng_f() < 0.75f){ scpy(S->bat.msg, "GOT AWAY CLEAN!", 96); scpy(S->bat.msg2, "", 72); S->bat.phase=6; S->bat.t=1.0f; sfx(11); }
        else { scpy(S->bat.msg, "COULDN'T GET AWAY!", 96); scpy(S->bat.msg2, "", 72); bw_foe_strike(pi); S->bat.phase=2; S->bat.t=1.2f; }
        return;
    }
    if (b->energy < cost){ scpy(S->bat.msg, "NOT ENOUGH ENERGY!", 96); scpy(S->bat.msg2, "SLEEP AT A TENT TO REFILL.", 72); sfx(16); return; }
    b->energy -= cost;
    if (S->buff){ mult *= 2.0f; S->buff=0; }   // the Adrenal Cookie, cashing in
    float dmg = buddy_dmg(b) * 3.2f * mult * rng_range(0.9f, 1.1f);
    S->bat.lastdmg = dmg;
    S->bat.hhp -= dmg; if (S->bat.hhp<0) S->bat.hhp=0;
    b->lunge_t = 0.22f;
    { char *q=S->bat.msg; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " USED "); q=sappend(q, nm); q=sappend(q, "!"); *q=0;
      char *q2=S->bat.msg2; q2=sappend(q2, "IT TOOK "); q2=sapp_int(q2, (int)(dmg+0.5f)); q2=sappend(q2, " DAMAGE."); *q2=0; }
    S->bat.phase=1; S->bat.t=1.15f; sfx(3);
}
// touch parity for the battle menu (Daniel, Nov 4: on mobile there
// was no way to tap an attack — the menu only listened to keys/pads).
// Tap a move in the grid to use it. Geometry mirrors bw_battle_draw.
static void bw_battle_tap(int pi, int vx, int vy, int vw2, int vh2){
    BwSim *S = &BWS[pi];
    if (!S->bat.active || S->bat.phase!=0) return;
    int rows = (S->bat.natt+1)/2, bh = 84 + (rows>3? (rows-3)*15 : 0), rstep = rows>3? 15 : 18;
    int top = vy+vh2-bh+22;                        // first row's text y
    if (IN.tap_y < top-6 || IN.tap_y >= top-6 + rows*rstep) return;
    if (IN.tap_x < vx+10) return;
    int r = (IN.tap_y - (top-6)) / rstep;
    int c = (IN.tap_x - (vx+16)) / (vw2/2-20); if (c<0) c=0; if (c>1) c=1;
    int idx = r*2 + c;
    if (idx<0 || idx>=S->bat.natt) return;
    S->bat.sel = idx; IN.tapped = 0; bw_battle_pick(pi);
}
static void bw_battle_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    if (S->bat.phase==0){
        if (in->up){ S->bat.sel = (S->bat.sel + S->bat.natt - 1) % S->bat.natt; sfx(6); }
        if (in->down){ S->bat.sel = (S->bat.sel + 1) % S->bat.natt; sfx(6); }
        if (in->sel) bw_battle_pick(pi);
        return;
    }
    S->bat.t -= dt;
    if (S->bat.t>0) return;
    if (S->bat.phase==5){ S->bat.phase=0; }
    else if (S->bat.phase==1){
        if (S->bat.hhp<=0){
            int xp = 4 + 2*S->bat.lvl; b->xp += xp;   // calibrated Nov 3 (Daniel: BW paid far too much, too easily)
            { char *q=S->bat.msg; q=sappend(q, bw_foe_name(S)); q=sappend(q, " CRASHED!"); *q=0;
              char *q2=S->bat.msg2; q2=sappend(q2, BUD_ELF[b->name_idx%BUD_NAMES_N]); q2=sappend(q2, " GAINED ");
              q2=sapp_int(q2, xp); q2=sappend(q2, " XP."); *q2=0; }
            S->bat.phase=3; S->bat.t=1.6f; sfx(11);
        } else { bw_foe_strike(pi); S->bat.phase=2; S->bat.t=1.2f; }
    }
    else if (S->bat.phase==2){
        if (b->hp<=0){ scpy(S->bat.msg, "YOUR BUDDY FAINTED!", 96); scpy(S->bat.msg2, "EVERYTHING GOES DARK...", 72); S->bat.phase=4; S->bat.t=1.8f; sfx(16); }
        else S->bat.phase=0;
    }
    else if (S->bat.phase==3){ bw_battle_end(pi, 1); }
    else if (S->bat.phase==4){ bw_battle_end(pi, 0); }
    else if (S->bat.phase==6){ bw_battle_end(pi, 0); }
}
// --- enter / exit ------------------------------------------------------------
static void bw_gate_try(int pi){
    Player *p=&G.p[pi]; Buddy *b=&G.bud[pi];
    if (G.net_mode==NET_CLIENT) return;
    // split-screen co-op is welcome (you play in your own half); a LAN
    // session with guests connected is not — their screens have no BW
    if (G.net_mode==NET_HOST && any_remote_connected()){ spawn_floater(p->x, p->y-24, "BANZAI WORLD IS A SOLO TRIP", COL(255,200,150)); sfx(16); return; }
    if (!b->active){ spawn_floater(p->x, p->y-24, "YOU NEED A BANZAI BUDDY FIRST", COL(255,150,150)); sfx(16); return; }
    if (b->fainted){ spawn_floater(p->x, p->y-24, "YOUR BUDDY FAINTED - REVIVE IT FIRST", COL(255,150,150)); sfx(16); return; }
    bwgate[pi]=1; bwgate_sel[pi]=0; sfx(19);
}
static void bw_enter_mode(int pi, int mode){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    if (G.net_mode==NET_CLIENT) return;
    if (!b->active || b->fainted) return;
    bwgate[pi]=0;
    S->from_arcade = bw_arc_src[pi]; S->arc_cab = bw_arc_cab[pi]; bw_arc_src[pi]=0;
    if (!b->bw_started){
        b->bw_started = 1;
        b->bw_seed = (int)(0x1000 + (rng_u32() % 60000));
        b->bw_x = 0; b->bw_y = 56;
    }
    b->bw_mode = mode;
    if (mode==2){ ent_reset(pi); }
    if (mode==1 && !b->story_started){
        b->story_started = 1; bw_qset(b, 0, 1);   // quest 0: meet the elder
    }
    S->defeated_n = 0; S->bat.active = 0; S->sleep_t = 0; S->grace_t = 0; S->note_t = 0;
    for (int i=0;i<BW_ROAMN;i++) S->roam[i].alert = 0;   // a fresh entry is a fresh temper: no hunt survives the gate (alerts also freeze during battles, so a stale one could otherwise outlive its mode)
    S->log_open = 0; S->site_cur = -2; S->bw_menu = 0; S->bw_msel = 0;
    if (mode==2){ b->x = 3.5f*BW_TILE; b->y = ENT_DOORY; }
    else if (mode==3){ plat_enter(pi); }
    else if (mode==4){ craft_enter(pi); }
    else if (mode==5){ tank_enter(pi); }
    else if (mode==6){ cas_enter(pi); }
    else if (mode==7){ crt_enter(pi); }
    else if (mode==8){ war_enter(pi); }
    else { b->x = b->bw_x; b->y = b->bw_y; }
    b->in_bw = 1;   // the DEV stays parked at the gate in the mall
    if (mode==2){ char *q=S->note; q=sappend(q, "ENTRANCES. A COZY HOTEL. "); q=sappend(q, lbl_int()); q=sappend(q, " OPENS DOORS + DRAWERS. MIND THE ZIP BUG."); *q=0; }
    else if (mode==3){ char *q=S->note; q=sappend(q, "LEVEL "); q=sapp_int(q, S->plat_lvl); q=sappend(q, " - "); q=sappend(q, REGION_NAME[S->plat_region]); q=sappend(q, ". "); q=sappend(q, lbl_fire()); q=sappend(q, " JUMPS. STOMP BUGS!"); *q=0; }
    else if (mode==4){ char *q=S->note; q=sappend(q, "BUDDY CRAFT. "); q=sappend(q, lbl_int()); q=sappend(q, " GATHERS + RESTS. "); q=sappend(q, lbl_fire()); q=sappend(q, " SWINGS. "); q=sappend(q, lbl_wall()); q=sappend(q, " CRAFTS."); *q=0; }
    else if (mode==5){ char *q=S->note; q=sappend(q, "TANK RESCUE. "); q=sappend(q, lbl_fire()); q=sappend(q, " FIRES THE CANNON. FREE CAPTIVES, DROP AT PADS."); *q=0; }
    else if (mode==6){ char *q=S->note; q=sappend(q, "CASTLE. TRAPPED! RUN INTO THINGS TO TAKE OR FIGHT. FIND THE KEY, ESCAPE!"); *q=0; }
    else if (mode==7){ char *q=S->note; q=sappend(q, "THE WAITING ROOM. PICK YOUR COLOUR!"); *q=0; }
    else if (mode==8){ char *q=S->note; q=sappend(q, "WAR! TAP A REGION TO INVADE IT. CONQUER ALL EIGHT!"); *q=0; }
    else if (mode==1 && b->story_bits==(1<<0)) scpy(S->note, "STORY MODE. PATCH VILLAGE LIES NORTH-WEST.", 72);
    else if (mode==1) scpy(S->note, "BACK TO THE STORY. THE MEADOW REMEMBERS.", 72);
    else scpy(S->note, "BANZAI WORLD. WATCH FOR HACKERS.", 72);
    S->note_t = 3.4f;
    sfx(19);
}
// the gate panel: STORY MODE / FREE PLAY / LEAVE, drawn over a view
static void bw_gate_draw(int pi, int vx, int vy, int vw2, int vh2){
    Buddy *b=&G.bud[pi];
    int w = vw2<340? vw2-16 : 320, h = 150;
    int x = vx + (vw2-w)/2, y = vy + (vh2-h)/2;
    if (y < vy+4) y = vy+4;
    rect_fill(x, y, w, h, COL(18,22,20)); rect_line(x, y, w, h, COL(198,164,84));
    draw_text_center(x+w/2, y+10, "BANZAI WORLD", 2, COL(255,214,64));
    { char buf[64]; char *q=buf; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[b->species%BUDN]); q=sappend(q, " IS ITCHING TO GO"); *q=0;
      draw_text_center(x+w/2, y+32, buf, 1, COL(200,214,200)); }
    // the gate is for the WORLD now (Nov 10): story and free play.
    // Entrances, Super Buddy Run and Buddy Craft moved to the Arcade.
    static const char *DESC[3] = {
        "A SASSY LITTLE ADVENTURE. QUESTS, PLACES, TROUBLE.",
        "THE ENDLESS MEADOW. ROAM, BONK, NAP.",
        "STAY IN THE MALL." };
    for (int r=0;r<3;r++){
        int ry = y + 52 + r*20;
        const char *nm = r==0? (b->story_started? "CONTINUE STORY" : "STORY MODE") : r==1? "FREE PLAY" : "LEAVE";
        if (bwgate_sel[pi]==r){ rect_fill(x+10, ry-3, w-20, 15, COL(70,58,30)); draw_text(x+16, ry, ">", 1, COL(255,220,90)); }
        draw_text(x+30, ry, nm, 1, bwgate_sel[pi]==r? COL(255,244,210) : COL(170,180,170));
    }
    draw_text_center(x+w/2, y+h-16, DESC[bwgate_sel[pi]%3], 1, COL(150,170,190));
    if (mobile_ui() && !pad_mode() && pi==G.my_id)
        draw_text_center(x+w/2, y+h-30, "TAP A MODE TO PLAY", 1, COL(120,132,150));
    else { char buf[64]; char *q=buf; q=sappend(q, lbl_fire()); q=sappend(q, " - GO IN    "); q=sappend(q, lbl_exit()); q=sappend(q, " - STAY"); *q=0;
      draw_text_center(x+w/2, y+h-30, buf, 1, COL(120,132,150)); }
}
static void bw_gate_pick(int pi){
    int s2 = bwgate_sel[pi];
    if (s2==0) bw_enter_mode(pi, 1);
    else if (s2==1) bw_enter_mode(pi, 0);
    else { bwgate[pi]=0; sfx(6); }
}
// touch parity for the gate panel (the store panels all take taps;
// this one never did, so on a phone the rows were decoration —
// Daniel, Nov 4). Tap a row to choose it; tap outside to back out.
// Geometry mirrors bw_gate_draw exactly.
static void bw_gate_tap(int pi, int vx, int vy, int vw2, int vh2){
    int w = vw2<340? vw2-16 : 320, h = 150;
    int x = vx + (vw2-w)/2, y = vy + (vh2-h)/2;
    if (y < vy+4) y = vy+4;
    IN.tapped = 0;
    if (IN.tap_x<x || IN.tap_x>=x+w || IN.tap_y<y || IN.tap_y>=y+h){ bwgate[pi]=0; sfx(6); return; }
    for (int r=0;r<3;r++){
        int ry = y + 52 + r*20;
        if (IN.tap_y>=ry-4 && IN.tap_y<=ry+15){ bwgate_sel[pi]=r; bw_gate_pick(pi); return; }
    }
}
// --- the Arcade cabinet panel -------------------------------------------------
// One cabinet per game: PLAY launches the cabinet's game with the
// buddy, LEAVE keeps you in the arcade. Mirrors the gate panel's
// behaviour (keys, pad, taps) so every device drives it the same way.
static void arc_gate_try(int pi){
    Player *p=&G.p[pi]; Buddy *b=&G.bud[pi];
    if (G.net_mode==NET_CLIENT) return;
    if (G.net_mode==NET_HOST && any_remote_connected()){ spawn_floater(p->x, p->y-24, "THE ARCADE IS A SOLO TRIP", COL(255,200,150)); sfx(16); return; }
    if (!b->active){ spawn_floater(p->x, p->y-24, "YOU NEED A BANZAI BUDDY FIRST", COL(255,150,150)); sfx(16); return; }
    if (b->fainted){ spawn_floater(p->x, p->y-24, "YOUR BUDDY FAINTED - REVIVE IT FIRST", COL(255,150,150)); sfx(16); return; }
    arcgate[pi]=1; arcgate_sel[pi]=0; sfx(19);
}
static void arc_gate_draw(int pi, int vx, int vy, int vw2, int vh2){
    Buddy *b=&G.bud[pi];
    int cab = arc_cab[pi]%7;
    int w = vw2<340? vw2-16 : 320, h = 132;
    int x = vx + (vw2-w)/2, y = vy + (vh2-h)/2;
    if (y < vy+4) y = vy+4;
    rect_fill(x, y, w, h, COL(14,16,30)); rect_line(x, y, w, h, COL(120,240,255));
    draw_text_center(x+w/2, y+10, ARC_NAME[cab], 2, COL(140,255,255));
    { char buf[64]; char *q=buf; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[b->species%BUDN]); q=sappend(q, " GRABS THE CONTROLS"); *q=0;
      draw_text_center(x+w/2, y+32, buf, 1, COL(200,214,200)); }
    static const char *DESC[7] = {
        "A COZY HOTEL OF DOORS. HIDE IN THE BUSHES. MIND THE ZIP BUG.",
        "A SIDE-SCROLLING RUN. STOMP BUGS. PLANT THE FLAG.",
        "AN ENDLESS WILD. CHOP, MINE, CRAFT, BUILD.",
        "DRIVE THE TANK. FREE THE CAPTIVES. BREAK THE BOSS.",
        "TRAPPED IN A CASTLE! 13 TREASURES, ONE KEY, ONE GATE.",
        "GRAND PRIX! 4 RACES, 8 REGIONS, 2 CUPS. RANK BY POINTS.",
        "A TAP-FIRST RTS. HARVEST BITS, RAISE BUDDIES, CONQUER 8 REGIONS." };
    for (int r=0;r<2;r++){
        int ry = y + 52 + r*20;
        const char *nm = r==0? "PLAY" : "LEAVE";
        if (arcgate_sel[pi]==r){ rect_fill(x+10, ry-3, w-20, 15, COL(30,60,80)); draw_text(x+16, ry, ">", 1, COL(140,255,255)); }
        draw_text(x+30, ry, nm, 1, arcgate_sel[pi]==r? COL(235,255,255) : COL(170,180,190));
    }
    draw_text_center(x+w/2, y+h-16, DESC[cab], 1, COL(150,170,190));
    if (mobile_ui() && !pad_mode() && pi==G.my_id)
        draw_text_center(x+w/2, y+h-30, "TAP TO PLAY", 1, COL(120,132,150));
    else { char buf[64]; char *q=buf; q=sappend(q, lbl_fire()); q=sappend(q, " - PLAY    "); q=sappend(q, lbl_exit()); q=sappend(q, " - LEAVE"); *q=0;
      draw_text_center(x+w/2, y+h-30, buf, 1, COL(120,132,150)); }
}
static void arc_gate_pick(int pi){
    if (arcgate_sel[pi]==0){
        int cab = arc_cab[pi]%7;
        arcgate[pi]=0;
        bw_arc_src[pi]=1; bw_arc_cab[pi]=cab;
        bw_enter_mode(pi, ARC_MODE[cab]);
    } else { arcgate[pi]=0; sfx(6); }
}
static void arc_gate_tap(int pi, int vx, int vy, int vw2, int vh2){
    int w = vw2<340? vw2-16 : 320, h = 132;
    int x = vx + (vw2-w)/2, y = vy + (vh2-h)/2;
    if (y < vy+4) y = vy+4;
    IN.tapped = 0;
    if (IN.tap_x<x || IN.tap_x>=x+w || IN.tap_y<y || IN.tap_y>=y+h){ arcgate[pi]=0; sfx(6); return; }
    for (int r=0;r<2;r++){
        int ry = y + 52 + r*20;
        if (IN.tap_y>=ry-4 && IN.tap_y<=ry+15){ arcgate_sel[pi]=r; arc_gate_pick(pi); return; }
    }
}
// P1's current view rectangle in co-op, computed exactly as the
// render pass computes view 0 (shared view, or P1's half of the split)
static void p1_view_rect(int *x, int *y, int *w, int *h){
    if (vsplit_f <= 0.02f && !vsplit){ *x=0; *y=0; *w=VW; *h=VH; return; }
    if (vorient){
        int seam = vp1first? VW - (int)((VW/2)*vsplit_f) : (int)((VW/2)*vsplit_f);
        *y=0; *h=VH;
        if (vp1first){ *x=0; *w=seam; } else { *x=seam; *w=VW-seam; }
    } else {
        int seam = vp1first? VH - (int)((VH/2)*vsplit_f) : (int)((VH/2)*vsplit_f);
        *x=0; *w=VW;
        if (vp1first){ *y=0; *h=seam; } else { *y=seam; *h=VH-seam; }
    }
}
static void bw_exit(int pi, int fainted){
    BwSim *S = &BWS[pi];
    Player *p=&G.p[pi]; Buddy *b=&G.bud[pi];
    if (b->bw_mode==2 || b->bw_mode==3 || b->bw_mode==5 || b->bw_mode==6 || b->bw_mode==7 || b->bw_mode==8){ b->bw_x = 0; b->bw_y = 56; }
    else if (b->bw_mode==4){ b->craft_x = b->x; b->craft_y = b->y; }
    else { b->bw_x = b->x; b->bw_y = b->y; }
    S->bat.active = 0;
    S->bw_menu = 0;
    b->in_bw = 0;
    if (S->from_arcade){ int cb2 = S->arc_cab%7; p->x = ARC_SX[cb2]; p->y = ARC_CY + ARC_ROWY[cb2] + 66; }   // back to your cabinet, whichever row it stands in
    else { p->x = BW_SX; p->y = MALL_CY+96; }
    S->from_arcade = 0;
    p->aim = -PI/2;
    b->x = p->x - 30; b->y = p->y + 10;
    do_save();
    sfx(19);
    if (fainted) spawn_floater(p->x, p->y-26, "YOU WAKE UP IN THE MALL. YOUR BUDDY FAINTED.", COL(255,180,150));
}
// --- the frame ---------------------------------------------------------------
static void bw_roam_refresh(Buddy *b){
    BwSim *S = &BWS[(int)(b - G.bud)];
    if (b->bw_mode==2 || b->bw_mode==6 || b->bw_mode==7) return;
    u32 seed=(u32)b->bw_seed;
    int cx0=bw_cellof(b->x), cy0=bw_cellof(b->y);
    int want_cell[BW_ROAMN]; int nw=0;
    for (int cx=cx0-3; cx<=cx0+3 && nw<BW_ROAMN; cx++)
        for (int cy=cy0-3; cy<=cy0+3 && nw<BW_ROAMN; cy++){
            float ox, oy; if (bw_cell_feat(cx, cy, seed, &ox, &oy, b->bw_mode==1)!=2) continue;
            int key=bw_cell_key(cx, cy);
            if (bw_defeated_has(S, key)) continue;
            float wx=cx*BW_CELL+ox, wy=cy*BW_CELL+oy;
            if (bw_outside_world(wx, wy)) continue;   // no hackers at sea
            float dx=wx-b->x, dy=wy-b->y;
            if (dx*dx+dy*dy > 640*640) continue;
            want_cell[nw++]=key;
        }
    // keep the roamers we already have, drop the rest, spawn the new
    for (int i=0;i<BW_ROAMN;i++){
        if (!S->roam[i].cell && S->roam[i].x==0 && S->roam[i].y==0) continue;
        int keep=0; for (int j=0;j<nw;j++) if (want_cell[j]==S->roam[i].cell) keep=1;
        if (!keep){ S->roam[i].cell=0; S->roam[i].x=0; S->roam[i].y=0; }
    }
    for (int j=0;j<nw;j++){
        int have=0; for (int i=0;i<BW_ROAMN;i++) if (S->roam[i].cell==want_cell[j]) have=1;
        if (have) continue;
        for (int i=0;i<BW_ROAMN;i++) if (S->roam[i].cell==0 && S->roam[i].x==0 && S->roam[i].y==0){
            // recover the cell coords from the key is not possible; recompute by scan
            int cx0b=bw_cellof(b->x), cy0b=bw_cellof(b->y), done=0;
            for (int cx=cx0b-3; cx<=cx0b+3 && !done; cx++)
                for (int cy=cy0b-3; cy<=cy0b+3 && !done; cy++){
                    if (bw_cell_key(cx, cy)!=want_cell[j]) continue;
                    float ox, oy; bw_cell_feat(cx, cy, seed, &ox, &oy, b->bw_mode==1);
                    S->roam[i].cell=want_cell[j];
                    S->roam[i].x=cx*BW_CELL+ox; S->roam[i].y=cy*BW_CELL+oy;
                    S->roam[i].wt=0; S->roam[i].dir=0; S->roam[i].alert=0; S->roam[i].lvl=bw_level_at(b, S->roam[i].cell);
                    done=1;
                }
            break;
        }
    }
}
static void bw_press_term(int pi, int t){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    int ord[3]; bw_term_order(b, ord);
    int prog = bw_puz_prog(b);
    if (t == ord[prog]){
        prog++; bw_puz_prog_set(b, prog); sfx(19);
        if (prog>=3){
            b->story_bits |= (1 << 15);
            scpy(S->note, "OLD LOCKS LET GO SOMEWHERE. THE INNER AISLE IS OPEN.", 72); S->note_t=4.0f;
        } else {
            char *q=S->note; q=sappend(q, "THE "); q=sappend(q, BW_TERM_NAME[t]); q=sappend(q, " TERMINAL CHIRPS. "); q=sapp_int(q, prog); q=sappend(q, "/3"); *q=0;
            S->note_t=2.6f;
        }
    } else {
        bw_puz_prog_set(b, 0); sfx(16);
        scpy(S->note, "THE TERMINALS BUZZ AND FORGET EVERYTHING. START OVER.", 72); S->note_t=3.2f;
    }
}
static void bw_read_sign(int pi){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    int ord[3]; bw_term_order(b, ord);
    char *q=S->note; q=sappend(q, "SCRATCHED DEEP: ");
    for (int i=0;i<3;i++){ if (i) q=sappend(q, ". "); q=sappend(q, BW_TERM_NAME[ord[i]]); }
    q=sappend(q, ". - R."); *q=0;
    S->note_t=4.5f; sfx(6);
}
static void bw_pick_tool(int pi, int t){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    b->story_bits |= (1<<(24+t)); sfx(11);
    char *q=S->note; q=sappend(q, "FOUND MOP'S "); q=sappend(q, BW_TOOL_NAME[t]); q=sappend(q, "! (");
    q=sapp_int(q, bw_tools_found(b)); q=sappend(q, "/3)"); *q=0;
    S->note_t=3.0f;
}
static void bw_read_hsign(int pi, int idx){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    int first = !((b->story_bits2>>(11+idx))&1);
    if (first) b->story_bits2 |= (1<<(11+idx));
    sfx(6);
    if (first && bw_qstate2(b,9)==1 && bw_signs_read(b)>=3){
        bw_qset2(b,9,2); b->xp += 20;
        scpy(S->note, "THREE SIGNS READ. THE MEADOW APPROVES. +20 XP.", 72); S->note_t=4.0f;
        return;
    }
    char *q=S->note; q=sappend(q, BW_HSIGN_TXT[idx]);
    if (bw_qstate2(b,9)==1){ q=sappend(q, " ("); q=sapp_int(q, bw_signs_read(b)); q=sappend(q, "/3)"); }
    *q=0; S->note_t=4.2f;
}
static void bw_meet_bitling(int pi){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    sfx(11);
    if (bw_qstate2(b,8)==1){
        bw_qset2(b,8,2); b->xp += 30;
        b->items[0] += 1; if (b->items[0]>9) b->items[0]=9;
        scpy(S->note, "BITLING CHIRPS AND TROTS HOME. +30 XP. PATCHINA WILL BE GLAD.", 72); S->note_t=4.5f;
    } else {
        scpy(S->note, "BITLING: BEEP. (IT SEEMS LOST, AND HAPPY ABOUT IT.)", 72); S->note_t=3.4f;
    }
}
static void bw_talk(int pi, int npc){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    sfx(6);
    S->grace_t = 4.0f;   // a conversation is a breather (Nov 4): with hunters about, nobody should get jumped mid-dialogue or while opening the quest log right after
    if (npc==0){   // Elder Patchina
        if (bw_qstate(b,0)==1){
            bw_qset(b,0,2); bw_qset(b,1,1); bw_qset(b,2,1); bw_qset2(b,9,1);
            b->items[0] += 2; if (b->items[0]>9) b->items[0]=9;
            scpy(S->note, "PATCHINA: BONK 3 HACKERS AT THE SERVER FARM, DEAR.", 72); S->note_t=4.5f;
        } else if (bw_qstate(b,1)==2 && bw_qstate(b,3)==0){
            bw_qset(b,3,1); bw_qset_x(b,5,1);
            scpy(S->note, "PATCHINA: ZED'S GATE NEEDS ITS CRANK. IT SANK IN THE WESTERN RUINS.", 72); S->note_t=4.5f;
        } else if (bw_qstate(b,1)==2 && bw_qstate_x(b,5)==1){
            scpy(S->note, "PATCHINA: WEST, DEAR. THE RUINS. MIND THE HOARDER.", 72); S->note_t=4.0f;
        } else if (bw_qstate(b,1)==2 && bw_qstate2(b,8)==0){
            bw_qset2(b,8,1);
            scpy(S->note, "PATCHINA: A STRAY PROCESS WANDERS SOUTH OF US. FETCH IT HOME.", 72); S->note_t=4.5f;
        } else if (bw_qstate(b,1)==2){
            scpy(S->note, "PATCHINA: THROUGH THE GATE, DEAR. GIVE HIM A LOVELY FUNERAL.", 72); S->note_t=4.0f;
        } else {
            scpy(S->note, "PATCHINA: THE FARM, DEAR. EAST-NORTH-EAST. MIND THE RACKS.", 72); S->note_t=4.0f;
        }
    } else if (npc==1){   // Mop
        if (bw_qstate_x(b,4)==1 && bw_tools_found(b)>=3){
            bw_qset_x(b,4,2); b->xp += 40;
            b->items[2] += 2; if (b->items[2]>9) b->items[2]=9;
            scpy(S->note, "MOP: MY TOOLS! COOKIES FOR YOU. +40 XP.", 72); S->note_t=4.0f;
        } else if (bw_qstate_x(b,4)==2 && bw_qstate2(b,7)==0){
            bw_qset2(b,7,1); b->story_bits2 |= (1<<10);   // parcel carried
            scpy(S->note, "MOP: TAKE THIS PARCEL TO GLITCH AT THE LAKE. MIND IT.", 72); S->note_t=4.2f;
        } else if (bw_qstate(b,0)==2 && bw_qstate_x(b,4)==0){
            bw_qset_x(b,4,1);
            scpy(S->note, "MOP: LOST MY WRENCH, HAMMER AND BRUSH OUT THERE. FIND THEM?", 72); S->note_t=4.2f;
        } else if (bw_qstate_x(b,4)==1){
            char *q=S->note; q=sappend(q, "MOP: "); q=sapp_int(q, bw_tools_found(b)); q=sappend(q, "/3 TOOLS HOME. THE MEADOW HIDES THEM WELL."); *q=0;
            S->note_t=3.6f;
        } else if (bw_qstate_x(b,4)==2){
            scpy(S->note, "MOP: THE MEADOW FEARS MY MOP AGAIN.", 72); S->note_t=3.6f;
        } else {
            scpy(S->note, "MOP: I PATCH MY SOCKS WITH OLD CODE. SLIGHTLY CURSED.", 72); S->note_t=3.6f;
        }
    } else {   // Glitch, the lake hermit
        if (bw_qstate(b,2)==1){
            bw_qset(b,2,2);
            b->items[1] += 3; if (b->items[1]>9) b->items[1]=9;
            scpy(S->note, "GLITCH: COMPANY! TAKE THESE. +3 ENERGY DRINKS.", 72); S->note_t=4.0f;
        } else if (bw_qstate2(b,7)==1 && ((b->story_bits2>>10)&1)){
            bw_qset2(b,7,2); b->story_bits2 &= ~(1<<10);
            b->xp += 40; b->items[2] += 1; if (b->items[2]>9) b->items[2]=9;
            scpy(S->note, "GLITCH: MOP'S PARCEL! UNBROKEN. +40 XP. THE COOKIE IS YOURS.", 72); S->note_t=4.5f;
        } else if (bw_qstate2(b,6)==1 && bw_lakekills(b)>=2){
            bw_qset2(b,6,2); b->xp += 50;
            b->items[1] += 2; if (b->items[1]>9) b->items[1]=9;
            scpy(S->note, "GLITCH: QUIET ICE AGAIN. +50 XP. DRINKS FOR THE ROAD.", 72); S->note_t=4.5f;
        } else if (bw_qstate(b,2)==2 && bw_qstate2(b,6)==0){
            bw_qset2(b,6,1);
            scpy(S->note, "GLITCH: THE LAKE IS CRAWLING. BONK 2 ON THE ICE FOR ME.", 72); S->note_t=4.5f;
        } else if (bw_qstate_x(b,5)==1 && !bw_has_crank(b)){
            scpy(S->note, "GLITCH: THE CACHE SINGS UNDER THE WESTERN STONES. IT SITS ON IT.", 72); S->note_t=4.0f;
        } else if (bw_has_crank(b) && !((b->story_bits>>17)&1)){
            scpy(S->note, "GLITCH: YOU CRANKED IT. ZED NEXT. GIVE HIM MY WORST.", 72); S->note_t=4.0f;
        } else {
            scpy(S->note, "GLITCH: THE LAKE KEEPS WHAT THE COMPILER THREW AWAY.", 72); S->note_t=3.6f;
        }
    }
}
static void bw_use_item(int pi, int it){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    if (b->items[it]<=0){ scpy(S->note, "NONE LEFT. HACKERS DROP THEM, CAMPS STOCK THEM.", 72); S->note_t=2.4f; sfx(16); return; }
    if (it==0){
        if (b->hp>=b->maxhp){ scpy(S->note, "ALREADY FINE. SAVE THE PATCH.", 72); S->note_t=2.2f; return; }
        b->items[0]--; b->hp += b->maxhp*0.4f; if (b->hp>b->maxhp) b->hp=b->maxhp;
        scpy(S->note, "PATCH NOTES APPLIED. STINGS LESS.", 72); S->note_t=2.4f; sfx(11);
    } else if (it==1){
        if (b->energy>=BUD_ENERGY_MAX){ scpy(S->note, "ALREADY BUZZING.", 72); S->note_t=2.2f; return; }
        b->items[1]--; b->energy += 50; if (b->energy>BUD_ENERGY_MAX) b->energy=BUD_ENERGY_MAX;
        scpy(S->note, "ENERGY DRINK DOWN. BUZZING.", 72); S->note_t=2.4f; sfx(11);
    } else if (it==2){
        b->items[2]--; S->buff=1;
        scpy(S->note, "COOKIE EATEN. THE NEXT HIT LANDS DOUBLE.", 72); S->note_t=2.6f; sfx(11);
    } else {
        scpy(S->note, "CTRL-Z ONLY UNDOES BATTLES. NICE TRY, THOUGH.", 72); S->note_t=2.4f; sfx(6);
    }
}
// --- Banzai World P1 input, unified across devices (Daniel, Nov 5) ----
// Daniel's rule: pick a touch scheme once — STICK or DRAG — and it works
// the same in every minigame. Before this fill existed as one place, the
// touch FIRE button never produced a `sel` (no stick-throwing in
// Entrances, no swinging in Buddy Craft), menu navigation ignored the
// stick entirely, and DRAG steered nothing at all in Banzai World.
// Both P1 call sites (solo/co-op frame and the net-client frame) pack
// P1's BwIn through here, so every mode inherits the same scheme.
static int bw_tf_prev, bw_drag_prev, bw_nav_dir; static float bw_nav_rep;
static void bw_fill_p1(BwIn *bin, float dt){
    Buddy *b = &G.bud[G.my_id]; BwSim *S = &BWS[G.my_id];
    bin->keys = IN.keys | IN.touch_keys;
    int tf = (IN.touch_keys & K_FIRE)? 1:0;
    bin->sel = IN.select_pressed || (tf && !bw_tf_prev);
    // the K_UP fold is the platformer's hold-to-jump-higher; in TANK
    // RESCUE (mode 5) Up is just steering north, and folding it into
    // sel_held made the cannon fire the whole way up the field —
    // steering must never pull the trigger there (Nov 10 fix)
    bin->sel_held = ((IN.act_held>>ACT_FIRE)&1) || ((IN.raw>>G.bind[ACT_FIRE])&1) || ((bin->keys & K_UP) && b->bw_mode!=5) || tf;
    bw_tf_prev = tf;
    bin->interact = IN.interact_pressed || IN.enter_pressed;
    bin->exit_ = IN.esc_pressed;
    bin->pause = (IN.pause_pressed && G.net_mode!=NET_CLIENT)? 1:0;   // Start/P opens the BW menu now (guests can't pause — standing rule)
    bin->wall = IN.wall_pressed;
    bin->up = IN.menu_up; bin->down = IN.menu_down;
    // stick flicks walk BW menus (craft menu, story log, battles): a
    // fresh edge, then the same auto-repeat timing the arrow keys get
    int dir = (IN.touch_keys & K_UP)? -1 : (IN.touch_keys & K_DOWN)? 1 : 0;
    if (dir && dir!=bw_nav_dir){ if (dir<0) bin->up=1; else bin->down=1; bw_nav_rep = 0.38f; }
    else if (dir){ bw_nav_rep -= dt; if (bw_nav_rep<=0){ if (dir<0) bin->up=1; else bin->down=1; bw_nav_rep = 0.09f; } }
    bw_nav_dir = dir;
    // holding FIRE (or a DRAG hold) in the field-combat modes swings at
    // the weapon's own pace; the single edge above is for menus/throws
    if ((tf || (G.touch_ctl==0 && IN.touch_down)) && !S->bat.active && !S->craft_menu && !S->bw_menu
        && (b->bw_mode==2 || b->bw_mode==4) && b->atk_cd<=0) bin->sel = 1;
    // DRAG in Banzai World: steer toward the finger, meadow-style. The
    // buddy sits at his view's centre in the top-down modes; in the
    // platformer, run toward the finger and jump when it rides high.
    // (WAR!, mode 8, has no walking buddy to steer — its drags pan
    // the camera inside war_update instead.)
    if (G.touch_ctl==0 && IN.touch_down && !S->bat.active && b->bw_mode!=8){
        int vx,vy,vw2,vh2; p1_view_rect(&vx,&vy,&vw2,&vh2);
        if (vw2>=VW && !S->craft_menu){
            int dx = IN.touch_x-(vx+vw2/2), dy = IN.touch_y-(vy+vh2/2);
            if (b->bw_mode==3){
                if (dx>16) bin->keys |= K_RIGHT; else if (dx<-16) bin->keys |= K_LEFT;
                if (dy<-44){ if (!bw_drag_prev) bin->sel=1; bin->sel_held=1; }
            } else {
                if (dx>16) bin->keys |= K_RIGHT; else if (dx<-16) bin->keys |= K_LEFT;
                if (dy>16) bin->keys |= K_DOWN; else if (dy<-16) bin->keys |= K_UP;
                if (b->bw_mode!=5) bin->sel_held = 1;   // drag-steering a tank is not firing it
            }
        }
        bw_drag_prev = 1;
    } else bw_drag_prev = 0;
    // in DRAG there is no USE button: a tap IS the interact button
    // (doors, drawers, gathering, talking). The craft menu and the
    // story log consume their own taps, so hands off while they're up.
    if (G.touch_ctl==0 && IN.tapped && !S->bat.active && !S->craft_menu && !S->log_open && !S->bw_menu && b->bw_mode!=8) bin->interact = 1;
}

static void bw_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    if (b->mv_t>0) b->mv_t-=dt;
    // the Start-menu (Daniel, Nov 6): Start/P used to dump you out of
    // the minigame; now it opens a menu — RESUME / CONTROLS / LEAVE —
    // and the CONTROLS panel shows THIS mode's buttons. While it is
    // open the world holds its breath. Escape still leaves instantly.
    if (S->bw_menu){
        if (S->bw_menu==2){   // the controls panel: anything backs out
            if (in->sel || in->interact || in->exit_ || in->pause) S->bw_menu=1;
            if (pi==G.my_id && IN.tapped){ S->bw_menu=1; IN.tapped=0; }
        } else {
            if (in->up){ S->bw_msel = (S->bw_msel+2)%3; sfx(6); }
            if (in->down){ S->bw_msel = (S->bw_msel+1)%3; sfx(6); }
            int act = (in->sel || in->interact)? S->bw_msel : -1;
            if (pi==G.my_id && IN.tapped){
                int vx,vy,vw2,vh2; p1_view_rect(&vx,&vy,&vw2,&vh2);
                int pw=300, ph=140, px=vx+(vw2-pw)/2, py=vy+(vh2-ph)/2;
                int tx=IN.tap_x, ty=IN.tap_y; IN.tapped=0;
                act = -2;   // a tap outside the panel just closes it
                if (tx>=px && tx<px+pw && ty>=py && ty<py+ph)
                    for (int i=0;i<3;i++){ int ry=py+48+i*26;
                        if (ty>=ry-6 && ty<ry+17){ act=i; break; } }
            }
            if (act==0) S->bw_menu=0;
            else if (act==1) S->bw_menu=2;
            else if (act==2){ S->bw_menu=0; bw_exit(pi, 0); return; }
            else if (act==-2) S->bw_menu=0;
            else if (in->exit_ || in->pause) S->bw_menu=0;
        }
        return;
    }
    if (in->pause){ S->bw_menu=1; S->bw_msel=0; sfx(6); return; }
    if (!b->active){ bw_exit(pi, 0); return; }
    if (S->grace_t>0) S->grace_t-=dt;
    if (S->note_t>0) S->note_t-=dt;
    if (S->bat.active){ b->bw_x = b->x; b->bw_y = b->y; bw_battle_update(pi, dt, in); return; }
    if (S->sleep_t>0){
        S->sleep_t -= dt;
        if (!S->slept && S->sleep_t<=0.7f){
            S->slept=1; b->hp=b->maxhp; b->energy=BUD_ENERGY_MAX;
            if (b->items[0]<9) b->items[0]++;   // every camp keeps a spare Patch Notes
            scpy(S->note, "YOU SLEPT SOUNDLY. GOOD AS NEW.", 72); S->note_t=2.6f;
            do_save();
        }
        return;
    }
    if (!S->log_open && in->wall && !S->bat.active && b->bw_mode<2){ S->log_open = 1; S->log_sel=0; sfx(6); return; }
    if (S->log_open){
        if (in->up){ S->log_sel = (S->log_sel+3)%4; sfx(6); }
        if (in->down){ S->log_sel = (S->log_sel+1)%4; sfx(6); }
        if (in->sel) bw_use_item(pi, S->log_sel);
        if (in->exit_ || in->wall) S->log_open = 0;
        return;
    }
    if (b->bw_mode==2){ ent_update(pi, dt, in); return; }
    if (b->bw_mode==3){ plat_update(pi, dt, in); return; }
    if (b->bw_mode==4){ craft_update(pi, dt, in); return; }
    if (b->bw_mode==5){ tank_update(pi, dt, in); return; }
    if (b->bw_mode==6){ cas_update(pi, dt, in); return; }
    if (b->bw_mode==7){ crt_update(pi, dt, in); return; }
    if (b->bw_mode==8){ war_update(pi, dt, in); return; }
    float mx=0, my=0; int k=in->keys;
    if (k&K_UP) my-=1; if (k&K_DOWN) my+=1; if (k&K_LEFT) mx-=1; if (k&K_RIGHT) mx+=1;
    if (mx!=0 || my!=0){
        float d=fsqrt(mx*mx+my*my); mx/=d; my/=d;
        b->aim = fatan2(my, mx);
        float sp = BUD_SPD[b->species%BUDN];
        float nx = b->x + mx*sp*dt, ny = b->y + my*sp*dt;
        if (!bw_solid_b(b, nx, b->y) && !bw_feat_solid(b, nx, b->y)) b->x = nx;
        if (!bw_solid_b(b, b->x, ny) && !bw_feat_solid(b, b->x, ny)) b->y = ny;
        b->walk_ph += dt*9;
        b->mv_t = 0.15f;
    }
    if (b->lunge_t>0) b->lunge_t-=dt;
    // NO passive energy regen in the story world (Daniel, Nov 6):
    // energy comes back at tents and from Energy Drinks, never by
    // just waiting around. (Entrances, Buddy Craft and the meadow
    // keep their own rules — this is the story/free roam only.)
    b->bw_x = b->x; b->bw_y = b->y;
    // roamers wander; one touching you is a battle
    // the two big residents are fought by walking up to them
    if (b->bw_mode==1 && !S->bat.active && S->sleep_t<=0 && !S->log_open){
        // (grace deliberately NOT checked here: it shields you from
        // being jumped by roamers, it must never refuse a boss you
        // deliberately walk up to — that refusal cost a test run)
        if (bw_puz_done(b) && !((b->story_bits>>16)&1)){
            float dx=b->x-BW_SITE_XY[1][0], dy=b->y-BW_SITE_XY[1][1];
            if (dx*dx+dy*dy < 27*27){ bw_boss_start(pi, 1); return; }
        }
        if (!((b->story_bits>>17)&1)){
            float dx=b->x-BW_SITE_XY[3][0], dy=b->y-BW_SITE_XY[3][1];
            if (dx*dx+dy*dy < 32*32){
                if (bw_qstate(b,1)==2 && bw_qstate(b,3)==0) bw_qset(b,3,1);
                bw_boss_start(pi, 2); return;
            }
        }
        if (!((b->story_bits>>22)&1)){
            float dx=b->x-BW_SITE_XY[4][0], dy=b->y-BW_SITE_XY[4][1];
            if (dx*dx+dy*dy < 30*30){ bw_boss_start(pi, 3); return; }
        }
    }
    bw_roam_refresh(b);
    for (int i=0;i<BW_ROAMN;i++){
        BWHack *h=&S->roam[i];
        if (h->cell==0 && h->x==0 && h->y==0) continue;
        // Story Mode hunters (Nov 4, Daniel: "let bad guys actually
        // chase you if they see you where they are facing, or if you
        // get in proximity"): a hacker that spots the buddy — close
        // enough to hear (64), or inside its facing cone out to 180 —
        // gives chase at 88 u/s (every buddy outruns it, barely) and
        // remembers the hunt for 1.8 s after losing sight. Free Play
        // hackers just wander, as they always have.
        int hunting = 0;
        if (h->alert > 0) h->alert -= dt;   // the hunt is forgotten in every mode
        if (b->bw_mode==1){
            float dx=b->x-h->x, dy=b->y-h->y; float d2=dx*dx+dy*dy;
            int sees = 0;
            if (d2 < 64*64) sees = 1;
            else if (d2 < 180*180){
                float dl = fsqrt(d2);
                if (dl>0 && (fcos(h->dir)*dx + fsin(h->dir)*dy)/dl > 0.30f) sees = 1;
            }
            if (sees) h->alert = 1.8f;
            if (h->alert > 0){ h->dir = fatan2(dy, dx); hunting = 1; }
        }
        if (hunting){
            float hx = h->x + fcos(h->dir)*88*dt, hy = h->y + fsin(h->dir)*88*dt;
            if (!bw_solid_b(b, hx, hy)){ h->x=hx; h->y=hy; }
            else { float hx2 = h->x + fcos(h->dir)*88*dt; if (!bw_solid_b(b, hx2, h->y)) h->x=hx2;
                   else { float hy2 = h->y + fsin(h->dir)*88*dt; if (!bw_solid_b(b, h->x, hy2)) h->y=hy2; } }
        } else {
            h->wt -= dt;
            if (h->wt<=0){ h->wt = 1.6f + rng_f()*1.6f; h->dir = rng_f()*6.28318f; }
            float hx = h->x + fcos(h->dir)*26*dt, hy = h->y + fsin(h->dir)*26*dt;
            if (!bw_solid_b(b, hx, hy)){ h->x=hx; h->y=hy; }
        }
        float dx=b->x-h->x, dy=b->y-h->y;
        if (S->grace_t<=0 && dx*dx+dy*dy < 21*21){ bw_battle_start(b, h); return; }
    }
    // story: arriving somewhere new is worth a title card of a note
    if (b->bw_mode==1){
        int st = bw_site_at_xy(b, b->x, b->y);
        if (st!=S->site_cur){
            S->site_cur = st;
            if (st>=0){
                static const char *ARRIVE[5] = {
                    "PATCH VILLAGE. SOMEONE HERE KEEPS THE LIGHTS ON.",
                    "THE RUINED SERVER FARM. IT HUMS IN ITS SLEEP.",
                    "THE FROZEN LAKE OF DEPRECATED CODE.",
                    "ZED'S COMPOUND. THE FENCE IS A FIREWALL. LITERALLY.",
                    "THE OVERGROWN RUINS. SOMETHING SITS ON THE PAST." };
                scpy(S->note, ARRIVE[st], 72); S->note_t = 3.2f;
            }
        }
    }
    if (in->exit_){ bw_exit(pi, 0); return; }   // pad: Back or Start; keys: Esc/P
    if (in->interact){
        int npc = bw_npc_near(b, 46);
        if (npc>=0){ bw_talk(pi, npc); return; }
        int term = bw_term_near(b, 44);
        if (term>=0){ bw_press_term(pi, term); return; }
        if (bw_sign_near(b, 44)){ bw_read_sign(pi); return; }
        int hs = bw_hsign_near(b, 44);
        if (hs>=0){ bw_read_hsign(pi, hs); return; }
        if (bw_bitling_near(b, 44)){ bw_meet_bitling(pi); return; }
        int tool = bw_tool_near(b, 44);
        if (tool>=0){ bw_pick_tool(pi, tool); return; }
        float dx=b->x, dy=b->y+6;
        if (dx*dx+dy*dy < 46*46){ bw_exit(pi, 0); return; }       // at the gate
        float tx, ty;
        if (bw_tent_near(b, 42, &tx, &ty)){ S->sleep_t=1.4f; S->slept=0; sfx(11); }
    }
}
static void bw_draw_hacker(float wx, float wy, int lvl, float lunge, int named);
static void bw_draw_big_hacker(float wx, float wy, int boss, int named);
static void bw_draw_tent(float wx, float wy);
// --- ENTRANCES (Nov 5, Daniel) -------------------------------------------------
// A Doors-like hotel run inside Banzai World, built for Wesley: the loop
// of Doors (numbered doors, one-rule monsters, hide, hunt for keys and
// loot) with the horror drained out — getting caught is a bonk and a
// kazoo, never a scare. Daniel's verdict on the original: too scary.
// Rooms are uniform 12x8-tile slabs laid end to end along +x; room k's
// right wall holds ENTRANCE k+1. Everything about a room derives from
// hash(seed, k), so the hotel is endless and needs no stored map.
// Tiles: 0 floor, 1 wall, 2 door (closed), 3 bush (hide in it),
// 4 drawer, 5 shelf, 6 tent, 7 the EXIT door, 8 the lobby door home.
#define ENT_STRIDE 15        // room width in tiles (was 12 — bigger rooms, longer runs to the door and the bushes: Daniel, Nov 5)
static u32 ent_rnd(Buddy *b, int k, int salt){ return hash2(k*31+7, salt*131+3, (u32)b->bw_seed ^ 0xE77u); }
static int ent_room_of(float x){ int k = (int)(x / (ENT_STRIDE*BW_TILE)) + 1; return k<1? 1 : k; }
static float ent_room_x0(int k){ return (float)((k-1)*ENT_STRIDE*BW_TILE); }   // left wall face
static int ent_in_chase_seg(int k){ for (int d=0; d<5; d++){ int s=k-d; if (s>=19 && s%30==19) return 1; } return 0; }   // Inspector segments: rooms 19-23, 49-53, 79-83 — at least 30 doors apart, five doors long (Daniel, Nov 6)
static int ent_door_locked(int pi, int k){
    Buddy *b=&G.bud[pi]; BwSim *S=&BWS[pi];
    if (S->ent_sub) return ent_rnd(b, k, 2400) % 5 == 0 && k>=3;
    if (k<3 || k==25 || k==50 || k==51) return 0;   // 25 is the Library: the fuses are its lock
    if (ent_in_chase_seg(k)) return 0;
    return ent_rnd(b, k, 400) % 5 == 0;
}
static int ent_dupe_room(Buddy *b, int k){
    if (k<4 || k==25 || k%10==0 || ent_in_chase_seg(k)) return 0;
    return ent_rnd(b, k, 305) % 10 < 6;   // painted doors in most rooms now (Daniel, Nov 6) — and they look the part
}
// the real door is not always in the middle any more (Daniel, Nov 6):
// its row wanders per room, and the west door of the next room is the
// same doorway seen from the other side
static int ent_door_ly(Buddy *b, int k){
    static const int ROWS[5] = { 2, 3, 4, 5, 6 };
    return ROWS[ent_rnd(b, k, 350) % 5];
}
static int ent_doory(Buddy *b, int k){ return ent_door_ly(b, k)*BW_TILE + 24; }
// painted doors share the real door's row set, so row alone tells
// you nothing: 1-2 dupes, never on the real row
static int ent_dupe_n(Buddy *b, int k){ return ent_dupe_room(b, k)? 1 + (int)(ent_rnd(b, k, 307) % 2) : 0; }
static int ent_dupe_ly_i(Buddy *b, int k, int i){
    static const int ROWS[5] = { 2, 3, 4, 5, 6 };
    int real = ent_door_ly(b, k), cand[4], n = 0;
    for (int j=0;j<5;j++) if (ROWS[j]!=real) cand[n++] = ROWS[j];
    int a = cand[ent_rnd(b, k, 300) % 4];
    if (!i) return a;
    int c2 = cand[(ent_rnd(b, k, 300) % 4 + 1 + (int)(ent_rnd(b, k, 302) % 3)) % 4];
    if (c2==a) c2 = cand[(ent_rnd(b, k, 300) % 4 + 2) % 4];
    return c2;
}
static int ent_dupe_ly(Buddy *b, int k){ return ent_dupe_ly_i(b, k, 0); }
// the noise-sensitive Big Bug keeps a few ordinary rooms too now
// (Daniel, Nov 6) — nowhere the chase, a landing, or the Library
static int ent_bigbug_room(int k){
    if (k<6 || k==25 || k%10==0 || ent_in_chase_seg(k)) return 0;
    return k%13==7 || k%17==9;
}
static int ent_furn(int pi, int tx, int ty){
    Buddy *b=&G.bud[pi]; BwSim *S=&BWS[pi];
    if (tx<0) return 0;
    int k = tx/ENT_STRIDE + 1, lx = tx%ENT_STRIDE, ly = ty;
    if (S->ent_sub){   // the service floor: sparser, greyer — vents to hide in, the odd crate, a cot every 20
        if (k%20==0 && lx==3 && ly==1) return 6;
        int nd = (int)(ent_rnd(b, k, 2200) % 2);
        for (int i=0;i<nd;i++){
            int dx = 2 + (int)(ent_rnd(b, k, 2210+i) % (ENT_STRIDE-4)), dy = (ent_rnd(b, k, 2220+i)&1)? 1 : ENT_ROWS-2;
            if (lx==dx && ly==dy) return 4;
        }
        int nb = 3 + (int)(ent_rnd(b, k, 2100) % 3);   // the service floor gets its share too (3-5 vents)
        for (int i=0;i<nb;i++){
            int bx = 1 + (int)(ent_rnd(b, k, 2110+i) % (ENT_STRIDE-2)), by = 1 + (int)((ent_rnd(b, k, 2120+i)>>3) % (ENT_ROWS-2));
            if ((by==ent_door_ly(b, k-1) && bx<=2) || (by==ent_door_ly(b, k) && bx>=ENT_STRIDE-3)) continue;
            if (lx==bx && ly==by) return 3;
        }
        return 0;
    }
    if (k==25){   // the library: shelf rows with two gaps each
        if ((ly==3 || ly==6) && lx>=2 && lx<=ENT_STRIDE-3){
            int g1 = 2 + (int)(ent_rnd(b, 25, 900 + ly) % (ENT_STRIDE-4));
            int g2 = 2 + (int)(ent_rnd(b, 25, 940 + ly) % (ENT_STRIDE-4)); if (g2==g1) g2 = g1>=ENT_STRIDE-3? 2 : g1+1;
            if (lx!=g1 && lx!=g2) return 5;
        }
        return 0;
    }
    if (k%10==0 && lx==3 && ly==1) return 6;   // a landing's tent
    int nd = 1 + (int)(ent_rnd(b, k, 200) % 2);
    for (int i=0;i<nd;i++){
        int dx = 2 + (int)(ent_rnd(b, k, 210+i) % (ENT_STRIDE-4)), dy = (ent_rnd(b, k, 220+i)&1)? 1 : ENT_ROWS-2;
        if (lx==dx && ly==dy) return 4;
    }
    int nb = 4 + (int)(ent_rnd(b, k, 100) % 3);   // 4-6 bushes a room: more shelter, more often (Daniel, Nov 6)
    for (int i=0;i<nb;i++){
        int bx = 1 + (int)(ent_rnd(b, k, 110+i) % (ENT_STRIDE-2)), by = 1 + (int)((ent_rnd(b, k, 120+i)>>3) % (ENT_ROWS-2));
        if ((by==ent_door_ly(b, k-1) && bx<=2) || (by==ent_door_ly(b, k) && bx>=ENT_STRIDE-3)) continue;   // keep the doorway runs clear
        if (lx==bx && ly==by) return 3;
    }
    return 0;
}
static void ent_key_pos(int pi, int k, float *wx, float *wy){
    Buddy *b=&G.bud[pi]; BwSim *S=&BWS[pi];
    if (S->ent_sub){   // downstairs keys hide on the room's crate, rolled with downstairs salts
        int dx = 2 + (int)(ent_rnd(b, k, 2210) % (ENT_STRIDE-4)), dy = (ent_rnd(b, k, 2220)&1)? 1 : ENT_ROWS-2;
        *wx = ((k-1)*ENT_STRIDE + dx)*BW_TILE + 24; *wy = dy*BW_TILE + 10;
        return;
    }
    int nd = 1 + (int)(ent_rnd(b, k, 200) % 2);
    if (nd>=1){ int dx = 2 + (int)(ent_rnd(b, k, 210) % (ENT_STRIDE-4)), dy = (ent_rnd(b, k, 220)&1)? 1 : ENT_ROWS-2;
        *wx = ((k-1)*ENT_STRIDE + dx)*BW_TILE + 24; *wy = dy*BW_TILE + 10; }
    else { *wx = ((k-1)*ENT_STRIDE + 2 + (int)(ent_rnd(b, k, 410) % 7))*BW_TILE + 24; *wy = (ENT_ROWS-2)*BW_TILE + 24; }
}
static const int ENT_FUSE_TX[5] = { 2, 12, 7, 2, 12 };
static const int ENT_FUSE_TY[5] = { 1, 1, 4, 8, 8 };
static int ent_tile(int pi, int tx, int ty){
    Buddy *b=&G.bud[pi]; BwSim *S=&BWS[pi];
    if (tx<0 || ty<0 || ty>=ENT_ROWS) return 1;
    int k = tx/ENT_STRIDE + 1, lx = tx%ENT_STRIDE, ly = ty;
    if (!S->ent_sub && k==51 && lx==ENT_STRIDE/2 && ly==ENT_ROWS-1) return 7;
    if (ly==0 || ly==ENT_ROWS-1) return 1;
    if (lx==0 || lx==ENT_STRIDE-1){
        if (lx==0 && k==1 && ly==ENT_DOOR_LY) return 8;   // the stairs home: always the middle of room 1's west wall
        int door = (lx==ENT_STRIDE-1)? k : k-1;
        if (door>=1 && ly==ent_door_ly(b, door)){
            int upto = S->ent_sub? S->ent_door_sub : S->ent_door_upto;
            if (door <= upto) return 0;
            if (!S->ent_sub && door==25 && S->ent_libdone) return 0;
            return 2;
        }
        if (!S->ent_sub && lx==ENT_STRIDE-1 && !S->ent_dupe_used && ent_dupe_room(b, k)
            && (ly==ent_dupe_ly_i(b, k, 0) || (ent_dupe_n(b, k)>1 && ly==ent_dupe_ly_i(b, k, 1)))) return 2;
        return 1;
    }
    if (!S->ent_sub && k==25 && lx==ENT_STRIDE-2 && ly==ENT_ROWS-2) return 9;   // the tall bookcase (the secret is behind it)
    return ent_furn(pi, tx, ty);
}
static int ent_solid(int pi, float x, float y){
    int t = ent_tile(pi, bw_tileof(x), bw_tileof(y));
    return t==1 || t==2 || t==4 || t==5 || t==6 || t==7 || t==8 || t==9;
}
static int ent_solid_insp(int pi, float x, float y){
    // the Inspector's version: a real doorway (tile 2 on that door's
    // own row) is NOT solid to him — he comes through doors (Daniel,
    // Nov 6). A painted dupe door may share the row set but never the
    // right column-and-row at once, and stays a wall to him.
    Buddy *b=&G.bud[pi];
    int tx = bw_tileof(x), ty = bw_tileof(y);
    int t = ent_tile(pi, tx, ty);
    if (t==2){
        int k = tx/ENT_STRIDE + 1, lx = tx%ENT_STRIDE;
        int door = (lx==ENT_STRIDE-1)? k : k-1;
        return !(door>=1 && ty==ent_door_ly(b, door));
    }
    return t==1 || t==4 || t==5 || t==6 || t==7 || t==8 || t==9;
}
static int ent_room_looted(BwSim *S, int k){
    for (int i=0;i<S->ent_loot_n;i++) if (S->ent_loot_rooms[i]==k) return 1;
    return 0;
}
static void ent_reset(int pi){
    BwSim *S=&BWS[pi];
    S->ent_room = 1; S->ent_door_upto = 0; S->ent_key = 0; S->ent_bits = 0;
    S->ent_sticks = 0; S->ent_lant = 0; S->ent_ev = 0; S->ent_phase = 0;
    S->ent_dark = 0; S->ent_dupe_used = 0; S->ent_libdone = 0; S->ent_fuse_mask = 0;
    S->ent_t = 0; S->ent_grace = 0; S->ent_zpass = 0; S->ent_typs = 0;
    S->ent_sub = 0; S->ent_door_sub = 0; S->ent_passes = 1; S->ent_tamb = 7; S->ent_zt = 0;
    S->ent_chase = 0; S->ent_noise = 0; S->ent_loot_n = 0; S->ent_finished = 0;
    S->ent_ix = 0; S->ent_iy = 0; S->ent_istun = 0; S->ent_bbx = 0; S->ent_bby = 0;
    S->ent_fn = 0; S->ent_mt = 0; S->ent_mstep = 0;
    S->ent_rvx = 0; S->ent_rvy = 0;
}
static void ent_room_setup(int pi, int k){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    S->ent_dupe_used = 0; S->ent_ev = 0; S->ent_phase = 0; S->ent_noise = 0; S->ent_fn = 0;
    if (S->ent_sub){   // the service floor has its own, worse manners
        S->ent_dark = (k%3==1)? 1 : 0;
        if (k%20==0){ /* a maintenance landing: quiet */ }
        else if (ent_rnd(b, k, 3300) % 100 < 14){ S->ent_ev = 7; S->ent_phase = 1; S->ent_t = 0.45f; S->ent_passes = 1;
            scpy(S->note, "THE AIR TEARS. SOMETHING IS COMING THROUGH.", 72); S->note_t = 2.2f; sfx(21); }
        else if (k%6==4){ S->ent_ev = 2; S->ent_phase = 1; S->ent_t = 3.0f; S->ent_zpass = 0;
            S->ent_passes = 2 + (int)(ent_rnd(b, k, 3350) % 3);
            scpy(S->note, "MANY LEGS. MANY FACES. THE MULTIBUG! INTO A VENT!", 72); S->note_t = 2.8f; sfx(20); }
        else if (S->ent_dark){ S->ent_ev = 3; S->ent_phase = 1; S->ent_twait = 1.3f; S->ent_typs = 3; }
        return;
    }
    S->ent_dark = (k%5==2 && k!=25)? 1 : 0;
    if (k>=19 && k%30==19 && !S->ent_chase){ S->ent_chase = k; S->ent_ev = 5; S->ent_phase = 0; }
    else if (S->ent_chase && k>=S->ent_chase && k<=S->ent_chase+4){ S->ent_ev = 5; S->ent_phase = 1; }
    // phase stays 1 across the segment rooms: he has always spawned by
    // the time one is entered (the buddy crosses the start room's
    // middle to leave it), and his position is world-continuous, so he
    // walks through each doorway in plain sight. Resetting to phase 0
    // here was the vanish Daniel saw (Nov 6): undrawn and idle until
    // the buddy crossed the new room's middle, then re-formed at its
    // west edge as if he'd teleported.
    else if (S->ent_chase && k>=S->ent_chase+5){ S->ent_chase = 0; S->ent_bits += 10;
        scpy(S->note, "YOU LOST THE INSPECTOR! +10 BITS", 72); S->note_t = 2.4f; sfx(11); }
    if (S->ent_ev==5){   // a chase room burns: floor fires to dodge, his idea of decor (Daniel, Nov 6)
        S->ent_fn = 0;
        int nf = 2 + (int)(ent_rnd(b, k, 5100) % 2);
        for (int i=0;i<nf && S->ent_fn<4;i++){
            int fx = 2 + (int)(ent_rnd(b, k, 5110+i*7) % (ENT_STRIDE-4));
            int fy = 1 + (int)((ent_rnd(b, k, 5120+i*7)>>3) % (ENT_ROWS-2));
            if ((fy==ent_door_ly(b, k-1) && fx<=3) || (fy==ent_door_ly(b, k) && fx>=ENT_STRIDE-4)) continue;   // never wall off a doorway
            if (ent_furn(pi, (k-1)*ENT_STRIDE + fx, fy)) continue;          // not inside the furniture
            S->ent_fx[S->ent_fn] = ent_room_x0(k) + fx*BW_TILE + 24;
            S->ent_fy[S->ent_fn] = fy*BW_TILE + 24;
            S->ent_fn++;
        }
    }
    if (S->ent_ev) return;
    if (k==25){
        S->ent_ev = 6;
        if (!S->ent_libdone) S->ent_fuse_mask = 0;
        S->ent_bbx = ent_room_x0(25) + (ENT_STRIDE/2)*BW_TILE + 24; S->ent_bby = 1.6f*BW_TILE;
        S->ent_bbdir = 0; S->ent_bbcharge = 0; S->ent_t = 1.0f;
    }
    else if (k%10==0){ /* a landing: quiet, lamplit, safe */ }
    else if (k>=12 && ent_rnd(b, k, 3300) % 100 < 6){ S->ent_ev = 7; S->ent_phase = 1; S->ent_t = 0.45f; S->ent_passes = 1;
        scpy(S->note, "THE AIR TEARS. SOMETHING IS COMING THROUGH.", 72); S->note_t = 2.2f; sfx(21); }
    else if (k%7==3){ S->ent_ev = 1; S->ent_phase = 1; S->ent_t = 3.0f; S->ent_passes = 1;
        scpy(S->note, "YOU HEAR IT SCREAMING UP THE HALL... HIDE IN A BUSH!", 72); S->note_t = 2.8f; sfx(20); }
    else if (k%11==6){ S->ent_ev = 2; S->ent_phase = 1; S->ent_t = 3.0f; S->ent_zpass = 0; S->ent_passes = 3;
        scpy(S->note, "A SCREAM, BOTH WAYS... IT KEEPS COMING BACK. BUSH!", 72); S->note_t = 2.8f; sfx(20); }
    else if (S->ent_dark){ S->ent_ev = 3; S->ent_phase = 1; S->ent_twait = 1.3f; S->ent_typs = 3; }
    else if (k%8==5){ S->ent_ev = 4;
        S->ent_rvx = ent_room_x0(k) + (2 + (int)(ent_rnd(b, k, 700) % (ENT_STRIDE-4)))*BW_TILE + 24;
        S->ent_rvy = (2 + (int)(ent_rnd(b, k, 710) % (ENT_ROWS-4)))*BW_TILE + 24;
        scpy(S->note, "THE CODE REVIEWER IS IN. DO NOT MEET ITS EYES.", 72); S->note_t = 2.8f; }
    else if (ent_bigbug_room(k)){ S->ent_ev = 6;   // a Big Bug keeps this room too: mind your noise (Daniel, Nov 6)
        S->ent_bbx = ent_room_x0(k) + (3 + (int)(ent_rnd(b, k, 800) % (ENT_STRIDE-6)))*BW_TILE + 24;
        S->ent_bby = ((ent_rnd(b, k, 810)&1)? 2 : ENT_ROWS-3)*BW_TILE + 24;
        S->ent_bbdir = 0; S->ent_bbcharge = 0; S->ent_t = 1.0f;
        scpy(S->note, "SOMETHING ENORMOUS IS SNORING HERE. SOFT FEET.", 72); S->note_t = 2.8f; }
}
static void ent_enter_room(int pi, int k){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    S->ent_room = k;
    ent_room_setup(pi, k);
    if (S->ent_sub){ if (k > b->ent_sub_best) b->ent_sub_best = k; return; }
    if (k > b->ent_best) b->ent_best = k;
    if (k==51 && !S->ent_finished){
        S->ent_finished = 1; b->ent_done++;
        buddy_award_xp(pi, 20);   // v1.19: a completed Entrances run pays the buddy, on top of the quest XP
        scpy(S->note, b->active? "YOU MADE IT OUT! EVERY ENTRANCE, CLEARED! +20 BUDDY XP" : "YOU MADE IT OUT! EVERY ENTRANCE, CLEARED!", 72); S->note_t = 4.5f;
        do_save(); sfx(11);
    }
    else if (k==25){ scpy(S->note, "ENTRANCE 25: THE LIBRARY. SNEAK. FIND 5 FUSES.", 72); S->note_t = 3.6f; }
    else if (k==51){ scpy(S->note, "THE EXIT ROOM. GO HOME PROUD, OR KEEP GOING.", 72); S->note_t = 3.4f; }
    else if (k%10==0){ scpy(S->note, "A SAFE LANDING. THE TENT RESTS AND SAVES.", 72); S->note_t = 3.0f; }
    else if (S->ent_dark && S->ent_ev!=1 && S->ent_ev!=2){ scpy(S->note, "IT IS DARK IN HERE. LISTEN FOR A PSST.", 72); S->note_t = 2.8f; }
}
static void ent_bonk(int pi, float frac, float kx){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->ent_grace>0) return;
    b->hp -= b->maxhp * frac; if (b->hp<0) b->hp = 0;
    b->calm = 0; S->ent_grace = 1.6f;
    if (kx!=0){ float nx = b->x + kx; if (!bw_solid_b(b, nx, b->y)) b->x = nx; }
    sfx(4);
}
static void ent_neutralize(int pi, int via_ctrlz){
    BwSim *S=&BWS[pi];
    if ((S->ent_ev==1 || S->ent_ev==2 || S->ent_ev==7) && S->ent_phase==2){
        int wasev = S->ent_ev; S->ent_ev = 0; S->ent_phase = 0;
        scpy(S->note, wasev==7? "BONK! THE GLITCH SPITS IT BACK OUT." : via_ctrlz? "CTRL-Z! THE ZIP BUG UNDOES ITSELF." : "BONK! THE ZIP BUG SPINS AWAY DIZZY.", 72); S->note_t = 2.2f;
    } else if (S->ent_ev==5 && S->ent_phase==1){
        if (via_ctrlz){ S->ent_chase = 0; S->ent_ev = 0; S->ent_phase = 0;
            scpy(S->note, "CTRL-Z! THE INSPECTOR UNDOES HIMSELF.", 72); S->note_t = 2.4f; }
        else { S->ent_istun = 3.5f; S->ent_ix -= 110;
            scpy(S->note, "BONK! THE INSPECTOR SEES STARS.", 72); S->note_t = 2.2f; }
    } else if (S->ent_ev==3 && S->ent_phase==2){
        S->ent_bits += 2; S->ent_typs--;
        S->ent_phase = S->ent_typs>0? 1 : 0; if (!S->ent_typs) S->ent_ev = 0; S->ent_twait = 1.5f;
        scpy(S->note, "BONK! TYPO CORRECTED. +2 BITS", 72); S->note_t = 2.0f;
    } else if (S->ent_ev==6){
        S->ent_bbcharge = 0; S->ent_noise = 0;
        if (S->ent_room==25 && !S->ent_sub){ S->ent_bbx = ent_room_x0(25) + (ENT_STRIDE/2)*BW_TILE + 24; S->ent_bby = 1.6f*BW_TILE; }
        else { Buddy *b=&G.bud[pi]; int k = S->ent_room;   // a resident Big Bug just shuffles back to its corner
            S->ent_bbx = ent_room_x0(k) + (3 + (int)(ent_rnd(b, k, 800) % (ENT_STRIDE-6)))*BW_TILE + 24;
            S->ent_bby = ((ent_rnd(b, k, 810)&1)? 2 : ENT_ROWS-3)*BW_TILE + 24; }
        scpy(S->note, via_ctrlz? "CTRL-Z! THE BIG BUG BLINKS BACK TO ITS DESK." : "BONK! THE BIG BUG BACKS OFF, HUFFING.", 72); S->note_t = 2.2f;
    }
}
static void ent_update(int pi, float dt, const BwIn *in){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (in->exit_){ bw_exit(pi, 0); return; }
    if (S->ent_grace>0) S->ent_grace -= dt;
    if (b->atk_cd>0) b->atk_cd -= dt;
    // movement (the buddy's own BW walk, hotel tiles)
    float mx=0, my=0; int k2=in->keys;
    if (k2&K_UP) my-=1; if (k2&K_DOWN) my+=1; if (k2&K_LEFT) mx-=1; if (k2&K_RIGHT) mx+=1;
    int moving = (mx!=0 || my!=0);
    if (moving){
        float d=fsqrt(mx*mx+my*my); mx/=d; my/=d;
        b->aim = fatan2(my, mx);
        float sp = BUD_SPD[b->species%BUDN];
        float nx = b->x + mx*sp*dt, ny = b->y + my*sp*dt;
        if (!bw_solid_b(b, nx, b->y)) b->x = nx;
        if (!bw_solid_b(b, b->x, ny)) b->y = ny;
        b->walk_ph += dt*9;
        b->mv_t = 0.15f;
    }
    if (b->lunge_t>0) b->lunge_t-=dt;
    if (b->energy<BUD_ENERGY_MAX){ b->energy += BUD_EN_REGEN*dt; if (b->energy>BUD_ENERGY_MAX) b->energy=BUD_ENERGY_MAX; }
    b->bw_x = b->x; b->bw_y = b->y;
    // breath regen: the hotel is not as kind as the meadow (Daniel,
    // Nov 5 — harder) — six calm seconds, and a slower trickle
    b->calm += dt;
    if (b->calm>6 && b->hp>0 && b->hp<b->maxhp){ b->hp += 1.4f*dt; if (b->hp>b->maxhp) b->hp=b->maxhp; }
    // room tracking
    int rk = ent_room_of(b->x);
    if (rk != S->ent_room) ent_enter_room(pi, rk);
    int room = S->ent_room;
    // ambient dread: now and then the hotel reminds you it is a hotel
    S->ent_tamb -= dt;
    if (S->ent_tamb<=0){
        S->ent_tamb = 9.0f + (float)(ent_rnd(b, room, 2900 + (int)(G.anim_t*3)) % 8);
        int landing = S->ent_sub? (room%20==0) : (room%10==0);
        if (!landing && S->note_t<=0){
            static const char *AMB[4] = { "SOMEWHERE, A DOOR CREAKED.", "THE LAMPS BUZZ. THEN, BRIEFLY, DON'T.",
                                          "SOMETHING SMALL SCUTTLED AWAY.", "BREATHING, SOMEWHERE. PROBABLY YOURS." };
            scpy(S->note, AMB[ent_rnd(b, room, 2950 + (int)(G.anim_t*5)) % 4], 72); S->note_t = 2.4f; sfx(6);
        }
    }
    float rx0 = ent_room_x0(room);
    int hidden = ent_tile(pi, bw_tileof(b->x), bw_tileof(b->y))==3;
    // --- the room's resident trouble ---
    if (S->ent_ev==1 || S->ent_ev==2 || S->ent_ev==7){
        if (S->ent_phase==1){
            S->ent_t -= dt;
            if (S->ent_t<=0){
                S->ent_phase = 2;
                S->ent_zdir = (S->ent_ev==2 && (S->ent_zpass&1))? -1 : 1;
                if (S->ent_ev==7) S->ent_zdir = (ent_rnd(b, room, 3400)&1)? -1 : 1;
                S->ent_zx = S->ent_zdir>0? rx0 + BW_TILE + 10 : rx0 + (ENT_STRIDE-1)*BW_TILE - 10;
                S->ent_zy = ent_doory(b, room);   // the pass rides the door row: that is the run the moths mark
                S->ent_zspd = S->ent_ev==7? 470.0f : S->ent_sub? 440.0f : 400.0f;
                S->ent_zt = 0;
                if (S->ent_ev!=7) sfx(20);   // the glitch brings its own noise
            }
        } else if (S->ent_phase==2){
            float prevzx = S->ent_zx;
            S->ent_zx += S->ent_zdir * S->ent_zspd * dt;
            // the roar (Daniel: LOUDER): a wall of rushing noise that
            // swells as the thing bears down on you
            S->ent_zt -= dt;
            if (S->ent_zt<=0){
                S->ent_zt = 0.09f;
                float adx = b->x - S->ent_zx; if (adx<0) adx=-adx;
                int vel = 92 - (int)(adx*0.07f); if (vel<52) vel=52; if (vel>92) vel=92;
                qnote(0,3,26,vel,3); qnote(0.02f,3,47,vel-25,1);
            }
            // like Rush, it owns the whole room: anywhere not hidden
            // when it sweeps past your position, it gets you (45% —
            // Doors does not do love taps)
            if (!hidden && S->ent_grace<=0 && prevzx != S->ent_zx && (prevzx - b->x)*(S->ent_zx - b->x) <= 0.0f){
                ent_bonk(pi, 0.45f, -S->ent_zdir*26);
                scpy(S->note, S->ent_sub? "IT GOT YOU! VENTS ARE FOR HIDING." : "THE ZIP BUG GOT YOU! BUSHES ARE FOR HIDING.", 72); S->note_t = 2.4f;
            }
            if (S->ent_zx < rx0 + 10 || S->ent_zx > rx0 + ENT_STRIDE*BW_TILE - 10){
                S->ent_zpass++;
                if (S->ent_zpass>=S->ent_passes){ int wasev = S->ent_ev; S->ent_ev = 0; S->ent_phase = 0;
                    if (wasev==2){ scpy(S->note, S->ent_sub? "THE MULTIBUG PEELS AWAY. FOR NOW." : "THE BOOMERANG BUG GIVES UP AT LAST.", 72); S->note_t = 2.2f; } }
                else { S->ent_phase = 1; S->ent_t = 2.0f; sfx(20); }   // every return pass gets a real warning too (Daniel, Nov 6)
            }
        }
    }
    else if (S->ent_ev==3){
        if (S->ent_phase==1){
            S->ent_twait -= dt;
            if (S->ent_twait<=0){
                u32 h = ent_rnd(b, room, 500 + S->ent_typs*17 + (int)(G.anim_t*7));
                S->ent_tyx = rx0 + (1 + (int)(h % (ENT_STRIDE-2)))*BW_TILE + 24;
                S->ent_tyy = (1 + (int)((h>>4) % (ENT_ROWS-2)))*BW_TILE + 24;
                S->ent_tt = 2.4f; S->ent_phase = 2; sfx(6);
                scpy(S->note, "psst... GO SAY HI BEFORE IT NIBBLES.", 72); S->note_t = 1.8f;
            }
        } else if (S->ent_phase==2){
            S->ent_tt -= dt;
            float dx=b->x-S->ent_tyx, dy=b->y-S->ent_tyy;
            if (dx*dx+dy*dy < 24*24){
                S->ent_bits += 2; S->ent_typs--; sfx(19);
                scpy(S->note, "YOU FOUND THE TYPO! +2 BITS", 72); S->note_t = 1.8f;
                S->ent_phase = S->ent_typs>0? 1 : 0; if (!S->ent_typs) S->ent_ev = 0; S->ent_twait = 1.5f;
            } else if (S->ent_tt<=0){
                ent_bonk(pi, 0.10f, 0);
                scpy(S->note, "THE TYPO NIBBLED YOU. TOO SLOW!", 72); S->note_t = 2.0f;
                S->ent_typs--;
                S->ent_phase = S->ent_typs>0? 1 : 0; if (!S->ent_typs) S->ent_ev = 0; S->ent_twait = 1.5f;
            }
        }
    }
    else if (S->ent_ev==4){
        float dx=S->ent_rvx-b->x, dy=S->ent_rvy-b->y; float d2=dx*dx+dy*dy;
        if (d2 < 180*180 && d2 > 1){
            float dl = fsqrt(d2);
            if ((fcos(b->aim)*dx + fsin(b->aim)*dy)/dl > 0.55f){
                b->hp -= b->maxhp * 0.16f * dt; if (b->hp<0) b->hp = 0; b->calm = 0;
            }
        }
    }
    else if (S->ent_ev==5 && S->ent_chase){
        if (S->ent_phase==0){
            if (b->x > rx0 + (ENT_STRIDE/2)*BW_TILE){
                S->ent_phase = 1;
                S->ent_ix = rx0 + BW_TILE + 14; S->ent_iy = ent_doory(b, S->ent_chase - 1);
                scpy(S->note, "THE INSPECTOR! RUN!", 72); S->note_t = 2.2f; sfx(16);
            }
        } else {
            // doors in a chase just fly open — no stopping for keys
            float ddx = (rx0 + (ENT_STRIDE-1)*BW_TILE + 24) - b->x, ddy = ent_doory(b, room) - b->y;
            if (ddx*ddx+ddy*ddy < 52*52 && room > S->ent_door_upto){ S->ent_door_upto = room; sfx(19); }
            if (room >= S->ent_chase+5){
                S->ent_chase = 0; S->ent_ev = 0; S->ent_phase = 0; S->ent_bits += 10;
                scpy(S->note, "YOU LOST THE INSPECTOR! +10 BITS", 72); S->note_t = 2.4f; sfx(11);
            } else {
                // the chase has a sound: a low pulse that keeps
                // missing the beat, and something far off screaming
                // about it (Daniel, Nov 6: scary music)
                S->ent_mt -= dt;
                if (S->ent_mt<=0){
                    S->ent_mt = 0.21f;
                    static const int CHN[8] = { 38, 38, 39, 38, 38, 41, 39, 36 };
                    int cn = CHN[S->ent_mstep & 7]; S->ent_mstep++;
                    qnote(0, 3, cn, 68, 2); qnote(0, 3, cn-12, 42, 3);
                    if ((S->ent_mstep & 7)==5) qnote(0.03f, 3, 87, 24, 0);
                }
                // floor fires: his rooms, his rules — step wide
                for (int i=0;i<S->ent_fn;i++){
                    float fdx=b->x-S->ent_fx[i], fdy=b->y-S->ent_fy[i];
                    if (fdx*fdx+fdy*fdy < 17*17 && S->ent_grace<=0){
                        ent_bonk(pi, 0.12f, 0);
                        scpy(S->note, "FIRE ON THE FLOOR! STEP WIDE!", 72); S->note_t = 2.0f;
                    }
                }
                if (S->ent_istun>0) S->ent_istun -= dt;
                else {
                    // he comes THROUGH the doorways (Daniel, Nov 6):
                    // in another room, he makes for the door tile
                    // between you first. His position lives in the
                    // world, never in a room setup, so crossing a
                    // threshold no longer vanishes and re-forms him —
                    // he walks through, in plain sight. A smidge
                    // faster than the buddy (1.10x), and quicker
                    // still while he is a room behind (1.18x more),
                    // so the pressure never lets up. Run.
                    float tx2 = b->x, ty2 = b->y;
                    int ik = ent_room_of(S->ent_ix), bk2 = ent_room_of(b->x);
                    if (ik < bk2){ tx2 = ent_room_x0(ik) + (ENT_STRIDE-1)*BW_TILE + 24; ty2 = ent_doory(b, ik); }
                    else if (ik > bk2){ tx2 = ent_room_x0(ik) + 24; ty2 = ent_doory(b, ik-1); }
                    float isp = BUD_SPD[b->species%BUDN] * 1.10f;
                    if (ik!=bk2) isp *= 1.18f;
                    float dx=tx2-S->ent_ix, dy=ty2-S->ent_iy; float dl=fsqrt(dx*dx+dy*dy);
                    if (dl>1){
                        float nx = S->ent_ix + dx/dl*isp*dt, ny = S->ent_iy + dy/dl*isp*dt;
                        if (!ent_solid_insp(pi, nx, S->ent_iy)) S->ent_ix = nx;
                        if (!ent_solid_insp(pi, S->ent_ix, ny)) S->ent_iy = ny;
                    }
                    float cx=b->x-S->ent_ix, cy=b->y-S->ent_iy;
                    if (cx*cx+cy*cy < 22*22 && S->ent_grace<=0){
                        ent_bonk(pi, 0.40f, 20);
                        scpy(S->note, "CAUGHT! THE INSPECTOR CONFISCATES SOME HP.", 72); S->note_t = 2.2f;
                        int back = room-1; if (back < S->ent_chase) back = S->ent_chase;
                        S->ent_ix = ent_room_x0(back) + BW_TILE + 14; S->ent_iy = ent_doory(b, back-1);
                        S->ent_istun = 2.2f;
                    }
                }
            }
        }
    }
    else if (S->ent_ev==6){
        // fuses first: walk over one to pocket it (the Library only —
        // a resident Big Bug elsewhere has no fuses to guard)
        if (room==25 && !S->ent_libdone){
            for (int i=0;i<5;i++){
                if (S->ent_fuse_mask>>i & 1) continue;
                float fx = ent_room_x0(25) + ENT_FUSE_TX[i]*BW_TILE + 24, fy = ENT_FUSE_TY[i]*BW_TILE + 24;
                float dx=b->x-fx, dy=b->y-fy;
                if (dx*dx+dy*dy < 24*24){
                    S->ent_fuse_mask |= 1<<i;
                    int n=0; for (int j=0;j<5;j++) if (S->ent_fuse_mask>>j & 1) n++;
                    if (n>=5){ S->ent_libdone = 1; scpy(S->note, "ALL 5 FUSES! THE LIBRARY DOOR CLICKS OPEN.", 72); S->note_t = 3.2f; sfx(11); }
                    else { char *q=S->note; q=sappend(q, "FUSE "); q=sapp_int(q, n); q=sappend(q, " OF 5."); *q=0; S->note_t = 1.8f; sfx(19); }
                }
            }
        }
        // the Big Bug: blind, but it hears footsteps
        float bdx = b->x - S->ent_bbx, bdy = b->y - S->ent_bby; float bd = fsqrt(bdx*bdx + bdy*bdy);
        if (S->ent_bbcharge>0){
            S->ent_bbcharge -= dt;
            float nx = S->ent_bbx + fcos(S->ent_bbdir)*235*dt, ny = S->ent_bby + fsin(S->ent_bbdir)*235*dt;
            if (!bw_solid_b(b, nx, S->ent_bby)) S->ent_bbx = nx;
            if (!bw_solid_b(b, S->ent_bbx, ny)) S->ent_bby = ny;
            if (bd < 27 && S->ent_grace<=0){
                ent_bonk(pi, 0.40f, 0);
                scpy(S->note, "THE BIG BUG SAT ON YOU. MOVE QUIETLY!", 72); S->note_t = 2.2f;
                S->ent_bbcharge = 0; S->ent_noise = 0;
            }
        } else {
            S->ent_t -= dt;
            if (S->ent_t<=0){ S->ent_t = 1.3f + (float)(ent_rnd(b, room, 800 + (int)(G.anim_t*3)) % 10) * 0.13f;
                S->ent_bbdir = (float)(ent_rnd(b, room, 810 + (int)(G.anim_t*5)) % 628) * 0.01f; }
            float nx = S->ent_bbx + fcos(S->ent_bbdir)*27*dt, ny = S->ent_bby + fsin(S->ent_bbdir)*27*dt;
            if (!bw_solid_b(b, nx, S->ent_bby)) S->ent_bbx = nx; else S->ent_bbdir += 1.7f;
            if (!bw_solid_b(b, S->ent_bbx, ny)) S->ent_bby = ny; else S->ent_bbdir += 1.7f;
            if (bd < 27 && S->ent_grace<=0){   // like Figure, it does not need to be charging to end you
                ent_bonk(pi, 0.40f, 0);
                scpy(S->note, "YOU BUMPED INTO THE BIG BUG. IT NOTICED.", 72); S->note_t = 2.2f;
                S->ent_noise = 0;
            }
            if (moving && bd < 160) S->ent_noise += 58*dt; else S->ent_noise -= 26*dt;
            if (S->ent_noise<0) S->ent_noise = 0;
            if (S->ent_noise>=100){
                S->ent_noise = 0; S->ent_bbcharge = 0.8f; S->ent_bbdir = fatan2(bdy, bdx);
                scpy(S->note, "IT HEARD YOU!", 72); S->note_t = 1.6f; sfx(16);
            }
        }
    }
    // --- hands: swing, patch, open, search ---
    if (in->sel && b->atk_cd<=0){
        b->atk_cd = 0.5f;
        int intarget = 0;
        if ((S->ent_ev==1 || S->ent_ev==2 || S->ent_ev==7) && S->ent_phase==2){ float dx=b->x-S->ent_zx, dy=b->y-S->ent_zy; intarget = dx*dx+dy*dy < 58*58; }
        else if (S->ent_ev==5 && S->ent_phase==1){ float dx=b->x-S->ent_ix, dy=b->y-S->ent_iy; intarget = dx*dx+dy*dy < 58*58; }
        else if (S->ent_ev==3 && S->ent_phase==2){ float dx=b->x-S->ent_tyx, dy=b->y-S->ent_tyy; intarget = dx*dx+dy*dy < 58*58; }
        else if (S->ent_ev==6){ float dx=b->x-S->ent_bbx, dy=b->y-S->ent_bby; intarget = dx*dx+dy*dy < 58*58; }
        if (intarget){
            if (S->ent_sticks>0){ S->ent_sticks--; sfx(4); ent_neutralize(pi, 0); }
            else if (b->items[3]>0){ b->items[3]--; sfx(11); ent_neutralize(pi, 1); }
            else { scpy(S->note, "NOTHING TO SWING. FIND A STICK IN A DRAWER!", 72); S->note_t = 2.0f; sfx(16); }
        }
    }
    if (in->wall && b->atk_cd<=0){
        b->atk_cd = 0.5f;
        if (b->items[0]>0 && b->hp < b->maxhp){ b->items[0]--; b->hp += b->maxhp*0.4f; if (b->hp>b->maxhp) b->hp=b->maxhp;
            scpy(S->note, "PATCH NOTES. MUCH BETTER.", 72); S->note_t = 1.8f; sfx(11); }
        else if (b->items[1]>0 && b->energy < BUD_ENERGY_MAX){ b->items[1]--; b->energy = BUD_ENERGY_MAX;
            scpy(S->note, "ENERGY DRINK! ZOOM.", 72); S->note_t = 1.8f; sfx(11); }
        else if (b->items[0]>0 || b->items[1]>0){ scpy(S->note, "ALL TOPPED UP ALREADY.", 72); S->note_t = 1.6f; }
        else { scpy(S->note, "NO SUPPLIES. CHECK THE DRAWERS.", 72); S->note_t = 1.8f; sfx(16); }
    }
    if (in->interact){
        float doorx = rx0 + (ENT_STRIDE-1)*BW_TILE + 24, doory = ent_doory(b, room);
        float dx=b->x-doorx, dy=b->y-doory;
        int upto = S->ent_sub? S->ent_door_sub : S->ent_door_upto;
        if (dx*dx+dy*dy < 54*54 && room > upto){
            if (!S->ent_sub && room==25 && !S->ent_libdone){ scpy(S->note, "IT NEEDS 5 FUSES BEFORE IT BUDGES. SHHH.", 72); S->note_t = 2.4f; sfx(16); return; }
            if (ent_door_locked(pi, room)){
                if (S->ent_key){ S->ent_key = 0; if (S->ent_sub) S->ent_door_sub = room; else S->ent_door_upto = room; scpy(S->note, "UNLOCKED!", 72); S->note_t = 1.6f; sfx(19); }
                else { scpy(S->note, "LOCKED. THE KEY IS HIDING IN THIS ROOM.", 72); S->note_t = 2.4f; sfx(16); }
                return;
            }
            if (S->ent_sub) S->ent_door_sub = room; else S->ent_door_upto = room; sfx(19); return;
        }
        if (!S->ent_sub && !S->ent_dupe_used && ent_dupe_room(b, room)){
            for (int di=0; di<ent_dupe_n(b, room); di++){
                float fx = doorx, fy = ent_dupe_ly_i(b, room, di)*BW_TILE + 24;
                float ddx=b->x-fx, ddy=b->y-fy;
                if (ddx*ddx+ddy*ddy < 54*54){
                    S->ent_dupe_used = 1; S->ent_grace = 0;
                    ent_bonk(pi, 0.12f, -20);
                    scpy(S->note, "DUPE! THAT DOOR WAS PAINTED ON. RUDE.", 72); S->note_t = 2.6f; sfx(16);
                    return;
                }
            }
        }
        if (room==1){
            // the west door of room 1 (the stairs home, or B-1's
            // stairwell) sits on its own fixed middle row (tile 8,
            // ENT_DOOR_LY) — NOT on room 1's east door row, which
            // wanders per room since batch C. Measuring from the east
            // row stranded the climb in B-1 whenever the east row was
            // off-centre (Nov 6: the buddy could press himself against
            // the stairs forever and never come up).
            float mdx=b->x-24, mdy=b->y-ENT_DOORY;
            if (mdx*mdx+mdy*mdy < 54*54){
                if (S->ent_sub){   // B-1's west door is a stairwell, not the mall
                    S->ent_sub = 0; S->ent_key = 0;
                    b->x = ent_room_x0(25) + (ENT_STRIDE-3)*BW_TILE + 24; b->y = (ENT_ROWS-2)*BW_TILE + 24; b->bw_x = b->x; b->bw_y = b->y;
                    scpy(S->note, "UP THE STAIRS. THE LIBRARY AGAIN. IT MISSED YOU.", 72); S->note_t = 2.8f; sfx(12);
                    return;
                }
                bw_exit(pi, 0); return;
            }
        }
        if (!S->ent_sub && room==51){
            float ex = ent_room_x0(51) + (ENT_STRIDE/2)*BW_TILE + 24, ey = (ENT_ROWS-1)*BW_TILE + 24;
            float edx=b->x-ex, edy=b->y-ey;
            if (edx*edx+edy*edy < 58*58){ bw_exit(pi, 0); return; }
        }
        if (!S->ent_sub && room==25){   // the tall bookcase, and what is behind it
            float bx2 = ent_room_x0(25) + (ENT_STRIDE-2)*BW_TILE + 24, by2 = (ENT_ROWS-2)*BW_TILE + 24;
            float ddx=b->x-bx2, ddy=b->y-by2;
            if (ddx*ddx+ddy*ddy < 58*58){
                if (!S->ent_libdone){ scpy(S->note, "A TALL BOOKCASE. IT WILL NOT BUDGE. THE ROOM FEELS UNFINISHED.", 72); S->note_t = 2.4f; sfx(16); return; }
                S->ent_sub = 1; S->ent_door_sub = 0; S->ent_key = 0;
                b->x = 2*BW_TILE + 24; b->y = ENT_DOORY; b->bw_x = b->x; b->bw_y = b->y;
                scpy(S->note, "THE BOOKCASE SLIDES! A STAIRWELL. DOWN YOU GO.", 72); S->note_t = 3.2f; sfx(12);
                return;
            }
        }
        // drawers: search the room's furniture for one in reach
        int done_int = 0;
        for (int lyy=1; lyy<=ENT_ROWS-2 && !done_int; lyy++) for (int lxx=1; lxx<=ENT_STRIDE-2 && !done_int; lxx++){
            int tx = (room-1)*ENT_STRIDE + lxx, ty = lyy;
            if (ent_furn(pi, tx, ty)==6){
                float txx = tx*BW_TILE + 24, tyy = ty*BW_TILE + 24;
                float ddx=b->x-txx, ddy=b->y-tyy;
                if (ddx*ddx+ddy*ddy < 50*50){ S->sleep_t = 1.4f; S->slept = 0; sfx(11); done_int = 1; }
            } else if (ent_tile(pi, tx, ty)==4){
                float txx = tx*BW_TILE + 24, tyy = ty*BW_TILE + 24;
                float ddx=b->x-txx, ddy=b->y-tyy;
                if (ddx*ddx+ddy*ddy < 48*48){
                    done_int = 1;
                    int lk = S->ent_sub? room+1000 : room;   // downstairs loot is its own ledger
                    if (ent_room_looted(S, lk)){ scpy(S->note, "JUST DUST. AND A BUTTON.", 72); S->note_t = 1.6f; }
                    else {
                        if (S->ent_loot_n < 96) S->ent_loot_rooms[S->ent_loot_n++] = lk;
                        int roll = (int)(ent_rnd(b, room, 600 + S->ent_loot_n) % 100);
                        if (roll < 55){ int nb2 = 3 + (int)(ent_rnd(b, room, 610) % 6); S->ent_bits += nb2;
                            char *q=S->note; q=sappend(q, "BITS IN THE DRAWER! +"); q=sapp_int(q, nb2); *q=0; S->note_t = 1.8f; }
                        else if (roll < 75){ S->ent_sticks++; { char *q=S->note; q=sappend(q, "A GOOD STICK! "); q=sappend(q, lbl_fire()); q=sappend(q, " SWINGS IT."); *q=0; } S->note_t = 2.0f; }
                        else if (roll < 88){ b->items[0]++; scpy(S->note, "PATCH NOTES IN THE DRAWER!", 72); S->note_t = 1.8f; }
                        else if (roll < 95){ b->items[1]++; scpy(S->note, "AN ENERGY DRINK, STILL FIZZY!", 72); S->note_t = 1.8f; }
                        else if (S->ent_lant < 2){ S->ent_lant++; scpy(S->note, "A BRIGHTER LANTERN!", 72); S->note_t = 2.0f; }
                        else { S->ent_bits += 5; scpy(S->note, "A VERY SHINY BUTTON. +5 BITS", 72); S->note_t = 1.8f; }
                        sfx(11);
                    }
                }
            }
        }
        if (done_int) return;
    }
    // the brass key just sits there waiting to be pocketed
    if (!S->ent_key && room > (S->ent_sub? S->ent_door_sub : S->ent_door_upto) && ent_door_locked(pi, room)){
        float kx, ky; ent_key_pos(pi, room, &kx, &ky);
        float dx=b->x-kx, dy=b->y-ky;
        if (dx*dx+dy*dy < 26*26){ S->ent_key = 1; scpy(S->note, "GOT THE BRASS KEY!", 72); S->note_t = 2.0f; sfx(19); }
    }
    if (b->hp<=0){ b->hp = 0; b->fainted = 1; bw_exit(pi, 1); return; }
}


// --- ENTRANCES drawing ---
static void ent_draw_door(int x, int y, const char *plate, int fake){
    rect_fill(x+6, y+2, BW_TILE-12, BW_TILE-2, COL(104,72,40));
    rect_line(x+6, y+2, BW_TILE-12, BW_TILE-2, COL(56,36,20));
    rect_fill(x+10, y+20, BW_TILE-20, 3, COL(84,56,30));
    disc_fill(x+BW_TILE-13, y+26, 2, COL(230,196,90));
    rect_fill(x+13, y+6, BW_TILE-26, 11, COL(198,164,84));
    draw_text_center(x+BW_TILE/2, y+8, plate, 1, COL(40,30,10));
    // painted doors used to wear a red plate and a crack — a free
    // tell. Now a dupe is drawn exactly like the real thing (Daniel,
    // Nov 6: less obvious); only walking into it gives it away.
    (void)fake;
}
static void bw_render_ent(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    G.camx=b->x; G.camy=b->y;
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    rect_fill(vx, vy, vw2, vh2, COL(16,12,20));
    int tx0=bw_tileof(b->x - vw2/2 - BW_TILE), tx1=bw_tileof(b->x + vw2/2 + BW_TILE);
    int ty0=bw_tileof(b->y - vh2/2 - BW_TILE), ty1=bw_tileof(b->y + vh2/2 + BW_TILE);
    // tiles + floor-level dressing
    for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
        int x=SX(tx*BW_TILE), y=SY(ty*BW_TILE);
        if (x+ BW_TILE<vx || x>vx+vw2 || y+BW_TILE<vy || y>vy+vh2) continue;
        int t = ent_tile(pi, tx, ty);
        int k = tx>=0? tx/ENT_STRIDE + 1 : 1, lx = tx>=0? tx%ENT_STRIDE : 0;
        u32 h = hash2(tx, ty, (u32)b->bw_seed ^ 0x7A55u);
        if (t==1){ if (S->ent_sub){ rect_fill(x, y, BW_TILE+1, BW_TILE+1, COL(50,50,56)); rect_fill(x, y, BW_TILE+1, 7, COL(72,72,78)); rect_fill(x, y+BW_TILE-5, BW_TILE+1, 5, COL(36,36,40)); }
                   else { rect_fill(x, y, BW_TILE+1, BW_TILE+1, COL(47,37,54)); rect_fill(x, y, BW_TILE+1, 7, COL(68,54,72)); rect_fill(x, y+BW_TILE-5, BW_TILE+1, 5, COL(33,26,39)); }
                   // a panel seam, wandering per tile (glow-up)
                   rect_fill(x + 6 + (int)((h>>8)%28), y+8, 1, BW_TILE-14, S->ent_sub? COL(41,41,45) : COL(37,28,43));
                   continue; }
        // floor: lamplit wood, a warmer checker on landings, confetti past the end
        { u32 base = S->ent_sub? (((tx+ty)&1)==0? COL(72,72,68) : COL(65,65,62))
                              : (k%10==0)? (((tx+ty)&1)==0? COL(100,76,47) : COL(92,70,42))
                              : (((tx+ty)&1)==0? COL(84,62,41) : COL(77,57,37));
          if (!S->ent_sub && k==25) base = ((tx+ty)&1)==0? COL(80,55,45) : COL(74,50,42);
          rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(base, 0.94f + (float)((h>>20)%12)*0.01f));
          rect_fill(x, y+BW_TILE-2, BW_TILE+1, 2, COL(70,52,36));
          // a welcome mat in front of every door (glow-up): the hotel
          // is spooky, not rude
          if (t==0 && !S->ent_sub){
              int nd = ent_tile(pi, tx+1, ty)==2 || ent_tile(pi, tx-1, ty)==2 || ent_tile(pi, tx, ty+1)==2 || ent_tile(pi, tx, ty-1)==2
                    || ent_tile(pi, tx+1, ty)==7 || ent_tile(pi, tx-1, ty)==7 || ent_tile(pi, tx, ty+1)==8 || ent_tile(pi, tx, ty-1)==8;
              if (nd){ rect_fill(x+11, y+31, 26, 11, COL(112,44,44)); rect_line(x+11, y+31, 26, 11, COL(84,32,32)); }
          } }
        if (k==51){ static const u32 CONFC[4] = { COL(255,120,150), COL(120,220,255), COL(255,214,64), COL(150,255,150) };
            for (int c2=0;c2<3;c2++){ u32 hh = hash2(tx*3+c2, ty*5-c2, 0xC04Fu); pset(x + (int)(hh%40) + 4, y + (int)((hh>>6)%40) + 4, CONFC[hh%4]); } }
        if (t==2 || t==7 || t==8){
            int door = (lx==ENT_STRIDE-1)? k : k-1;
            char plate[8];
            if (t==8) scpy(plate, S->ent_sub? "UP" : "OUT", 8);
            else if (t==7) scpy(plate, "EXIT", 8);
            else if (!S->ent_sub && door+1==51) scpy(plate, "EXIT", 8);
            else if (S->ent_sub){ char *q=plate; q=sappend(q, "B"); q=sapp_int(q, door+1); *q=0; }
            else { char *q=plate; q=sapp_int(q, door+1); *q=0; }
            int fake = (t==2 && lx==ENT_STRIDE-1 && !S->ent_sub && ent_dupe_room(b, k) && (ty==ent_dupe_ly_i(b, k, 0) || (ent_dupe_n(b, k)>1 && ty==ent_dupe_ly_i(b, k, 1))));
            ent_draw_door(x, y, plate, fake);
            if (t==2 && lx==ENT_STRIDE-1 && door>=1 && ty==ent_door_ly(b, door) && !S->ent_dupe_used){
                // the guiding moths hover at the REAL door — the hotel
                // is a kind place, if you look
                for (int m2=0;m2<2;m2++){ float ph = G.anim_t*2.2f + m2*2.4f;
                    int mx2 = x + 24 + (int)(fsin(ph)*13), my2 = y + 8 + (int)(fcos(ph*1.3f)*4);
                    disc_fill(mx2, my2, 2, COL(255,246,190)); }
            }
            continue;
        }
        if (t==3){ if (S->ent_sub){ rect_fill(x+6, y+10, BW_TILE-12, 28, COL(38,40,44)); rect_line(x+6, y+10, BW_TILE-12, 28, COL(90,94,100));
                for (int s2=0;s2<4;s2++) rect_fill(x+9, y+14+s2*6, BW_TILE-18, 3, COL(16,17,20)); continue; }
            int bx2 = x+24, by2 = y+26;
            disc_fill(bx2-6, by2+3, 6, COL(44,102,42)); disc_fill(bx2+6, by2+3, 6, COL(44,102,42));
            disc_fill(bx2, by2-2, 7, COL(52,116,48)); disc_fill(bx2-2, by2-5, 3, COL(86,150,70)); continue; }
        if (t==4){ int looted = ent_room_looted(S, S->ent_sub? k+1000 : k);
            rect_fill(x+7, y+12, BW_TILE-14, 24, S->ent_sub? COL(96,92,86) : COL(122,88,52)); rect_line(x+7, y+12, BW_TILE-14, 24, S->ent_sub? COL(52,50,46) : COL(70,48,26));
            rect_fill(x+10, y+ (looted? 10 : 15), BW_TILE-20, 4, S->ent_sub? COL(70,68,64) : COL(96,66,36));
            disc_fill(x+24, y+26, 2, COL(230,196,90)); continue; }
        if (t==5){ if (S->ent_sub){ rect_fill(x+4, y+2, BW_TILE-8, BW_TILE-2, COL(52,54,58));
                for (int p2=0;p2<3;p2++){ rect_fill(x+10+p2*12, y+4, 5, BW_TILE-8, COL(96,100,106)); rect_fill(x+10+p2*12, y+16, 5, 3, COL(60,62,66)); } continue; }
            rect_fill(x+3, y+2, BW_TILE-6, BW_TILE-2, COL(96,64,40)); rect_line(x+3, y+2, BW_TILE-6, BW_TILE-2, COL(56,36,20));
            for (int bk=0;bk<4;bk++){ u32 hh = hash2(tx*9+bk, ty*7, 0xB00Cu);
                rect_fill(x+8+bk*9, y+8 + (int)(hh%12), 5, 16, (hh&1)? COL(150,70,60) : COL(70,110,150)); } continue; }
        if (t==6){ bw_draw_tent(tx*BW_TILE + 24, ty*BW_TILE + 24); continue; }
        if (t==9){ rect_fill(x+3, y+1, BW_TILE-6, BW_TILE-1, COL(74,50,30)); rect_line(x+3, y+1, BW_TILE-6, BW_TILE-1, COL(40,26,14));
            for (int bk=0;bk<5;bk++){ u32 hh = hash2(tx*7+bk, ty*3, 0xB00Cu);
                rect_fill(x+7+bk*7, y+6 + (int)(hh%8), 4, 18, (hh&1)? COL(150,70,60) : COL(70,110,150)); }
            for (int bk=0;bk<5;bk++){ u32 hh = hash2(tx*11+bk, ty*5, 0xB00Du);
                rect_fill(x+7+bk*7, y+28 + (int)(hh%6), 4, 15, (hh&1)? COL(90,130,80) : COL(160,120,60)); } continue; }
    }
    int room = S->ent_room;
    // the key, waiting on its drawer
    if (!S->ent_key && room > (S->ent_sub? S->ent_door_sub : S->ent_door_upto) && ent_door_locked(pi, room)){
        float kx, ky; ent_key_pos(pi, room, &kx, &ky);
        int x=SX(kx), y=SY(ky) + (int)(fsin(G.anim_t*3)*2);
        disc_fill(x, y, 4, COL(255,214,64)); disc_fill(x, y, 2, COL(255,240,170));
        rect_fill(x+3, y-1, 7, 2, COL(255,214,64));
    }
    // library fuses
    if (room==25 && S->ent_ev==6){
        for (int i=0;i<5;i++){
            if (S->ent_fuse_mask>>i & 1) continue;
            int x=SX(ent_room_x0(25) + ENT_FUSE_TX[i]*BW_TILE + 24), y=SY(ENT_FUSE_TY[i]*BW_TILE + 24);
            disc_fill(x, y, 6, COL(60,120,60)); rect_fill(x-3, y-6, 6, 12, COL(120,255,140)); disc_fill(x, y-8, 2, COL(220,255,220));
        }
    }
    // chase fires, licking at the carpet (only ever in Inspector rooms)
    if (S->ent_ev==5){
        for (int i=0;i<S->ent_fn;i++){
            int x=SX(S->ent_fx[i]), y=SY(S->ent_fy[i]);
            int fl = (int)(fsin(G.anim_t*11 + i*1.7f)*2);
            glow_ellipse(x, y+2, 21, 13, COL(255,120,40), 44);
            ellipse_fill(x, y+4, 13, 6, COL(110,38,14));
            line_thick(x-6, y+5, x-1, y-9-fl, 3, COL(255,110,30));
            line_thick(x-1, y-9-fl, x+3, y+5, 3, COL(255,110,30));
            line_thick(x+3, y+5, x+6, y-5+fl, 3, COL(255,165,55));
            disc_fill(x, y-1, 3, COL(255,224,130));
        }
    }
    // residents, split around the buddy by depth
    for (int pass=0; pass<2; pass++){
        if (pass==0){
            // (props already sit in the tile pass; entities sort by y)
        }
        // the Zip Bug: a hacker at full sprint, with speed lines
        if ((S->ent_ev==1 || S->ent_ev==2 || S->ent_ev==7) && S->ent_phase==2 && ((S->ent_zy < b->y) == (pass==0))){
            bw_draw_hacker(S->ent_zx, S->ent_zy, 3, 0, 0);
            if (S->ent_sub && S->ent_ev==2){   // the Multibug is a crowd pretending to be one bug
                bw_draw_hacker(S->ent_zx - S->ent_zdir*26, S->ent_zy - 15, 4, 0, 0);
                bw_draw_hacker(S->ent_zx - S->ent_zdir*20, S->ent_zy + 13, 2, 0, 0);
            }
            int zx=SX(S->ent_zx), zy=SY(S->ent_zy);
            for (int l2=1;l2<4;l2++) line_thick(zx - (int)S->ent_zdir*(14+l2*7), zy-4+l2*3, zx - (int)S->ent_zdir*(20+l2*7), zy-4+l2*3, 1, COL(220,230,240));
        }
        if (S->ent_ev==3 && S->ent_phase==2 && ((S->ent_tyy < b->y) == (pass==0))){
            int x=SX(S->ent_tyx), y=SY(S->ent_tyy);
            disc_fill(x, y, 6, COL(196,120,210));
            glow_ellipse(x, y-2, 8, 5, COL(80,160,255), 70);
            disc_fill(x-2, y-2, 2, COL(150,220,255)); disc_fill(x+2, y-2, 2, COL(150,220,255));
            draw_text_center(x, y-20, "psst", 1, COL(240,220,255));
        }
        if (S->ent_ev==4 && ((S->ent_rvy < b->y) == (pass==0))){
            int x=SX(S->ent_rvx), y=SY(S->ent_rvy) + (int)(fsin(G.anim_t*2)*2);
            disc_fill(x, y, 9, COL(240,240,235)); disc_fill(x-8, y-6, 5, COL(240,240,235)); disc_fill(x+8, y-6, 5, COL(240,240,235));
            disc_fill(x, y, 4, COL(90,170,255)); disc_fill(x-8, y-6, 3, COL(90,170,255)); disc_fill(x+8, y-6, 3, COL(90,170,255));
            disc_fill(x, y, 3, COL(30,30,60)); disc_fill(x-8, y-6, 2, COL(30,30,60)); disc_fill(x+8, y-6, 2, COL(30,30,60));
        }
        if (S->ent_ev==5 && S->ent_phase==1 && ((S->ent_iy < b->y) == (pass==0))){
            bw_draw_big_hacker(S->ent_ix, S->ent_iy, 0, 0);
            if (S->ent_istun>0) draw_text_center(SX(S->ent_ix), SY(S->ent_iy)-40, "* * *", 1, COL(255,230,120));
            // guiding moths light the way to the next door
            for (int m2=0;m2<3;m2++){ float fx2 = b->x + 60 + m2*46 + fsin(G.anim_t*3 + m2)*6;
                disc_fill(SX(fx2), SY(ent_doory(b, room)) - 14 + (int)(fsin(G.anim_t*4 + m2*2)*3), 2, COL(255,246,190)); }
        }
        if (S->ent_ev==6 && ((S->ent_bby < b->y) == (pass==0))){
            bw_draw_big_hacker(S->ent_bbx, S->ent_bby, 3, 0);
            if (S->ent_bbcharge>0) draw_text_center(SX(S->ent_bbx), SY(S->ent_bby)-40, "!!", 2, COL(255,92,80));
        }
        if (pass==0) draw_buddy(b);
    }
    ui_vignette(vx, vy, vw2, vh2);
    // the Zip Bug warning: the lamps flicker
    if ((S->ent_ev==1 || S->ent_ev==2) && S->ent_phase==1 && ((int)(G.anim_t*9)%2))
        blend_rect(vx, vy, vw2, vh2, COL(255,236,170), 22);
    // the whole hotel is dim now (Daniel, Nov 5 — darker); Typo rooms
    // and the Library are properly DARK, landings keep their lamplight,
    // and every lamp breathes a little — worse when something screams
    {
        int bx=SX(b->x), by=SY(b->y);
        int dark = S->ent_dark || (!S->ent_sub && room==25);
        int landing = S->ent_sub? (room%20==0) : (room%10==0);
        float fl = 1.0f + 0.045f*fsin(G.anim_t*13.7f)*fsin(G.anim_t*7.9f+1.3f);
        if ((S->ent_ev==1 || S->ent_ev==2 || S->ent_ev==7) && S->ent_phase==1) fl *= ((int)(G.anim_t*11)%2)? 0.70f : 1.12f;
        int rin, amax;
        if (landing){ rin = 148; amax = 92; }
        else if (dark){ rin = 56; amax = 208; }
        else if (S->ent_sub){ rin = 102; amax = 152; }
        else { rin = 118; amax = 130; }
        rin = (int)((rin + S->ent_lant*36) * fl); if (rin<28) rin=28;
        int rout = rin + (dark? 74 : 96);
        for (int cy=vy; cy<vy+vh2; cy+=16) for (int cx=vx; cx<vx+vw2; cx+=16){
            int ddx = cx+8-bx, ddy = cy+8-by; int d = (int)fsqrt((float)(ddx*ddx + ddy*ddy));
            int a = d<=rin? 0 : d>=rout? amax : (d-rin)*amax/(rout-rin);
            if (a>0) blend_rect(cx, cy, 16, 16, COL(8,8,18), a);
        }
    }
    // dust motes drifting through the lamplight (glow-up): slow and
    // warm, and only inside the lamp's reach — the dark stays clean
    { int bx=SX(b->x), by=SY(b->y);
      for (int m2=0;m2<12;m2++){
          u32 hh = hash2(m2*17+3, 7, 0xD057u);
          float ph = G.anim_t*(0.11f + (float)(hh%5)*0.02f) + (float)(hh>>8);
          int mx2 = bx + (int)(fsin(ph + (float)(hh%9))*70.0f) + (int)((hh>>4)%40) - 20;
          int my2 = by + (int)(fcos(ph*0.8f + (float)(hh%7))*44.0f);
          int ddx = mx2-bx, ddy = my2-by;
          if (ddx*ddx + ddy*ddy < 110*110 && mx2>=vx && mx2<vx+vw2 && my2>=vy && my2<vy+vh2)
              pset(mx2, my2, COL(255,236,190));
      } }
    // a glitch tears the picture while it tears the air
    if (S->ent_ev==7){
        for (int s2=0;s2<7;s2++){
            u32 hh = hash2(s2*31, (int)(G.anim_t*24), 0x6C17u);
            int gy = vy + (int)(hh % (vh2-14));
            int gx = vx + (int)((hh>>7) % 40) - 20;
            blend_rect(gx, gy, vw2, 3, (s2&1)? COL(255,40,90) : COL(60,240,255), 46 + (int)(hh>>11)%40);
            for (int n2=0;n2<8;n2++){ u32 hn = hash2(s2*57+n2*13, (int)(G.anim_t*31), 0x57A7u);
                pset(vx + (int)(hn % vw2), vy + (int)((hn>>8) % vh2), (hn&1)? COL(255,255,255) : COL(20,20,30)); }
        }
    }
    // the buddy card + the run card
    rect_fill(vx+8, vy+8, 168, 46, COL(20,26,20)); rect_line(vx+8, vy+8, 168, 46, COL(198,164,84));
    { char buf[64]; char *q=buf; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[b->species%BUDN]); *q=0;
      draw_text(vx+14, vy+13, buf, 1, COL(255,244,210)); }
    { float f = b->maxhp>0? b->hp/b->maxhp : 0; if (f<0) f=0;
      rect_fill(vx+14, vy+26, 120, 5, COL(40,30,40)); rect_fill(vx+14, vy+26, (int)(120*f), 5, f>0.35f? COL(120,230,90) : COL(255,170,60));
      char hb[24]; char *q=hb; q=sapp_int(q, (int)b->hp); q=sappend(q, "/"); q=sapp_int(q, (int)b->maxhp); *q=0;
      draw_text(vx+138, vy+25, hb, 1, COL(230,240,230)); }
    { float f = b->energy/BUD_ENERGY_MAX; if (f<0) f=0;
      rect_fill(vx+14, vy+36, 120, 4, COL(40,30,40)); rect_fill(vx+14, vy+36, (int)(120*f), 4, COL(120,200,255));
      draw_text(vx+138, vy+35, "ENERGY", 1, COL(170,220,255)); }
    { char buf[72]; char *q=buf; if (S->ent_sub){ q=sappend(q, "FLOOR B-"); q=sapp_int(q, room); } else { q=sappend(q, "ENTRANCE "); q=sapp_int(q, room); }
      rect_fill(vx+vw2-158, vy+8, 150, 46, COL(20,26,20)); rect_line(vx+vw2-158, vy+8, 150, 46, COL(198,164,84));
      draw_text(vx+vw2-150, vy+13, buf, 1, COL(255,244,210));
      q=buf; q=sappend(q, "BITS "); q=sapp_int(q, S->ent_bits); q=sappend(q, " - STICKS "); q=sapp_int(q, S->ent_sticks); *q=0;
      draw_text(vx+vw2-150, vy+25, buf, 1, COL(230,240,230));
      q=buf; if (S->ent_sub){ q=sappend(q, "BEST B-"); q=sapp_int(q, b->ent_sub_best); } else { q=sappend(q, "BEST ROOM "); q=sapp_int(q, b->ent_best); }
      if (S->ent_key) q=sappend(q, " - KEY!");
      if (!S->ent_sub && b->ent_done>0){ q=sappend(q, " - OUT x"); q=sapp_int(q, b->ent_done); }
      *q=0;
      draw_text(vx+vw2-150, vy+37, buf, 1, S->ent_key? COL(255,214,64) : COL(170,200,170)); }
    if (S->ent_ev==6){
        int n=0; for (int j=0;j<5;j++) if (S->ent_fuse_mask>>j & 1) n++;
        char buf[40]; char *q=buf; q=sappend(q, "FUSES "); q=sapp_int(q, n); q=sappend(q, "/5   NOISE"); *q=0;
        draw_text(vx+14, vy+62, buf, 1, COL(255,244,210));
        rect_fill(vx+14, vy+74, 120, 4, COL(40,30,40));
        rect_fill(vx+14, vy+74, (int)(120 * S->ent_noise/100.0f), 4, S->ent_noise>66? COL(255,92,80) : COL(255,200,110)); }
    // context hint — dynamic situations only; the standing controls
    // line is gone (controls live in the Start-menu's CONTROLS panel),
    // and the prompts name the actual key/button for the device in
    // hand (Daniel, Nov 6)
    { const char *hint = 0; char ehb[72];
      float doorx = ent_room_x0(room) + (ENT_STRIDE-1)*BW_TILE + 24, doory = ent_doory(b, room);
      float dx=b->x-doorx, dy=b->y-doory;
      int hidden = ent_tile(pi, bw_tileof(b->x), bw_tileof(b->y))==3;
      if (S->ent_ev==5 && S->ent_phase==1) hint = "RUN! FOLLOW THE MOTHS!";
      else if (hidden) hint = S->ent_sub? "HIDING IN THE VENT. VERY SNEAKY." : "HIDING IN THE BUSH. VERY SNEAKY.";
      else if (S->ent_ev==6) hint = (room==25 && !S->ent_libdone)? "STAND STILL TO BE QUIET. FUSES GLOW GREEN." : (room==25? "THE LIBRARY DOOR IS OPEN." : "IT HEARS FOOTSTEPS. STAND STILL TO BE QUIET.");
      else if (!S->ent_sub && room==25 && (b->x-(ent_room_x0(25)+(ENT_STRIDE-2)*BW_TILE+24))*(b->x-(ent_room_x0(25)+(ENT_STRIDE-2)*BW_TILE+24)) + (b->y-((ENT_ROWS-2)*BW_TILE+24))*(b->y-((ENT_ROWS-2)*BW_TILE+24)) < 58*58)
          hint = S->ent_libdone? key_prompt(ehb, lbl_int(), "BEHIND THE BOOKCASE...") : "A TALL BOOKCASE. IT WILL NOT BUDGE... YET.";
      else if (ent_door_locked(pi, room) && dx*dx+dy*dy < 54*54) hint = S->ent_key? key_prompt(ehb, lbl_int(), "UNLOCK IT!") : "LOCKED - THE KEY IS IN THIS ROOM";
      else if (dx*dx+dy*dy < 54*54 && room > (S->ent_sub? S->ent_door_sub : S->ent_door_upto)) hint = key_prompt(ehb, lbl_int(), "OPEN THE DOOR");
      else if (!S->ent_sub && room==51) hint = "THE EXIT DOOR IS IN THE SOUTH WALL";
      else if (room==1 && b->x < 90) hint = key_prompt(ehb, lbl_int(), S->ent_sub? "UP THE STAIRS" : "BACK TO THE MALL");
      if (hint){ int hw2 = text_w(hint, 1);
      rect_fill(vx + vw2/2 - hw2/2 - 6, vy + vh2-22, hw2+12, 14, COL(20,26,20));
      draw_text_center(vx + vw2/2, vy + vh2-19, hint, 1, COL(240,248,240)); } }
    if (S->sleep_t>0) draw_text_center(vx + vw2/2, vy + vh2/2-4, "Z Z Z", 2, COL(200,210,255));
    bw_draw_note(pi);
}

// --- drawing -------------------------------------------------------------------
static void bw_draw_big_hacker(float wx, float wy, int boss, int named){
    int x=SX(wx), y=SY(wy); if (x<-70||x>clip_x1+70||y<-90||y>clip_y1+70) return;
    int raid = boss==1, hoard = boss==3;
    shadow_ellipse(x, y+14, 24, 5);
    circle_fill(x, y, 21, OUTLINE); circle_fill(x, y, 19, raid? COL(44,52,92) : hoard? COL(66,52,26) : COL(30,30,38));
    rect_fill(x-13, y-8, 26, 15, COL(16,20,30)); rect_line(x-13, y-8, 26, 15, OUTLINE);
    glow_ellipse(x, y-3, 13, 6, COL(80,160,255), 60);   // eyes burn blue in the dark, halo and all (Daniel, Nov 5)
    u32 eyec = raid? COL(140,220,255) : hoard? COL(255,204,80) : COL(150,220,255);
    pset(x-6, y-3, eyec); pset(x-5, y-3, eyec);
    pset(x+5, y-3, eyec); pset(x+6, y-3, eyec);
    line_thick(x-12, y-16, x-16, y-24, 2, OUTLINE); circle_fill(x-16, y-25, 3, raid? COL(90,200,255) : hoard? COL(255,214,64) : COL(255,160,60));
    line_thick(x+12, y-16, x+16, y-24, 2, OUTLINE); circle_fill(x+16, y-25, 3, raid? COL(90,200,255) : hoard? COL(255,214,64) : COL(255,160,60));
    if (!raid && !hoard){ for (int cw=-2; cw<=2; cw++) line_thick(x+cw*5, y-19, x+cw*5, y-26, 1, COL(70,70,80)); }
    if (hoard){ // the sack it sits on: everything it ever cached
        circle_fill(x+21, y+9, 10, OUTLINE); circle_fill(x+21, y+9, 8, COL(122,94,52));
        line_thick(x+18, y+1, x+24, y+1, 1, OUTLINE); pset(x+19, y+7, COL(255,224,130)); }
    if (named) draw_text_center(x, y+22, raid? "BROKEN RAID" : hoard? "CACHE HOARDER" : "ZERO-DAY ZED", 1, COL(255,190,170));
}
static void bw_draw_hacker(float wx, float wy, int lvl, float lunge, int named){
    int x=SX(wx), y=SY(wy);
    if (x<clip_x0-40||x>clip_x1+40||y<clip_y0-50||y>clip_y1+50) return;
    int bob = (int)(fsin(G.anim_t*3.2f + wx*0.1f)*1.0f);
    int lx = (int)(lunge*22);
    shadow_ellipse(x, y+11, 11, 3);
    rect_fill(x-8+lx, y-6+bob, 16, 15, COL(38,36,48)); rect_line(x-8+lx, y-6+bob, 16, 15, OUTLINE);   // hoodie body
    circle_fill(x+lx, y-12+bob, 8, COL(38,36,48)); circle_fill(x+lx, y-12+bob, 8, OUTLINE);
    circle_fill(x+lx, y-12+bob, 7, COL(24,22,34));                                                     // hood
    ellipse_fill(x+lx, y-11+bob, 5, 4, COL(16,14,24));                                                 // shadowed face
    glow_ellipse(x+lx, y-11+bob, 8, 5, COL(80,160,255), 65);
    disc_fill(x-2+lx, y-11+bob, 1, COL(160,225,255)); disc_fill(x+2+lx, y-11+bob, 1, COL(160,225,255)); // eyes in the dark, burning blue
    rect_fill(x-6+lx, y+2+bob, 13, 8, COL(70,74,84)); rect_line(x-6+lx, y+2+bob, 13, 8, OUTLINE);      // laptop
    rect_fill(x-4+lx, y+4+bob, 9, 4, COL(90,255,140));
    if (named) draw_text_center(x, y-26+bob, bw_hacker_name(lvl), 1, COL(200,255,200));
}
static void bw_draw_tree(int x, int y, int pine){
    rect_fill(x-2, y-2, 4, 9, COL(110,76,44)); rect_line(x-2, y-2, 4, 9, OUTLINE);
    if (pine){   // the pointed kind: three stacked skirts of deep green
        tri_fill(x-13, y-6, x, y-20, x+13, y-6, COL(30,92,40));
        tri_fill(x-11, y-13, x, y-26, x+11, y-13, COL(36,104,44));
        tri_fill(x-8, y-20, x, y-31, x+8, y-20, COL(48,122,52));
        line_thick(x-13, y-6, x, y-20, 1, OUTLINE); line_thick(x, y-20, x+13, y-6, 1, OUTLINE);
        disc_fill(x-3, y-16, 1, COL(120,205,100));
        return;
    }
    circle_fill(x, y-10, 13, OUTLINE); circle_fill(x, y-10, 12, COL(36,104,44));
    circle_fill(x-5, y-14, 7, COL(56,136,56)); circle_fill(x+4, y-7, 5, COL(56,136,56));
    disc_fill(x-3, y-13, 2, COL(94,186,82)); disc_fill(x+3, y-15, 1, COL(120,205,100));
}
static void bw_draw_rock(int x, int y){
    // a crag, not a cup: the old smooth dome on its saucer base read as
    // a random teacup in the land (Daniel, Nov 3). Angular faces + a crack.
    shadow_ellipse(x, y+7, 14, 3);
    tri_fill(x-14, y+7, x-10, y-4, x+14, y+7, OUTLINE);
    tri_fill(x-10, y-4, x-2, y-9, x+14, y+7, OUTLINE);
    tri_fill(x-12, y+6, x-9, y-3, x-1, y+6, COL(102,110,128));
    tri_fill(x-9, y-3, x-2, y-8, x-1, y+6, COL(126,136,156));
    tri_fill(x-2, y-8, x+8, y-4, x+12, y+6, COL(110,118,138));
    tri_fill(x-2, y-8, x+12, y+6, x-1, y+6, COL(96,104,122));
    line_thick(x+1, y-6, x-1, y+1, 1, OUTLINE); line_thick(x-1, y+1, x+2, y+5, 1, OUTLINE);
}
static void bw_draw_tent(float wx, float wy){
    int x=SX(wx), y=SY(wy);
    shadow_ellipse(x, y+8, 16, 3);
    tri_fill(x-16, y+8, x, y-14, x+16, y+8, COL(226,126,44));
    tri_fill(x, y-14, x+16, y+8, x, y+8, COL(240,142,54));        // sunlit east face (glow-up)
    tri_fill(x-16, y+8, x, y-14, x+16, y+8, OUTLINE);
    tri_fill(x-5, y+8, x, y-4, x+5, y+8, COL(60,36,24));
    line_thick(x, y-14, x, y-19, 1, OUTLINE);
    { int wv = (int)(fsin(G.anim_t*3.1f + wx*0.05f)*1.5f);        // a little pennant, waving
      tri_fill(x, y-19, x+7, y-17+wv, x, y-15, COL(216,52,52)); }
    draw_text_center(x, y-28, "CAMP", 1, COL(255,240,210));
}
static void bw_draw_gate(void){
    int x0=SX(0), y0=SY(-30);
    disc_fill(x0, y0+18, 30, COL(140,190,110));
    int px1=SX(-26), px2=SX(26), py0=SY(-58), py1=SY(-6);
    rect_fill(px1-3, py0, 6, py1-py0, COL(196,64,44)); rect_line(px1-3, py0, 6, py1-py0, OUTLINE);
    rect_fill(px2-3, py0, 6, py1-py0, COL(196,64,44)); rect_line(px2-3, py0, 6, py1-py0, OUTLINE);
    rect_fill(px1-10, py0-4, (px2-px1)+20, 6, COL(196,64,44)); rect_line(px1-10, py0-4, (px2-px1)+20, 6, OUTLINE);
    rect_fill(px1-10, py0-4, (px2-px1)+20, 2, COL(226,110,84));   // sunlit top edge (glow-up)
    rect_fill(px1-2, py0, 1, py1-py0, COL(226,110,84));
    rect_fill(px2-2, py0, 1, py1-py0, COL(226,110,84));
    rect_fill(px1-6, py0+8, (px2-px1)+12, 4, COL(170,52,38));
    draw_text_center(x0, py0-16, "BUG MALL", 1, COL(255,255,240));
}
static void bw_draw_objects(int pi, int pass){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi]; u32 seed=(u32)b->bw_seed;
    int tx0=bw_tileof(b->x - VW/2 - BW_TILE), tx1=bw_tileof(b->x + VW/2 + BW_TILE);
    int ty0=bw_tileof(b->y - VH/2 - BW_TILE), ty1=bw_tileof(b->y + VH/2 + BW_TILE);
    for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
        int t = bw_tile_at(tx, ty, seed); if (!t) continue;
        if (bw_outside_world(tx*BW_TILE + BW_TILE/2, ty*BW_TILE + BW_TILE/2)) continue;   // nothing grows at sea
        if (b->bw_mode==1 && bw_ridge_pass_zone(tx*BW_TILE + BW_TILE/2, ty*BW_TILE + BW_TILE/2)) continue;   // the pass is kept clear
        float wy = ty*BW_TILE + BW_TILE/2;
        if ((wy < b->y) != (pass==0)) continue;
        int x=SX(tx*BW_TILE + BW_TILE/2), y=SY(wy);
        if (t==1) bw_draw_tree(x, y, (int)(hash2(tx, ty, seed ^ 0x7E33u) % 3 == 0)); else bw_draw_rock(x, y);
    }
    int cx0=bw_cellof(b->x)-3, cy0=bw_cellof(b->y)-3;
    for (int cx=cx0; cx<=cx0+6; cx++) for (int cy=cy0; cy<=cy0+6; cy++){
        float ox, oy; if (bw_cell_feat(cx, cy, seed, &ox, &oy, b->bw_mode==1)!=1) continue;
        float wx=cx*BW_CELL+ox, wy=cy*BW_CELL+oy;
        float dx=wx-b->x, dy=wy-b->y; if (dx*dx+dy*dy > 700*700) continue;
        if ((wy < b->y) != (pass==0)) continue;
        bw_draw_tent(wx, wy);
    }
    for (int i=0;i<BW_ROAMN;i++){
        BWHack *h=&S->roam[i];
        if (h->cell==0 && h->x==0 && h->y==0) continue;
        if ((h->y < b->y) != (pass==0)) continue;
        bw_draw_hacker(h->x, h->y, h->lvl, 0, 1);
        if (h->alert > 0.4f) draw_text_center(SX(h->x), SY(h->y)-36, "!", 2, COL(255,92,80));
    }
    if (b->bw_mode==1){   // the hill ridges: grassy mounds where the wall runs
        for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
            float wx = tx*BW_TILE + BW_TILE/2, wy = ty*BW_TILE + BW_TILE/2;
            int on = 0;
            for (int ri=0; ri<4 && !on; ri++){
                float along; float d = bw_ridge_dist(wx, wy, BW_RIDGES[ri], &along);
                if (d >= 30) continue;
                float g = along - BW_RIDGES[ri][4]; if (g<0) g=-g;
                if (g <= BW_RIDGES[ri][5] + 26) continue;   // the pass stays visibly open
                on = 1;
            }
            if (!on) continue;
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy);
            u32 hh = hash2(tx, ty, seed ^ 0x4117u);
            int jx = (int)(hh % 13) - 6, jy = (int)((hh>>6) % 9) - 4;
            ellipse_fill(x+jx, y+8+jy, 23, 9, COL(88,116,54));
            ellipse_fill(x-3+jx, y+3+jy, 16, 8, COL(102,132,60));
            ellipse_fill(x+2+jx, y-1+jy, 9, 5, COL(122,152,74));
            disc_fill(x-6+jx, y+2+jy, 1, COL(150,178,102)); disc_fill(x+7+jx, y+5+jy, 1, COL(150,178,102));
        }
    }
    // --- story props: huts, racks, the fence, and people ---------------------
    if (b->bw_mode==1){
        static const float HUT[3][2] = { { -2700, -1480 }, { -2500, -1480 }, { -2600, -1300 } };
        for (int i=0;i<3;i++){
            float wx=HUT[i][0], wy=HUT[i][1]+10;
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-70||x>clip_x1+70||y<-70||y>clip_y1+70) continue;
            shadow_ellipse(x, y+12, 30, 5);
            rect_fill(x-28, y-16, 56, 28, COL(158,116,74)); rect_line(x-28, y-16, 56, 28, OUTLINE);
            tri_fill(x-34, y-16, x, y-38, x+34, y-16, COL(128,66,44));
            line_thick(x-34, y-16, x, y-38, 1, OUTLINE); line_thick(x, y-38, x+34, y-16, 1, OUTLINE);
            rect_fill(x-6, y-2, 12, 14, COL(70,44,26)); rect_line(x-6, y-2, 12, 14, OUTLINE);
            rect_fill(x+14, y-10, 9, 9, COL(255,214,110)); rect_line(x+14, y-10, 9, 9, OUTLINE);
        }
        for (int gx=-2; gx<=2; gx++) for (int gy=-1; gy<=1; gy++){
            float wx = BW_SITE_XY[1][0] + gx*92, wy = BW_SITE_XY[1][1] + gy*112 + 16;
            { float cdx = wx-BW_SITE_XY[1][0], cdy = wy-16-BW_SITE_XY[1][1];
              if (cdx*cdx+cdy*cdy < 105*105) continue; }
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-70||x>clip_x1+70||y<-70||y>clip_y1+70) continue;
            shadow_ellipse(x, y+2, 28, 4);
            rect_fill(x-27, y-34, 54, 36, COL(40,44,56)); rect_line(x-27, y-34, 54, 36, OUTLINE);
            rect_fill(x-27, y-34, 54, 6, COL(58,64,78));
            for (int d2=0; d2<4; d2++){
                int on = (int)(G.anim_t*2 + gx*3 + gy*7 + d2*2) % 5 != 0;
                pset(x-18 + d2*11, y-18, on? COL(96,255,150) : COL(40,80,50));
                pset(x-18 + d2*11, y-10, COL(90,100,120));
            }
        }
        // the ring around the inner aisle (south rack sealed until solved)
        for (int k=0;k<8;k++){
            float a = k*0.7854f;
            float wx = BW_SITE_XY[1][0] + fcos(a)*108, wy = BW_SITE_XY[1][1] + fsin(a)*108 + 14;
            int sealed = (k==2 && !bw_puz_done(b));
            if (k==2 && bw_puz_done(b)){
                if (pass==0){ int x=SX(wx), y=SY(wy);   // the open gate: a worn plate
                    if (x>-40 && x<clip_x1+40 && y>-30 && y<clip_y1+30){ rect_fill(x-22, y-8, 44, 12, COL(96,88,60)); rect_line(x-22, y-8, 44, 12, OUTLINE); } }
                continue;
            }
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-70||x>clip_x1+70||y<-70||y>clip_y1+70) continue;
            shadow_ellipse(x, y+2, 24, 4);
            rect_fill(x-23, y-28, 46, 30, sealed? COL(52,40,44) : COL(40,44,56)); rect_line(x-23, y-28, 46, 30, OUTLINE);
            if (sealed){ int on = (int)(G.anim_t*2.4f)%2==0; circle_fill(x, y-14, 3, on? COL(255,70,60) : COL(120,30,26)); }
            else { pset(x-12, y-18, COL(96,255,150)); pset(x+2, y-18, COL(90,100,120)); pset(x+12, y-10, COL(96,255,150)); }
        }
        // the three terminals, wearing their colors (lit once pressed in order)
        { int ord[3]; bw_term_order(b, ord);
          int prog = bw_puz_prog(b);
          for (int i=0;i<3;i++){
              float wx=BW_TERM_XY[i][0], wy=BW_TERM_XY[i][1]+8;
              if ((wy < b->y) != (pass==0)) continue;
              int x=SX(wx), y=SY(wy); if (x<-40||x>clip_x1+40||y<-50||y>clip_y1+40) continue;
              shadow_ellipse(x, y+4, 10, 3);
              rect_fill(x-3, y-4, 6, 8, COL(90,84,80));
              rect_fill(x-9, y-16, 18, 13, COL(30,34,44)); rect_line(x-9, y-16, 18, 13, OUTLINE);
              int posn = ord[0]==i? 0 : ord[1]==i? 1 : 2;
              int lit = bw_puz_done(b) || posn < prog;
              circle_fill(x, y-10, 4, lit? BW_TERM_COL[i] : col_shade(BW_TERM_COL[i], 0.4f));
              if (lit) pset(x, y-10, COL(255,255,255));
          }
          // the scratched sign by the south entrance
          { float wx=3400, wy=-2100+8;
            if ((wy < b->y) == (pass==0)){
                int x=SX(wx), y=SY(wy);
                if (x>-40 && x<clip_x1+40 && y>-50 && y<clip_y1+40){
                    rect_fill(x-2, y-2, 4, 10, COL(90,84,80));
                    rect_fill(x-11, y-14, 22, 13, COL(64,58,50)); rect_line(x-11, y-14, 22, 13, OUTLINE);
                    line_thick(x-7, y-10, x+6, y-10, 1, COL(200,190,170));
                    line_thick(x-7, y-6, x+2, y-6, 1, COL(200,190,170));
                }
            }
          }
        }
        // Zed's fence: posts and wire all the way around
        { float cx=BW_SITE_XY[3][0], cy=BW_SITE_XY[3][1], r=BW_SITE_XY[3][2];
          for (int k=0;k<48;k++){
              float a = k*0.1309f, wx = cx + fcos(a)*r, wy = cy + fsin(a)*r;
              if ((wy < b->y) != (pass==0)) continue;
              int x=SX(wx), y=SY(wy); if (x<-30||x>clip_x1+30||y<-40||y>clip_y1+30) continue;
              rect_fill(x-2, y-16, 4, 20, COL(70,60,66)); rect_line(x-2, y-16, 4, 20, OUTLINE);
              float a2 = (k+1)*0.1309f;
              int x2=SX(cx + fcos(a2)*r), y2=SY(cy + fsin(a2)*r);
              line_thick(x, y-13, x2, y2-13, 1, COL(150,140,150));
              line_thick(x, y-6, x2, y2-6, 1, COL(120,112,120));
          }
          if (pass==1){
              int x=SX(cx), y=SY(cy + r + 20);
              if (x>-90 && x<clip_x1+90 && y>-30 && y<clip_y1+30)
                  draw_text_center(x, y, "ZED'S PLACE. KNOCK IF YOU MUST.", 1, COL(255,190,150));
          }
        }
        // the Overgrown Ruins: a ring of standing stones and the cache itself
        for (int k=0;k<7;k++){
            float a = k*0.8976f, wx = BW_SITE_XY[4][0] + fcos(a)*120, wy = BW_SITE_XY[4][1] + fsin(a)*120 + 8;
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-40||x>clip_x1+40||y<-60||y>clip_y1+40) continue;
            int hh = 22 + (int)(hash2(k, 7, 99)%8);
            shadow_ellipse(x, y+10, 10, 3);
            rect_fill(x-5, y+10-hh, 10, hh, COL(122,126,118)); rect_line(x-5, y+10-hh, 10, hh, OUTLINE);
            rect_fill(x-5, y+10-hh, 4, hh, COL(140,144,134));
            pset(x-2, y+2-hh/2, COL(96,120,72)); pset(x+2, y+6-hh/2, COL(96,120,72));   // moss
        }
        { float wx=BW_SITE_XY[4][0], wy=BW_SITE_XY[4][1]+26;   // the sunken cache, under the Hoarder
          if ((wy < b->y) == (pass==0)){
              int x=SX(wx), y=SY(wy);
              if (x>-50 && x<clip_x1+50 && y>-40 && y<clip_y1+40){
                  int open = (b->story_bits>>22)&1;
                  shadow_ellipse(x, y+8, 17, 4);
                  rect_fill(x-15, y-4, 30, 13, COL(104,74,40)); rect_line(x-15, y-4, 30, 13, OUTLINE);
                  rect_fill(x-15, y-4, 30, 4, COL(255,196,64));
                  if (open){ rect_fill(x-15, y-12, 30, 7, COL(84,58,30)); rect_line(x-15, y-12, 30, 7, OUTLINE);
                      pset(x-6, y+2, COL(255,224,130)); pset(x+4, y+3, COL(255,224,130)); }
                  else { rect_fill(x-15, y-9, 30, 6, COL(122,88,48)); rect_line(x-15, y-9, 30, 6, OUTLINE); }
              } } }
        // Mop's lost tools, glinting where the meadow dropped them
        for (int t2=0;t2<3;t2++){
            if ((b->story_bits>>(24+t2))&1) continue;
            float wx=BW_TOOL_XY[t2][0], wy=BW_TOOL_XY[t2][1];
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-30||x>clip_x1+30||y<-30||y>clip_y1+30) continue;
            int bob = (int)(fsin(G.anim_t*3 + t2*2.1f)*1.5f);
            disc_fill(x, y+bob, 4, COL(255,214,64)); disc_fill(x, y+bob, 2, COL(255,244,190));
            line_thick(x-4, y+5+bob, x+4, y+5+bob, 1, OUTLINE);
            if ((int)(G.anim_t*2 + t2)%3==0) pset(x+5, y-5+bob, COL(255,255,255));
        }
        // the big residents, at home
        for (int bs=0; bs<3; bs++){
            int alive = bs==0? (bw_puz_done(b) && !((b->story_bits>>16)&1)) : bs==1? !((b->story_bits>>17)&1) : !((b->story_bits>>22)&1);
            if (!alive) continue;
            float wx = BW_SITE_XY[bs==0? 1 : bs==1? 3 : 4][0];
            float wy = BW_SITE_XY[bs==0? 1 : bs==1? 3 : 4][1] + 10;
            if ((wy < b->y) != (pass==0)) continue;
            bw_draw_big_hacker(wx, wy, bs==0? 1 : bs==1? 2 : 3, 1);
        }
        // help signs: a post, a board, and something scratched on it
        for (int i=0;i<6;i++){
            float wx=BW_HSIGN_XY[i][0], wy=BW_HSIGN_XY[i][1];
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-40||x>clip_x1+40||y<-50||y>clip_y1+40) continue;
            shadow_ellipse(x, y+10, 10, 3);
            rect_fill(x-2, y-2, 4, 12, COL(110,76,44)); rect_line(x-2, y-2, 4, 12, OUTLINE);
            rect_fill(x-13, y-14, 26, 13, COL(196,158,102)); rect_line(x-13, y-14, 26, 13, OUTLINE);
            line_thick(x-9, y-10, x+6, y-10, 1, COL(120,86,48)); line_thick(x-9, y-6, x+2, y-6, 1, COL(120,86,48));
        }
        // Bitling, the stray process, on its slow happy drift
        { float wx, wy; bw_bitling_pos(&wx, &wy);
          if ((wy < b->y) == (pass==0)){
            int x=SX(wx), y=SY(wy);
            if (x>-40 && x<clip_x1+40 && y>-50 && y<clip_y1+40){
                int bob = (int)(fsin(G.anim_t*5.0f)*1.5f);
                shadow_ellipse(x, y+9, 9, 3);
                rect_fill(x-7, y-4+bob, 14, 12, COL(70,160,170)); rect_line(x-7, y-4+bob, 14, 12, OUTLINE);
                circle_fill(x, y-8+bob, 6, COL(70,160,170)); circle_fill(x, y-8+bob, 6, OUTLINE);
                disc_fill(x-2, y-8+bob, 1, COL(255,250,220)); disc_fill(x+2, y-8+bob, 1, COL(255,250,220));
                line_thick(x, y-14+bob, x+4, y-19+bob, 1, OUTLINE);
                disc_fill(x+4, y-19+bob, 2, COL(255,120,160));
            }
          } }
        // people
        static const char *NPC_NAME[3] = { "PATCHINA", "MOP", "GLITCH" };
        for (int i=0;i<3;i++){
            float wx=BW_NPC_XY[i][0], wy=BW_NPC_XY[i][1];
            if ((wy < b->y) != (pass==0)) continue;
            int x=SX(wx), y=SY(wy); if (x<-40||x>clip_x1+40||y<-60||y>clip_y1+40) continue;
            shadow_ellipse(x, y+10, 11, 3);
            tri_fill(x-9, y+10, x, y-12, x+9, y+10, BW_NPC_ROBE[i]);
            line_thick(x-9, y+10, x, y-12, 1, OUTLINE); line_thick(x, y-12, x+9, y+10, 1, OUTLINE);
            circle_fill(x, y-15, 6, OUTLINE); circle_fill(x, y-15, 5, COL(242,204,164));
            if (i!=1){ tri_fill(x-4, y-11, x, y-4, x+4, y-11, COL(238,238,240)); }   // the beard (Mop is beardless, the scandal)
            pset(x-2, y-16, OUTLINE); pset(x+2, y-16, OUTLINE);
            int questy = (i==0 && (bw_qstate(b,0)==1 || (bw_qstate(b,1)==2 && bw_qstate2(b,8)==0)))
                      || (i==1 && ((bw_qstate_x(b,4)==1 && bw_tools_found(b)>=3) || (bw_qstate_x(b,4)==2 && bw_qstate2(b,7)==0)))
                      || (i==2 && (bw_qstate(b,2)==1 || (bw_qstate2(b,7)==1 && ((b->story_bits2>>10)&1)) || (bw_qstate2(b,6)==1 && bw_lakekills(b)>=2) || (bw_qstate(b,2)==2 && bw_qstate2(b,6)==0)));
            if (questy) draw_text_center(x, y-34 - (int)(fsin(G.anim_t*4)*1.5f), "!", 2, COL(255,214,64));
            draw_text_center(x, y+16, NPC_NAME[i], 1, COL(255,246,220));
        }
    }
}
// ---- minimap (Nov 3, Daniel: "desperately need a minimap in the
// story mode." A quest compass arrow shipped alongside and was cut the
// same day at his word — "too much of a gimme"; minimap only.)
// The current story objective, if any: main quests first, the hermit
// errand last; after the farm falls, Patchina hands out the funeral.
static int bw_quest_target(Buddy *b, float *tx, float *ty){
    if (b->bw_mode!=1 || !b->story_started) return 0;
    if (bw_qstate(b,0)==1){ *tx=BW_NPC_XY[0][0]; *ty=BW_NPC_XY[0][1]; return 1; }
    if (bw_qstate(b,1)==1){ *tx=BW_SITE_XY[1][0]; *ty=BW_SITE_XY[1][1]; return 1; }
    if (bw_qstate(b,1)==2 && bw_qstate(b,3)==0){ *tx=BW_NPC_XY[0][0]; *ty=BW_NPC_XY[0][1]; return 1; }
    if (bw_qstate_x(b,5)==1){ *tx=BW_SITE_XY[4][0]; *ty=BW_SITE_XY[4][1]; return 1; }
    if (bw_qstate(b,3)==1){ *tx=BW_SITE_XY[3][0]; *ty=BW_SITE_XY[3][1]; return 1; }
    if (bw_qstate_x(b,4)==1){
        int best=-1; float bd=1e18f;
        for (int i=0;i<3;i++){ if ((b->story_bits>>(24+i))&1) continue;
            float dx=BW_TOOL_XY[i][0]-b->x, dy=BW_TOOL_XY[i][1]-b->y, d=dx*dx+dy*dy;
            if (d<bd){ bd=d; best=i; } }
        if (best>=0){ *tx=BW_TOOL_XY[best][0]; *ty=BW_TOOL_XY[best][1]; return 1; }
        *tx=BW_NPC_XY[1][0]; *ty=BW_NPC_XY[1][1]; return 1;   // all found: back to Mop
    }
    if (bw_qstate2(b,7)==1){ *tx=BW_NPC_XY[2][0]; *ty=BW_NPC_XY[2][1]; return 1; }   // the parcel goes to Glitch
    if (bw_qstate2(b,6)==1){
        if (bw_lakekills(b)>=2){ *tx=BW_NPC_XY[2][0]; *ty=BW_NPC_XY[2][1]; }
        else { *tx=BW_SITE_XY[2][0]; *ty=BW_SITE_XY[2][1]; }
        return 1;
    }
    if (bw_qstate2(b,8)==1){ bw_bitling_pos(tx, ty); return 1; }
    if (bw_qstate(b,2)==1){ *tx=BW_NPC_XY[2][0]; *ty=BW_NPC_XY[2][1]; return 1; }
    if (bw_qstate2(b,9)==1){
        int best=-1; float bd=1e18f;
        for (int i=0;i<6;i++){ if ((b->story_bits2>>(11+i))&1) continue;
            float dx=BW_HSIGN_XY[i][0]-b->x, dy=BW_HSIGN_XY[i][1]-b->y, d=dx*dx+dy*dy;
            if (d<bd){ bd=d; best=i; } }
        if (best>=0){ *tx=BW_HSIGN_XY[best][0]; *ty=BW_HSIGN_XY[best][1]; return 1; }
    }
    return 0;
}
static void bw_draw_minimap(int pi){
    Buddy *b=&G.bud[pi];
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW;
    const int MW=126, MH=98;
    int mx=vx+vw2-8-MW, my=vy+8;
    rect_fill(mx, my, MW, MH, COL(12,16,20)); rect_line(mx, my, MW, MH, COL(198,164,84));
    int px0=mx+7, py0=my+7, pw=MW-14, ph=MH-14;
    // The map rides with you (Nov 4, Daniel: "you can go off the map
    // lol"): a 3900x2580 window CENTRED on the buddy over an ocean
    // that runs to the world's hard edge. You are always the dot in
    // the middle; an objective beyond the window hugs its edge.
    const float WINW=3900.0f, WINH=2580.0f;
    float mox = b->x - WINW/2, moy = b->y - WINH/2;
    #define BW_MMX(wx) (px0 + (int)(((wx)-mox)*(float)pw/WINW))
    #define BW_MMY(wy) (py0 + (int)(((wy)-moy)*(float)ph/WINH))
    rect_fill(px0, py0, pw, ph, COL(32,80,136));   // ocean, everywhere the land is not
    { int lx0=BW_MMX(BW_WX0), ly0=BW_MMY(BW_WY0), lx1=BW_MMX(BW_WX1), ly1=BW_MMY(BW_WY1);
      int cx0 = lx0<px0? px0 : lx0, cy0 = ly0<py0? py0 : ly0;
      int cx1 = lx1>px0+pw? px0+pw : lx1, cy1 = ly1>py0+ph? py0+ph : ly1;
      if (cx1>cx0 && cy1>cy0) rect_fill(cx0, cy0, cx1-cx0, cy1-cy0, COL(52,96,44)); }
    if (b->bw_mode==1){
        static const u32 BSC[5] = { COL(232,196,120), COL(186,192,198), COL(170,220,245), COL(255,110,90), COL(170,180,120) };
        for (int i=0;i<5;i++){
            float wx=BW_SITE_XY[i][0], wy=BW_SITE_XY[i][1];
            if (wx<b->x-WINW/2 || wx>b->x+WINW/2 || wy<b->y-WINH/2 || wy>b->y+WINH/2) continue;
            disc_fill(BW_MMX(wx), BW_MMY(wy), 3, BSC[i]);
        }
    } else {
        if (0>=b->x-WINW/2 && 0<=b->x+WINW/2 && 0>=b->y-WINH/2 && 0<=b->y+WINH/2)
            disc_fill(BW_MMX(0), BW_MMY(0), 2, COL(240,248,240));   // the gate home
    }
    float tx, ty;
    if (bw_quest_target(b, &tx, &ty)){
        int sx=BW_MMX(tx), sy=BW_MMY(ty), r = fsin(G.anim_t*4.0f)>0? 5 : 4;
        if (sx<px0+5) sx=px0+5; if (sx>px0+pw-5) sx=px0+pw-5;
        if (sy<py0+5) sy=py0+5; if (sy>py0+ph-5) sy=py0+ph-5;
        tri_fill(sx-r, sy, sx, sy-r, sx+r, sy, COL(255,214,64));
        tri_fill(sx-r, sy, sx, sy+r, sx+r, sy, COL(255,214,64));
    }
    { int sx=BW_MMX(b->x), sy=BW_MMY(b->y); disc_fill(sx, sy, 3, OUTLINE); disc_fill(sx, sy, 2, COL(255,244,210)); }
    #undef BW_MMX
    #undef BW_MMY
}
// Story text lives in a dialogue box now (Nov 4, Daniel: "the text just
// shows up across the top, underneath the map even — have it be in a
// dialogue box with better contrast at the bottom"). Every note — NPC
// lines, quest updates, drops — used to be one bare line of pale text
// floating over the meadow at y+66. Now: an opaque dark box with a gold
// rim along the BOTTOM of the view, text word-wrapped to two lines,
// drawn after the minimap so nothing ever slides underneath it. The
// box sits exactly where the context-hint pill lives; the note wins
// while it is up. Battle text keeps its own box in the battle render.
static void bw_draw_note(int pi){
    BwSim *S = &BWS[pi];
    if (S->note_t<=0 || !S->note[0]) return;
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    int bw2 = vw2-16; if (bw2<120) bw2=vw2;
    int bx = vx + (vw2-bw2)/2, by = vy + vh2 - 48, bh = 44;
    rect_fill(bx, by, bw2, bh, COL(12,14,20)); rect_line(bx, by, bw2, bh, COL(198,164,84));
    int maxw = bw2 - 20;
    // word-wrap into at most two lines by measured width
    char l1[84], l2[84]; l1[0]=0; l2[0]=0; int line=1;
    const char *p = S->note;
    while (*p){
        while (*p==' ') p++;
        if (!*p) break;
        const char *w = p; while (*p && *p!=' ') p++;
        int wl = (int)(p-w);
        char word[40]; int wc = wl>38? 38 : wl; for (int i=0;i<wc;i++) word[i]=w[i]; word[wc]=0;
        for (int attempt=0; attempt<2; attempt++){
            char *cur = line==1? l1 : l2;
            char trial[88]; trial[0]=0;
            char *tp = sappend(trial, cur);
            if (*cur) tp = sappend(tp, " ");
            tp = sappend(tp, word); (void)tp;
            if (text_w(trial, 1) <= maxw || !*cur){ scpy(cur, trial, 84); break; }
            if (line==1){ line=2; continue; }
            // both lines full: the note is longer than the box (cannot
            // happen for the 72-char note buffer at full width)
        }
    }
    if (l2[0]){ draw_text(bx+10, by+8, l1, 1, COL(255,250,235)); draw_text(bx+10, by+24, l2, 1, COL(255,250,235)); }
    else draw_text(bx+10, by+16, l1, 1, COL(255,250,235));
}

