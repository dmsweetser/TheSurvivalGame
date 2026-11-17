// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: cart.c — SUPER BUDDY CART (bw_mode 7).
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]
// ---------------------------------------------------------------------------
// SUPER BUDDY CART (bw_mode 7, Nov 14 2026, Daniel): a kart racer in
// the Mario Kart mould, small and honest — one single-screen circuit
// (a rounded-rectangle loop in a 640x360 logical frame, like CASTLE's
// rooms), 3 laps, 6 racers: your buddy plus five AI rivals with a
// little rubber in their bands. Grass is the wall: leave the road
// and you trudge. Three boost pads reward the racing line. The
// records (best finish, best time) live on the Buddy as cp / cq.
// ---------------------------------------------------------------------------
#define CRT_OX 30000.0f
#define CRT_OY 20000.0f
#define CRT_MAXW 168
static float CRT_WX[CRT_MAXW], CRT_WY[CRT_MAXW], CRT_CUM[CRT_MAXW + 1];
static int CRT_N = 0;
static float CRT_LEN = 0, CRT_OFF = 0;
static float CRT_PAD[3][2];
// trackside scenery: trees, tire stacks, one grandstand (Nov 14 overhaul)
#define CRT_SCN_N 49
static float CRT_SCX[CRT_SCN_N], CRT_SCY[CRT_SCN_N]; static int CRT_SCT[CRT_SCN_N];
static void crt_point_at(float ap, float lat, float *x, float *y, float *hx, float *hy);
static u32 crt_coat_for(int species, int color, int slot){
    static const u32 NAT[BUDN] = { COL(199,152,94), COL(228,152,72), COL(143,106,68), COL(128,92,58), COL(148,144,138), COL(238,240,242) };
    if (color == 9) return bud_hsv(0.11f + slot * 0.19f + G.anim_t * 0.22f);
    Buddy tmp; tmp.species = species % BUDN; tmp.color = color; tmp.active = 1;
    return bud_coat(&tmp, NAT[species % BUDN]);
}
static float crt_wrap(float a){
    if (a > PI) a -= 2*PI; if (a < -PI) a += 2*PI;
    if (a > PI) a -= 2*PI; if (a < -PI) a += 2*PI;
    return a;
}
static void crt_addpt(float x, float y){
    if (CRT_N > 0){ float dx = x - CRT_WX[CRT_N-1], dy = y - CRT_WY[CRT_N-1];
        if (dx*dx + dy*dy < 25) return; }
    if (CRT_N < CRT_MAXW){ CRT_WX[CRT_N] = x; CRT_WY[CRT_N] = y; CRT_N++; }
}
static void crt_line(float x0, float y0, float x1, float y1){
    float d = fsqrt((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0));
    int n = (int)(d / 11.0f); if (n < 1) n = 1;
    for (int i = 0; i < n; i++){ float t = (float)i / (float)n;
        crt_addpt(x0 + (x1-x0)*t, y0 + (y1-y0)*t); }
}
static void crt_arc(float cx, float cy, float a0, float a1){
    int n = (int)(ffabs(a1-a0) * 70.0f / 11.0f); if (n < 2) n = 2;
    for (int i = 0; i < n; i++){ float a = a0 + (a1-a0) * (float)i / (float)n;
        crt_addpt(cx + 70.0f*fcos(a), cy + 70.0f*fsin(a)); }
}
// one circuit per REGION (Nov 14): track 0 is the hand-built
// Meadows oval; tracks 1-7 are radial circuits whose wiggle grows
// with the region's distance from the epicenter — the far country
// is twistier, exactly like its bugs are meaner. a1/a2: lobe
// amplitudes (2- and 3-lobed), p1/p2: their phases.
// one sky per region for the kart view, same discipline as the run
static const u32 CRT_SKY[NREGIONS][2] = {
    { COL(96,168,235),  COL(196,228,250) },   // Clean Code Meadows
    { COL(104,172,228), COL(206,232,246) },   // Spaghetti Fields
    { COL(122,158,196), COL(214,220,208) },   // Legacy Lands
    { COL(206,158,106), COL(240,214,170) },   // Kernel Badlands
    { COL(168,204,230), COL(226,240,250) },   // Frozen Cache Fields
    { COL(36,32,88),    COL(96,84,160) },     // Neon RAM Flats (dusk)
    { COL(80,146,124),  COL(190,226,196) },   // Cable Jungle
    { COL(92,88,112),   COL(168,162,180) },   // The Unmaintained Depths
};
// the standings order: points, then the latest race's place, then grid order
static void crt_order(BwSim *S, int *ord){
    for (int i = 0; i < 6; i++) ord[i] = i;
    for (int a = 0; a < 5; a++) for (int b2 = a + 1; b2 < 6; b2++){
        int x = ord[a], y = ord[b2], swap = 0;
        if (S->crt_pts[y] > S->crt_pts[x]) swap = 1;
        else if (S->crt_pts[y] == S->crt_pts[x]){
            int lx = S->crt_lastpl[x]? S->crt_lastpl[x] : 9, ly = S->crt_lastpl[y]? S->crt_lastpl[y] : 9;
            if (ly < lx) swap = 1;
        }
        if (swap){ ord[a] = y; ord[b2] = x; }
    }
}
static const float CRT_PARAM[7][4] = {
    { 10, 0,  0.6f, 0.0f },   // 1 Spaghetti Fields: a gentle sway
    { 15, 5,  2.1f, 0.4f },   // 2 Legacy Lands
    { 18, 8,  4.0f, 2.2f },   // 3 Kernel Badlands
    { 13, 13, 1.2f, 3.9f },   // 4 Frozen Cache Fields
    { 20, 10, 5.3f, 1.1f },   // 5 Neon RAM Flats
    { 16, 16, 0.2f, 5.0f },   // 6 Cable Jungle
    { 22, 13, 3.3f, 2.8f },   // 7 The Unmaintained Depths
};
static void crt_build(int track){
    CRT_N = 0;
    if (track <= 0){
        crt_line(180, 290, 460, 290);          // bottom straight (start/finish)
        crt_arc(460, 220, PI/2, 0);            // bottom-right corner
        crt_line(530, 220, 530, 140);          // right straight
        crt_arc(460, 140, 0, -PI/2);           // top-right corner
        crt_line(460, 70, 180, 70);            // top straight
        crt_arc(180, 140, -PI/2, -PI);         // top-left corner
        crt_line(110, 140, 110, 220);          // left straight
        crt_arc(180, 220, PI, PI/2);           // bottom-left corner, closing the loop
    } else {
        const float *P = CRT_PARAM[track-1];
        int M = 168;
        for (int i = 0; i < M; i++){
            float th = (float)i / (float)M * 6.2831853f;
            float r = 92.0f + P[0]*fsin(2.0f*th + P[2]) + P[1]*fsin(3.0f*th + P[3]);
            crt_addpt(320.0f + fcos(th) * r * 1.82f, 180.0f + fsin(th) * r);
        }
    }
    CRT_CUM[0] = 0;
    for (int i = 1; i <= CRT_N; i++){ int a = i-1, b = i % CRT_N;
        float dx = CRT_WX[b] - CRT_WX[a], dy = CRT_WY[b] - CRT_WY[a];
        CRT_CUM[i] = CRT_CUM[i-1] + fsqrt(dx*dx + dy*dy); }
    CRT_LEN = CRT_CUM[CRT_N];
    { int bi = 0; float bd = 1e18f;        // the start line sits at the bottom, near (320, 290)
      for (int i = 0; i < CRT_N; i++){ float dx = CRT_WX[i]-320, dy = CRT_WY[i]-290;
          float d = dx*dx + dy*dy; if (d < bd){ bd = d; bi = i; } }
      CRT_OFF = CRT_CUM[bi]; }
    if (track <= 0){
        CRT_PAD[0][0] = 320; CRT_PAD[0][1] = 70;
        CRT_PAD[1][0] = 530; CRT_PAD[1][1] = 180;
        CRT_PAD[2][0] = 110; CRT_PAD[2][1] = 180;
    } else {
        static const float PF[3] = { 0.30f, 0.56f, 0.82f };
        for (int k = 0; k < 3; k++){ float x, y, hx, hy;
            crt_point_at(CRT_OFF + PF[k] * CRT_LEN, 0, &x, &y, &hx, &hy);
            CRT_PAD[k][0] = x; CRT_PAD[k][1] = y; }
    }
    // scenery: a ring of trees and tire stacks outside the road, a few
    // infield trees, and the grandstand watching the top straight
    { int n = 0;
      for (int k = 0; k < 40 && n < CRT_SCN_N - 9; k++){
          u32 h = hash2(k * 7 + 3, 91, 0xCA27u);
          float ap = (k + 0.5f) * CRT_LEN / 40.0f + (float)(h % 17) - 8.0f;
          float lat = (h & 64)? 1.0f : -1.0f;
          lat *= 56.0f + (float)((h >> 8) % 34);
          float x, y, hx, hy; crt_point_at(ap, lat, &x, &y, &hx, &hy);
          if (x < 14 || x > 626 || y < 14 || y > 346) continue;
          CRT_SCX[n] = x; CRT_SCY[n] = y; CRT_SCT[n] = ((h >> 12) % 6 == 0)? 1 : 0; n++;
      }
      if (track <= 0){
          static const float INF[8][2] = { {320,180},{240,150},{400,210},{270,225},{370,140},{200,200},{440,160},{320,118} };
          for (int k = 0; k < 8 && n < CRT_SCN_N - 1; k++){ CRT_SCX[n] = INF[k][0]; CRT_SCY[n] = INF[k][1]; CRT_SCT[n] = 0; n++; }
          CRT_SCX[n] = 320; CRT_SCY[n] = 14; CRT_SCT[n] = 3; n++;
      } else {
          // infield: sample the inside of the loop, keeping only
          // spots a clear 50 units from the asphalt
          for (int k = 0; k < 14 && n < CRT_SCN_N - 2; k++){
              float x, y, hx, hy;
              crt_point_at(CRT_OFF + (k + 0.5f) * CRT_LEN / 14.0f, -72.0f - (float)(k % 3) * 14.0f, &x, &y, &hx, &hy);
              if (x < 20 || x > 620 || y < 20 || y > 340) continue;
              float bd2 = 1e18f;
              for (int i = 0; i < CRT_N; i += 2){ float dx = CRT_WX[i]-x, dy = CRT_WY[i]-y; float d = dx*dx + dy*dy; if (d < bd2) bd2 = d; }
              if (bd2 < 50.0f * 50.0f) continue;
              CRT_SCX[n] = x; CRT_SCY[n] = y; CRT_SCT[n] = 0; n++;
          }
          // the grandstand watches the start from outside the loop
          // (Nov 15: pushed further out and further past the line —
          // at 116 it loomed over the grid as a screen-filling wall)
          { float x, y, hx, hy; crt_point_at(CRT_OFF + CRT_LEN * 0.10f, 152.0f, &x, &y, &hx, &hy);
            if (x > 56 && x < 584 && y > 22 && y < 338){ CRT_SCX[n] = x; CRT_SCY[n] = y; CRT_SCT[n] = 3; n++; } }
      }
      while (n < CRT_SCN_N){ CRT_SCX[n] = -100; CRT_SCY[n] = -100; CRT_SCT[n] = 0; n++; }
    }
}
static void crt_point_at(float ap, float lat, float *x, float *y, float *hx, float *hy){
    ap = ap - (float)(int)(ap / CRT_LEN) * CRT_LEN; if (ap < 0) ap += CRT_LEN;
    int i = 0; while (i < CRT_N-1 && CRT_CUM[i+1] <= ap) i++;
    int j = (i + 1) % CRT_N;
    float seg = CRT_CUM[i+1] - CRT_CUM[i];
    float t = seg > 0? (ap - CRT_CUM[i]) / seg : 0;
    float px = CRT_WX[i] + (CRT_WX[j] - CRT_WX[i]) * t;
    float py = CRT_WY[i] + (CRT_WY[j] - CRT_WY[i]) * t;
    float dx = CRT_WX[j] - CRT_WX[i], dy = CRT_WY[j] - CRT_WY[i];
    float d = fsqrt(dx*dx + dy*dy); if (d < 0.001f) d = 1; dx /= d; dy /= d;
    *x = px + (-dy) * lat; *y = py + dx * lat;
    if (hx) *hx = dx; if (hy) *hy = dy;
}
static int crt_nearest_full(float x, float y){
    int bi = 0; float bd = 1e18f;
    for (int i = 0; i < CRT_N; i++){ float dx = x - CRT_WX[i], dy = y - CRT_WY[i];
        float d = dx*dx + dy*dy; if (d < bd){ bd = d; bi = i; } }
    return bi;
}
static void crt_track_prog(BwSim *S, int i){
    int prev = S->crt_seg[i]; int bi = prev; float bd = 1e18f;
    for (int k = -16; k <= 16; k++){ int idx = (prev + k) % CRT_N; if (idx < 0) idx += CRT_N;
        float dx = S->crt_x[i] - CRT_WX[idx], dy = S->crt_y[i] - CRT_WY[idx];
        float d = dx*dx + dy*dy; if (d < bd){ bd = d; bi = idx; } }
    float d = CRT_CUM[bi] - CRT_CUM[prev];
    if (d < -CRT_LEN/2) d += CRT_LEN; else if (d > CRT_LEN/2) d -= CRT_LEN;
    S->crt_prog[i] += d; S->crt_seg[i] = bi;
}
static void crt_places(BwSim *S){
    for (int i = 0; i < 6; i++){ int pl = 1;
        for (int j = 0; j < 6; j++){ if (j == i) continue;
            if (S->crt_finplace[j] && (!S->crt_finplace[i] || S->crt_finplace[j] < S->crt_finplace[i])) pl++;
            else if (!S->crt_finplace[j] && !S->crt_finplace[i] && S->crt_prog[j] > S->crt_prog[i]) pl++;
        }
        S->crt_place[i] = pl; }
}
static BwIn crt_gin[MAXP];   // a race guest's latest input, keyed by host pi
// picking the kart cabinet lands you in the WAITING ROOM (state 3):
// choose your colour, and in local co-op wait for the other player to
// pick the cart on their own side too — only then are you linked
static void crt_enter(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    S->crt_cup = 0; S->crt_race = 0; S->crt_track = 0;
    for (int i = 0; i < 6; i++){ S->crt_pts[i] = 0; S->crt_lastpl[i] = 0; S->crt_gprank[i] = 0; }
    crt_build(0);
    S->crt_state = 3; S->crt_t = 0; S->crt_count = 3.4f; S->crt_fint = 0;
    S->crt_fincount = 0; S->crt_player_place = 0;
    S->crt_duo = -1; S->crt_guest = 0;
    S->crt_hfin[0] = 0; S->crt_hfin[1] = 0; S->crt_endt = -1.0f;
    S->crt_colsel = (pi == G.p2_slot && G.my_id != pi)? 1 : 0;
    S->crt_colsel2 = 1; S->crt_ready = 0; S->crt_prevk = 0; S->crt_gprevk = 0;
    for (int i = 0; i < 6; i++) S->crt_kcol[i] = i;
    b->x = CRT_OX + 320; b->y = CRT_OY + 295;
    b->bw_x = b->x; b->bw_y = b->y;
}
// the waiting room is over: lay out the grid and start the countdown
static void crt_start_race(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    S->crt_state = 0; S->crt_t = 0; S->crt_count = 3.4f; S->crt_fint = 0;
    S->crt_fincount = 0; S->crt_player_place = 0;
    S->crt_hfin[0] = 0; S->crt_hfin[1] = 0; S->crt_endt = -1.0f;
    for (int q2 = 0; q2 < 20; q2++) S->crt_puffl[q2] = 0;
    S->crt_puffn = 0; S->crt_pufft = 0;
    S->crt_prevplace[0] = S->crt_prevplace[1] = 0;
    S->crt_prevlap[0] = S->crt_prevlap[1] = 0;
    static const float SKILL[5] = { 0.95f, 0.97f, 0.99f, 1.005f, 1.02f };
    static const int SLOT[6] = { 3, 0, 1, 2, 4, 5 };   // your buddy starts 4th: a race, not a parade
    for (int i = 0; i < 6; i++){
        int slot = SLOT[i];
        float back = 30.0f + (float)(slot / 2) * 26.0f;
        float lat = (slot % 2 == 0)? -15.0f : 15.0f;
        float x, y, hx, hy; crt_point_at(CRT_OFF - back, lat, &x, &y, &hx, &hy);
        S->crt_x[i] = x; S->crt_y[i] = y; S->crt_h[i] = fatan2(hy, hx);
        S->crt_v[i] = 0; S->crt_prog[i] = -back; S->crt_boost[i] = 0;
        S->crt_finplace[i] = 0; S->crt_place[i] = slot + 1;
        S->crt_seg[i] = crt_nearest_full(x, y);
        S->crt_off[i] = (i == 0)? 0 : ((i % 2 == 0)? -1 : 1) * (7.0f + (float)((i * 37) % 9));
        // the far country's rivals are quicker, like its bugs
        S->crt_skill[i] = (i == 0)? 1.0f : SKILL[i-1] * (0.99f + rng_f() * 0.02f) * (1.0f + 0.012f * (float)S->crt_track);
    }
    // colours: the humans wear their waiting-room picks, the rivals
    // take whatever is left in palette order
    { int used[6] = { 0, 0, 0, 0, 0, 0 };
      S->crt_kcol[0] = S->crt_colsel; used[S->crt_colsel] = 1;
      int start = 1;
      if (S->crt_duo >= 0){ S->crt_kcol[1] = S->crt_colsel2; used[S->crt_colsel2] = 1; start = 2; }
      int nxt = 0;
      for (int i = start; i < 6; i++){ while (used[nxt]) nxt++; S->crt_kcol[i] = nxt; used[nxt] = 1; } }
    // the DRIVERS: the humans race as their own buddies — species, coat
    // and name come straight off the Buddy. The AI seats are fellow
    // Banzai Buddies out for a spin, one of each species in the stable.
    { Buddy *hb = &G.bud[pi];
      S->crt_species[0] = hb->species; S->crt_dcolidx[0] = hb->color; S->crt_nameidx[0] = hb->name_idx;
      int start = 1;
      if (S->crt_duo >= 0){ Buddy *gb = &G.bud[S->crt_duo];
          S->crt_species[1] = gb->species; S->crt_dcolidx[1] = gb->color; S->crt_nameidx[1] = gb->name_idx; start = 2; }
      static const int AI_SP[6] = { 1, 2, 3, 5, 4, 0 };
      static const int AI_CL[6] = { 0, 3, 5, 0, 0, 2 };
      static const int AI_NM[6] = { 7, 22, 40, 51, 58, 17 };
      for (int i = start; i < 6; i++){ int a = (i - start) % 6;
          S->crt_species[i] = AI_SP[a]; S->crt_dcolidx[i] = AI_CL[a]; S->crt_nameidx[i] = AI_NM[a];
          S->crt_line[i] = S->crt_off[i]; S->crt_slidet[i] = 0; }
      S->crt_slidet[0] = 0; if (S->crt_duo >= 0) S->crt_slidet[1] = 0;
    }
    b->x = CRT_OX + S->crt_x[0]; b->y = CRT_OY + S->crt_y[0];
    b->bw_x = b->x; b->bw_y = b->y; b->aim = S->crt_h[0];
    { char *q = S->note; q = sappend(q, "RACE "); q = sapp_int(q, S->crt_race + 1); q = sappend(q, "/4 - "); q = sappend(q, REGION_NAME[S->crt_track]); q = sappend(q, ". HOLD FIRE TO GO!"); *q = 0; S->note_t = 2.4f; }
    if (S->crt_duo >= 0){ Buddy *gb = &G.bud[S->crt_duo];
        gb->x = CRT_OX + S->crt_x[1]; gb->y = CRT_OY + S->crt_y[1];
        gb->bw_x = gb->x; gb->bw_y = gb->y; gb->aim = S->crt_h[1]; }
}

