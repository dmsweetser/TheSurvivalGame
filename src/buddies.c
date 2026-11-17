// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: buddies.c — game design data, entity helpers, buddy species & powers, world props, bug stats, difficulty, combat, update logic, interlocked walls.
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]
// ---------------------------------------------------------------------------
// Game design data
// ---------------------------------------------------------------------------
#define MAXP 4            // players (1 local + up to 3 LAN)
#define MAXBUGS 240
#define MAXPROJ 320
#define MAXWALLS 420
#define MAXPART 500
#define MAXFLOAT 48
#define MAXSTACK 420      // carcasses carried per player
#define WORLD_SEED 1337u

// Weapons: 0 Keyboard, 1 Mouse, 2 Duck, 3 PM Launcher, 4 Debugger
#define NWEAP 5
static const char *WEAPON_NAME[NWEAP] = {
    "MECHANICAL KEYBOARD", "MOUSE TOSSER", "RUBBER DUCK CANNON",
    "PM LAUNCHER", "THE DEBUGGER"
};
static const char *WEAPON_BLURB[NWEAP] = {
    "Fires keycaps. Clickety-clack of doom.",
    "Throws mice. Batteries not included.",
    "Ducks bounce bug to bug. Quack.",
    "Lobs a Project Manager. Huge AoE synergy.",
    "Fire: beams extend, spin once, close. Copies add beams."
};
static const int WEAPON_PRICE[NWEAP] = { 0, 25, 60, 120, 200 };
static float weap_dmg(int w){ return w==0?6.0f : w==1?24.0f : w==2?15.0f : 65.0f; }
static float weap_cd(int w){ return w==0?0.115f : w==1?0.38f : w==2?0.30f : 1.05f; }
// weapons stack: owned[w] is a COPY COUNT (0 = not owned), UNCAPPED —
// stack as high as you can afford. Extra copies fan their shots out
// across your front; the Debugger adds beams in a ring around you.
// Copy N costs N x the base price (see store_item_price).
#define MAXCOPIES 999
#define MAXBEAMS 48           // Debugger ring rendering/simulation cap
// The Debugger fires as ONE SWEEP, not a held hose (Daniel, Oct 31):
// beams extend, spin a single revolution, then close, then rest. The
// old held beam froze + shoved everything it touched every 0.07s, so a
// full ring of copies was an untouchable fountain — nothing could ever
// close in and the run never ended. The dead time between sweeps is
// what lets the swarm back in.
#define BEAM_EXT  0.18f       // beams grow to full reach
#define BEAM_SPIN 0.85f       // one full revolution
#define BEAM_RET  0.15f       // beams close (no damage while closing)
#define BEAM_REST 0.55f       // dead time before the next sweep

// Store upgrade tracks: dmg, rate, shield, armor, speed, rollback,
// hotfix. Slot 7 was the AI AGENT; the robot retired when Banzai
// Buddies took the job, and the slot now sells THE BACKFIRE BEAN
// (Krista's idea, Nov 5): a very expensive little fellow who flies
// you home to the Epicenter. His fuel is his own business.
#define NUPG 8
static const char *UPG_NAME[NUPG] = {
    "OVERCLOCK (DMG+)", "CAFFEINE DRIP (RATE+)", "FIREWALL SHIELD",
    "HOODIE ARMOR (HP+)", "ESPRESSO LEGS (SPD+)", "ROLLBACK TOKEN",
    "HOTFIX (HEAL 50)", "THE BACKFIRE BEAN"
};
static const int UPG_PRICE[NUPG][4] = {   // price per level, -1 = no more
    { 40, 90, 180, 360 },   // dmg +25%/lvl
    { 35, 80, 170, 340 },   // rate +18%/lvl
    { 50, 110, 230, -1 },   // shield cap 30/60/100
    { 45, 95, 200, -1 },    // +25 max hp / lvl
    { 30, 75, -1, -1 },     // speed: heavy hats slow you less / lvl
    { 150, 150, 150, 150 }, // consumable revive token (rebuyable)
    { 20, -1, -1, -1 },     // hotfix: consumable heal, rebuyable
    { 500, 500, 500, 500 }, // slot 7: THE BACKFIRE BEAN — flat 500, always in stock
};
static const int UPG_MAXLV[NUPG] = { 999, 999, 999, 999, 999, 999, 0, 999 };
static int shield_for(int lv){ return lv<=0?0 : lv==1?30 : lv==2?60 : 100 + 40*(lv-3); }
// upgrade prices past the printed table: keep climbing off the top
// rung — the top price times how far past it you've climbed.
static int upg_price(int u, int lv){
    int last = -1, li = 0;
    for (int i=0;i<4;i++) if (UPG_PRICE[u][i]>0){ last = UPG_PRICE[u][i]; li = i; }
    if (lv<=li) return UPG_PRICE[u][lv];
    return last * (lv - li + 1);
}

typedef struct {
    int alive, connected, is_local;
    int in_store;                     // browsing the Bug Bounty (raises the shield)
    char name[13];
    float x, y, aim, walk_ph;
    int hp, maxhp, shield, shield_max;
    float shield_regen_t;
    float fire_cd, hurt_t, dead_t, revive_t;   // revive_t: post-rollback grace
    int on_wall;                      // interlock group he's standing on, -1 = ground
    float shed_cd;                    // bite-sheds-hat cooldown (endgame pressure)
    float idle_t;                     // time since last movement (idle anim)
    float wof;                        // WALK OF FAME hold timer (mall crawl parade)
    int in_mall;                      // inside the Bug Mall interior
    int in_arcade;                    // inside the Arcade room (in_mall stays 1: same safe zone)
    int station;                      // mall station whose panel is open, -1 none
    float banked;                     // kg stored at the Bug Bank (safe, not a record)
    int hatstand;                     // owns the Psychic Hat Stand
    int whale_own, whale_on;          // Whales Only: owned / switched-on vanity bits
    int weapon;                       // current
    int owned[NWEAP];
    int upg[NUPG];
    int rollback;                     // revive tokens held
    int beans;                        // BACKFIRE BEANS held (the ride home)
    float bean_t;                     // >0 while a bean ride is playing out
    float bean_jet;                   // >0 while jetpacking on a bean (epicenter launches)
    float jet_fall;                   // brief tumble after a jet ends
    // the hat: a stack of carcass weights (kg). This is also the wallet.
    float stack[MAXSTACK];
    int stack_n;
    float carried;                    // total kg on head
    float heaviest;                   // heaviest hat this run
    int kills;
    float dmg_mul, rate_mul;          // derived from upgrades
    int color;                        // hoodie palette index
    float net_aim; int net_keys;      // last input from net (host side)
    int cur_keys; float cur_aim;      // resolved input this frame (any source)
    int want_wall, want_buy;          // edge-triggered actions (net or local)
    float wall_cd;
    float tx, ty;                     // render interpolation target (client)
} Player;

typedef struct {
    int alive, kind;                  // kind: 0 roach 1 beetle 2 spider 3 moth
    float x, y, vx, vy, w;            // w = weight kg (drives size/hp/value)
    float hp, maxhp, speed, dmg;
    float leg_ph, wobble, hit_t, bp_t; // bp_t: breakpoint pause (Debugger)
    int target_wall;                  // wall index being chewed, -1 none
    int btarget;                      // owner index of the Banzai Buddy it's chasing, -1 none
    int aggro;                        // bot that has hit this bug and drawn
                                      // its ire, -1 = it hunts the players
    float chew_t;
    float stuck_t, grease_t; int grease_s;   // anti-stuck grease (Nov 10): slide around props
    u32 seed;
} Bug;

typedef struct {
    int alive, kind, owner;           // kind = weapon id
    float x, y, vx, vy, dmg, life;
    int bounces, target;              // duck chaining
    int agent;                        // who fired it: -1 = a player, -2 = a buddy
    float spin;
} Proj;

typedef struct {
    int alive, horiz;                 // horiz: segment runs E-W else N-S
    float x, y, hp, maxhp;
    int stack;                        // walls piled on this spot (reinforcement layers), >=1
    int grp;                          // interlock group leader index, -1 none
    int grp_old;                      // group before the last recompute (pool shares)
    int endpt;                        // end of a run: <=1 neighbour in its group
} Wall;

// (The AI AGENT robot clones were retired on Nov 2 — Daniel: the
// Banzai Buddies replace them. The system is gone end to end.)

