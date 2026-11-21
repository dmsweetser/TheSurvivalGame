// THE SURVIVAL GAME — enemies/elites.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 974-1019 in the Nov 2026 fine-grained refactor; code unchanged.

static float fs_flash_t = 0;      // step-done banner timer (play time only)
static int   fs_flash_amt = 0;    // the XP the banner is celebrating
static int   fs_flash_fin = 0;    // the banner is the finale
// WEEKLY CHALLENGE (v1.25 feature 6): the completion / new-week
// banner state, twin of the First Shift flash. wk_flash_kind:
// 0 none, 1 challenge complete, 2 a new week rolled in mid-play.
// The challenge core lives in save.c; the banner draws in world.c.
static float wk_flash_t = 0;
static int   wk_flash_kind = 0;
// NAMED ELITES (v1.25 feature 8): the claim banner state, third in
// the flash stack. bn_flash_name is the claimed elite's name index
// (the banner names the fallen). The bounty core lives in save.c;
// the banner and the tracker draw in world.c; the tables live here
// with the bug machinery they describe.
static float bn_flash_t = 0;
static int   bn_flash_name = -1;
#define BN_XP 25          // one expedition-sized hunt: Castle pays 50, the Weekly 50 for a week
#define BN_COOLDOWN 150.0f   // seconds of play between a claim and the next crawl-out
#define BN_WCAP 500.0f    // the huntable ceiling on elite weight (kg)
static const int ELITE_N = 12;
static const char *ELITE_NAME[12] = {
    "NULLPOINTER", "SEGFAULT", "HEISENBUG", "OFFBYONE", "DEADLOCK", "MEMORY LEAK",
    "RACE CONDITION", "INFINITE LOOP", "SPAGHETTI CODE", "BLUE SCREEN", "KERNEL PANIC", "HOTFIX"
};
static const char *ELITE_TITLE[12] = {
    "DEVOURER OF STACKS", "THE UNHANDLED EXCEPTION", "GONE WHEN YOU LOOK AT IT",
    "ONE MORE THAN YOU COUNTED", "FIRST TO BITE, LAST TO LET GO",
    "BIGGER EVERY TIME YOU LOOK AWAY", "ALREADY BEHIND YOU",
    "IT COMES BACK. IT COMES BACK.", "A KNOT WITH LEGS",
    "THE ONE THAT ENDS THE DAY", "EVEN THE KERNEL RAN AWAY",
    "SHIPPED ON A FRIDAY. STILL HERE."
};
static const char *ELITE_KINDNAME[4] = { "ROACH", "BEETLE", "SPIDER", "MOTH" };
// the bounty's identity is a pure function of the serial — hash2,
// never the gameplay RNG — so every install posts the same sequence
// of names and a save needs nothing but the counter.
static int bn_name_idx(int serial){ return (int)(hash2(serial, 31337, 0xB0B1E5u) % 12u); }
static int bn_kind_of(int serial){ return (int)(hash2(serial, 271828, 0x5EED5u) % 4u); }
// set by the spawner's bounty roll (and by dbg_bnforce): the next
// bug spawn_bug brings into the world is the bounty's elite,
// subject to the elevation gate in spawn_bug itself.
static int bn_force = 0;
static int bn_any_live(void){
    for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive && G.bugs[i].elite) return i;
    return -1;
}
