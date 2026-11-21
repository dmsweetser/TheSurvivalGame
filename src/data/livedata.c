// THE SURVIVAL GAME — data/livedata.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
//
// LIVE TUNING (v1.26 Item 3) — the live-data tier of the Source terminal.
// A registry of design tunables the game reads through LIVE variables:
// the terminal's TUNE view edits them with range validation, they apply
// immediately (no compiler), and the override set persists via plat_mod
// under the reserved pseudo-path "livedata.txt" (one name=value per
// line), reapplied at boot. The shipped code is never modified by this
// tier — only validated data moves; a bad override set can at worst be
// ignored (unknown names skipped, out-of-range values rejected one by
// one, an oversized/malformed blob ignored wholesale, boot never fails).
//
// Audit notes (what is NOT here, and why): weap_dmg/weap_cd are
// function-encoded ternary chains, not tables — audited out. UPG_MAXLV
// carries sentinel semantics (999/0) — out. The -1 "no more levels"
// entries of upg_price_tab are sentinels, not prices — out. Bug stats
// proper are formulas over weight (spawn_bug in game/difficulty.c), not
// a per-strain table; the table-driven bug-facing numbers that DO exist
// are the DIFF_* difficulty tables and THREAT_RATIO, and they are in.
// MAX*/array-size #defines fail the constant-expression audit by
// construction — out.
//
// Placement: after game/srcedit.c so every registered variable except
// the two medal tables is already defined; the medal tables are
// declared tentatively here and initialized in their owner files.

// tentative declarations — initialized definitions live in
// modes/run/run_levels.c (plat_medal) and modes/cart/cart_track.c (crt_medal)
static int plat_medal[16][3];
static int crt_medal[8][3];

typedef struct {
    const char *name;     // livedata.txt key + tune-list label
    const char *file;     // src-relative file holding the literal
    int kind;             // 0 = int, 1 = float
    void *ptr;            // the live variable
    float vmin, vmax;     // validation range (inclusive)
    const char *anchor;   // substitution anchor in the file text
    int elem;             // which numeric literal after the anchor (0-based)
} TuneEntry;