// BANZAI BUDDY: a fighting companion bought at the mall — puppy, kitty
// or moose. Follows its owner, fights beside them, earns XP from kills,
// and spends it on training and taught moves at the buddy store.
typedef struct {
    int active, species;              // 0 puppy 1 kitty 2 moose 3 platypus 4 pet rock 5 kitchen sink
    float x, y, aim, walk_ph;
    float mv_t;                       // >0 while actually moving (drives the rigid walk flop in draw_buddy)
    int on_wall;                      // interlock group he's standing on, -1 = ground (mirrors Player)
    float hp, maxhp;
    int xp;
    int up_hp, up_atk, up_def;       // training levels (bought with XP)
    int skills;                       // bit0: move 2 taught, bit1: special taught
    int fainted;                      // out cold at 0 HP — never dead; the shop revives
    int name_idx;                     // which Elder-Days name it answers to
    int color;                        // coat: 0 natural, 1-8 standard, 9 neon rainbow
    float atk_cd, move_cd, spec_cd, lunge_t, hurt_t, down_t;
    float energy;                     // special attacks spend it; it refills as you go (and fully on sleep)
    float move3_cd, move4_cd;
    float bw_x, bw_y;                 // where this buddy stands in Banzai World
    int in_bw;                        // currently adventuring in Banzai World (per-player mode)
    int bw_mode;                      // this trip: 0 free play, 1 story mode
    int story_started, story_bits;    // story progress: 2 bits per quest (0 unseen, 1 active, 2 done); farm kills in bits 8-11
    int story_bits2;                  // quests 6-9 state + their flags (Nov 4 story expansion); saved as sq2, defaults 0 on old saves
    int ent_best, ent_done;           // ENTRANCES records (Nov 5): deepest room reached, runs finished; saved packed as er
    int ent_sub_best;                 // SUBFLOOR record: deepest B-room reached; saved as sb
    int plat_done;                    // SUPER BUDDY RUN record: levels completed; saved as pd
    float craft_x, craft_y;           // BUDDY CRAFT: this buddy's standing spot in the endless wild; saved as cx/cy
    int craft_wood, craft_stone, craft_fiber, craft_car, craft_pies, craft_gear;   // BUDDY CRAFT pack; saved as cm/cn/cg
    int craft_stews;                  // BUDDY CRAFT: stews in the pack (rides the cn token)
    int craft_fish;                   // BUDDY CRAFT: lake fish in the pack (saved as cf)
    float craft_tod;                  // BUDDY CRAFT: time of day, 0..240s cycle (day, dusk, night, dawn); saved as ct
    int craft_bn;                     // BUDDY CRAFT: placed builds — they PERSIST (Daniel: leaving used to wipe them); saved as cw
    int craft_btx[48], craft_bty[48], craft_bk[48], craft_bhp[48];   // build tiles, kind, chew hp left
    int craft_gn;                     // BUDDY CRAFT: mined-out node keys (persist too); saved as cd
    int craft_gone[96];
    int items[4];                     // BW-only consumables: 0 Patch Notes, 1 Energy Drink, 2 Adrenal Cookie, 3 Ctrl-Z
    int bw_seed, bw_started;          // its own procedural world, once begun
    float calm;                       // seconds since last hurt (drives the regen)
    float stuck_t, grease_t; int grease_s;   // anti-stuck grease (Nov 10): slide around props
    int tank_done;                    // TANK RESCUE record: deepest level cleared; saved as td
    int cas_best;                     // CASTLE record: best escape score; saved as cb
    int crt_best_pos, crt_best_time;  // SUPER BUDDY CART records: best finish (0 = none) as cp, best race time in deciseconds as cq
    int crt_cupbest;                  // best Grand Prix rank per cup, packed: home + far*8 (0 = unranked), as cr
    int war_prog;                     // WAR! record: regions conquered (0..8); the world map unlocks region war_prog next, as wr
    float roll;                       // the pet rock's rolling angle
    float steady_t;                   // pet rock's ROCK STEADY: unhurtable while > 0
    float tx, ty;                     // render interpolation target (client)
} Buddy;

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
    float settle_t, msg_t;
    char msg[40];
    int act;                          // queued FIRE edge from station_input
    int standing[10];
    float pfx[10], pfy[10], pfvx[10], pfvy[10], pft[10];   // toppling pins
} Bowl;

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
    int touch_ctl;                    // mobile control scheme: 0 = drag, 1 = virtual d-pad
    int zen, zen_run;                 // zen preference + the running game's zen flag
    int eddie;                        // EDDIE MODE: bugs render as fuzzy orange puffballs (cosmetic)
    int whale_own, whale_on;          // WHALES ONLY: this user's unlocks (persisted globally)
    int difficulty, diff_run;         // 0 EASY (= the game as it was) 1 MEDIUM 2 HARD 3 BUGAGGEDON
    int set_sel, set_capture;         // settings screen: row, awaiting a key
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
    // menu text fields, filled by the platform (name / join IP)
    char my_name[13];
    char join_ip[48];
    int edit_field;                   // 0 none, 1 name, 2 ip
    int key_edges;                    // key bits that went down this frame
    float anim_t;                     // global animation clock
    u32 run_seed;
} Game;
static Game G;
static void refresh_has_save(void);
static int want_autostart = 0;   // shell asks to skip the menu (hosted page)

// Physical keys — the ONLY keyboard vocabulary the platform layers speak
// now. Each backend reports which physical keys are held (IN.raw); the
// core turns them into game actions through the player's key bindings,
// so every build has exactly the same controls. Letters A-Z are 0-25,
// digits 0-9 are 26-35, then the named keys.
enum {
    PK_UP=36, PK_DOWN, PK_LEFT, PK_RIGHT, PK_SPACE, PK_ENTER, PK_ESC,
    PK_SHIFT, PK_BACKSPACE, PK_COUNT
};
// Game actions. The first 14 are rebindable in Settings; WPREV/WNEXT/ESC
// exist so gamepads can speak in actions directly.
enum {
    ACT_UP=0, ACT_DOWN, ACT_LEFT, ACT_RIGHT, ACT_FIRE, ACT_INTERACT,
    ACT_WALL, ACT_PAUSE, ACT_SPRINT, ACT_W1, ACT_W2, ACT_W3, ACT_W4,
    ACT_W5, ACT_WPREV, ACT_WNEXT, ACT_ESC, ACT_ENTER, ACT_BEAN, ACT_COUNT
};
static const char *act_name(int a){
    static const char *n[] = { "MOVE UP","MOVE DOWN","MOVE LEFT","MOVE RIGHT",
        "FIRE","INTERACT","BUILD WALL","PAUSE","SPRINT",
        "WEAPON 1","WEAPON 2","WEAPON 3","WEAPON 4","WEAPON 5" };
    return n[a];
}
static const char *key_name(int pk){
    static char b[8];
    if (pk>=0 && pk<26){ b[0]=(char)('A'+pk); b[1]=0; return b; }
    if (pk>=26 && pk<36){ b[0]=(char)('0'+pk-26); b[1]=0; return b; }
    switch (pk){
        case PK_UP: return "UP"; case PK_DOWN: return "DOWN";
        case PK_LEFT: return "LEFT"; case PK_RIGHT: return "RIGHT";
        case PK_SPACE: return "SPACE"; case PK_ENTER: return "ENTER";
        case PK_ESC: return "ESC"; case PK_SHIFT: return "SHIFT";
        case PK_BACKSPACE: return "BKSP";
    }
    return "?";
}
static void bind_defaults(int *bind){
    bind[ACT_UP]= 'W'-'A'; bind[ACT_DOWN]='S'-'A';
    bind[ACT_LEFT]='A'-'A'; bind[ACT_RIGHT]='D'-'A';
    bind[ACT_FIRE]=PK_SPACE; bind[ACT_INTERACT]='E'-'A';
    bind[ACT_WALL]='F'-'A'; bind[ACT_PAUSE]='P'-'A';
    bind[ACT_SPRINT]=PK_SHIFT;
    for (int w=0;w<5;w++) bind[ACT_W1+w]=26+1+w;   // digits 1-5
}

// input, filled by the platform each frame
typedef struct {
    u64 raw;                          // held physical keys (PK_* bits)
    u64 edges_in;                     // keydown events since last frame:
                                      // lossless edges (a tap between two
                                      // frames still counts exactly once)
    u64 pulse;                        // one-frame key taps (touch buttons)
    u32 act_held, act_edges;          // gamepad actions, in ACT_* space
    u32 act2_held, act2_edges;        // second gamepad (local co-op P2)
    float pad_ax[4];                  // pad 1's analog sticks, raw -1..1 in web-axes order (LX, LY, RX, RY; +Y is DOWN) — WAR!'s cursor flies on these (v1.16)
    u32 pad_edges;                    // this frame's act_edges, snapshotted by derive_input before it clears them (WAR!'s pad buttons read here)
    int pad_count;                    // controllers the platform can see
    float dev_t, pad_last, key_last;  // last-device stamps: which input the player is actually USING (drives hint labels)
    int weapon_cycle;                 // derived: -1/0/+1 from gamepad bumpers
    int keys;                         // bits below
    float mx, my;                     // mouse in screen coords
    int mdown, mclicked, rclicked;
    char typed[16]; int typed_n;      // characters typed this frame (for fields)
    int backspace;
    int weapon_key;                   // 0 none, else 1..5 pressed this frame
    int bean_key;                     // the Backfire Bean trigger (key 6 / BEAN tap)
    int wall_pressed, interact_pressed, esc_pressed;
    int enter_pressed;                // Enter key specifically (E is interact;
                                      // menus need to tell them apart so you
                                      // can type the letter E into a field)
    int select_pressed;               // Space (the select key) or Enter, one frame
    int pause_pressed;                // P key, one frame (single-player pause)
    int touch_x, touch_y, touch_down; // mobile web: finger pos (fb coords) + held
    int touch_seen;                   // mobile web: touch used at least once (prompt wording)
    int mobile_boot;                  // mobile web: shell reported a touch-first device at boot
    int tap_x, tap_y, tapped;         // mobile web: a quick tap landed this frame
    struct { int active, id, x, y, x0, y0, ax, ay, moved, zone; float age; } tp[6];  // live touch points (ax/ay: joystick anchor)
    int touch_keys;                   // stick scheme: K_* bits held via on-screen controls
    int tp_act_edge, tp_wpn_edge, tp_wall_edge;   // button taps, latched for the next frame
    int tp_held;                      // button zones currently held (for rendering)
    int joy_on, joy_ax, joy_ay, joy_kx, joy_ky;   // floating joystick render state
    int menu_up, menu_down;           // ARROW keys only, as one-frame edges.
                                      // Menus navigate with these, never with
                                      // the WASD bits — otherwise typing a
                                      // name like WES would steer the menu.
} Input;
static Input IN;
// Touch-first presentation: no keyboard exists, so the Settings (key
// binding) screen is hidden and every instruction is written for taps.
static int mobile_ui(void){ return IN.touch_seen || IN.mobile_boot; }
// gamepad presentation: the keyboard/touch control hints and the
// menu's settings (key-binding) option hide themselves — Daniel's
// rule. This used to be "a pad is plugged in", which lied constantly:
// a pad sitting connected flipped every hint to pad labels while the
// player was on keys (and vice versa). Now it follows the LAST DEVICE
// actually touched: pad buttons/sticks stamp pad_last in
// derive_input, physical keys stamp key_last, and the newer stamp
// wins. Before either is touched, a connected pad still means pad
// labels (the boot default Daniel expects on the couch).
static int pad_mode(void){
    if (IN.pad_count<1) return 0;
    if (IN.pad_last<=0 && IN.key_last<=0) return 1;
    return IN.pad_last>=IN.key_last;
}
// The title menu's ROW LAYOUT latches its pad/keyboard flavour when the
// menu opens. pad_mode() follows the last device touched, which is right
// for hint labels — but letting it restructure the menu mid-navigation
// silently moves rows under the cursor: one keyboard Down press
// re-inserted the SETTINGS row and pushed 2P CO-OP down a row, so a
// player counting rows toggled EDDIE MODE instead and co-op never
// engaged (the mallfix P2 drop-in failure, Nov 14 2026). The latch
// resets whenever the game leaves the title menu.
static int menu_pad_mode(void){
    if (!G.menu_pad) G.menu_pad = pad_mode()? 2 : 1;
    return G.menu_pad==2;
}
// the one true label for an action, for the device in hand. Pad map
// (standard): A fire/confirm, B interact, X wall, START pause/menu,
// BACK back. Keyboard labels honour the player's own bindings.
static const char *lbl_fire(void){ return pad_mode()? "A" : (mobile_ui()? "TAP" : key_name(G.bind[ACT_FIRE])); }
static const char *lbl_int(void){ return pad_mode()? "B" : (mobile_ui()? "TAP" : key_name(G.bind[ACT_INTERACT])); }
static const char *lbl_wall(void){ return pad_mode()? "X" : (mobile_ui()? "TAP" : key_name(G.bind[ACT_WALL])); }
static const char *lbl_bean(void){ return pad_mode()? "Y" : (mobile_ui()? "TAP BEAN" : "6"); }
// a contextual key prompt: "<KEY> - <VERB>", composed on the spot
// (key_name's buffer is shared, so never store its pointer)
static const char *key_prompt(char *buf, const char *key, const char *verb){
    char *q=buf; q=sappend(q, key); q=sappend(q, " - "); q=sappend(q, verb); *q=0; return buf;
}
static const char *lbl_exit(void){ return pad_mode()? "START" : "ESC"; }

