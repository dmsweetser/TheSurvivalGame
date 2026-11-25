// THE SURVIVAL GAME — mall/bowl_state.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 245-410 in the Nov 2026 fine-grained refactor; code unchanged.

// BUG BOWL: one player's bowling game at the mall lanes. Lane space:
// x across (-60..60 is the lane, beyond is gutter), y down-lane from
// the foul line (0) to the back of the deck (~390). Full bowling
// scoring: strikes, spares, and a three-ball tenth frame.
typedef struct {
    int active, game_over, final;
    int frame, ball, ri_frame;        // ri_frame: roll index where this frame began
    int rolls[21]; char marks[21]; int nrolls;
    int phase;                        // 0 position 1 angle 2 power 3 rolling 4 settle 5 over 6 attract
    float thrower_x, angle; int angle_dir;
    float power; int power_dir;
    float bx, by, bvx, bvy; int bl_live, bl_gutter, bl_kind;
    int pins_before;
    int strikes;                      // strikes this game (10 on a full rack), counted in bowl_record
    float settle_t, msg_t;
    char msg[40];
    int act;                          // queued FIRE edge from station_input
    int standing[10];
    float pfx[10], pfy[10], pfvx[10], pfvy[10], pft[10];   // toppling pins
} Bowl;

// BUG BOWL LEAGUE (v1.25 feat 10): one player's league book at the
// lanes — a career line plus a six-night season against three house
// rivals whose scores are a pure hash2 function of (season, night,
// rival), so only the PLAYER's side ever needs saving. The core
// (calibration, finalize) lives in mall.c beside bowl_record; the
// persistence is one pl-line tail token (bl=) in save.c. See
// v125-feat10-spec.md.
typedef struct {
    int games, best, pins, strikes;      // career: games, best game, total pins, total strikes
    int season, night, spins;            // season >= 1 once started; night = nights completed
                                         // this season (0..5 between games); spins = your pins
    int trophies;                        // championships won
    int last_score, last_place, last_night, last_season;  // last night's result (place 1..4)
    int fin_place, fin_season;           // last CLOSED season's place + its number (0 = none)
} BowlLeague;

typedef struct { int alive; float x,y,vx,vy,life,maxlife; int size; u32 col; } Particle;
typedef struct { int alive; float x,y,life; char text[24]; u32 col; } Floater;
// a carcass in flight (to a head, to the store, or to a wall site)
typedef struct {
    int alive, mode;                  // 0 to head, 1 to store, 2 to wall site
    float x0,y0, x1,y1, t, dur, w;
    int player, wall_slot;
} CarcassFx;

enum { GS_MENU=0, GS_PLAY=1, GS_STORE=2, GS_OVER=3, GS_HELP=4, GS_PAUSE=5, GS_SETTINGS=6, GS_SLOTS=7, GS_BWORLD=8 };
enum { NET_OFF=0, NET_HOST=1, NET_CLIENT=2 };

