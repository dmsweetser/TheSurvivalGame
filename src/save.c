// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: save.c — save / load, run setup + continue, save slots, the networking packet format, store purchases, mall station panels.
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]
// ---------------------------------------------------------------------------
// Save / load — a small line-based text format in one block.
// ---------------------------------------------------------------------------
#define SAVE_CAP 65536
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
#define WAR_SNAP_CAP 12288
static char war_snap[MAXP][WAR_SNAP_CAP];
static int war_snap_on[MAXP];
static int war_snap_reg[MAXP] = { -1, -1, -1, -1 };   // v1.18: WHICH region the parked battle belongs to — the map needs it to offer a resume instead of a restart
static void war_snap_clear(int pi){ war_snap_on[pi]=0; war_snap[pi][0]=0; war_snap_reg[pi]=-1; }
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
                    { int anymc=0, anymr=0;
                      for (int ti=0; ti<8; ti++) if (bd->crt_tbest[ti]>0) anymc=1;
                      for (int ti=0; ti<16; ti++) if (bd->plat_tbest[ti]>0) anymr=1;
                      if (anymc){ *p++=' '; *p=0; p = put_key(p, "mc");
                          for (int ti=0; ti<8; ti++){ if (ti) *p++=','; p = put_int(p, bd->crt_tbest[ti]); } }
                      if (anymr){ *p++=' '; *p=0; p = put_key(p, "mr");
                          for (int ti=0; ti<16; ti++){ if (ti) *p++=','; p = put_int(p, bd->plat_tbest[ti]); } } }
                    *p++=' '; *p=0; p = put_key(p, "wr"); p = put_int(p, bd->war_prog);
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

// ---------------------------------------------------------------------------
// Run setup + continue-from-save
// ---------------------------------------------------------------------------
static void player_fresh(Player *p, const char *name, int color, float x, float y){
    int was_conn = p->connected;
    smemset(p, 0, sizeof(*p));
    p->connected = was_conn; p->alive = 1;
    scpy(p->name, name[0]? name : "DEV", 13);
    p->x=x; p->y=y; p->aim=-PI/2; p->cur_aim=-PI/2;
    p->maxhp=100; p->hp=100; p->weapon=0; p->owned[0]=1;
    p->dmg_mul=1; p->rate_mul=1; p->color=color;
    p->on_wall=-1; p->station=-1;
}
static void reset_world(void){
    smemset(G.bugs, 0, sizeof(G.bugs));
    smemset(G.projs, 0, sizeof(G.projs));
    smemset(G.walls, 0, sizeof(G.walls));
    for (int i=0;i<MAXWALLS;i++){ G.walls[i].grp=-1; G.walls[i].grp_old=-1; grp_n[i]=0; grp_pool[i]=0; grp_cap[i]=0; grp_dsb[i]=0; }
    smemset(G.parts, 0, sizeof(G.parts));
    smemset(G.floats, 0, sizeof(G.floats));
    smemset(G.fx, 0, sizeof(G.fx));
    smemset(G.bud, 0, sizeof(G.bud));
    smemset(G.bowl, 0, sizeof(G.bowl));
    smemset(G.blg, 0, sizeof(G.blg));   // BUG BOWL LEAGUE (feat 10): a fresh world starts a fresh book; Continue re-applies it from the pl-lines
    smemset(wall_build_n, 0, sizeof(wall_build_n));
    G.time=0; G.total_kills=0; G.total_earned=0;
    G.spawn_t=1; G.autosave_t=5; pay_active=0;
    rng_seed(0xC0FFEEu ^ (u32)(plat_now()*1000));
}
// local co-op store sessions (0 = P1 browsing, 1 = P2 browsing)
static int lstore[2];
static int help_page=0;   // HOW TO PLAY flips to a second page for the mall & friends
static int lstation[2];   // which mall station each shopper is browsing
static void coop_reset_views(void){ lstore[0]=lstore[1]=0; lstation[0]=lstation[1]=0; }
// local co-op: bring in player 2 (second controller, same machine).
// The run still lives in ONE save slot — P2 is saved and restored as a
// player line in the same blob, exactly like a LAN guest would be, and
// claimed back by name on load. P2 drops in beside P1.
static void coop_spawn_p2(float x, float y){
    if (G.p2_slot>=0) return;
    for (int i=1;i<MAXP;i++) if (!G.p[i].connected){
        player_fresh(&G.p[i], "P2", 1, x, y);
        G.p[i].connected=1;
        G.p[i].whale_own=G.whale_own; G.p[i].whale_on=G.whale_on;
        G.p2_slot=i;
        return;
    }
}
// ---------------------------------------------------------------------------
// FIRST SHIFT (v1.25) — the guided first-session chain: six small
// objectives the first time a slot is started, each paying buddy XP
// through the one shared door (buddy_award_xp). Conditions poll live
// state every frame, so anything the player already did counts the
// moment its step comes up. Full design + save notes: v125-spec.md.
// ---------------------------------------------------------------------------
static int fs_fingerprint(void){
    // everything the shop till can grant, summed over the crew —
    // weapon copies past the free starter, upgrade levels, the hat
    // stand, beans, adopted buddies. A fresh crew sums to 0, and a
    // co-op partner dropping in with a starter keyboard adds 0 too.
    int fp = 0;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        Player *p=&G.p[i];
        fp += (p->owned[0]>0? p->owned[0]-1 : 0);
        for (int w=1;w<NWEAP;w++) fp += p->owned[w];
        for (int u=0;u<5;u++) fp += p->upg[u];
        fp += p->hatstand + p->beans + (G.bud[i].active? 1:0);
    }
    return fp;
}
static int fs_step_done(int step){
    switch (step){
        case 0: return G.total_kills>=1;
        case 1: for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].carried>=10.0f) return 1; return 0;
        case 2: for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1) return 1; return 0;
        case 3: return fs_fingerprint() > G.fs_fp0;
        case 4: for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].banked>0.0f) return 1; return 0;
        case 5: for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.bud[i].in_bw) return 1; return 0;
    }
    return 0;
}
static void fs_update(float dt){
    // the banner only burns down where it can be seen (GS_PLAY), so
    // a step finished inside a station panel still gets its moment
    if (fs_flash_t>0 && G.state==GS_PLAY) fs_flash_t -= dt;
    if (G.fs_state!=1 || G.net_mode==NET_CLIENT) return;
    if (G.state!=GS_PLAY && G.state!=GS_STORE) return;
    int step = G.fs_step;
    if (step<0 || step>5 || !fs_step_done(step)) return;
    int amt = FS_XP[step];
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        if (G.bud[i].active) buddy_award_xp(i, amt);
        else G.fs_pool[i] += amt;      // tuition: lands on adoption
    }
    { char b[24]; char *q=b; q=sappend(q, "FIRST SHIFT +"); q=sapp_int(q, amt); q=sappend(q, " XP"); *q=0;
      Player *me=&G.p[G.my_id>=0? G.my_id : 0];
      spawn_floater(me->x, me->y-30, b, COL(150,255,190)); }
    sfx(11);
    fs_flash_amt = amt;
    fs_flash_fin = (step==5);
    fs_flash_t = 3.5f;
    if (step>=5){ G.fs_state=2; G.fs_step=6; }
    else G.fs_step = step+1;
    if (G.autosave_t>0.5f) G.autosave_t = 0.5f;   // persist promptly
}
static void fs_skip(void){
    if (G.fs_state!=1) return;
    G.fs_state=3; fs_flash_t=0;
    do_save();
}
// ---------------------------------------------------------------------------
// WEEKLY CHALLENGE (v1.25 feature 6) — one goal a week, identical for
// every player in that week because it is a pure function of the week
// id, drawn from counters the game already keeps. The board (Mall
// station 6) is the card; this is the bookkeeping: week math, the
// challenge table, baseline snapshots for the delta sources, the
// completion poll and the payout. Full design: v125-feat6-spec.md.
// ---------------------------------------------------------------------------
#define WK_XP 50   // one week-sized job: First Shift pays 65 for six small steps
enum { WK_T_KILLS, WK_T_HAT, WK_T_BANK, WK_T_WAR, WK_T_MEDAL,
       WK_T_TANK, WK_T_QUEST, WK_T_SHOP, WK_T_CASH, WK_T_WALLS, WK_NTYPES };
