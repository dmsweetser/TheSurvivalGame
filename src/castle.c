// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: castle.c — CASTLE (bw_mode 6).
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]
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
static void cas_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    if (S->cas_won){
        S->cas_wont -= dt;
        if (S->cas_wont <= 0){
            int sc = S->cas_score;
            if (sc > b->cas_best) b->cas_best = sc;
            buddy_award_xp(pi, 50);   // v1.19: an escape pays the buddy
            bw_exit(pi, 0);
            Player *p = &G.p[pi];
            char buf[64]; char *q = buf;
            q = sappend(q, "YOU ESCAPED THE CASTLE! SCORE ");
            q = sapp_int(q, sc); if (b->active) q = sappend(q, " +50 BUDDY XP"); *q = 0;
            spawn_floater(p->x, p->y - 26, buf, COL(255, 220, 120));
        }
        return;
    }
    if (in->exit_){
        // v1.22 (Daniel, Nov 17): the v1.11 "no leaving without the
        // key" gate was never his intent — CASTLE is a cabinet, not
        // a prison, and walking back to the Arcade ALWAYS works.
        // The key still gates the courtyard gate (the win) and
        // nothing else.
        bw_exit(pi, 0); return;
    }
    if (S->cas_hitcd > 0) S->cas_hitcd -= dt;
    if (S->cas_notecd > 0) S->cas_notecd -= dt;
    // movement at a fixed castle pace
    float mx = 0, my = 0; int k2 = in->keys;
    if (k2&K_UP) my -= 1; if (k2&K_DOWN) my += 1;
    if (k2&K_LEFT) mx -= 1; if (k2&K_RIGHT) mx += 1;
    if (mx != 0 || my != 0){
        float d = fsqrt(mx*mx + my*my); mx /= d; my /= d;
        b->aim = fatan2(my, mx);
        float nx = b->x + mx * 150.0f * dt, ny = b->y + my * 150.0f * dt;
        if (!bw_solid_b(b, nx, b->y)) b->x = nx;
        if (!bw_solid_b(b, b->x, ny)) b->y = ny;
        b->walk_ph += dt * 9;
    }
    b->bw_x = b->x; b->bw_y = b->y;
    float lx = b->x - CAS_OX, ly = b->y - CAS_OY;
    int room = S->cas_room, col = room % 5, row = room / 5, nroom = -1;
    if (lx < 8 && (CAS_DOORS[room]&8) && col > 0) nroom = room - 1;
    else if (lx > 632 && (CAS_DOORS[room]&2) && col < 4) nroom = room + 1;
    else if (ly < 36 && (CAS_DOORS[room]&1) && row > 0) nroom = room - 5;
    else if (ly > 352 && (CAS_DOORS[room]&4) && row < 3) nroom = room + 5;
    if (nroom >= 0){
        S->cas_room = nroom;
        if (nroom == room - 1){ b->x = CAS_OX + 596; b->y = CAS_OY + 194; }
        else if (nroom == room + 1){ b->x = CAS_OX + 44; b->y = CAS_OY + 194; }
        else if (nroom == room - 5){ b->x = CAS_OX + 320; b->y = CAS_OY + 316; }
        else { b->x = CAS_OX + 320; b->y = CAS_OY + 72; }
        b->bw_x = b->x; b->bw_y = b->y;
        cas_note(S, CAS_NAMES[nroom], 1.5f);
        sfx(19);
        return;
    }
    // the gate, in the courtyard where you started
    if (room == 17 && ly > 306 && lx >= 282 && lx <= 358){
        if (S->cas_key){
            S->cas_won = 1; S->cas_wont = 2.6f; S->cas_score += 100;
            cas_note(S, "THE GATE CREAKS OPEN... YOU ARE FREE!", 2.6f);
            qnote(0, 0, 60, 80, 6); qnote(0.12f, 0, 64, 80, 6); qnote(0.24f, 0, 67, 80, 6); qnote(0.36f, 0, 72, 90, 10);
            return;
        } else if (S->cas_notecd <= 0){
            S->cas_notecd = 2.0f;
            cas_note(S, "THE GATE IS LOCKED FAST. SOMEWHERE THERE IS A KEY.", 2.0f);
            sfx(16);
        }
    }
    // pickups: run into a thing to take it, like the original
    for (int id=0; id<17; id++){
        if (S->cas_taken[id] || S->cas_iroom[id] != room) continue;
        float ix, iy; cas_item_pos(pi, id, &ix, &iy);
        float dx = b->x - ix, dy = b->y - iy;
        if (dx*dx + dy*dy < 24*24){
            S->cas_taken[id] = 1;
            if (CAS_ITEM_KIND[id] == 0){
                S->cas_treasures++; S->cas_score += 100;
                char buf[48]; char *q = buf;
                q = sappend(q, "TREASURE! "); q = sapp_int(q, S->cas_treasures); q = sappend(q, " OF 13."); *q = 0;
                cas_note(S, buf, 1.8f);
                qnote(0, 0, 76, 70, 3); qnote(0.07f, 0, 81, 70, 3); qnote(0.14f, 0, 88, 80, 5);
            } else if (CAS_ITEM_KIND[id] == 1){
                S->cas_sword = 1; cas_note(S, "YOU TAKE THE SWORD. NOW YOUR BUMPS COUNT.", 2.2f); sfx(19);
            } else if (CAS_ITEM_KIND[id] == 2){
                S->cas_key = 1; cas_note(S, "THE GATE KEY! THE COURTYARD GATE WILL OPEN NOW.", 2.2f); sfx(19);
            } else {
                b->hp += 30; if (b->hp > b->maxhp) b->hp = b->maxhp;
                cas_note(S, "A TONIC! +30 HP.", 1.6f); sfx(19);
            }
            splat(ix, iy, COL(255, 220, 120), 8, 70);
        }
    }
    // the room's monsters
    for (int i=0;i<15;i++){
        if (!S->cas_malive[i] || S->cas_mroom[i] != room) continue;
        float *mxp = &S->cas_mx[i], *myp = &S->cas_my[i];
        float dx = b->x - *mxp, dy = b->y - *myp;
        float d2 = dx*dx + dy*dy;
        S->cas_mwt[i] -= dt;
        float vx2 = 0, vy2 = 0;
        if (d2 < 210*210 && d2 > 1){
            float d = fsqrt(d2); vx2 = dx/d; vy2 = dy/d;
        } else if (S->cas_mwt[i] <= 0){
            S->cas_mwt[i] = 1.2f + (float)((unsigned)(i*37 + (int)(G.anim_t*2.0f)) % 10) * 0.13f;
            unsigned h = (unsigned)i * 2654435761u + (unsigned)(int)(G.anim_t * 1.7f) * 40503u;
            vx2 = (float)((int)(h % 3) - 1); vy2 = (float)((int)((h >> 8) % 3) - 1);
        }
        if (vx2 != 0 || vy2 != 0){
            float sp = CAS_MSPD[S->cas_mtype[i]];
            float nx2 = *mxp + vx2 * sp * dt, ny2 = *myp + vy2 * sp * dt;
            if (!cas_solid(pi, nx2, *myp)) *mxp = nx2;
            if (!cas_solid(pi, *mxp, ny2)) *myp = ny2;
            dx = b->x - *mxp; dy = b->y - *myp; d2 = dx*dx + dy*dy;
        }
        // bump combat: an exchange, both ways, like the original
        if (d2 < 23*23 && S->cas_hitcd <= 0){
            S->cas_hitcd = 0.65f;
            if (S->cas_sword){
                S->cas_mhp[i]--;
                qnote(0, 3, 34, 80, 2);
                if (S->cas_mhp[i] <= 0){
                    S->cas_malive[i] = 0; S->cas_score += 10;
                    splat(*mxp, *myp, COL(200, 90, 90), 10, 80);
                    cas_note(S, "IT DIES. +10.", 1.2f);
                } else {
                    float kx = *mxp - dx * 0.5f, ky = *myp - dy * 0.5f;
                    if (kx < CAS_OX + 40) kx = CAS_OX + 40; if (kx > CAS_OX + 600) kx = CAS_OX + 600;
                    if (ky < CAS_OY + 68) ky = CAS_OY + 68; if (ky > CAS_OY + 320) ky = CAS_OY + 320;
                    if (!cas_solid(pi, kx, ky)){ *mxp = kx; *myp = ky; }
                    cas_note(S, "YOU HIT IT!", 0.9f);
                }
            } else {
                cas_note(S, "BARE HANDS DO NOTHING. YOU NEED A SWORD!", 1.6f);
            }
            b->hp -= CAS_MDMG[S->cas_mtype[i]];
            { float kx = b->x - dx * 0.35f, ky = b->y - dy * 0.35f;
              if (!cas_solid(pi, kx, ky)){ b->x = kx; b->y = ky; } }
            if (b->hp <= 0){ b->hp = 0; b->fainted = 1; bw_exit(pi, 1); return; }
        }
    }
}
static void cas_render(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    int vx = G.view_x0, vy = G.view_y0, vw2 = G.view_w>0? G.view_w : VW, vh2 = G.view_h>0? G.view_h : VH;
    float cs = (float)vw2 / 640.0f; { float csy = (float)vh2 / 360.0f; if (csy < cs) cs = csy; }
    int ox = vx + (int)((vw2 - 640*cs) / 2), oy = vy + (int)((vh2 - 360*cs) / 2);
    int room = S->cas_room;
#define CRX(lx) (ox + (int)((lx) * cs))
#define CRY(ly) (oy + (int)((ly) * cs))
#define CRS(v) ((int)((v) * cs) < 1 ? 1 : (int)((v) * cs))
    rect_fill(vx, vy, vw2, vh2, COL(6, 8, 6));
    // floor: a dim checker, text-mode green — v1.22: every flagstone
    // wears its own shade now (hashed, never stored) with a sparse
    // grit speckle, so the courtyard reads as stone, not graph paper
    for (int ty = 0; ty < 12; ty++) for (int tx = 0; tx < 20; tx++){
        u32 fb = ((tx + ty) & 1)? COL(13, 19, 13) : COL(10, 15, 10);
        u32 fc = col_shade(fb, 0.90f + hash_f(tx, ty, 7)*0.20f);
        rect_fill(CRX(tx*32), CRY(ty*32), CRS(32) + 1, CRS(32) + 1, fc);
        float sp = hash_f(tx, ty, 21);
        if (sp > 0.70f){
            u32 sc = col_shade(fb, 1.35f);
            pset(CRX(tx*32 + (int)(sp*83.0f)%26 + 3), CRY(ty*32 + (int)(sp*47.0f)%26 + 3), sc);
            pset(CRX(tx*32 + (int)(sp*29.0f)%22 + 6), CRY(ty*32 + (int)(sp*61.0f)%22 + 6), sc);
        }
    }
    // the stone walls, with mortar lines
    { u32 wc = COL(74, 88, 74), wl = COL(52, 64, 52);
      rect_fill(CRX(0), CRY(0), CRS(640), CRS(CAS_Y0), wc);
      rect_fill(CRX(0), CRY(CAS_Y1), CRS(640), CRS(360 - CAS_Y1), wc);
      rect_fill(CRX(0), CRY(0), CRS(CAS_X0), CRS(360), wc);
      rect_fill(CRX(CAS_X1), CRY(0), CRS(640 - CAS_X1), CRS(360), wc);
      for (int bx = 0; bx < 640; bx += 32){ rect_fill(CRX(bx), CRY(0), 1, CRS(CAS_Y0), wl); rect_fill(CRX(bx), CRY(CAS_Y1), 1, CRS(360 - CAS_Y1), wl); }
      for (int by = 0; by < 360; by += 32){ rect_fill(CRX(0), CRY(by), CRS(CAS_X0), 1, wl); rect_fill(CRX(CAS_X1), CRY(by), CRS(640 - CAS_X1), 1, wl); }
      rect_fill(CRX(0), CRY(CAS_Y0), CRS(640), 2, COL(120, 220, 130));
      // doors: dark openings with a wooden leaf swung in
      if (CAS_DOORS[room]&1){ rect_fill(CRX(288), CRY(0), CRS(64), CRS(CAS_Y0), COL(4, 4, 4)); rect_fill(CRX(288), CRY(CAS_Y0 - 12), CRS(10), CRS(12), COL(120, 84, 44)); }
      if (CAS_DOORS[room]&4){ rect_fill(CRX(288), CRY(CAS_Y1), CRS(64), CRS(360 - CAS_Y1), COL(4, 4, 4)); rect_fill(CRX(342), CRY(CAS_Y1), CRS(10), CRS(12), COL(120, 84, 44)); }
      if (CAS_DOORS[room]&8){ rect_fill(CRX(0), CRY(162), CRS(CAS_X0), CRS(64), COL(4, 4, 4)); rect_fill(CRX(CAS_X0 - 12), CRY(162), CRS(12), CRS(10), COL(120, 84, 44)); }
      if (CAS_DOORS[room]&2){ rect_fill(CRX(CAS_X1), CRY(162), CRS(640 - CAS_X1), CRS(64), COL(4, 4, 4)); rect_fill(CRX(CAS_X1), CRY(216), CRS(12), CRS(10), COL(120, 84, 44)); }
      // the courtyard gate: iron bars on the south wall (never a door)
      if (room == 17){
          int lift = S->cas_won? CRS(16) : 0;
          rect_fill(CRX(288), CRY(CAS_Y1), CRS(64), CRS(360 - CAS_Y1), COL(4, 4, 4));
          for (int bx = 292; bx < 350; bx += 8) rect_fill(CRX(bx), CRY(CAS_Y1) - lift, CRS(3), CRS(360 - CAS_Y1), COL(150, 150, 160));
          rect_fill(CRX(288), CRY(CAS_Y1 + 10) - lift, CRS(64), CRS(3), COL(150, 150, 160));
          if (!S->cas_key) disc_fill(CRX(320), CRY(346), CRS(5), COL(255, 205, 40));
      }
    }
    // v1.22: the walls fall off into shade where they meet the floor,
    // and every doorway wears a pair of flickering sconces — the
    // castle is lit now, not just drawn
    blend_rect(CRX(CAS_X0), CRY(CAS_Y0), CRS(CAS_X1 - CAS_X0), CRS(9), COL(0, 0, 0), 56);
    blend_rect(CRX(CAS_X0), CRY(CAS_Y1) - CRS(9), CRS(CAS_X1 - CAS_X0), CRS(9), COL(0, 0, 0), 56);
    blend_rect(CRX(CAS_X0), CRY(CAS_Y0), CRS(9), CRS(CAS_Y1 - CAS_Y0), COL(0, 0, 0), 56);
    blend_rect(CRX(CAS_X1) - CRS(9), CRY(CAS_Y0), CRS(9), CRS(CAS_Y1 - CAS_Y0), COL(0, 0, 0), 56);
    { static const short SCN[8][3] = { {1, 272, 28}, {1, 368, 28}, {4, 272, 346}, {4, 368, 346},
                                       {8, 14, 146}, {8, 14, 242}, {2, 626, 146}, {2, 626, 242} };
      for (int si = 0; si < 8; si++){
          if (!(CAS_DOORS[room] & SCN[si][0])) continue;
          int sx2 = CRX(SCN[si][1]), sy2 = CRY(SCN[si][2]);
          int fl = 82 + (int)(fsin(G.anim_t*11.0f + (float)si*2.3f)*16.0f) + (int)(hash2(SCN[si][1], SCN[si][2], 5u)%12u);
          glow_ellipse(sx2, sy2, CRS(20), CRS(13), COL(255, 170, 70), fl);
          disc_fill(sx2, sy2, CRS(3), COL(255, 214, 120));
          disc_fill(sx2, sy2 - CRS(2), CRS(1) + 1, COL(255, 240, 200));
      } }
    // the room's stone block
    { const short *P = CAS_PIL[room];
      if (P[2] > 0){ rect_fill(CRX(P[0]), CRY(P[1]), CRS(P[2]), CRS(P[3]), COL(64, 76, 64)); rect_line(CRX(P[0]), CRY(P[1]), CRS(P[2]), CRS(P[3]), COL(96, 110, 96)); } }
    // items
    for (int id = 0; id < 17; id++){
        if (S->cas_taken[id] || S->cas_iroom[id] != room) continue;
        float fx, fy; cas_item_pos(pi, id, &fx, &fy);
        int cx = CRX(fx - CAS_OX), cy = CRY(fy - CAS_OY);
        int bob = (int)(fsin(G.anim_t * 3.0f + (float)id * 1.7f) * 2.0f);
        cy += CRS(bob);
        if (CAS_ITEM_KIND[id] == 0){           // a treasure: gold and glinting
            // v1.22: a slow breathing halo so the gold calls across the room
            glow_ellipse(cx, cy, CRS(14), CRS(10), COL(255, 205, 40), 58 + (int)(fsin(G.anim_t*3.0f + (float)id*1.7f)*18.0f));
            disc_fill(cx, cy, CRS(8), COL(255, 205, 40));
            tri_fill(cx - CRS(8), cy, cx, cy - CRS(10), cx + CRS(8), cy, COL(255, 230, 130));
            disc_fill(cx - CRS(3), cy - CRS(3), CRS(2), COL(255, 250, 220));
        } else if (CAS_ITEM_KIND[id] == 1){    // the sword
            rect_fill(cx - CRS(2), cy - CRS(13), CRS(4), CRS(18), COL(215, 220, 230));
            rect_fill(cx - CRS(6), cy + CRS(5), CRS(12), CRS(3), COL(255, 205, 40));
            rect_fill(cx - CRS(2), cy + CRS(8), CRS(4), CRS(6), COL(120, 84, 44));
        } else if (CAS_ITEM_KIND[id] == 2){    // the gate key
            disc_fill(cx - CRS(4), cy - CRS(4), CRS(5), COL(255, 205, 40));
            rect_fill(cx, cy - CRS(1), CRS(13), CRS(3), COL(255, 205, 40));
            rect_fill(cx + CRS(9), cy + CRS(2), CRS(3), CRS(5), COL(255, 205, 40));
        } else {                               // a tonic
            rect_fill(cx - CRS(5), cy - CRS(6), CRS(10), CRS(14), COL(200, 70, 70));
            rect_fill(cx - CRS(2), cy - CRS(10), CRS(4), CRS(4), COL(200, 200, 210));
            rect_fill(cx - CRS(5), cy - CRS(1), CRS(10), CRS(2), COL(240, 150, 150));
        }
    }
    // monsters
    for (int i = 0; i < 15; i++){
        if (!S->cas_malive[i] || S->cas_mroom[i] != room) continue;
        int cx = CRX(S->cas_mx[i] - CAS_OX), cy = CRY(S->cas_my[i] - CAS_OY);
        int ty = S->cas_mtype[i];
        // v1.22: the grounded monsters cast a shadow now; the bat flies
        if (ty == 1) shadow_ellipse(cx, cy + CRS(7), CRS(11), CRS(3));
        else if (ty == 2) shadow_ellipse(cx, cy + CRS(15), CRS(13), CRS(4));
        else if (ty == 3) shadow_ellipse(cx, cy + CRS(15), CRS(12), CRS(4));
        if (ty == 0){           // bat
            int flap = (int)(fsin(G.anim_t * 10.0f + (float)i * 2.0f) * CRS(5));
            tri_fill(cx, cy, cx - CRS(16), cy - CRS(6) - flap, cx - CRS(13), cy + CRS(5), COL(96, 84, 150));
            tri_fill(cx, cy, cx + CRS(16), cy - CRS(6) - flap, cx + CRS(13), cy + CRS(5), COL(96, 84, 150));
            disc_fill(cx, cy, CRS(6), COL(56, 48, 84));
            disc_fill(cx - CRS(2), cy - CRS(2), CRS(1) + 1, COL(255, 90, 90)); disc_fill(cx + CRS(2), cy - CRS(2), CRS(1) + 1, COL(255, 90, 90));
        } else if (ty == 1){    // snake
            for (int sg = 0; sg < 3; sg++){
                int sx2 = cx + (sg - 1) * CRS(9), sy2 = cy + (int)(fsin(G.anim_t * 6.0f + (float)(i + sg) * 1.3f) * CRS(3));
                disc_fill(sx2, sy2, CRS(6), COL(80, 180, 80));
            }
            disc_fill(cx + CRS(8), cy - CRS(2), CRS(1) + 1, COL(255, 220, 120));
        } else if (ty == 2){    // ogre
            rect_fill(cx - CRS(11), cy - CRS(6), CRS(22), CRS(20), COL(122, 142, 70));
            disc_fill(cx, cy - CRS(12), CRS(8), COL(150, 168, 96));
            rect_fill(cx - CRS(16), cy - CRS(4), CRS(5), CRS(14), COL(122, 142, 70));
            rect_fill(cx + CRS(11), cy - CRS(4), CRS(5), CRS(14), COL(122, 142, 70));
            disc_fill(cx - CRS(3), cy - CRS(13), CRS(1) + 1, COL(30, 26, 20)); disc_fill(cx + CRS(3), cy - CRS(13), CRS(1) + 1, COL(30, 26, 20));
        } else {                // demon
            rect_fill(cx - CRS(10), cy - CRS(8), CRS(20), CRS(22), COL(172, 62, 52));
            tri_fill(cx - CRS(10), cy - CRS(8), cx - CRS(13), cy - CRS(18), cx - CRS(4), cy - CRS(11), COL(240, 220, 180));
            tri_fill(cx + CRS(10), cy - CRS(8), cx + CRS(13), cy - CRS(18), cx + CRS(4), cy - CRS(11), COL(240, 220, 180));
            disc_fill(cx - CRS(3), cy - CRS(3), CRS(1) + 1, COL(255, 220, 90)); disc_fill(cx + CRS(3), cy - CRS(3), CRS(1) + 1, COL(255, 220, 90));
        }
        if (S->cas_mhp[i] < CAS_MHP[ty])   // hurt pips
            for (int hp2 = 0; hp2 < S->cas_mhp[i]; hp2++)
                rect_fill(cx - CRS(8) + hp2 * CRS(6), cy - CRS(24), CRS(4), CRS(3), COL(255, 120, 120));
    }
    // the adventurer (the buddy, in a green cloak — a nod to a clover)
    { int cx = CRX(b->x - CAS_OX), cy = CRY(b->y - CAS_OY) + (int)(fsin(b->walk_ph) * cs * 1.5f);
      ellipse_fill(cx, cy + CRS(13), CRS(11), CRS(4), COL(0, 0, 0));
      tri_fill(cx - CRS(10), cy + CRS(14), cx, cy - CRS(4), cx + CRS(10), cy + CRS(14), COL(52, 142, 72));
      rect_fill(cx - CRS(7), cy - CRS(2), CRS(14), CRS(14), COL(52, 142, 72));
      disc_fill(cx, cy - CRS(8), CRS(6), COL(240, 200, 164));
      tri_fill(cx - CRS(7), cy - CRS(9), cx, cy - CRS(17), cx + CRS(7), cy - CRS(9), COL(38, 110, 54));
      if (S->cas_sword){
          int hx = cx + (int)(fcos(b->aim) * CRS(13)), hy = cy + (int)(fsin(b->aim) * CRS(13));
          line_thick(cx, cy, hx, hy, CRS(3), COL(215, 220, 230));
          disc_fill(cx + (int)(fcos(b->aim) * CRS(8)), cy + (int)(fsin(b->aim) * CRS(8)), CRS(2), COL(255, 205, 40));
      }
      if (S->cas_hitcd > 0.35f){ rect_line(cx - CRS(13), cy - CRS(20), CRS(26), CRS(36), COL(255, 90, 90)); }   // hurt flash frame
    }
    // HUD, written on the top wall
    { char buf[80]; char *q;
      draw_text(CRX(40), CRY(10), S->cas_shuffle? "CASTLE - SHUFFLE" : "CASTLE", 1, COL(255, 205, 40));
      draw_text_center(CRX(320), CRY(10), CAS_NAMES[room], 1, COL(190, 235, 190));
      q = buf; q = sappend(q, "SCORE "); q = sapp_int(q, S->cas_score); *q = 0;
      draw_text(CRX(600) - (int)(slen(buf) * 8 * cs), CRY(10), buf, 1, COL(235, 238, 245));
      draw_text(CRX(40), CRY(30), "HP", 1, COL(190, 235, 190));
      rect_fill(CRX(66), CRY(30), CRS(110), CRS(8), COL(40, 44, 40));
      { int hw = b->maxhp > 0? (int)(110.0f * b->hp / b->maxhp) : 0; if (hw < 0) hw = 0;
        if (hw > 0) rect_fill(CRX(66), CRY(30), CRS(hw), CRS(8), (b->hp * 3 < b->maxhp)? COL(230, 80, 80) : COL(110, 220, 120)); }
      q = buf; q = sappend(q, "TREASURES "); q = sapp_int(q, S->cas_treasures); q = sappend(q, "/13"); *q = 0;
      draw_text_center(CRX(320), CRY(30), buf, 1, COL(255, 220, 120));
      { int ix = CRX(470);
        if (S->cas_sword){ draw_text(ix, CRY(30), "SWORD", 1, COL(215, 220, 230)); ix += (int)(48 * cs); }
        if (S->cas_key) draw_text(ix, CRY(30), "KEY", 1, COL(255, 205, 40)); }
    }
    if (S->cas_won){
        rect_fill(CRX(120), CRY(120), CRS(400), CRS(110), COL(16, 20, 16));
        rect_line(CRX(120), CRY(120), CRS(400), CRS(110), COL(255, 205, 40));
        draw_text_center(CRX(320), CRY(142), "YOU ESCAPED THE CASTLE!", 2, COL(255, 220, 120));
        { char buf[64]; char *q = buf;
          q = sappend(q, "SCORE "); q = sapp_int(q, S->cas_score);
          q = sappend(q, "   BEST "); q = sapp_int(q, S->cas_score > b->cas_best? S->cas_score : b->cas_best); *q = 0;
          draw_text_center(CRX(320), CRY(182), buf, 1, COL(235, 238, 245)); }
    }
    if (S->note_t > 0){
        rect_fill(vx, vy + vh2 - 22, vw2, 22, COL(10, 12, 10));
        draw_text_center(vx + vw2/2, vy + vh2 - 15, S->note, 1, COL(220, 255, 220));
    }
#undef CRX
#undef CRY
#undef CRS
}

