// THE SURVIVAL GAME — world/sky.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 35-127 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// THE LIVING SKY (v1.25 feature 12): a day/night tint over the
// overworld and Banzai World's outdoors, plus signature weather in
// three regions. Spec: hidden_files/v125-feat12-spec.md.
// The driver is the device's local wall clock (plat_minutes /
// plat_daynum — the same trust the Weekly Challenge already makes),
// so the feature is STATELESS: nothing saved, nothing synced, two
// loads at the same moment always agree. Colour only — no spawn,
// difficulty, movement or record anywhere reads this module.
// ---------------------------------------------------------------------------
static float tod_test_min = -1.0f;   // test-only pin (dbg_todset); never saved
static int   tod_test_day = -1;      // test-only pin (dbg_todday); never saved
static void tod_test_set_min(float m){ tod_test_min = m; }
static void tod_test_set_day(int d){ tod_test_day = d; }
static float tod_minutes(void){
    if (tod_test_min >= 0.0f) return tod_test_min;
    float m = plat_minutes();
    if (m < 0.0f) m = 0.0f; if (m >= 1440.0f) m = 1439.99f;
    return m;
}
static int tod_daynum(void){ return tod_test_day >= 0? tod_test_day : plat_daynum(); }
static float tod_smooth(float p){ return p*p*(3.0f - 2.0f*p); }   // smoothstep, bounded input
// darkness 0..1: night plateau 19:30-04:30, dawn ramp 04:30-06:30,
// day until 17:30, dusk ramp 17:30-19:30
static float tod_darkf(void){
    float m = tod_minutes();
    if (m < 270.0f) return 1.0f;
    if (m < 390.0f) return 1.0f - tod_smooth((m - 270.0f)/120.0f);
    if (m < 1050.0f) return 0.0f;
    if (m < 1170.0f) return tod_smooth((m - 1050.0f)/120.0f);
    return 1.0f;
}
// warmth 0..1: a bell over each ramp (peaks mid-dawn / mid-dusk,
// where darkf crosses 0.5), zero at noon and at deep night
static float tod_warmf(void){
    float m = tod_minutes();
    if (m >= 270.0f && m < 390.0f) return fsin(PI * (m - 270.0f)/120.0f);
    if (m >= 1050.0f && m < 1170.0f) return fsin(PI * (m - 1050.0f)/120.0f);
    return 0.0f;
}
// a sky colour under the living sky: warmed at the ramps, cooled
// toward night blue after dark. Skies may go deeper than the ground
// tint — a sky is not a readability surface.
static u32 tod_sky(u32 c){
    float dk = tod_darkf(), wf = tod_warmf();
    if (wf > 0.001f) c = col_mix(c, COL(255,166,88), 0.22f*wf);
    if (dk > 0.001f) c = col_mix(c, COL(16,20,50), 0.62f*dk);
    return c;
}
// the tint itself, laid over one rendered view (clip bounds): warm
// first, then the night — capped at 96/255 so the world cools, it
// never hides. Interiors never reach this call (render early-outs).
static void tod_apply_tint(void){
    float dk = tod_darkf(), wf = tod_warmf();
    int x0 = clip_x0, y0 = clip_y0, w = clip_x1 - clip_x0, h = clip_y1 - clip_y0;
    if (w <= 0 || h <= 0) return;
    if (wf > 0.004f) blend_rect(x0, y0, w, h, COL(255,146,54), (int)(38.0f*wf));
    if (dk > 0.004f) blend_rect(x0, y0, w, h, COL(10,14,38), (int)(96.0f*dk));
}
// --- regional weather: kinds are a region's signature; the state is
// a pure hash of (epoch, region). Epochs are 3-hour blocks of local
// time, so weather is a slow fact about the day. 0 none, 1 rain,
// 2 snow, 3 shimmer.
static const int WX_KIND[NREGIONS] = { 0, 0, 0, 3, 2, 0, 1, 0 };   // Badlands shimmer, Frozen snow, Jungle rain
static int wx_epoch_now(void){ return tod_daynum()*8 + (int)(tod_minutes()/180.0f); }
static int wx_state(int region, int epoch){
    if (region < 0 || region >= NREGIONS || WX_KIND[region]==0) return 0;
    int v = (int)((hash2(epoch, region*7 + 1, 0x5EEDu) >> 8) % 4);
    return v < 2? 0 : v - 1;   // {0,1 -> quiet, 2 -> light, 3 -> steady}
}
static int station_at(Player *p){
    if (!p->in_mall) return -1;
    for (int s=0;s<4;s++){
        float dx=p->x - MALL_SX[s], dy=p->y - (MALL_CY-148);
        if (dx*dx + dy*dy < 64*64) return s;
    }
    { float dx=p->x - BOWL_SX, dy=p->y - (MALL_CY+132);   // the bowling counter
      if (dx*dx + dy*dy < 64*64) return 4; }
    { float dx=p->x - CASE_SX, dy=p->y - (MALL_CY+132);   // the trophy case
      if (dx*dx + dy*dy < 64*64) return 5; }
    { float dx=p->x - BOARD_SX, dy=p->y - (MALL_CY+132);   // the challenge board
      if (dx*dx + dy*dy < 64*64) return 6; }
    { float dx=p->x - BOUNTY_WX, dy=p->y - BOUNTY_WY;   // the bounty board, west wall
      if (dx*dx + dy*dy < 60*60) return 7; }
    { float dx=p->x - BOOTH_SX, dy=p->y - BOOTH_DY;   // the photo booth island
      // feat 9's rule rides along: the booth poses you WITH your buddy,
      // so it answers only a player whose buddy is here and healthy —
      // behind the Banzai menu, the booth rows simply didn't exist
      // for anyone else, and the fixture keeps that shape.
      if (dx*dx + dy*dy < 56*56 && G.bud[p-G.p].active && !G.bud[p-G.p].fainted) return 8; }
    { int s9 = src_station_at(p); if (s9>=0) return s9; }   // v1.26: the Source terminal (its own room)
    return -1;
}
#define BW_SX 215                                    // the Banzai World door, south-east