enum {
    K_UP=1<<0, K_DOWN=1<<1, K_LEFT=1<<2, K_RIGHT=1<<3,
    K_FIRE=1<<4, K_WALL=1<<5, K_INTERACT=1<<6, K_SPRINT=1<<7
};
// ---- touch controls: the shell forwards every finger to the core.
// Scheme 0 (drag): the first finger steers + fires; a quick, still touch
// is a tap (menus, store, entering the store). Scheme 1 (d-pad): during
// play, on-screen controls — a d-pad bottom-left, FIRE / USE (interact)
// / WPN (cycle weapon) bottom-right — while taps still drive the menus.
#define TP_DPAD_CX 66
#define TP_DPAD_CY 288
#define TP_FIRE_CX (VW-58)
#define TP_FIRE_CY 278
#define TP_ACT_CX  (VW-126)
#define TP_ACT_CY  318
#define TP_WPN_CX  (VW-130)
#define TP_WPN_CY  232
#define TP_WALL_CX (VW-58)
#define TP_WALL_CY 198
// zones: 5 = FIRE, 6 = USE, 7 = WPN, 8 = joystick, 9 = WALL
static int tp_zone_at(int x, int y){
    int dx=x-TP_FIRE_CX, dy=y-TP_FIRE_CY; if (dx*dx+dy*dy<38*38) return 5;
    dx=x-TP_ACT_CX; dy=y-TP_ACT_CY; if (dx*dx+dy*dy<26*26) return 6;
    dx=x-TP_WPN_CX; dy=y-TP_WPN_CY; if (dx*dx+dy*dy<22*22) return 7;
    dx=x-TP_WALL_CX; dy=y-TP_WALL_CY; if (dx*dx+dy*dy<26*26) return 9;
    dx=x-TP_DPAD_CX; dy=y-TP_DPAD_CY;
    if (dx*dx+dy*dy<60*60){
        if (dx>=-12 && dx<=12 && dy>=-12 && dy<=12) return 0;
        if (dx*dx>dy*dy) return dx>0? 4:3;
        return dy>0? 2:1;
    }
    return 0;
}
static void disc_fill(int cx, int cy, int r, u32 c){
    for (int dy=-r; dy<=r; dy++){ int dx=(int)fsqrt((float)(r*r-dy*dy)); rect_fill(cx-dx, cy+dy, dx*2, 1, c); }
}
static int chip_at(Player *me, int tx, int ty);   // weapon chips (defined with the HUD)
static void touch_update(float dt){
    // weapon chips: a tap on the strip equips that weapon directly —
    // the only way to switch on a touchscreen in drag mode
    if (G.state==GS_PLAY && IN.tapped){
        Player *me=&G.p[G.my_id];
        int cw = chip_at(me, IN.tap_x, IN.tap_y);
        if (cw>=0){ IN.weapon_key = cw+1; IN.tapped=0; }
        // the BEAN readout in the status card is a button too
        else if (me->beans>0 && IN.tap_x>=138 && IN.tap_x<232 && IN.tap_y>=64 && IN.tap_y<80){ IN.bean_key = 1; IN.tapped=0; }
    }
    IN.touch_keys=0; IN.tp_held=0; IN.joy_on=0;
    if (IN.tp_act_edge){ IN.interact_pressed=1; IN.tp_act_edge=0; }
    if (IN.tp_wpn_edge){ IN.weapon_cycle=1; IN.tp_wpn_edge=0; }
    if (IN.tp_wall_edge){ IN.wall_pressed=1; IN.tp_wall_edge=0; }
    IN.touch_down=0;
    for (int i=0;i<6;i++){
        if (!IN.tp[i].active) continue;
        IN.tp[i].age += dt;
        if (G.touch_ctl==0){
            if (!IN.touch_down && IN.tp[i].zone!=9 && IN.tp[i].zone!=7){ IN.touch_x=IN.tp[i].x; IN.touch_y=IN.tp[i].y; IN.touch_down=1; }
        } else if (G.state==GS_PLAY && IN.tp[i].zone==5){
            IN.tp_held |= 32; IN.touch_keys |= K_FIRE;
        } else if (G.state==GS_PLAY && IN.tp[i].zone==8){
            // floating joystick (Minecraft-style): the base sits where the
            // thumb landed and slides along when the thumb drags past the
            // rim, so the knob is always at full reach under the finger.
            float dx = (float)(IN.tp[i].x - IN.tp[i].ax), dy = (float)(IN.tp[i].y - IN.tp[i].ay);
            float len = fsqrt(dx*dx + dy*dy);
            if (len > 48.0f){
                float s = (len - 48.0f) / len;
                IN.tp[i].ax += (int)(dx * s); IN.tp[i].ay += (int)(dy * s);
                dx = (float)(IN.tp[i].x - IN.tp[i].ax); dy = (float)(IN.tp[i].y - IN.tp[i].ay);
            }
            IN.joy_on=1; IN.joy_ax=IN.tp[i].ax; IN.joy_ay=IN.tp[i].ay;
            IN.joy_kx=IN.tp[i].ax+(int)dx; IN.joy_ky=IN.tp[i].ay+(int)dy;
            if (dx>9) IN.touch_keys|=K_RIGHT; else if (dx<-9) IN.touch_keys|=K_LEFT;
            if (dy>9) IN.touch_keys|=K_DOWN; else if (dy<-9) IN.touch_keys|=K_UP;
        }
    }
}
// raw-key bookkeeping for derive_input (defined later); the settings
// screen's capture mode reads g_raw_edges directly
static u64 prev_raw=0, g_raw_edges=0; static int prev_sem=0;
static float menu_rep_t=0; static int menu_rep_dir=0;
static int raw_dn(int pk){ return pk>=0 && pk<64? (int)((IN.raw>>pk)&1) : 0; }
static int raw_edge(int pk){ return pk>=0 && pk<64? (int)((g_raw_edges>>pk)&1) : 0; }
static void move_key_str(char *b);   // forward: defined with the help screen

// palettes
static const u32 HOODIE[MAXP] = { COL(64,140,240), COL(240,90,90), COL(90,200,110), COL(240,180,60) };
static const u32 BUG_COL[4] = { COL(150,90,200), COL(90,170,80), COL(220,120,50), COL(100,180,220) };
static const u32 BUG_BELLY[4] = { COL(190,150,230), COL(140,210,120), COL(245,170,100), COL(160,220,245) };

// regions by distance from the epicenter (stretched: the good stuff
// is FAR out). The first three bands are the original country, kept
// exactly as they were; beyond the Legacy Lands the world keeps
// changing its mind — scorched kernel badlands, a frozen cache field,
// neon RAM flats, an overgrown cable jungle, and at the end of the
// world the Depths, pushed far out to where they belong (Daniel,
// Nov 9: "add more regions as you go further from the epicenter").
// Everything regional is a pure function of distance, so the ground,
// the prop tables and the bug strains all agree with each other —
// and with the host in co-op — with nothing stored anywhere.
#define NREGIONS 8
static const char *REGION_NAME[NREGIONS] = {
    "CLEAN CODE MEADOWS", "SPAGHETTI FIELDS", "LEGACY LANDS",
    "KERNEL BADLANDS", "FROZEN CACHE FIELDS", "NEON RAM FLATS",
    "CABLE JUNGLE", "THE UNMAINTAINED DEPTHS"
};
static const float REGION_EDGE[NREGIONS-1] = { 500, 1400, 2600, 4200, 6200, 8800, 12000 };
static const u32 REGION_GA[NREGIONS] = {
    COL(116,184,96),  COL(150,164,84),  COL(128,116,92),  COL(112,70,48),
    COL(176,204,220), COL(56,50,104),   COL(56,102,60),   COL(88,80,104)
};
static const u32 REGION_GB[NREGIONS] = {
    COL(108,176,90),  COL(142,156,78),  COL(120,108,86),  COL(102,62,42),
    COL(166,196,212), COL(48,43,94),    COL(48,94,54),    COL(80,72,96)
};
// ground sprinkle tint per region (the little grass-tuft strokes)
static const u32 REGION_TUFT[NREGIONS] = {
    COL(80,140,70),  COL(96,132,74),  COL(120,124,88),  COL(158,102,58),
    COL(214,234,244), COL(110,235,255), COL(74,156,84), COL(140,128,140)
};
// each far region's dominant bug strain (BUG_COL order); -1 = fair mix
static const int REGION_BUG[NREGIONS] = { -1, -1, -1, 2, 3, 0, 1, -1 };
static int region_at(float d){
    int r = 0;
    while (r < NREGIONS-1 && d >= REGION_EDGE[r]) r++;
    return r;
}
static const char *zone_name(float d){ return REGION_NAME[region_at(d)]; }
static u32 zone_ground(float d, int checker){
    int r = region_at(d);
    return checker ? REGION_GA[r] : REGION_GB[r];
}

// hat tiers — predefined shapes the stack grows through
static const char *hat_tier_name(int n, float kg){
    if (n<=0)        return "BALD";
    if (n==1)        return "SINGLE ROACH";
    if (n<=3)        return "BUG CAP";
    if (n<=6)        return "BUG BEANIE";
    if (n<=11)       return "STACK OVERFLOW";
    if (n<=18)       return "TOWER OF TECH DEBT";
    if (kg<160.0f)   return "CROWN OF CARCASSES";
    return "THE LEGACY SYSTEM";
}
// Espresso Legs. Nov 9 rebalance (Daniel): "cap the unburdened
// speed upgrade max so I'm not zooming everywhere — it should
// continue to have a benefit burdened, and make the burdened
// benefit more substantial per level." So: the bare-speed bonus
// now CAPS at +40% (207 px/s — where a level-10 build already
// cruises) no matter how Endless the levels get, and the
// upgrade's endless value moved into the load curve instead:
// each level raises by 50% (was 35%) the weight the curve
// shrugs off, so every level claws back a bigger share of the
// hat's penalty than it used to, forever. Level 0 is untouched:
// same base, same curve as always.
static float speed_curve(Player *p){   // the raw burden curve, pre-floor
    float bmul = 1.0f + 0.04f*p->upg[4]; if (bmul>1.40f) bmul=1.40f;
    float cap = 55.0f * (1.0f + 0.50f*p->upg[4]);
    return 148.0f * bmul / (1.0f + p->carried/cap);   // heavy hat = slow dev
}
static float player_speed(Player *p){
    float sp = speed_curve(p);
    // the floor (Daniel, Nov 14): however heavy the hat gets, the
    // crawl never drops below 40 px/s — rather slow, never extra
    // painfully slow. (The old curve sank toward ~28 under a full
    // 240 hat and toward nothing beyond it.) Sprint multiplies
    // after the floor, so it still helps at the bottom.
    if (sp < 40.0f) sp = 40.0f;
    if (p->cur_keys & K_SPRINT) sp *= 1.22f;
    return sp;
}