enum { WK_S_KILLS, WK_S_WAR, WK_S_MEDAL, WK_S_TANK, WK_S_QUEST, WK_S_SHOP, WK_S_CASH };
static const char *WK_NAME[WK_NTYPES] = {
    "BUG SQUASHER", "BIG HAT WEEK", "BANK HOLIDAY", "WAR EFFORT", "MEDAL HUNT",
    "TANK TROOP", "STORY TIME", "MALL RATS", "CASH IN", "WALL BUILDERS"
};
// the perceived day, for tests only (dbg_wksetday): -1 = the device
// clock. Runtime state — it never reaches a save.
static int wk_day_override = -1;
static void wk_test_set_day(int d){ wk_day_override = d; }
static int wk_week_now(void){
    int d = wk_day_override>=0? wk_day_override : plat_daynum();
    return (d + 3) / 7;   // epoch day 0 was a Thursday; +3 starts blocks on Mondays
}
static void wk_challenge(int week, int *type, int *target){
    static const int NVAR[WK_NTYPES] = { 3,3,3,2,2,2,1,2,2,2 };
    static const int VAR[WK_NTYPES][3] = {
        {30,50,80}, {40,60,100}, {50,100,200}, {1,2,0}, {1,2,0},
        {2,3,0}, {1,0,0}, {1,3,0}, {100,250,0}, {4,6,0}
    };
    // hash2, never the gameplay RNG: the derivation must be identical
    // on every install in the same week, and the RNG is clock-seeded.
    u32 h = hash2(week, 7919, 0x9E3779B9u);
    int t = (int)(h % (u32)WK_NTYPES);
    u32 h2 = hash2(week, 104729, 0x85EBCA6Bu);
    *type = t;
    *target = VAR[t][(int)(h2 % (u32)NVAR[t])];
}
// the current value of a delta source. Per-buddy sources read the
// crew best (max over connected players) — the First Shift rule that
// any player's doing it counts; run totals are shared by construction.
static int wk_src_val(int src){
    int best = 0;
    switch (src){
        case WK_S_KILLS: return G.total_kills;
        case WK_S_SHOP: return fs_fingerprint();
        case WK_S_CASH: return (int)G.total_earned;
        case WK_S_WAR:
            for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.bud[i].war_prog>best) best = G.bud[i].war_prog;
            return best;
        case WK_S_TANK:
            for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.bud[i].tank_done>best) best = G.bud[i].tank_done;
            return best;
        case WK_S_MEDAL:
            for (int i=0;i<MAXP;i++) if (G.p[i].connected){
                Buddy *b=&G.bud[i];
                int n = crt_medal_count(b,1)+crt_medal_count(b,2)+crt_medal_count(b,3)
                      + plat_medal_count(b,1)+plat_medal_count(b,2)+plat_medal_count(b,3);
                if (n>best) best = n;
            }
            return best;
        case WK_S_QUEST:
            // quests 0-13 (feat 14): the task says "FINISH A STORY
            // QUEST" and the Rebuilding's quests are story quests;
            // the week-start baseline keeps pre-finished ones honest
            for (int i=0;i<MAXP;i++) if (G.p[i].connected){
                int n=0; for (int q=0;q<14;q++) if (bw_qstate_g(&G.bud[i],q)==2) n++;
                if (n>best) best = n;
            }
            return best;
    }
    return 0;
}
static void wk_snapshot(void){
    for (int s=0;s<7;s++) G.wk_base[s] = wk_src_val(s);
}
static int wk_progress(void){
    int type, target; wk_challenge(G.wk_week, &type, &target);
    int v = 0;
    switch (type){
        case WK_T_KILLS: v = G.total_kills - G.wk_base[WK_S_KILLS]; break;
        case WK_T_WAR:   v = wk_src_val(WK_S_WAR)   - G.wk_base[WK_S_WAR]; break;
        case WK_T_MEDAL: v = wk_src_val(WK_S_MEDAL) - G.wk_base[WK_S_MEDAL]; break;
        case WK_T_TANK:  v = wk_src_val(WK_S_TANK)  - G.wk_base[WK_S_TANK]; break;
        case WK_T_QUEST: v = wk_src_val(WK_S_QUEST) - G.wk_base[WK_S_QUEST]; break;
        case WK_T_SHOP:  v = fs_fingerprint()       - G.wk_base[WK_S_SHOP]; break;
        case WK_T_CASH:  v = (int)G.total_earned    - G.wk_base[WK_S_CASH]; break;
        case WK_T_HAT:
            for (int i=0;i<MAXP;i++) if (G.p[i].connected && (int)G.p[i].carried>v) v = (int)G.p[i].carried;
            break;
        case WK_T_BANK:
            for (int i=0;i<MAXP;i++) if (G.p[i].connected && (int)G.p[i].banked>v) v = (int)G.p[i].banked;
            break;
        case WK_T_WALLS:
            for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1) v++;
            break;
    }
    return v<0? 0 : v;
}
// open the current week for this slot: the outgoing week becomes the
// board's PREVIOUS row (with its done flag), the new week starts with
// fresh baselines, and "done" is derived so the new week is unpaid.
static void wk_rollover(void){
    int now = wk_week_now();
    if (G.wk_week==now) return;
    if (G.wk_week!=0){
        G.wk_prev_week = G.wk_week;
        G.wk_prev_done = (G.wk_done_week==G.wk_week)? 1 : 0;
    }
    G.wk_week = now;
    wk_snapshot();
}
static float wk_roll_t = 0;
static void wk_update(float dt){
    if (wk_flash_t>0 && G.state==GS_PLAY) wk_flash_t -= dt;
    if (G.net_mode==NET_CLIENT || G.wk_week==0) return;   // the host owns the week (fs_update's rule)
    if (G.state!=GS_PLAY && G.state!=GS_STORE) return;
    wk_roll_t -= dt;
    if (wk_roll_t<=0){   // a session left running over midnight rolls too
        wk_roll_t = 1.0f;
        int before = G.wk_week;
        wk_rollover();
        if (G.wk_week!=before){
            wk_flash_kind = 2; wk_flash_t = 3.5f;
            if (G.autosave_t>0.5f) G.autosave_t = 0.5f;   // persist promptly
        }
    }
    if (G.wk_done_week==G.wk_week) return;
    int type, target; wk_challenge(G.wk_week, &type, &target);
    if (wk_progress() < target) return;
    // complete: pay every connected player's buddy through the one
    // shared door; a player with no buddy yet banks it in the First
    // Shift tuition pool, which buddy_adopt flushes on adoption.
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        if (G.bud[i].active) buddy_award_xp(i, WK_XP);
        else G.fs_pool[i] += WK_XP;
    }
    { char b[24]; char *q=b; q=sappend(q, "WEEKLY +"); q=sapp_int(q, WK_XP); q=sappend(q, " XP"); *q=0;
      Player *me=&G.p[G.my_id>=0? G.my_id : 0];
      spawn_floater(me->x, me->y-30, b, COL(150,255,190)); }
    sfx(11);
    G.wk_done_week = G.wk_week;
    G.wk_total++;
    wk_flash_kind = 1; wk_flash_t = 3.5f;
    if (G.autosave_t>0.5f) G.autosave_t = 0.5f;   // persist promptly
}
static char *wk_task(int type, int target, char *buf){
    char *q=buf;
    switch (type){
        case WK_T_KILLS: q=sappend(q, "SQUASH "); q=sapp_int(q, target); q=sappend(q, " BUGS THIS WEEK."); break;
        case WK_T_HAT:   q=sappend(q, "WEAR A HAT OF "); q=sapp_int(q, target); q=sappend(q, " KG OR MORE."); break;
        case WK_T_BANK:  q=sappend(q, "HOLD "); q=sapp_int(q, target); q=sappend(q, " KG IN THE BUG BANK."); break;
        case WK_T_WAR:   q=sappend(q, "CONQUER "); q=sapp_int(q, target); q=sappend(q, target==1? " WAR! REGION." : " WAR! REGIONS."); break;
        case WK_T_MEDAL: q=sappend(q, "EARN "); q=sapp_int(q, target); q=sappend(q, target==1? " NEW MEDAL IN CART OR RUN." : " NEW MEDALS IN CART OR RUN."); break;
        case WK_T_TANK:  q=sappend(q, "CLEAR "); q=sapp_int(q, target); q=sappend(q, " TANK RESCUE LEVELS."); break;
        case WK_T_QUEST: q=sappend(q, "FINISH A STORY QUEST IN BANZAI WORLD."); break;
        case WK_T_SHOP:  q=sappend(q, "BUY "); q=sapp_int(q, target); q=sappend(q, target==1? " THING AT THE MALL." : " THINGS AT THE MALL."); break;
        case WK_T_CASH:  q=sappend(q, "CASH IN "); q=sapp_int(q, target); q=sappend(q, " KG AT THE COUNTER."); break;
        case WK_T_WALLS: q=sappend(q, "STAND "); q=sapp_int(q, target); q=sappend(q, " CARCASS WALLS AT ONCE."); break;
        default: q=sappend(q, "..."); break;
    }
    *q=0; return q;   // the END pointer: wk_row_blurb appends the pay line
}
// the board's row model (Mall station 6): one helper feeds the row
// count, the names, the right-hand values and the blurbs, so the
// panel can never disagree with itself (the Trophy Case rule).
enum { WK_HDR_WEEK, WK_CHAL, WK_HDR_LEDGER, WK_TOTAL, WK_PREV };
static int wk_row_ids(int *ids){
    int n=0;
    ids[n++]=WK_HDR_WEEK;
    ids[n++]=WK_CHAL;
    if (G.net_mode==NET_CLIENT) return n;   // a guest reads the card; the host keeps the book
    ids[n++]=WK_HDR_LEDGER;
    ids[n++]=WK_TOTAL;
    if (G.wk_prev_week) ids[n++]=WK_PREV;
    return n;
}
static int wk_row_id(int row){
    int ids[8]; int n=wk_row_ids(ids);
    return (row>=0 && row<n)? ids[row] : -1;
}
static int wk_rows(void){ int ids[8]; return wk_row_ids(ids); }
static const char *wk_row_name(int row, char *buf){
    char *q=buf;
    switch (wk_row_id(row)){
        case WK_HDR_WEEK: return "THIS WEEK";
        case WK_CHAL: { int t,tg; wk_challenge(G.wk_week, &t, &tg); return WK_NAME[t]; }
        case WK_HDR_LEDGER: return "THE LEDGER";
        case WK_TOTAL: return "CHALLENGES DONE";
        case WK_PREV: { int t,tg; wk_challenge(G.wk_prev_week, &t, &tg);
                        q=sappend(q, "PREVIOUS: "); q=sappend(q, WK_NAME[t]); *q=0; return buf; }
        default: return "";
    }
}
static const char *wk_row_right(int row, char *buf){
    char *q=buf;
    switch (wk_row_id(row)){
        case WK_CHAL: {
            if (G.net_mode==NET_CLIENT) return "HOST'S BOARD";
            int t,tg; wk_challenge(G.wk_week, &t, &tg);
            if (G.wk_done_week==G.wk_week) return "DONE!";
            q=sapp_int(q, wk_progress()); q=sappend(q, "/"); q=sapp_int(q, tg); *q=0; return buf; }
        case WK_TOTAL: q=sapp_int(q, G.wk_total); *q=0; return buf;
        case WK_PREV: return G.wk_prev_done? "DONE" : "MISSED";
        default: return "";
    }
}
static char wk_blurb_buf[160];
static const char *wk_row_blurb(int row){
    char *q;
    switch (wk_row_id(row)){
        case WK_HDR_WEEK: return "ONE FOR EVERYBODY, EVERY WEEK. MISS ONE AND NOTHING IS LOST.";
        case WK_CHAL: {
            if (G.net_mode==NET_CLIENT) return "THE HOST KEEPS THIS WEEK'S BOARD. YOUR OWN SLOT HAS ONE TOO.";
            int t,tg; wk_challenge(G.wk_week, &t, &tg);
            q=wk_task(t, tg, wk_blurb_buf);
            if (G.wk_done_week==G.wk_week) q=sappend(q, " DONE - PAID AND POSTED.");
            else { q=sappend(q, " PAYS "); q=sapp_int(q, WK_XP); q=sappend(q, " BUDDY XP WHEN IT LANDS."); }
            *q=0; return wk_blurb_buf; }
        case WK_HDR_LEDGER: return "THE BOOK OF WEEKS. IT ONLY EVER COUNTS THE FINISHES.";
        case WK_TOTAL: return "EVERY WEEK YOU FINISH, COUNTED HERE. NO STREAKS. NO GUILT.";
        case WK_PREV: return G.wk_prev_done? "LAST TIME OUT, AND YOU DID IT. THE BOARD REMEMBERS KINDLY."
                                           : "UNFINISHED LAST WEEK. IT COST NOTHING. THERE IS ALWAYS THIS WEEK.";
        default: return "";
    }
}
// ---------------------------------------------------------------------------
// NAMED ELITES + BOUNTY BOARD (v1.25 feature 8) — the bookkeeping
// behind Mall station 7. The elite itself lives in buddies.c (the
// tables, the spawn elevation, the kill hand-off); this is the
// contract: the claim settlement, the cooldown, the live tracking
// for the board's LAST SEEN row, and the panel's row model. Full
// design: v125-feat8-spec.md.
// ---------------------------------------------------------------------------
// settle a pending claim: the killer's bank gains the elite's weight
// again, every connected buddy gains BN_XP through the shared door
// (or the tuition pool), the ledger and the serial advance, and the
// cooldown starts. Called only from bn_update, never from combat.
static void bn_settle_claim(void){
    int pi = G.bn_claim_pi;
    if (pi<0 || pi>=MAXP || !G.p[pi].connected){
        pi = -1;
        for (int i=0;i<MAXP;i++) if (G.p[i].connected){ pi = i; break; }
    }
    if (pi>=0) G.p[pi].banked += G.bn_claim_w;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        if (G.bud[i].active) buddy_award_xp(i, BN_XP);
        else G.fs_pool[i] += BN_XP;
    }
    { char b[32]; char *q=b; q=sappend(q, "BOUNTY +"); q=sapp_int(q, (int)G.bn_claim_w); q=sappend(q, " KG BANKED"); *q=0;
      Player *me=&G.p[G.my_id>=0? G.my_id : 0];
      spawn_floater(me->x, me->y-30, b, COL(255,214,120)); }
    sfx(11);
    sfx(25);   // feat 15: BOUNTY CLAIMED — the sting beside the till-jingle
    bn_flash_name = G.bn_claim_name;
    bn_flash_t = 3.5f;
    G.bn_done++;
    G.bn_serial++;
    G.bn_cool = BN_COOLDOWN;
    G.bn_claim_pending = 0;
    G.bn_last_reg = -1;   // a new contract: no sighting of THIS one yet
    if (G.autosave_t>0.5f) G.autosave_t = 0.5f;   // persist promptly
}
static void bn_update(float dt){
    if (bn_flash_t>0 && G.state==GS_PLAY) bn_flash_t -= dt;
    if (G.net_mode==NET_CLIENT) return;   // the host owns the bounty (wk_update's rule)
    if (G.state!=GS_PLAY && G.state!=GS_STORE) return;
    if (G.bn_cool>0) G.bn_cool -= dt;
    int claimed = G.bn_claim_pending;
    if (claimed) bn_settle_claim();
    // live tracking for the board and the pause line: where is the
    // elite right now — and if it vanished without a claim, it
    // slipped away (the 1750 px left-behind cull), so remember the
    // region it was last seen in. The contract itself never moves.
    int li = bn_any_live();
    if (li>=0){
        Bug *b=&G.bugs[li];
        float d = fsqrt(b->x*b->x + b->y*b->y);
        G.bn_live = 1; G.bn_live_reg = region_at(d);
    } else {
        if (G.bn_live && !claimed) G.bn_last_reg = G.bn_live_reg;
        G.bn_live = 0;
    }
}
// the board's row model (Mall station 7): one helper feeds the row
// count, the names, the right-hand values and the blurbs, so the
// panel can never disagree with itself (the Trophy Case rule).
enum { BN_HDR_WANTED, BN_BOUNTY, BN_SEEN, BN_HDR_LEDGER, BN_CLAIMED };
static int bn_row_ids(int *ids){
    int n=0;
    ids[n++]=BN_HDR_WANTED;
    ids[n++]=BN_BOUNTY;
    if (G.net_mode==NET_CLIENT) return n;   // a guest reads the poster; the host keeps the book
    ids[n++]=BN_SEEN;
    ids[n++]=BN_HDR_LEDGER;
    ids[n++]=BN_CLAIMED;
    return n;
}
static int bn_row_id(int row){
    int ids[8]; int n=bn_row_ids(ids);
    return (row>=0 && row<n)? ids[row] : -1;
}
static int bn_rows(void){ int ids[8]; return bn_row_ids(ids); }
static const char *bn_row_name(int row, char *buf){
    switch (bn_row_id(row)){
        case BN_HDR_WANTED: return "WANTED";
        case BN_BOUNTY: return ELITE_NAME[bn_name_idx(G.bn_serial)];
        case BN_SEEN: return "LAST SEEN";
        case BN_HDR_LEDGER: return "THE LEDGER";
        case BN_CLAIMED: return "BOUNTIES CLAIMED";
        default: return "";
    }
    (void)buf;
}
static const char *bn_row_right(int row, char *buf){
    char *q=buf;
    switch (bn_row_id(row)){
        case BN_BOUNTY:
            if (G.net_mode==NET_CLIENT) return "HOST'S DESK";
            return G.bn_live? "LOOSE!" : "AT LARGE";
        case BN_SEEN:
            if (G.bn_live){ q=sappend(q, REGION_NAME[G.bn_live_reg]); *q=0; return buf; }
            if (G.bn_last_reg>=0){ q=sappend(q, REGION_NAME[G.bn_last_reg]); *q=0; return buf; }
            return "NOWHERE YET";
        case BN_CLAIMED: q=sapp_int(q, G.bn_done); *q=0; return buf;
        default: return "";
    }
}
static char bn_blurb_buf[192];
static const char *bn_row_blurb(int row){
    char *q;
    switch (bn_row_id(row)){
        case BN_HDR_WANTED: return "ONE NAME AT A TIME. IT CRAWLS OUT WHEN IT PLEASES - MORE OFTEN FAR FROM HOME.";
        case BN_BOUNTY: {
            if (G.net_mode==NET_CLIENT) return "THE HOST KEEPS THE BOUNTIES. THE ELITE, THOUGH, IS REAL - WATCH FOR ITS NAME OVERHEAD.";
            int ni = bn_name_idx(G.bn_serial);
            q=bn_blurb_buf;
            q=sappend(q, ELITE_TITLE[ni]);
            q=sappend(q, ". A "); q=sappend(q, ELITE_KINDNAME[bn_kind_of(G.bn_serial)]);
            q=sappend(q, " THREE TIMES TOO BIG FOR ITS BOOTS. BRING IT DOWN: ITS WEIGHT AGAIN IN THE BANK FOR THE KILLER, ");
            q=sapp_int(q, BN_XP); q=sappend(q, " XP FOR EVERY BUDDY.");
            *q=0; return bn_blurb_buf; }
        case BN_SEEN:
            if (G.bn_live) return "IT IS OUT THERE NOW - FOLLOW THE AMBER ARROW.";
            if (G.bn_last_reg>=0) return "IT SLIPPED AWAY. THE CONTRACT STANDS - IT WILL CRAWL OUT AGAIN.";
            return "NO SIGHTING YET. THE BOARD ONLY KNOWS THE NAME.";
        case BN_HDR_LEDGER: return "THE BOOK OF NAMES. IT ONLY EVER COUNTS THE FALLEN.";
        case BN_CLAIMED: return "EVERY NAMED ONE BROUGHT DOWN, COUNTED HERE. THE NEXT NAME IS ALREADY CHALKED UP.";
        default: return "";
    }
}
static void start_run(int net_mode){
    G.edit_field = 0;   // a run never owns a menu text field (Nov 9)
    reset_world();
    for (int i=0;i<MAXP;i++){ G.p[i].connected=0; G.p[i].is_local=0; }
    G.net_mode = net_mode; G.my_id = 0;
    G.zen_run = G.zen;
    G.diff_run = G.difficulty;
    G.p2_slot=-1; coop_reset_views();
    player_fresh(&G.p[0], G.my_name, 0, 0, 70);
    G.p[0].connected=1; G.p[0].is_local=1;
    G.p[0].whale_own=G.whale_own; G.p[0].whale_on=G.whale_on;
    if (net_mode!=NET_CLIENT && G.coop_pref && IN.pad_count>=2) coop_spawn_p2(-30, 70);
    // FIRST SHIFT (v1.25): a genuinely fresh run arms the guided
    // chain — except for a LAN client, who doesn't own the run.
    G.fs_state = (net_mode!=NET_CLIENT)? 1 : 0;
    G.fs_step = 0;
    for (int i=0;i<MAXP;i++) G.fs_pool[i]=0;
    G.fs_fp0 = fs_fingerprint();
    fs_flash_t = 0;
    // WEEKLY CHALLENGE (v1.25 feature 6): a fresh slot joins the
    // current week with clean books — the ledger starts empty and
    // the baselines snapshot the fresh run's zeros. A LAN client
    // never tracks a week (the host owns it).
    G.wk_week = (net_mode!=NET_CLIENT)? wk_week_now() : 0;
    G.wk_done_week = 0; G.wk_total = 0; G.wk_prev_week = 0; G.wk_prev_done = 0;
    wk_snapshot();
    // NAMED ELITES (v1.25 feature 8): a fresh slot posts serial 0
    // with a clean ledger. A LAN client never owns a bounty.
    G.bn_serial = 0; G.bn_done = 0; G.bn_cool = 0;
    G.bn_claim_pending = 0; G.bn_live = 0; G.bn_live_reg = 0; G.bn_last_reg = -1;
    bn_flash_t = 0; bn_flash_name = -1;
    // HAT PORTRAIT BOOTH (v1.25 feature 9): a fresh slot has no
    // portrait and the dials sit at their defaults (backdrop 0,
    // hat AS WORN). The shell hand-off starts silent.
    for (int i=0;i<MAXP;i++){ smemset(&portrait[i], 0, sizeof(Portrait)); port_bd[i]=0; port_hat[i]=1; }
    G.port_req_serial = 0;
    G.camx=0; G.camy=40;
    G.state=GS_PLAY;
    if (net_mode!=NET_OFF){
        if (plat_net_start(net_mode, G.join_ip)!=0 && net_mode==NET_CLIENT){
            G.state=GS_MENU; scpy(G.store_msg, "COULD NOT CONNECT", 64);
        }
        if (net_mode==NET_CLIENT){ G.p[0].connected=0; G.p[0].is_local=0; G.my_id=-1; client_joined=0; }
    }
}
static int load_records_and_check_save(void){
    int n = plat_load(savebuf, SAVE_CAP-1);
    if (n<=0) return 0;
    savebuf[n]=0;
    if (slen(savebuf)<4 || savebuf[0]!='T') return 0;
    const char *v;
    if ((v=line_val(savebuf,"rec_name"))) { int i=0; while (v[i] && v[i]!='\n' && i<12){ G.rec_name[i]=v[i]; i++; } G.rec_name[i]=0; }
    if ((v=line_val(savebuf,"rec_hat"))) G.rec_hat=parse_flt(v);
    if ((v=line_val(savebuf,"rec_kills"))) G.rec_kills=parse_int(v);
    if ((v=line_val(savebuf,"rec_bowl"))) G.rec_bowl=parse_int(v);
    if ((v=line_val(savebuf,"rec_time"))) G.rec_time=parse_flt(v);
    if ((v=line_val(savebuf,"my_name"))) { int i=0; while (v[i] && v[i]!='\n' && i<12){ G.my_name[i]=v[i]; i++; } G.my_name[i]=0; }
    if ((v=line_val(savebuf,"bind"))) { const char *q=v;
        for (int i=0;i<14;i++){ int pk=parse_int(q); if (pk>=0 && pk<PK_COUNT) G.bind[i]=pk;
            while (*q && *q!=',' && *q!='\n') q++; if (*q==',') q++; else break; } }
    if ((v=line_val(savebuf,"touchctl"))) G.touch_ctl = parse_int(v)? 1:0;
    if ((v=line_val(savebuf,"zen"))) G.zen = parse_int(v)? 1:0;
    if ((v=line_val(savebuf,"eddie"))) G.eddie = parse_int(v)? 1:0;
    if ((v=line_val(savebuf,"whale"))) G.whale_own = parse_int(v) & 32767;
    if ((v=line_val(savebuf,"whaleon"))) G.whale_on = parse_int(v) & 32767;
    if ((v=line_val(savebuf,"coop"))) G.coop_pref = parse_int(v)? 1:0;
    if ((v=line_val(savebuf,"diff"))){ G.difficulty = parse_int(v); if (G.difficulty<0 || G.difficulty>3) G.difficulty=0; }
    // COMFORT (v1.25 feature 2): line keys an older save simply does
    // not have — the game_init defaults (10/10/off/on) stand.
    if ((v=line_val(savebuf,"volm"))){ G.vol_music = parse_int(v); if (G.vol_music<0) G.vol_music=0; if (G.vol_music>10) G.vol_music=10; }
    if ((v=line_val(savebuf,"vols"))){ G.vol_sfx = parse_int(v); if (G.vol_sfx<0) G.vol_sfx=0; if (G.vol_sfx>10) G.vol_sfx=10; }
    if ((v=line_val(savebuf,"mute"))) G.mute_all = parse_int(v)? 1:0;
    if ((v=line_val(savebuf,"shk"))) G.shake_on = parse_int(v)? 1:0;
    if ((v=line_val(savebuf,"slot"))){ G.slot = parse_int(v); if (G.slot<1 || G.slot>3) G.slot = 1; }
    if ((v=line_val(savebuf,"run")) && parse_int(v)==1) return 1;
    return 0;
}
static void continue_run(int slot){
    int n = plat_load_slot(slot, savebuf, SAVE_CAP-1);
    if (n<=0) return;
    savebuf[n]=0;
    G.slot = slot;
    reset_world();
    for (int i=0;i<MAXP;i++){ G.p[i].connected=0; G.p[i].is_local=0; war_snap_clear(i); }   // parked WAR! battles reload from THIS blob only
    G.net_mode=NET_OFF; G.my_id=0;
    const char *v;
    G.zen_run = G.zen;
    if ((v=line_val(savebuf,"zrun"))) G.zen_run = parse_int(v)? 1:0;
    G.diff_run = 0;   // saves from before difficulty existed were all EASY
    if ((v=line_val(savebuf,"drun"))){ G.diff_run = parse_int(v); if (G.diff_run<0 || G.diff_run>3) G.diff_run=0; }
    if ((v=line_val(savebuf,"time"))) G.time=parse_flt(v);
    if ((v=line_val(savebuf,"tkills"))) G.total_kills=parse_int(v);
    if ((v=line_val(savebuf,"tearned"))) G.total_earned=parse_flt(v);
    // FIRST SHIFT (v1.25): default OFF before parsing — a save from
    // before the chain existed has no fs= line and must never see
    // it. A new-build save resumes its chain mid-step, pool and all
    // (the pools ride the pl-line fp= tokens, parsed below).
    G.fs_state=0; G.fs_step=0; G.fs_fp0=0;
    for (int i=0;i<MAXP;i++) G.fs_pool[i]=0;
    if ((v=line_val(savebuf,"fs"))){ const char *q=v;
        G.fs_state=parse_int(q); while (*q && *q!=',' && *q!='\n') q++; if (*q==',') q++;
        G.fs_step=parse_int(q); while (*q && *q!=',' && *q!='\n') q++; if (*q==',') q++;
        G.fs_fp0=parse_int(q);
        if (G.fs_state<0 || G.fs_state>3) G.fs_state=0;
        if (G.fs_step<0 || G.fs_step>6) G.fs_step=0;   // 6 = past the last step (chain done)
    }
    // WEEKLY CHALLENGE (v1.25 feature 6): defaults before parsing —
    // a save from before the board existed has no wk= line and keeps
    // the zeros. The rollover at the END of this function (after the
    // pl-lines apply every field the baselines read) then opens the
    // current week for it with baselines at the loaded counters, so
    // lifetime progress never retro-completes a delta challenge.
    G.wk_week=0; G.wk_done_week=0; G.wk_total=0; G.wk_prev_week=0; G.wk_prev_done=0;
    for (int i=0;i<7;i++) G.wk_base[i]=0;
    if ((v=line_val(savebuf,"wk"))){ const char *q=v;
        int *dst[12] = { &G.wk_week, &G.wk_done_week, &G.wk_total, &G.wk_prev_week, &G.wk_prev_done,
                         &G.wk_base[0], &G.wk_base[1], &G.wk_base[2], &G.wk_base[3],
                         &G.wk_base[4], &G.wk_base[5], &G.wk_base[6] };
        for (int i=0;i<12;i++){ *dst[i]=parse_int(q); while (*q && *q!=',' && *q!='\n') q++; if (*q==',') q++; else break; }
        if (G.wk_week<0) G.wk_week=0;
        if (G.wk_total<0) G.wk_total=0;
    }
    // NAMED ELITES (v1.25 feature 8): defaults before parsing — a
    // save from before the board existed has no bn= line and posts
    // serial 0 with a clean ledger. Only the contract survives a
    // load (serial, claims, cooldown); the elite itself is a live
    // bug and is never in the blob, and last-seen is runtime memory
    // the board honestly forgets.
    G.bn_serial=0; G.bn_done=0; G.bn_cool=0;
    G.bn_claim_pending=0; G.bn_live=0; G.bn_live_reg=0; G.bn_last_reg=-1;
    bn_flash_t=0; bn_flash_name=-1;
    if ((v=line_val(savebuf,"bn"))){ const char *q=v;
        G.bn_serial=parse_int(q); while (*q && *q!=',' && *q!='\n') q++; if (*q==',') q++;
        G.bn_done=parse_int(q); while (*q && *q!=',' && *q!='\n') q++; if (*q==',') q++;
        G.bn_cool=(float)parse_int(q);
        if (G.bn_serial<0) G.bn_serial=0;
        if (G.bn_done<0) G.bn_done=0;
        if (G.bn_cool<0) G.bn_cool=0;
    }
    // HAT PORTRAIT BOOTH (v1.25 feature 9): defaults before parsing
    // — a pre-feature save has no pt= tokens and simply has no
    // portrait; the dials sit at their defaults. The per-pl-line
    // temps in the tail walk below re-zero each player's record
    // before its own tokens are read.
    for (int i=0;i<MAXP;i++){ smemset(&portrait[i], 0, sizeof(Portrait)); port_bd[i]=0; port_hat[i]=1; }
    G.port_req_serial=0;
    // players: lines begin "pl= " ... find each occurrence
    const char *s = savebuf;
    while (*s){
        if (s[0]=='p' && s[1]=='l' && s[2]=='=' && s[3]==' '){
            const char *q = s+4;
            int idx = parse_int(q); while (*q && *q!=' ') q++; q++;
            char nm[13]; int ni=0; while (*q && *q!=' ' && ni<12){ nm[ni++]=*q++; } nm[ni]=0; q++;
            if (idx>=0 && idx<MAXP){
                Player *pl=&G.p[idx];
                player_fresh(pl, nm, 0, 0, 70);
                pl->connected=1; pl->is_local = (idx==0);
                pl->x=parse_flt(q); while (*q && *q!=' ') q++; q++;
                pl->y=parse_flt(q); while (*q && *q!=' ') q++; q++;
                pl->hp=parse_int(q); while (*q && *q!=' ') q++; q++;
                pl->alive=parse_int(q); while (*q && *q!=' ') q++; q++;
                pl->weapon=parse_int(q); while (*q && *q!=' ') q++; q++;
                if (savebuf[3]=='3'){                            // TSG3: raw copy counts
                    for (int w=0;w<NWEAP;w++){ pl->owned[w]=parse_int(q); while (*q && *q!=' ') q++; q++; }
                } else {
                    int ow=parse_int(q); while (*q && *q!=' ') q++; q++;
                    if (savebuf[3]=='1') unpack_owned_mask(pl, ow);   // TSG1 saves: bitmask
                    else unpack_copies(pl, ow);                      // TSG2: copy counts
                }
                for (int u=0;u<5;u++){ pl->upg[u]=parse_int(q); while (*q && *q!=' ') q++; q++; }
                pl->rollback=parse_int(q); while (*q && *q!=' ') q++; q++;
                pl->kills=parse_int(q); while (*q && *q!=' ') q++; q++;
                pl->heaviest=parse_flt(q); while (*q && *q!=' ') q++; q++;
                pl->color=parse_int(q); while (*q && *q!=' ') q++; q++;
                int sn=parse_int(q); while (*q && *q!=':') q++; q++;
                pl->stack_n=0; pl->carried=0;
                for (int s2=0;s2<sn;s2++){
                    int w2=parse_int(q); hat_push(pl, w2*0.5f);
                    // the last stack item is followed by a SPACE and the
                    // key= token tail (bk/hs/ml/bd/...): the skip must
                    // stop there too. It didn't — with any carcass on
                    // your hat (sn>=1) this ran to the end of the line
                    // (or the first comma inside cw=), devouring the
                    // whole tail: buddy, banked kg, story, craft — all
                    // lost on Continue while the slot list, which scans
                    // the text, still showed them (Daniel, Nov 5).
                    while (*q && *q!=',' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                }
                // optional key= tokens (bank / hat stand / mall / buddy)
                { int bsp=0, bxp=0, b1=0, b2=0, b3=0, bsk=0, bfn=0, bnm=-1, bcl=0, bnv=0, bwo=0, bwn=0, ben=100, bgx=0, bgy=56, bgz=0, ber=0, bgf=0;
                  int bsm=0, bsq=0, bsi=0, bsq2=0, bsb=0, bpd=0, btd=0, bcb=0, bcs=0, bcp=0, bcq=0, bcr=0, bwr=0, bcx=0, bcy=0, bcm=0, bcn=0, bcg=0, bct=0, bcf=0, bfp=0, be2=0;
                  int btp=0, btw=0;   // HAT TOPPERS temps (feat 5): absent token = nothing earned, nothing worn
                  int tcw_bn=0, tcw_x[48], tcw_y[48], tcw_k[48], tcw_h[48];
                  int tcd_gn=0, tcd_g[96];
                  int tmc[8]={0}, tmr[16]={0};   // MEDAL TIMES temps (feat 3): absent token = all zeros = no medals
                  // HAT PORTRAIT BOOTH temps (feat 9): the pt= token's
                  // twelve ints + the frozen stack's kind digits.
                  // Absent token = dial 0, no portrait. This player's
                  // record re-zeros here, before its tokens are read.
                  int tpt_on=0, tpt[12]={0}, tptk_n=0; char tptk[MAXSTACK];
                  smemset(&portrait[idx], 0, sizeof(Portrait)); port_bd[idx]=0; port_hat[idx]=1;
                  // BUG BOWL LEAGUE temps (feat 10): the bl= token's
                  // fourteen ints. Absent token = an empty book. This
                  // player's book re-zeros here, before its token is
                  // read (the feat-9 portrait pattern).
                  int tbl_on=0, tbl[14]={0};
                  smemset(&G.blg[idx], 0, sizeof(BowlLeague));
                  // WAR! snapshot (v1.17): the six values ride as raw
                  // text (',' inside records, ';' between them, never
                  // a space) and are rebuilt into war_snap[idx] below.
                  // The nodes key is "wg", NOT the spec's "wn" — wn is
                  // already whale_on in this same token tail.
                  char twb[64], twu[4096], tws[2688], twg[192], twt[4800], twf[512];   // v1.18: caps re-audited — wt must hold 160 terrain entries now, and the old tws/twf caps could already clip a big battle's tail
                  int twar=0; twb[0]=twu[0]=tws[0]=twg[0]=twt[0]=twf[0]=0;
                  while (*q==' '){
                      q++;
                      char kk[4]; int ki=0;
                      while (*q && *q!='=' && *q!=' ' && *q!='\n' && ki<3) kk[ki++]=*q++;
                      kk[ki]=0;
                      if (*q=='=') q++;
                      if (kk[0]=='c' && kk[1]=='w' && kk[2]==0){   // compound build list: tx,ty,kind,hp;…
                          while (*q && *q!=' ' && *q!='\n' && tcw_bn<48){
                              tcw_x[tcw_bn]=parse_int(q); while (*q && *q!=',' && *q!=';' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                              tcw_y[tcw_bn]=parse_int(q); while (*q && *q!=',' && *q!=';' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                              tcw_k[tcw_bn]=parse_int(q); while (*q && *q!=',' && *q!=';' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                              tcw_h[tcw_bn]=parse_int(q); while (*q && *q!=';' && *q!=' ' && *q!='\n') q++; if (*q==';') q++;
                              tcw_bn++;
                          }
                      } else if (kk[0]=='c' && kk[1]=='d' && kk[2]==0){   // compound mined-node list: key;key;…
                          while (*q && *q!=' ' && *q!='\n' && tcd_gn<96){
                              tcd_g[tcd_gn++]=parse_int(q); while (*q && *q!=';' && *q!=' ' && *q!='\n') q++; if (*q==';') q++;
                          }
                      } else if (kk[0]=='m' && kk[1]=='c' && kk[2]==0){   // MEDAL TIMES: 8 comma-separated cart bests (ds)
                          int ti=0;
                          while (*q && *q!=' ' && *q!='\n' && ti<8){
                              tmc[ti++]=parse_int(q); while (*q && *q!=',' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                          }
                          while (*q && *q!=' ' && *q!='\n') q++;   // consume any overflow exactly to the delimiter (Nov 5 lesson)
                      } else if (kk[0]=='m' && kk[1]=='r' && kk[2]==0){   // MEDAL TIMES: 16 comma-separated run bests (ds)
                          int ti=0;
                          while (*q && *q!=' ' && *q!='\n' && ti<16){
                              tmr[ti++]=parse_int(q); while (*q && *q!=',' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                          }
                          while (*q && *q!=' ' && *q!='\n') q++;
                      } else if (kk[0]=='b' && kk[1]=='l' && kk[2]==0){   // BUG BOWL LEAGUE (feat 10): 14 comma-separated league-book ints
                          tbl_on=1;
                          int ti=0;
                          while (*q && *q!=' ' && *q!='\n' && ti<14){
                              tbl[ti++]=parse_int(q); while (*q && *q!=',' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                          }
                          while (*q && *q!=' ' && *q!='\n') q++;   // consume any overflow exactly to the delimiter (Nov 5 lesson)
                      } else if (kk[0]=='p' && kk[1]=='t' && kk[2]==0){   // HAT PORTRAIT BOOTH (feat 9): dial,has,backdrop,haton,top,kg,stack_n,species,budcolor,nameidx,pcolor,whale : kind digits
                          tpt_on=1;
                          int ti=0;
                          while (*q && *q!=':' && *q!=' ' && *q!='\n' && ti<12){
                              tpt[ti++]=parse_int(q); while (*q && *q!=',' && *q!=':' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                          }
                          if (*q==':'){
                              q++;
                              while (*q>='0' && *q<='9' && tptk_n<MAXSTACK) tptk[tptk_n++]=*q++;
                          }
                          while (*q && *q!=' ' && *q!='\n') q++;   // consume any overflow exactly to the delimiter (Nov 5 lesson)
                      } else if (kk[0]=='w' && kk[2]==0 && (kk[1]=='b' || kk[1]=='u' || kk[1]=='s' || kk[1]=='g' || kk[1]=='t' || kk[1]=='f')){
                          // a WAR! snapshot value: copy it raw (bounded);
                          // the generic tail-skip below then steps over it
                          char *dst; int cap;
                          if (kk[1]=='b'){ dst=twb; cap=(int)sizeof(twb); }
                          else if (kk[1]=='u'){ dst=twu; cap=(int)sizeof(twu); }
                          else if (kk[1]=='s'){ dst=tws; cap=(int)sizeof(tws); }
                          else if (kk[1]=='g'){ dst=twg; cap=(int)sizeof(twg); }
                          else if (kk[1]=='t'){ dst=twt; cap=(int)sizeof(twt); }
                          else { dst=twf; cap=(int)sizeof(twf); }
                          { int di=0; while (*q && *q!=' ' && *q!='\n' && di<cap-1) dst[di++]=*q++;
                            dst[di]=0; while (*q && *q!=' ' && *q!='\n') q++; }
                          if (kk[1]=='b') twar=1;   // the header anchors the snapshot; the rest are its luggage
                      }
                      int vv=parse_int(q);
                      if (kk[0]=='b' && kk[1]=='k' && kk[2]==0) pl->banked=vv*0.5f;
                      else if (kk[0]=='b' && kk[1]=='b' && kk[2]==0) pl->beans=vv;
                      else if (kk[0]=='f' && kk[1]=='p' && kk[2]==0) bfp=vv;
                      else if (kk[0]=='h' && kk[1]=='s') pl->hatstand=vv?1:0;
                      else if (kk[0]=='m' && kk[1]=='l') pl->in_mall=vv?1:0;
                      else if (kk[0]=='a' && kk[1]=='r') pl->in_arcade=vv?1:0;
                      else if (kk[0]=='w' && kk[1]=='o') bwo=vv&32767;
                      else if (kk[0]=='w' && kk[1]=='n') bwn=vv&32767;
                      else if (kk[0]=='b' && kk[1]=='d') bsp=vv;
                      else if (kk[0]=='b' && kk[1]=='x') bxp=vv;
                      else if (kk[0]=='b' && kk[1]=='1') b1=vv;
                      else if (kk[0]=='b' && kk[1]=='2') b2=vv;
                      else if (kk[0]=='b' && kk[1]=='3') b3=vv;
                      else if (kk[0]=='b' && kk[1]=='s') bsk=vv;
                      else if (kk[0]=='b' && kk[1]=='e') ben=vv;
                      else if (kk[0]=='g' && kk[1]=='x') bgx=vv;
                      else if (kk[0]=='g' && kk[1]=='y') bgy=vv;
                      else if (kk[0]=='g' && kk[1]=='z') bgz=vv;
                      else if (kk[0]=='g' && kk[1]=='f') bgf=vv;
                      else if (kk[0]=='b' && kk[1]=='f') bfn=vv;
                      else if (kk[0]=='b' && kk[1]=='n' && kk[2]=='v') bnv=vv;
                      else if (kk[0]=='b' && kk[1]=='n') bnm=vv;
                      else if (kk[0]=='b' && kk[1]=='c') bcl=vv;
                      else if (kk[0]=='s' && kk[1]=='m') bsm=vv;
                      else if (kk[0]=='s' && kk[1]=='q' && kk[2]=='2') bsq2=vv;
                      else if (kk[0]=='e' && kk[1]=='r') ber=vv;
                      else if (kk[0]=='e' && kk[1]=='2') be2=vv;   // DEEPER DOORS (feat 11): absent token = Chapter 2 unstarted
                      else if (kk[0]=='s' && kk[1]=='b') bsb=vv;
                      else if (kk[0]=='p' && kk[1]=='d') bpd=vv;
                      else if (kk[0]=='t' && kk[1]=='d') btd=vv;
                      else if (kk[0]=='c' && kk[1]=='b') bcb=vv;
                      else if (kk[0]=='c' && kk[1]=='s') bcs=vv;
                      else if (kk[0]=='c' && kk[1]=='p') bcp=vv;
                      else if (kk[0]=='c' && kk[1]=='q') bcq=vv;
                      else if (kk[0]=='c' && kk[1]=='r') bcr=vv;
                      else if (kk[0]=='w' && kk[1]=='r') bwr=vv;
                      else if (kk[0]=='t' && kk[1]=='p') btp=vv;
                      else if (kk[0]=='t' && kk[1]=='w') btw=vv;
                      else if (kk[0]=='c' && kk[1]=='x') bcx=vv;
                      else if (kk[0]=='c' && kk[1]=='y') bcy=vv;
                      else if (kk[0]=='c' && kk[1]=='m') bcm=vv;
                      else if (kk[0]=='c' && kk[1]=='n') bcn=vv;
                      else if (kk[0]=='c' && kk[1]=='g') bcg=vv;
                      else if (kk[0]=='c' && kk[1]=='t') bct=vv;
                      else if (kk[0]=='c' && kk[1]=='f') bcf=vv;
                      else if (kk[0]=='s' && kk[1]=='q') bsq=vv;
                      else if (kk[0]=='s' && kk[1]=='i') bsi=vv;
                      while (*q && *q!=' ' && *q!='\n') q++;
                  }
                  pl->whale_own=bwo; pl->whale_on=bwn;
                  G.fs_pool[idx]=bfp;
                  // BUG BOWL LEAGUE (feat 10): the league book,
                  // Player-level (it applies whether or not a buddy
                  // rides this pl-line), clamped field by field.
                  if (tbl_on){ BowlLeague *L=&G.blg[idx];
                      L->games = tbl[0]<0? 0 : tbl[0];
                      L->best = tbl[1]<0? 0 : tbl[1]>300? 300 : tbl[1];
                      L->pins = tbl[2]<0? 0 : tbl[2];
                      L->strikes = tbl[3]<0? 0 : tbl[3];
                      L->season = tbl[4]<0? 0 : tbl[4];
                      L->night = tbl[5]<0? 0 : tbl[5]>5? 5 : tbl[5];
                      L->spins = tbl[6]<0? 0 : tbl[6];
                      L->trophies = tbl[7]<0? 0 : tbl[7];
                      L->last_score = tbl[8]<0? 0 : tbl[8]>300? 300 : tbl[8];
                      L->last_place = tbl[9]<0? 0 : tbl[9]>4? 4 : tbl[9];
                      L->last_night = tbl[10]<0? 0 : tbl[10]>6? 6 : tbl[10];
                      L->last_season = tbl[11]<0? 0 : tbl[11];
                      L->fin_place = tbl[12]<0? 0 : tbl[12]>4? 4 : tbl[12];
                      L->fin_season = tbl[13]<0? 0 : tbl[13];
                  }
                  if (bsp>0){
                      Buddy *bd=&G.bud[idx];
                      smemset(bd, 0, sizeof(*bd));
                      bd->active=1; bd->species=(bsp-1)%BUDN; bd->xp=bxp; bd->on_wall=-1;
                      bd->up_hp=b1; bd->up_atk=b2; bd->up_def=b3; bd->skills=bsk;
                      bd->energy = ben<0? 0 : ben>100? 100 : ben;
                      bd->bw_x = bgx; bd->bw_y = bgy; bd->bw_seed = bgz; bd->bw_started = bgf? 1:0;
                      bd->fainted=bfn? 1:0;
                      if (bnm<0) bd->name_idx = BUD_DEF_NAME[bd->species];
                      else if (bnv>=2) bd->name_idx = bnm%BUD_NAMES_N;
                      else {   // legacy save: the index names a slot in the OLD list — carry the name across
                          bd->name_idx = BUD_DEF_NAME[bd->species];
                          const char *want = BUD_ELF_LEGACY[bnm%20];
                          for (int n2=0;n2<BUD_NAMES_N;n2++)
                              if (scmp(BUD_ELF[n2], want)==0){ bd->name_idx=n2; break; }
                      }
                      bd->color = bcl%BUD_COLORS_N;
                      bd->story_started = bsm? 1:0; bd->story_bits = bsq; bd->story_bits2 = bsq2;
                      bd->ent_best = ber % 10000; bd->ent_done = ber / 10000;
                      bd->ent_sub_best = bsb;
                      bd->ent2_best = be2 % 10000; bd->ent2_done = be2 / 10000;   // DEEPER DOORS (feat 11)
                      bd->plat_done = bpd<0? 0 : bpd;
                      bd->tank_done = btd<0? 0 : btd;
                      bd->cas_best = bcb<0? 0 : bcb;
                      bd->cas_shufn = bcs<0? 0 : bcs;   // CASTLE SHUFFLE (feat 7): absent on pre-feature saves -> 0
                      bd->crt_best_pos = bcp<0? 0 : bcp;
                      bd->crt_cupbest = bcr<0? 0 : bcr;
                      for (int ti=0; ti<8; ti++) bd->crt_tbest[ti] = tmc[ti]<0? 0 : tmc[ti];    // MEDAL TIMES (feat 3)
                      for (int ti=0; ti<16; ti++) bd->plat_tbest[ti] = tmr[ti]<0? 0 : tmr[ti];
                      bd->war_prog = bwr<0? 0 : bwr>8? 8 : bwr;
                      bd->top_earned = btp<0? 0 : (btp & TOP_ALL);   // HAT TOPPERS (feat 5)
                      bd->top_worn = (btw>=1 && btw<=TOP_N && (bd->top_earned & TOP_BIT(btw)))? btw : 0;
                      // HAT PORTRAIT BOOTH (feat 9): the dial and the
                      // frozen portrait, clamped field by field — a
                      // hand-edited or future token can ask for
                      // anything; the booth only ever shows a pose
                      // the game's own tables can draw.
                      port_bd[idx] = tpt_on? (tpt[0]<0? 0 : tpt[0]%PORT_BD_N) : 0;
                      if (tpt_on && tpt[1]){
                          Portrait *pt2=&portrait[idx];
                          pt2->has = 1;
                          pt2->backdrop = tpt[2]<0? 0 : tpt[2]%PORT_BD_N;
                          pt2->hat_on = tpt[3]? 1 : 0;
                          pt2->top_worn = (tpt[4]>=0 && tpt[4]<=TOP_N)? tpt[4] : 0;
                          pt2->kg = tpt[5]<0? 0 : tpt[5];
                          pt2->stack_n = tpt[6]<0? 0 : tpt[6]>MAXSTACK? MAXSTACK : tpt[6];
                          pt2->species = tpt[7]<0? 0 : tpt[7]%BUDN;
                          pt2->bud_color = tpt[8]<0? 0 : tpt[8]%BUD_COLORS_N;
                          pt2->name_idx = tpt[9]<0? 0 : tpt[9]%BUD_NAMES_N;
                          pt2->pcolor = tpt[10]&3;
                          pt2->whale_on = tpt[11]&32767;
                          for (int ki=0; ki<pt2->stack_n; ki++)
                              pt2->kinds[ki] = (ki<tptk_n)? (unsigned char)(tptk[ki]-'0')&3 : 0;
                      }
                      bd->crt_best_time = bcq<0? 0 : bcq;
                      bd->craft_x = bcx; bd->craft_y = bcy;
                      bd->craft_wood = bcm % 1000; bd->craft_stone = (bcm/1000) % 1000; bd->craft_fiber = bcm / 1000000;
                      bd->craft_car = bcn % 1000; bd->craft_pies = (bcn/1000) % 1000; bd->craft_stews = bcn / 1000000;
                      bd->craft_gear = bcg;
                      bd->craft_tod = bct * 0.1f; if (bd->craft_tod<0 || bd->craft_tod>240) bd->craft_tod = 40;
                      bd->craft_fish = bcf;
                      bd->craft_bn = tcw_bn;
                      for (int bi=0;bi<tcw_bn;bi++){ bd->craft_btx[bi]=tcw_x[bi]; bd->craft_bty[bi]=tcw_y[bi]; bd->craft_bk[bi]=tcw_k[bi]; bd->craft_bhp[bi]=tcw_h[bi]; }
                      if (twar){   // rebuild the canonical WAR! snapshot text for this player
                          char *sp = war_snap[idx];
                          sp = sappend(sp, "wb="); sp = sappend(sp, twb);
                          sp = sappend(sp, " wu="); sp = sappend(sp, twu);
                          sp = sappend(sp, " ws="); sp = sappend(sp, tws);
                          sp = sappend(sp, " wg="); sp = sappend(sp, twg);
                          sp = sappend(sp, " wt="); sp = sappend(sp, twt);
                          sp = sappend(sp, " wf="); sp = sappend(sp, twf);
                          *sp = 0; war_snap_on[idx] = 1;
                          war_snap_reg[idx] = parse_int(twb);   // v1.18: the wb header's first field is the region
                      }
                      bd->craft_gn = tcd_gn;
                      for (int gi=0;gi<tcd_gn;gi++) bd->craft_gone[gi]=tcd_g[gi];
                      { int pk=bsi; for (int i2=0;i2<4;i2++){ bd->items[i2]=pk%10; pk/=10; } }
                      buddy_refresh(idx);
                      topper_refresh(idx, 0);   // HAT TOPPERS: derive from the records just applied (silent at load)
                      bd->hp = bd->fainted? 0 : bd->maxhp;   // a fainted buddy loads still fainted
                      bd->x=pl->x+26; bd->y=pl->y+12;
                  }
                }
                pl->dmg_mul = 1.0f + 0.25f*pl->upg[0];
                pl->rate_mul = 1.0f + 0.18f*pl->upg[1];
                pl->shield_max = shield_for(pl->upg[2]);
                pl->shield = pl->shield_max;
                pl->maxhp = 100 + 25*pl->upg[3];
            }
        }
        while (*s && *s!='\n') s++;
        if (*s=='\n') s++;
    }
    s = savebuf;
    while (*s){
        if (s[0]=='w' && s[1]=='l' && s[2]=='=' && s[3]==' '){
            const char *q=s+4;
            for (int i=0;i<MAXWALLS;i++) if (!G.walls[i].alive){
                Wall *w=&G.walls[i];
                w->x=parse_flt(q); while (*q && *q!=' ') q++; q++;
                w->y=parse_flt(q); while (*q && *q!=' ') q++; q++;
                w->hp=parse_flt(q); while (*q && *q!=' ') q++; q++;
                w->maxhp=parse_flt(q); while (*q && *q!=' ') q++; q++;
                w->horiz=parse_int(q); while (*q && *q!=' ' && *q!='\n') q++; if (*q==' ') q++;
                // stack count rides at the end of the line (Nov 7);
                // older saves have nothing there — one wall, as saved
                w->stack = (*q && *q!='\n')? parse_int(q) : 1;
                if (w->stack<1) w->stack=1; if (w->stack>999) w->stack=999;
                w->alive=1;
                break;
            }
        }
        while (*s && *s!='\n') s++;
        if (*s=='\n') s++;
    }
    // absorb any legacy stacks (saves from before the absorb rule can
    // hold several entries on one spot): same place, same direction
    // becomes one wall with the summed strength before pools rebuild —
    // and (Nov 7) the merged pile keeps COUNT of its layers, so the
    // stack renders, tiers and sheds like the walls it was made of
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1)
        for (int j=i+1;j<MAXWALLS;j++) if (G.walls[j].alive==1 && G.walls[j].horiz==G.walls[i].horiz){
            float dx=G.walls[j].x-G.walls[i].x, dy=G.walls[j].y-G.walls[i].y;
            if (dx*dx+dy*dy < 36){ G.walls[i].hp += G.walls[j].hp; G.walls[i].maxhp += G.walls[j].maxhp;
                G.walls[i].stack += G.walls[j].stack>0? G.walls[j].stack : 1;
                if (G.walls[i].stack>999) G.walls[i].stack=999;
                G.walls[j].alive=0; }
        }
    walls_recompute_ex(1);   // pooled health rebuilds from the saved fractions
    if (!G.p[0].connected) player_fresh(&G.p[0], G.my_name, 0, 0, 70), G.p[0].connected=1, G.p[0].is_local=1;
    // Daniel's rule (Nov 2): a loaded run always resumes at the
    // centre of the Bug Mall, wherever the save was made. Restoring
    // in place put mall saves about 120px north of where the player
    // stood (the interior lives at MALL_CY, far from the world
    // spot the raw numbers suggest) and scattered everyone else.
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        G.p[i].x = i==0? 0.0f : 34.0f*i; G.p[i].y = MALL_CY + (i==0? 0.0f : 10.0f);
        G.p[i].in_mall=1; G.p[i].in_store=0; G.p[i].station=-1;
        // the resume point is the Mall centre, so the Arcade flag the
        // save carried (ar=, written wherever the run was saved) must
        // NOT survive the load: a player saved mid-Arcade used to
        // reload standing in the Mall still flagged in_arcade, and
        // arcade_door_at (which requires !in_arcade) then refused
        // them re-entry until they left by the exit mat (found by the
        // feat-7 battery; fixed in the v1.25 E2E pass).
        G.p[i].in_arcade=0;
        // a fainted buddy can't follow its owner home, so the load used
        // to leave it lying wherever the save was made — invisible, and
        // indistinguishable from never having bought it (Nov 3). Carry
        // it home: it wakes up on the mall floor beside its owner.
        if (G.bud[i].active && G.bud[i].fainted){
            G.bud[i].x = G.p[i].x + 26; G.bud[i].y = G.p[i].y + 12;
        }
    }
    if (G.bud[0].active && G.bud[0].fainted)
        spawn_floater(G.p[0].x, G.p[0].y - 30, "YOUR BUDDY IS FAINTED - REVIVE IT AT BANZAI BUDDIES", COL(255,180,150));
    // local co-op: claim back the saved P2 (same slot, same run). If the
    // save predates co-op there is no P2 line — with two controllers
    // plugged in, P2 drops in fresh beside P1 instead.
    G.p2_slot=-1; coop_reset_views();
    if (G.coop_pref){
        for (int i=1;i<MAXP;i++)
            if (G.p[i].connected && scmp(G.p[i].name, "P2")==0){ G.p2_slot=i; break; }
        if (G.p2_slot<0 && IN.pad_count>=2) coop_spawn_p2(G.p[0].x-30, G.p[0].y);
    }
    // the local user's Whales unlocks come from the profile, not the run
    G.p[0].whale_own=G.whale_own; G.p[0].whale_on=G.whale_on;
    if (G.p2_slot>=0){ G.p[G.p2_slot].whale_own=G.whale_own; G.p[G.p2_slot].whale_on=G.whale_on; }
    // WEEKLY CHALLENGE (v1.25 feature 6): with every record the
    // baselines read now applied (and P2 claimed back), open the
    // current week for this slot — a save from an older week, or
    // from before the board existed, rolls over here, silently;
    // the board tells the story.
    wk_rollover();
    G.camx=G.p[0].x; G.camy=G.p[0].y;
    G.state=GS_PLAY;
}

// ---------------------------------------------------------------------------
// Save slots: probe at boot, summarize for the picker, run the picker.
// Legacy saves (one global blob with the run inside) become slot 1.
// ---------------------------------------------------------------------------
static void probe_slots(void){
    for (int s=1;s<=3;s++){
        int n = plat_load_slot(s, slotbuf, SAVE_CAP-1);
        G.slot_has[s] = 0;
        if (n>0){ slotbuf[n]=0; const char *v = line_val(slotbuf,"run");
            if (v && parse_int(v)==1) G.slot_has[s]=1; }
    }
    if (!G.slot_has[1]){
        // legacy: the global blob (still in savebuf from the records
        // load) carries the run — lift its run section into slot 1,
        // keeping the original format version character.
        const char *v = line_val(savebuf,"run");
        if (v && parse_int(v)==1){
            const char *rs = v - 4;                    // start of "run="
            char *q = slotbuf;
            q = sappend(q, "TSG"); *q++ = savebuf[3]; *q++='\n'; *q=0;
            while (*rs) *q++ = *rs++;
            *q = 0;
            plat_save_slot(1, slotbuf, (int)(q - slotbuf));
            G.slot_has[1] = 1;
        }
    }
    refresh_has_save();
}
// picker summaries, peeked from the slot blobs (never loads the run)
static char pk_name[4][13];
static int pk_hat[4], pk_kills[4], pk_time[4], pk_zen[4];
static char pk_bname[4][16];   // the slot's buddy, "" when none
static int pk_bspecies[4];
static void peek_slots(void){
    for (int s=1;s<=3;s++){
        pk_name[s][0]=0; pk_hat[s]=0; pk_kills[s]=0; pk_time[s]=0; pk_zen[s]=0;
        pk_bname[s][0]=0; pk_bspecies[s]=-1;
        if (!G.slot_has[s]) continue;
        int n = plat_load_slot(s, slotbuf, SAVE_CAP-1);
        if (n<=0){ G.slot_has[s]=0; continue; }
        slotbuf[n]=0;
        const char *v;
        if ((v=line_val(slotbuf,"time"))) pk_time[s]=parse_int(v);
        if ((v=line_val(slotbuf,"tkills"))) pk_kills[s]=parse_int(v);
        if ((v=line_val(slotbuf,"zrun"))) pk_zen[s]=parse_int(v)? 1:0;
        if ((v=line_val(slotbuf,"sum"))){
            const char *q=v;
            while (*q==' ') q++;   // the writer's separator space after
            // "sum=" — without this skip the name reads empty and every
            // field after it shifts one token early (the other half of
            // the phantom-pet bug: the tail walk then lands mid-list)
            int ni=0;
            while (*q && *q!=' ' && *q!='\n' && ni<12){ pk_name[s][ni++]=*q++; } pk_name[s][ni]=0;
            if (*q==' ') q++;
            pk_hat[s]=parse_int(q);
            // walk the rest of the summary: hat kills time zen, then
            // the buddy tail [bnameidx bspecies]. This once walked only
            // three fields, so it read ZEN as the name index and the
            // name index as the species — the picker then cheerfully
            // listed a pet the player does not have (Daniel, Nov 4)
            for (int adv=0; adv<4; adv++){ while (*q && *q!=' ' && *q!='\n') q++; if (*q==' ') q++; }
            if (*q && *q!='\n'){
                int bni=parse_int(q);
                while (*q && *q!=' ' && *q!='\n') q++; if (*q==' ') q++;
                int bsp=parse_int(q);
                if (bni>=0 && bsp>=0 && bsp<BUDN){
                    scpy(pk_bname[s], BUD_ELF[bni%BUD_NAMES_N], 16);
                    pk_bspecies[s]=bsp;
                }
            }
        } else {
            // legacy blob: the name rides on the first pl= line
            const char *ls = slotbuf;
            while (*ls){
                if (ls[0]=='p' && ls[1]=='l' && ls[2]=='=' && ls[3]==' '){
                    const char *q=ls+4;
                    parse_int(q); while (*q && *q!=' ') q++; if (*q) q++;
                    int ni=0; while (*q && *q!=' ' && *q!='\n' && ni<12){ pk_name[s][ni++]=*q++; } pk_name[s][ni]=0;
                    break;
                }
                while (*ls && *ls!='\n') ls++;
                if (*ls=='\n') ls++;
            }
        }
        if (!pk_name[s][0]) scpy(pk_name[s], "DEV", 13);
    }
    refresh_has_save();
}
static void open_slots(int mode){
    G.slots_mode=mode; G.slots_confirm=0;
    peek_slots();
    int pick = (G.slot>=1 && G.slot<=3)? G.slot-1 : 0;
    if (mode==0 && !G.slot_has[pick+1])
        for (int s=1;s<=3;s++) if (G.slot_has[s]){ pick=s-1; break; }
    G.slots_sel = pick;
    G.state=GS_SLOTS;
}
static void slots_activate(int i);   // defined with the menu code below
static void slots_input(void){
    if (IN.menu_up){ G.slots_sel=(G.slots_sel+3)%4; G.slots_confirm=0; }
    if (IN.menu_down){ G.slots_sel=(G.slots_sel+1)%4; G.slots_confirm=0; }
    if (IN.esc_pressed){ G.state=GS_MENU; return; }
    if (IN.tapped){
        for (int i=0;i<3;i++){ int y=116+i*52;
            if (IN.tap_y>=y-4 && IN.tap_y<=y+48){
                if (G.slots_mode==1 && G.slot_has[i+1] && G.slots_confirm!=i+1){
                    G.slots_sel=i; G.slots_confirm=i+1; return;
                }
                G.slots_sel=i; slots_activate(i); return;
            }
        }
        if (IN.tap_y>=272 && IN.tap_y<=300){ G.state=GS_MENU; return; }
    }
    if (IN.select_pressed || IN.interact_pressed) slots_activate(G.slots_sel);
}

// ---------------------------------------------------------------------------
// Networking — one packet format for both platforms.
//   client->host: 'J' name | 'I' keys,aim,weapon | 'B' item | 'L' leave
//   host->client: 'R' roster | 'A' your-id | 'S' snapshot | 'W' walls | 'G' over
// ---------------------------------------------------------------------------
static u8 pkt[65536];
static int pkt_src_token[MAXP];       // last transport token per slot (host)
static float net_send_t=0;
// event queue for snapshots (splats, purchases, wall builds...)
typedef struct { u8 type; float x, y; } NetEv;
static NetEv ev_queue[32]; static int ev_n=0;
static void push_ev(int type, float x, float y){
    if (G.net_mode!=NET_HOST) return;
    if (ev_n<32){ ev_queue[ev_n].type=(u8)type; ev_queue[ev_n].x=x; ev_queue[ev_n].y=y; ev_n++; }
}
static void wr_u8(int *o, u8 v){ pkt[(*o)++]=v; }
static void wr_u16(int *o, u16 v){ pkt[(*o)++]=v&255; pkt[(*o)++]=(v>>8)&255; }
static void wr_flt(int *o, float f){ union{float f; u8 b[4];} u; u.f=f;
    for (int i=0;i<4;i++) pkt[(*o)++]=u.b[i]; }
static u8 rd_u8(const u8 *b, int *o){ return b[(*o)++]; }
static u16 rd_u16(const u8 *b, int *o){ u16 v=b[*o] | (b[*o+1]<<8); *o+=2; return v; }
static float rd_flt(const u8 *b, int *o){ union{float f; u8 b[4];} u;
    for (int i=0;i<4;i++) u.b[i]=b[(*o)++]; return u.f; }

static void host_send_roster(int dest){
    int o=0; wr_u8(&o, 'R');
    int n=0; for (int i=0;i<MAXP;i++) if (G.p[i].connected) n++;
    wr_u8(&o, (u8)n);
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        wr_u8(&o, (u8)i); wr_u8(&o, (u8)G.p[i].color);
        for (int k=0;k<12;k++) wr_u8(&o, (u8)G.p[i].name[k]);
    }
    plat_net_send(pkt, o, dest);
}
static void host_send_walls(int dest){
    int o=0; wr_u8(&o, 'W');
    int n=0; for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1) n++;
    wr_u16(&o, (u16)n);
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1){
        Wall *w=&G.walls[i];
        wr_flt(&o, w->x); wr_flt(&o, w->y); wr_flt(&o, w->hp); wr_flt(&o, w->maxhp);
        wr_u8(&o, (u8)w->horiz); wr_u8(&o, (u8)(w->stack>0? w->stack : 1));
    }
    plat_net_send(pkt, o, dest);
}
static void host_snapshot(void){
    int o=0; wr_u8(&o, 'S');
    wr_flt(&o, G.time);
    wr_u16(&o, (u16)G.total_kills);
    wr_u8(&o, (u8)G.diff_run);
    for (int i=0;i<MAXP;i++){
        Player *p=&G.p[i];
        wr_u8(&o, p->connected?1:0);
        if (!p->connected) continue;
        wr_flt(&o, p->x); wr_flt(&o, p->y); wr_flt(&o, p->aim);
        wr_u16(&o, (u16)(p->hp<0?0:p->hp)); wr_u16(&o, (u16)p->shield);
        wr_u8(&o, (u8)p->weapon); wr_u8(&o, p->alive?1:0);
        wr_u16(&o, (u16)p->stack_n); wr_flt(&o, p->carried); wr_flt(&o, p->heaviest);
        wr_u16(&o, (u16)p->kills);
        wr_u8(&o, (u8)p->rollback);
        for (int w=0;w<NWEAP;w++) wr_u16(&o, (u16)p->owned[w]);
        for (int u=0;u<5;u++) wr_u16(&o, (u16)p->upg[u]);
        wr_flt(&o, p->banked); wr_u8(&o, (u8)p->hatstand); wr_u8(&o, (u8)p->in_mall);
        wr_u8(&o, p->on_wall>=0? 1:0);
        wr_u8(&o, (u8)p->whale_own); wr_u8(&o, (u8)p->whale_on);
        wr_u8(&o, (u8)(p->whale_own>>8)); wr_u8(&o, (u8)(p->whale_on>>8));   // round 2 vanities (SKUs 7-14)
        wr_u8(&o, (u8)p->top_worn);   // HAT TOPPERS (feat 5): guests see the worn trophy
    }
    int bn=0; for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive) bn++;
    wr_u16(&o, (u16)bn);
    for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
        Bug *b=&G.bugs[i];
        wr_flt(&o, b->x); wr_flt(&o, b->y); wr_flt(&o, b->w);
        wr_u8(&o, (u8)(255*b->hp/b->maxhp)); wr_u8(&o, (u8)b->kind);
        wr_u8(&o, b->bp_t>0?1:0); wr_u8(&o, b->hit_t>0?1:0);
        wr_flt(&o, b->leg_ph);
        wr_u8(&o, (u8)b->elite);   // NAMED ELITES (feat 8): guests see the nameplate
    }
    wr_u8(&o, (u8)ev_n);
    for (int i=0;i<ev_n;i++){ wr_u8(&o, ev_queue[i].type); wr_flt(&o, ev_queue[i].x); wr_flt(&o, ev_queue[i].y); }
    ev_n=0;
    wr_u8(&o, shield_active()?1:0);
    wr_u8(&o, 0);   // agents retired (Nov 2): the count byte stays, always zero
    int bn2=0; for (int i=0;i<MAXP;i++) if (G.bud[i].active) bn2++;
    wr_u8(&o, (u8)bn2);
    for (int i=0;i<MAXP;i++) if (G.bud[i].active){
        Buddy *bd=&G.bud[i];
        wr_u8(&o, (u8)i); wr_u8(&o, (u8)bd->species);
        wr_flt(&o, bd->x); wr_flt(&o, bd->y);
        wr_u8(&o, (u8)(bd->maxhp>0? 255.0f*bd->hp/bd->maxhp : 0));
        wr_u8(&o, bd->lunge_t>0? 1:0);
        wr_u16(&o, (u16)(bd->xp>60000? 60000 : bd->xp));
        wr_u8(&o, (u8)bd->up_hp); wr_u8(&o, (u8)bd->up_atk); wr_u8(&o, (u8)bd->up_def); wr_u8(&o, (u8)bd->skills);
        wr_u8(&o, bd->fainted? 1:0);
        wr_flt(&o, bd->walk_ph);
        wr_u8(&o, (u8)bd->name_idx); wr_u8(&o, (u8)bd->color);
        wr_u8(&o, (u8)bd->energy);
        wr_u8(&o, (u8)(bd->on_wall+1));
    }
    plat_net_send(pkt, o, -1);
}
static void net_host_poll(void){
    u8 buf[2048]; int src=0;
    for (int iter=0; iter<10; iter++){
        int n = plat_net_recv(buf, 2048, &src);
        if (n<=0) break;
        if (buf[0]=='J'){
            // find the sender's slot or a free one
            int slot=-1;
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src) slot=i;
            if (slot<0) for (int i=1;i<MAXP;i++) if (!G.p[i].connected){ slot=i; break; }
            if (slot<0) continue;
            char nm[13]; int ni=0;
            for (int k=0;k<12 && 1+k<n;k++){ char c=(char)buf[1+k]; nm[ni++]= c? c:' '; }
            nm[ni]=0; while (ni>0 && nm[ni-1]==' ') nm[--ni]=0;
            if (!G.p[slot].connected){
                player_fresh(&G.p[slot], nm[0]?nm:"DEV", slot, rng_range(-40,40), 80);
                G.p[slot].connected=1;
            }
            if (n>=15){ G.p[slot].whale_own = buf[13]; G.p[slot].whale_on = buf[14];
                if (n>=17){ G.p[slot].whale_own |= buf[15]<<8; G.p[slot].whale_on |= buf[16]<<8; } }
            pkt_src_token[slot]=src;
            int o=0; wr_u8(&o,'A'); wr_u8(&o,(u8)slot);
            plat_net_send(pkt, o, src);
            host_send_roster(-1); host_send_walls(src);
        } else if (buf[0]=='I'){
            int slot=-1;
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src) slot=i;
            if (slot<0 || n<8) continue;
            int o=1;
            G.p[slot].net_keys = rd_u8(buf,&o);
            G.p[slot].net_aim = rd_flt(buf,&o);
            int wp = rd_u8(buf,&o);
            if (wp<NWEAP && G.p[slot].owned[wp]) G.p[slot].weapon=wp;
            int act = rd_u8(buf,&o);
            if (act&1) G.p[slot].want_wall=1;
            if (act&2){ /* interact: handled as buy context by client UI */ }
        } else if (buf[0]=='B' && n>=2){
            int slot=-1;
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src) slot=i;
            if (slot>=0) try_buy(slot, buf[1]);
        } else if (buf[0]=='M' && n>=3){
            int slot=-1;
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src) slot=i;
            if (slot>=0) station_action(slot, buf[1], buf[2]);
        } else if (buf[0]=='E'){
            int slot=-1;
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src) slot=i;
            if (slot>=0){
                Player *p=&G.p[slot];
                if (!p->in_mall && near_store(p)) mall_enter(slot);
                else if (p->in_mall && on_exit_mat(p)) mall_exit(slot);
            }
        } else if (buf[0]=='T' && n>=2){
            int slot=-1;
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src) slot=i;
            if (slot>=0) G.p[slot].in_store = buf[1]? 1:0;
        } else if (buf[0]=='L'){
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src){
                G.p[i].connected=0; host_send_roster(-1);
            }
        }
    }
}
static void net_client_poll(void){
    u8 buf[65536]; int src=0;
    for (int iter=0; iter<6; iter++){
        int n = plat_net_recv(buf, 65536, &src);
        if (n<=0) break;
        if (buf[0]=='A' && n>=2 && G.my_id<0){ G.my_id = buf[1]; }
        else if (buf[0]=='R'){
            int o=1; int cnt=rd_u8(buf,&o);
            for (int i=0;i<MAXP;i++) if (i!=G.my_id) G.p[i].connected=0;
            for (int k=0;k<cnt;k++){
                int id=rd_u8(buf,&o); int col=rd_u8(buf,&o);
                char nm[13]; for (int j=0;j<12;j++) nm[j]=(char)rd_u8(buf,&o); nm[12]=0;
                if (id>=0 && id<MAXP){
                    G.p[id].connected=1; G.p[id].color=col;
                    scpy(G.p[id].name, nm, 13);
                    if (id==G.my_id) G.p[id].is_local=1;
                }
            }
        } else if (buf[0]=='S'){
            int o=1; G.time = rd_flt(buf,&o);
            G.total_kills = rd_u16(buf,&o);
            G.diff_run = rd_u8(buf,&o); if (G.diff_run<0 || G.diff_run>3) G.diff_run=0;
            for (int i=0;i<MAXP;i++){
                int conn = rd_u8(buf,&o);
                if (!conn){ if (i!=G.my_id) G.p[i].connected=0; continue; }
                Player *p=&G.p[i];
                p->connected=1;
                p->x=rd_flt(buf,&o); p->y=rd_flt(buf,&o);
                { float av=rd_flt(buf,&o); if (i!=G.my_id) p->aim=av; }
                // (our own facing stays local + instant; the host echoes it
                // back in snapshots but we don't want the arm to lag)
                p->hp=rd_u16(buf,&o); p->shield=rd_u16(buf,&o);
                p->weapon=rd_u8(buf,&o); p->alive=rd_u8(buf,&o);
                int sn=rd_u16(buf,&o); p->carried=rd_flt(buf,&o); p->heaviest=rd_flt(buf,&o);
                p->kills=rd_u16(buf,&o); p->rollback=rd_u8(buf,&o);
                for (int w=0;w<NWEAP;w++) p->owned[w]=rd_u16(buf,&o);
                for (int u=0;u<5;u++) p->upg[u]=rd_u16(buf,&o);
                p->banked=rd_flt(buf,&o); p->hatstand=rd_u8(buf,&o); p->in_mall=rd_u8(buf,&o);
                p->on_wall = rd_u8(buf,&o)? 0 : -1;
                p->whale_own=rd_u8(buf,&o); p->whale_on=rd_u8(buf,&o);
                p->whale_own |= rd_u8(buf,&o)<<8; p->whale_on |= rd_u8(buf,&o)<<8;   // round 2 hi bytes
                p->top_worn = rd_u8(buf,&o); if (p->top_worn<0 || p->top_worn>TOP_N) p->top_worn=0;   // HAT TOPPERS (feat 5)
                p->stack_n=sn;
                // synthesize a plausible stack for the hat drawing
                float each = sn>0? p->carried/sn : 0;
                for (int s2=0;s2<sn && s2<MAXSTACK;s2++) p->stack[s2]=each*(0.6f+0.4f*((s2*7)%5)/4.0f);
                p->dmg_mul = 1.0f + 0.25f*p->upg[0];
                p->rate_mul = 1.0f + 0.18f*p->upg[1];
                p->shield_max = shield_for(p->upg[2]);
                p->maxhp = 100 + 25*p->upg[3];
                p->walk_ph += 0.1f;
            }
            int bn = rd_u16(buf,&o);
            smemset(G.bugs, 0, sizeof(G.bugs));
            for (int i=0;i<bn && i<MAXBUGS;i++){
                Bug *b=&G.bugs[i];
                b->alive=1;
                b->x=rd_flt(buf,&o); b->y=rd_flt(buf,&o); b->w=rd_flt(buf,&o);
                float hf=rd_u8(buf,&o)/255.0f; b->kind=rd_u8(buf,&o);
                b->bp_t=rd_u8(buf,&o)? 0.2f:0; b->hit_t=rd_u8(buf,&o)? 0.1f:0;
                b->leg_ph=rd_flt(buf,&o);
                b->elite=rd_u8(buf,&o); if (b->elite<0 || b->elite>ELITE_N) b->elite=0;   // NAMED ELITES (feat 8)
                b->maxhp=100; b->hp=hf*100;
                b->seed=(u32)(i*7919);
            }
            int evc = rd_u8(buf,&o);
            for (int i=0;i<evc;i++){
                int t=rd_u8(buf,&o); float ex=rd_flt(buf,&o), ey=rd_flt(buf,&o);
                if (t==1) splat(ex, ey, BUG_COL[i%4], 8, 110);
                if (t==2) splat(ex, ey, COL(170,120,70), 10, 110);
                if (t==3) sfx(11);
                if (t==4) sfx(5);
            }
            shield_remote = rd_u8(buf,&o);
            { int agn = rd_u8(buf,&o); (void)agn; }   // agents retired: count byte discarded
            { int bn2 = rd_u8(buf,&o);
              for (int i=0;i<MAXP;i++) G.bud[i].active=0;
              for (int i=0;i<bn2;i++){
                  int oi=rd_u8(buf,&o); int sp=rd_u8(buf,&o);
                  float bx=rd_flt(buf,&o), by=rd_flt(buf,&o);
                  int hf=rd_u8(buf,&o); int lu=rd_u8(buf,&o);
                  int xp=rd_u16(buf,&o);
                  int uh=rd_u8(buf,&o), ua=rd_u8(buf,&o), ud=rd_u8(buf,&o), sk=rd_u8(buf,&o);
                  int fn=rd_u8(buf,&o);
                  float wp=rd_flt(buf,&o);
                  int nm2=rd_u8(buf,&o); int cl2=rd_u8(buf,&o); int en2=rd_u8(buf,&o);
                  int ow2=rd_u8(buf,&o);
                  if (oi>=0 && oi<MAXP){
                      Buddy *bd=&G.bud[oi];
                      bd->active=1; bd->species=sp%BUDN; bd->x=bx; bd->y=by;
                      bd->on_wall = ow2-1;
                      bd->up_hp=uh; bd->up_atk=ua; bd->up_def=ud; bd->skills=sk; bd->xp=xp;
                      bd->fainted=fn? 1:0;
                      bd->name_idx=nm2%BUD_NAMES_N; bd->color=cl2%BUD_COLORS_N; bd->energy=en2;
                      buddy_refresh(oi);
                      bd->hp = bd->maxhp * hf/255.0f;
                      bd->lunge_t = lu? 0.15f : 0;
                      if (wp != bd->walk_ph) bd->mv_t = 0.15f;   // a guest's buddy flops on the client's screen too
                      bd->walk_ph = wp;
                  }
              } }
            if (G.my_id>=0 && G.state==GS_MENU) G.state=GS_PLAY;
        } else if (buf[0]=='W'){
            int o=1; int cnt=rd_u16(buf,&o);
            smemset(G.walls, 0, sizeof(G.walls));
            for (int i=0;i<cnt && i<MAXWALLS;i++){
                Wall *w=&G.walls[i];
                w->x=rd_flt(buf,&o); w->y=rd_flt(buf,&o);
                w->hp=rd_flt(buf,&o); w->maxhp=rd_flt(buf,&o);
                w->horiz=rd_u8(buf,&o); w->stack=rd_u8(buf,&o); if (w->stack<1) w->stack=1;
                w->alive=1; w->grp=-1; w->grp_old=-1;
            }
            walls_recompute();   // client side: groups/tiers only, no pools
        } else if (buf[0]=='G'){
            G.state=GS_OVER; G.gameover_t=0;
        }
    }
}
static void net_client_send_input(void){
    int o=0; wr_u8(&o,'I');
    wr_u8(&o, (u8)G.p[G.my_id<0?0:G.my_id].cur_keys);
    // facing, resolved from the movement keys in resolve_local_input
    wr_flt(&o, client_aim);
    wr_u8(&o, (u8)client_weapon);
    wr_u8(&o, (u8)(client_want_wall?1:0));
    plat_net_send(pkt, o, 0);
    client_want_wall=0;
}

