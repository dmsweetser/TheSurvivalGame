// THE SURVIVAL GAME — music/adaptive.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from music.c lines 498-574 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
static float mus_bw_dgr[MAXP];   // Banzai danger, pushed per frame by bw_update (banzai.c)
static void mus_bw_note(int pi, float v){ if (pi>=0 && pi<MAXP) mus_bw_dgr[pi]=v; }
static int mus_lvl = 0;          // the latched level: 0 sparse, 1 full
static int mus_desired = 0;      // where the meter wants the latch
static float mus_clear_t = 99.0f; // continuous seconds the signal has read CLEAR
static int mus_hot_now = 0;      // the raw signal, this frame (dbg_mushot)
static int mus_ctx_now = 0;      // 0 ungated, 1 overworld, 2 Banzai (dbg_musctx)
static int mus_ctx_prev = -1;
// Whose danger drives the music — mus_piece's own priority, so the
// intensity always belongs to the piece actually playing.
static int mus_context(int *bw_pi){
    *bw_pi = -1;
    if (G.state!=GS_PLAY) return 0;
    if (G.net_mode==NET_CLIENT) return 0;   // the host's bugs are the only real ones; a guest's music is v1.24's
    int me = G.my_id;
    if (me>=0 && G.bud[me].in_bw){
        int m = G.bud[me].bw_mode;
        if (m==0 || m==1){ *bw_pi = me; return 2; }
        return 0;                            // cabinets + Craft: the full arrangement, as always
    }
    int p2 = G.p2_slot;
    if (p2>=0 && G.bud[p2].in_bw){
        int m = G.bud[p2].bw_mode;
        if (m==0 || m==1){ *bw_pi = p2; return 2; }
        return 0;
    }
    return 1;
}
// The overworld signal: -1 = nobody in the field (not a gated
// moment — everyone is in the Mall), 0 = clear, 1 = hot. Distances
// are to the NEAREST anchor: a connected, alive, in-field player,
// or their active field buddy.
static int mus_ow_hot(void){
    float ax[MAXP*2], ay[MAXP*2]; int na=0;
    for (int i=0;i<MAXP;i++){
        Player *p=&G.p[i]; Buddy *b=&G.bud[i];
        if (p->connected && p->alive && !p->in_mall && !b->in_bw){ ax[na]=p->x; ay[na]=p->y; na++; }
        if (p->connected && !p->in_mall && b->active && !b->fainted && !b->in_bw){ ax[na]=b->x; ay[na]=b->y; na++; }
    }
    if (!na) return -1;
    int near_n=0;
    for (int i=0;i<MAXBUGS;i++){
        Bug *b=&G.bugs[i]; if (!b->alive) continue;
        float best=1e18f;
        for (int j=0;j<na;j++){ float dx=b->x-ax[j], dy=b->y-ay[j], d2=dx*dx+dy*dy; if (d2<best) best=d2; }
        if (b->elite && best < 800.0f*800.0f) return 1;   // a named bug, stalking the party
        if (best < 230.0f*230.0f) return 1;               // a committed chase
        if (best < 430.0f*430.0f) near_n++;
    }
    return near_n>=4? 1 : 0;   // a crowd pressing in — the rings are tuned inside the spawner's approach stream, so a fighting player crosses them both ways and a passive one is overrun
}
// Per frame: read the signal, run the release timer, set the desire.
// The latch itself moves only at a bar boundary, in the step block
// below — a threat flickering at the edge of the ring cannot stutter
// the arrangement, and a fight's end holds the full arrangement for
// a 4-second breath before the sparse one returns.
static void mus_meter(float dt){
    int bw_pi=-1;
    int ctx = mus_context(&bw_pi);
    if (ctx!=mus_ctx_prev){ mus_clear_t = 99.0f; mus_ctx_prev = ctx; }
    int raw = 0;
    if (ctx==1){ int h = mus_ow_hot(); if (h<0) ctx = 0; else raw = h; }
    else if (ctx==2){ raw = mus_bw_dgr[bw_pi] > 0.0f? 1 : 0; }
    mus_ctx_now = ctx;
    mus_hot_now = raw;
    if (!ctx){ mus_desired = 1; return; }   // ungated moments hold the latch full
    if (raw){ mus_clear_t = 0; mus_desired = 1; }
    else {
        mus_clear_t += dt;
        mus_desired = mus_clear_t>=4.0f? 0 : mus_lvl;   // the held breath
    }
}
static int mus_dbg_lvl(void){ return mus_lvl; }
static int mus_dbg_hot(void){ return mus_hot_now; }
static int mus_dbg_ctx(void){ return mus_ctx_now; }
static float mus_step_t=0; static int mus_step=0;