// ---------------------------------------------------------------------------
// Entity helpers
// ---------------------------------------------------------------------------
// WHALES ONLY: PARADE DUCK — per-player follower state (purely visual)
static float duck_x[MAXP], duck_y[MAXP], duck_ph[MAXP];
static int duck_on[MAXP], duck_face[MAXP];
static void spawn_particle(float x, float y, u32 col, float spd, float life, int size){
    for (int i=0;i<MAXPART;i++) if (!G.parts[i].alive){
        Particle *q=&G.parts[i];
        float a = rng_f()*TAU;
        q->alive=1; q->x=x; q->y=y; q->vx=fcos(a)*spd*rng_f(); q->vy=fsin(a)*spd*rng_f();
        q->life=q->maxlife=life*(0.5f+rng_f()*0.5f); q->size=size; q->col=col; return;
    }
}
static void splat(float x, float y, u32 col, int n, float spd){
    for (int i=0;i<n;i++) spawn_particle(x, y, col, spd, 0.55f, 2+(int)(rng_f()*3));
}
static void spawn_floater(float x, float y, const char *txt, u32 col){
    for (int i=0;i<MAXFLOAT;i++) if (!G.floats[i].alive){
        Floater *f=&G.floats[i];
        f->alive=1; f->x=x; f->y=y; f->life=1.1f; scpy(f->text, txt, 24); f->col=col; return;
    }
}
static void floater_num(float x, float y, const char *pre, int v, const char *post, u32 col){
    char b[24]; fmt_int(b, pre, v, post); spawn_floater(x, y, b, col);
}
static CarcassFx *spawn_fx(int mode, float x0, float y0, float x1, float y1, float w, int player){
    for (int i=0;i<(int)(sizeof(G.fx)/sizeof(G.fx[0]));i++) if (!G.fx[i].alive){
        CarcassFx *f=&G.fx[i];
        f->alive=1; f->mode=mode; f->x0=x0; f->y0=y0; f->x1=x1; f->y1=y1;
        f->t=0; f->dur=0.42f; f->w=w; f->player=player; f->wall_slot=-1; return f;
    }
    return 0;
}
// hat stack ops — the stack is both hat and wallet
static void hat_push(Player *p, float w){
    if (p->stack_n < MAXSTACK) p->stack[p->stack_n++] = w;
    else p->stack[MAXSTACK-1] += w;                    // merge overflow
    p->carried += w;
    if (p->carried > p->heaviest) p->heaviest = p->carried;
}
static float hat_pop(Player *p){                        // returns weight or 0
    if (p->stack_n<=0) return 0;
    float w = p->stack[--p->stack_n];
    p->carried -= w; if (p->carried<0) p->carried=0;
    return w;
}

// ---------------------------------------------------------------------------
// Banzai Buddies — species data and derived stats
// ---------------------------------------------------------------------------
#define BUDN 6
static const char *BUD_NAME[BUDN] = { "PUPPY", "KITTY", "MOOSE", "PLATYPUS", "PET ROCK", "KITCHEN SINK" };
static const char *BUD_MOVE2[BUDN] = { "POUNCE", "RAKE", "CHARGE", "SPUR DIVE", "ROLL OUT", "SPLASH DOWN" };
static const char *BUD_SPEC[BUDN] = { "BARK", "HAIRBALL", "STOMP", "VENOM STING", "ROCK STEADY", "OVERFLOW" };
static const char *BUD_MOVE3[BUDN] = { "FETCH", "FLYING CLAWS", "ANTLER SWEEP", "BILL SLAM", "SEDIMENT SLAM", "DISPOSAL" };
static const char *BUD_MOVE4[BUDN] = { "PUPPY EYES", "AMBUSH", "TIMBER!", "SIXTH SENSE", "AVALANCHE", "EVERYTHING" };
static const char *BUD_BASIC[BUDN] = { "CHOMP", "SCRATCH", "HEADBUTT", "BILL POKE", "BONK", "CLANG" };
// Buddy energy (Daniel): specials are not free anymore. The basic attack
// costs nothing; each taught move has a price, and the pool refills on
// its own as you go. Sleeping (Banzai World) refills it outright.
#define BUD_ENERGY_MAX 100.0f
#define BUD_EN_MOVE2 20.0f
#define BUD_EN_SPEC  45.0f
#define BUD_EN_MOVE3 30.0f
#define BUD_EN_MOVE4 55.0f
#define BUD_EN_REGEN 5.0f
static const int BUD_PRICE[BUDN] = { 100, 150, 300, 200, 60, 175 };
static const float BUD_HP0[BUDN] = { 95, 75, 230, 120, 420, 160 };
static const float BUD_DMG0[BUDN] = { 7, 5, 15, 9, 6, 10 };
static const float BUD_CD0[BUDN] = { 0.55f, 0.32f, 0.9f, 0.6f, 1.1f, 0.7f };
static const float BUD_SPD[BUDN] = { 185, 195, 165, 175, 118, 148 };   // the rock is in no hurry
static const float BUD_DEF[BUDN] = { 1.0f, 1.0f, 0.9f, 0.9f, 0.45f, 0.8f };   // damage taken multiplier
// the name list: obscure Elves (and a few Maiar) of the Elder Days,
// out of The Silmarillion — diacritics folded to plain letters
#define BUD_NAMES_N 60
// elven names from The Silmarillion, The Hobbit and The Lord of the
// Rings, ASCII-folded and alphabetized (Daniel, Nov 2). Half-elves
// Earendil, Elrond and Elwing make the cut; the old list's Maiar
// (Salmar, Osse and friends) do not — they were never elves.
static const char *BUD_ELF[BUD_NAMES_N] = {
    "AEGNOR", "AMRAS", "AMROD", "ANGROD", "AREDHEL", "ARGON", "ARWEN", "BELEG",
    "CARANTHIR", "CELEBORN", "CELEBRIAN", "CELEGORM", "CIRDAN", "CURUFIN", "DAERON", "DENETHOR",
    "EARENDIL", "ECTHELION", "EGALMOTH", "ELENWE", "ELLADAN", "ELROHIR", "ELROND", "ELWING",
    "EOL", "ERESTOR", "FEANOR", "FINARFIN", "FINDUILAS", "FINGOLFIN", "FINROD", "FINWE",
    "GALADRIEL", "GALION", "GILDOR", "GILGALAD", "GLORFINDEL", "GWINDOR", "HALDIR", "IDRIL",
    "INGWE", "LEGOLAS", "LINDIR", "LUTHIEN", "MABLUNG", "MAEDHROS", "MAEGLIN", "MAGLOR",
    "MIRIEL", "NERDANEL", "NIMLOTH", "ORODRETH", "OROPHIN", "PENGOLODH", "RUMIL", "SAEROS",
    "THINGOL", "THRANDUIL", "TURGON", "VORONWE"
};
// the original 20-name list, kept only so saves written before the
// expansion can map their stored index to the same name (the bnv
// save token marks new-format saves; the Maiar names that are gone
// fall back to the species default)
static const char *BUD_ELF_LEGACY[20] = {
    "AEGNOR", "AMROD", "AMRAS", "ANGROD", "CARANTHIR", "EOL", "MAEGLIN", "VORONWE",
    "GWINDOR", "MABLUNG", "BELEG", "FINDUILAS", "NERDANEL", "SALMAR", "ILMARE",
    "EONWE", "TILION", "ARIEN", "OSSE", "UINEN"
};
static const int BUD_DEF_NAME[BUDN] = { 7, 43, 57, 37, 24, 12 };   // BELEG, LUTHIEN, THRANDUIL, GWINDOR, EOL, CIRDAN
// buddy coats: 0 is the species' own color; 1-8 are the standard set;
// 9 is the psychedelic neon rainbow (cycles with time)
#define BUD_COLORS_N 10
static const char *BUD_COLOR_NAME[BUD_COLORS_N] = {
    "NATURAL", "RED", "ORANGE", "YELLOW", "GREEN", "BLUE", "PURPLE", "PINK", "BROWN", "NEON RAINBOW"
};
static u32 bud_hsv(float h){   // h in turns; full saturation, full neon
    h -= (int)h; if (h<0) h+=1.0f;
    float x = h*6.0f; int s = (int)x; float f = x - s;
    int up = (int)(f*255.0f), dn = 255-up;
    switch (s%6){
        case 0: return COL(255, up, 0); case 1: return COL(dn, 255, 0);
        case 2: return COL(0, 255, up); case 3: return COL(0, dn, 255);
        case 4: return COL(up, 0, 255); default: return COL(255, 0, dn);
    }
}
static u32 bud_coat(Buddy *b, u32 natural){
    static const u32 STD[8] = { COL(216,64,54), COL(240,140,40), COL(244,208,64), COL(84,190,84),
                                COL(72,130,230), COL(150,90,210), COL(244,140,190), COL(140,95,60) };
    if (b->color==9) return bud_hsv(G.anim_t*0.22f + (float)(b - G.bud)*0.37f);
    if (b->color>=1 && b->color<=8) return STD[b->color-1];
    return natural;
}
// adoption draft state: species -> name -> color, then the till runs
static int bud_stage[MAXP];        // 0 panel, 1 name (adopt), 2 color (adopt), 3 name (rename), 4 color (recolor)
static int bud_page[MAXP];         // name-list page while picking
static int bud_dsp[MAXP];          // drafted species for an adoption
static int bud_dname[MAXP];        // drafted name for an adoption
static int bud_color_draft[MAXP];  // drafted coat for an adoption
static void buddy_refresh(int pi){
    Buddy *b=&G.bud[pi];
    b->maxhp = BUD_HP0[b->species%BUDN] + 25.0f*b->up_hp;
    if (b->hp > b->maxhp) b->hp = b->maxhp;
}
static float buddy_dmg(Buddy *b){ return BUD_DMG0[b->species%BUDN] * (1.0f + 0.2f*b->up_atk); }
static void buddy_adopt(int pi, int species){
    Buddy *b=&G.bud[pi];
    smemset(b, 0, sizeof(*b));
    b->active=1; b->species=species; b->on_wall=-1;
    b->name_idx = bud_dname[pi]; b->color = bud_color_draft[pi];
    buddy_refresh(pi);
    b->hp = b->maxhp;
    b->energy = BUD_ENERGY_MAX;
    b->x = G.p[pi].x + 26; b->y = G.p[pi].y + 12;
    b->aim = -PI/2;
    splat(b->x, b->y, COL(255,214,120), 12, 100);
}