// ---------------------------------------------------------------------------
// Store purchases + store input
// ---------------------------------------------------------------------------
static void store_note(const char *msg){
    scpy(G.store_msg, msg, 64); G.store_msg_t = 150;
}
static void try_buy(int pi, int sel){
    Player *p=&G.p[pi];
    if (pay_active) return;
    if (G.state!=GS_STORE && G.net_mode!=NET_HOST) { /* local buys happen in store UI */ }
    if (!near_store(p) && !p->in_store){
        if (pi==G.my_id) store_note("YOU MUST BE AT THE STORE");
        return;
    }
    int owned; int price = store_item_price(sel, p, &owned);
    if (owned){
        if (pi==G.my_id){ store_note(sel<5? "FULLY STACKED"
                                          : (sel==11? "ALREADY AT FULL HP" : "FULLY UPGRADED")); sfx(16); }
        return;
    }
    if (p->carried < (float)price){
        if (pi==G.my_id){
            char b[64]; char *q=b;
            q=sappend(q, "NOT ENOUGH HAT - NEED ");
            q=sapp_int(q, (int)(price - p->carried)); q=sappend(q, " KG MORE");
            store_note(b); sfx(16);
        }
        return;
    }
    start_payment(pi, sel<5?0:1, sel<5? sel : sel-5, price);
}
static void client_tell_store(int open){
    // let the host know we're browsing, so the Epicenter shield goes up
    int o=0; wr_u8(&o,'T'); wr_u8(&o,(u8)open); plat_net_send(pkt, o, 0);
}
// drive one player's store browsing. The nav inputs come from whoever
// owns the store session: P1's keyboard/pad-1 fields, or P2's pad-2
// edges in local co-op — each shopper keeps their own cursor.
static void store_input(int pi, int up, int down, int activate){
    int *selp = (G.p2_slot>=0 && pi==G.p2_slot)? &G.store_sel2 : &G.store_sel;
    if (up){ *selp = (*selp + STORE_ROWS-1)%STORE_ROWS; sfx(6); }
    if (down){ *selp = (*selp + 1)%STORE_ROWS; sfx(6); }
    // touch: tap a row to select it, tap the selected row to buy it,
    // tap anywhere outside the panel to leave the store (P1 only)
    if (pi==G.my_id && IN.tapped){
        if (IN.tap_x<STORE_X-6 || IN.tap_x>STORE_X+STORE_W+6 ||
            IN.tap_y<STORE_Y-6 || IN.tap_y>STORE_Y+STORE_H+6) G.tap_close_store=1;
        else for (int i=0;i<STORE_ROWS;i++){
            int ry = STORE_ROW0 + (i<5? i*STORE_ROWH : i*STORE_ROWH+STORE_UPGOFF);
            if (IN.tap_y>=ry-5 && IN.tap_y<=ry+STORE_ROWH-2){
                if (i==*selp) activate=1;
                else { *selp=i; sfx(6); }
                break;
            }
        }
    }
    if (activate){
        if (G.net_mode==NET_CLIENT){
            int o=0; wr_u8(&o,'B'); wr_u8(&o,(u8)*selp);
            plat_net_send(pkt, o, 0);
            sfx(10);
        } else try_buy(pi, *selp);
    }
}

