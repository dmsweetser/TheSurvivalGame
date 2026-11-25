// THE SURVIVAL GAME — save/saveio.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from save.c lines 1-329 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// Save / load — a small line-based text format in one block.
// ---------------------------------------------------------------------------
#define SAVE_CAP 262144   // v1.29: doubled for the second (skirmish) WAR! snapshot per player — see the audit below. v1.33: doubled again for the 120x90 fog key and the raised unit/struct caps
static char savebuf[SAVE_CAP];
static int parse_int(const char *s){ int v=0, neg=0; if (*s=='-'){ neg=1; s++; }
    while (*s>='0'&&*s<='9'){ v=v*10+(*s-'0'); s++; } return neg?-v:v; }
static float parse_flt(const char *s){ float v=0, f=0.1f; int neg=0;
    if (*s=='-'){ neg=1; s++; }
    while (*s>='0'&&*s<='9'){ v=v*10+(*s-'0'); s++; }
    if (*s=='.'){ s++; while (*s>='0'&&*s<='9'){ v+=(*s-'0')*f; f*=0.1f; s++; } }
    return neg?-v:v; }
static char *put_int(char *p, int v){ return sapp_int(p, v); }
static char *put_flt(char *p, float v){ // two decimals is plenty
    int iv = (int)(v*100 + (v>=0? 0.5f : -0.5f));
    p = sapp_int(p, iv/100); *p++='.'; int f = iv%100; if (f<0) f=-f;
    *p++ = (char)('0'+f/10); *p++ = (char)('0'+f%10); *p=0; return p;
}
static char *put_key(char *p, const char *k){ p=sappend(p,k); *p++='='; *p=0; return p; }
// weapon copy counts pack base-6 into one int (counts run 0..MAXCOPIES)
static int pack_copies(Player *p){ int v=0,m=1; for (int w=0;w<NWEAP;w++){ v += p->owned[w]*m; m*=6; } return v; }
static void unpack_copies(Player *p, int v){ for (int w=0;w<NWEAP;w++){ p->owned[w]=v%6; v/=6; } }
static void unpack_owned_mask(Player *p, int v){ for (int w=0;w<NWEAP;w++) p->owned[w]=(v>>w)&1; }
static char slotbuf[SAVE_CAP];
static void refresh_has_save(void){ G.has_save = (G.slot_has[1]||G.slot_has[2]||G.slot_has[3])? 1:0; }
// WAR! battle snapshots (v1.17): a parked mid-battle state per player,
// stored as the canonical save-token text ("wb=... wu=... ws=...
// wg=... wt=... wf=...") so the writer can emit it verbatim and the
// loader can rebuild it token by token. Sized for the worst case:
// 80 unit records, 28 structs, 160 terrain (v1.18: barriers joined
// the wt record, and a barrier's hp field is 5 digits), the 38-word
// fog bitset — wb ~40 + wu ~3.8K + ws ~1.9K + wg ~0.1K + wt ~4.3K
// + wf ~0.5K = ~10.7K of text worst case; the 12288 cap holds it
// with ~1.6K to spare.
// (SAVE_CAP audit, redone Nov 16 for v1.18: per-player worst ~3.5K
// of pl-line + <=10.7K war snapshot = ~14.2K; x4 players = ~56.8K
// + ~420 shared walls + ~0.4K headers = ~57.6K worst case, inside
// the 65536 cap with ~7.9K to spare. Feat 9's pt= token adds at
// most ~0.5K per player — ~59.6K worst case, still inside. Feat
// 10's bl= token adds at most ~90 bytes per player (~0.4K over
// four) — ~60K worst case, still inside.)
// (v1.33 audit: the fog key is now the whole 120x90 array — 338
// words, ~2.3K of text — the unit cap rose to 128 and the struct
// cap to 44 (wu ~6.3K, ws ~2.8K), and the wt worst case stands at
// ~4.3K: one snapshot's worst case is ~16K. The cap rises to
// 24576; the SAVE_CAP audit below is redone with it.)
#define WAR_SNAP_CAP 24576
static char war_snap[MAXP][WAR_SNAP_CAP];
static int war_snap_on[MAXP];
// v1.29: the SKIRMISH park — a second snapshot slot per player,
// same canonical text under kb/ku/ks/kg/kt/kf/kh tokens, so a
// skirmish persists beside (never instead of) the campaign park.
// SAVE_CAP audit extended: per-player worst ~3.5K pl-line +
// <=10.7K campaign snapshot + <=10.7K skirmish snapshot = ~24.9K;
// x4 players = ~99.6K + ~420 shared walls + ~0.4K headers =
// ~100K, inside the doubled 131072 cap with ~31K to spare.
// (v1.33: with WAR_SNAP_CAP at 24576 the per-player worst is
// ~3.5K pl-line + ~16K + ~16K = ~35.5K; x4 = ~142K + walls and
// headers — past the old cap, so SAVE_CAP doubles again below.)
static char war_snap_sk[MAXP][WAR_SNAP_CAP];
static int war_snap_sk_on[MAXP];
static int war_snap_reg[MAXP] = { -1, -1, -1, -1 };   // v1.18: WHICH region the parked battle belongs to — the map needs it to offer a resume instead of a restart
static void war_snap_clear(int pi){ war_snap_on[pi]=0; war_snap[pi][0]=0; war_snap_reg[pi]=-1; }
static void war_snap_sk_clear(int pi){ war_snap_sk_on[pi]=0; war_snap_sk[pi][0]=0; }   // v1.29: defined here, with the buffers — war/hive.c and war/path.c (both before war/snap.c in the unity order) call it
static void do_save(void){
    char *p = savebuf;
    p = sappend(p, "TSG3\n");   // v3: weapon counts written raw (stacks are uncapped now); v2 was base-6 packed, v1 a bitmask — both still load
    p = put_key(p, "rec_name"); p = sappend(p, G.rec_name); *p++='\n'; *p=0;
    p = put_key(p, "rec_hat");  p = put_flt(p, G.rec_hat); *p++='\n'; *p=0;
    p = put_key(p, "rec_kills"); p = put_int(p, G.rec_kills); *p++='\n'; *p=0;
    p = put_key(p, "rec_bowl"); p = put_int(p, G.rec_bowl); *p++='\n'; *p=0;
    p = put_key(p, "rec_time"); p = put_flt(p, G.rec_time); *p++='\n'; *p=0;
    p = put_key(p, "my_name");  p = sappend(p, G.my_name); *p++='\n'; *p=0;
    p = put_key(p, "bind");
    for (int i=0;i<14;i++){ if (i){ *p++=','; *p=0; } p = put_int(p, G.bind[i]); }
    *p++='\n'; *p=0;
    p = put_key(p, "touchctl"); p = put_int(p, G.touch_ctl); *p++='\n'; *p=0;
    p = put_key(p, "zen"); p = put_int(p, G.zen); *p++='\n'; *p=0;
    p = put_key(p, "eddie"); p = put_int(p, G.eddie); *p++='\n'; *p=0;
    p = put_key(p, "whale"); p = put_int(p, G.whale_own); *p++='\n'; *p=0;
    p = put_key(p, "whaleon"); p = put_int(p, G.whale_on); *p++='\n'; *p=0;
    p = put_key(p, "coop"); p = put_int(p, G.coop_pref); *p++='\n'; *p=0;
    p = put_key(p, "diff"); p = put_int(p, G.difficulty); *p++='\n'; *p=0;
    p = put_key(p, "volm"); p = put_int(p, G.vol_music); *p++='\n'; *p=0;
    p = put_key(p, "vols"); p = put_int(p, G.vol_sfx); *p++='\n'; *p=0;
    p = put_key(p, "mute"); p = put_int(p, G.mute_all); *p++='\n'; *p=0;
    p = put_key(p, "shk"); p = put_int(p, G.shake_on); *p++='\n'; *p=0;
    p = put_key(p, "slot"); p = put_int(p, G.slot); *p++='\n'; *p=0;
    char *runstart = 0;
    int run_live = G.state==GS_PLAY || G.state==GS_STORE || G.state==GS_PAUSE
                || G.bud[G.my_id].in_bw || (G.p2_slot>=0 && G.bud[G.p2_slot].in_bw);
    if (run_live){
        runstart = p;
        p = put_key(p, "run"); p = put_int(p, 1); *p++='\n'; *p=0;
        p = put_key(p, "zrun"); p = put_int(p, G.zen_run); *p++='\n'; *p=0;
        p = put_key(p, "drun"); p = put_int(p, G.diff_run); *p++='\n'; *p=0;
        p = put_key(p, "time"); p = put_flt(p, G.time); *p++='\n'; *p=0;
        p = put_key(p, "tkills"); p = put_int(p, G.total_kills); *p++='\n'; *p=0;
        p = put_key(p, "tearned"); p = put_flt(p, G.total_earned); *p++='\n'; *p=0;
        p = put_key(p, "fs"); p = put_int(p, G.fs_state); *p++=','; *p=0; p = put_int(p, G.fs_step); *p++=','; *p=0; p = put_int(p, G.fs_fp0); *p++='\n'; *p=0;
        p = put_key(p, "wk"); p = put_int(p, G.wk_week); *p++=','; *p=0; p = put_int(p, G.wk_done_week); *p++=','; *p=0; p = put_int(p, G.wk_total); *p++=','; *p=0; p = put_int(p, G.wk_prev_week); *p++=','; *p=0; p = put_int(p, G.wk_prev_done);
        for (int i=0;i<7;i++){ *p++=','; *p=0; p = put_int(p, G.wk_base[i]); }
        *p++='\n'; *p=0;
        p = put_key(p, "bn"); p = put_int(p, G.bn_serial); *p++=','; *p=0; p = put_int(p, G.bn_done); *p++=','; *p=0; p = put_int(p, (int)(G.bn_cool+0.5f)); *p++='\n'; *p=0;
        for (int i=0;i<MAXP;i++) if (G.p[i].connected){
            Player *pl=&G.p[i];
            p = put_key(p, "pl"); *p++=' '; *p=0;
            p = put_int(p, i); *p++=' '; *p=0;
            p = sappend(p, pl->name); *p++=' '; *p=0;
            p = put_flt(p, pl->x); *p++=' '; *p=0;
            p = put_flt(p, pl->y); *p++=' '; *p=0;
            p = put_int(p, pl->hp); *p++=' '; *p=0;
            p = put_int(p, pl->alive); *p++=' '; *p=0;
            p = put_int(p, pl->weapon); *p++=' '; *p=0;
            for (int w=0;w<NWEAP;w++){ p = put_int(p, pl->owned[w]); *p++=' '; *p=0; }
            for (int u=0;u<5;u++){ p = put_int(p, pl->upg[u]); *p++=' '; *p=0; }
            p = put_int(p, pl->rollback); *p++=' '; *p=0;
            p = put_int(p, pl->kills); *p++=' '; *p=0;
            p = put_flt(p, pl->heaviest); *p++=' '; *p=0;
            p = put_int(p, pl->color); *p++=' '; *p=0;
            // the WHOLE hat saves: this cap used to be 200, so any
            // hat taller than 200 carcasses silently lost the rest
            // on save/Continue (Daniel's vanishing hat, Nov 10).
            // SAVE_CAP grew to fit a full 420-stack per player.
            int sn = pl->stack_n>MAXSTACK? MAXSTACK : pl->stack_n;
            p = put_int(p, sn); *p++=':'; *p=0;
            for (int s2=0;s2<sn;s2++){ p = put_int(p, (int)(pl->stack[s2]*2)); if (s2<sn-1){ *p++=','; *p=0; } }
            // mall-era extras, key= tokens after the stack (old saves
            // simply don't have them; the loader treats them as optional)
            if (pl->banked>0 || pl->hatstand || pl->in_mall || G.bud[i].active || pl->whale_own || pl->beans>0 || G.fs_pool[i]>0 || G.blg[i].games>0){
                Buddy *bd=&G.bud[i];
                *p++=' '; *p=0; p = put_key(p, "bk"); p = put_int(p, (int)(pl->banked*2));
                *p++=' '; *p=0; p = put_key(p, "hs"); p = put_int(p, pl->hatstand);
                *p++=' '; *p=0; p = put_key(p, "ml"); p = put_int(p, pl->in_mall);
                *p++=' '; *p=0; p = put_key(p, "ar"); p = put_int(p, pl->in_arcade);
                *p++=' '; *p=0; p = put_key(p, "wo"); p = put_int(p, pl->whale_own);
                *p++=' '; *p=0; p = put_key(p, "wn"); p = put_int(p, pl->whale_on);
                *p++=' '; *p=0; p = put_key(p, "bb"); p = put_int(p, pl->beans);
                if (G.fs_pool[i]>0){ *p++=' '; *p=0; p = put_key(p, "fp"); p = put_int(p, G.fs_pool[i]); }
                // BUG BOWL LEAGUE (v1.25 feat 10): the league book,
                // Player-level (the DEV bowls — no buddy required),
                // written only once the book has a game in it, so a
                // league-less save stays byte-identical to before.
                if (G.blg[i].games>0){
                    BowlLeague *L=&G.blg[i];
                    *p++=' '; *p=0; p = put_key(p, "bl");
                    p = put_int(p, L->games); *p++=','; *p=0; p = put_int(p, L->best); *p++=','; *p=0;
                    p = put_int(p, L->pins); *p++=','; *p=0; p = put_int(p, L->strikes); *p++=','; *p=0;
                    p = put_int(p, L->season); *p++=','; *p=0; p = put_int(p, L->night); *p++=','; *p=0;
                    p = put_int(p, L->spins); *p++=','; *p=0; p = put_int(p, L->trophies); *p++=','; *p=0;
                    p = put_int(p, L->last_score); *p++=','; *p=0; p = put_int(p, L->last_place); *p++=','; *p=0;
                    p = put_int(p, L->last_night); *p++=','; *p=0; p = put_int(p, L->last_season); *p++=','; *p=0;
                    p = put_int(p, L->fin_place); *p++=','; *p=0; p = put_int(p, L->fin_season);
                }
                if (bd->active){
                    *p++=' '; *p=0; p = put_key(p, "bd"); p = put_int(p, bd->species+1);
                    *p++=' '; *p=0; p = put_key(p, "bx"); p = put_int(p, bd->xp);
                    *p++=' '; *p=0; p = put_key(p, "b1"); p = put_int(p, bd->up_hp);
                    *p++=' '; *p=0; p = put_key(p, "b2"); p = put_int(p, bd->up_atk);
                    *p++=' '; *p=0; p = put_key(p, "b3"); p = put_int(p, bd->up_def);
                    *p++=' '; *p=0; p = put_key(p, "bs"); p = put_int(p, bd->skills);
                    *p++=' '; *p=0; p = put_key(p, "be"); p = put_int(p, (int)bd->energy);
                    *p++=' '; *p=0; p = put_key(p, "gx"); p = put_int(p, (int)bd->bw_x);
                    *p++=' '; *p=0; p = put_key(p, "gy"); p = put_int(p, (int)bd->bw_y);
                    *p++=' '; *p=0; p = put_key(p, "gz"); p = put_int(p, bd->bw_seed);
                    *p++=' '; *p=0; p = put_key(p, "gf"); p = put_int(p, bd->bw_started);
                    *p++=' '; *p=0; p = put_key(p, "bf"); p = put_int(p, bd->fainted);
                    *p++=' '; *p=0; p = put_key(p, "bn"); p = put_int(p, bd->name_idx);
                    *p++=' '; *p=0; p = put_key(p, "bc"); p = put_int(p, bd->color);
                    *p++=' '; *p=0; p = put_key(p, "bnv"); p = put_int(p, 2);
                    *p++=' '; *p=0; p = put_key(p, "sm"); p = put_int(p, bd->story_started);
                    *p++=' '; *p=0; p = put_key(p, "sq"); p = put_int(p, bd->story_bits);
                    *p++=' '; *p=0; p = put_key(p, "sq2"); p = put_int(p, bd->story_bits2);
                    *p++=' '; *p=0; p = put_key(p, "er"); p = put_int(p, bd->ent_best + bd->ent_done*10000);
                    *p++=' '; *p=0; p = put_key(p, "sb"); p = put_int(p, bd->ent_sub_best);
                    // DEEPER DOORS (v1.25 feat 11): Chapter 2's records
                    // in the er packing idiom, written only once a
                    // Deeper run has reached anywhere, so a
                    // Chapter-1-only save stays byte-identical.
                    if (bd->ent2_best>0 || bd->ent2_done>0){ *p++=' '; *p=0; p = put_key(p, "e2"); p = put_int(p, bd->ent2_best + bd->ent2_done*10000); }
                    *p++=' '; *p=0; p = put_key(p, "pd"); p = put_int(p, bd->plat_done);
                    *p++=' '; *p=0; p = put_key(p, "td"); p = put_int(p, bd->tank_done);
                    *p++=' '; *p=0; p = put_key(p, "cb"); p = put_int(p, bd->cas_best);
                    // CASTLE SHUFFLE (v1.25 feat 7): the shuffle-run
                    // counter, written only once the first shuffle is
                    // dealt, so a classic-only save is unchanged.
                    if (bd->cas_shufn>0){ *p++=' '; *p=0; p = put_key(p, "cs"); p = put_int(p, bd->cas_shufn); }
                    *p++=' '; *p=0; p = put_key(p, "cp"); p = put_int(p, bd->crt_best_pos);
                    *p++=' '; *p=0; p = put_key(p, "cq"); p = put_int(p, bd->crt_best_time);
                    *p++=' '; *p=0; p = put_key(p, "cr"); p = put_int(p, bd->crt_cupbest);
                    // MEDAL TIMES (v1.25 feat 3): per-track / per-level
                    // best times ride as compound tokens in the cw/cd
                    // idiom, written ONLY when some entry is set, so a
                    // medal-less save stays byte-identical to before.
                    { int anymc=0, anymr=0, anymd=0;
                      for (int ti=0; ti<8; ti++) if (bd->crt_tbest[ti]>0) anymc=1;
                      for (int ti=0; ti<8; ti++) if (bd->crt_medal_best[ti]>0) anymd=1;
                      for (int ti=0; ti<16; ti++) if (bd->plat_tbest[ti]>0) anymr=1;
                      if (anymc){ *p++=' '; *p=0; p = put_key(p, "mc");
                          for (int ti=0; ti<8; ti++){ if (ti) *p++=','; p = put_int(p, bd->crt_tbest[ti]); } }
                      // v1.27: the EARNED Cart medals ride as md. It is
                      // written whenever mc or md has anything to say,
                      // so a save WITHOUT md is provably pre-v1.27 and
                      // the loader migrates its medals from the times.
                      if (anymc || anymd){ *p++=' '; *p=0; p = put_key(p, "md");
                          for (int ti=0; ti<8; ti++){ if (ti) *p++=','; p = put_int(p, bd->crt_medal_best[ti]); } }
                      if (anymr){ *p++=' '; *p=0; p = put_key(p, "mr");
                          for (int ti=0; ti<16; ti++){ if (ti) *p++=','; p = put_int(p, bd->plat_tbest[ti]); } } }
                    *p++=' '; *p=0; p = put_key(p, "wr"); p = put_int(p, bd->war_prog);
                    // MORDOR FUN RUN (v1.28): the records ride as mo/mb
                    // and the in-progress run as one compound mu token
                    // (on,dist,mile,hp,secs,lat — the bl/pt idiom),
                    // written only once the mode has anything to say,
                    // so a Mordor-less save stays byte-identical.
                    if (bd->mor_done>0 || bd->mor_best>0 || bd->mor_on){
                        *p++=' '; *p=0; p = put_key(p, "mo"); p = put_int(p, bd->mor_done);
                        *p++=' '; *p=0; p = put_key(p, "mb"); p = put_int(p, bd->mor_best);
                        *p++=' '; *p=0; p = put_key(p, "mu");
                        p = put_int(p, bd->mor_on); *p++=','; *p=0;
                        p = put_int(p, bd->mor_dist); *p++=','; *p=0;
                        p = put_int(p, bd->mor_mile); *p++=','; *p=0;
                        p = put_int(p, bd->mor_hp); *p++=','; *p=0;
                        p = put_int(p, bd->mor_secs); *p++=','; *p=0;
                        p = put_int(p, bd->mor_lat);
                    }
                    // HAT TOPPERS (v1.25 feat 5): the earned mask and
                    // the worn id, written only when set, so a
                    // topper-less save stays byte-identical to before.
                    if (bd->top_earned){ *p++=' '; *p=0; p = put_key(p, "tp"); p = put_int(p, bd->top_earned); }
                    if (bd->top_worn){ *p++=' '; *p=0; p = put_key(p, "tw"); p = put_int(p, bd->top_worn); }
                    // HAT PORTRAIT BOOTH (v1.25 feat 9): the backdrop
                    // dial + the frozen portrait, written only when
                    // either exists, so a portrait-less save stays
                    // byte-identical to before. The frozen stack
                    // rides as one digit per carcass (its drawn kind)
                    // after the colon — the cw/cd compound idiom.
                    if (portrait[i].has || port_bd[i]!=0){
                        Portrait *pt2=&portrait[i];
                        *p++=' '; *p=0; p = put_key(p, "pt");
                        p = put_int(p, port_bd[i]); *p++=','; *p=0;
                        p = put_int(p, pt2->has); *p++=','; *p=0;
                        p = put_int(p, pt2->backdrop); *p++=','; *p=0;
                        p = put_int(p, pt2->hat_on); *p++=','; *p=0;
                        p = put_int(p, pt2->top_worn); *p++=','; *p=0;
                        p = put_int(p, pt2->kg); *p++=','; *p=0;
                        p = put_int(p, pt2->stack_n); *p++=','; *p=0;
                        p = put_int(p, pt2->species); *p++=','; *p=0;
                        p = put_int(p, pt2->bud_color); *p++=','; *p=0;
                        p = put_int(p, pt2->name_idx); *p++=','; *p=0;
                        p = put_int(p, pt2->pcolor); *p++=','; *p=0;
                        p = put_int(p, pt2->whale_on); *p++=':'; *p=0;
                        for (int ki=0; ki<pt2->stack_n && ki<MAXSTACK; ki++) *p++=(char)('0' + (pt2->kinds[ki]&3));
                        *p=0;
                    }
                    // WAR! battle snapshot (v1.17): a parked snapshot
                    // rides verbatim; a LIVE battle serializes fresh
                    // into the snapshot buffer first, so quitting the
                    // app mid-battle keeps the battle too. (Via a hook:
                    // BWS itself is defined far below the save code.)
                    { const char *wsnap = war_snap_for_save(i);
                      if (wsnap){ *p++=' '; *p=0; p = sappend(p, wsnap); } }
                    // WAR! skirmish snapshot (v1.29): the second
                    // park's text is written natively with k-prefixed
                    // keys (kb..kh), so the loader rebuilds the two
                    // parks independently; it rides verbatim too.
                    { const char *sksnap = war_snap_sk_for_save(i);
                      if (sksnap){ *p++=' '; *p=0; p = sappend(p, sksnap); } }
                    *p++=' '; *p=0; p = put_key(p, "cx"); p = put_int(p, (int)bd->craft_x);
                    *p++=' '; *p=0; p = put_key(p, "cy"); p = put_int(p, (int)bd->craft_y);
                    *p++=' '; *p=0; p = put_key(p, "cm"); p = put_int(p, bd->craft_wood + bd->craft_stone*1000 + bd->craft_fiber*1000000);
                    *p++=' '; *p=0; p = put_key(p, "cn"); p = put_int(p, bd->craft_car + bd->craft_pies*1000 + bd->craft_stews*1000000);
                    *p++=' '; *p=0; p = put_key(p, "cg"); p = put_int(p, bd->craft_gear);
                    *p++=' '; *p=0; p = put_key(p, "si"); p = put_int(p, bd->items[0] + bd->items[1]*10 + bd->items[2]*100 + bd->items[3]*1000);
                    *p++=' '; *p=0; p = put_key(p, "ct"); p = put_int(p, (int)(bd->craft_tod*10));
                    *p++=' '; *p=0; p = put_key(p, "cf"); p = put_int(p, bd->craft_fish);
                    if (bd->craft_bn>0){   // placed builds ride as one compound token: tx,ty,kind,hp;tx,ty,...
                        *p++=' '; *p=0; p = put_key(p, "cw");
                        for (int bi=0;bi<bd->craft_bn;bi++){
                            if (bi) { *p++=';'; *p=0; }
                            p = put_int(p, bd->craft_btx[bi]); *p++=','; *p=0;
                            p = put_int(p, bd->craft_bty[bi]); *p++=','; *p=0;
                            p = put_int(p, bd->craft_bk[bi]); *p++=','; *p=0;
                            p = put_int(p, bd->craft_bhp[bi]);
                        }
                    }
                    if (bd->craft_gn>0){   // mined-out node keys, ';'-joined
                        *p++=' '; *p=0; p = put_key(p, "cd");
                        for (int gi=0;gi<bd->craft_gn;gi++){
                            if (gi) { *p++=';'; *p=0; }
                            p = put_int(p, bd->craft_gone[gi]);
                        }
                    }
                }
            }
            *p++='\n'; *p=0;
        }
        int wn=0; for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1) wn++;
        p = put_key(p, "walls"); p = put_int(p, wn); *p++='\n'; *p=0;
        for (int i=0;i<MAXWALLS && wn>0;i++) if (G.walls[i].alive==1){
            Wall *w=&G.walls[i];
            p = put_key(p, "wl"); *p++=' '; *p=0;
            p = put_flt(p, w->x); *p++=' '; *p=0;
            p = put_flt(p, w->y); *p++=' '; *p=0;
            p = put_flt(p, w->hp); *p++=' '; *p=0;
            p = put_flt(p, w->maxhp); *p++=' '; *p=0;
            p = put_int(p, w->horiz); *p++=' '; *p=0;
            p = put_int(p, w->stack>0? w->stack : 1); *p++='\n'; *p=0;
        }
    }
    if (runstart){
        // the run goes to its slot (with a one-line summary the slot
        // picker can show); the global blob keeps records + settings.
        char *q = slotbuf;
        q = sappend(q, "TSG3\n");
        q = put_key(q, "sum"); *q++=' '; *q=0;
        q = sappend(q, G.p[0].name); *q++=' '; *q=0;
        q = put_int(q, (int)G.p[0].heaviest); *q++=' '; *q=0;
        q = put_int(q, G.total_kills); *q++=' '; *q=0;
        q = put_int(q, (int)G.time); *q++=' '; *q=0;
        q = put_int(q, G.zen_run); *q++=' '; *q=0;
        // whose run is this, at a glance (Daniel): the buddy rides the
        // summary too, so slots can be told apart in the picker.
        if (G.bud[0].active){ q = put_int(q, G.bud[0].name_idx); *q++=' '; *q=0; q = put_int(q, G.bud[0].species); }
        else { q = put_int(q, -1); *q++=' '; *q=0; q = put_int(q, -1); }
        *q++='\n'; *q=0;
        int rn = (int)(p - runstart);
        for (int i=0;i<rn;i++) *q++ = runstart[i];
        *q = 0;
        if (G.slot<1 || G.slot>3) G.slot = 1;
        plat_save_slot(G.slot, slotbuf, (int)(q - slotbuf));
        G.slot_has[G.slot] = 1; refresh_has_save();
        plat_save(savebuf, (int)(runstart - savebuf));
    } else {
        plat_save(savebuf, (int)(p - savebuf));
    }
}
static const char *line_val(const char *buf, const char *key){
    // returns pointer just past "key=" at a line start, or 0
    int kl = slen(key);
    const char *s = buf;
    while (*s){
        if (s[0]==key[0]){
            int ok=1; for (int i=0;i<kl;i++) if (s[i]!=key[i]){ ok=0; break; }
            if (ok && s[kl]=='=') return s+kl+1;
        }
        while (*s && *s!='\n') s++;
        if (*s=='\n') s++;
    }
    return 0;
}
static void save_records(void){
    // update all-time records from the current run, then persist
    // everything. Zen runs don't set records: the hats are enormous.
    if (!G.zen_run){
        for (int i=0;i<MAXP;i++) if (G.p[i].connected){
            if (G.p[i].heaviest > G.rec_hat){
                G.rec_hat = G.p[i].heaviest;
                scpy(G.rec_name, G.p[i].name, 13);
            }
        }
        if (G.total_kills > G.rec_kills) G.rec_kills = G.total_kills;
        if (G.time > G.rec_time) G.rec_time = G.time;
    }
    // HAT TOPPERS (feat 5): the earned set derives from the records
    // above, so refresh it in the same breath — bits and the records
    // they came from persist in the same write.
    for (int i=0;i<MAXP;i++) if (G.p[i].connected) topper_refresh(i, 0);
    do_save();
}