// pending store payment (one at a time; the till is not self-service)
static int   store_open_local=0;      // client-side store UI flag
static int   client_station=0;        // client-side: which mall station panel is open
static int   shield_remote=0;         // client copy: shield state from snapshots
static int   pay_active=0, pay_player=0, pay_kind=0, pay_id=0;
static float pay_left=0, pay_t=0, pay_total=0;
static void grant_item(int pi, int kind, int id){
    Player *p=&G.p[pi];
    if (kind==3){ p->hatstand=1; sfx(11); }                       // Psychic Hat Stand
    else if (kind==4){ buddy_adopt(pi, id); sfx(11); }            // adopt a Banzai Buddy
    else if (kind==5){ Buddy *b=&G.bud[pi]; b->species=id;        // re-species the buddy
        buddy_refresh(pi); b->hp=b->maxhp; b->fainted=0;
        b->x=p->x+26; b->y=p->y+12; sfx(11); }
    else if (kind==6){ Buddy *b=&G.bud[pi];                       // REVIVE a fainted buddy
        b->fainted=0; buddy_refresh(pi); b->hp=b->maxhp;
        b->x=p->x+26; b->y=p->y+12;
        splat(b->x, b->y, COL(150,255,170), 14, 110);
        spawn_floater(b->x, b->y-20, "REVIVED!", COL(150,255,170));
        sfx(11); }
    else if (kind==0){ if (p->owned[id]<MAXCOPIES) p->owned[id]++; p->weapon=id; sfx(11); }
    else {
        if (id==5){ p->rollback++; }
        else if (id==7){ p->beans++; }   // one Backfire Bean, holstered and ready to rip
        else if (id==6){ p->hp += 50; if (p->hp > p->maxhp) p->hp = p->maxhp; }
        else p->upg[id]++;
        p->dmg_mul  = 1.0f + 0.25f*p->upg[0];
        p->rate_mul = 1.0f + 0.18f*p->upg[1];
        p->shield_max = shield_for(p->upg[2]);
        p->maxhp = 100 + 25*p->upg[3];
        if (p->hp > p->maxhp) p->hp = p->maxhp;
        sfx(11);
    }
    if (pi==G.my_id){
        scpy(G.store_msg, kind==3? "THE STAND IS YOURS!"
                                  : kind==4? "BUDDY ADOPTED!"
                                  : kind==6? "BUDDY REVIVED!"
                                  : kind==5? "NEW BUDDY - SAME TRAINING!"
                                  : kind==0? "WEAPON INSTALLED!"
                                  : (id==7? "ONE BACKFIRE BEAN, READY TO RIP" : id==6? "PATCHED UP! +50 HP" : "UPGRADE INSTALLED!"), 64);
        G.store_msg_t = 150;
    }
    push_ev(3, p->x, p->y);
}
static void start_payment(int pi, int kind, int id, int price){
    pay_active=1; pay_player=pi; pay_kind=kind; pay_id=id;
    pay_left=(float)price; pay_total=(float)price; pay_t=0;
}
static void update_payment(float dt){
    if (!pay_active) return;
    Player *p=&G.p[pay_player];
    pay_t -= dt;
    if (pay_t<=0){
        pay_t = 0.05f;
        // in the mall the carcasses fly to the counter, not the plaza
        float fdx = p->in_mall? p->x : 0, fdy = p->in_mall? p->y-70 : -30;
        // BATCH COUNTING (Daniel, Nov 10): one carcass per tick made a
        // big hat take forever to pay with — at scale the till counts
        // in handfuls. The batch grows with how many carcasses the
        // remaining bill will take, so a small price still counts out
        // one by one (the charm of the till) while a 400-carcass hat
        // settles in about a second. The exact-change slice still
        // finishes every payment, so the totals never drift.
        int need = 0; float acc = 0;
        for (int i=p->stack_n-1; i>=0 && acc<pay_left; i--){ acc += p->stack[i]; need++; }
        int batch = need<=6? 1 : need<=24? 3 : need<=80? 6 : need<=200? 12 : 20;
        int fx_left = 2;   // only the first couple of each handful fly visibly
        for (int k=0;k<batch && pay_left>0 && p->stack_n>0;k++){
            if (p->stack[p->stack_n-1] >= pay_left){
                // the till slices the top carcass: exact change, the rest
                // stays on the hat (bugs are weighed, not coinage)
                float w = pay_left;
                p->stack[p->stack_n-1] -= w;
                if (p->stack[p->stack_n-1] <= 0.01f) p->stack_n--;
                p->carried -= w; if (p->carried<0) p->carried=0;
                if (fx_left>0){ spawn_fx(1, p->x, p->y-26, fdx, fdy, w, pay_player); fx_left--; }
                pay_left = 0;
            } else {
                float w = hat_pop(p);
                if (w>0 && fx_left>0){ spawn_fx(1, p->x, p->y-26, fdx, fdy, w, pay_player); fx_left--; }
                pay_left -= w;
            }
        }
        sfx(10);
        if (pay_left<=0 || p->stack_n<=0){
            pay_active=0;
            G.total_earned += pay_total;
            grant_item(pay_player, pay_kind, pay_id);
        }
    }
}

// ---------------------------------------------------------------------------
// Procedural world props — deterministic per chunk. The field is a
// junkyard now (Daniel, Nov 2): the old rocks became electronic
// debris. Types: 0 none, 1 shattered PCB + parts spill, 2 crate with
// a CRT in it (solid), 3 DRAM sticks + a coax coil, 4 fax machine
// with a modem on top (solid), 5 a tipped game console in the dirt
// with its cables spilled out. Solid types block players and bugs.
// ---------------------------------------------------------------------------
#define CHUNK 256
// what washes up in each region: the home country's junkyard (types
// 1-5) gives way to the far countries' own wrecks — burnt server
// husks and heat vents in the Badlands, frost-locked drive banks in
// the Cache Fields, neon memory towers in the RAM Flats, cable trees
// in the Jungle, half-deleted monoliths in the Depths (types 6-15,
// drawn in draw_prop). One table slot per hash roll, same density
// and positions as always — only the identity is regional.
static const int REGION_PROPS[NREGIONS][5] = {
    { 1, 2, 3, 4, 5 },        // Meadows: the classic junkyard
    { 1, 2, 3, 4, 5 },        // Spaghetti Fields
    { 1, 2, 3, 4, 5 },        // Legacy Lands
    { 6, 7, 1, 6, 3 },        // Kernel Badlands
    { 8, 9, 3, 8, 1 },        // Frozen Cache Fields
    { 10, 11, 3, 10, 11 },    // Neon RAM Flats
    { 12, 13, 5, 12, 1 },     // Cable Jungle
    { 14, 15, 1, 14, 5 }      // The Unmaintained Depths
};
static void chunk_prop(int cx, int cy, int slot, float *px, float *py, int *type, int *solid){
    u32 h = hash2(cx, cy, WORLD_SEED + (u32)slot*7919u);
    *type = 0; *solid = 0; *px = 0; *py = 0;
    if (cx>=-1 && cx<=0 && cy>=-1 && cy<=0) return;      // keep the store plaza clear
    float r = (float)(h>>8)/16777216.0f;
    if (r > 0.42f) return;                               // sparse
    *px = cx*CHUNK + 40 + (float)((h>>3)%170);
    *py = cy*CHUNK + 40 + (float)((h>>11)%170);
    { float pd = fsqrt((*px)*(*px) + (*py)*(*py));
      *type = REGION_PROPS[region_at(pd)][(h>>16)%5]; }
    if (*type==2 || *type==4 || *type==6 || *type==8 || *type==10 || *type==12 || *type==14) *solid = 1;
}
static int prop_collide(float x, float y, float rad){
    int cx = (int)(x/CHUNK) - (x<0?1:0), cy = (int)(y/CHUNK) - (y<0?1:0);
    for (int i=-1;i<=1;i++) for (int j=-1;j<=1;j++)
        for (int s=0;s<2;s++){
            float px,py; int t,sol;
            chunk_prop(cx+i, cy+j, s, &px,&py,&t,&sol);
            if (sol){ float dx=x-px, dy=y-py; if (dx*dx+dy*dy < (rad+20)*(rad+20)) return 1; }
        }
    return 0;
}