static const TuneEntry TUNE_REG[] = {
    { "SRC_DOOR_TROPHIES", "data/design.c", 0, &SRC_DOOR_TROPHIES, 0, 20, "SRC_DOOR_TROPHIES", 0 },
    { "BEAM_EXT", "data/design.c", 1, &BEAM_EXT, 0.01f, 10.0f, "BEAM_EXT", 0 },
    { "BEAM_SPIN", "data/design.c", 1, &BEAM_SPIN, 0.01f, 10.0f, "BEAM_SPIN", 0 },
    { "BEAM_RET", "data/design.c", 1, &BEAM_RET, 0.01f, 10.0f, "BEAM_RET", 0 },
    { "BEAM_REST", "data/design.c", 1, &BEAM_REST, 0.01f, 10.0f, "BEAM_REST", 0 },
    { "WEAPON_PRICE_0", "data/design.c", 0, &weap_price[0], 0, 10000, "weap_price[NWEAP]", 0 },
    { "WEAPON_PRICE_1", "data/design.c", 0, &weap_price[1], 0, 10000, "weap_price[NWEAP]", 1 },
    { "WEAPON_PRICE_2", "data/design.c", 0, &weap_price[2], 0, 10000, "weap_price[NWEAP]", 2 },
    { "WEAPON_PRICE_3", "data/design.c", 0, &weap_price[3], 0, 10000, "weap_price[NWEAP]", 3 },
    { "WEAPON_PRICE_4", "data/design.c", 0, &weap_price[4], 0, 10000, "weap_price[NWEAP]", 4 },
    { "UPG_PRICE_0_0", "data/design.c", 0, &upg_price_tab[0][0], 0, 400, "upg_price_tab[NUPG][4]", 0 },
    { "UPG_PRICE_0_1", "data/design.c", 0, &upg_price_tab[0][1], 0, 900, "upg_price_tab[NUPG][4]", 1 },
    { "UPG_PRICE_0_2", "data/design.c", 0, &upg_price_tab[0][2], 0, 1800, "upg_price_tab[NUPG][4]", 2 },
    { "UPG_PRICE_0_3", "data/design.c", 0, &upg_price_tab[0][3], 0, 3600, "upg_price_tab[NUPG][4]", 3 },
    { "UPG_PRICE_1_0", "data/design.c", 0, &upg_price_tab[1][0], 0, 350, "upg_price_tab[NUPG][4]", 4 },
    { "UPG_PRICE_1_1", "data/design.c", 0, &upg_price_tab[1][1], 0, 800, "upg_price_tab[NUPG][4]", 5 },
    { "UPG_PRICE_1_2", "data/design.c", 0, &upg_price_tab[1][2], 0, 1700, "upg_price_tab[NUPG][4]", 6 },
    { "UPG_PRICE_1_3", "data/design.c", 0, &upg_price_tab[1][3], 0, 3400, "upg_price_tab[NUPG][4]", 7 },
    { "UPG_PRICE_2_0", "data/design.c", 0, &upg_price_tab[2][0], 0, 500, "upg_price_tab[NUPG][4]", 8 },
    { "UPG_PRICE_2_1", "data/design.c", 0, &upg_price_tab[2][1], 0, 1100, "upg_price_tab[NUPG][4]", 9 },
    { "UPG_PRICE_2_2", "data/design.c", 0, &upg_price_tab[2][2], 0, 2300, "upg_price_tab[NUPG][4]", 10 },
    { "UPG_PRICE_3_0", "data/design.c", 0, &upg_price_tab[3][0], 0, 450, "upg_price_tab[NUPG][4]", 12 },
    { "UPG_PRICE_3_1", "data/design.c", 0, &upg_price_tab[3][1], 0, 950, "upg_price_tab[NUPG][4]", 13 },
    { "UPG_PRICE_3_2", "data/design.c", 0, &upg_price_tab[3][2], 0, 2000, "upg_price_tab[NUPG][4]", 14 },
    { "UPG_PRICE_4_0", "data/design.c", 0, &upg_price_tab[4][0], 0, 300, "upg_price_tab[NUPG][4]", 16 },
    { "UPG_PRICE_4_1", "data/design.c", 0, &upg_price_tab[4][1], 0, 750, "upg_price_tab[NUPG][4]", 17 },
    { "UPG_PRICE_5_0", "data/design.c", 0, &upg_price_tab[5][0], 0, 1500, "upg_price_tab[NUPG][4]", 20 },
    { "UPG_PRICE_5_1", "data/design.c", 0, &upg_price_tab[5][1], 0, 1500, "upg_price_tab[NUPG][4]", 21 },
    { "UPG_PRICE_5_2", "data/design.c", 0, &upg_price_tab[5][2], 0, 1500, "upg_price_tab[NUPG][4]", 22 },
    { "UPG_PRICE_5_3", "data/design.c", 0, &upg_price_tab[5][3], 0, 1500, "upg_price_tab[NUPG][4]", 23 },
    { "UPG_PRICE_6_0", "data/design.c", 0, &upg_price_tab[6][0], 0, 200, "upg_price_tab[NUPG][4]", 24 },
    { "UPG_PRICE_7_0", "data/design.c", 0, &upg_price_tab[7][0], 0, 5000, "upg_price_tab[NUPG][4]", 28 },
    { "UPG_PRICE_7_1", "data/design.c", 0, &upg_price_tab[7][1], 0, 5000, "upg_price_tab[NUPG][4]", 29 },
    { "UPG_PRICE_7_2", "data/design.c", 0, &upg_price_tab[7][2], 0, 5000, "upg_price_tab[NUPG][4]", 30 },
    { "UPG_PRICE_7_3", "data/design.c", 0, &upg_price_tab[7][3], 0, 5000, "upg_price_tab[NUPG][4]", 31 },
    { "THREAT_RATIO", "enemies/bugs.c", 1, &THREAT_RATIO, 0.0f, 2.0f, "THREAT_RATIO", 0 },
    { "DIFF_HP_EASY", "game/difficulty.c", 1, &diff_hp[0], 0.1f, 26.0f, "diff_hp[4]", 0 },
    { "DIFF_HP_MEDIUM", "game/difficulty.c", 1, &diff_hp[1], 0.1f, 26.0f, "diff_hp[4]", 1 },
    { "DIFF_HP_HARD", "game/difficulty.c", 1, &diff_hp[2], 0.1f, 26.0f, "diff_hp[4]", 2 },
    { "DIFF_HP_BUGAGGEDON", "game/difficulty.c", 1, &diff_hp[3], 0.1f, 26.0f, "diff_hp[4]", 3 },
    { "DIFF_DMG_EASY", "game/difficulty.c", 1, &diff_dmg[0], 0.0f, 22.0f, "diff_dmg[4]", 0 },
    { "DIFF_DMG_MEDIUM", "game/difficulty.c", 1, &diff_dmg[1], 0.0f, 22.0f, "diff_dmg[4]", 1 },
    { "DIFF_DMG_HARD", "game/difficulty.c", 1, &diff_dmg[2], 0.0f, 22.0f, "diff_dmg[4]", 2 },
    { "DIFF_DMG_BUGAGGEDON", "game/difficulty.c", 1, &diff_dmg[3], 0.0f, 22.0f, "diff_dmg[4]", 3 },
    { "DIFF_SPD_EASY", "game/difficulty.c", 1, &diff_spd[0], 0.1f, 12.5f, "diff_spd[4]", 0 },
    { "DIFF_SPD_MEDIUM", "game/difficulty.c", 1, &diff_spd[1], 0.1f, 12.5f, "diff_spd[4]", 1 },
    { "DIFF_SPD_HARD", "game/difficulty.c", 1, &diff_spd[2], 0.1f, 12.5f, "diff_spd[4]", 2 },
    { "DIFF_SPD_BUGAGGEDON", "game/difficulty.c", 1, &diff_spd[3], 0.1f, 12.5f, "diff_spd[4]", 3 },
    { "DIFF_POP_EASY", "game/difficulty.c", 1, &diff_pop[0], 0.0f, 24.0f, "diff_pop[4]", 0 },
    { "DIFF_POP_MEDIUM", "game/difficulty.c", 1, &diff_pop[1], 0.0f, 24.0f, "diff_pop[4]", 1 },
    { "DIFF_POP_HARD", "game/difficulty.c", 1, &diff_pop[2], 0.0f, 24.0f, "diff_pop[4]", 2 },
    { "DIFF_POP_BUGAGGEDON", "game/difficulty.c", 1, &diff_pop[3], 0.0f, 24.0f, "diff_pop[4]", 3 },
    { "DIFF_RATE_EASY", "game/difficulty.c", 1, &diff_rate[0], 0.1f, 22.0f, "diff_rate[4]", 0 },
    { "DIFF_RATE_MEDIUM", "game/difficulty.c", 1, &diff_rate[1], 0.1f, 22.0f, "diff_rate[4]", 1 },
    { "DIFF_RATE_HARD", "game/difficulty.c", 1, &diff_rate[2], 0.1f, 22.0f, "diff_rate[4]", 2 },
    { "DIFF_RATE_BUGAGGEDON", "game/difficulty.c", 1, &diff_rate[3], 0.1f, 22.0f, "diff_rate[4]", 3 },
    { "DIFF_W_EASY", "game/difficulty.c", 1, &diff_w[0], 0.0f, 16.0f, "diff_w[4]", 0 },
    { "DIFF_W_MEDIUM", "game/difficulty.c", 1, &diff_w[1], 0.0f, 16.0f, "diff_w[4]", 1 },
    { "DIFF_W_HARD", "game/difficulty.c", 1, &diff_w[2], 0.0f, 16.0f, "diff_w[4]", 2 },
    { "DIFF_W_BUGAGGEDON", "game/difficulty.c", 1, &diff_w[3], 0.0f, 16.0f, "diff_w[4]", 3 },
    { "PLAT_MEDAL_01_G", "modes/run/run_levels.c", 0, &plat_medal[0][0], 1, 10000, "plat_medal[16][3]", 0 },
    { "PLAT_MEDAL_01_S", "modes/run/run_levels.c", 0, &plat_medal[0][1], 1, 10000, "plat_medal[16][3]", 1 },
    { "PLAT_MEDAL_01_B", "modes/run/run_levels.c", 0, &plat_medal[0][2], 1, 10000, "plat_medal[16][3]", 2 },
    { "PLAT_MEDAL_02_G", "modes/run/run_levels.c", 0, &plat_medal[1][0], 1, 10000, "plat_medal[16][3]", 3 },
    { "PLAT_MEDAL_02_S", "modes/run/run_levels.c", 0, &plat_medal[1][1], 1, 10000, "plat_medal[16][3]", 4 },
    { "PLAT_MEDAL_02_B", "modes/run/run_levels.c", 0, &plat_medal[1][2], 1, 10000, "plat_medal[16][3]", 5 },
    { "PLAT_MEDAL_03_G", "modes/run/run_levels.c", 0, &plat_medal[2][0], 1, 10000, "plat_medal[16][3]", 6 },
    { "PLAT_MEDAL_03_S", "modes/run/run_levels.c", 0, &plat_medal[2][1], 1, 10000, "plat_medal[16][3]", 7 },
    { "PLAT_MEDAL_03_B", "modes/run/run_levels.c", 0, &plat_medal[2][2], 1, 10000, "plat_medal[16][3]", 8 },
    { "PLAT_MEDAL_04_G", "modes/run/run_levels.c", 0, &plat_medal[3][0], 1, 10000, "plat_medal[16][3]", 9 },
    { "PLAT_MEDAL_04_S", "modes/run/run_levels.c", 0, &plat_medal[3][1], 1, 10000, "plat_medal[16][3]", 10 },
    { "PLAT_MEDAL_04_B", "modes/run/run_levels.c", 0, &plat_medal[3][2], 1, 10000, "plat_medal[16][3]", 11 },
    { "PLAT_MEDAL_05_G", "modes/run/run_levels.c", 0, &plat_medal[4][0], 1, 10000, "plat_medal[16][3]", 12 },
    { "PLAT_MEDAL_05_S", "modes/run/run_levels.c", 0, &plat_medal[4][1], 1, 10000, "plat_medal[16][3]", 13 },
    { "PLAT_MEDAL_05_B", "modes/run/run_levels.c", 0, &plat_medal[4][2], 1, 10000, "plat_medal[16][3]", 14 },
    { "PLAT_MEDAL_06_G", "modes/run/run_levels.c", 0, &plat_medal[5][0], 1, 10000, "plat_medal[16][3]", 15 },
    { "PLAT_MEDAL_06_S", "modes/run/run_levels.c", 0, &plat_medal[5][1], 1, 10000, "plat_medal[16][3]", 16 },
    { "PLAT_MEDAL_06_B", "modes/run/run_levels.c", 0, &plat_medal[5][2], 1, 10000, "plat_medal[16][3]", 17 },
    { "PLAT_MEDAL_07_G", "modes/run/run_levels.c", 0, &plat_medal[6][0], 1, 10000, "plat_medal[16][3]", 18 },
    { "PLAT_MEDAL_07_S", "modes/run/run_levels.c", 0, &plat_medal[6][1], 1, 10000, "plat_medal[16][3]", 19 },
    { "PLAT_MEDAL_07_B", "modes/run/run_levels.c", 0, &plat_medal[6][2], 1, 10000, "plat_medal[16][3]", 20 },
    { "PLAT_MEDAL_08_G", "modes/run/run_levels.c", 0, &plat_medal[7][0], 1, 10000, "plat_medal[16][3]", 21 },
    { "PLAT_MEDAL_08_S", "modes/run/run_levels.c", 0, &plat_medal[7][1], 1, 10000, "plat_medal[16][3]", 22 },
    { "PLAT_MEDAL_08_B", "modes/run/run_levels.c", 0, &plat_medal[7][2], 1, 10000, "plat_medal[16][3]", 23 },
    { "PLAT_MEDAL_09_G", "modes/run/run_levels.c", 0, &plat_medal[8][0], 1, 10000, "plat_medal[16][3]", 24 },
    { "PLAT_MEDAL_09_S", "modes/run/run_levels.c", 0, &plat_medal[8][1], 1, 10000, "plat_medal[16][3]", 25 },
    { "PLAT_MEDAL_09_B", "modes/run/run_levels.c", 0, &plat_medal[8][2], 1, 10000, "plat_medal[16][3]", 26 },
    { "PLAT_MEDAL_10_G", "modes/run/run_levels.c", 0, &plat_medal[9][0], 1, 10000, "plat_medal[16][3]", 27 },
    { "PLAT_MEDAL_10_S", "modes/run/run_levels.c", 0, &plat_medal[9][1], 1, 10000, "plat_medal[16][3]", 28 },
    { "PLAT_MEDAL_10_B", "modes/run/run_levels.c", 0, &plat_medal[9][2], 1, 10000, "plat_medal[16][3]", 29 },
    { "PLAT_MEDAL_11_G", "modes/run/run_levels.c", 0, &plat_medal[10][0], 1, 10000, "plat_medal[16][3]", 30 },
    { "PLAT_MEDAL_11_S", "modes/run/run_levels.c", 0, &plat_medal[10][1], 1, 10000, "plat_medal[16][3]", 31 },
    { "PLAT_MEDAL_11_B", "modes/run/run_levels.c", 0, &plat_medal[10][2], 1, 10000, "plat_medal[16][3]", 32 },
    { "PLAT_MEDAL_12_G", "modes/run/run_levels.c", 0, &plat_medal[11][0], 1, 10000, "plat_medal[16][3]", 33 },
    { "PLAT_MEDAL_12_S", "modes/run/run_levels.c", 0, &plat_medal[11][1], 1, 10000, "plat_medal[16][3]", 34 },
    { "PLAT_MEDAL_12_B", "modes/run/run_levels.c", 0, &plat_medal[11][2], 1, 10000, "plat_medal[16][3]", 35 },
    { "PLAT_MEDAL_13_G", "modes/run/run_levels.c", 0, &plat_medal[12][0], 1, 10000, "plat_medal[16][3]", 36 },
    { "PLAT_MEDAL_13_S", "modes/run/run_levels.c", 0, &plat_medal[12][1], 1, 10000, "plat_medal[16][3]", 37 },
    { "PLAT_MEDAL_13_B", "modes/run/run_levels.c", 0, &plat_medal[12][2], 1, 10000, "plat_medal[16][3]", 38 },
    { "PLAT_MEDAL_14_G", "modes/run/run_levels.c", 0, &plat_medal[13][0], 1, 10000, "plat_medal[16][3]", 39 },
    { "PLAT_MEDAL_14_S", "modes/run/run_levels.c", 0, &plat_medal[13][1], 1, 10000, "plat_medal[16][3]", 40 },
    { "PLAT_MEDAL_14_B", "modes/run/run_levels.c", 0, &plat_medal[13][2], 1, 10000, "plat_medal[16][3]", 41 },
    { "PLAT_MEDAL_15_G", "modes/run/run_levels.c", 0, &plat_medal[14][0], 1, 10000, "plat_medal[16][3]", 42 },
    { "PLAT_MEDAL_15_S", "modes/run/run_levels.c", 0, &plat_medal[14][1], 1, 10000, "plat_medal[16][3]", 43 },
    { "PLAT_MEDAL_15_B", "modes/run/run_levels.c", 0, &plat_medal[14][2], 1, 10000, "plat_medal[16][3]", 44 },
    { "PLAT_MEDAL_16_G", "modes/run/run_levels.c", 0, &plat_medal[15][0], 1, 10000, "plat_medal[16][3]", 45 },
    { "PLAT_MEDAL_16_S", "modes/run/run_levels.c", 0, &plat_medal[15][1], 1, 10000, "plat_medal[16][3]", 46 },
    { "PLAT_MEDAL_16_B", "modes/run/run_levels.c", 0, &plat_medal[15][2], 1, 10000, "plat_medal[16][3]", 47 },
    { "CRT_MEDAL_1_G", "modes/cart/cart_track.c", 0, &crt_medal[0][0], 1, 10000, "crt_medal[8][3]", 0 },
    { "CRT_MEDAL_1_S", "modes/cart/cart_track.c", 0, &crt_medal[0][1], 1, 10000, "crt_medal[8][3]", 1 },
    { "CRT_MEDAL_1_B", "modes/cart/cart_track.c", 0, &crt_medal[0][2], 1, 10000, "crt_medal[8][3]", 2 },
    { "CRT_MEDAL_2_G", "modes/cart/cart_track.c", 0, &crt_medal[1][0], 1, 10000, "crt_medal[8][3]", 3 },
    { "CRT_MEDAL_2_S", "modes/cart/cart_track.c", 0, &crt_medal[1][1], 1, 10000, "crt_medal[8][3]", 4 },
    { "CRT_MEDAL_2_B", "modes/cart/cart_track.c", 0, &crt_medal[1][2], 1, 10000, "crt_medal[8][3]", 5 },
    { "CRT_MEDAL_3_G", "modes/cart/cart_track.c", 0, &crt_medal[2][0], 1, 10000, "crt_medal[8][3]", 6 },
    { "CRT_MEDAL_3_S", "modes/cart/cart_track.c", 0, &crt_medal[2][1], 1, 10000, "crt_medal[8][3]", 7 },
    { "CRT_MEDAL_3_B", "modes/cart/cart_track.c", 0, &crt_medal[2][2], 1, 10000, "crt_medal[8][3]", 8 },
    { "CRT_MEDAL_4_G", "modes/cart/cart_track.c", 0, &crt_medal[3][0], 1, 10000, "crt_medal[8][3]", 9 },
    { "CRT_MEDAL_4_S", "modes/cart/cart_track.c", 0, &crt_medal[3][1], 1, 10000, "crt_medal[8][3]", 10 },
    { "CRT_MEDAL_4_B", "modes/cart/cart_track.c", 0, &crt_medal[3][2], 1, 10000, "crt_medal[8][3]", 11 },
    { "CRT_MEDAL_5_G", "modes/cart/cart_track.c", 0, &crt_medal[4][0], 1, 10000, "crt_medal[8][3]", 12 },
    { "CRT_MEDAL_5_S", "modes/cart/cart_track.c", 0, &crt_medal[4][1], 1, 10000, "crt_medal[8][3]", 13 },
    { "CRT_MEDAL_5_B", "modes/cart/cart_track.c", 0, &crt_medal[4][2], 1, 10000, "crt_medal[8][3]", 14 },
    { "CRT_MEDAL_6_G", "modes/cart/cart_track.c", 0, &crt_medal[5][0], 1, 10000, "crt_medal[8][3]", 15 },
    { "CRT_MEDAL_6_S", "modes/cart/cart_track.c", 0, &crt_medal[5][1], 1, 10000, "crt_medal[8][3]", 16 },
    { "CRT_MEDAL_6_B", "modes/cart/cart_track.c", 0, &crt_medal[5][2], 1, 10000, "crt_medal[8][3]", 17 },
    { "CRT_MEDAL_7_G", "modes/cart/cart_track.c", 0, &crt_medal[6][0], 1, 10000, "crt_medal[8][3]", 18 },
    { "CRT_MEDAL_7_S", "modes/cart/cart_track.c", 0, &crt_medal[6][1], 1, 10000, "crt_medal[8][3]", 19 },
    { "CRT_MEDAL_7_B", "modes/cart/cart_track.c", 0, &crt_medal[6][2], 1, 10000, "crt_medal[8][3]", 20 },
    { "CRT_MEDAL_8_G", "modes/cart/cart_track.c", 0, &crt_medal[7][0], 1, 10000, "crt_medal[8][3]", 21 },
    { "CRT_MEDAL_8_S", "modes/cart/cart_track.c", 0, &crt_medal[7][1], 1, 10000, "crt_medal[8][3]", 22 },
    { "CRT_MEDAL_8_B", "modes/cart/cart_track.c", 0, &crt_medal[7][2], 1, 10000, "crt_medal[8][3]", 23 },
};
#define TUNE_N ((int)(sizeof(TUNE_REG)/sizeof(TUNE_REG[0])))
static float tune_def[TUNE_N];    // stock defaults, snapshotted at boot
static int tune_over[TUNE_N];     // 1 while the live value != its default
static int tune_ready = 0;

