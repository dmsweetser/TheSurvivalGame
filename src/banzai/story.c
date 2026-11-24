// THE SURVIVAL GAME — banzai/story.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from banzai.c lines 52-655 in the Nov 2026 fine-grained refactor; code unchanged.

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
// THE REBUILDING (v1.25 feat 14): the post-game chapter's quests
// 10-13 live in story_bits2's spare bits 17-24 — no new save token,
// and a pre-chapter save loads with the chapter unstarted (bits 0).
static int bw_qstate3(Buddy *b, int q){ return (b->story_bits2 >> (17+(q-10)*2)) & 3; }   // quests 10-13
static void bw_qset3(Buddy *b, int q, int s){ int sh=17+(q-10)*2; b->story_bits2 = (b->story_bits2 & ~(3<<sh)) | ((s&3)<<sh); }
static int bw_qstate_g(Buddy *b, int q){ return q<6? bw_qstate_x(b,q) : q<10? bw_qstate2(b,q) : bw_qstate3(b,q); }   // any quest, 0-13
// "Story complete" has exactly one definition in the game — the
// Trophy Case's count of quests 0-9 at state 2 — and the chapter's
// unlock test uses it, so the two can never disagree.
static int bw_story_complete(Buddy *b){ int n=0; for (int q=0;q<10;q++) if (bw_qstate_g(b,q)==2) n++; return n==10; }
static int bw_chapter_done(Buddy *b){ if (!bw_story_complete(b)) return 0; for (int q=10;q<14;q++) if (bw_qstate3(b,q)!=2) return 0; return 1; }   // the Story Complete+ flag
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
// Duster, the village sweep-bot (feat 14, quest 11): a little round
// bot who must be walked to the Server Farm to scrub the racks. Its
// home post stands by the village green, clear of the hut boxes; its
// heels live in the owner's BwSim like everything else about a trip.
#define CH2_DUSTER_X (-2470)
#define CH2_DUSTER_Y (-1380)
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
typedef struct { float x, y, hp, maxhp, atk_t, hit_t; int kind, chunk, slot; } MorEn;   // MORDOR FUN RUN foes (v1.28): 0 goblin, 1 wolf, 2 spider, 3 orc. chunk/slot name the deterministic spawn they came from, so a kill can be written into the session's dead-bitmap.
// MORDOR FUN RUN (v1.28): the MOOSE SCALE constants (MOR_MOOSE_PX
// and friends) live in data/design.c with the Buddy's mor_* fields —
// the save loader (save/start.c) is included before this file and
// already needs MOR_LEN/MOR_MILES for its clamps.
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
#define WAR_MAXT 256   // WAR! terrain capacity (debris + barrier segments): v1.18 raised it from 72 — worst case was 68 debris + ~81 barrier segments = 149; v1.27 widened the scatter roll to 80 and kept the lines, so the cap rises to 256 rather than let a fat field truncate a barrier line (defined here, before BwSim; the WAR! block below points back at this)
typedef struct {
    BwBattle bat;
    BWHack roam[BW_ROAMN];
    int defeated[64], defeated_n;
    float grace_t, sleep_t, note_t;
    int slept, log_open, log_sel, log_prevk, buff, site_cur;   // log_prevk: v1.30 — DRAG nav edges inside the story log
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
    // DEEPER DOORS (v1.25 feat 11): Chapter 2 run state — per-trip
    // only, never saved; the records live on the Buddy (e2).
    // ent_chap is 1 for a Chapter 1 run (the default everywhere)
    // and 2 for a Deeper run; Lumen's hover point and the duet
    // door's charge ride along.
    int ent_chap;
    float ent_lux, ent_luy, ent_duet_t;
    // THE REBUILDING (v1.25 feat 14): the post-game chapter's trip
    // state — per-trip only, never saved; the records live on the
    // Buddy (sq2 bits 17-24). The cider run's clock, Duster the
    // sweep-bot's post and heels, and the Patch Day clock.
    float ch2_race_t; int ch2_race_on;
    float ch2_esc_x, ch2_esc_y; int ch2_esc_on;
    float ch2_wp_x, ch2_wp_y; int ch2_wp_on;         // feat 14: the escort's pathing waypoint
    float ch2_path_t, ch2_stall, ch2_base;           // feat 14: repath cooldown, stall clock, progress baseline
    float ch2_fest_t;
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
    // MEDAL TIMES (v1.25 feat 3): a per-level timer (plat_t above is a
    // session clock and never resets per level), the level-select
    // state, and what the clear banner reports for the run just done
    float plat_lvl_t, plat_selt;
    int plat_sel, plat_pick, plat_show_tds, plat_show_tier, plat_show_newbest;
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
    int cas_shuffle, cas_seed;         // CASTLE SHUFFLE (v1.25 feat 7): this run is a shuffle, and the seed it was dealt
    unsigned char cas_iroom[17];       // the room each item sits in THIS run (the classic deal or a shuffle deal)
    float cas_ix[17], cas_iy[17];      // ...and its dealt position (classic fills these from the original formula)
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
    int war_demoarm;             // v1.21: the struct index whose DEMOLISH is ARMED (two-step sell-back), -1 = none; lives only while that building stays selected
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
    // WAR! LOCAL CO-OP COMMAND (v1.25 feat 13): two local players
    // command ONE battle — the battle lives in the OWNER's sim (who
    // launched it), the other commander is the GUEST. war_duo sits
    // on the owner's sim (the guest's pi, -1 = none; crt_duo's
    // convention), war_guest on the guest's sim (owner pi + 1,
    // 0 = not a guest; crt_guest's convention). A link is trusted
    // only when both sides agree, so the zero-init state can never
    // fake one. war_pause_by: who opened the shared battle pause.
    // All three are runtime-only, never saved, never snapshotted.
    int war_duo, war_guest, war_pause_by;
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
    // v1.29: the RESUME / NEW SKIRMISH panel raised by war_enter
    // when a saved skirmish is parked — the Mordor panel pattern
    // (selection, two-step discard confirm, an entry pick lock so
    // the entering select edge cannot pick RESUME itself).
    int war_sk_panel, war_sk_psel;
    float war_sk_confirm, war_sk_lock;
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
    int war_hbits[28];           // v1.21: each HIVE's own bit purse — the hive harvests the same nodes and pays for every bug it sends (rides the snapshot as the wh key)
    // v1.21 cosmetic pass: purely visual state — hit-flash timers
    // and a small particle ring (0 poof / 1 harvest sparkle / 2
    // structure poof). Written by the hurt/harvest hooks, read ONLY
    // by the draw code, never saved, never fed back into the sim.
    float war_uflash[80], war_sflash[28];
    float war_fx[24], war_fy[24], war_ft[24]; int war_fk[24];
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
    // MORDOR FUN RUN (bw_mode 9, v1.28): one run's SESSION scratch.
    // The run itself (distance, milestone, HP, clock, on/off) lives
    // on the Buddy (mu token) so it survives leaving; what lives
    // here is only what a fresh entry may rebuild: the live foes,
    // the session's dead-bitmap (chunk*5+slot per bit; a faint
    // clears the bits at/after the banked milestone, so the current
    // stretch resets and the banked road stays cleared), the entry
    // panel, and the timers. MOR_DEAD_BYTES covers the full route:
    // (460000/300 + 2) chunks * 5 slots / 8 = 960 bytes.
    MorEn mor_en[40];
    int mor_en_n, mor_panel, mor_psel, mor_kills, mor_swings;
    unsigned char mor_dead[960];
    float mor_t, mor_swing_cd, mor_swing_t, mor_grace, mor_faintt, mor_cleart, mor_spawn_t, mor_confirm_t, mor_hurt_t, mor_pick_lock;
    // v1.29: road powerups — the taken-stations bitmap for this
    // cabinet session (50 stations; a faint clears the bits at/
    // after the banked milestone, mirroring mor_dead) and the two
    // timed buffs. Run-scoped on purpose: the buffs tick in real
    // time, lapse when the cabinet is left, clear on a faint, and
    // are NEVER written to the save tokens.
    unsigned char mor_powtaken[8];
    float mor_amp_t, mor_spd_t;
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