// ---------------------------------------------------------------------------
// Bugs — stats scale with distance from the Epicenter, per the design:
// further out = bigger max size, more bugs, and the small ones get faster.
// ---------------------------------------------------------------------------
static float bug_radius(float w){ return 7.0f + fsqrt(w)*2.3f; }
// max bug weight at distance d from the Epicenter. The curve stays nearly
// flat for a long way out (pocket-change roaches) and only rears up in the
// far country — you have to travel to find the big ones.
// --- player power and world threat -----------------------------------
// Your capability score: best-weapon DPS plus effective health. A fresh
// DEV scores ~152, and world THREAT is your score relative to that — so
// it starts at x1.0 and climbs as you upgrade, feeding you bigger, faster,
// more numerous bugs even on the Epicenter's doorstep.
static float player_dps(Player *p){
    float best = 0;
    for (int w=0;w<NWEAP;w++) if (p->owned[w]){
        float d;
        if (w==4) d = (8.0f/0.07f) * p->dmg_mul
                    * ((BEAM_EXT+BEAM_SPIN)/(BEAM_EXT+BEAM_SPIN+BEAM_RET+BEAM_REST)); // beam dps x sweep duty cycle
        else d = weap_dmg(w) * (float)p->owned[w] / weap_cd(w) * p->dmg_mul * p->rate_mul;
        if (d>best) best=d;
    }
    return best;
}
static float player_power(Player *p){
    return player_dps(p) + (float)p->maxhp + (float)p->shield_max*2.0f;
}
static float threat_override = -1;   // debug/test hook only; -1 = use the real threat
// THREAT RATIO (Daniel, Nov 9): "it doesn't need to tip more in the
// user's favor, just change the general ratio." The world's answer
// to your power is scaled by this one constant: the curve keeps its
// shape at every upgrade level, but only 3/4 of the excess over a
// fresh DEV's x1.0 ever arrives. A new run is untouched (power 152
// still means threat x1.0), and every threat channel downstream —
// bug size, the far-country inflation, the crowd a powerful player
// draws — reads G.threat, so the whole composite eases by the same
// proportion everywhere.
#define THREAT_RATIO 0.75f
static void compute_threat(void){
    float pw = 152.0f;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        float v = player_power(&G.p[i]); if (v>pw) pw=v;
    }
    G.threat = 1.0f + (pw/152.0f - 1.0f) * THREAT_RATIO; if (G.threat<1.0f) G.threat=1.0f;
    if (threat_override>=0) G.threat = threat_override;
}
// hauling a downed partner back up costs carcasses off YOUR hat,
// priced off the DOWNED player's capability (Daniel): a fresh
// sidekick is cheap to save, a stacked one takes a real haul. Half health and a grace ring, like
// the Rollback Token, and the swarm on them gets shoved back.
static int revive_price(Player *down){
    int v = 30 + (int)(player_dps(down)*0.25f) + down->maxhp/8;
    return v<15? 15 : v;
}
static Player *downed_near(Player *p){
    for (int j=0;j<MAXP;j++){ Player *q=&G.p[j];
        if (q==p || !q->connected || q->alive) continue;
        float dx=q->x-p->x, dy=q->y-p->y;
        if (dx*dx+dy*dy <= 46*46) return q;
    }
    return 0;
}
static int try_revive(int pi){
    Player *p=&G.p[pi];
    Player *q = downed_near(p);
    if (!q) return 0;
    int price = revive_price(q);
    if (p->carried < (float)price){
        char b[48]; char *s=b; s=sappend(s, "REVIVE NEEDS "); s=sapp_int(s, price); s=sappend(s, " KG"); *s=0;
        spawn_floater(p->x, p->y-30, b, COL(255,120,110));
        sfx(16);
        return 1;   // the press was spent on the attempt
    }
    float need = (float)price;
    while (need > 0 && p->stack_n>0) need -= hat_pop(p);
    q->alive=1; q->hp=q->maxhp/2; q->shield=q->shield_max/2;
    q->revive_t=3.0f; q->dead_t=0;
    for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
        float dx=G.bugs[i].x-q->x, dy=G.bugs[i].y-q->y, d2=dx*dx+dy*dy;
        if (d2 < 140*140 && d2>0.01f){ float d=fsqrt(d2); G.bugs[i].vx += dx/d*340; G.bugs[i].vy += dy/d*340; }
    }
    spawn_floater(q->x, q->y-34, "REVIVED!", COL(120,255,160));
    splat(q->x, q->y, COL(120,255,160), 22, 160);
    sfx(12);
    return 1;
}
// the owner's best gun for a bot to carry: highest per-shot damage among
// the projectile weapons (the Debugger counts as its keyboard fallback)
static int best_weapon_idx(Player *p, float *per_shot){
    int bw=0; float bd=0;
    for (int w=0;w<4;w++) if (p->owned[w]){ float d=weap_dmg(w)*p->dmg_mul; if (d>bd){ bd=d; bw=w; } }
    if (p->owned[4] && 30.0f*p->dmg_mul>bd){ bd=30.0f*p->dmg_mul; bw=0; }
    if (per_shot) *per_shot=bd;
    return bw;
}
// ---------------------------------------------------------------------------
// Difficulty — EASY is the game exactly as it has always played; every
// step up only makes it harder (Daniel's rule). Picked on the title
// screen, locked into the run at start (diff_run), saved with the run.
// ---------------------------------------------------------------------------
static const char *DIFF_NAME[4] = { "EASY", "MEDIUM", "HARD", "BUGAGGEDON" };
static const float DIFF_HP[4]  = { 1.0f, 1.35f, 1.8f, 2.6f };
static const float DIFF_DMG[4] = { 1.0f, 1.25f, 1.6f, 2.2f };
static const float DIFF_SPD[4] = { 1.0f, 1.08f, 1.16f, 1.25f };
static const float DIFF_POP[4] = { 1.0f, 1.3f, 1.7f, 2.4f };
static const float DIFF_RATE[4]= { 1.0f, 1.25f, 1.6f, 2.2f };
static const float DIFF_W[4]   = { 1.0f, 1.15f, 1.3f, 1.6f };
static float d_hp(void){ return DIFF_HP[G.diff_run&3]; }
static float d_dmg(void){ return DIFF_DMG[G.diff_run&3]; }
static float d_spd(void){ return DIFF_SPD[G.diff_run&3]; }
static float d_pop(void){ return DIFF_POP[G.diff_run&3]; }
static float d_rate(void){ return DIFF_RATE[G.diff_run&3]; }
static float d_w(void){ return DIFF_W[G.diff_run&3]; }

static float bug_max_weight(float d){
    float t = d/3000.0f; if (t>1.3f) t=1.3f;
    float m = 2.0f + 133.0f * t*t*(0.35f + 0.65f*t);
    float cap = 150.0f * (1.0f + (G.threat-1.0f)*0.35f);   // threat lifts the ceiling (0.6 -> 0.35, Nov 2)
    return m>cap? cap : m;
}
static void spawn_bug(float x, float y){
    for (int i=0;i<MAXBUGS;i++) if (!G.bugs[i].alive){
        Bug *b=&G.bugs[i];
        float d = fsqrt(x*x + y*y);
        // THREAT: an upgraded player makes the whole map act farther-out
        // than it is — the bugs near home grow teeth too. Retuned Nov 2
        // (Daniel: pacing too aggressive, too many big bugs): the
        // inflation is 1100 -> 650 and the weight roll below is
        // quartic, so high THREAT mostly means MORE small bugs, not a
        // screen full of giants on the doorstep.
        float de = d + (G.threat-1.0f)*650.0f;
        float maxW = bug_max_weight(de) * d_w();
        float r1 = rng_f();
        float w = 0.6f + r1*r1*r1*r1*maxW;               // quartic skew: mostly tiny, giants rare
        // SIZE scales with THREAT, indefinitely (Daniel): the stronger
        // you get, the bigger everything that crawls out gets — weight,
        // radius, HP and bite all ride on w, and there is no ceiling.
        // The camera gives ground in steps to compensate (zoom_target_of).
        w *= 1.0f + (G.threat-1.0f)*0.30f;
        b->alive=1; b->kind=(int)(rng_f()*4); b->aggro=-1; b->target_wall=-1; b->btarget=-1;
        // regional strains (Nov 9): each far country has a dominant
        // kind — rustbacks in the Badlands, frostbites in the Cache
        // Fields, neons in the RAM Flats, mossbacks in the Jungle.
        // Half the locals wear the regional colors; the rest stay a
        // fair mix. This flavors the spawn AFTER all the threat and
        // distance scaling above, so it composes with that curve
        // instead of bypassing it.
        { int rg = region_at(d);
          if (REGION_BUG[rg]>=0 && rng_f()<0.5f) b->kind = REGION_BUG[rg]; }
        b->x=x; b->y=y; b->vx=0; b->vy=0; b->w=w;
        b->maxhp = b->hp = (4.0f + w*2.4f) * d_hp();
        // speed: near the Epicenter everything crawls, big or small. Far
        // out the whole swarm speeds up — and the little ones most of all.
        float dfac = de/2600.0f; if (dfac>1) dfac=1;
        float smallness = 1.0f - (w/60.0f>1?1:w/60.0f);
        b->speed = (20.0f + 38.0f*dfac + 34.0f*dfac*smallness) * rng_range(0.85f,1.15f) * d_spd();
        b->dmg = (1.5f + w*0.10f) * d_dmg();
        // the Depths keep ancients: a rare few that were old when the
        // codebase was young, and noticeably heavier for it
        if (region_at(d)==NREGIONS-1 && rng_f()<0.08f){
            b->w *= 1.35f;
            b->maxhp = b->hp = (4.0f + b->w*2.4f) * d_hp();
            b->dmg = (1.5f + b->w*0.10f) * d_dmg();
        }
        if (G.zen_run){   // zen mode: every bug gigantic, every bug slow
            b->w = w*8.0f + 6.0f;
            b->maxhp = b->hp = (4.0f + b->w*2.4f) * d_hp();
            b->dmg = (1.5f + b->w*0.10f) * d_dmg();
            b->speed *= 0.30f;
        }
        b->leg_ph = rng_f()*TAU; b->wobble = rng_f()*TAU;
        b->hit_t=0; b->bp_t=0; b->target_wall=-1; b->chew_t=0;
        // the seed is a trig PHASE (pupil drift, Eddie's fuzz), not a
        // number to keep: a full-range u32 (up to 4.3e9) blows past
        // wrap_pi's float range, the Taylor sin explodes into the
        // thousands, and the bug's pupils land half a screen away
        // (Daniel, Nov 6). 0..6282 milli-radians is plenty of variety.
        b->seed = rng_u32()%6283;
        return;
    }
}
static void spawner(float dt){
    compute_threat();
    G.spawn_t -= dt;
    if (G.spawn_t>0) return;
    int alive=0; for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive) alive++;
    float far=0; int npl=0;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].alive && !G.p[i].in_mall){
        npl++;
        float d=fsqrt(G.p[i].x*G.p[i].x + G.p[i].y*G.p[i].y); if (d>far) far=d;
    }
    // population target: a sparse trickle near home, a swarm far out.
    int time_term = (int)(G.time*0.15f); if (time_term>10) time_term=10;
    int target = 6 + (int)(far*0.017f) + time_term + 3*(npl>0? npl-1:0);
    target = (int)(target * (1.0f + (G.threat-1.0f)*0.18f));   // power draws a crowd (0.30 -> 0.18, Nov 2)
    target = (int)(target * d_pop());                          // difficulty draws a bigger one
    if (target>200) target=200;
    if (G.zen_run && target>8) target = 8 + (target-8)/2;   // giants need elbow room
    G.spawn_t = (far<600? 1.0f : 0.55f) / (1.0f + (G.threat-1.0f)*0.08f) / d_rate();   // slow drip at home
    if (alive>=target || npl==0) return;
    // spawn near a random connected player, just off-screen. Anywhere is
    // fair game now — including the Epicenter plaza itself.
    int pi=-1, tries=0;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && !G.p[i].in_mall) pi=i;
    if (pi<0) return;
    pi = (int)(rng_f()*(pi+1)); if (!G.p[pi].connected) pi=0;
    while (tries<6){
        tries++;
        float a = rng_f()*TAU, rad = rng_range(620.0f, 860.0f);
        float x = G.p[pi].x + fcos(a)*rad, y = G.p[pi].y + fsin(a)*rad;
        if (x>-110 && x<110 && y>-135 && y<-25) continue; // not inside the hut
        spawn_bug(x, y);
        if (far>2000 && alive<target-25) spawn_bug(x + rng_range(-60,60), y + rng_range(-60,60));
        return;
    }
}