static float tune_get_i(int i){
    const TuneEntry *e = &TUNE_REG[i];
    return e->kind? *(const float*)e->ptr : (float)*(const int*)e->ptr;
}
static void tune_set_i(int i, float v){
    const TuneEntry *e = &TUNE_REG[i];
    if (e->kind) *(float*)e->ptr = v;
    else *(int*)e->ptr = (int)(v>=0? v+0.5f : v-0.5f);
}
static void tune_refresh_over(int i){ tune_over[i] = (tune_get_i(i)!=tune_def[i])? 1:0; }

int livedata_count(void){ return TUNE_N; }
const TuneEntry *livedata_entry(int i){ return &TUNE_REG[i]; }
int livedata_find(const char *name){
    for (int i=0;i<TUNE_N;i++) if (!scmp(TUNE_REG[i].name, name)) return i;
    return -1;
}
float livedata_get(int i){ return tune_get_i(i); }
float livedata_default(int i){ return tune_def[i]; }
int livedata_is_over(int i){ return tune_over[i]; }
int livedata_overrides(void){ int n=0; for (int i=0;i<TUNE_N;i++) n += tune_over[i]? 1:0; return n; }
int livedata_file_has(const char *file){
    for (int i=0;i<TUNE_N;i++) if (!scmp(TUNE_REG[i].file, file)) return 1;
    return 0;
}
int livedata_file_entries(const char *file, int *out, int max){
    int n=0;
    for (int i=0;i<TUNE_N && n<max;i++) if (!scmp(TUNE_REG[i].file, file)) out[n++]=i;
    return n;
}

