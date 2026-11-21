// THE SURVIVAL GAME — world/props.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 1128-1177 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// Procedural world props — deterministic per chunk. The field is a
// junkyard now (Daniel, Nov 2): the old rocks became electronic
// debris. Types: 0 none, 1 shattered PCB + parts spill, 2 crate with
// a CRT in it (solid), 3 DRAM sticks + a coax coil, 4 fax machine
// with a modem on top (solid), 5 a tipped game console in the dirt
// with its cables spilled out. Solid types block players and bugs.
// ---------------------------------------------------------------------------
#define CHUNK 256
// what washes up in each region: the home country's junkyard (types
// 1-5) gives way to the far countries' own wrecks — burnt server
// husks and heat vents in the Badlands, frost-locked drive banks in
// the Cache Fields, neon memory towers in the RAM Flats, cable trees
// in the Jungle, half-deleted monoliths in the Depths (types 6-15,
// drawn in draw_prop). One table slot per hash roll, same density
// and positions as always — only the identity is regional.
static const int REGION_PROPS[NREGIONS][5] = {
    { 1, 2, 3, 4, 5 },        // Meadows: the classic junkyard
    { 1, 2, 3, 4, 5 },        // Spaghetti Fields
    { 1, 2, 3, 4, 5 },        // Legacy Lands
    { 6, 7, 1, 6, 3 },        // Kernel Badlands
    { 8, 9, 3, 8, 1 },        // Frozen Cache Fields
    { 10, 11, 3, 10, 11 },    // Neon RAM Flats
    { 12, 13, 5, 12, 1 },     // Cable Jungle
    { 14, 15, 1, 14, 5 }      // The Unmaintained Depths
};
static void chunk_prop(int cx, int cy, int slot, float *px, float *py, int *type, int *solid){
    u32 h = hash2(cx, cy, WORLD_SEED + (u32)slot*7919u);
    *type = 0; *solid = 0; *px = 0; *py = 0;
    if (cx>=-1 && cx<=0 && cy>=-1 && cy<=0) return;      // keep the store plaza clear
    float r = (float)(h>>8)/16777216.0f;
    if (r > 0.42f) return;                               // sparse
    *px = cx*CHUNK + 40 + (float)((h>>3)%170);
    *py = cy*CHUNK + 40 + (float)((h>>11)%170);
    { float pd = fsqrt((*px)*(*px) + (*py)*(*py));
      *type = REGION_PROPS[region_at(pd)][(h>>16)%5]; }
    if (*type==2 || *type==4 || *type==6 || *type==8 || *type==10 || *type==12 || *type==14) *solid = 1;
}
static int prop_collide(float x, float y, float rad){
    int cx = (int)(x/CHUNK) - (x<0?1:0), cy = (int)(y/CHUNK) - (y<0?1:0);
    for (int i=-1;i<=1;i++) for (int j=-1;j<=1;j++)
        for (int s=0;s<2;s++){
            float px,py; int t,sol;
            chunk_prop(cx+i, cy+j, s, &px,&py,&t,&sol);
            if (sol){ float dx=x-px, dy=y-py; if (dx*dx+dy*dy < (rad+20)*(rad+20)) return 1; }
        }
    return 0;
}

// ---------------------------------------------------------------------------