// ---------------------------------------------------------------------------
// Combat
// ---------------------------------------------------------------------------
static void fire_proj(int owner, int kind, float x, float y, float ang, float dmg, int agent){
    for (int i=0;i<MAXPROJ;i++) if (!G.projs[i].alive){
        Proj *q=&G.projs[i];
        q->alive=1; q->kind=kind; q->owner=owner; q->x=x; q->y=y; q->dmg=dmg; q->agent=agent;
        q->bounces=0; q->target=-1; q->spin=rng_f()*TAU;
        if (kind==0){ float s=430; q->vx=fcos(ang)*s; q->vy=fsin(ang)*s; q->life=0.85f; }
        if (kind==1){ float s=330; q->vx=fcos(ang)*s; q->vy=fsin(ang)*s; q->life=1.15f; }
        if (kind==2){ float s=360; q->vx=fcos(ang)*s; q->vy=fsin(ang)*s; q->life=1.5f; }
        if (kind==3){ float s=265; q->vx=fcos(ang)*s; q->vy=fsin(ang)*s; q->life=1.6f; }
        return;
    }
}
static void hurt_bug(int bi, float dmg, int owner, float kx, float ky);
static void hurt_buddy(int owner, float dmg);
static void explode_pm(float x, float y, float dmg, int owner, int agent){
    splat(x, y, COL(255,200,80), 16, 130);
    for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
        float dx=G.bugs[i].x-x, dy=G.bugs[i].y-y;
        if (dx*dx+dy*dy < 95*95){
            float d=fsqrt(dx*dx+dy*dy)+0.01f;
            hurt_bug(i, dmg, owner, dx/d*180, dy/d*180);
        }
    }
    spawn_floater(x, y-14, "SYNERGY!", COL(255,220,90));
    sfx(7);
}
static void kill_bug(int bi, int owner){
    Bug *b=&G.bugs[bi];
    b->alive=0;
    G.total_kills++;
    // assist XP: a buddy that had bitten this bug learns from its fall
    if (b->btarget>=0 && b->btarget!=owner && G.bud[b->btarget].active && !G.bud[b->btarget].fainted)
        G.bud[b->btarget].xp += 2 + (int)(b->w*0.3f);
    if (owner>=0 && owner<MAXP){
        G.p[owner].kills++;
        // the carcass flies to the killer's head and joins the stack
        CarcassFx *f = spawn_fx(0, b->x, b->y, G.p[owner].x, G.p[owner].y-24, b->w, owner);
        if (!f) hat_push(&G.p[owner], b->w);           // fx full: instant
    }
    splat(b->x, b->y, BUG_COL[b->kind], 10, 110);
    // two-tone burst (glow-up): pale belly bits drift longer, and a
    // few white sparks snap out fast — a squash should FEEL juicy
    for (int i=0;i<5;i++) spawn_particle(b->x, b->y, BUG_BELLY[b->kind], 70, 0.8f, 2);
    for (int i=0;i<3;i++) spawn_particle(b->x, b->y, COL(255,252,240), 170, 0.3f, 1);
    if (owner>=0 && owner<MAXP && (G.p[owner].whale_on & 1024)){   // WHALES ONLY: CONFETTI SQUASH
        static const u32 PARTY[6] = { COL(255,90,120), COL(255,200,60), COL(90,220,120),
                                      COL(90,180,255), COL(200,120,255), COL(255,140,60) };
        for (int i=0;i<14;i++) spawn_particle(b->x, b->y, PARTY[i%6], 95, 1.15f, 2);
    }
    push_ev(1, b->x, b->y);
    sfx(5);
}
static void hurt_bug(int bi, float dmg, int owner, float kx, float ky){
    Bug *b=&G.bugs[bi];
    if (!b->alive) return;
    b->hp -= dmg; b->hit_t = 0.12f;
    b->vx += kx*0.4f; b->vy += ky*0.4f;
    if (b->hp<=0) kill_bug(bi, owner);
}
// a bean jetpack ends the moment its rider takes a hit or fires
// (Daniel, Nov 10): the bean dumps you where you are, mid-field.
static void bean_jet_end(int pi, int shot){
    Player *p=&G.p[pi];
    if (p->bean_jet<=0) return;
    p->bean_jet=0; p->jet_fall=0.32f;
    splat(p->x, p->y+6, COL(168,216,120), 12, 100);
    spawn_floater(p->x, p->y-30, shot? "BACK TO EARTH!" : "KNOCKED FROM THE SKY!", COL(255,220,150));
    sfx(5);
}
static void hurt_player(int pi, float dmg){
    Player *p=&G.p[pi];
    if (!p->alive || G.state!=GS_PLAY) return;
    if (p->revive_t>0) return;               // just rolled back: untouchable
    if (p->bean_jet>0) bean_jet_end(pi, 0);   // one hit ends the jet
    if (p->shield>0){
        float absorbed = dmg < (float)p->shield ? dmg : (float)p->shield;
        p->shield -= (int)absorbed; dmg -= absorbed;
        p->shield_regen_t = 3.0f;
        if (absorbed>0) sfx(8);
    }
    if (dmg>0){
        p->hp -= (int)dmg; p->hurt_t = 0.25f; sfx(9);
        splat(p->x, p->y, COL(230,60,60), 5, 80);
        // ENDGAME PRESSURE (Daniel's pick #1): a heavy hat is a
        // liability. Once you're carrying real weight, every bite that
        // gets through knocks the top carcass clean off your head.
        // What's on your skull is at risk; what's in the Bug Bank is not.
        if (p->carried>=50.0f && p->stack_n>0 && p->shed_cd<=0){
            float w = hat_pop(p);
            if (w>0){
                p->shed_cd = 0.7f;
                splat(p->x, p->y-24, BUG_COL[(int)w%4], 7, 120);
                floater_num(p->x, p->y-40, "-", (int)w, "KG", COL(255,120,110));
            }
        }
    }
    if (p->hp<=0){
        if (p->rollback>0){
            p->rollback--; p->hp = p->maxhp/2; p->shield = p->shield_max/2;
            p->revive_t = 3.0f;              // 3s of grace to get clear
            // and shove the swarm that killed you off your face
            for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
                float dx=G.bugs[i].x-p->x, dy=G.bugs[i].y-p->y, d2=dx*dx+dy*dy;
                if (d2 < 140*140 && d2>0.01f){ float d=fsqrt(d2); G.bugs[i].vx += dx/d*340; G.bugs[i].vy += dy/d*340; }
            }
            spawn_floater(p->x, p->y-34, "ROLLBACK!", COL(120,255,160));
            splat(p->x, p->y, COL(120,255,160), 22, 160);
            sfx(12);
            return;
        }
        p->alive=0; p->dead_t=0;
        // the hat comes crashing down
        for (int i=0;i<p->stack_n && i<26;i++) spawn_particle(p->x, p->y-20, BUG_COL[i%4], 150, 0.9f, 3);
        p->stack_n=0; p->carried=0;
        sfx(13);
    }
}

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

// ---------------------------------------------------------------------------
// Interlocked walls (Daniel's rules): a wall placed touching others joins
// their RUN. A run shares ONE health pool worth 1.1x the sum of its
// segments' individual health; bugs chewing anywhere drain the pool, and
// the segment being chewed gives way once the damage since the last break
// passes its own share. A placement that would close a complete circuit
// is refused. Runs of 5+ grow cosmetic tiers — and you can walk on top.
// ---------------------------------------------------------------------------
static float grp_pool[MAXWALLS];   // shared health remaining (leader-indexed)
static float grp_cap[MAXWALLS];    // 1.1 * sum of member base hp
static float grp_dsb[MAXWALLS];    // damage since the last segment broke
static int   grp_n[MAXWALLS];      // member count
static int grp_tier(int n){ return n>=100? 4 : n>=20? 3 : n>=10? 2 : n>=5? 1 : 0; }
static int wall_adjacent(Wall *a, Wall *b){
    float ahx = a->horiz? 26:9, ahy = a->horiz? 9:26;
    float bhx = b->horiz? 26:9, bhy = b->horiz? 9:26;
    float dx = a->x-b->x; if (dx<0) dx=-dx;
    float dy = a->y-b->y; if (dy<0) dy=-dy;
    return dx < ahx+bhx+10 && dy < ahy+bhy+10;
}
static int uf_find(int *par, int i){ while (par[i]!=i){ par[i]=par[par[i]]; i=par[i]; } return i; }
// rebuild the groups. Pool health carries across reshapes by share: a
// wall coming from an old group brings that group's remaining pool in
// proportion to its base; a loner brings what it had left, plus the
// interlock bonus on the remainder. fresh=1 (save load): pool = cap x
// the saved damage fraction instead.
static void walls_recompute_ex(int fresh){
    static int par[MAXWALLS], lead[MAXWALLS];
    // Nov 15: snapshot the OUTGOING pool state before anything is
    // rebuilt. The carry below used to test grp_n[grp_old] AFTER
    // grp_n had been recounted for the NEW grouping — when a run's
    // leader died, the survivors silently fell out of the share
    // branch, and a chewed loner's pool record (never updated by
    // loner damage) let it rejoin a run at health it no longer had.
    static int oldn[MAXWALLS]; static float oldcap[MAXWALLS], oldpool[MAXWALLS];
    for (int i=0;i<MAXWALLS;i++){ oldn[i]=grp_n[i]; oldcap[i]=grp_cap[i]; oldpool[i]=grp_pool[i]; }
    int sim = (G.net_mode!=NET_CLIENT);
    for (int i=0;i<MAXWALLS;i++){ par[i]=i; lead[i]=-1; }
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1)
        for (int j=i+1;j<MAXWALLS;j++) if (G.walls[j].alive==1 && wall_adjacent(&G.walls[i], &G.walls[j])){
            int a=uf_find(par,i), b=uf_find(par,j); if (a!=b) par[b]=a;
        }
    static float npool[MAXWALLS], ncap[MAXWALLS];
    for (int i=0;i<MAXWALLS;i++){ npool[i]=0; ncap[i]=0; grp_n[i]=0; }
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1)
        G.walls[i].grp = uf_find(par,i);   // root for now; leader fixed below
    // leader must be the smallest member index (stable identity)
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1){
        int r = G.walls[i].grp;
        if (lead[r]<0 || i<lead[r]) lead[r]=i;
    }
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1){
        int L = lead[G.walls[i].grp]; G.walls[i].grp = L;
        // a reinforced stack counts as the several walls it is: it
        // feeds the run's tiers, its walk-on-top size and its pool
        // membership exactly like the separate entries it replaced
        grp_n[L] += G.walls[i].stack>0? G.walls[i].stack : 1;
        ncap[L] += G.walls[i].maxhp;
    }
    if (sim){
        for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1){
            Wall *w=&G.walls[i]; int L=w->grp;
            if (fresh) npool[L] += w->hp * 1.1f;                    // saved hp was a fraction of base; x1.1 rebuilds the pool share
            else if (w->grp_old>=0 && oldn[w->grp_old]>=2 && oldcap[w->grp_old]>0)
                npool[L] += oldpool[w->grp_old] * (w->maxhp*1.1f) / oldcap[w->grp_old];
            else npool[L] += w->hp + w->maxhp*0.1f;                 // loner/new: what it had + interlock bonus on the base
        }
        for (int i=0;i<MAXWALLS;i++) if (grp_n[i]>0){
            grp_cap[i] = ncap[i]*1.1f;
            grp_pool[i] = npool[i]; if (grp_pool[i]>grp_cap[i]) grp_pool[i]=grp_cap[i]; if (grp_pool[i]<0) grp_pool[i]=0;
            grp_dsb[i]=0;
            if (grp_n[i]>=2){
                float frac = grp_cap[i]>0? grp_pool[i]/grp_cap[i] : 0;
                for (int m=0;m<MAXWALLS;m++) if (G.walls[m].alive==1 && G.walls[m].grp==i)
                    G.walls[m].hp = G.walls[m].maxhp * frac;
            }
        }
        if (G.net_mode==NET_HOST) wall_send_t = 0.05f;
    }
    // endpoints: members with at most one neighbour inside the run
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1){
        int cnt=0;
        for (int j=0;j<MAXWALLS;j++) if (j!=i && G.walls[j].alive==1 && G.walls[j].grp==G.walls[i].grp && wall_adjacent(&G.walls[i], &G.walls[j])) cnt++;
        G.walls[i].endpt = (cnt<=1 && grp_n[G.walls[i].grp]>=2)? 1:0;
    }
    for (int i=0;i<MAXWALLS;i++) G.walls[i].grp_old = G.walls[i].grp;
}
static void walls_recompute(void){ walls_recompute_ex(0); }
static void break_wall(int wi){
    Wall *w=&G.walls[wi];
    // Reinforcement (restored Nov 7): a stacked wall gives way ONE
    // layer at a time — the top course crumbles off, and the rest of
    // the pile, with its remaining share of the run's pool, fights
    // on. Only a stack chewed down to its last layer, or one drained
    // dry (hp ~0), falls for good.
    if (w->stack>1 && w->hp>1.0f){
        float share = w->maxhp / w->stack;
        w->stack--;
        w->maxhp -= share; if (w->maxhp<1.0f) w->maxhp=1.0f;
        if (w->hp>w->maxhp) w->hp=w->maxhp;
        splat(w->x, w->y, COL(170,120,70), 10, 110);
        push_ev(2, w->x, w->y);
        if (G.net_mode==NET_HOST) wall_send_t = 0.05f;
        sfx(17);
        walls_recompute();
        return;
    }
    w->alive=0; w->grp=-1; w->grp_old=-1;
    splat(w->x, w->y, COL(170,120,70), 14, 120);
    push_ev(2, w->x, w->y);
    if (G.net_mode==NET_HOST) wall_send_t = 0.05f;
    sfx(17);
    walls_recompute();
}
static void damage_wall(int wi, float dmg){
    Wall *w=&G.walls[wi];
    if (w->alive!=1) return;
    int L = w->grp;
    if (L>=0 && grp_n[L]>=2){
        grp_pool[L] -= dmg; grp_dsb[L] += dmg;
        float frac = grp_cap[L]>0? grp_pool[L]/grp_cap[L] : 0; if (frac<0) frac=0;
        for (int m=0;m<MAXWALLS;m++) if (G.walls[m].alive==1 && G.walls[m].grp==L)
            G.walls[m].hp = G.walls[m].maxhp * frac;
        // the chewed segment gives way after taking 1.1x ONE layer's
        // share — for a reinforced stack that sheds a layer, not the pile
        float layer_hp = w->maxhp / (w->stack>0? w->stack : 1);
        if (grp_pool[L]<=0 || grp_dsb[L] >= 1.1f*layer_hp) break_wall(wi);
    } else {
        w->hp -= dmg;
        // keep a loner's pool record truthful (Nov 15): the record
        // is what the share carry reads when this wall later joins
        // a run — left stale, a chewed wall rejoined at the health
        // it had when the record was last written, not its own.
        if (w->grp>=0){ grp_cap[w->grp] = w->maxhp*1.1f;
            grp_pool[w->grp] = w->hp + w->maxhp*0.1f; if (grp_pool[w->grp]<0) grp_pool[w->grp]=0; }
        if (w->hp<=0) break_wall(wi);
    }
}
// would a wall at (wx,wy,horiz) close a circuit? For every pair of its
// neighbours already in one run, walk the existing path between them:
// if that path plus the new wall encloses real area, it's a loop.
static int placement_closes_loop(float wx, float wy, int horiz){
    Wall cand; cand.alive=1; cand.horiz=horiz; cand.x=wx; cand.y=wy;
    int nb[8], nn=0;
    for (int i=0;i<MAXWALLS && nn<8;i++)
        if (G.walls[i].alive==1 && wall_adjacent(&cand, &G.walls[i])) nb[nn++]=i;
    for (int a=0;a<nn;a++) for (int b=a+1;b<nn;b++){
        if (G.walls[nb[a]].grp != G.walls[nb[b]].grp || G.walls[nb[a]].grp<0) continue;
        // BFS the run from nb[a] to nb[b]
        static int par2[MAXWALLS]; static int qq[MAXWALLS];
        for (int i=0;i<MAXWALLS;i++) par2[i]=-2;
        int qh=0, qt=0; qq[qt++]=nb[a]; par2[nb[a]]=-1;
        while (qh<qt && par2[nb[b]]==-2){
            int cur=qq[qh++];
            for (int j=0;j<MAXWALLS;j++)
                if (G.walls[j].alive==1 && G.walls[j].grp==G.walls[cur].grp && par2[j]==-2 && wall_adjacent(&G.walls[cur], &G.walls[j])){
                    par2[j]=cur; qq[qt++]=j;
                }
        }
        if (par2[nb[b]]==-2) continue;
        // shoelace over the closed polygon: path points, then the
        // candidate, then back to the start — EVERY edge counts
        // (dropping the candidate's closing edge once silently
        // shrank the area and let loops through)
        float pts[440][2]; int np=0;
        for (int c=nb[b]; c!=-1; c=par2[c]){ pts[np][0]=G.walls[c].x; pts[np][1]=G.walls[c].y; np++; }
        pts[np][0]=wx; pts[np][1]=wy; np++;
        float area=0;
        for (int i=0;i<np;i++){
            int j=(i+1)%np;
            area += pts[i][0]*pts[j][1] - pts[j][0]*pts[i][1];
        }
        if (area<0) area=-area;
        if (area*0.5f > 2400.0f) return 1;
    }
    return 0;
}

