// THE SURVIVAL GAME — modes/craft/craft_world.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from craft.c lines 1-129 in the Nov 2026 fine-grained refactor; code unchanged.

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
