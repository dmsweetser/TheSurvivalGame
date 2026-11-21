// THE SURVIVAL GAME — modes/cart/cart_track.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from cart.c lines 1-302 in the Nov 2026 fine-grained refactor; code unchanged.

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
#define CRT_MAXW 384
static float CRT_WX[CRT_MAXW], CRT_WY[CRT_MAXW], CRT_CUM[CRT_MAXW + 1];
static int CRT_N = 0;
static float CRT_LEN = 0, CRT_OFF = 0;
static float CRT_PAD[5][2];
// v1.23 progression: each regional track lives in its own scaled
// world (CRT_WW x CRT_WH, centre CRT_CX/CRT_CY, base radius CRT_RAD)
// so the circuits can grow far beyond the old 640x360 frame; the road
// half-width (CRT_HW, with rumble/apron/edge derived), the boost-pad
// count and the on-road tire-stack obstacles (CRT_OBS) all ride along.
static float CRT_WW = 640, CRT_WH = 360, CRT_CX = 320, CRT_CY = 180, CRT_RAD = 92;
static float CRT_HW = 30, CRT_RUM = 37, CRT_APR = 45, CRT_EDG = 32.5f;
static int CRT_NPAD = 3, CRT_NOBS = 0;
static float CRT_OBS[5][2];
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
// v1.23 progression (Daniel): the races grow — each regional circuit
// is its v1.11 self scaled up by CRT_SCALE and given a third, faster
// wiggle (CRT_H3: amplitude in scale-1 units, harmonic, phase), so
// centreline length ramps 1.15x / 1.30x / 1.50x / 1.70x / 1.90x /
// 2.10x / 2.40x against the Meadows oval (tuned offline against a
// replica of this builder; verified in-game via dbg_crtskip).
// CRT_M is the waypoint sample count (spacing stays ~8 units).
static const float CRT_SCALE[7] = { 1.504f, 1.658f, 2.070f, 2.106f, 2.594f, 2.580f, 2.992f };
static const float CRT_H3[7][3] = {
    { 11, 4, 4.5f }, { 9, 4, 4.5f }, { 7, 4, 0.0f }, { 11, 4, 4.5f },
    { 9, 4, 1.8f },  { 13, 4, 3.6f }, { 13, 4, 0.0f },
};
static const int CRT_M[7] = { 168, 188, 217, 246, 275, 304, 347 };
// MEDAL TIMES (v1.25 feat 3): {gold, silver, bronze} thresholds in
// deciseconds per track, DERIVED from the slowest rival's measured
// full-race pace in the shipped game (kart 1, player parked; spec:
// v125-feat3-spec.md): gold = 0.92x, silver = 1.00x, bronze = 1.16x of
// that anchor, rounded to the second. Track 5's anchor is smoothed to
// track 4's pace (its raw AI time is a steering-quirk outlier), so
// difficulty stays monotonic in track length. Medals derive from the
// stored per-track best (Buddy.crt_tbest) — retune here, saves follow.
// LIVE TUNING (v1.26 Item 3): mutable — data/livedata.c holds a tentative
// declaration and the registry points here; literals are stock defaults.
static int crt_medal[8][3] = {
    { 230, 250, 290 }, { 270, 290, 340 }, { 290, 320, 370 }, { 330, 360, 420 },
    { 620, 680, 790 }, { 700, 760, 880 }, { 770, 840, 970 }, { 830, 900, 1040 },
};
static const char *CRT_SHORT[8] = { "MEADOWS", "SPAGHETTI", "LEGACY", "BADLANDS", "FROZEN", "NEON RAM", "JUNGLE", "DEPTHS" };
// TROPHY CASE (v1.25 feat 4): how many of this buddy's tracks derive
// to a given medal tier (0 = a best exists but earned no medal). The
// case is a pure view — it counts through here so the medal table
// keeps exactly one reader per piece.
static int crt_medal_count(Buddy *b, int tier){
    int n=0;
    for (int t=0;t<8;t++) if (medal_tier(b->crt_tbest[t], crt_medal[t])==tier && (tier>0 || b->crt_tbest[t]>0)) n++;
    return n;
}
// append a decisecond time as m:ss.d (the finish panel's format)
static char *sapp_time(char *q, int tds){
    int sec = (tds / 10) % 60;
    q = sapp_int(q, tds / 600); q = sappend(q, ":");
    if (sec < 10) q = sappend(q, "0");
    q = sapp_int(q, sec); q = sappend(q, "."); q = sapp_int(q, tds % 10);
    return q;
}
// per-track dressing (index = track): the road narrows, the boost
// pads multiply and tire stacks appear on the road in the far country
static const float CRT_HW_T[8]  = { 30, 30, 30, 30, 28, 26, 24, 22 };
static const int CRT_NPAD_T[8]  = { 3, 3, 3, 4, 4, 5, 5, 5 };
static const int CRT_NOBS_T[8]  = { 0, 0, 0, 0, 2, 3, 4, 5 };
static void crt_build(int track){
    CRT_N = 0;
    if (track <= 0){
        CRT_WW = 640; CRT_WH = 360; CRT_CX = 320; CRT_CY = 180; CRT_RAD = 92;
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
        const float *H = CRT_H3[track-1];
        float s = CRT_SCALE[track-1];
        int M = CRT_M[track-1];
        CRT_WW = 640.0f * s; CRT_WH = 360.0f * s;
        CRT_CX = 320.0f * s; CRT_CY = 180.0f * s; CRT_RAD = 92.0f * s;
        for (int i = 0; i < M; i++){
            float th = (float)i / (float)M * 6.2831853f;
            float r = CRT_RAD + P[0]*s*fsin(2.0f*th + P[2]) + P[1]*s*fsin(3.0f*th + P[3])
                    + H[0]*s*fsin(H[1]*th + H[2]);
            crt_addpt(CRT_CX + fcos(th) * r * 1.82f, CRT_CY + fsin(th) * r);
        }
    }
    CRT_HW = CRT_HW_T[track];
    CRT_RUM = CRT_HW * 37.0f / 30.0f; CRT_APR = CRT_HW * 1.5f; CRT_EDG = CRT_HW * 32.5f / 30.0f;
    CRT_NPAD = CRT_NPAD_T[track]; CRT_NOBS = CRT_NOBS_T[track];
    CRT_CUM[0] = 0;
    for (int i = 1; i <= CRT_N; i++){ int a = i-1, b = i % CRT_N;
        float dx = CRT_WX[b] - CRT_WX[a], dy = CRT_WY[b] - CRT_WY[a];
        CRT_CUM[i] = CRT_CUM[i-1] + fsqrt(dx*dx + dy*dy); }
    CRT_LEN = CRT_CUM[CRT_N];
    { int bi = 0; float bd = 1e18f;        // the start line sits at the bottom of the loop
      float tx = CRT_CX, ty = (track <= 0)? 290.0f : CRT_CY + CRT_RAD * 1.02f;
      for (int i = 0; i < CRT_N; i++){ float dx = CRT_WX[i]-tx, dy = CRT_WY[i]-ty;
          float d = dx*dx + dy*dy; if (d < bd){ bd = d; bi = i; } }
      CRT_OFF = CRT_CUM[bi]; }
    if (track <= 0){
        CRT_PAD[0][0] = 320; CRT_PAD[0][1] = 70;
        CRT_PAD[1][0] = 530; CRT_PAD[1][1] = 180;
        CRT_PAD[2][0] = 110; CRT_PAD[2][1] = 180;
    } else {
        static const float PF3[3] = { 0.30f, 0.56f, 0.82f };
        static const float PF4[4] = { 0.20f, 0.44f, 0.66f, 0.88f };
        static const float PF5[5] = { 0.14f, 0.32f, 0.50f, 0.68f, 0.86f };
        const float *PF = (CRT_NPAD == 5)? PF5 : (CRT_NPAD == 4)? PF4 : PF3;
        for (int k = 0; k < CRT_NPAD; k++){ float x, y, hx, hy;
            crt_point_at(CRT_OFF + PF[k] * CRT_LEN, 0, &x, &y, &hx, &hy);
            CRT_PAD[k][0] = x; CRT_PAD[k][1] = y; }
        // tire-stack obstacles on the road's edge, alternating sides —
        // the far country's racing line is not a clean one
        static const float OF2[2] = { 0.35f, 0.72f };
        static const float OF3[3] = { 0.26f, 0.52f, 0.78f };
        static const float OF4[4] = { 0.24f, 0.46f, 0.64f, 0.84f };
        static const float OF5[5] = { 0.18f, 0.36f, 0.54f, 0.72f, 0.88f };
        const float *OF = (CRT_NOBS == 5)? OF5 : (CRT_NOBS == 4)? OF4 : (CRT_NOBS == 3)? OF3 : OF2;
        for (int k = 0; k < CRT_NOBS; k++){ float x, y, hx, hy;
            float lat = ((k % 2)? -1.0f : 1.0f) * (CRT_HW - 4.0f);
            crt_point_at(CRT_OFF + OF[k] * CRT_LEN, lat, &x, &y, &hx, &hy);
            CRT_OBS[k][0] = x; CRT_OBS[k][1] = y; }
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
          if (x < 14 || x > CRT_WW - 14 || y < 14 || y > CRT_WH - 14) continue;
          if (track > 0){   // v1.23: on the big folded circuits a ring
                            // spot can land on a NEIGHBOURING branch —
                            // keep scenery a clear 48 off the asphalt
              float bd2 = 1e18f;
              for (int i = 0; i < CRT_N; i += 2){ float dx = CRT_WX[i]-x, dy = CRT_WY[i]-y; float d = dx*dx + dy*dy; if (d < bd2) bd2 = d; }
              if (bd2 < 48.0f * 48.0f) continue;
          }
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
              if (x < 20 || x > CRT_WW - 20 || y < 20 || y > CRT_WH - 20) continue;
              float bd2 = 1e18f;
              for (int i = 0; i < CRT_N; i += 2){ float dx = CRT_WX[i]-x, dy = CRT_WY[i]-y; float d = dx*dx + dy*dy; if (d < bd2) bd2 = d; }
              if (bd2 < 50.0f * 50.0f) continue;
              CRT_SCX[n] = x; CRT_SCY[n] = y; CRT_SCT[n] = 0; n++;
          }
          // the grandstand watches the start from outside the loop
          // (Nov 15: pushed further out and further past the line —
          // at 116 it loomed over the grid as a screen-filling wall)
          { float x, y, hx, hy; int placed = 0;
            for (int side = 0; side < 2 && !placed; side++){
                crt_point_at(CRT_OFF + CRT_LEN * 0.10f, side? -152.0f : 152.0f, &x, &y, &hx, &hy);
                if (!(x > 56 && x < CRT_WW - 56 && y > 22 && y < CRT_WH - 22)) continue;
                float bd2 = 1e18f;   // v1.23: never on a neighbouring branch
                for (int i = 0; i < CRT_N; i += 2){ float dx = CRT_WX[i]-x, dy = CRT_WY[i]-y; float d = dx*dx + dy*dy; if (d < bd2) bd2 = d; }
                if (bd2 < 48.0f * 48.0f) continue;
                CRT_SCX[n] = x; CRT_SCY[n] = y; CRT_SCT[n] = 3; n++; placed = 1;
            } }
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