static int wall_box_hit(float x, float y, float rad, Wall *w){
    float hx = w->horiz? 26.0f : 9.0f, hy = w->horiz? 9.0f : 26.0f;
    return (x > w->x-hx-rad && x < w->x+hx+rad && y > w->y-hy-rad && y < w->y+hy+rad);
}
// The Bug Mall is SOLID — the whole storefront: building, awning,
// counter and crates (the same footprint tap_on_store uses). Entry
// is by INTERACT from the plaza, so the collision needs no door
// gap. Applied to players, bugs, bots and buddies: nothing walks
// through the mall. (It lives inside collide_walls too, and is
// called directly by the movers that skip wall collision.)
static void collide_hut(float *x, float *y, float rad){
    if (*x > -92-rad && *x < 92+rad && *y > -116-rad && *y < -14+rad){
        float dyl = *y-(-116-rad), dyr = (-14+rad)-*y;
        float dxl = *x-(-92-rad), dxr = (92+rad)-*x;
        float m=dxl; int side=0;
        if (dxr<m){ m=dxr; side=1; } if (dyl<m){ m=dyl; side=2; } if (dyr<m){ side=3; }
        if (side==0) *x=-92-rad; else if (side==1) *x=92+rad;
        else if (side==2) *y=-116-rad; else *y=-14+rad;
    }
}
static void collide_walls(float *x, float *y, float rad){
    for (int i=0;i<MAXWALLS;i++){
        Wall *w=&G.walls[i];
        if (w->alive!=1) continue;
        float hx = w->horiz? 26.0f : 9.0f, hy = w->horiz? 9.0f : 26.0f;
        if (*x > w->x-hx-rad && *x < w->x+hx+rad && *y > w->y-hy-rad && *y < w->y+hy+rad){
            float dxl = *x-(w->x-hx-rad), dxr = (w->x+hx+rad)-*x;
            float dyl = *y-(w->y-hy-rad), dyr = (w->y+hy+rad)-*y;
            float m = dxl; int side=0;
            if (dxr<m){ m=dxr; side=1; } if (dyl<m){ m=dyl; side=2; } if (dyr<m){ side=3; }
            if (side==0) *x = w->x-hx-rad; else if (side==1) *x = w->x+hx+rad;
            else if (side==2) *y = w->y-hy-rad; else *y = w->y+hy+rad;
        }
    }
    collide_hut(x, y, rad);
}
// is the player still standing over his own run?
static int over_own_run(Player *p){
    if (p->on_wall<0) return 0;
    for (int i=0;i<MAXWALLS;i++)
        if (G.walls[i].alive==1 && G.walls[i].grp==p->on_wall && wall_box_hit(p->x, p->y, 11, &G.walls[i])) return 1;
    return 0;
}
// wall collision with a door in it: step INTO a cosmetically-merged run
// (5+ interlocked) and you're up on top of it, walking the wall. Step
// off the run anywhere and you drop back to the ground.
static void wall_step(Player *p){
    if (p->in_mall){ p->on_wall=-1; return; }
    int hit_grp=-1, hit_other=0;
    for (int i=0;i<MAXWALLS;i++){
        Wall *w=&G.walls[i];
        if (w->alive!=1) continue;
        if (!wall_box_hit(p->x, p->y, 11, w)) continue;
        if (w->grp>=0 && grp_n[w->grp]>=5){ if (hit_grp<0) hit_grp=w->grp; }
        else hit_other=1;
    }
    if (hit_grp>=0 && !hit_other){ p->on_wall=hit_grp; return; }   // up (or still up)
    if (hit_other){
        collide_walls(&p->x, &p->y, 11);
        if (p->on_wall>=0 && !over_own_run(p)) p->on_wall=-1;
        return;
    }
    if (p->on_wall>=0 && !over_own_run(p)) p->on_wall=-1;           // stepped off the end
}
// the buddy climbs exactly when the player can (Daniel): same runs of
// 5+ interlocked walls, same drop-off rule — wall_step, mirrored.
static int buddy_over_run(Buddy *b){
    if (b->on_wall<0) return 0;
    for (int i=0;i<MAXWALLS;i++)
        if (G.walls[i].alive==1 && G.walls[i].grp==b->on_wall && wall_box_hit(b->x, b->y, 9, &G.walls[i])) return 1;
    return 0;
}
static void buddy_wall_step(Buddy *b){
    int hit_grp=-1, hit_other=0;
    for (int i=0;i<MAXWALLS;i++){
        Wall *w=&G.walls[i];
        if (w->alive!=1) continue;
        if (!wall_box_hit(b->x, b->y, 9, w)) continue;
        if (w->grp>=0 && grp_n[w->grp]>=5){ if (hit_grp<0) hit_grp=w->grp; }
        else hit_other=1;
    }
    if (hit_grp>=0 && !hit_other){ b->on_wall=hit_grp; return; }
    if (hit_other){
        collide_walls(&b->x, &b->y, 9);
        if (b->on_wall>=0 && !buddy_over_run(b)) b->on_wall=-1;
        return;
    }
    if (b->on_wall>=0 && !buddy_over_run(b)) b->on_wall=-1;
}
static int near_store(Player *p){
    float dx=p->x, dy=p->y+10;
    return (dx*dx+dy*dy < 130*130);
}
// a tap only opens the store when it lands ON the store building —
// tapping anywhere else in the plaza is just a tap (mobile drag fix)
static int SX(float wx); static int SY(float wy);   // defined with the renderer
static float zoom_target_of(Player *p);             // defined with the renderer
static int tap_on_store(void){
    return IN.tap_x>=SX(-92) && IN.tap_x<=SX(92) &&
           IN.tap_y>=SY(-116) && IN.tap_y<=SY(-14);
}