typedef struct {
    int state, prev_state;
    int net_mode, my_id;
    float time;                       // run elapsed
    Player p[MAXP];
    Bug bugs[MAXBUGS];
    Proj projs[MAXPROJ];
    Wall walls[MAXWALLS];
    Particle parts[MAXPART];
    Floater floats[MAXFLOAT];
    CarcassFx fx[MAXP*24 + 64];
    float camx, camy, shake;
    float threat;                     // world difficulty x1.0+, from player power
    float zoom;                       // camera zoom (1 = normal, <1 = zoomed out)
    float spawn_t, autosave_t, net_t, music_t;
    int total_kills;
    float total_earned;               // kg cashed in, all time this run
    int menu_sel, store_sel, store_msg_t;
    int store_sel2;                   // co-op: P2's own store cursor
    int store_pi;                     // whose store is being drawn/driven
    int store_station;                // mall station of the open panel (0 = the shop)
    Buddy bud[MAXP];                  // one Banzai Buddy per player slot
    Bowl bowl[MAXP];                  // one Bug Bowl game per player slot
    BowlLeague blg[MAXP];             // BUG BOWL LEAGUE (feat 10): one league book per player slot
    int coop_pref;                    // 2P local co-op wanted (persisted)
    int p2_slot;                      // player index of the local P2, -1 none
    int view_x0, view_w, view_pi;     // split-screen: current render view
    int view_y0, view_h;              // (y twin: horizontal splits stack views)
    int cam_lock;                     // split pass: cam/zoom set by the caller
    int ui_compact;                   // store panel squeezed into a split half
    int tap_close_store;              // mobile: tapped outside the store panel
    char store_msg[64];
    float gameover_t;
    int bind[14];                     // action -> physical key id (see PK_*)
    int touch_ctl;                    // FOSSIL (v1.33): the mobile scheme field — the drag scheme is removed, the joystick is the only deck; pinned to 1 at init/load, kept so the save token round-trips
    int zen, zen_run;                 // zen preference + the running game's zen flag
    int eddie;                        // EDDIE MODE: bugs render as fuzzy orange puffballs (cosmetic)
    int whale_own, whale_on;          // WHALES ONLY: this user's unlocks (persisted globally)
    int difficulty, diff_run;         // 0 EASY (= the game as it was) 1 MEDIUM 2 HARD 3 BUGAGGEDON
    int set_sel, set_capture;         // settings screen: row, awaiting a key
    // COMFORT (v1.25 feature 2): persisted audio/shake preferences,
    // edited on the settings screen above the key bindings and saved
    // in the global blob beside them. Volumes are 0..10 (10 = the
    // mix exactly as shipped); mute_all silences both audio buses;
    // shake_on gates the screen-shake effect game-wide.
    int vol_music, vol_sfx, mute_all, shake_on;
    // records (persisted across runs)
    char rec_name[13]; float rec_hat; int rec_kills; float rec_time; int rec_bowl;
    int has_save;
    int menu_pad;                     // title-menu layout latch: 0 unset, 1 keyboard rows, 2 pad rows
    // save slots: the run lives in one of three slot blobs; records and
    // settings stay in the global blob. slot = the slot this run uses.
    int slot; int slot_has[4];
    int slots_mode, slots_sel, slots_confirm;   // slot picker UI state
    int pause_sel;                              // pause menu selection
    int pause_ctl;                              // pause menu: the CONTROLS panel is open
    // FIRST SHIFT (v1.25): the guided first-session chain. Per-slot
    // state (it rides the run's slot blob): 0 = off (a save from
    // before the chain existed, or a run that never armed it),
    // 1 = active, 2 = done, 3 = skipped. fs_fp0 is the purchase
    // fingerprint captured when the chain armed. fs_pool holds a
    // player's step XP until they adopt a buddy — the award door
    // pays buddies only, and the reward must not evaporate for a
    // player who hasn't adopted one yet.
    int fs_state, fs_step, fs_fp0;
    int fs_pool[MAXP];
    // WEEKLY CHALLENGE (v1.25 feature 6): one goal a week, a pure
    // function of the week id, tracked per slot. wk_week is the week
    // this slot's challenge belongs to (0 = never initialized: a
    // pre-feature save before its first rollover, or a LAN client —
    // the host owns the week). wk_done_week is the week of the most
    // recent completion ("done this week" is derived: done_week ==
    // wk_week); wk_total counts completions all-time (no streaks —
    // misses are never stored as anything but the prev row's fact).
    // wk_prev_week/wk_prev_done remember the outgoing week at the
    // last rollover for the board's PREVIOUS row. wk_base[] holds
    // the week's opening snapshot of the seven delta sources.
    int wk_week, wk_done_week, wk_total, wk_prev_week, wk_prev_done;
    int wk_base[7];
    // NAMED ELITES + BOUNTY BOARD (v1.25 feature 8): one posted
    // bounty per slot, naming one elite. bn_serial is the contract
    // counter — the elite's name and kind are a pure function of it
    // (hash2, in buddies.c below), so the save needs nothing else.
    // bn_done counts claims all-time (the board's ledger). bn_cool
    // is the post-claim cooldown in seconds of play: while it runs,
    // the next name is posted but its bug cannot crawl out yet.
    // The bn_claim_* fields are the hand-off from kill_bug (which
    // runs deep inside combat) to bn_update (save.c), which settles
    // the claim on the frame tail. bn_live / bn_live_reg /
    // bn_last_reg are runtime tracking for the board's LAST SEEN
    // row and the pause line — never saved (the board honestly
    // forgets where the elite was across a load).
    int bn_serial, bn_done;
    float bn_cool;
    int bn_claim_pending, bn_claim_pi, bn_claim_name;
    float bn_claim_w;
    int bn_live, bn_live_reg, bn_last_reg;
    // HAT PORTRAIT BOOTH (v1.25 feature 9): the shell hand-off for
    // the PNG. station_action freezes the portrait and arms the
    // booth draw; the draw — the one actually showing the pose, in
    // whichever view drew it — fills these stage-coordinate fields
    // and bumps port_req_serial; the shell polls portrait_info and
    // crops exactly this rectangle out of its frame. Written only
    // by the booth draw, so the rect is honest in every layout.
    int port_req_serial;
    int port_req_x, port_req_y, port_req_w, port_req_h;
    char port_req_name[16];
    int port_req_kg;
    // menu text fields, filled by the platform (name / join IP)
    char my_name[13];
    char join_ip[48];
    int edit_field;                   // 0 none, 1 name, 2 ip
    int key_edges;                    // key bits that went down this frame
    float anim_t;                     // global animation clock
    u32 run_seed;
} Game;
static Game G;