// ---------------------------------------------------------------------------
// Mall stations — the panels for the Gallery, the Bug Bank and the
// Banzai Buddies store. Station 0 is the Bug Bounty counter and keeps
// the original store panel + input. Everything buys with hat weight or
// buddy XP, through the same one-at-a-time till.
// ---------------------------------------------------------------------------
#define HATSTAND_PRICE 150
// a revive is priced off the buddy itself (Daniel, Nov 2): its
// species, its training, and its taught moves — a fresh pet rock
// is cheap to wake, a trained moose with both moves is not. A
// faint never costs the buddy anything it earned: training, XP
// and skills all ride through untouched (see hurt_buddy).
static int bud_revive_price(Buddy *b){
    int skills = (b->skills&1) + ((b->skills>>1)&1) + ((b->skills>>2)&1) + ((b->skills>>3)&1);
    return 10 + BUD_PRICE[b->species%BUDN]/5
         + 6*(b->up_hp + b->up_atk + b->up_def) + 15*skills;
}
#define MALL_ROW0 (STORE_ROW0 + 26)
// training prices (Daniel): the first lessons are cheap; mastery is a
// fortune. Flat 12/24/36... became roughly x1.45 a level past the basics.
static int bud_train_cost(int lvl){
    float c = 10.0f;
    for (int i=0;i<lvl;i++) c = c*1.45f + 2.0f;
    return (int)(c + 0.5f);
}
