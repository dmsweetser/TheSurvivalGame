// THE SURVIVAL GAME — modes/castle/castle.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from castle.c lines 1-198 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// CASTLE (bw_mode 6, Nov 14 2026) — a love letter to Castle Adventure
// (Kevin Bales, 1984), the DOS game Krista loves: you are trapped in a
// deserted castle; gather the treasures, mind the monsters, find the
// gate key and escape through the gate in the courtyard where you
// started. Twenty rooms (the original had 83), thirteen treasures,
// fifteen monsters, and the same 1550-point perfect game: 13 x 100
// for treasures, 15 x 10 for monsters, 100 for escaping. Bump combat,
// like the original: run into a monster to fight it — but only a
// sword makes your bumps count. Rooms are one screen each, joined by
// doors, in a pocket of world space far away from Banzai World.
// ---------------------------------------------------------------------------
#define CAS_OX 20000.0f
#define CAS_OY 20000.0f
#define CAS_X0 28
#define CAS_X1 612
#define CAS_Y0 56
#define CAS_Y1 332
static const unsigned char CAS_DOORS[20] = {   // bits: N=1, E=2, S=4, W=8
    6, 14, 14, 14, 12,
    7, 15, 15, 15, 13,
    7, 15, 15, 15, 13,
    3, 11, 11, 10, 9
};
static const char *CAS_NAMES[20] = {
    "THE WEST TOWER", "THE THRONE ROOM", "THE VAULT", "THE EAST TOWER", "THE BATTLEMENT",
    "THE ARMORY", "THE LIBRARY", "THE GREAT HALL", "THE CHAPEL", "THE GALLERY",
    "THE DUNGEON", "THE CRYPT", "THE CROSSING", "THE KITCHEN", "THE STORE ROOM",
    "THE CELLAR", "THE WELL ROOM", "THE COURTYARD", "THE GATEHOUSE", "THE STABLE"
};
// one stone block per room (x, y, w, h in room-local coords; w=0: none)
static const short CAS_PIL[20][4] = {
    {0,0,0,0}, {296,120,48,48}, {0,0,0,0}, {0,0,0,0}, {120,120,40,120},
    {0,0,0,0}, {150,150,60,24}, {286,110,68,68}, {0,0,0,0}, {480,150,60,24},
    {120,200,90,30}, {286,140,68,40}, {0,0,0,0}, {460,110,70,40}, {140,120,50,50},
    {0,0,0,0}, {296,150,48,48}, {0,0,0,0}, {150,200,80,26}, {470,210,60,30}
};
// items by id: 0-12 treasures, 13 sword, 14 key, 15-16 tonics
static const unsigned char CAS_ITEM_ROOM[17] = { 0, 1, 2, 2, 4, 6, 8, 11, 13, 15, 16, 18, 19, 5, 10, 13, 15 };
static const unsigned char CAS_ITEM_KIND[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3 };
// monsters: (room, type) — 0 bat, 1 snake, 2 ogre, 3 demon
static const unsigned char CAS_MON[15][2] = {
    {1,3}, {2,3}, {3,0}, {5,1}, {7,2}, {7,2}, {9,0}, {10,2},
    {11,0}, {11,0}, {12,1}, {12,1}, {13,1}, {14,2}, {19,0}
};
static const int CAS_MHP[4] = { 1, 2, 3, 4 };
static const float CAS_MSPD[4] = { 96.0f, 72.0f, 50.0f, 60.0f };
static const int CAS_MDMG[4] = { 4, 6, 10, 14 };
// spots a room's things can sit at (room-local)
static const short CAS_SPOT[6][2] = { {170,140}, {470,140}, {320,150}, {170,262}, {470,262}, {320,268} };
// an item's position THIS run: cas_enter deals every item a room
// and a spot (the classic tables, or a shuffle) into the sim state
static void cas_item_pos(int pi, int id, float *x, float *y){
    BwSim *S = &BWS[pi];
    *x = S->cas_ix[id]; *y = S->cas_iy[id];
}
static int cas_in_gap_ns(float lx){ return lx>=288 && lx<=352; }
static int cas_in_gap_ew(float ly){ return ly>=162 && ly<=226; }
static int cas_solid(int pi, float x, float y){
    BwSim *S = &BWS[pi];
    float lx = x - CAS_OX, ly = y - CAS_OY;
    int room = S->cas_room;
    const short *P = CAS_PIL[room];
    if (P[2]>0 && lx>=P[0] && lx<P[0]+P[2] && ly>=P[1] && ly<P[1]+P[3]) return 1;
    if (lx < CAS_X0) return (cas_in_gap_ew(ly) && (CAS_DOORS[room]&8))? 0 : 1;
    if (lx > CAS_X1) return (cas_in_gap_ew(ly) && (CAS_DOORS[room]&2))? 0 : 1;
    if (ly < CAS_Y0) return (cas_in_gap_ns(lx) && (CAS_DOORS[room]&1))? 0 : 1;
    if (ly > CAS_Y1) return (cas_in_gap_ns(lx) && (CAS_DOORS[room]&4))? 0 : 1;
    return 0;
}
static void cas_note(BwSim *S, const char *t, float dur){ scpy(S->note, t, 72); S->note_t = dur; }
// ---------------------------------------------------------------------------
// CASTLE SHUFFLE (v1.25 feat 7): after a first clear, the CASTLE
// cabinet offers SHUFFLE — the same twenty rooms, doors and stone,
// with the CONTENTS re-dealt from a seed: every item's room and
// spot, every monster's room and spot. The geometry never moves,
// so the fixed door graph keeps every room reachable from the
// courtyard; the deal only has to respect three placement rules
// (the spec carries the full invariant argument):
//   - nothing is dealt into the courtyard (17): the start stays
//     safe and empty, exactly as in the classic deal;
//   - the key and the sword take the deal order's first two rooms,
//     so they never share a room and never sit in the courtyard;
//   - every thing sits on a FREE SPOT — one of the room's six
//     spots that is not inside the room's stone block (rect + 6) —
//     so a pickup can always be touched and a monster can always
//     move. A room that runs out of free spots passes its remaining
//     things along the deal order to the next room with a spot
//     left; with 32 things and 95+ free spots, nothing is dropped.
// ---------------------------------------------------------------------------
int g_cas_seed_override = -1;   // dbg_cassetseed: deal THIS seed at the next shuffle entry (one-shot)
static u32 cas_rng_s;
static u32 cas_rng(void){ cas_rng_s ^= cas_rng_s<<13; cas_rng_s ^= cas_rng_s>>17; cas_rng_s ^= cas_rng_s<<5; return cas_rng_s; }
static int cas_free_spots(int room, int out[6]){
    const short *P = CAS_PIL[room]; int n = 0;
    for (int s=0;s<6;s++){
        int x = CAS_SPOT[s][0], y = CAS_SPOT[s][1];
        if (P[2]>0 && x>=P[0]-6 && x<P[0]+P[2]+6 && y>=P[1]-6 && y<P[1]+P[3]+6) continue;
        out[n++] = s;
    }
    return n;
}
// the classic deal, filled with the original formulas, so a
// classic run is the shipped castle bit for bit
static void cas_deal_classic(BwSim *S){
    for (int id=0; id<17; id++){
        int room = CAS_ITEM_ROOM[id], k = 0;
        for (int i=0;i<id;i++) if (CAS_ITEM_ROOM[i]==room) k++;
        S->cas_iroom[id] = (unsigned char)room;
        S->cas_ix[id] = CAS_OX + CAS_SPOT[(room + k) % 6][0];
        S->cas_iy[id] = CAS_OY + CAS_SPOT[(room + k) % 6][1];
    }
    for (int i=0;i<15;i++){
        int room = CAS_MON[i][0], k = 0;
        for (int j=0;j<i;j++) if (CAS_MON[j][0]==room) k++;
        S->cas_mroom[i] = room; S->cas_mtype[i] = CAS_MON[i][1];
        S->cas_mx[i] = CAS_OX + CAS_SPOT[(room + 2 + k) % 6][0];
        S->cas_my[i] = CAS_OY + CAS_SPOT[(room + 2 + k) % 6][1];
    }
}
static void cas_deal_shuffle(BwSim *S, int seed){
    cas_rng_s = hash2(seed, 0xCA57, 0x5EEDu) | 1u;
    // the 19 rooms that may hold things, in deal order
    int rooms[19], rn = 0;
    for (int r=0;r<20;r++) if (r!=17) rooms[rn++] = r;
    for (int i=rn-1;i>0;i--){ int j = (int)(cas_rng() % (u32)(i+1)); int t = rooms[i]; rooms[i] = rooms[j]; rooms[j] = t; }
    // the key and the sword take the first two rooms of the deal
    S->cas_iroom[14] = (unsigned char)rooms[0];
    S->cas_iroom[13] = (unsigned char)rooms[1];
    for (int id=0; id<17; id++){
        if (id==13 || id==14) continue;
        S->cas_iroom[id] = (unsigned char)rooms[cas_rng() % 19u];
    }
    for (int i=0;i<15;i++){
        S->cas_mroom[i] = rooms[cas_rng() % 19u];
        S->cas_mtype[i] = CAS_MON[i][1];   // a monster keeps its kind: the 1550 game is unchanged
    }
    // spots: each room's free spots, shuffled; things claim them
    // items first (ascending id), then monsters (ascending index)
    int free_sp[20][6], free_n[20], free_i[20];
    for (int r=0;r<20;r++){
        free_n[r] = (r==17)? 0 : cas_free_spots(r, free_sp[r]);
        free_i[r] = 0;
        for (int i=free_n[r]-1;i>0;i--){ int j = (int)(cas_rng() % (u32)(i+1)); int t = free_sp[r][i]; free_sp[r][i] = free_sp[r][j]; free_sp[r][j] = t; }
    }
    int placed[32];
    for (int t2=0;t2<32;t2++) placed[t2] = 0;
    for (int pass=0; pass<2; pass++){
        for (int t2=0; t2<32; t2++){
            if (placed[t2]) continue;
            int is_mon = t2>=17, idx = is_mon? t2-17 : t2;
            int want = is_mon? S->cas_mroom[idx] : S->cas_iroom[idx];
            int room = -1, spot = -1;
            if (pass==0){
                if (free_i[want] < free_n[want]){ room = want; spot = free_sp[want][free_i[want]++]; }
            } else {
                for (int q=0;q<19 && room<0;q++){
                    int r2 = rooms[q];
                    if (free_i[r2] < free_n[r2]){ room = r2; spot = free_sp[r2][free_i[r2]++]; }
                }
            }
            if (room<0) continue;
            placed[t2] = 1;
            if (is_mon){ S->cas_mroom[idx] = room; S->cas_mx[idx] = CAS_OX + CAS_SPOT[spot][0]; S->cas_my[idx] = CAS_OY + CAS_SPOT[spot][1]; }
            else { S->cas_iroom[idx] = (unsigned char)room; S->cas_ix[idx] = CAS_OX + CAS_SPOT[spot][0]; S->cas_iy[idx] = CAS_OY + CAS_SPOT[spot][1]; }
        }
    }
}
static void cas_enter(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    S->cas_room = 17; S->cas_score = 0; S->cas_treasures = 0;
    S->cas_sword = 0; S->cas_key = 0; S->cas_won = 0;
    S->cas_hitcd = 0; S->cas_notecd = 0; S->cas_wont = 0;
    for (int i=0;i<20;i++) S->cas_taken[i] = 0;
    // the deal: a shuffle entry deals the buddy's next seed (or the
    // debug override); anything else deals the classic castle
    S->cas_seed = 0;
    if (S->cas_shuffle){
        int seed;
        if (g_cas_seed_override >= 0){ seed = g_cas_seed_override; g_cas_seed_override = -1; }
        else { b->cas_shufn++; seed = b->cas_shufn; }
        S->cas_seed = seed;
        cas_deal_shuffle(S, seed);
    } else cas_deal_classic(S);
    for (int i=0;i<15;i++){
        S->cas_mhp[i] = CAS_MHP[S->cas_mtype[i]]; S->cas_malive[i] = 1;
        S->cas_mwt[i] = (float)(i % 5) * 0.7f;
    }
    b->x = CAS_OX + 320; b->y = CAS_OY + 252;
    b->bw_x = b->x; b->bw_y = b->y;
    b->aim = -PI/2;
}
