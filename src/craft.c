// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: craft.c — BUDDY CRAFT (Banzai World mode 4).
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]
// ===========================================================================
// BUDDY CRAFT (Banzai World mode 4, Nov 5 — Daniel asked for a
// Minecraft-style analogue; heavily expanded the same evening):
// a top-down, endless, procedural wild of biomes with a full
// day/night cycle. Chop trees, mine rocks, gather fiber, bonk feral
// bugs (worse after dark), and craft through five categories —
// tools, weapons, building, defense, food — with tiers, upgrades
// and prerequisites. Everything you gather, craft, BUILD and mine
// persists on the Buddy (cm/cn/cg/cw/cd/ct): leaving no longer
// wipes your base (Daniel's bug), and campfires really do let you
// sleep — right through the night, if it's dark.
// build kinds: 0 wood wall, 1 campfire, 2 stone wall, 3 fireplace,
// 4 zapper tower, 5 spike trap, 6 torch post.
// ===========================================================================
static int cdiv2(int a, int b){ int q = a/b; if ((a%b)!=0 && ((a<0)!=(b<0))) q--; return q; }
static u32 craft_seed(Buddy *b){ return (u32)b->bw_seed ^ 0xC4A7u; }
static float craft_noise(int tx, int ty, u32 seed){
    int cs = 9;
    int cx = cdiv2(tx, cs), cy = cdiv2(ty, cs);
    float fx = (tx - cx*cs)/(float)cs, fy = (ty - cy*cs)/(float)cs;
    float sx = fx*fx*(3-2*fx), sy = fy*fy*(3-2*fy);
    float v00 = hash_f(cx, cy, seed), v10 = hash_f(cx+1, cy, seed);
    float v01 = hash_f(cx, cy+1, seed), v11 = hash_f(cx+1, cy+1, seed);
    return (v00*(1-sx) + v10*sx)*(1-sy) + (v01*(1-sx) + v11*sx)*sy;
}
// biomes: 0 plains, 1 forest, 2 desert, 3 snow, 4 swamp, 5 water
static int craft_biome_at(Buddy *b, int tx, int ty){
    if (tx>=-3 && tx<=3 && ty>=-3 && ty<=4) return 0;   // the spawn clearing is always plains
    u32 sd = craft_seed(b);
    if (craft_noise(tx, ty, sd ^ 0x11AAu) > 0.60f) return 5;
    float t = craft_noise(tx, ty, sd ^ 0x22BBu), m = craft_noise(tx, ty, sd ^ 0x33CCu);
    if (t < 0.30f) return 3;
    if (t > 0.68f && m < 0.46f) return 2;
    if (m > 0.66f) return 4;
    if (m > 0.50f) return 1;
    return 0;
}
static int craft_key(int tx, int ty){ return ((tx+2048)<<12) + (ty+2048); }
static int craft_gone_has(Buddy *b, int key){
    for (int i=0;i<b->craft_gn;i++) if (b->craft_gone[i]==key) return 1;
    return 0;
}
// nodes: 0 none, 1 tree, 2 rock, 3 cactus, 4 fiber tuft
static int craft_node_at(int pi, int tx, int ty){
    Buddy *b=&G.bud[pi];
    if (craft_gone_has(b, craft_key(tx, ty))) return 0;
    int biome = craft_biome_at(b, tx, ty);
    if (biome==5) return 0;
    if (tx>=-2 && tx<=2 && ty>=-2 && ty<=3) return 0;   // keep the spawn clearing clear
    int roll = (int)(hash2(tx, ty, craft_seed(b) ^ 0xB10Cu) % 1000);
    switch (biome){
        case 1: return roll<240? 1 : roll<270? 2 : roll<350? 4 : 0;
        case 2: return roll<90? 3 : roll<170? 2 : 0;
        case 3: return roll<150? 1 : roll<220? 2 : 0;
        case 4: return roll<120? 1 : roll<260? 4 : roll<280? 2 : 0;
        default: return roll<60? 1 : roll<180? 4 : roll<200? 2 : 0;
    }
}
static int craft_build_at(Buddy *b, int tx, int ty){
    for (int i=0;i<b->craft_bn;i++) if (b->craft_btx[i]==tx && b->craft_bty[i]==ty) return i;
    return -1;
}
static int craft_build_kind_count(Buddy *b, int kind){
    int n=0; for (int i=0;i<b->craft_bn;i++) if (b->craft_bk[i]==kind) n++;
    return n;
}
static int craft_buildhp_max(int kind){ return kind==0? 30 : kind==2? 90 : kind==4? 60 : kind==7? 40 : kind==9? 25 : kind==10? 170 : 9999; }
static void craft_build_remove(Buddy *b, int idx){
    if (idx<0 || idx>=b->craft_bn) return;
    b->craft_bn--;
    for (int j=idx;j<b->craft_bn;j++){
        b->craft_btx[j]=b->craft_btx[j+1]; b->craft_bty[j]=b->craft_bty[j+1];
        b->craft_bk[j]=b->craft_bk[j+1]; b->craft_bhp[j]=b->craft_bhp[j+1];
    }
}
// solidity: water drowns everyone without a raft; trees, rocks,
// cacti and the sturdy builds block. for_buddy honours the raft.
static int craft_solid_at(int pi, float x, float y, int for_buddy){
    Buddy *b=&G.bud[pi];
    int tx = bw_tileof(x), ty = bw_tileof(y);
    if (craft_biome_at(b, tx, ty)==5){
        int dbi = craft_build_at(b, tx, ty);
        if (dbi>=0 && b->craft_bk[dbi]==7) return for_buddy? 0 : 1;   // a dock: the buddy walks it, bugs don't
        if (for_buddy && (b->craft_gear&512)) return 0;
        return 1;
    }
    int nd = craft_node_at(pi, tx, ty);
    if (nd==1 || nd==2 || nd==3) return 1;
    int bi = craft_build_at(b, tx, ty);
    if (bi>=0){ int k=b->craft_bk[bi]; if (k==0 || k==2 || k==4 || k==10) return 1; }
    return 0;
}
// hard for bolts: nodes and sturdy builds, water is fine to cross
static int craft_hard_at(int pi, float x, float y){
    Buddy *b=&G.bud[pi];
    int tx = bw_tileof(x), ty = bw_tileof(y);
    int nd = craft_node_at(pi, tx, ty);
    if (nd==1 || nd==2 || nd==3) return 1;
    int bi = craft_build_at(b, tx, ty);
    if (bi>=0){ int k=b->craft_bk[bi]; if (k==0 || k==2 || k==4) return 1; }
    return 0;
}
// the day/night cycle: 240s — dawn [225..15], day, dusk [140..170],
// night [170..225]. darkness 0..1 drives spawns, light and peril.
static float craft_darkf(Buddy *b){
    float t = b->craft_tod;
    if (t<15) return 1.0f - t/15.0f;
    if (t<140) return 0;
    if (t<170) return (t-140)/30.0f;
    if (t<225) return 1.0f;
    return 1.0f - (t-225)/15.0f;
}
static int craft_isnight(Buddy *b){ return craft_darkf(b) > 0.55f; }
// the nearest lit fire (campfire or fireplace) within r of (x, y)
static int craft_fire_near(Buddy *b, float x, float y, float r, float *fx, float *fy){
    for (int i=0;i<b->craft_bn;i++){
        if (b->craft_bk[i]!=1 && b->craft_bk[i]!=3) continue;
        float cx2 = b->craft_btx[i]*BW_TILE+24, cy2 = b->craft_bty[i]*BW_TILE+24;
        float dx=x-cx2, dy=y-cy2;
        if (dx*dx+dy*dy < r*r){ if (fx) *fx=cx2; if (fy) *fy=cy2; return 1; }
    }
    return 0;
}
static void craft_enter(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (b->craft_x==0 && b->craft_y==0){ b->x = 0; b->y = 56; }
    else { b->x = b->craft_x; b->y = b->craft_y; }
    if (b->craft_tod<=0) b->craft_tod = 40;   // first trip starts in the morning
    S->craft_menu=0; S->craft_sel=0; S->craft_cat=0; S->craft_scroll=0; S->craft_biome=-1; S->craft_hitn=0;
    S->craft_search=0; S->craft_filter[0]=0;
    S->craft_bugn=0; S->craft_grace=0; S->craft_bt=2.0f; S->craft_demolish=0;
    S->craft_swarm_t=70; S->craft_cache_t=40; S->craft_cache_on=0; S->craft_haste=0;
    S->craft_darkp=craft_darkf(b); S->craft_beam_t=0; S->craft_prevk=0;
    for (int i=0;i<4;i++) S->craft_bolt_a[i]=0;
    for (int i=0;i<48;i++) S->craft_tz[i]=0;
    S->craft_shn=0; S->craft_sh_t=3.0f; S->craft_bmr_a=0; S->craft_nap=0;
}
static const char *CRAFT_BIOME_NAME[6] = { "THE GREEN REACHES", "THE DEEPWOOD", "THE SUNBAKED STACKS", "THE FROZEN REACHES", "THE MURK", "THE WIDE WATER" };
// ---------------------------------------------------------------------------
// the recipe book: five categories, tiers with prerequisites.
// build >= 0 deploys that build kind; build -2 bakes a pie, -3 cooks
// a stew. bit is the gear flag set on craft (or the "built it once"
// flag for buildings); req is the gear flag you must already hold.
// ---------------------------------------------------------------------------
typedef struct { const char *name; int cat, w, s, f, c, bit, req, build; } CRec;
static const CRec CREC[31] = {
    { "WOODEN AXE",       0, 3,0,0,0, 1,      0,    -1 },   // 0
    { "STONE PICK",       0, 3,3,0,0, 2,      0,    -1 },   // 1
    { "STONE AXE",        0, 2,4,0,0, 16,     1,    -1 },   // 2
    { "CARAPACE PICK",    0, 0,2,0,4, 32,     2,    -1 },   // 3
    { "RAFT",             0, 5,0,3,0, 512,    2048, -1 },   // 4
    { "LANTERN",          0, 1,0,2,0, 8192,   0,    -1 },   // 5: a light of your own, all night
    { "FISHING ROD",      0, 3,0,2,0, 16384,  0,    -1 },   // 6
    { "SAIL RAFT",        0, 4,0,4,0, 262144, 512,  -1 },   // 7: the raft, faster — sharks permitting
    { "CARAPACE SWORD",   1, 2,0,0,4, 4,      0,    -1 },   // 8
    { "CARAPACE SPEAR",   1, 3,0,0,6, 64,     4,    -1 },   // 9
    { "HOTFIX ZAPPER",    1, 0,4,2,4, 128,    32,   -1 },   // 10
    { "BOOMERANG",        1, 3,0,2,0, 32768,  0,    -1 },   // 11: it comes back. They don't.
    { "STORM ZAPPER",     1, 0,2,0,5, 65536,  128,  -1 },   // 12: bolts pierce, and fly faster
    { "FIBER ARMOR",      1, 0,0,6,0, 8,      0,    -1 },   // 13
    { "REINFORCED ARMOR", 1, 0,0,4,4, 256,    8,    -1 },   // 14
    { "SHELL PLATE",      1, 0,0,2,8, 131072, 256,  -1 },   // 15
    { "WOODEN WALL",      2, 4,0,0,0, 1024,   0,    0 },    // 16
    { "STONE WALL",       2, 0,6,0,0, 4096,   2,    2 },    // 17
    { "CAMPFIRE",         2, 5,2,0,0, 2048,   0,    1 },    // 18
    { "FIREPLACE",        2, 4,6,0,0, 0,      2048, 3 },    // 19
    { "TORCH POST",       2, 2,0,1,0, 0,      0,    6 },    // 20
    { "DOCK",             2, 5,0,0,0, 0,      0,    7 },    // 21: the one build that WANTS the water
    { "BEDROLL",          2, 1,0,4,0, 0,      0,    8 },    // 22: a nap anywhere — the wild watches
    { "REINFORCED WALL",  2, 3,3,0,0, 0,      2,    10 },   // 23
    { "LANTERN POST",     2, 0,2,2,0, 0,      0,    11 },   // 24
    { "SPIKE TRAP",       3, 0,2,2,0, 0,      0,    5 },    // 25
    { "ZAPPER TOWER",     3, 0,6,0,3, 0,      4096, 4 },    // 26
    { "SCARECROW",        3, 3,0,3,0, 0,      0,    9 },    // 27: bugs bite IT first
    { "HEART PIE",        4, 0,0,2,1, 0,      0,    -2 },   // 28
    { "CARAPACE STEW",    4, 0,0,2,2, 0,      0,    -3 },   // 29
    { "COOKED FISH",      4, 0,0,0,0, 0,      2048, -5 },   // 30: eats a caught fish, needs a fire first
};
#define CREC_N 31
static const char *CRAFT_CAT_NAME[5] = { "TOOLS", "WEAPONS", "BUILDING", "DEFENSE", "FOOD" };
static const char *craft_reqname(int req){
    switch (req){
        case 1: return "WOODEN AXE"; case 2: return "STONE PICK";
        case 4: return "CARAPACE SWORD"; case 8: return "FIBER ARMOR";
        case 32: return "CARAPACE PICK"; case 1024: return "BUILD A WOODEN WALL";
        case 128: return "HOTFIX ZAPPER"; case 256: return "REINFORCED ARMOR";
        case 512: return "THE RAFT";
        case 2048: return "BUILD A CAMPFIRE"; case 4096: return "BUILD A STONE WALL";
        default: return "MORE CRAFTING";
    }
}
// menu rows for a category: 100 eat pie, 101 eat stew, 102 demolish,
// 103 eat a lake fish
static int craft_rows(Buddy *b, int cat, int *ids){
    int n=0;
    if (cat==4){ if (b->craft_fish>0) ids[n++]=103; if (b->craft_pies>0) ids[n++]=100; if (b->craft_stews>0) ids[n++]=101; }
    for (int i=0;i<CREC_N;i++) if (CREC[i].cat==cat) ids[n++]=i;
    if (cat==2) ids[n++]=102;
    return n;
}
// THE LIST: every category as a segment of ONE long list — a segment
// header is encoded -(cat+1), anything else is a craft_rows id. There
// are no tabs any more: a sloppy diagonal on the pad can only scroll,
// it can never flip you into another list (Daniel's daughter, Nov 6).
static int craft_all_rows(Buddy *b, int *rows){
    int n=0;
    for (int c=0;c<5;c++){
        int ids[16]; int m = craft_rows(b, c, ids);
        if (!m) continue;
        rows[n++] = -(c+1);
        for (int i=0;i<m;i++) rows[n++]=ids[i];
    }
    return n;
}
// the menu's rows, honouring the search filter (Nov 14): with a
// filter the list goes FLAT — matching items only, no segment
// headers — and an empty result is one -99 sentinel row so every
// consumer (nav, render, detail strip) has something safe to hold.
static const char *craft_row_name(int id);
static int craft_ci_contains(const char *hay, const char *needle){
    int nl = slen(needle); if (!nl) return 1;
    for (int i=0; hay[i]; i++){
        int j=0;
        while (needle[j] && hay[i+j]){
            char a = hay[i+j], b2 = needle[j];
            if (a>='a' && a<='z') a -= 32;
            if (b2>='a' && b2<='z') b2 -= 32;
            if (a != b2) break;
            j++;
        }
        if (j==nl) return 1;
    }
    return 0;
}
static int craft_menu_rows(BwSim *S, Buddy *b, int *rows){
    if (!S->craft_filter[0]) return craft_all_rows(b, rows);
    int n=0;
    for (int c=0;c<5;c++){
        int ids[16]; int m = craft_rows(b, c, ids);
        for (int i=0;i<m && n<64;i++)
            if (craft_ci_contains(craft_row_name(ids[i]), S->craft_filter)) rows[n++]=ids[i];
    }
    if (!n){ rows[0] = -99; return 1; }
    return n;
}
// move the selection one CRAFTABLE row in a direction, skipping the
// segment headers, wrapping around the ends of the list
static int craft_step(const int *rows, int n, int from, int dir){
    int i=from;
    for (int t=0;t<n;t++){ i+=dir; if (i>=n) i=0; if (i<0) i=n-1; if (rows[i]>=0) return i; }
    return from;
}
static int craft_row_cat(int id){
    if (id<0) return 0;
    if (id>=100) return id==102? 2 : 4;
    return CREC[id].cat;
}
static int craft_afford(Buddy *b, const CRec *R){
    return b->craft_wood>=R->w && b->craft_stone>=R->s && b->craft_fiber>=R->f && b->craft_car>=R->c;
}
static void craft_spend(Buddy *b, const CRec *R){
    b->craft_wood-=R->w; b->craft_stone-=R->s; b->craft_fiber-=R->f; b->craft_car-=R->c;
}
static void craft_note(BwSim *S, const char *t){ scpy(S->note, t, 72); S->note_t=2.0f; }
static void craft_do(int pi, int id){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (id==100){   // eat a heart pie
        b->craft_pies--; b->hp += b->maxhp*0.5f; if (b->hp>b->maxhp) b->hp=b->maxhp;
        craft_note(S, "HEART PIE. WARM AND BUTTERY. +50% HP"); sfx(11); do_save(); return;
    }
    if (id==101){   // eat a carapace stew
        b->craft_stews--; b->hp += b->maxhp*0.5f; if (b->hp>b->maxhp) b->hp=b->maxhp;
        S->craft_haste = 25.0f;
        craft_note(S, "CARAPACE STEW. WARM, AND SWIFT FOR A WHILE."); sfx(11); do_save(); return;
    }
    if (id==103){   // eat a lake fish, straight from the line
        b->craft_fish--; b->hp += b->maxhp*0.35f; if (b->hp>b->maxhp) b->hp=b->maxhp;
        craft_note(S, "DOWN THE HATCH. +35% HP. SLIGHTLY FISHY."); sfx(11); do_save(); return;
    }
    if (id==102){   // demolish mode: the next interacts take builds apart
        S->craft_demolish = 12.0f; S->craft_menu = 0;
        { char *q=S->note; q=sappend(q, "DEMOLISH ARMED - "); q=sappend(q, lbl_int()); q=sappend(q, " REMOVES A BUILD. HALF BACK."); *q=0; }
        S->note_t=2.4f; sfx(6); return;
    }
    const CRec *R = &CREC[id];
    if (R->req && !(b->craft_gear & R->req)){
        { char *q=S->note; q=sappend(q, "YOU NEED THE "); q=sappend(q, craft_reqname(R->req)); q=sappend(q, " FIRST."); *q=0; }
        S->note_t=1.8f; sfx(16); return;
    }
    if (!craft_afford(b, R)){ craft_note(S, "NOT ENOUGH MATERIALS. GO GATHER."); S->note_t=1.8f; sfx(16); return; }
    if (R->build==-5){   // cooked fish: eats a caught fish, right now, by the fire
        if (b->craft_fish<1){ craft_note(S, "NO FISH TO COOK. THE WATER PROVIDES."); S->note_t=1.8f; sfx(16); return; }
        b->craft_fish--; b->hp += b->maxhp*0.6f; if (b->hp>b->maxhp) b->hp=b->maxhp;
        craft_note(S, "COOKED FISH. CRISP EDGES, WARM MIDDLE. +60% HP"); sfx(11); do_save(); return;
    }
    if (R->build==-2 || R->build==-3){   // food, into the pack
        if (R->build==-2 && b->craft_pies>=99){ craft_note(S, "YOUR PACK IS FULL OF PIES."); sfx(16); return; }
        if (R->build==-3 && b->craft_stews>=99){ craft_note(S, "YOUR PACK IS FULL OF STEWS."); sfx(16); return; }
        craft_spend(b, R);
        if (R->build==-2){ b->craft_pies++; craft_note(S, "A HEART PIE, BAKED AND BAGGED."); }
        else { b->craft_stews++; craft_note(S, "A CARAPACE STEW, SEALED AND STEAMING."); }
        sfx(11); do_save(); return;
    }
    if (R->build<0){   // gear: one of each, forever
        if (b->craft_gear & R->bit){ craft_note(S, "YOU ALREADY CRAFTED THAT."); S->note_t=1.6f; sfx(16); return; }
        craft_spend(b, R);
        b->craft_gear |= R->bit;
        { char *q=S->note; q=sappend(q, "CRAFTED: "); q=sappend(q, R->name); q=sappend(q, "!"); *q=0; }
        S->note_t=2.0f; sfx(11); do_save(); return;
    }
    // a build: fires plant at your feet, everything else on the tile ahead
    int kind = R->build;
    int tx, ty;
    if (kind==1 || kind==3){ tx = bw_tileof(b->x); ty = bw_tileof(b->y); }
    else { tx = bw_tileof(b->x + fcos(b->aim)*BW_TILE); ty = bw_tileof(b->y + fsin(b->aim)*BW_TILE); }
    if (craft_build_at(b, tx, ty)>=0){ craft_note(S, "SOMETHING ALREADY STANDS THERE."); S->note_t=1.8f; sfx(16); return; }
    if (kind==7){   // the dock is the exception: it belongs ON the water
        if (craft_biome_at(b, tx, ty)!=5){ craft_note(S, "A DOCK NEEDS WATER."); S->note_t=1.8f; sfx(16); return; }
    }
    else if (craft_biome_at(b, tx, ty)==5){ craft_note(S, "NOT IN THE WATER."); S->note_t=1.8f; sfx(16); return; }
    if (kind!=1 && kind!=3 && kind!=7){
        if (craft_node_at(pi, tx, ty)){ craft_note(S, "NO ROOM THERE. CLEAR IT FIRST."); S->note_t=1.8f; sfx(16); return; }
        // never entomb the builder: the tile must clear his own body
        float cxp = b->x < tx*BW_TILE? tx*BW_TILE : b->x > tx*BW_TILE+BW_TILE? tx*BW_TILE+BW_TILE : b->x;
        float cyp = b->y < ty*BW_TILE? ty*BW_TILE : b->y > ty*BW_TILE+BW_TILE? ty*BW_TILE+BW_TILE : b->y;
        float dx=b->x-cxp, dy=b->y-cyp;
        if (dx*dx+dy*dy < 23*23){ craft_note(S, "TOO CLOSE - STEP BACK A LITTLE."); S->note_t=1.8f; sfx(16); return; }
    }
    if (b->craft_bn>=48){ craft_note(S, "THAT IS ENOUGH BUILDING FOR ANYONE."); S->note_t=1.8f; sfx(16); return; }
    if (kind==4 && craft_build_kind_count(b, 4)>=3){ craft_note(S, "THREE TOWERS HUM ALREADY."); S->note_t=1.8f; sfx(16); return; }
    if (kind==5 && craft_build_kind_count(b, 5)>=8){ craft_note(S, "ENOUGH SPIKES FOR ONE WILD."); S->note_t=1.8f; sfx(16); return; }
    if (kind==6 && craft_build_kind_count(b, 6)>=8){ craft_note(S, "ENOUGH TORCHES FOR ONE WILD."); S->note_t=1.8f; sfx(16); return; }
    if ((kind==1 || kind==3) && craft_build_kind_count(b, 1)+craft_build_kind_count(b, 3)>=6){ craft_note(S, "ENOUGH FIRES FOR ONE WILD."); S->note_t=1.8f; sfx(16); return; }
    if (kind==7 && craft_build_kind_count(b, 7)>=10){ craft_note(S, "ENOUGH DOCKS FOR ONE WILD."); S->note_t=1.8f; sfx(16); return; }
    if (kind==8 && craft_build_kind_count(b, 8)>=4){ craft_note(S, "ENOUGH BEDROLLS FOR ONE WILD."); S->note_t=1.8f; sfx(16); return; }
    if (kind==9 && craft_build_kind_count(b, 9)>=3){ craft_note(S, "THREE SCARECROWS ARE WATCHING ALREADY."); S->note_t=1.8f; sfx(16); return; }
    if (kind==11 && craft_build_kind_count(b, 11)>=8){ craft_note(S, "ENOUGH LANTERNS FOR ONE WILD."); S->note_t=1.8f; sfx(16); return; }
    craft_spend(b, R);
    b->craft_btx[b->craft_bn]=tx; b->craft_bty[b->craft_bn]=ty;
    b->craft_bk[b->craft_bn]=kind; b->craft_bhp[b->craft_bn]=craft_buildhp_max(kind);
    b->craft_bn++;
    if (R->bit) b->craft_gear |= R->bit;
    if (kind==0) craft_note(S, "A WOODEN WALL! STURDY ENOUGH.");
    else if (kind==2) craft_note(S, "A STONE WALL. TRY CHEWING THAT.");
    else if (kind==1){ char *q=S->note; q=sappend(q, "A CAMPFIRE! STAND CLOSE AND REST WITH "); q=sappend(q, lbl_int()); q=sappend(q, "."); *q=0; S->note_t=2.2f; }
    else if (kind==3) craft_note(S, "A FIREPLACE. THE HEART OF A HOME.");
    else if (kind==4) craft_note(S, "A ZAPPER TOWER! IT GUARDS THIS GROUND.");
    else if (kind==5) craft_note(S, "SPIKES SET. MIND YOUR OWN FEET.");
    else if (kind==7) craft_note(S, "A DOCK! THE WATER IS A FLOOR NOW. MOSTLY.");
    else if (kind==8){ char *q=S->note; q=sappend(q, "A BEDROLL. REST WITH "); q=sappend(q, lbl_int()); q=sappend(q, " - A NAP, NOT A SLEEP."); *q=0; S->note_t=2.2f; }
    else if (kind==9) craft_note(S, "A SCARECROW. BUGS BITE IT FIRST. USUALLY.");
    else if (kind==10) craft_note(S, "A REINFORCED WALL. COME AND CHEW. WE DARE YOU.");
    else if (kind==11) craft_note(S, "A LANTERN POST. THE DARK BACKS UP.");
    else craft_note(S, "A TORCH POST. A LITTLE SUN, ON A STICK.");
    sfx(19); do_save();
}
static void craft_hit_node(int pi, int tx, int ty){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    int nd = craft_node_at(pi, tx, ty);
    if (!nd) return;
    int key = craft_key(tx, ty);
    int tool = 0;   // 0 hands, else the gear bit of the best tool for this node
    if (nd==1) tool = (b->craft_gear&16)? 16 : (b->craft_gear&1)? 1 : 0;
    else if (nd==2) tool = (b->craft_gear&32)? 32 : (b->craft_gear&2)? 2 : 0;
    else if (nd==3 && ((b->craft_gear&16) || (b->craft_gear&32))) tool = 1;
    int base = nd==1? 3 : nd==2? 4 : nd==3? 2 : 1;
    if (tool) base = 1;
    int idx=-1;
    for (int i=0;i<S->craft_hitn;i++) if (S->craft_hk[i]==key) idx=i;
    int left = idx>=0? S->craft_hv[idx] : base;
    left--;
    sfx(4);
    splat(tx*BW_TILE+24, ty*BW_TILE+24, nd==1? COL(90,160,80) : nd==2? COL(150,150,155) : nd==3? COL(110,180,90) : COL(200,220,120), 4, 70);
    if (left>0){
        if (idx<0 && S->craft_hitn<24){ S->craft_hk[S->craft_hitn]=key; S->craft_hv[S->craft_hitn]=left; S->craft_hitn++; }
        else if (idx>=0) S->craft_hv[idx]=left;
        return;
    }
    if (idx>=0){ S->craft_hitn--; S->craft_hk[idx]=S->craft_hk[S->craft_hitn]; S->craft_hv[idx]=S->craft_hv[S->craft_hitn]; }
    if (b->craft_gn<96) b->craft_gone[b->craft_gn++]=key;
    int yw=0, ys=0, yf=0;
    if (nd==1){ yw = (b->craft_gear&16)? 5 : (b->craft_gear&1)? 4 : 3; }
    if (nd==2){ ys = (b->craft_gear&32)? 5 : (b->craft_gear&2)? 4 : 3; }
    if (nd==3) yf = 2;
    if (nd==4) yf = 1;
    b->craft_wood += yw; if (b->craft_wood>999) b->craft_wood=999;
    b->craft_stone += ys; if (b->craft_stone>999) b->craft_stone=999;
    b->craft_fiber += yf; if (b->craft_fiber>999) b->craft_fiber=999;
    { char *q=S->note;
      if (yw){ q=sappend(q, "+"); q=sapp_int(q, yw); q=sappend(q, " WOOD"); }
      else if (ys){ q=sappend(q, "+"); q=sapp_int(q, ys); q=sappend(q, " STONE"); }
      else { q=sappend(q, "+"); q=sapp_int(q, yf); q=sappend(q, " FIBER"); }
      *q=0; S->note_t=1.4f; }
    sfx(11);
    do_save();   // a felled tree is progress; keep it
}
static void craft_loot_cache(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    S->craft_cache_on=0; S->craft_cache_t=55;
    u32 h = hash2((int)(b->x), (int)(G.anim_t*10), craft_seed(b));
    int m1 = (int)(h%4), a1 = 2 + (int)((h>>4)%3);
    int m2 = (int)((h>>8)%4), a2 = 1 + (int)((h>>12)%3);
    int gain[4] = {0,0,0,0}; gain[m1]+=a1; gain[m2]+=a2;
    b->craft_wood += gain[0]; b->craft_stone += gain[1]; b->craft_fiber += gain[2]; b->craft_car += gain[3];
    if (b->craft_wood>999) b->craft_wood=999; if (b->craft_stone>999) b->craft_stone=999;
    if (b->craft_fiber>999) b->craft_fiber=999; if (b->craft_car>999) b->craft_car=999;
    static const char *MN[4] = { "WOOD", "STONE", "FIBER", "CARAPACE" };
    { char *q=S->note; int first=1;
      for (int i=0;i<4;i++) if (gain[i]){ if (!first) q=sappend(q, " +"); else q=sappend(q, "+");
          q=sapp_int(q, gain[i]); q=sappend(q, " "); q=sappend(q, MN[i]); first=0; }
      q=sappend(q, " - A SUPPLY CACHE!"); *q=0; S->note_t=2.2f; }
    sfx(11); do_save();
}
static void craft_demolish_at(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    int txs[2] = { bw_tileof(b->x + fcos(b->aim)*BW_TILE), bw_tileof(b->x) };
    int tys[2] = { bw_tileof(b->y + fsin(b->aim)*BW_TILE), bw_tileof(b->y) };
    static const int KIND2REC[12] = { 16, 18, 17, 19, 26, 25, 20, 21, 22, 27, 23, 24 };
    for (int t=0;t<2;t++){
        int bi = craft_build_at(b, txs[t], tys[t]);
        if (bi<0) continue;
        const CRec *R = &CREC[KIND2REC[b->craft_bk[bi]%12]];
        int rw=R->w/2, rs=R->s/2, rf=R->f/2, rc=R->c/2;
        b->craft_wood+=rw; b->craft_stone+=rs; b->craft_fiber+=rf; b->craft_car+=rc;
        splat(txs[t]*BW_TILE+24, tys[t]*BW_TILE+24, COL(170,140,90), 8, 90);
        craft_build_remove(b, bi);
        { char *q=S->note; q=sappend(q, "DEMOLISHED.");
          if (rw){ q=sappend(q, " +"); q=sapp_int(q, rw); q=sappend(q, "W"); }
          if (rs){ q=sappend(q, " +"); q=sapp_int(q, rs); q=sappend(q, "S"); }
          if (rf){ q=sappend(q, " +"); q=sapp_int(q, rf); q=sappend(q, "F"); }
          if (rc){ q=sappend(q, " +"); q=sapp_int(q, rc); q=sappend(q, "C"); }
          q=sappend(q, " BACK."); *q=0; S->note_t=2.0f; }
        S->craft_demolish=0; sfx(4); do_save(); return;
    }
    craft_note(S, "NOTHING TO DEMOLISH THERE."); S->note_t=1.4f; sfx(16);
}
static void craft_bug_die(int pi, int i){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    int kind = S->craft_bk[i];
    splat(S->craft_bx[i], S->craft_by[i], kind==2? COL(250,205,80) : COL(150,80,70), 6, 90);
    if (kind==2){
        b->craft_car += 5; if (b->craft_car>999) b->craft_car=999;
        b->craft_fiber += 3; if (b->craft_fiber>999) b->craft_fiber=999;
        craft_note(S, "A GOLDEN BUG! +5 CARAPACE +3 FIBER");
    } else {
        int gain = kind==1? 2 + (int)(hash2(i*7+1, (int)(b->x), craft_seed(b))%2)
                           : 1 + (int)(hash2(i*7+1, (int)(b->x), craft_seed(b))%2);
        b->craft_car += gain; if (b->craft_car>999) b->craft_car=999;
        { char *q=S->note; q=sappend(q, "+"); q=sapp_int(q, gain); q=sappend(q, " CARAPACE"); *q=0; S->note_t=1.4f; }
    }
    S->craft_bhp[i]=0;
    sfx(11);
}
static void craft_spawn_bug(int pi, float ang, float dist, int force_kind){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->craft_bugn>=14) return;
    float nx = b->x + fcos(ang)*dist, ny = b->y + fsin(ang)*dist;
    if (craft_solid_at(pi, nx, ny, 0)) return;
    int i=S->craft_bugn++;
    int kind = force_kind;
    if (kind<0){
        u32 h = hash2((int)(nx*3), (int)(ny*3) + (int)(G.anim_t*7), craft_seed(b));
        int r = (int)(h%100);
        kind = r<4? 2 : (craft_isnight(b) && r<44)? 1 : 0;
    }
    S->craft_bx[i]=nx; S->craft_by[i]=ny; S->craft_bk[i]=kind;
    S->craft_bhp[i] = kind==1? 4 : kind==2? 1 : 2;
    S->craft_bph[i] = hash_f(i*13, (int)(nx), craft_seed(b))*6.28f;
    S->craft_btr[i]=0;
}
static int craft_bedroll_near(Buddy *b){
    for (int i=0;i<b->craft_bn;i++){
        if (b->craft_bk[i]!=8) continue;
        float dx=b->x-(b->craft_btx[i]*BW_TILE+24), dy=b->y-(b->craft_bty[i]*BW_TILE+24);
        if (dx*dx+dy*dy < 70*70) return 1;
    }
    return 0;
}
static void craft_fish_at(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    float r = hash_f((int)(b->x/24), (int)(b->y/24) + (int)(G.anim_t*7), craft_seed(b));
    sfx(12);
    splat(b->x + fcos(b->aim)*34, b->y + fsin(b->aim)*34, COL(170,220,255), 4, 60);
    if (r < 0.45f){ b->craft_fish++; craft_note(S, "CAUGHT A LAKE FISH!"); }
    else if (r < 0.70f){ b->craft_fiber++; craft_note(S, "SOGGY FIBER. THE LAKE PROVIDES. SORT OF."); }
    else if (r < 0.85f){ b->craft_car++; craft_note(S, "A SNAPPING TURTLE-BUG! +1 CARAPACE."); }
    else craft_note(S, "IT GOT AWAY. THE ONE THAT GOT AWAY WAS HUGE.");
    S->note_t = 2.0f;
    do_save();
}
static void craft_shark_die(int pi, int i){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    splat(S->craft_sh_x[i], S->craft_sh_y[i], COL(190,200,210), 10, 110); sfx(17);
    b->craft_car += 2; if ((i&1)==0) b->craft_car += 1;
    b->craft_fiber += 1;
    craft_note(S, "SHARK DOWN! THE TEETH STOP HERE. +CARAPACE");
    S->craft_sh_hp[i]=0;
    do_save();
}
static void craft_shark_spawn(int pi, float ang, float dist){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->craft_shn>=6) return;
    for (int t=0;t<6;t++){
        float a = ang + t*1.05f;
        float x = b->x + fcos(a)*dist, y = b->y + fsin(a)*dist;
        if (craft_biome_at(b, bw_tileof(x), bw_tileof(y))!=5) continue;
        int i=S->craft_shn++;
        S->craft_sh_x[i]=x; S->craft_sh_y[i]=y; S->craft_sh_hp[i]=3;
        S->craft_sh_ph[i]=hash_f((int)x, (int)y, craft_seed(b))*6.28f; S->craft_sh_tr[i]=0; S->craft_sh_hd[i]=a;
        return;
    }
}
static void craft_update(int pi, float dt, const BwIn *in){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (in->exit_ && !S->craft_menu){ bw_exit(pi, 0); return; }
    if (S->craft_grace>0) S->craft_grace -= dt;
    if (b->atk_cd>0) b->atk_cd -= dt;
    if (b->lunge_t>0) b->lunge_t -= dt;
    if (b->hurt_t>0) b->hurt_t -= dt;
    if (S->craft_haste>0) S->craft_haste -= dt;
    if (S->craft_demolish>0) S->craft_demolish -= dt;
    if (S->craft_beam_t>0) S->craft_beam_t -= dt;
    // the day turns, in this mode only
    b->craft_tod += dt; if (b->craft_tod>=240) b->craft_tod -= 240;
    { float dk = craft_darkf(b);
      if (dk>0.6f && S->craft_darkp<=0.6f) craft_note(S, "NIGHT FALLS. THE WILD GETS HUNGRY.");
      S->craft_darkp = dk; }
    // woke from a rest (bw_update ran the sleep itself)
    if (S->slept){
        S->slept = 0;
        if (S->craft_nap){   // a bedroll nap: time passes, the night does not skip
            S->craft_nap = 0;
            b->craft_tod += 90; if (b->craft_tod>=240) b->craft_tod -= 240;
            scpy(S->note, "A FITFUL NAP UNDER THE STARS.", 72); S->note_t=2.6f;
            if (craft_darkf(b)>0.4f){   // and something noticed you sleeping out there
                craft_spawn_bug(pi, hash_f((int)(b->x/48), (int)(G.anim_t*10), craft_seed(b))*6.2832f, 300, -1);
                scpy(S->note, "YOU WAKE TO COMPANY. THE WILD FOUND YOU.", 72); S->note_t=2.6f;
            }
        }
        else if (craft_darkf(b)>0.4f){
            b->craft_tod = 232;   // slept through the dangerous hours
            scpy(S->note, "YOU SLEPT TILL DAWN. GOOD AS NEW.", 72); S->note_t=2.6f;
        }
    }
    // the crafting menu swallows the rest of the input
    if (S->craft_menu){
        // ONE list with segment headers (craft_all_rows). Up/down are
        // the only way the selection moves — left/right do nothing in
        // here on purpose, so a sensitive dpad can never flip lists.
        int rows[64]; int n = craft_menu_rows(S, b, rows);
        // SEARCH (Nov 14): "/" (or a tap on the search row) starts
        // it — P itself belongs to the BW menu and never reaches
        // this update. Typed letters filter the list flat: the fix
        // for scrolling thirty rows on a phone. While searching,
        // typed characters are text, never commands.
        int typed_now = 0;
        if (!S->craft_search && pi==G.my_id){
            for (int i=0;i<IN.typed_n;i++) if (IN.typed[i]=='/'){ S->craft_search = 1; break; }
        }
        if (S->craft_search){
            if (pi==G.my_id){
                for (int i=0;i<IN.typed_n;i++){
                    char c = IN.typed[i];
                    if (c>='a' && c<='z') c -= 32;
                    if ((c>='A' && c<='Z') || (c>='0' && c<='9') || c==' '){
                        int L = slen(S->craft_filter);
                        if (L<12){ S->craft_filter[L]=c; S->craft_filter[L+1]=0; typed_now=1; }
                    }
                }
                if (IN.backspace){ int L = slen(S->craft_filter); if (L>0) S->craft_filter[L-1]=0; typed_now=1; }
            }
            if (typed_now){ S->craft_sel=0; S->craft_scroll=0; n = craft_menu_rows(S, b, rows); }
            // leaving search clears the filter: the soft-keyboard
            // session restarts from an empty field every time, so a
            // standing filter could never be edited again — done
            // searching means the whole list comes back
            if (!typed_now && in->exit_){ S->craft_search=0; S->craft_filter[0]=0; S->craft_sel=0; S->craft_scroll=0; S->craft_prevk = in->keys; return; }
        }
        if (S->craft_sel>=n) S->craft_sel=0;
        if (rows[S->craft_sel]<0) S->craft_sel = craft_step(rows, n, S->craft_sel, 1);
        if (in->up){ S->craft_sel = craft_step(rows, n, S->craft_sel, -1); sfx(6); }
        if (in->down){ S->craft_sel = craft_step(rows, n, S->craft_sel, 1); sfx(6); }
        S->craft_cat = craft_row_cat(rows[S->craft_sel]);   // kept truthful for the detail strip/tests
        // keep the selection inside the visible window of the list
        if (S->craft_sel < S->craft_scroll) S->craft_scroll = S->craft_sel;
        if (S->craft_sel > S->craft_scroll+6) S->craft_scroll = S->craft_sel-6;
        if (n>7){ if (S->craft_scroll > n-7) S->craft_scroll = n-7; } else S->craft_scroll = 0;
        if (S->craft_scroll<0) S->craft_scroll=0;
        // touch: the menu is a real touch menu (both schemes) — tap a
        // row to pick it, tap it again to craft, tap a segment header
        // to hop to that segment, tap outside to close. Geometry
        // mirrors craft_render exactly.
        if (pi==G.my_id && IN.tapped){
            int vx,vy,vw2,vh2; p1_view_rect(&vx,&vy,&vw2,&vh2);
            int visn = n<7? n : 7;
            int w=380, h=40+visn*20+16+24+22;
            int x=vx+(vw2-w)/2, y=vy+(vh2-h)/2;
            if (w>vw2-8){ w=vw2-8; x=vx+4; }
            int tx=IN.tap_x, ty=IN.tap_y; IN.tapped=0;
            if (tx>=x+10 && tx<=x+w-10 && ty>=y+28 && ty<y+50){
                // the search row toggles typing; ending a search
                // clears its filter (see the keyboard path)
                S->craft_search = !S->craft_search;
                if (!S->craft_search){ S->craft_filter[0]=0; S->craft_sel=0; S->craft_scroll=0; }
                sfx(6);
            }
            else if (tx<x || tx>x+w || ty<y || ty>y+h){ S->craft_menu=0; S->craft_search=0; sfx(6); }
            else for (int r=S->craft_scroll; r<S->craft_scroll+visn && r<n; r++){
                int ry = y+52+(r-S->craft_scroll)*20;
                if (ty>=ry-4 && ty<ry+15){
                    if (rows[r]<0){ S->craft_sel = craft_step(rows, n, r, 1); sfx(6); }
                    else if (r==S->craft_sel) craft_do(pi, rows[r]);
                    else { S->craft_sel=r; sfx(6); }
                    break;
                }
            }
            S->craft_prevk = in->keys;
            return;
        }
        if ((in->sel || in->interact) && !typed_now && rows[S->craft_sel]>=0) craft_do(pi, rows[S->craft_sel]);
        if (in->exit_ || in->wall){ S->craft_menu=0; S->craft_search=0; }
        S->craft_prevk = in->keys;
        return;
    }
    if (in->wall){ int rows[64]; int n = craft_all_rows(b, rows);
        S->craft_menu=1; S->craft_sel = n>1? craft_step(rows, n, 0, 1) : 0; S->craft_scroll=0; S->craft_demolish=0; S->craft_filter[0]=0; S->craft_search=0; sfx(6); S->craft_prevk = in->keys; return; }
    // movement, meadow-style, against the wild's own solids
    float mx=0, my=0; int k=in->keys;
    if (k&K_UP) my-=1; if (k&K_DOWN) my+=1; if (k&K_LEFT) mx-=1; if (k&K_RIGHT) mx+=1;
    int on_dock = 0;
    { int dbi = craft_build_at(b, bw_tileof(b->x), bw_tileof(b->y)); if (dbi>=0 && b->craft_bk[dbi]==7) on_dock = 1; }
    int on_water = craft_biome_at(b, bw_tileof(b->x), bw_tileof(b->y))==5 && (b->craft_gear&512) && !on_dock;
    if (mx!=0 || my!=0){
        float d=fsqrt(mx*mx+my*my); mx/=d; my/=d;
        b->aim = fatan2(my, mx);
        float sp = BUD_SPD[b->species%BUDN];
        if (on_water) sp *= (b->craft_gear&262144)? 0.88f : 0.62f;   // a sail makes the raft honest
        if (S->craft_haste>0) sp *= 1.35f;
        float nx = b->x + mx*sp*dt, ny = b->y + my*sp*dt;
        if (!craft_solid_at(pi, nx, b->y, 1)) b->x = nx;
        if (!craft_solid_at(pi, b->x, ny, 1)) b->y = ny;
        b->walk_ph += dt*9;
        b->mv_t = 0.15f;
    }
    // backstop: never leave the builder inside a solid tile — shove
    // him out by the nearest open edge (a placed wall used to be
    // able to entomb him; placement refuses now, this is the belt
    // to its suspenders)
    if (craft_solid_at(pi, b->x, b->y, 1)){
        int tx=bw_tileof(b->x), ty=bw_tileof(b->y);
        float cand[4][2] = { { tx*BW_TILE-16.0f, b->y }, { (tx+1)*BW_TILE+16.0f, b->y },
                             { b->x, ty*BW_TILE-16.0f }, { b->x, (ty+1)*BW_TILE+16.0f } };
        float best=1e9f; int bi2=-1;
        for (int c2=0;c2<4;c2++){
            if (craft_solid_at(pi, cand[c2][0], cand[c2][1], 1)) continue;
            float dx=cand[c2][0]-b->x, dy=cand[c2][1]-b->y, d2=dx*dx+dy*dy;
            if (d2<best){ best=d2; bi2=c2; }
        }
        if (bi2>=0){ b->x=cand[bi2][0]; b->y=cand[bi2][1]; }
    }
    if (b->energy<BUD_ENERGY_MAX){ b->energy += BUD_EN_REGEN*dt; if (b->energy>BUD_ENERGY_MAX) b->energy=BUD_ENERGY_MAX; }
    b->bw_x = b->x; b->bw_y = b->y;
    b->calm += dt;
    if (b->calm>4.0f && b->hp>0 && b->hp<b->maxhp){ b->hp += 2.2f*dt; if (b->hp>b->maxhp) b->hp=b->maxhp; }
    // fires warm you faster — the fireplace reaches further
    for (int i=0;i<b->craft_bn;i++){
        if (b->craft_bk[i]!=1 && b->craft_bk[i]!=3) continue;
        float r = b->craft_bk[i]==3? 112.0f : 74.0f, heal = b->craft_bk[i]==3? 10.0f : 6.0f;
        float dx=b->x-(b->craft_btx[i]*BW_TILE+24), dy=b->y-(b->craft_bty[i]*BW_TILE+24);
        if (dx*dx+dy*dy < r*r && b->hp>0 && b->hp<b->maxhp){ b->hp += heal*dt; if (b->hp>b->maxhp) b->hp=b->maxhp; }
    }
    // where are we?
    int biome = craft_biome_at(b, bw_tileof(b->x), bw_tileof(b->y));
    if (biome != S->craft_biome){
        S->craft_biome = biome;
        if (biome!=5){ scpy(S->note, CRAFT_BIOME_NAME[biome], 72); S->note_t = 2.4f; }
    }
    // hands: interact gathers, rests, loots, demolishes; the fire
    // wins ties — at camp, the interact button means REST
    int ttx = bw_tileof(b->x + fcos(b->aim)*34), tty = bw_tileof(b->y + fsin(b->aim)*34);
    if (in->interact && b->atk_cd<=0){
        b->atk_cd = 0.38f;
        if (S->craft_demolish>0) craft_demolish_at(pi);
        else if (craft_fire_near(b, b->x, b->y, 84, 0, 0)){ S->sleep_t = 1.4f; S->slept = 0; S->craft_nap = 0; sfx(11); }
        else if (craft_bedroll_near(b)){ S->sleep_t = 1.4f; S->slept = 0; S->craft_nap = 1; sfx(11); }
        else if ((b->craft_gear&16384) && craft_biome_at(b, ttx, tty)==5) craft_fish_at(pi);
        else if (S->craft_cache_on){
            float dx=S->craft_cache_x-b->x, dy=S->craft_cache_y-b->y;
            if (dx*dx+dy*dy < 50*50) craft_loot_cache(pi);
            else if (craft_node_at(pi, ttx, tty)) craft_hit_node(pi, ttx, tty);
        }
        else if (craft_node_at(pi, ttx, tty)) craft_hit_node(pi, ttx, tty);
    }
    if (in->sel && b->atk_cd<=0){
        if ((b->craft_gear&128) && S->craft_demolish<=0){   // the zapper shoots instead of swinging
            int storm = (b->craft_gear&65536)? 1 : 0;
            b->atk_cd = storm? 0.22f : 0.34f; b->lunge_t = 0.12f;
            for (int i=0;i<4;i++) if (!S->craft_bolt_a[i]){
                S->craft_bolt_a[i]=1; S->craft_bolt_t[i]=0.55f;
                S->craft_bolt_x[i]=b->x + fcos(b->aim)*16; S->craft_bolt_y[i]=b->y + fsin(b->aim)*16;
                S->craft_bolt_vx[i]=fcos(b->aim)*330; S->craft_bolt_vy[i]=fsin(b->aim)*330;
                S->craft_bolt_pc[i]=storm; S->craft_bolt_hc[i]=0;
                sfx(12); break;
            }
        } else if ((b->craft_gear&32768) && !S->craft_bmr_a && S->craft_demolish<=0){   // the boomerang goes out — and comes back
            b->atk_cd = 0.5f; b->lunge_t = 0.14f;
            S->craft_bmr_a=1; S->craft_bmr_t=0;
            S->craft_bmr_x=b->x + fcos(b->aim)*16; S->craft_bmr_y=b->y + fsin(b->aim)*16;
            S->craft_bmr_vx=fcos(b->aim)*360; S->craft_bmr_vy=fsin(b->aim)*360;
            sfx(12);
        } else {
            b->atk_cd = 0.42f; b->lunge_t = 0.16f;
            int dmg = (b->craft_gear&64)? 3 : (b->craft_gear&4)? 2 : 1;
            float reach = (b->craft_gear&64)? 70.0f : 52.0f;
            if (S->craft_cache_on){
                float dx=S->craft_cache_x-b->x, dy=S->craft_cache_y-b->y;
                if (dx*dx+dy*dy < (reach+22)*(reach+22)) craft_loot_cache(pi);
            }
            for (int i=0;i<S->craft_bugn;i++){
                if (S->craft_bhp[i]<=0) continue;
                float dx=S->craft_bx[i]-b->x, dy=S->craft_by[i]-b->y;
                float d2 = dx*dx+dy*dy;
                if (d2 < reach*reach){
                    float dl = fsqrt(d2)+0.01f;
                    if ((dx*fcos(b->aim) + dy*fsin(b->aim))/dl > 0.15f || d2 < 24*24){
                        S->craft_bhp[i] -= dmg; sfx(4);
                        splat(S->craft_bx[i], S->craft_by[i], COL(150,80,70), 5, 80);
                        if (S->craft_bhp[i]<=0) craft_bug_die(pi, i);
                    }
                }
            }
            for (int i=0;i<S->craft_shn;i++){   // a swing reaches the sharks at the raft's edge too
                if (S->craft_sh_hp[i]<=0) continue;
                float dx=S->craft_sh_x[i]-b->x, dy=S->craft_sh_y[i]-b->y;
                float d2 = dx*dx+dy*dy;
                if (d2 < (reach+8)*(reach+8)){
                    float dl = fsqrt(d2)+0.01f;
                    if ((dx*fcos(b->aim) + dy*fsin(b->aim))/dl > 0.15f || d2 < 26*26){
                        S->craft_sh_hp[i] -= dmg; sfx(4);
                        splat(S->craft_sh_x[i], S->craft_sh_y[i], COL(190,200,210), 5, 80);
                        if (S->craft_sh_hp[i]<=0) craft_shark_die(pi, i);
                    }
                }
            }
            if (craft_node_at(pi, ttx, tty)) craft_hit_node(pi, ttx, tty);
        }
    }
    // zapper bolts in flight (storm bolts pierce one extra target)
    for (int i=0;i<4;i++) if (S->craft_bolt_a[i]){
        S->craft_bolt_t[i] -= dt;
        if (S->craft_bolt_hc[i]>0) S->craft_bolt_hc[i] -= dt;
        S->craft_bolt_x[i] += S->craft_bolt_vx[i]*dt;
        S->craft_bolt_y[i] += S->craft_bolt_vy[i]*dt;
        if (S->craft_bolt_t[i]<=0 || craft_hard_at(pi, S->craft_bolt_x[i], S->craft_bolt_y[i])){ S->craft_bolt_a[i]=0; continue; }
        if (S->craft_bolt_hc[i]>0) continue;
        int consumed = 0;
        for (int j=0;j<S->craft_bugn && !consumed;j++){
            if (S->craft_bhp[j]<=0) continue;
            float dx=S->craft_bx[j]-S->craft_bolt_x[i], dy=S->craft_by[j]-S->craft_bolt_y[i];
            if (dx*dx+dy*dy < 15*15){
                S->craft_bhp[j] -= 2; sfx(4);
                splat(S->craft_bx[j], S->craft_by[j], COL(255,170,80), 5, 80);
                if (S->craft_bhp[j]<=0) craft_bug_die(pi, j);
                if (S->craft_bolt_pc[i]>0){ S->craft_bolt_pc[i]--; S->craft_bolt_hc[i]=0.18f; }
                else { S->craft_bolt_a[i]=0; consumed=1; }
            }
        }
        for (int j=0;j<S->craft_shn && !consumed && S->craft_bolt_a[i];j++){
            if (S->craft_sh_hp[j]<=0) continue;
            float dx=S->craft_sh_x[j]-S->craft_bolt_x[i], dy=S->craft_sh_y[j]-S->craft_bolt_y[i];
            if (dx*dx+dy*dy < 17*17){
                S->craft_sh_hp[j] -= 2; sfx(4);
                splat(S->craft_sh_x[j], S->craft_sh_y[j], COL(255,170,80), 5, 80);
                if (S->craft_sh_hp[j]<=0) craft_shark_die(pi, j);
                if (S->craft_bolt_pc[i]>0){ S->craft_bolt_pc[i]--; S->craft_bolt_hc[i]=0.18f; }
                else S->craft_bolt_a[i]=0;
            }
        }
    }
    // the boomerang: out fast, then it homes back to the hand
    if (S->craft_bmr_a){
        S->craft_bmr_t += dt;
        if (S->craft_bmr_t < 0.42f){
            S->craft_bmr_x += S->craft_bmr_vx*dt; S->craft_bmr_y += S->craft_bmr_vy*dt;
            if (craft_hard_at(pi, S->craft_bmr_x, S->craft_bmr_y)) S->craft_bmr_t = 0.42f;
        } else {
            float dx=b->x-S->craft_bmr_x, dy=b->y-S->craft_bmr_y, d2=dx*dx+dy*dy;
            if (d2 < 22*22 || S->craft_bmr_t > 2.2f){ S->craft_bmr_a=0; }
            else { float dl=fsqrt(d2)+0.01f; S->craft_bmr_x += dx/dl*400*dt; S->craft_bmr_y += dy/dl*400*dt; }
        }
        if (S->craft_bmr_a){
            for (int j=0;j<S->craft_bugn;j++){
                if (S->craft_bhp[j]<=0 || S->craft_btr[j]>0.25f) continue;
                float dx=S->craft_bx[j]-S->craft_bmr_x, dy=S->craft_by[j]-S->craft_bmr_y;
                if (dx*dx+dy*dy < 17*17){
                    S->craft_bhp[j] -= 2; S->craft_btr[j]=0.3f; sfx(4);
                    splat(S->craft_bx[j], S->craft_by[j], COL(255,190,90), 5, 80);
                    if (S->craft_bhp[j]<=0) craft_bug_die(pi, j);
                }
            }
            for (int j=0;j<S->craft_shn;j++){
                if (S->craft_sh_hp[j]<=0 || S->craft_sh_tr[j]>0) continue;
                float dx=S->craft_sh_x[j]-S->craft_bmr_x, dy=S->craft_sh_y[j]-S->craft_bmr_y;
                if (dx*dx+dy*dy < 18*18){
                    S->craft_sh_hp[j] -= 2; S->craft_sh_tr[j]=0.3f; sfx(4);
                    splat(S->craft_sh_x[j], S->craft_sh_y[j], COL(255,190,90), 5, 80);
                    if (S->craft_sh_hp[j]<=0) craft_shark_die(pi, j);
                }
            }
        }
    }
    // feral bugs: the wild is not empty, and the night is worse
    int night = craft_isnight(b);
    S->craft_bt -= dt;
    if (S->craft_bt<=0){
        S->craft_bt = night? 0.9f : 2.5f;
        if (S->craft_bugn < (night? 13 : 8)){
            float a = hash_f((int)(b->x/48), (int)(G.anim_t*10), craft_seed(b)) * 6.2832f;
            craft_spawn_bug(pi, a, 500, -1);
        }
    }
    // the occasional swarm keeps everyone honest
    S->craft_swarm_t -= dt;
    if (S->craft_swarm_t<=0){
        S->craft_swarm_t = 85 + hash_f((int)(b->x/48), (int)(b->y/48), craft_seed(b))*40;
        int n2 = night? 7 : 5;
        for (int s2=0;s2<n2;s2++) craft_spawn_bug(pi, s2*1.13f + b->x*0.01f, 260, -1);
        craft_note(S, "A SWARM IS COMING!"); S->note_t=2.4f; sfx(16);
    }
    // supply caches drift in now and then
    S->craft_cache_t -= dt;
    if (!S->craft_cache_on && S->craft_cache_t<=0){
        int placed=0;
        for (int t2=0;t2<8 && !placed;t2++){
            float a = hash_f((int)(G.anim_t*3)+t2*17, (int)(b->y/48), craft_seed(b)) * 6.2832f;
            float dd = 230 + hash_f(t2*31, (int)(b->x/48), craft_seed(b))*90;
            float cx2 = b->x + fcos(a)*dd, cy2 = b->y + fsin(a)*dd;
            if (!craft_solid_at(pi, cx2, cy2, 0) && craft_biome_at(b, bw_tileof(cx2), bw_tileof(cy2))!=5){
                S->craft_cache_x=cx2; S->craft_cache_y=cy2; S->craft_cache_on=1; S->craft_cache_t=75; placed=1;
            }
        }
        if (!placed) S->craft_cache_t=20;
    }
    if (S->craft_cache_on && S->craft_cache_t<=0){ S->craft_cache_on=0; S->craft_cache_t=50; }
    for (int i=0;i<S->craft_bugn;i++){
        if (S->craft_bhp[i]<=0) continue;
        int bk = S->craft_bk[i];
        float dx=b->x-S->craft_bx[i], dy=b->y-S->craft_by[i];
        float d2=dx*dx+dy*dy;
        S->craft_bph[i] += dt*0.7f;
        if (S->craft_btr[i]>0) S->craft_btr[i] -= dt;
        // standing on spikes hurts and slows (checked before moving)
        int on_trap = craft_build_at(b, bw_tileof(S->craft_bx[i]), bw_tileof(S->craft_by[i]))>=0
                   && b->craft_bk[craft_build_at(b, bw_tileof(S->craft_bx[i]), bw_tileof(S->craft_by[i]))]==5;
        float mvx, mvy, sp;
        float chase_r = bk==1? 300.0f : 240.0f;
        // a scarecrow is easier to catch than a buddy, and bugs know it
        float txp=b->x, typ=b->y, td2=d2; int decoy=-1;
        if (bk!=2){
            for (int j2=0;j2<b->craft_bn;j2++){
                if (b->craft_bk[j2]!=9) continue;
                float sx2=b->craft_btx[j2]*BW_TILE+24, sy2=b->craft_bty[j2]*BW_TILE+24;
                float sdx=sx2-S->craft_bx[i], sdy=sy2-S->craft_by[i], sd2=sdx*sdx+sdy*sdy;
                if (sd2<d2 && sd2<320*320){ txp=sx2; typ=sy2; td2=sd2; decoy=j2; }
            }
        }
        float tdx=txp-S->craft_bx[i], tdy=typ-S->craft_by[i];
        if (bk==2 && d2 < 210*210){ float dl=fsqrt(d2)+0.01f; mvx=-dx/dl; mvy=-dy/dl; sp=125.0f; }   // the golden one runs
        else if (td2 < chase_r*chase_r){ float dl=fsqrt(td2)+0.01f; mvx=tdx/dl; mvy=tdy/dl; sp = bk==1? 74.0f : 56.0f; }
        else { mvx=fcos(S->craft_bph[i]); mvy=fsin(S->craft_bph[i]); sp = bk==1? 30.0f : 22.0f; }
        if (on_trap) sp *= 0.55f;
        float nx = S->craft_bx[i] + mvx*sp*dt, ny = S->craft_by[i] + mvy*sp*dt;
        if (!craft_solid_at(pi, nx, S->craft_by[i], 0)) S->craft_bx[i]=nx;
        if (!craft_solid_at(pi, S->craft_bx[i], ny, 0)) S->craft_by[i]=ny;
        if (on_trap && S->craft_btr[i]<=0){
            S->craft_bhp[i] -= 1; S->craft_btr[i]=0.7f; sfx(4);
            if (S->craft_bhp[i]<=0){ craft_bug_die(pi, i); continue; }
        }
        if (decoy>=0 && td2 < 27*27 && S->craft_btr[i]<=0){
            S->craft_btr[i]=0.8f; b->craft_bhp[decoy] -= 1; sfx(4);
            if (b->craft_bhp[decoy]<=0){ craft_build_remove(b, decoy); craft_note(S, "THE SCARECROW IS DOWN!"); S->note_t=2.0f; do_save(); }
        }
        // after dark, bugs chew through builds that pen them in
        if (night && bk!=2){
            for (int j=0;j<b->craft_bn;j++){
                int kk2=b->craft_bk[j];
                if (kk2!=0 && kk2!=2 && kk2!=4 && kk2!=10) continue;
                float cx2=b->craft_btx[j]*BW_TILE+24, cy2=b->craft_bty[j]*BW_TILE+24;
                float ddx=S->craft_bx[i]-cx2, ddy=S->craft_by[i]-cy2;
                if (ddx*ddx+ddy*ddy < 42*42 && S->craft_btr[i]<=0){
                    b->craft_bhp[j] -= 1;
                    S->craft_btr[i] = (kk2==2||kk2==10)? 0.7f : kk2==4? 0.35f : 0.22f;
                    sfx(4);
                    if (b->craft_bhp[j]<=0){
                        craft_build_remove(b, j);
                        craft_note(S, "SOMETHING WAS CHEWED THROUGH IN THE DARK!"); S->note_t=2.2f; sfx(16); do_save();
                    }
                    break;
                }
            }
        }
        if (d2 < 25*25 && S->craft_grace<=0 && !b->fainted && bk!=2){
            float dmgf = (bk==1? 0.16f : 0.12f) * ((b->craft_gear&131072)? 0.15f : (b->craft_gear&256)? 0.3f : (b->craft_gear&8)? 0.5f : 1.0f);
            b->hp -= b->maxhp * dmgf; b->calm = 0; b->hurt_t = 0.3f;
            S->craft_grace = 1.3f; sfx(16);
            scpy(S->note, (b->craft_gear&8) || (b->craft_gear&256) || (b->craft_gear&131072)? "THE ARMOR TOOK THE BITE." : "A FERAL BUG BIT YOU!", 72); S->note_t=1.8f;
            if (b->hp<=0){ b->hp=0; b->fainted=1; bw_exit(pi, 1); return; }
        }
        if (d2 > 900*900) S->craft_bhp[i]=0;   // wandered off for good
    }
    // sharks: the water was never safe, it was just unguarded
    if (on_water){
        S->craft_sh_t -= dt;
        if (S->craft_sh_t<=0){
            S->craft_sh_t = 2.2f;
            if (S->craft_shn<4) craft_shark_spawn(pi, hash_f((int)(b->x/48), (int)(G.anim_t*10), craft_seed(b))*6.2832f, 300.0f);
        }
    }
    for (int i=0;i<S->craft_shn;i++){
        if (S->craft_sh_hp[i]<=0) continue;
        if (S->craft_sh_tr[i]>0) S->craft_sh_tr[i] -= dt;
        S->craft_sh_ph[i] += dt*0.9f;
        float dx=b->x-S->craft_sh_x[i], dy=b->y-S->craft_sh_y[i], d2=dx*dx+dy*dy;
        float mvx, mvy, sp;
        if (d2 < 380*380){ float dl=fsqrt(d2)+0.01f; mvx=dx/dl; mvy=dy/dl; sp=97.0f; }
        else { mvx=fcos(S->craft_sh_ph[i]); mvy=fsin(S->craft_sh_ph[i]); sp=26.0f; }
        S->craft_sh_hd[i] = fatan2(mvy, mvx);
        float nx=S->craft_sh_x[i]+mvx*sp*dt, ny=S->craft_sh_y[i]+mvy*sp*dt;
        // keep the whole fish in the water: the centre AND a
        // body's-length ahead of it must both be wet, or the grey
        // shows on the sand (Daniel, Nov 6)
        if (craft_biome_at(b, bw_tileof(nx), bw_tileof(S->craft_sh_y[i]))==5
         && craft_biome_at(b, bw_tileof(nx + mvx*15), bw_tileof(S->craft_sh_y[i]))==5) S->craft_sh_x[i]=nx;
        if (craft_biome_at(b, bw_tileof(S->craft_sh_x[i]), bw_tileof(ny))==5
         && craft_biome_at(b, bw_tileof(S->craft_sh_x[i]), bw_tileof(ny + mvy*15))==5) S->craft_sh_y[i]=ny;
        if (d2 < 27*27 && S->craft_grace<=0 && !b->fainted){
            float dmgf = 0.20f * ((b->craft_gear&131072)? 0.15f : (b->craft_gear&256)? 0.3f : (b->craft_gear&8)? 0.5f : 1.0f);
            b->hp -= b->maxhp*dmgf; b->calm=0; b->hurt_t=0.3f; S->craft_grace=1.3f; sfx(16);
            scpy(S->note, "SHARK BITE! THE WATER HAS TEETH.", 72); S->note_t=1.8f;
            splat(b->x, b->y, COL(200,215,225), 6, 90);
            if (b->hp<=0){ b->hp=0; b->fainted=1; bw_exit(pi, 1); return; }
        }
        if (d2 > 1000*1000) S->craft_sh_hp[i]=0;   // lost the scent
    }
    if (S->craft_shn>0 && S->craft_sh_hp[0]<=0){
        int w=0;
        for (int i=0;i<S->craft_shn;i++) if (S->craft_sh_hp[i]>0){ S->craft_sh_x[w]=S->craft_sh_x[i]; S->craft_sh_y[w]=S->craft_sh_y[i]; S->craft_sh_hp[w]=S->craft_sh_hp[i]; S->craft_sh_ph[w]=S->craft_sh_ph[i]; S->craft_sh_tr[w]=S->craft_sh_tr[i]; S->craft_sh_hd[w]=S->craft_sh_hd[i]; w++; }
        S->craft_shn=w;
    }
    // zapper towers stand guard over the base
    for (int j=0;j<b->craft_bn;j++){
        if (b->craft_bk[j]!=4) continue;
        if (S->craft_tz[j]>0){ S->craft_tz[j] -= dt; continue; }
        float cx2=b->craft_btx[j]*BW_TILE+24, cy2=b->craft_bty[j]*BW_TILE+24;
        int tgt=-1; float td=132*132;
        for (int i=0;i<S->craft_bugn;i++){
            if (S->craft_bhp[i]<=0) continue;
            float dx=S->craft_bx[i]-cx2, dy=S->craft_by[i]-cy2, d2=dx*dx+dy*dy;
            if (d2<td){ td=d2; tgt=i; }
        }
        if (tgt>=0){
            S->craft_tz[j]=1.1f; S->craft_beam_t=0.12f;
            S->craft_beam_x1=cx2; S->craft_beam_y1=cy2-16;
            S->craft_beam_x2=S->craft_bx[tgt]; S->craft_beam_y2=S->craft_by[tgt];
            S->craft_bhp[tgt] -= 1; sfx(12);
            splat(S->craft_bx[tgt], S->craft_by[tgt], COL(160,210,255), 4, 80);
            if (S->craft_bhp[tgt]<=0) craft_bug_die(pi, tgt);
        } else S->craft_tz[j]=0.25f;
    }
    // compact the bug list occasionally
    if (S->craft_bugn>0 && S->craft_bhp[0]<=0){
        int w=0;
        for (int i=0;i<S->craft_bugn;i++) if (S->craft_bhp[i]>0){ S->craft_bx[w]=S->craft_bx[i]; S->craft_by[w]=S->craft_by[i]; S->craft_bhp[w]=S->craft_bhp[i]; S->craft_bph[w]=S->craft_bph[i]; S->craft_bk[w]=S->craft_bk[i]; S->craft_btr[w]=S->craft_btr[i]; w++; }
        S->craft_bugn=w;
    }
    S->craft_prevk = in->keys;
}
static void glow_ellipse(int cx, int cy, int rx, int ry, u32 c, int alpha){
    for (int pass=0; pass<2; pass++){
        int prx = pass? rx*2/3 : rx, pry = pass? ry*2/3 : ry, al = pass? alpha : alpha/2;
        if (prx<1) prx=1; if (pry<1) pry=1;
        for (int yy=cy-pry; yy<=cy+pry; yy++){
            float t=(float)(yy-cy)/(float)pry;
            int dx=(int)(prx*fsqrt(1.0f-t*t));
            for (int xx=cx-dx; xx<=cx+dx; xx++) pset_blend(xx, yy, c, al);
        }
    }
}
// row labels shared by the list rows and the selection detail strip,
// so the two can never disagree about a name or a price
static const char *craft_row_name(int id){
    if (id==103) return "EAT LAKE FISH (+35% HP)";
    if (id==100) return "EAT HEART PIE (+50% HP)";
    if (id==101) return "EAT CARAPACE STEW (+50% HP, SWIFT)";
    if (id==102) return "DEMOLISH MODE (HALF BACK)";
    return CREC[id].name;
}
static const char *craft_row_shortname(int id){
    if (id==103) return "LAKE FISH";
    if (id==100) return "HEART PIE";
    if (id==101) return "CARAPACE STEW";
    if (id==102) return "DEMOLISH MODE";
    return CREC[id].name;
}
static void craft_row_status(Buddy *b, int id, char *st){
    char *sq=st;
    if (id==103){ sq=sappend(sq, "HAVE "); sq=sapp_int(sq, b->craft_fish); }
    else if (id==100){ sq=sappend(sq, "HAVE "); sq=sapp_int(sq, b->craft_pies); }
    else if (id==101){ sq=sappend(sq, "HAVE "); sq=sapp_int(sq, b->craft_stews); }
    else if (id==102){ sq=sappend(sq, "ARM"); }
    else {
        const CRec *R = &CREC[id];
        int owned = R->build<0 && R->bit && (b->craft_gear & R->bit);
        int built = R->build>=0 && R->bit && (b->craft_gear & R->bit);
        if (owned) sq=sappend(sq, "OWNED");
        else if (built){ sq=sappend(sq, "BUILT "); sq=sapp_int(sq, craft_build_kind_count(b, R->build)); }
        else if (R->req && !(b->craft_gear & R->req)){ sq=sappend(sq, "NEED "); sq=sappend(sq, craft_reqname(R->req)); }
        else if (id==30){ if (b->craft_fish>0){ sq=sappend(sq, "FISH "); sq=sapp_int(sq, b->craft_fish); } else sq=sappend(sq, "NEED A FISH"); }
        else {
            if (R->w){ sq=sappend(sq, "W"); sq=sapp_int(sq, R->w); sq=sappend(sq, " "); }
            if (R->s){ sq=sappend(sq, "S"); sq=sapp_int(sq, R->s); sq=sappend(sq, " "); }
            if (R->f){ sq=sappend(sq, "F"); sq=sapp_int(sq, R->f); sq=sappend(sq, " "); }
            if (R->c){ sq=sappend(sq, "C"); sq=sapp_int(sq, R->c); }
        }
    }
    *sq=0;
}
static u32 craft_row_color(Buddy *b, int id){
    if (id==102) return COL(255,180,130);
    if (id>=100) return COL(240,244,250);
    const CRec *R = &CREC[id];
    int owned = R->build<0 && R->bit && (b->craft_gear & R->bit);
    int built = R->build>=0 && R->bit && (b->craft_gear & R->bit);
    if (owned || built) return COL(250,215,130);
    if (R->req && !(b->craft_gear & R->req)) return COL(240,150,130);
    if (id==30 && b->craft_fish<1) return COL(140,146,150);
    return craft_afford(b, R)? COL(240,244,250) : COL(140,146,150);
}
// little pictograms for the crafting list — you scan pictures, not text
static void craft_icon(int x, int y, int id){
    switch (id){
        case 0: case 2:   // axes (the stone one wears grey)
            line_thick(x+3, y+11, x+9, y+4, 2, COL(140,96,54));
            rect_fill(x+7, y+1, 6, 5, id==0? COL(120,124,132) : COL(150,152,158)); break;
        case 1: case 3:   // picks (the carapace one burns orange)
            line_thick(x+6, y+12, x+6, y+4, 2, COL(140,96,54));
            line_thick(x+1, y+5, x+6, y+1, 2, id==1? COL(150,152,158) : COL(255,150,60));
            line_thick(x+11, y+5, x+6, y+1, 2, id==1? COL(150,152,158) : COL(255,150,60)); break;
        case 4: case 7:   // rafts (the sail one flies a triangle)
            rect_fill(x+1, y+8, 12, 3, COL(156,116,66)); rect_fill(x+1, y+4, 12, 3, COL(146,106,58));
            if (id==7){ line_thick(x+7, y+8, x+7, y+0, 1, COL(110,76,44)); rect_fill(x+8, y+1, 4, 5, COL(240,238,225)); } break;
        case 5:   // lantern
            rect_fill(x+4, y+3, 6, 8, COL(240,210,110)); rect_line(x+4, y+3, 6, 8, COL(90,80,60)); disc_fill(x+7, y+7, 1, COL(255,250,220)); break;
        case 6:   // fishing rod
            line_thick(x+2, y+12, x+11, y+2, 2, COL(140,96,54)); line_thick(x+11, y+2, x+11, y+7, 1, COL(220,226,235)); disc_fill(x+11, y+9, 1, COL(255,120,130)); break;
        case 8:   // sword
            line_thick(x+3, y+11, x+10, y+3, 2, COL(210,214,220)); rect_fill(x+2, y+9, 5, 2, COL(198,164,84)); break;
        case 9:   // spear
            line_thick(x+2, y+12, x+10, y+4, 2, COL(140,96,54)); line_thick(x+10, y+4, x+12, y+1, 2, COL(255,170,80)); break;
        case 10: case 12:   // zappers (the storm one crackles twice)
            rect_fill(x+4, y+5, 6, 6, COL(120,124,134)); disc_fill(x+7, y+3, 2, COL(120,190,255));
            if (id==12) disc_fill(x+11, y+6, 2, COL(160,215,255)); break;
        case 11:   // boomerang
            disc_fill(x+3, y+9, 2, COL(230,160,70)); disc_fill(x+7, y+4, 2, COL(230,160,70)); disc_fill(x+11, y+9, 2, COL(230,160,70)); break;
        case 13: case 14: case 15:   // armor tiers
            rect_fill(x+3, y+3, 8, 9, id==13? COL(170,190,110) : id==14? COL(140,146,152) : COL(255,150,60));
            rect_fill(x+5, y+3, 4, 3, COL(18,22,20));
            if (id!=13) rect_fill(x+3, y+7, 8, 2, COL(90,94,102)); break;
        case 16: case 23:   // walls (the reinforced one wears iron)
            rect_fill(x+1, y+3, 12, 9, id==16? COL(156,110,62) : COL(120,84,48)); rect_fill(x+1, y+7, 12, 2, COL(96,64,32));
            if (id==23){ rect_fill(x+1, y+3, 2, 9, COL(96,100,108)); rect_fill(x+11, y+3, 2, 9, COL(96,100,108)); } break;
        case 17:   // stone wall
            rect_fill(x+1, y+3, 12, 9, COL(138,142,150)); disc_fill(x+4, y+6, 1, COL(96,100,108)); disc_fill(x+10, y+9, 1, COL(96,100,108)); break;
        case 18: case 19:   // fires
            line_thick(x+3, y+11, x+11, y+9, 2, COL(110,76,44));
            ellipse_fill(x+7, y+6, 3, 5, COL(255,140,50)); disc_fill(x+7, y+6, 1, COL(255,215,110));
            if (id==19){ rect_fill(x+1, y+3, 2, 9, COL(104,108,116)); rect_fill(x+11, y+3, 2, 9, COL(104,108,116)); } break;
        case 20:   // torch post
            rect_fill(x+6, y+5, 2, 7, COL(110,76,44)); ellipse_fill(x+7, y+3, 2, 3, COL(255,160,60)); break;
        case 21:   // dock
            rect_fill(x+1, y+4, 12, 3, COL(156,116,66));
            line_thick(x+1, y+10, x+4, y+9, 1, COL(120,190,235)); line_thick(x+5, y+11, x+8, y+10, 1, COL(120,190,235)); line_thick(x+9, y+10, x+12, y+9, 1, COL(120,190,235)); break;
        case 22:   // bedroll
            rect_fill(x+2, y+5, 10, 6, COL(150,60,60)); rect_fill(x+2, y+7, 10, 1, COL(240,230,200)); rect_fill(x+2, y+4, 4, 3, COL(240,244,250)); break;
        case 24:   // lantern post
            rect_fill(x+6, y+4, 2, 8, COL(84,66,44)); rect_fill(x+4, y+1, 6, 5, COL(240,210,110)); break;
        case 25:   // spike trap
            line_thick(x+3, y+11, x+4, y+4, 2, COL(168,172,180)); line_thick(x+7, y+11, x+7, y+3, 2, COL(168,172,180)); line_thick(x+11, y+11, x+10, y+4, 2, COL(120,124,132)); break;
        case 26:   // zapper tower
            rect_fill(x+3, y+6, 8, 6, COL(120,124,134)); disc_fill(x+7, y+3, 3, COL(120,190,255)); break;
        case 27:   // scarecrow
            line_thick(x+7, y+12, x+7, y+5, 2, COL(110,76,44)); rect_fill(x+2, y+5, 10, 2, COL(110,76,44)); disc_fill(x+7, y+3, 3, COL(255,150,50)); break;
        case 28: case 100:   // pies
            disc_fill(x+7, y+7, 5, COL(226,170,90)); line_thick(x+4, y+5, x+10, y+9, 1, COL(170,120,60)); line_thick(x+10, y+5, x+4, y+9, 1, COL(170,120,60)); break;
        case 29: case 101:   // stew pot
            ellipse_fill(x+7, y+8, 6, 4, COL(90,94,102)); rect_fill(x+3, y+4, 8, 2, COL(140,74,54)); break;
        case 30: case 103:   // fish
            ellipse_fill(x+6, y+7, 5, 3, COL(140,200,235)); line_thick(x+10, y+7, x+13, y+4, 2, COL(110,170,210)); line_thick(x+10, y+7, x+13, y+10, 2, COL(110,170,210)); disc_fill(x+4, y+6, 1, COL(20,30,40)); break;
        case 102:   // demolish hammer
            line_thick(x+4, y+12, x+9, y+6, 2, COL(140,96,54)); rect_fill(x+7, y+1, 6, 5, COL(255,150,90)); break;
        default:
            disc_fill(x+7, y+7, 3, COL(200,200,205)); break;
    }
}
static void craft_render(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    G.camx=b->x; G.camy=b->y;
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    int tx0=bw_tileof(b->x - vw2/2 - BW_TILE), tx1=bw_tileof(b->x + vw2/2 + BW_TILE);
    int ty0=bw_tileof(b->y - vh2/2 - BW_TILE), ty1=bw_tileof(b->y + vh2/2 + BW_TILE);
    for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
        int x=SX(tx*BW_TILE), y=SY(ty*BW_TILE);
        if (x+BW_TILE<vx || x>vx+vw2 || y+BW_TILE<vy || y>vy+vh2) continue;
        int biome = craft_biome_at(b, tx, ty);
        u32 h = hash2(tx, ty, craft_seed(b) ^ 0x7A55u);
        if (biome==5){
            u32 base = ((tx+ty)&1)==0? COL(44,104,164) : COL(38,96,154);
            rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(base, 0.94f + (float)((h>>20)%14)*0.01f));
            if (h % 7 == 0){ int wxp = x + (int)((h>>6) % 30) + 6 + (int)(G.anim_t*6 + (h%40)) % 13 - 6,
                                wyp = y + (int)((h>>11) % 32) + 8;
                line_thick(wxp, wyp, wxp+4, wyp-2, 1, COL(150,200,235)); line_thick(wxp+4, wyp-2, wxp+8, wyp, 1, COL(150,200,235)); }
            { int dbi = craft_build_at(b, tx, ty);   // a dock rides the water tile itself
              if (dbi>=0 && b->craft_bk[dbi]==7){
                  rect_fill(x+1, y+8, BW_TILE-2, 30, COL(156,116,66));
                  rect_fill(x+1, y+17, BW_TILE-2, 3, COL(120,86,44));
                  rect_fill(x+1, y+28, BW_TILE-2, 3, COL(120,86,44));
                  rect_line(x+1, y+8, BW_TILE-2, 30, COL(96,66,34));
                  disc_fill(x+6, y+12, 1, COL(70,70,74)); disc_fill(x+BW_TILE-7, y+12, 1, COL(70,70,74));
                  disc_fill(x+6, y+33, 1, COL(70,70,74)); disc_fill(x+BW_TILE-7, y+33, 1, COL(70,70,74));
              } }
            // foam where water meets land (glow-up): a dashed lip
            // that breathes on the same slow clock as the sparkles
            { int fo = ((int)(G.anim_t*2) + ((h>>5)&1)) & 1; u32 fc = COL(225,242,250);
              if (craft_biome_at(b, tx, ty-1)!=5){ for (int s2=0;s2<3;s2++) rect_fill(x+4+s2*14+fo*4, y+1, 8, 2, fc); }
              if (craft_biome_at(b, tx, ty+1)!=5){ for (int s2=0;s2<3;s2++) rect_fill(x+4+s2*14+((fo^1)*4), y+BW_TILE-3, 8, 2, fc); }
              if (craft_biome_at(b, tx-1, ty)!=5){ for (int s2=0;s2<3;s2++) rect_fill(x+1, y+4+s2*14+fo*4, 2, 8, fc); }
              if (craft_biome_at(b, tx+1, ty)!=5){ for (int s2=0;s2<3;s2++) rect_fill(x+BW_TILE-3, y+4+s2*14+((fo^1)*4), 2, 8, fc); } }
            continue;
        }
        { u32 base;
          if (biome==0) base = ((tx+ty)&1)==0? COL(106,180,82) : COL(96,168,72);
          else if (biome==1) base = ((tx+ty)&1)==0? COL(66,134,60) : COL(58,124,52);
          else if (biome==2) base = ((tx+ty)&1)==0? COL(219,192,124) : COL(209,182,114);
          else if (biome==3) base = ((tx+ty)&1)==0? COL(228,234,240) : COL(216,224,232);
          else base = ((tx+ty)&1)==0? COL(90,112,68) : COL(80,102,60);
          rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(base, 0.94f + (float)((h>>20)%14)*0.01f));
          // sparse darker freckles so the flat biomes have grain (glow-up)
          if ((h>>3)%9==0){ u32 sc2 = col_shade(base, 0.86f);
              pset(x + (int)((h>>6)%38) + 5, y + (int)((h>>11)%38) + 5, sc2);
              pset(x + (int)((h>>16)%30) + 9, y + (int)((h>>21)%30) + 9, sc2); }
          // shore: land kissing water gets a sandy rim
          if (craft_biome_at(b, tx+1, ty)==5 || craft_biome_at(b, tx-1, ty)==5 || craft_biome_at(b, tx, ty+1)==5 || craft_biome_at(b, tx, ty-1)==5)
              rect_fill(x, y+BW_TILE-6, BW_TILE+1, 6, COL(226,204,140));
        }
        int nd = craft_node_at(pi, tx, ty);
        if (nd==1){   // a tree (a snowy pine in the snow)
            shadow_ellipse(x+24, y+41, 12, 3);
            rect_fill(x+21, y+26, 6, 14, COL(110,76,44));
            if (biome==3){ disc_fill(x+24, y+16, 13, COL(52,110,96)); disc_fill(x+24, y+10, 9, COL(66,128,112)); disc_fill(x+24, y+6, 5, COL(240,246,250)); }
            else { disc_fill(x+17, y+20, 10, COL(52,128,54)); disc_fill(x+31, y+20, 10, COL(58,138,58)); disc_fill(x+24, y+13, 11, COL(70,156,66)); }
        } else if (nd==2){
            ellipse_fill(x+24, y+30, 13, 9, COL(142,146,152));
            ellipse_fill(x+19, y+25, 7, 5, COL(164,168,174));
            disc_fill(x+27, y+24, 2, COL(110,114,120));
        } else if (nd==3){
            rect_fill(x+19, y+12, 10, 30, COL(74,160,84));
            rect_fill(x+12, y+20, 7, 4, COL(74,160,84)); rect_fill(x+12, y+16, 4, 8, COL(74,160,84));
            rect_fill(x+29, y+24, 7, 4, COL(74,160,84)); rect_fill(x+33, y+20, 4, 8, COL(74,160,84));
            disc_fill(x+24, y+10, 3, COL(240,120,150));
        } else if (nd==4){
            for (int g2=0;g2<5;g2++){ int gx2 = x+10+g2*7; line_thick(gx2, y+38, gx2 + (int)(fsin(G.anim_t*2 + g2*1.3f + tx)*2), y+24 - (g2%3)*3, 1, biome==4? COL(120,160,80) : COL(150,200,90)); }
        }
        int bi2 = craft_build_at(b, tx, ty);
        if (bi2>=0){
            int bk = b->craft_bk[bi2], bhp = b->craft_bhp[bi2], bmax = craft_buildhp_max(bk);
            if (bk!=7) shadow_ellipse(x+24, y+41, 20, 4);   // builds sit ON the ground now (glow-up)
            if (bk==0){   // wooden wall
                rect_fill(x+2, y+8, BW_TILE-4, 30, COL(156,110,62));
                rect_fill(x+2, y+20, BW_TILE-4, 3, COL(120,82,42));
                rect_line(x+2, y+8, BW_TILE-4, 30, COL(96,64,32));
                disc_fill(x+7, y+13, 1, COL(70,70,74)); disc_fill(x+BW_TILE-8, y+13, 1, COL(70,70,74));
                if (bhp < bmax){ line_thick(x+8, y+14, x+24, y+30, 1, COL(70,44,20)); line_thick(x+26, y+12, x+18, y+26, 1, COL(70,44,20)); }
            } else if (bk==2){   // stone wall
                rect_fill(x+2, y+8, BW_TILE-4, 30, COL(138,142,150));
                rect_fill(x+2, y+22, BW_TILE-4, 3, COL(104,108,116));
                rect_line(x+2, y+8, BW_TILE-4, 30, COL(78,82,90));
                disc_fill(x+12, y+15, 2, COL(96,100,108)); disc_fill(x+32, y+28, 2, COL(96,100,108));
                if (bhp < bmax*2/3){ line_thick(x+10, y+12, x+22, y+28, 1, COL(60,62,70)); }
                if (bhp < bmax/3){ line_thick(x+30, y+10, x+20, y+30, 1, COL(60,62,70)); line_thick(x+6, y+30, x+24, y+18, 1, COL(60,62,70)); }
            } else if (bk==1 || bk==3){   // campfire / fireplace
                int cxp=x+24, cyp=y+24;
                if (bk==3){ ellipse_fill(cxp, cyp+5, 15, 8, COL(104,108,116)); ellipse_fill(cxp, cyp+3, 11, 6, COL(70,72,80)); }
                else ellipse_fill(cxp, cyp+4, 13, 6, COL(120,124,128));
                line_thick(cxp-8, cyp+5, cxp+8, cyp+2, 3, COL(110,76,44)); line_thick(cxp-8, cyp+2, cxp+8, cyp+5, 3, COL(96,64,32));
                int fl = (int)(fsin(G.anim_t*11 + bi2*2)*2);
                int fh = bk==3? 13 : 9;
                ellipse_fill(cxp, cyp-4, bk==3? 8:6, fh+fl, COL(255,140,50));
                ellipse_fill(cxp, cyp-3, bk==3? 5:3, fh-3+fl, COL(255,205,90));
            } else if (bk==4){   // zapper tower
                int cxp=x+24, cyp=y+24;
                rect_fill(cxp-9, cyp-2, 18, 16, COL(120,124,134)); rect_line(cxp-9, cyp-2, 18, 16, COL(80,84,94));
                rect_fill(cxp-3, cyp-12, 6, 11, COL(96,100,110));
                int pu = 3 + (int)(fsin(G.anim_t*5 + bi2)*1.5f);
                disc_fill(cxp, cyp-15, pu+2, COL(120,190,255)); disc_fill(cxp, cyp-15, 2, COL(230,246,255));
                if (bhp < bmax){ line_thick(cxp-7, cyp+2, cxp+4, cyp+12, 1, COL(60,62,70)); }
            } else if (bk==5){   // spike trap: quiet grey teeth in the grass
                for (int s3=0;s3<4;s3++){ int sxp = x+8+s3*10;
                    line_thick(sxp, y+36, sxp+3, y+22, 2, COL(168,172,180)); line_thick(sxp+6, y+36, sxp+3, y+22, 2, COL(120,124,132)); }
            } else if (bk==6){   // torch post
                int cxp=x+24, cyp=y+24;
                rect_fill(cxp-2, cyp-2, 4, 16, COL(110,76,44));
                int fl = (int)(fsin(G.anim_t*13 + bi2*3)*1.5f);
                ellipse_fill(cxp, cyp-6, 4, 6+fl, COL(255,160,60));
                disc_fill(cxp, cyp-6, 2, COL(255,225,140));
            } else if (bk==10){   // reinforced wall: dark planks, iron bands
                rect_fill(x+2, y+8, BW_TILE-4, 30, COL(120,84,48));
                rect_fill(x+2, y+20, BW_TILE-4, 3, COL(88,60,30));
                rect_fill(x+2, y+8, 5, 30, COL(96,100,108)); rect_fill(x+BW_TILE-7, y+8, 5, 30, COL(96,100,108));
                rect_line(x+2, y+8, BW_TILE-4, 30, COL(70,48,24));
                if (bhp < bmax){ line_thick(x+10, y+12, x+26, y+32, 1, COL(56,36,16)); line_thick(x+28, y+10, x+16, y+28, 1, COL(56,36,16)); }
            } else if (bk==8){   // bedroll: a striped blanket, waiting
                rect_fill(x+8, y+16, 32, 18, COL(150,60,60));
                rect_fill(x+8, y+21, 32, 3, COL(240,230,200)); rect_fill(x+8, y+28, 32, 3, COL(240,230,200));
                rect_fill(x+8, y+13, 12, 6, COL(240,244,250));
            } else if (bk==9){   // the scarecrow stands its watch
                int cxp=x+24, cyp=y+24;
                rect_fill(cxp-2, cyp-4, 4, 20, COL(110,76,44));
                rect_fill(cxp-11, cyp-1, 22, 3, COL(110,76,44));
                disc_fill(cxp, cyp-10, 6, COL(255,150,50));
                rect_fill(cxp-7, cyp-16, 14, 3, COL(96,64,32)); rect_fill(cxp-3, cyp-19, 6, 4, COL(96,64,32));
                disc_fill(cxp-2, cyp-10, 1, COL(30,26,24)); disc_fill(cxp+2, cyp-10, 1, COL(30,26,24));
                if (bhp < bmax){ line_thick(cxp-8, cyp+2, cxp+6, cyp+12, 1, COL(70,44,20)); }
            } else if (bk==11){   // lantern post: taller, glassed, patient
                int cxp=x+24, cyp=y+24;
                rect_fill(cxp-2, cyp-4, 4, 20, COL(84,66,44));
                rect_fill(cxp-5, cyp-13, 10, 10, COL(240,210,110));
                rect_line(cxp-5, cyp-13, 10, 10, COL(90,80,60));
                disc_fill(cxp, cyp-8, 2, COL(255,240,190));
                rect_fill(cxp-6, cyp-15, 12, 2, COL(70,56,36));
            }
        }
    }
    // the supply cache, when one has drifted in
    if (S->craft_cache_on){
        int x=SX(S->craft_cache_x), y=SY(S->craft_cache_y);
        shadow_ellipse(x, y+8, 12, 3);
        rect_fill(x-11, y-8, 22, 15, COL(168,120,58)); rect_line(x-11, y-8, 22, 15, COL(110,76,34));
        rect_fill(x-11, y-2, 22, 3, COL(110,76,34));
        disc_fill(x, y-1, 2, COL(250,220,120));
    }
    // feral bugs: sun-bleached, gloom-dark, or golden
    for (int i=0;i<S->craft_bugn;i++){
        if (S->craft_bhp[i]<=0) continue;
        int x=SX(S->craft_bx[i]), y=SY(S->craft_by[i]);
        int bk = S->craft_bk[i];
        shadow_ellipse(x, y+8, 11, 3);
        if (bk==2){
            ellipse_fill(x, y, 11, 8, COL(232,180,60));
            disc_fill(x-4, y-3, 2, COL(255,255,255)); disc_fill(x+4, y-3, 2, COL(120,80,10));
            if (((int)(G.anim_t*3)+i)&1) disc_fill(x+7, y-8, 1, COL(255,246,200));
        } else if (bk==1){
            ellipse_fill(x, y, 12, 9, COL(74,44,96));
            disc_fill(x-4, y-3, 2, COL(255,70,60)); disc_fill(x+4, y-3, 2, COL(255,70,60));
        } else {
            ellipse_fill(x, y, 11, 8, COL(140,74,54));
            disc_fill(x-4, y-3, 2, COL(255,230,120)); disc_fill(x+4, y-3, 2, COL(255,230,120));
        }
        for (int l=0;l<3;l++){ int ph = (int)(fsin(G.anim_t*10 + i + l*2)*2); line_thick(x-9+l*8, y+5, x-11+l*8, y+9+ph, 1, COL(80,44,30)); }
    }
    // fins in the water — and the grey behind them
    for (int i=0;i<S->craft_shn;i++){
        if (S->craft_sh_hp[i]<=0) continue;
        int x=SX(S->craft_sh_x[i]), y=SY(S->craft_sh_y[i]);
        // the body stays UNDER the surface — a grey ghost in the
        // blue, drawn blended — and only the fin breaks the water,
        // pointing where he's actually headed (Daniel, Nov 6: they
        // rendered weird and showed up on the sand)
        glow_ellipse(x, y+3, 16, 7, COL(74,88,102), 120);
        float hd = S->craft_sh_hd[i];
        int px = (int)(-fsin(hd)*6), tipx = x + (int)(fcos(hd)*3);
        line_thick(x+px, y-1, tipx, y-11, 3, COL(122,134,148));
        line_thick(tipx, y-11, x-px, y-1, 3, COL(104,116,130));
    }
    // zapper bolts and tower beams
    for (int i=0;i<4;i++) if (S->craft_bolt_a[i]){
        int x=SX(S->craft_bolt_x[i]), y=SY(S->craft_bolt_y[i]);
        disc_fill(x, y, 4, COL(255,150,50)); disc_fill(x, y, 2, COL(255,225,150));
    }
    if (S->craft_bmr_a){   // the boomerang, mid-flight, spinning
        int x=SX(S->craft_bmr_x), y=SY(S->craft_bmr_y);
        float a = G.anim_t*22;
        for (int k2=0;k2<3;k2++){ float aa=a+k2*2.09f; disc_fill(x+(int)(fcos(aa)*5), y+(int)(fsin(aa)*5), 3, COL(230,160,70)); }
        disc_fill(x, y, 2, COL(255,220,150));
    }
    if (S->craft_beam_t>0){
        line_thick(SX(S->craft_beam_x1), SY(S->craft_beam_y1), SX(S->craft_beam_x2), SY(S->craft_beam_y2), 2, COL(170,215,255));
        line_thick(SX(S->craft_beam_x1), SY(S->craft_beam_y1), SX(S->craft_beam_x2), SY(S->craft_beam_y2), 1, COL(240,250,255));
    }
    // the raft, when he's afloat (not when the dock is under him)
    { int dbi2 = craft_build_at(b, bw_tileof(b->x), bw_tileof(b->y));
      if (craft_biome_at(b, bw_tileof(b->x), bw_tileof(b->y))==5 && (b->craft_gear&512) && !(dbi2>=0 && b->craft_bk[dbi2]==7)){
        int x=SX(b->x), y=SY(b->y);
        ellipse_fill(x, y+9, 20, 7, COL(146,102,58));
        rect_fill(x-18, y+6, 36, 3, COL(120,82,42));
        line_thick(x-14, y+3, x-14, y+12, 2, COL(110,76,44)); line_thick(x+12, y+3, x+12, y+12, 2, COL(110,76,44));
        if (b->craft_gear&262144){ line_thick(x+3, y+5, x+3, y-13, 2, COL(110,76,44)); rect_fill(x+4, y-12, 10, 13, COL(240,238,225)); rect_line(x+4, y-12, 10, 13, COL(190,186,170)); }
      } }
    draw_buddy(b);
    // night falls over everything but the fires
    float dk = craft_darkf(b);
    if (dk>0.02f){
        blend_rect(vx, vy, vw2, vh2, COL(8,10,28), (int)(dk*165));
        for (int i=0;i<b->craft_bn;i++){
            int bk = b->craft_bk[i];
            int cxp=SX(b->craft_btx[i]*BW_TILE+24), cyp=SY(b->craft_bty[i]*BW_TILE+24);
            if (bk==1) glow_ellipse(cxp, cyp, 92, 70, COL(255,170,80), (int)(95*dk));
            else if (bk==3) glow_ellipse(cxp, cyp, 128, 96, COL(255,176,84), (int)(105*dk));
            else if (bk==6) glow_ellipse(cxp, cyp, 82, 62, COL(255,190,96), (int)(85*dk));
            else if (bk==4) glow_ellipse(cxp, cyp-8, 40, 30, COL(140,200,255), (int)(55*dk));
            else if (bk==11) glow_ellipse(cxp, cyp, 122, 94, COL(255,200,110), (int)(95*dk));
        }
        glow_ellipse(SX(b->x), SY(b->y), (b->craft_gear&8192)? 88 : 56, (b->craft_gear&8192)? 68 : 44, COL(255,220,160), (int)(70*dk));
    }
    // HUD: two rows, nothing shares a line. Row 1: the pack (left)
    // and the hour (right). Row 2: the HP bar, then your gear. The old
    // one-row version right-aligned the gear INTO the pack text once
    // you owned a few tools — worse in split view (Daniel, Nov 5).
    rect_fill(vx, vy, vw2, 34, COL(16,20,28));
    { char buf[160]; char *q=buf;
      q=sappend(q, "WOOD "); q=sapp_int(q, b->craft_wood);
      q=sappend(q, "  STONE "); q=sapp_int(q, b->craft_stone);
      q=sappend(q, "  FIBER "); q=sapp_int(q, b->craft_fiber);
      q=sappend(q, "  CARAPACE "); q=sapp_int(q, b->craft_car);
      if (b->craft_fish>0){ q=sappend(q, "  FISH "); q=sapp_int(q, b->craft_fish); }
      if (b->craft_pies>0){ q=sappend(q, "  PIES "); q=sapp_int(q, b->craft_pies); }
      if (b->craft_stews>0){ q=sappend(q, "  STEWS "); q=sapp_int(q, b->craft_stews); }
      *q=0; draw_text(vx+8, vy+5, buf, 1, COL(240,244,250)); }
    { const char *tn = dk>0.55f? "NIGHT" : b->craft_tod<15 || b->craft_tod>=225? "DAWN" : b->craft_tod>=140? "DUSK" : "DAY";
      u32 tc = tn[0]=='N'? COL(170,190,240) : tn[1]=='A' && tn[0]=='D'? COL(250,200,130) : COL(255,226,140);
      draw_text(vx+vw2-8-text_w(tn,1), vy+5, tn, 1, tc); }
    rect_fill(vx+8, vy+21, 104, 6, COL(60,30,34));
    rect_fill(vx+8, vy+21, (int)(104 * (b->maxhp>0? b->hp/b->maxhp : 0)), 6, COL(230,84,96));
    { char buf[160]; char *q=buf;
      if (b->craft_gear&1) q=sappend(q, "AXE ");
      if (b->craft_gear&2) q=sappend(q, "PICK ");
      if (b->craft_gear&16) q=sappend(q, "S.AXE ");
      if (b->craft_gear&32) q=sappend(q, "C.PICK ");
      if (b->craft_gear&8192) q=sappend(q, "LAMP ");
      if (b->craft_gear&16384) q=sappend(q, "ROD ");
      if (b->craft_gear&512) q=sappend(q, "RAFT ");
      if (b->craft_gear&262144) q=sappend(q, "SAIL ");
      if (b->craft_gear&4) q=sappend(q, "SWD ");
      if (b->craft_gear&64) q=sappend(q, "SPEAR ");
      if (b->craft_gear&32768) q=sappend(q, "BOOM ");
      if (b->craft_gear&128) q=sappend(q, "ZAP ");
      if (b->craft_gear&65536) q=sappend(q, "S.ZAP ");
      if (b->craft_gear&8) q=sappend(q, "ARM ");
      if (b->craft_gear&256) q=sappend(q, "R.ARM ");
      if (b->craft_gear&131072) q=sappend(q, "SHELL");
      *q=0; if (q!=buf) draw_text(vx+124, vy+20, buf, 1, COL(250,215,130)); }
    if (S->craft_demolish>0 && !S->craft_menu){
        char buf[80]; char *q=buf; q=sappend(q, "DEMOLISH ARMED - "); q=sappend(q, lbl_int()); q=sappend(q, " REMOVES A BUILD"); *q=0;
        draw_text_center(vx+vw2/2, vy+40, buf, 1, COL(255,180,130));
    }
    if (S->sleep_t>0){
        float d = 1.0f - (S->sleep_t>0.7f? (S->sleep_t-0.7f)/0.7f : S->sleep_t/0.7f);
        blend_rect(vx, vy, vw2, vh2, COL(6,6,16), (int)(210*d));
        if (d>0.75f) draw_text_center(vx + vw2/2, vy + vh2/2-4, "Z Z Z", 2, COL(200,210,255));
    }
    if (S->craft_menu){
        int rows[64]; int n = craft_menu_rows(S, b, rows);
        int visn = n<7? n : 7;   // the list scrolls: seven rows, never a wall of text
        int w=380, h=40+visn*20+16+24+22;   // +24: the selection detail strip; +22: the search row
        int x=vx+(vw2-w)/2, y=vy+(vh2-h)/2;
        if (w>vw2-8){ w=vw2-8; x=vx+4; }
        rect_fill(x, y, w, h, COL(18,22,20)); rect_line(x, y, w, h, COL(198,164,84));
        draw_text_center(x+w/2, y+8, "CRAFTING", 2, COL(255,214,64));
        // the search row: tap it (or press P) and type — the list
        // filters flat to matching items, a mercy on a phone
        { int sy2 = y+28;
          rect_fill(x+10, sy2, w-20, 17, S->craft_search? COL(52,46,22) : COL(24,28,24));
          rect_line(x+10, sy2, w-20, 17, S->craft_search? COL(255,220,90) : COL(90,96,90));
          char sb[48]; char *q=sb; q=sappend(q, "SEARCH: ");
          if (S->craft_filter[0]) q=sappend(q, S->craft_filter);
          else if (!S->craft_search) q=sappend(q, mobile_ui()? "TAP HERE, THEN TYPE" : "PRESS /, THEN TYPE");
          if (S->craft_search && ((int)(G.anim_t*2.4f)&1)) q=sappend(q, "_");
          *q=0;
          draw_text(x+16, sy2+5, sb, 1, (S->craft_filter[0] || S->craft_search)? COL(255,240,200) : COL(150,170,190)); }
        for (int r=S->craft_scroll;r<S->craft_scroll+visn && r<n;r++){
            int id = rows[r], ry = y+52+(r-S->craft_scroll)*20;
            if (id==-99){   // the filter matched nothing
                draw_text_center(x+w/2, ry, "NOTHING MATCHES - TRY FEWER LETTERS", 1, COL(255,150,130));
                continue;
            }
            if (id<0){   // a segment header: a signpost in the list, never selectable
                rect_fill(x+10, ry-2, w-20, 13, COL(26,32,28));
                draw_text_center(x+w/2, ry, CRAFT_CAT_NAME[-id-1], 1, COL(198,164,84));
                continue;
            }
            char st[64]; craft_row_status(b, id, st);
            u32 colr = craft_row_color(b, id);
            if (S->craft_sel==r){ rect_fill(x+10, ry-3, w-20, 15, COL(70,58,30)); draw_text(x+14, ry, ">", 1, COL(255,220,90)); }
            craft_icon(x+26, ry-1, id);
            draw_text(x+46, ry, craft_row_name(id), 1, colr);
            draw_text(x+w-28-text_w(st,1), ry, st, 1, colr);
        }
        // the scrollbar: a track, and a thumb that says where you are
        if (n>visn){
            int track_y = y+52, track_h = visn*20-8;
            rect_fill(x+w-10, track_y, 3, track_h, COL(52,58,54));
            int th = track_h*visn/n; if (th<12) th=12;
            int ty2 = track_y + (track_h-th)*S->craft_scroll/(n-visn);
            rect_fill(x+w-10, ty2, 3, th, COL(255,220,90));
        }
        // the selection, up close: its icon in a gold chip, its name big —
        // what you are about to craft is never in doubt (Daniel, Nov 5)
        { int sid = rows[S->craft_sel]; int dy2 = y+h-40;
          rect_fill(x+10, dy2, w-20, 24, COL(24,28,24));
          if (sid==-99){
              draw_text(x+38, dy2+5, "NO MATCHES", 2, COL(255,150,130));
          } else {
          char sst[64]; craft_row_status(b, sid, sst);
          rect_line(x+14, dy2+4, 16, 16, COL(198,164,84));
          craft_icon(x+15, dy2+5, sid);
          draw_text(x+38, dy2+5, craft_row_shortname(sid), 2, craft_row_color(b, sid));
          draw_text(x+w-28-text_w(sst,1), dy2+8, sst, 1, COL(200,214,200)); } }
        { char buf[96]; char *q=buf;
          if (S->craft_search){
              draw_text_center(x+w/2, y+h-14, "TYPE TO FILTER - ESC WHEN DONE", 1, COL(150,170,190));
          } else if (mobile_ui() && !pad_mode()){
              draw_text_center(x+w/2, y+h-14, G.touch_ctl? "STICK PICKS - FIRE CRAFTS - TAP ROWS TOO" : "TAP A ROW - TAP IT AGAIN TO CRAFT - TAP OUTSIDE TO CLOSE", 1, COL(150,170,190));
          } else {
              q=sappend(q, pad_mode()? "DPAD/STICK" : "ARROWS"); q=sappend(q, " PICK - ");
              q=sappend(q, lbl_fire()); q=sappend(q, " CRAFTS - ");
              q=sappend(q, lbl_wall()); q=sappend(q, " CLOSES"); *q=0;
              draw_text_center(x+w/2, y+h-14, buf, 1, COL(150,170,190)); } }
    }
    // (no standing hint line here any more — controls live in the
    // Start-menu's CONTROLS panel; Daniel, Nov 6)
    // one contextual prompt (Daniel, Nov 6): what INTERACT would do if
    // pressed this instant, in the update's own priority order, named
    // for the device in hand. The note bar outranks it when speaking.
    if (!S->craft_menu && !S->bw_menu && S->note_t<=0 && S->sleep_t<=0){
        const char *verb = 0;
        int ttx = bw_tileof(b->x + fcos(b->aim)*34), tty = bw_tileof(b->y + fsin(b->aim)*34);
        if (S->craft_demolish>0) verb = "DEMOLISH";
        else if (craft_fire_near(b, b->x, b->y, 84, 0, 0) || craft_bedroll_near(b)) verb = "REST";
        else if ((b->craft_gear&16384) && craft_biome_at(b, ttx, tty)==5) verb = "FISH";
        else if (S->craft_cache_on){
            float dx=S->craft_cache_x-b->x, dy=S->craft_cache_y-b->y;
            if (dx*dx+dy*dy < 50*50) verb = "LOOT THE CACHE";
        }
        if (!verb){ int nd = craft_node_at(pi, ttx, tty);
            if (nd==1) verb = "CHOP"; else if (nd==2) verb = "MINE"; else if (nd) verb = "GATHER"; }
        if (verb){ char hb[72]; const char *hint = key_prompt(hb, lbl_int(), verb);
            int hw2 = text_w(hint, 1);
            // anchored like the main game's prompt (VH-34): in this
            // render the -22 slot lands half off the frame's bottom
            rect_fill(vx + vw2/2 - hw2/2 - 6, vy + vh2-38, hw2+12, 14, COL(20,26,20));
            draw_text_center(vx + vw2/2, vy + vh2-35, hint, 1, COL(240,248,240)); }
    }
    if (S->note_t>0){
        rect_fill(vx, vy+vh2-22, vw2, 22, COL(14,14,20));
        draw_text_center(vx+vw2/2, vy+vh2-15, S->note, 1, COL(255,240,200));
    }
}


