// THE SURVIVAL GAME — game/update.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 1569-1589 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// Update logic (runs on the host / in single player)
// ---------------------------------------------------------------------------
static float beam_vis[MAXP];
static float shield_frac[MAXP];   // fractional firewall-shield regen, carried between frames
static float whale_spark_t[MAXP]; // Whales Only sparkle-trail emitter timers
static float wof_spark_t[MAXP];   // WALK OF FAME confetti emitter timers
static const u32 WOF_COL[6] = { COL(255,90,120), COL(255,200,60), COL(90,220,120),
                                COL(80,180,255), COL(200,120,255), COL(255,140,60) };          // Debugger beam render timer per player
static float beam_ang[MAXP][MAXBEAMS]; // where each stacked beam points
static int beam_n[MAXP];              // how many beams (Debugger copies)
static int beam_len[MAXP];            // beam reach (grows per copy)
static float beam_frac[MAXP];         // current reach fraction (extend/retract)
static int beam_phase[MAXP];          // sweep state: 0 idle 1 extend 2 spin 3 retract
static float beam_t[MAXP];            // time inside the current sweep phase
static float beam_base[MAXP];         // aim captured when the sweep began
static float beam_tick[MAXP];         // damage-tick accumulator while beams are live
static int   wall_build_n[MAXWALLS];  // carcasses arrived at a building wall
static void do_save(void);            // forward: save section
static void save_records(void);       // forward: save section