// --- number formatting/parsing (core libc has no float helpers) ------
// plain decimal, up to 4 fraction digits, trailing zeros trimmed
static void tune_fmt_num(float v, char *buf){
    char *q = buf;
    if (v<0){ *q++='-'; v=-v; }
    int ip = (int)v;
    int fr = (int)((v-(float)ip)*10000.0f + 0.5f);
    if (fr>=10000){ ip++; fr=0; }
    q = sapp_int(q, ip);
    if (fr){
        char digs[4]; int div=1000;
        for (int k=0;k<4;k++){ digs[k]=(char)('0' + fr/div); fr%=div; div/=10; }
        int nd=4; while (nd>0 && digs[nd-1]=='0') nd--;
        *q++='.';
        for (int k=0;k<nd;k++) *q++=digs[k];
    }
    *q=0;
}
// the same, in source-literal form for text substitution ("1.35f")
static void tune_fmt_lit(const TuneEntry *e, float v, char *buf){
    if (e->kind==0){ char *q = sapp_int(buf, (int)(v>=0? v+0.5f : v-0.5f)); *q=0; return; }
    tune_fmt_num(v, buf);
    int n=0; while (buf[n]) n++;
    int hasdot=0; for (int k=0;k<n;k++) if (buf[k]=='.') hasdot=1;
    if (!hasdot){ buf[n++]='.'; buf[n++]='0'; }
    buf[n++]='f'; buf[n]=0;
}
// [-]d+[.d+] and nothing else (trailing blanks ok) — 1 on success
static int tune_parse_num(const char *s, float *out){
    int i=0, neg=0, digits=0; float v=0, scale=1;
    if (s[i]=='-'){ neg=1; i++; } else if (s[i]=='+') i++;
    while (s[i]>='0' && s[i]<='9'){ v=v*10.0f+(float)(s[i]-'0'); i++; digits++; }
    if (s[i]=='.'){ i++; while (s[i]>='0' && s[i]<='9'){ v=v*10.0f+(float)(s[i]-'0'); scale*=10.0f; i++; digits++; } }
    if (!digits) return 0;
    while (s[i]==' '||s[i]=='\r'||s[i]=='\t') i++;
    if (s[i]) return 0;
    *out = (neg? -v : v)/scale;
    return 1;
}