// one kart under human hands (host = kart 0, guest = kart 1)
static void crt_drive_kart(BwSim *S, int i, int keys, int go, float dt){
    int bi = crt_nearest_full(S->crt_x[i], S->crt_y[i]);
    float dxl = S->crt_x[i] - CRT_WX[bi], dyl = S->crt_y[i] - CRT_WY[bi];
    int onroad = (dxl*dxl + dyl*dyl) <= 31.0f * 31.0f;
    float vmax = onroad? 168.0f : 62.0f;
    if (S->crt_boost[i] > 0){ S->crt_boost[i] -= dt; vmax *= 1.38f; }
    float v = S->crt_v[i];
    // GO is the FIRE button held down (Nov 14, Daniel): the stick is
    // for steering only — up no longer accelerates. Down still brakes
    // and reverses; letting go of FIRE coasts.
    if (go){
        if (v > vmax){ v -= 150.0f * dt; if (v < vmax) v = vmax; }
        else { v += (v < 60.0f? 265.0f : 165.0f) * dt; if (v > vmax) v = vmax; }   // a proper launch, then the usual pull
    } else if (keys & K_DOWN){
        v -= 260.0f * dt; if (v < -58.0f) v = -58.0f;
    } else {
        if (v > 0){ v -= 90.0f * dt; if (v < 0) v = 0; }
        else if (v < 0){ v += 90.0f * dt; if (v > 0) v = 0; }
        if (v > vmax){ v -= 170.0f * dt; if (v < vmax) v = vmax; }
    }
    float steer = 0;
    if (keys & K_LEFT) steer -= 1;
    if (keys & K_RIGHT) steer += 1;
    float sf = v / 55.0f; if (sf > 1) sf = 1; if (sf < -1) sf = -1;
    S->crt_h[i] = crt_wrap(S->crt_h[i] + steer * 2.75f * sf * dt);
    // DRIFT (Nov 14): hold a hard steer at speed and the rear steps
    // out — the kart slides wide while the charge builds; let go
    // after a real slide and the stored-up grip fires you forward.
    if (steer != 0 && v > 118.0f){
        S->crt_slidet[i] += dt;
        if (S->crt_slidet[i] > 0.28f){
            float lx = fsin(S->crt_h[i]), ly = -fcos(S->crt_h[i]);   // leftward unit
            float mag = (v - 100.0f) * 0.38f * steer;
            S->crt_x[i] += lx * mag * dt;
            S->crt_y[i] += ly * mag * dt;
        }
    } else {
        if (S->crt_slidet[i] > 0.45f && S->crt_boost[i] < 0.38f) S->crt_boost[i] = 0.38f;   // mini-boost on release
        S->crt_slidet[i] = 0;
    }
    S->crt_v[i] = v;
    S->crt_x[i] += fcos(S->crt_h[i]) * v * dt;
    S->crt_y[i] += fsin(S->crt_h[i]) * v * dt;
}
static void crt_save_record(int who, int place, int tds){
    if (place <= 0) return;
    Buddy *bb = &G.bud[who];
    if (!bb->crt_best_pos || place < bb->crt_best_pos) bb->crt_best_pos = place;
    if (!bb->crt_best_time || tds < bb->crt_best_time) bb->crt_best_time = tds;
}
static void crt_finish_floater(int who, int place){
    Player *p = &G.p[who]; char buf[64]; char *q = buf;
    q = sappend(q, "SUPER BUDDY CART: YOU FINISHED P");
    q = sapp_int(q, place); q = sappend(q, "!"); *q = 0;
    spawn_floater(p->x, p->y - 26, buf, COL(255, 220, 120));
}
// a cup begins: points wiped, race 1 on the cup's first region
static void crt_gp_begin(int pi){
    BwSim *S = &BWS[pi];
    S->crt_race = 0;
    for (int i = 0; i < 6; i++){ S->crt_pts[i] = 0; S->crt_lastpl[i] = 0; S->crt_gprank[i] = 0; }
    S->crt_track = S->crt_cup * 4;
    crt_build(S->crt_track);
    crt_start_race(pi);
}
// the cup ends: rank the field, and bank each human's best rank
static void crt_gp_finalize(int pi){
    BwSim *S = &BWS[pi];
    int ord[6]; crt_order(S, ord);
    for (int r = 0; r < 6; r++) S->crt_gprank[ord[r]] = r + 1;
    for (int h = 0; h < 2; h++){
        int who = (h == 0)? pi : S->crt_duo;
        if (who < 0) continue;
        Buddy *hb = &G.bud[who];
        int rank = S->crt_gprank[h];
        int cur = (S->crt_cup == 0)? hb->crt_cupbest % 8 : hb->crt_cupbest / 8;
        if (!cur || rank < cur){
            if (S->crt_cup == 0) hb->crt_cupbest = (hb->crt_cupbest / 8) * 8 + rank;
            else hb->crt_cupbest = (hb->crt_cupbest % 8) + rank * 8;
        }
    }
    do_save();
}
static void crt_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    if (S->crt_guest){
        // this player is riding in the host's race: ferry the input over,
        // and let them bail out on their own (their kart races on as a rival)
        int host = S->crt_guest - 1;
        crt_gin[host] = *in;
        if (in->exit_){
            BwSim *HS = &BWS[host];
            if (HS->crt_duo == pi){ HS->crt_duo = -1; HS->crt_ready &= ~2; }
            S->crt_guest = 0;
            bw_exit(pi, 0);
            Player *p = &G.p[pi];
            spawn_floater(p->x, p->y - 26, HS->crt_state == 3? "YOU LEFT THE WAITING ROOM" : "YOU LEFT THE RACE", COL(255, 200, 150));
        }
        return;
    }
    if (S->crt_state == 3){
        // ---- the WAITING ROOM: pick a colour; in local co-op, link with
        // the other player only when they have picked the cart too, and
        // start when both are ready. Solo, FIRE starts a solo race.
        // (crt_t is the room's grace clock: the cabinet-pick press must
        // never double as the ready press, so FIRE only counts once the
        // room is a third of a second old — or re-aged at the link.)
        S->crt_t += dt;
        if (S->crt_duo < 0 && (G.net_mode == NET_OFF || (G.net_mode == NET_HOST && !any_remote_connected()))){
            int other = (pi == G.my_id)? G.p2_slot : G.my_id;
            if (other >= 0 && other != pi){
                BwSim *OS = &BWS[other]; Buddy *ob = &G.bud[other];
                if (ob->active && ob->in_bw && ob->bw_mode == 7 && OS->crt_state == 3
                    && OS->crt_duo < 0 && OS->crt_guest == 0){
                    S->crt_duo = other; OS->crt_guest = pi + 1;
                    S->crt_colsel2 = OS->crt_colsel;
                    S->crt_ready = 0; S->crt_gprevk = 0; crt_gin[pi] = (BwIn){0};
                    S->crt_t = 0;   // restart the grace clock at the link
                    sfx(19);
                }
            }
        }
        int edges = in->keys & ~S->crt_prevk; S->crt_prevk = in->keys;
        if (edges & K_LEFT){ do { S->crt_colsel = (S->crt_colsel + 5) % 6; } while (S->crt_duo >= 0 && S->crt_colsel == S->crt_colsel2); sfx(6); }
        if (edges & K_RIGHT){ do { S->crt_colsel = (S->crt_colsel + 1) % 6; } while (S->crt_duo >= 0 && S->crt_colsel == S->crt_colsel2); sfx(6); }
        // the cup is the host's call: UP for the HOME CUP (regions
        // 1-4), DOWN for the FAR CUP (regions 5-8)
        if (edges & K_UP){ S->crt_cup = 0; S->crt_track = 0; crt_build(0); sfx(6); }
        if (edges & K_DOWN){ S->crt_cup = 1; S->crt_track = 4; crt_build(4); sfx(6); }
        if (in->exit_){
            if (S->crt_duo >= 0){
                // send the partner back to their own solo waiting room
                int guest = S->crt_duo; BwSim *GS2 = &BWS[guest];
                GS2->crt_colsel = S->crt_colsel2;
                GS2->crt_guest = 0; S->crt_duo = -1;
            }
            bw_exit(pi, 0);
            return;
        }
        if (S->crt_duo >= 0){
            BwIn *g = &crt_gin[pi];
            int gedges = g->keys & ~S->crt_gprevk; S->crt_gprevk = g->keys;
            if (gedges & K_LEFT){ do { S->crt_colsel2 = (S->crt_colsel2 + 5) % 6; } while (S->crt_colsel2 == S->crt_colsel); }
            if (gedges & K_RIGHT){ do { S->crt_colsel2 = (S->crt_colsel2 + 1) % 6; } while (S->crt_colsel2 == S->crt_colsel); }
            if (g->sel && S->crt_t > 0.35f) S->crt_ready |= 2;
            if (in->sel && S->crt_t > 0.35f) S->crt_ready |= 1;
            if (S->crt_ready == 3) crt_gp_begin(pi);
        } else if (in->sel && S->crt_t > 0.35f){
            crt_gp_begin(pi);
        }
        return;
    }
    if (S->crt_state == 2){
        S->crt_fint -= dt;
        if (S->crt_fint <= 0){
            int tds = (int)(S->crt_t * 10.0f);
            int guest = S->crt_duo, gplace = S->crt_hfin[1];
            crt_save_record(pi, S->crt_hfin[0], tds);
            if (guest >= 0) crt_save_record(guest, gplace, tds);
            // bank the Grand Prix points for the whole field
            static const int PTS[6] = { 10, 8, 6, 5, 4, 3 };
            for (int i = 0; i < 6; i++){
                int pl = S->crt_finplace[i]? S->crt_finplace[i] : S->crt_place[i];
                S->crt_pts[i] += PTS[pl - 1];
                S->crt_lastpl[i] = pl;
            }
            if (S->crt_race < 3){ S->crt_state = 4; S->crt_fint = 5.0f; }
            else { S->crt_state = 5; S->crt_fint = 7.0f; crt_gp_finalize(pi); }
        }
        return;
    }
    if (in->exit_){
        if (S->crt_duo >= 0){
            int guest = S->crt_duo; BwSim *GS2 = &BWS[guest];
            GS2->crt_guest = 0; S->crt_duo = -1;
            bw_exit(guest, 0);
            Player *gp = &G.p[guest];
            spawn_floater(gp->x, gp->y - 26, "THE HOST LEFT THE RACE", COL(255, 200, 150));
        }
        bw_exit(pi, 0); return;
    }
    if (S->crt_state == 4){
        // the standings between races: breathe, read the table,
        // then roll on to the next region (FIRE hurries it along)
        S->crt_fint -= dt;
        if (S->crt_fint <= 0 || in->sel){
            S->crt_race++;
            S->crt_track = S->crt_cup * 4 + S->crt_race;
            crt_build(S->crt_track);
            crt_start_race(pi);
        }
        return;
    }
    if (S->crt_state == 5){
        // the final standings: the cup is decided — linger, then
        // everyone goes back to the mall with their rank
        S->crt_fint -= dt;
        if (S->crt_fint <= 0 || in->sel){
            int guest = S->crt_duo;
            int hrank = S->crt_gprank[0], grank = S->crt_gprank[1];
            if (guest >= 0){
                BwSim *GS2 = &BWS[guest];
                GS2->crt_guest = 0; S->crt_duo = -1;
                if (grank == 1) buddy_award_xp(guest, 40);   // v1.19: a Grand Prix win pays the buddy an extra 40
                bw_exit(guest, 0);
                Player *gp = &G.p[guest];
                char buf[64]; char *q = buf;
                q = sappend(q, "GRAND PRIX: YOU RANKED P"); q = sapp_int(q, grank); q = sappend(q, " OF 6!"); if (grank == 1 && G.bud[guest].active) q = sappend(q, " +40 BUDDY XP"); *q = 0;
                spawn_floater(gp->x, gp->y - 26, buf, COL(255, 220, 120));
            }
            if (hrank == 1) buddy_award_xp(pi, 40);   // v1.19: a Grand Prix win pays the buddy an extra 40
            bw_exit(pi, 0);
            { Player *p = &G.p[pi];
              char buf[64]; char *q = buf;
              q = sappend(q, "GRAND PRIX: YOU RANKED P"); q = sapp_int(q, hrank); q = sappend(q, " OF 6!"); if (hrank == 1 && b->active) q = sappend(q, " +40 BUDDY XP"); *q = 0;
              spawn_floater(p->x, p->y - 26, buf, COL(255, 220, 120)); }
        }
        return;
    }
    if (S->crt_state == 0){
        float prev = S->crt_count; S->crt_count -= dt;
        if ((int)prev != (int)S->crt_count && S->crt_count > 0) sfx(6);
        if (S->crt_count <= 0){ S->crt_state = 1; sfx(19); }
        return;
    }
    S->crt_t += dt;
    // ---- the human karts: host on kart 0, the co-op guest on kart 1
    crt_drive_kart(S, 0, in->keys, in->sel_held || in->sel, dt);
    if (S->crt_duo >= 0) crt_drive_kart(S, 1, crt_gin[pi].keys, crt_gin[pi].sel_held || crt_gin[pi].sel, dt);
    // ---- the rivals (one fewer when a guest is driving kart 1)
    for (int i = (S->crt_duo >= 0? 2 : 1); i < 6; i++){
        float tx, ty, hx, hy, t2x, t2y, h2x, h2y;
        // the racing line: lean toward the inside of the bend ahead,
        // smoothed so the kart flows instead of twitching
        crt_point_at(CRT_OFF + S->crt_prog[i] + 118.0f, 0, &t2x, &t2y, &h2x, &h2y);
        crt_point_at(CRT_OFF + S->crt_prog[i] + 48.0f, 0, &tx, &ty, &hx, &hy);
        { float cross = hx * h2y - hy * h2x;
          float want = S->crt_off[i] * 0.3f + cross * 20.0f;
          if (want > 14.0f) want = 14.0f; if (want < -14.0f) want = -14.0f;
          S->crt_line[i] += (want - S->crt_line[i]) * (dt * 2.2f > 1.0f? 1.0f : dt * 2.2f); }
        crt_point_at(CRT_OFF + S->crt_prog[i] + 48.0f, S->crt_line[i], &tx, &ty, &hx, &hy);
        // traffic sense: if a kart is right on your nose, shade away
        for (int j = 0; j < 6; j++){ if (j == i) continue;
            float ax = S->crt_x[j] - S->crt_x[i], ay = S->crt_y[j] - S->crt_y[i];
            float fwd = ax * fcos(S->crt_h[i]) + ay * fsin(S->crt_h[i]);
            float lat = ax * (-fsin(S->crt_h[i])) + ay * fcos(S->crt_h[i]);
            if (fwd > 2.0f && fwd < 36.0f && lat > -14.0f && lat < 14.0f){
                tx += (-fsin(S->crt_h[i])) * (lat > 0? -11.0f : 11.0f);
                ty += fcos(S->crt_h[i]) * (lat > 0? -11.0f : 11.0f);
            }
        }
        float des = fatan2(ty - S->crt_y[i], tx - S->crt_x[i]);
        float dh = crt_wrap(des - S->crt_h[i]);
        float turn = 3.3f * dt; if (dh > turn) dh = turn; if (dh < -turn) dh = -turn;
        S->crt_h[i] = crt_wrap(S->crt_h[i] + dh);
        float curve = ffabs(crt_wrap(fatan2(h2y, h2x) - fatan2(hy, hx)));
        float tv = 158.0f * S->crt_skill[i] * (1.0f - (curve > 0.5f? 0.30f : curve * 0.30f));
        float rubber = S->crt_prog[0] - S->crt_prog[i];
        tv *= (rubber > 160.0f)? 1.05f : (rubber < -160.0f)? 0.95f : 1.0f;
        { int bi = crt_nearest_full(S->crt_x[i], S->crt_y[i]);
          float dxl = S->crt_x[i] - CRT_WX[bi], dyl = S->crt_y[i] - CRT_WY[bi];
          if (dxl*dxl + dyl*dyl > 31.0f * 31.0f) tv *= 0.45f; }
        if (S->crt_boost[i] > 0){ S->crt_boost[i] -= dt; tv *= 1.30f; }
        float v = S->crt_v[i];
        if (v < tv){ v += 130.0f * dt; if (v > tv) v = tv; }
        else { v -= 210.0f * dt; if (v < tv) v = tv; }
        S->crt_v[i] = v;
        S->crt_x[i] += fcos(S->crt_h[i]) * v * dt;
        S->crt_y[i] += fsin(S->crt_h[i]) * v * dt;
    }
    // ---- boost pads
    for (int i = 0; i < 6; i++) for (int pd = 0; pd < 3; pd++){
        float dx = S->crt_x[i] - CRT_PAD[pd][0], dy = S->crt_y[i] - CRT_PAD[pd][1];
        if (dx*dx + dy*dy < 19*19 && S->crt_boost[i] <= 0){
            S->crt_boost[i] = 0.9f;
            if (i == 0 || (i == 1 && S->crt_duo >= 0)){ qnote(0, 0, 76, 80, 3); qnote(0.07f, 0, 81, 80, 3); }
        }
    }
    // ---- kart vs kart: soft bumps, nobody spins out
    for (int i = 0; i < 6; i++) for (int j = i + 1; j < 6; j++){
        float dx = S->crt_x[j] - S->crt_x[i], dy = S->crt_y[j] - S->crt_y[i];
        float d2 = dx*dx + dy*dy;
        if (d2 < 21*21 && d2 > 0.01f){ float d = fsqrt(d2);
            float push = (21.0f - d) / 2.0f, ux = dx / d, uy = dy / d;
            S->crt_x[i] -= ux * push; S->crt_y[i] -= uy * push;
            S->crt_x[j] += ux * push; S->crt_y[j] += uy * push;
            // trade a little pace instead of both karts just dying:
            // the pair settles toward a shared speed, nobody stalls
            float dv = (S->crt_v[j] - S->crt_v[i]) * 0.12f;
            S->crt_v[i] += dv; S->crt_v[j] -= dv;
            S->crt_v[i] *= 0.995f; S->crt_v[j] *= 0.995f; }
    }
    // ---- bounds, progress, finishes
    for (int i = 0; i < 6; i++){
        if (S->crt_x[i] < 18) S->crt_x[i] = 18; if (S->crt_x[i] > 622) S->crt_x[i] = 622;
        if (S->crt_y[i] < 18) S->crt_y[i] = 18; if (S->crt_y[i] > 342) S->crt_y[i] = 342;
        crt_track_prog(S, i);
        if (!S->crt_finplace[i] && S->crt_prog[i] >= 3.0f * CRT_LEN){
            S->crt_finplace[i] = ++S->crt_fincount;
            int human = (i == 0)? 0 : (i == 1 && S->crt_duo >= 0)? 1 : -1;
            if (human >= 0){
                S->crt_hfin[human] = S->crt_finplace[i];
                if (S->crt_endt < 0) S->crt_endt = 8.0f;   // first human home starts the clock
            }
        }
    }
    crt_places(S);
    // exhaust puffs from working engines: boosting karts, and the
    // humans whenever they're moving with intent
    S->crt_pufft -= dt;
    for (int q2 = 0; q2 < 20; q2++) if (S->crt_puffl[q2] > 0) S->crt_puffl[q2] -= dt;
    if (S->crt_pufft <= 0){
        S->crt_pufft = 0.07f;
        for (int i = 0; i < 6; i++){
            int human = (i == 0) || (i == 1 && S->crt_duo >= 0);
            if (S->crt_boost[i] > 0 || (human && S->crt_v[i] > 30.0f)){
                int s2 = S->crt_puffn; S->crt_puffn = (S->crt_puffn + 1) % 20;
                S->crt_puffx[s2] = S->crt_x[i] - fcos(S->crt_h[i]) * 12.0f;
                S->crt_puffy[s2] = S->crt_y[i] - fsin(S->crt_h[i]) * 12.0f;
                S->crt_puffl[s2] = 0.45f;
            }
        }
    }
    // race notes for the humans: a place gained, the final lap
    for (int h = 0; h < 2; h++){
        if (h == 1 && S->crt_duo < 0) break;
        if (S->crt_prevplace[h] && S->crt_place[h] < S->crt_prevplace[h] && !S->crt_finplace[h]){
            char buf[48]; char *q = buf;
            q = sappend(q, "UP TO P"); q = sapp_int(q, S->crt_place[h]); q = sappend(q, "!"); *q = 0;
            scpy(S->note, buf, 72); S->note_t = 1.6f;
        }
        S->crt_prevplace[h] = S->crt_place[h];
        { int lap = (int)(S->crt_prog[h] / CRT_LEN) + 1; if (lap > 3) lap = 3;
          if (S->crt_prevlap[h] && lap == 3 && S->crt_prevlap[h] < 3 && !S->crt_finplace[h]){
              scpy(S->note, "FINAL LAP!", 72); S->note_t = 1.8f;
          }
          S->crt_prevlap[h] = lap; }
    }
    // the race closes when every human is home, or the clock runs out
    if (S->crt_endt >= 0) S->crt_endt -= dt;
    { int all_home = S->crt_hfin[0] && (S->crt_duo < 0 || S->crt_hfin[1]);
      int timed_out = S->crt_endt <= 0 && S->crt_endt > -50.0f && (S->crt_hfin[0] || S->crt_hfin[1]);
      if (all_home || timed_out){
          if (!S->crt_hfin[0]) S->crt_hfin[0] = S->crt_place[0];
          if (S->crt_duo >= 0 && !S->crt_hfin[1]) S->crt_hfin[1] = S->crt_place[1];
          S->crt_player_place = S->crt_hfin[0];
          { static const int CRTXP[3] = { 40, 30, 20 };   // v1.19: a finish pays the buddy by placement — 1st/2nd/3rd per the table, any other finish 10
            int amt0 = (S->crt_hfin[0]>=1 && S->crt_hfin[0]<=3)? CRTXP[S->crt_hfin[0]-1] : 10;
            buddy_award_xp(pi, amt0);
            if (S->crt_duo >= 0){ int amt1 = (S->crt_hfin[1]>=1 && S->crt_hfin[1]<=3)? CRTXP[S->crt_hfin[1]-1] : 10; buddy_award_xp(S->crt_duo, amt1); }
            if (b->active){ char *q=S->note; q=sappend(q, "FINISH BONUS: +"); q=sapp_int(q, amt0); q=sappend(q, " BUDDY XP"); *q=0; S->note_t=3.4f; } }
          S->crt_state = 2; S->crt_fint = 3.4f;
          qnote(0, 0, 60, 90, 6); qnote(0.12f, 0, 64, 90, 6);
          qnote(0.24f, 0, 67, 90, 6); qnote(0.36f, 0, 72, 110, 10);
      } }
    b->x = CRT_OX + S->crt_x[0]; b->y = CRT_OY + S->crt_y[0];
    b->bw_x = b->x; b->bw_y = b->y; b->aim = S->crt_h[0];
    if (S->crt_duo >= 0){ Buddy *gb = &G.bud[S->crt_duo];
        gb->x = CRT_OX + S->crt_x[1]; gb->y = CRT_OY + S->crt_y[1];
        gb->bw_x = gb->x; gb->bw_y = gb->y; gb->aim = S->crt_h[1]; }
}
// the driver of a kart, drawn from behind at screen position (cx,
// seat_y) with the kart w px wide: the buddy's own back, head, the
// species giveaways — and, of course, the glasses.
static void crt_draw_driver(int cx, int seat_y, int w, int species, u32 coat){
    int hr = w * 19 / 100; if (hr < 3) hr = 3;
    u32 dark = col_shade(coat, 0.64f);
    int hx = cx, hy = seat_y - hr/2;
    if (species == 3)   // platypus: the flat tail rides behind the seat
        ellipse_fill(hx, seat_y + w*20/100, hr + hr/3, hr/2 + 1, dark);
    rect_fill(cx - w*23/100, seat_y, w*46/100, w*23/100, dark);   // shoulders and back
    if (species == 5){   // KITCHEN SINK: basin silhouette, faucet on top
        rect_fill(hx - hr, hy - hr/2, hr*2, hr + hr/2, coat);
        rect_line(hx - hr, hy - hr/2, hr*2, hr + hr/2, dark);
        rect_fill(hx + hr/4, hy - hr - hr/3, hr/5 + 1, hr/2 + hr/3, COL(178,184,190));
        rect_fill(hx - hr/3, hy - hr - hr/3, hr*2/3 + hr/4, hr/5 + 1, COL(178,184,190));
        disc_fill(hx - hr/2, hy - hr/4, hr/4 + 1, COL(178,184,190));
        disc_fill(hx + hr/2, hy - hr/4, hr/4 + 1, COL(178,184,190));
    } else if (species == 4){   // PET ROCK: a crag that drives
        rect_fill(hx - hr, hy - hr*3/4, hr*2, hr + hr*3/4, coat);
        if (w >= 15){
            line_thick(hx - hr/2, hy - hr/3, hx, hy + hr/4, 1, dark);
            line_thick(hx + hr/3, hy - hr/2, hx + hr/2, hy + hr/5, 1, dark);
        }
    } else {
        disc_fill(hx, hy, hr, dark);
        disc_fill(hx, hy, hr - (hr/6 + 1), coat);
        if (species == 0){   // PUPPY: floppy ears, mid-flop
            ellipse_fill(hx - hr - hr/5, hy + hr/6, hr/2, hr*3/4, dark);
            ellipse_fill(hx + hr + hr/5, hy + hr/6, hr/2, hr*3/4, dark);
        } else if (species == 1){   // KITTY: pointy ears, don't care
            tri_fill(hx - hr, hy - hr/3, hx - hr/3, hy - hr/3, hx - hr*3/4, hy - hr - hr/4, coat);
            tri_fill(hx + hr/3, hy - hr/3, hx + hr, hy - hr/3, hx + hr*3/4, hy - hr - hr/4, coat);
        } else if (species == 2){   // MOOSE: the antlers are the tell
            u32 ant = COL(214,186,138);
            int lw = w >= 30? 3 : 1;
            line_thick(hx - hr*3/4, hy - hr*3/4, hx - hr - hr/2, hy - hr - hr/3, lw, ant);
            line_thick(hx - hr - hr/3, hy - hr - hr/5, hx - hr - hr/2, hy - hr*3/4, lw > 1? 2 : 1, ant);
            line_thick(hx + hr*3/4, hy - hr*3/4, hx + hr + hr/2, hy - hr - hr/3, lw, ant);
            line_thick(hx + hr + hr/3, hy - hr - hr/5, hx + hr + hr/2, hy - hr*3/4, lw > 1? 2 : 1, ant);
        }
    }
    if (w >= 15){   // the famous glasses, oversized as is traditional
        int gr = hr * 2 / 5; if (gr < 2) gr = 2;
        circle_out(hx - hr/2, hy, gr, COL(20,18,26));
        circle_out(hx + hr/2, hy, gr, COL(20,18,26));
        line_thick(hx - hr/2 + gr, hy, hx + hr/2 - gr, hy, 1, COL(20,18,26));
    }
}
static void crt_render_view(int sim_pi, int viewer_pi){
    BwSim *S = &BWS[sim_pi]; Buddy *b = &G.bud[viewer_pi];
    int vk = (viewer_pi == sim_pi)? 0 : 1;   // the kart this viewer is driving
    int vx = G.view_x0, vy = G.view_y0, vw2 = G.view_w>0? G.view_w : VW, vh2 = G.view_h>0? G.view_h : VH;
    float cs = (float)vw2 / 640.0f; { float csy = (float)vh2 / 360.0f; if (csy < cs) cs = csy; }
    int ox = vx + (int)((vw2 - 640*cs) / 2), oy = vy + (int)((vh2 - 360*cs) / 2);
#define KTX(lx) (ox + (int)((lx) * cs))
#define KTY(ly) (oy + (int)((ly) * cs))
#define KTS(v) ((int)((v) * cs) < 1 ? 1 : (int)((v) * cs))
    if (S->crt_state == 3){
        // ---- the WAITING ROOM
        static const u32 KCOL2[6] = { COL(230, 60, 50), COL(70, 130, 240), COL(250, 200, 50),
                                      COL(90, 200, 90), COL(200, 90, 220), COL(240, 140, 40) };
        static const char *COLNAME[6] = { "RED", "BLUE", "YELLOW", "GREEN", "PURPLE", "ORANGE" };
        int mycol = (vk == 0)? S->crt_colsel : S->crt_colsel2;
        rect_fill(vx, vy, vw2, vh2, COL(18, 42, 24));
        for (int ty2 = 0; ty2 < 12; ty2++) for (int tx2 = 0; tx2 < 20; tx2++)
            if ((tx2 + ty2) & 1) rect_fill(KTX(tx2*32), KTY(ty2*32), KTS(32) + 1, KTS(32) + 1, COL(22, 52, 28));
        draw_text_center(vx + vw2/2, vy + 26, "SUPER BUDDY CART", vw2 >= 620? 3 : 2, COL(250, 200, 50));
        draw_text_center(vx + vw2/2, vy + 62, "THE WAITING ROOM", 2, COL(255, 255, 255));
        { char buf[80]; char *q = buf;
          q = sappend(q, "GRAND PRIX - "); q = sappend(q, S->crt_cup? "FAR CUP: CACHE TO THE DEPTHS" : "HOME CUP: MEADOWS TO BADLANDS"); *q = 0;
          draw_text_center(vx + vw2/2, vy + 84, buf, 1, COL(250, 200, 50)); }
        // your kart, big and proud, wearing your pick
        { int cx = vx + vw2/2, cy = vy + vh2/2 - 6;
          disc_fill(cx - KTS(48), cy - KTS(35), KTS(17), COL(16, 14, 24));
          disc_fill(cx + KTS(48), cy - KTS(35), KTS(17), COL(16, 14, 24));
          disc_fill(cx - KTS(48), cy + KTS(35), KTS(17), COL(16, 14, 24));
          disc_fill(cx + KTS(48), cy + KTS(35), KTS(17), COL(16, 14, 24));
          rect_fill(cx - KTS(61), cy - KTS(38), KTS(122), KTS(77), KCOL2[mycol]);
          rect_line(cx - KTS(61), cy - KTS(38), KTS(122), KTS(77), COL(255, 255, 255));
          crt_draw_driver(cx, cy - KTS(28), KTS(118), b->species, crt_coat_for(b->species, b->color, viewer_pi));
          tri_fill(cx - KTS(92), cy - KTS(16), cx - KTS(92), cy + KTS(16), cx - KTS(70), cy, COL(255, 255, 255));
          tri_fill(cx + KTS(92), cy - KTS(16), cx + KTS(92), cy + KTS(16), cx + KTS(70), cy, COL(255, 255, 255));
          draw_text_center(cx, cy + KTS(64), COLNAME[mycol], 2, COL(255, 255, 255)); }
        draw_text_center(vx + vw2/2, vy + 98, pad_mode()? "IN THE RACE: HOLD A TO GO, STEER WITH THE STICK" : "IN THE RACE: HOLD FIRE TO GO, STEER WITH THE ARROWS", 1, COL(170, 200, 190));
        draw_text_center(vx + vw2/2, vy + vh2 - 104, vk == 0? "LEFT / RIGHT - COLOUR      UP / DOWN - CUP" : "LEFT / RIGHT - COLOUR      THE HOST PICKS THE CUP", 1, COL(200, 230, 200));
        if (S->crt_duo >= 0){
            int imready = (vk == 0)? (S->crt_ready & 1) : (S->crt_ready & 2);
            int theyready = (vk == 0)? (S->crt_ready & 2) : (S->crt_ready & 1);
            { char buf[64]; char *q = buf;
              q = sappend(q, "PARTNER IS HERE, IN THE "); q = sappend(q, COLNAME[(vk == 0)? S->crt_colsel2 : S->crt_colsel]); q = sappend(q, " KART"); *q = 0;
              draw_text_center(vx + vw2/2, vy + vh2 - 82, buf, 1, COL(255, 240, 200)); }
            draw_text_center(vx + vw2/2, vy + vh2 - 62, theyready? "PARTNER: READY!" : "PARTNER: STILL PICKING...", 1, theyready? COL(120, 255, 120) : COL(220, 220, 220));
            draw_text_center(vx + vw2/2, vy + vh2 - 42, imready? "YOU: READY!" : "FIRE WHEN READY", 1, imready? COL(120, 255, 120) : COL(255, 255, 255));
        } else {
            draw_text_center(vx + vw2/2, vy + vh2 - 82, "NO PARTNER YET", 1, COL(255, 240, 200));
            draw_text_center(vx + vw2/2, vy + vh2 - 62, "THE OTHER PLAYER MUST PICK THE CART TOO", 1, COL(220, 220, 220));
            draw_text_center(vx + vw2/2, vy + vh2 - 42, "FIRE: START THE GRAND PRIX   -   LEAVE: BACK OUT", 1, COL(255, 255, 255));
        }
        if (S->note_t > 0){
            rect_fill(vx, vy + vh2 - 22, vw2, 22, COL(10, 12, 10));
            draw_text_center(vx + vw2/2, vy + vh2 - 15, S->note, 1, COL(220, 255, 220));
        }
        return;
    }
    if (S->crt_state == 4 || S->crt_state == 5){
        // ---- the STANDINGS between (and after) the cup's races
        int fin = (S->crt_state == 5);
        rect_fill(vx, vy, vw2, vh2, col_shade(REGION_GB[S->crt_track], 0.35f));
        for (int ty2 = 0; ty2 < 12; ty2++) for (int tx2 = 0; tx2 < 20; tx2++)
            if ((tx2 + ty2) & 1) rect_fill(KTX(tx2*32), KTY(ty2*32), KTS(32) + 1, KTS(32) + 1, col_shade(REGION_GB[S->crt_track], 0.45f));
        draw_text_center(vx + vw2/2, vy + 24, fin? "FINAL STANDINGS" : "STANDINGS", 3, COL(250, 200, 50));
        { char buf[80]; char *q = buf;
          q = sappend(q, S->crt_cup? "FAR CUP" : "HOME CUP");
          if (!fin){ q = sappend(q, " - AFTER RACE "); q = sapp_int(q, S->crt_race + 1); q = sappend(q, " OF 4"); }
          *q = 0; draw_text_center(vx + vw2/2, vy + 62, buf, 1, COL(255, 255, 255)); }
        int ord[6]; crt_order(S, ord);
        for (int r = 0; r < 6; r++){
            int k = ord[r], ry = vy + 92 + r * 30;
            if (k == vk) rect_fill(vx + vw2/2 - 170, ry - 5, 340, 24, COL(52, 46, 22));
            char buf[80]; char *q = buf;
            q = sappend(q, "P"); q = sapp_int(q, r + 1); q = sappend(q, "  ");
            q = sappend(q, BUD_ELF[S->crt_nameidx[k] % BUD_NAMES_N]);
            if (k == vk) q = sappend(q, "  (YOU)");
            else if (S->crt_duo >= 0 && k == 1 - vk) q = sappend(q, "  (PARTNER)");
            q = sappend(q, "  -  "); q = sapp_int(q, S->crt_pts[k]); q = sappend(q, " PTS"); *q = 0;
            draw_text_center(vx + vw2/2, ry, buf, 2, (k == vk)? COL(255, 240, 160) : COL(225, 230, 235));
        }
        if (fin){
            Buddy *vb = &G.bud[viewer_pi];
            char buf[80]; char *q = buf;
            q = sappend(q, "YOU RANKED P"); q = sapp_int(q, S->crt_gprank[vk]); q = sappend(q, " OF 6");
            int best = (S->crt_cup == 0)? vb->crt_cupbest % 8 : vb->crt_cupbest / 8;
            if (best){ q = sappend(q, "   -   CUP BEST P"); q = sapp_int(q, best); }
            *q = 0;
            draw_text_center(vx + vw2/2, vy + 92 + 6 * 30 + 12, buf, 1, COL(255, 240, 200));
            if (S->crt_gprank[vk] == 1) draw_text_center(vx + vw2/2, vy + 92 + 6 * 30 + 32, "CHAMPION OF THE CUP!", 1, COL(250, 200, 50));
        } else {
            char buf[80]; char *q = buf;
            q = sappend(q, "NEXT: "); q = sappend(q, REGION_NAME[S->crt_cup * 4 + S->crt_race + 1]); *q = 0;
            draw_text_center(vx + vw2/2, vy + 92 + 6 * 30 + 12, buf, 1, COL(200, 230, 200));
        }
        if (S->note_t > 0){
            rect_fill(vx, vy + vh2 - 22, vw2, 22, COL(10, 12, 10));
            draw_text_center(vx + vw2/2, vy + vh2 - 15, S->note, 1, COL(220, 255, 220));
        }
        return;
    }
    // ---- the race, SNES-style: the world stays flat, the camera sits
    // behind your kart, and the ground plane is projected forward —
    // grass bands, rumble strips and all — from your seat.
    static const u32 KCOL[6] = { COL(230, 60, 50), COL(70, 130, 240), COL(250, 200, 50),
                                 COL(90, 200, 90), COL(200, 90, 220), COL(240, 140, 40) };
    { float kx = S->crt_x[vk], ky = S->crt_y[vk], kh = S->crt_h[vk];
      float chx = fcos(kh), chy = fsin(kh);
      float camx = kx - chx * 40.0f, camy = ky - chy * 40.0f;
      float F = (float)vw2 * 0.82f;
      int horizon = vy + (int)(vh2 * 0.40f);
      // project a world point: returns 0 when it is behind the camera
      // (the lambda-free C89 way: a tiny macro pair over locals)
#define CRT_PROJ(wx, wy, outf, outsx, outsy) do { \
        float dx_ = (wx) - camx, dy_ = (wy) - camy; \
        float f_ = dx_ * chx + dy_ * chy; \
        float l_ = dx_ * (-chy) + dy_ * chx; \
        (outf) = f_; \
        (outsx) = vx + vw2/2 + (int)(l_ * F / (f_ < 1.0f? 1.0f : f_)); \
        (outsy) = horizon + (int)(15.0f * F / (f_ < 1.0f? 1.0f : f_)); \
      } while (0)
      // sky: a four-band gradient, a haloed sun, drifting clouds,
      // a far hill line, and the tree line in front of it — all of
      // it sliding as you steer, all of it wearing the REGION
      int rg = S->crt_track;
      u32 skyT = CRT_SKY[rg][0], skyH = CRT_SKY[rg][1];
      u32 gndA = col_shade(REGION_GA[rg], 0.55f), gndB = col_shade(REGION_GB[rg], 0.55f);
      u32 aprA = col_shade(REGION_GA[rg], 0.74f), aprB = col_shade(REGION_GB[rg], 0.74f);
      int dark = (rg == 5 || rg == 7);
      { int sh = horizon - vy; if (sh < 4) sh = 4;
        rect_fill(vx, vy, vw2, sh/4 + 1, col_shade(skyT, 0.74f));
        rect_fill(vx, vy + sh/4, vw2, sh/4 + 1, skyT);
        rect_fill(vx, vy + sh/2, vw2, sh/4 + 1, col_shade(skyH, 0.93f));
        rect_fill(vx, vy + 3*sh/4, vw2, sh - 3*sh/4 + 1, skyH); }
      disc_fill(vx + vw2 - 90, vy + 44, 31, dark? COL(200, 204, 225) : COL(255, 246, 196));
      disc_fill(vx + vw2 - 90, vy + 44, 20, dark? COL(226, 230, 242) : COL(255, 236, 150));
      { int slide = (int)(kh * 260.0f);
        u32 cloudc = dark? COL(126, 122, 158) : COL(248, 251, 255);
        for (int ci = 0; ci < 4; ci++){
            int cw = 54 + (ci * 29) % 40;
            int cxx = (ci * 317 + 60 - slide * 2 - (int)(S->crt_t * 6.0f)) % (vw2 + 240);
            if (cxx < 0) cxx += vw2 + 240;
            int cxp2 = vx + cxx - 120, cyp2 = vy + 14 + (ci * 23) % 30;
            ellipse_fill(cxp2, cyp2, cw/2, cw/6, cloudc);
            ellipse_fill(cxp2 + cw/5, cyp2 - cw/10, cw/3, cw/7, col_shade(cloudc, 1.03f));
            ellipse_fill(cxp2 - cw/4, cyp2 + 2, cw/3, cw/8, col_shade(cloudc, 0.94f));
        }
        for (int hi2 = -1; hi2 < vw2 / 130 + 2; hi2++){
            int hxp = vx + hi2 * 130 - ((slide % 130) + 130) % 130;
            int hh = 20 + (int)(hash2(hi2 + slide / 130, 7, 0x411u) % 22);
            tri_fill(hxp, horizon, hxp + 65, horizon - hh, hxp + 130, horizon, col_shade(REGION_GA[rg], 0.8f));
        }
        for (int ti = -1; ti < vw2 / 74 + 2; ti++){
            int txp = vx + ti * 74 - ((slide % 74) + 74) % 74;
            int th = 16 + ((ti * 37 + (slide / 74) * 13) & 15);
            tri_fill(txp, horizon, txp + 22, horizon - th - 10, txp + 44, horizon, col_shade(REGION_GA[rg], 0.6f));
            rect_fill(txp + 19, horizon - 6, 6, 6, COL(90, 64, 40));
        } }
      rect_fill(vx, horizon, vw2, vy + vh2 - horizon, gndB);
      // the road ahead: sample the centreline from your segment forward
      { int startseg = crt_nearest_full(kx, ky);
        enum { NS = 46 };
        int rsx[NS + 1], rsy[NS + 1], lsx[NS + 1], lsy[NS + 1], rrx[NS + 1], rry[NS + 1], lrx[NS + 1], lry[NS + 1];
        int arx[NS + 1], ary[NS + 1], alx[NS + 1], aly[NS + 1], erx[NS + 1], ery[NS + 1], elx[NS + 1], ely[NS + 1];
        float rf[NS + 1];
        for (int j = 0; j <= NS; j++){
            int si = (startseg + j) % CRT_N, sj = (si + 1) % CRT_N;
            float dxn = CRT_WX[sj] - CRT_WX[si], dyn = CRT_WY[sj] - CRT_WY[si];
            float dl = fsqrt(dxn*dxn + dyn*dyn); if (dl < 0.001f) dl = 1.0f;
            float nx = -dyn / dl, ny = dxn / dl;
            float f1, f2;
            CRT_PROJ(CRT_WX[si] + nx * 30.0f, CRT_WY[si] + ny * 30.0f, f1, rsx[j], rsy[j]);
            CRT_PROJ(CRT_WX[si] - nx * 30.0f, CRT_WY[si] - ny * 30.0f, f2, lsx[j], lsy[j]);
            rf[j] = (f1 + f2) * 0.5f;
            CRT_PROJ(CRT_WX[si] + nx * 37.0f, CRT_WY[si] + ny * 37.0f, f1, rrx[j], rry[j]);
            CRT_PROJ(CRT_WX[si] - nx * 37.0f, CRT_WY[si] - ny * 37.0f, f2, lrx[j], lry[j]);
            CRT_PROJ(CRT_WX[si] + nx * 45.0f, CRT_WY[si] + ny * 45.0f, f1, arx[j], ary[j]);
            CRT_PROJ(CRT_WX[si] - nx * 45.0f, CRT_WY[si] - ny * 45.0f, f2, alx[j], aly[j]);
            CRT_PROJ(CRT_WX[si] + nx * 32.5f, CRT_WY[si] + ny * 32.5f, f1, erx[j], ery[j]);
            CRT_PROJ(CRT_WX[si] - nx * 32.5f, CRT_WY[si] - ny * 32.5f, f2, elx[j], ely[j]);
        }
        for (int j = NS - 1; j >= 0; j--){
            if (rf[j + 1] < 4.0f) continue;
            int par = (startseg + j) & 1;
            if (rsy[j] > horizon) rect_fill(vx, rsy[j], vw2, rsy[j + 1] - rsy[j] + 1, par? gndA : gndB);
            if (rf[j] < 4.0f) continue;
            // the manicured apron the track sits in
            u32 ap = par? aprA : aprB;
            tri_fill(arx[j], ary[j], alx[j], aly[j], alx[j + 1], aly[j + 1], ap);
            tri_fill(arx[j], ary[j], alx[j + 1], aly[j + 1], arx[j + 1], ary[j + 1], ap);
            u32 rum = par? COL(226, 226, 226) : COL(205, 50, 45);
            tri_fill(rrx[j], rry[j], lrx[j], lry[j], lrx[j + 1], lry[j + 1], rum);
            tri_fill(rrx[j], rry[j], lrx[j + 1], lry[j + 1], rrx[j + 1], rry[j + 1], rum);
            u32 rd = par? COL(62, 62, 70) : COL(56, 56, 64);
            tri_fill(rsx[j], rsy[j], lsx[j], lsy[j], lsx[j + 1], lsy[j + 1], rd);
            tri_fill(rsx[j], rsy[j], lsx[j + 1], lsy[j + 1], rsx[j + 1], rsy[j + 1], rd);
            // crisp white edge lines just inside the rumble
            tri_fill(rsx[j], rsy[j], erx[j], ery[j], erx[j + 1], ery[j + 1], COL(232, 236, 238));
            tri_fill(rsx[j], rsy[j], erx[j + 1], ery[j + 1], rsx[j + 1], rsy[j + 1], COL(232, 236, 238));
            tri_fill(elx[j], ely[j], lsx[j], lsy[j], lsx[j + 1], lsy[j + 1], COL(232, 236, 238));
            tri_fill(elx[j], ely[j], lsx[j + 1], lsy[j + 1], elx[j + 1], ely[j + 1], COL(232, 236, 238));
            if (((startseg + j) % 3) == 0 && rf[j] < 340.0f){
                // the centre dashes: painted speed, rushing at you
                float cxj = (rsx[j] + lsx[j]) * 0.5f, cyj = (rsy[j] + lsy[j]) * 0.5f;
                float cxn = (rsx[j+1] + lsx[j+1]) * 0.5f, cyn = (rsy[j+1] + lsy[j+1]) * 0.5f;
                float ex = (float)(rsx[j] - lsx[j]), ey = (float)(rsy[j] - lsy[j]);
                float el = fsqrt(ex*ex + ey*ey); if (el < 1.0f) el = 1.0f;
                float exn = (float)(rsx[j+1] - lsx[j+1]), eyn = (float)(rsy[j+1] - lsy[j+1]);
                float eln = fsqrt(exn*exn + eyn*eyn); if (eln < 1.0f) eln = 1.0f;
                float hw = el * 0.030f; if (hw < 1.0f) hw = 1.0f;
                float hwn = eln * 0.030f; if (hwn < 1.0f) hwn = 1.0f;
                float ux = ex / el, uy = ey / el, uxn = exn / eln, uyn = eyn / eln;
                tri_fill((int)(cxj - ux*hw), (int)(cyj - uy*hw), (int)(cxj + ux*hw), (int)(cyj + uy*hw), (int)(cxn + uxn*hwn), (int)(cyn + uyn*hwn), COL(228, 228, 210));
                tri_fill((int)(cxj - ux*hw), (int)(cyj - uy*hw), (int)(cxn + uxn*hwn), (int)(cyn + uyn*hwn), (int)(cxn - uxn*hwn), (int)(cyn - uyn*hwn), COL(228, 228, 210));
            }
        }
        // the checkered start line, when it is in view
        { float fx, fy, fhx, fhy; crt_point_at(CRT_OFF, 0, &fx, &fy, &fhx, &fhy);
          float ff; int fsx, fsy; CRT_PROJ(fx, fy, ff, fsx, fsy);
          float fdx = fx - camx, fdy = fy - camy;
          float fl = fdx * (-chy) + fdy * chx;
          if (ff > 8.0f && ff < 620.0f && fl < ff * 1.1f + 50.0f && fl > -(ff * 1.1f + 50.0f)){
              float nx = -fhy, ny = fhx;
              for (int c2 = 0; c2 < 8; c2++){
                  float la = -28.0f + c2 * 7.0f, lb = la + 7.0f;
                  int ax, ay, bx, by, cx2, cy2, dx2, dy2; float tf;
                  CRT_PROJ(fx + nx * la, fy + ny * la, tf, ax, ay);
                  CRT_PROJ(fx + nx * lb, fy + ny * lb, tf, bx, by);
                  CRT_PROJ(fx + fhx * 8.0f + nx * lb, fy + fhy * 8.0f + ny * lb, tf, cx2, cy2);
                  CRT_PROJ(fx + fhx * 8.0f + nx * la, fy + fhy * 8.0f + ny * la, tf, dx2, dy2);
                  u32 cc = (c2 & 1)? COL(16, 14, 24) : COL(245, 245, 245);
                  tri_fill(ax, ay, bx, by, cx2, cy2, cc);
                  tri_fill(ax, ay, cx2, cy2, dx2, dy2, cc);
              }
              // the start gantry: two poles and the banner over the line
              { int gx[2], gy[2], gt[2]; float gf[2]; int ok = 1;
                for (int p2 = 0; p2 < 2; p2++){
                    float la = p2? 44.0f : -44.0f;
                    CRT_PROJ(fx + nx * la, fy + ny * la, gf[p2], gx[p2], gy[p2]);
                    if (gf[p2] < 6.0f) ok = 0;
                    gt[p2] = gy[p2] - (int)(30.0f * F / gf[p2]);
                }
                if (ok){
                    for (int p2 = 0; p2 < 2; p2++) rect_fill(gx[p2] - 2, gt[p2], 4, gy[p2] - gt[p2], COL(52, 54, 64));
                    for (int p2 = 0; p2 < 2; p2++){   // little flags, waving on the poles
                        int fw = (int)(7.0f * F / gf[p2]); if (fw < 3) fw = 3;
                        int dir = p2? -1 : 1;
                        int wob = (int)(fsin(S->crt_t * 7.0f + p2 * 2.1f) * fw * 0.18f);
                        tri_fill(gx[p2], gt[p2], gx[p2] + dir * fw, gt[p2] + fw/4 + wob, gx[p2], gt[p2] + fw/2, COL(250, 200, 50));
                    }
                    int bh = (int)(9.0f * F / ff); if (bh < 3) bh = 3;
                    tri_fill(gx[0], gt[0], gx[1], gt[1], gx[1], gt[1] + bh, COL(198, 52, 48));
                    tri_fill(gx[0], gt[0], gx[1], gt[1] + bh, gx[0], gt[0] + bh, COL(198, 52, 48));
                    if (gx[1] - gx[0] > 210) draw_text_center((gx[0] + gx[1]) / 2, (gt[0] + gt[1]) / 2 + bh/2 - 3, REGION_NAME[rg], 1, COL(255, 244, 220));
                } }
          } }
        // boost pads lying on the road
        for (int pd = 0; pd < 3; pd++){
            float pcx = CRT_PAD[pd][0], pcy = CRT_PAD[pd][1];
            float ff; int psx, psy; CRT_PROJ(pcx, pcy, ff, psx, psy);
            { float pdx = pcx - camx, pdy = pcy - camy;
              float pl = pdx * (-chy) + pdy * chx;
              if (pl > ff * 1.1f + 50.0f || pl < -(ff * 1.1f + 50.0f)) continue; }
            if (ff < 12.0f || ff > 640.0f) continue;
            int si = crt_nearest_full(pcx, pcy), sj = (si + 1) % CRT_N;
            float dxn = CRT_WX[sj] - CRT_WX[si], dyn = CRT_WY[sj] - CRT_WY[si];
            float dl = fsqrt(dxn*dxn + dyn*dyn); if (dl < 0.001f) dl = 1.0f;
            float ux = dxn / dl, uy = dyn / dl, nx = -uy, ny = ux;
            int ax, ay, bx, by, cx2, cy2, dx2, dy2; float tf;
            int gx2, gy2, hx2, hy2, ix2, iy2, jx2, jy2;
            CRT_PROJ(pcx + nx * 22.0f - ux * 12.0f, pcy + ny * 22.0f - uy * 12.0f, tf, gx2, gy2);
            CRT_PROJ(pcx - nx * 22.0f - ux * 12.0f, pcy - ny * 22.0f - uy * 12.0f, tf, hx2, hy2);
            CRT_PROJ(pcx - nx * 22.0f + ux * 12.0f, pcy - ny * 22.0f + uy * 12.0f, tf, ix2, iy2);
            CRT_PROJ(pcx + nx * 22.0f + ux * 12.0f, pcy + ny * 22.0f + uy * 12.0f, tf, jx2, jy2);
            tri_fill(gx2, gy2, hx2, hy2, ix2, iy2, COL(255, 216, 118));
            tri_fill(gx2, gy2, ix2, iy2, jx2, jy2, COL(255, 216, 118));
            CRT_PROJ(pcx + nx * 19.0f - ux * 9.0f, pcy + ny * 19.0f - uy * 9.0f, tf, ax, ay);
            CRT_PROJ(pcx - nx * 19.0f - ux * 9.0f, pcy - ny * 19.0f - uy * 9.0f, tf, bx, by);
            CRT_PROJ(pcx - nx * 19.0f + ux * 9.0f, pcy - ny * 19.0f + uy * 9.0f, tf, cx2, cy2);
            CRT_PROJ(pcx + nx * 19.0f + ux * 9.0f, pcy + ny * 19.0f + uy * 9.0f, tf, dx2, dy2);
            { u32 padc = ((int)(S->crt_t * 6.0f + pd * 1.7f) & 1)? COL(240, 130, 40) : COL(252, 176, 64);
              tri_fill(ax, ay, bx, by, cx2, cy2, padc);
              tri_fill(ax, ay, cx2, cy2, dx2, dy2, padc); }
            if (ff < 150.0f) draw_text_center(psx, psy - 4, ">>", 1, COL(255, 255, 255));
        }
      }
    // everything with depth — trackside scenery and rival karts —
    // in one far-to-near pass, so trees occlude and are occluded
    // exactly like the karts they stand beside
    { enum { MAXIT = 6 + CRT_SCN_N + 20 };
      float itf[MAXIT]; int itk[MAXIT], itx[MAXIT], itsx[MAXIT], itsy[MAXIT], itn = 0;
      for (int sn = 0; sn < CRT_SCN_N; sn++){
          if (CRT_SCX[sn] < -50.0f) continue;
          float f; int sx, sy; CRT_PROJ(CRT_SCX[sn], CRT_SCY[sn], f, sx, sy);
          if (f < 9.0f || f > 880.0f) continue;
          itf[itn] = f; itk[itn] = 1; itx[itn] = sn; itsx[itn] = sx; itsy[itn] = sy; itn++;
      }
      for (int q2 = 0; q2 < 20; q2++){
          if (S->crt_puffl[q2] <= 0) continue;
          float f; int sx, sy; CRT_PROJ(S->crt_puffx[q2], S->crt_puffy[q2], f, sx, sy);
          if (f < 6.0f || f > 700.0f) continue;
          itf[itn] = f; itk[itn] = 3; itx[itn] = q2; itsx[itn] = sx; itsy[itn] = sy; itn++;
      }
      for (int i = 0; i < 6; i++){ if (i == vk) continue;
          float f; int sx, sy; CRT_PROJ(S->crt_x[i], S->crt_y[i], f, sx, sy);
          if (f < 26.0f || f > 760.0f) continue;
          itf[itn] = f; itk[itn] = 0; itx[itn] = i; itsx[itn] = sx; itsy[itn] = sy; itn++;
      }
      for (int a = 1; a < itn; a++){ float tf = itf[a]; int tk = itk[a], tx2 = itx[a], tsx = itsx[a], tsy = itsy[a]; int b2 = a - 1;
          while (b2 >= 0 && itf[b2] < tf){ itf[b2+1] = itf[b2]; itk[b2+1] = itk[b2]; itx[b2+1] = itx[b2]; itsx[b2+1] = itsx[b2]; itsy[b2+1] = itsy[b2]; b2--; }
          itf[b2+1] = tf; itk[b2+1] = tk; itx[b2+1] = tx2; itsx[b2+1] = tsx; itsy[b2+1] = tsy; }
      for (int oi = 0; oi < itn; oi++){
        float f = itf[oi]; int sx = itsx[oi], base = itsy[oi];
        if (itk[oi] == 1){
            int sn = itx[oi], tp = CRT_SCT[sn];
            if (tp == 0){   // a tree (every seventh wears the region's tuft colour)
                int th = (int)(30.0f * F / f); if (th < 3) continue;
                if (sx < vx - th || sx > vx + vw2 + th) continue;
                ellipse_fill(sx, base, th/4, th/10, col_shade(REGION_GB[rg], 0.45f));
                rect_fill(sx - th/16 - 1, base - th*38/100, th/8 + 2, th*38/100, COL(96, 66, 40));
                u32 g1 = (sn % 7 == 3)? REGION_TUFT[rg] : (sn & 1)? col_shade(REGION_GA[rg], 0.62f) : col_shade(REGION_GA[rg], 0.5f);
                disc_fill(sx, base - th*52/100, th*30/100, g1);
                disc_fill(sx, base - th*80/100, th*22/100, col_shade(g1, 1.18f));
            } else if (tp == 1){   // a tire stack
                int tw = (int)(13.0f * F / f); if (tw < 3) continue;
                if (sx < vx - tw || sx > vx + vw2 + tw) continue;
                int thh = tw/3; if (thh < 1) thh = 1;
                rect_fill(sx - tw/2, base - thh, tw, thh, COL(38, 38, 44));
                rect_fill(sx - tw/2, base - thh*2, tw, thh, COL(226, 226, 226));
                rect_fill(sx - tw/2, base - thh*3, tw, thh, (sn & 1)? COL(205, 50, 45) : COL(38, 38, 44));
            } else {   // the grandstand, watching the top straight
                // Nov 15 (Daniel's "weird gray wall"): unclamped,
                // the stand's projection at grid distance filled
                // half the screen with flat concrete. Bound it to a
                // building's size, and don't draw it at all when the
                // kart is right alongside it.
                if (f < 42.0f) continue;
                int gw = (int)(175.0f * F / f); if (gw < 12) continue;
                if (gw > 380) gw = 380;
                int gh = (int)(42.0f * F / f); if (gh > 92) gh = 92;
                if (sx < vx - gw/2 || sx > vx + vw2 + gw/2) continue;
                rect_fill(sx - gw/2, base - gh, gw, gh, COL(116, 126, 148));
                for (int r2 = 0; r2 < 4; r2++) for (int c3 = 0; c3 < gw/9; c3++){
                    u32 h = hash2(sn * 31 + c3, r2 * 7 + 1, 0x57A0u);
                    static const u32 CROWD[5] = { COL(240,200,90), COL(230,90,80), COL(120,180,240), COL(240,240,240), COL(120,220,140) };
                    rect_fill(sx - gw/2 + 3 + c3 * 9, base - gh + 5 + r2 * (gh/5), 4, 4, CROWD[h % 5]);
                }
                rect_fill(sx - gw/2 - 4, base - gh - gh/6 - 2, gw + 8, gh/6 + 2, COL(64, 70, 88));
                for (int aw = 0; aw < (gw + 8) / 10; aw++)
                    if (aw & 1) rect_fill(sx - gw/2 - 4 + aw * 10, base - gh - gh/6 - 2, 10, gh/6 + 2, COL(198, 52, 48));
                if (gw > 300) draw_text_center(sx, base - gh - gh/6 + gh/12 - 3, "GRAND PRIX", 1, COL(255, 240, 200));
            }
            continue;
        }
        if (itk[oi] == 3){   // an exhaust puff, swelling as it fades
            int q2 = itx[oi];
            float lf = S->crt_puffl[q2]; if (lf < 0) lf = 0;
            int pr = (int)((0.45f - lf) * 30.0f * F / f) + 2;
            if (pr > 26) pr = 26;
            disc_fill(sx, base - pr/2, pr, COL(216, 220, 224));
            disc_fill(sx - pr/3, base - pr, pr/2, COL(240, 243, 245));
            continue;
        }
        int i = itx[oi];
        int kw = (int)(17.0f * F / f); if (kw < 3) kw = 3;
        if (kw > vw2 * 2 / 5) kw = vw2 * 2 / 5;
        int kh2 = (int)(kw * 0.66f); if (kh2 < 2) kh2 = 2;
        if (sx < vx - kw || sx > vx + vw2 + kw) continue;
        ellipse_fill(sx, base, kw/2 + 1, kh2/4 + 1, COL(20, 40, 22));
        if (S->crt_boost[i] > 0)
            tri_fill(sx - kw/4, base, sx + kw/4, base, sx, base + kh2/2 + 2, COL(255, 160, 60));
        rect_fill(sx - kw/2, base - kh2/2, kw/4 + 1, kh2/2, COL(16, 14, 24));
        rect_fill(sx + kw/2 - kw/4, base - kh2/2, kw/4 + 1, kh2/2, COL(16, 14, 24));
        rect_fill(sx - kw/2, base - kh2, kw, kh2 * 3/4, KCOL[S->crt_kcol[i]]);
        rect_line(sx - kw/2, base - kh2, kw, kh2 * 3/4, COL(16, 14, 24));
        if (kw >= 8){
            rect_fill(sx - kw/2 + 2, base - kh2 + 1, kw - 4, kh2/5, col_shade(KCOL[S->crt_kcol[i]], 1.30f));
            rect_fill(sx - kw/2 + kw/5, base - kh2/4, kw * 3/5, kh2/4, col_shade(KCOL[S->crt_kcol[i]], 0.60f));
        }
        crt_draw_driver(sx, base - kh2 + kh2/8 + ((S->crt_v[i] > 60.0f)? (int)(fsin(S->crt_t * 22.0f + i * 1.93f) * kw * 0.012f) : 0), kw, S->crt_species[i], crt_coat_for(S->crt_species[i], S->crt_dcolidx[i], i));
        if (f < 300.0f){
            int ty2 = base - kh2 - kw*42/100 - 10;
            if (S->crt_duo >= 0 && i == 1 - vk)
                tri_fill(sx - 5, ty2 - 2, sx + 5, ty2 - 2, sx, ty2 + 5, COL(255, 255, 255));
            draw_text_center(sx, ty2, BUD_ELF[S->crt_nameidx[i] % BUD_NAMES_N], 1, COL(255, 255, 255));
        }
      } }
    // your own kart, big at the bottom of the screen, from behind
    { int ow = vw2 / 5; if (ow < 110) ow = 110; if (ow > 240) ow = 240;
      int oh = ow * 5 / 8;
      int bob = (S->crt_v[vk] > 60.0f)? (int)(fsin(S->crt_t * 26.0f) * 1.6f) : 0;
      int cxp = vx + vw2/2, base = vy + vh2 - 4 + bob;
      ellipse_fill(cxp, base, ow/2, oh/6, COL(20, 40, 22));
      if (S->crt_boost[vk] > 0){
          u32 fl = ((int)(S->crt_t * 20.0f) & 1)? COL(255, 160, 60) : COL(255, 210, 90);
          tri_fill(cxp - ow/3, base - 2, cxp - ow/6, base - 2, cxp - ow/4, base + oh/3, fl);
          tri_fill(cxp + ow/6, base - 2, cxp + ow/3, base - 2, cxp + ow/4, base + oh/3, fl);
      }
      rect_fill(cxp - ow/2, base - oh/3, ow/5, oh/3, COL(16, 14, 24));
      rect_fill(cxp + ow/2 - ow/5, base - oh/3, ow/5, oh/3, COL(16, 14, 24));
      rect_fill(cxp - ow/2 + ow/10, base - oh, ow * 4/5, oh * 3/4, KCOL[S->crt_kcol[vk]]);
      rect_line(cxp - ow/2 + ow/10, base - oh, ow * 4/5, oh * 3/4, COL(16, 14, 24));
      rect_fill(cxp - ow/2 + ow/10 + 3, base - oh + 2, ow * 4/5 - 6, oh/6, col_shade(KCOL[S->crt_kcol[vk]], 1.30f));
      rect_fill(cxp - ow/2 + ow/10 + ow/6, base - oh/4, ow * 4/5 - ow/3, oh/4, col_shade(KCOL[S->crt_kcol[vk]], 0.60f));
      crt_draw_driver(cxp, base - oh + oh/8, ow, S->crt_species[vk], crt_coat_for(S->crt_species[vk], S->crt_dcolidx[vk], vk));
    }
    // minimap in the corner: the whole flat circuit, SMK-style.
    // (positioned from the CLIP rect, not the view rect: in split
    // screen the view origin stays at 0 and the clip does the split,
    // so a view-relative corner lands in the other player's half)
    if (clip_x1 - clip_x0 >= 380){
        int cw2 = clip_x1 - clip_x0;
        int mw = cw2 >= 470? 100 : 76, mh = cw2 >= 470? 62 : 48, mx0 = clip_x1 - mw - 8, my0 = clip_y0 + 30;
        rect_fill(mx0, my0, mw, mh, COL(10, 26, 14));
        rect_line(mx0, my0, mw, mh, COL(120, 160, 120));
        for (int i = 0; i < CRT_N; i += 2)
            rect_fill(mx0 + 4 + (int)(CRT_WX[i] * (mw - 8) / 640.0f), my0 + 4 + (int)(CRT_WY[i] * (mh - 8) / 360.0f), 2, 2, COL(120, 130, 120));
        for (int i = 0; i < 6; i++){
            u32 dc = (i == vk)? COL(255, 255, 255) : (S->crt_duo >= 0 && i == 1 - vk)? COL(255, 230, 90) : COL(160, 160, 160);
            rect_fill(mx0 + 3 + (int)(S->crt_x[i] * (mw - 8) / 640.0f), my0 + 3 + (int)(S->crt_y[i] * (mh - 8) / 360.0f), 3, 3, dc);
        }
    }
#undef CRT_PROJ
    }
    // HUD band
    rect_fill(vx, vy, vw2, 24, COL(10, 12, 16));
    { char buf[96]; char *q = buf;
      int lap = (int)(S->crt_prog[vk] / CRT_LEN) + 1; if (lap < 1) lap = 1; if (lap > 3) lap = 3;
      int tds = (int)(S->crt_t * 10.0f); int sec = (tds / 10) % 60;
      q = sappend(q, "RACE "); q = sapp_int(q, S->crt_race + 1); q = sappend(q, "/4  LAP "); q = sapp_int(q, lap); q = sappend(q, "/3  POS ");
      q = sapp_int(q, S->crt_place[vk]); q = sappend(q, "/6  TIME ");
      q = sapp_int(q, tds / 600); q = sappend(q, ":");
      if (sec < 10) q = sappend(q, "0");
      q = sapp_int(q, sec); q = sappend(q, "."); q = sapp_int(q, tds % 10); *q = 0;
      draw_text(vx + 10, vy + 8, buf, 1, COL(255, 240, 200)); }
    if (vw2 >= 600) draw_text_center(vx + vw2/2, vy + 8, REGION_NAME[S->crt_track], 1, COL(250, 200, 50));
    if (vw2 >= 600 && b->crt_best_pos){ char buf[48]; char *q = buf;
        q = sappend(q, "BEST P"); q = sapp_int(q, b->crt_best_pos); *q = 0;
        draw_text(vx + vw2 - 70, vy + 8, buf, 1, COL(200, 214, 200)); }
    if (S->crt_state == 0){
        int n = (int)(S->crt_count + 0.999f); if (n < 1) n = 1; if (n > 3) n = 3;
        char nb[2] = { (char)('0' + n), 0 };
        draw_text_center(vx + vw2/2, vy + vh2/2 - 66, nb, 8, COL(255, 220, 90));
        draw_text_center(vx + vw2/2, vy + vh2/2 + 22, "GET READY...", 2, COL(255, 255, 255));
        draw_text_center(vx + vw2/2, vy + vh2/2 + 52, pad_mode()? "HOLD A TO GO - STEER WITH THE STICK" : "HOLD FIRE TO GO - STEER WITH THE ARROWS", 1, COL(200, 230, 200));
    } else if (S->crt_state == 1 && S->crt_t < 0.9f){
        draw_text_center(vx + vw2/2, vy + vh2/2 - 40, "GO!", 6, COL(120, 255, 120));
    } else if (S->crt_state == 2){
        { float el = 3.4f - S->crt_fint; if (el < 0) el = 0;   // confetti over the finish
          static const u32 CONF[5] = { COL(250,200,50), COL(230,90,80), COL(120,180,240), COL(120,220,140), COL(240,240,240) };
          for (int ci = 0; ci < 42; ci++){
              u32 h1 = hash2(ci, 5, 0xC0F1u);
              int cxp2 = vx + (int)(h1 % (u32)(vw2 > 4? vw2 : 5));
              int cyp2 = vy + (int)((h1 / 7u + (u32)(el * (46.0f + (float)(h1 % 30u)))) % (u32)(vh2 > 4? vh2 : 5));
              rect_fill(cxp2, cyp2, 4, 3, CONF[h1 % 5]);
          } }
        rect_fill(vx + vw2/2 - 130, vy + vh2/2 - 44, 260, 84, COL(12, 14, 22));
        rect_line(vx + vw2/2 - 130, vy + vh2/2 - 44, 260, 84, COL(250, 200, 50));
        { char buf[64]; char *q = buf;
          q = sappend(q, "FINISH!  P"); q = sapp_int(q, S->crt_hfin[vk]); *q = 0;
          draw_text_center(vx + vw2/2, vy + vh2/2 - 32, buf, 3, COL(255, 240, 200)); }
        { char buf[64]; char *q = buf; int tds = (int)(S->crt_t * 10.0f); int sec = (tds / 10) % 60;
          q = sappend(q, "TIME "); q = sapp_int(q, tds / 600); q = sappend(q, ":");
          if (sec < 10) q = sappend(q, "0");
          q = sapp_int(q, sec); q = sappend(q, "."); q = sapp_int(q, tds % 10); *q = 0;
          draw_text_center(vx + vw2/2, vy + vh2/2 + 8, buf, 1, COL(200, 214, 200)); }
    }
    if (S->note_t > 0){
        rect_fill(vx, vy + vh2 - 22, vw2, 22, COL(10, 12, 10));
        draw_text_center(vx + vw2/2, vy + vh2 - 15, S->note, 1, COL(220, 255, 220));
    }
#undef KTX
#undef KTY
#undef KTS
}
static void crt_render(int pi){ crt_render_view(pi, pi); }

