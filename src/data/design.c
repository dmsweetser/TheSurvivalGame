// THE SURVIVAL GAME — data/design.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 1-218 in the Nov 2026 fine-grained refactor; code unchanged.

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
// LIVE TUNING (v1.26 Item 3): mutable — the registry in data/livedata.c
// points here; the literals below are the stock defaults.
static int weap_price[NWEAP] = { 0, 25, 60, 120, 200 };
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
// LIVE TUNING (v1.26 Item 3): these were #defines; every use site is a
// plain float expression (no constant-expression contexts — audited), so
// they are variables now and the registry points at them.
static float BEAM_EXT = 0.18f;       // beams grow to full reach
static float BEAM_SPIN = 0.85f;      // one full revolution
static float BEAM_RET = 0.15f;       // beams close (no damage while closing)
static float BEAM_REST = 0.55f;      // dead time before the next sweep

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
// LIVE TUNING (v1.26 Item 3): mutable copy (see weap_price above). The
// -1 entries are "no more levels" sentinels and are NOT registered.
static int upg_price_tab[NUPG][4] = {   // price per level, -1 = no more
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
    for (int i=0;i<4;i++) if (upg_price_tab[u][i]>0){ last = upg_price_tab[u][i]; li = i; }
    if (lv<=li) return upg_price_tab[u][lv];
    return last * (lv - li + 1);
}

// THE SOURCE ROOM gate (v1.26 Item 1a + Item 3): trophies needed to open
// the Mall's Source door. A registered LIVE TUNING variable (default 3,
// range 0..20) — the Mall door check reads this, never a #define.
static int SRC_DOOR_TROPHIES = 3;

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
    int top_worn;                     // HAT TOPPERS (feat 5): runtime mirror of Buddy.top_worn, the only thing draw_hat reads (0 = none)
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
    int elite;                        // v1.25 feat 8: 0 = ordinary; else the bounty's name index + 1
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

// MORDOR FUN RUN — THE MOOSE SCALE (v1.28): the whole Shire->Mordor
// road is 1000 MOOSE long, one moose being 460 world units at the
// game's scale (the living ruler stands by the start line in
// mordor_render.c). Total route 460,000 units: at buddy speeds
// (118-195 u/s) that is roughly 40-65 minutes of running, a long-haul
// goal chipped across sessions — exactly what the save/resume is for.
// 20 milestone markers, one every 50 moose (23,000 units, ~2 minutes
// apart). The route's distance axis s runs 0 (the Shire, y=MOR_Y0)
// to MOR_LEN (Mordor, y=MOR_Y0-MOR_LEN), northward like Tank's field.
// (Defined here, ahead of the Buddy struct, because save/start.c —
// included before the mode itself — clamps the mu token with them.)
#define MOR_MOOSE_PX 460
#define MOR_TOTAL_MOOSE 1000
#define MOR_LEN (MOR_TOTAL_MOOSE * MOR_MOOSE_PX)
#define MOR_Y0 460000
#define MOR_MILES 20
#define MOR_CHUNK 300
#define MOR_MAXEN 40

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
    int ent2_best, ent2_done;         // DEEPER DOORS records (v1.25 feat 11): deepest Chapter 2 room, Deeper completions; saved packed as e2
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
    int cas_shufn;                    // CASTLE SHUFFLE (v1.25 feat 7): shuffle runs dealt so far; the run seed, saved as cs
    int crt_best_pos, crt_best_time;  // SUPER BUDDY CART records: best finish (0 = none) as cp, best race time in deciseconds as cq
    int crt_cupbest;                  // best Grand Prix rank per cup, packed: home + far*8 (0 = unranked), as cr
    int crt_tbest[8];                 // MEDAL TIMES (v1.25 feat 3): best race time per track in deciseconds (0 = none), as mc; medals DERIVE from these
    int crt_medal_best[8];            // v1.27: best Cart medal EARNED per track (0..3), as md — a finish earns its time tier capped by placement (podium only), so the medal is banked at the finish, not derived from the time. Saves with mc but no md are pre-v1.27: their medals migrate from the times at load (crt_migrate_medals)
    int plat_tbest[16];               // MEDAL TIMES: best clear time per Run level 1..16 in deciseconds (0 = none), as mr
    int war_prog;                     // WAR! record: regions conquered (0..8); the world map unlocks region war_prog next, as wr
    int mor_done, mor_best;           // MORDOR FUN RUN records (v1.28): completions, best total time in seconds (0 = none); saved as mo/mb
    int mor_on, mor_dist, mor_mile, mor_hp, mor_secs, mor_lat;   // MORDOR FUN RUN: the in-progress run itself (v1.28) — on the road or not, distance along the route in world units, milestone banked (0..20), HP at save, elapsed seconds, lateral offset at save; saved packed as mu. The run lives here, not in BwSim, so leaving the cabinet / quitting / closing the browser all keep it.
    int scr_champ;                    // CRISPY DIRT record (v1.35): 1 once a HARD solo match is won; saved as sc. The cabinet's bits/inventory are session-only and never saved — this plaque is all that persists, with the XP.
    int top_earned, top_worn;         // HAT TOPPERS (v1.25 feat 5): earned bitmask (bit id-1), worn topper id 0..8; saved as tp/tw
    float roll;                       // the pet rock's rolling angle
    float steady_t;                   // pet rock's ROCK STEADY: unhurtable while > 0
    float tx, ty;                     // render interpolation target (client)
} Buddy;

// MEDAL TIMES (v1.25 feat 3): a medal is DERIVED from a stored best time
// against a {gold, silver, bronze} decisecond threshold row — never
// stored itself — so thresholds can be retuned without migrating saves.
// Tier: 0 = none, 1 = bronze, 2 = silver, 3 = gold.