int livedata_validate(int i, float v, char *err, int errcap){
    const TuneEntry *e = &TUNE_REG[i];
    if (v>=e->vmin && v<=e->vmax) return 0;
    if (err && errcap>0){
        char lo[24], hi[24], *q = err;
        tune_fmt_num(e->vmin, lo); tune_fmt_num(e->vmax, hi);
        q = sappend(q, "OUT OF RANGE "); q = sappend(q, lo);
        q = sappend(q, ".."); q = sappend(q, hi);
        (void)q;
    }
    return -1;
}
static void livedata_persist(void){
    static char buf[8192];
    char *q = buf;
    for (int i=0;i<TUNE_N;i++) if (tune_over[i]){
        char val[28]; tune_fmt_num(tune_get_i(i), val);
        int need = slen(TUNE_REG[i].name) + slen(val) + 2;
        if ((int)(q-buf)+need > (int)sizeof(buf)-4) break;
        q = sappend(q, TUNE_REG[i].name); q = sappend(q, "="); q = sappend(q, val); q = sappend(q, "\n");
    }
    plat_mod_save("livedata.txt", buf, (int)(q-buf));
}
// validate + write one live variable (no persist — the caller persists,
// so a batch apply stays one file write)
int livedata_apply(int i, float v, char *err, int errcap){
    if (livedata_validate(i, v, err, errcap)) return -1;
    tune_set_i(i, v); tune_refresh_over(i);
    return 0;
}
void livedata_persist_now(void){ livedata_persist(); }
// REST STOCK path: every live variable back to its stock default
void livedata_reset(void){
    for (int i=0;i<TUNE_N;i++){ tune_set_i(i, tune_def[i]); tune_over[i]=0; }
    livedata_persist();
}
// boot: snapshot defaults, then reapply the persisted override set.
// NEVER fails: unknown names ignored, bad values rejected one by one,
// an oversized blob ignored wholesale.
void livedata_boot(void){
    if (tune_ready) return;
    tune_ready = 1;
    for (int i=0;i<TUNE_N;i++){ tune_def[i]=tune_get_i(i); tune_over[i]=0; }
    static char buf[8192];
    int n = plat_mod_load("livedata.txt", buf, (int)sizeof(buf)-1);
    if (n<=0) return;
    if (n>=(int)sizeof(buf)-1) return;    // oversized: ignore wholesale
    buf[n]=0;
    int pos=0;
    while (pos<n){
        int eol=pos; while (eol<n && buf[eol]!='\n') eol++;
        buf[eol]=0;
        char *line = buf+pos, *eq = 0;
        for (char *c=line; *c; c++) if (*c=='='){ eq=c; break; }
        if (eq){
            *eq=0;
            float v; int idx = livedata_find(line);
            if (idx>=0 && tune_parse_num(eq+1, &v)
                && v>=TUNE_REG[idx].vmin && v<=TUNE_REG[idx].vmax){
                tune_set_i(idx, v); tune_refresh_over(idx);
            }
        }
        pos = eol+1;
    }
}
