// THE SURVIVAL GAME — buddies/draft.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 915-973 in the Nov 2026 fine-grained refactor; code unchanged.

// adoption draft state: species -> name -> color, then the till runs
static int bud_stage[MAXP];        // 0 panel, 1 name (adopt), 2 color (adopt), 3 name (rename), 4 color (recolor)
static int booth_stage[MAXP];      // PHOTO BOOTH (feat 9; its own station since the v1.25 MALL ENLARGEMENT): 0 the booth's rows, 1 the gallery
// HAT PORTRAIT BOOTH (v1.25 feature 9): the frozen pose, per player.
// A portrait is a PHOTO, not a mirror: at snap time the pose is
// copied here whole — backdrop, whether the stack was worn, the
// stack itself (one drawn-kind digit per carcass: draw_hat reads
// the stack only as (int)stack[i] % 4 plus the count, so a digit
// per carcass re-renders pixel-identically), the lettered weight,
// the topper, and the crew as they looked that day (buddy species /
// coat / name, the DEV's colour and whale dressing). It rides the
// pl-line as the pt= token (save.c). port_bd is the booth's
// backdrop dial (persisted with the token); port_hat is the pose
// dial — wear the stack for the photo, or bare — runtime only.
typedef struct {
    int has, backdrop, hat_on, top_worn, kg, stack_n;
    int species, bud_color, name_idx, pcolor, whale_on;
    unsigned char kinds[MAXSTACK];
} Portrait;
static Portrait portrait[MAXP];
static int port_bd[MAXP];
static int port_hat[MAXP];
#define PORT_BD_N 6
static const char *PORT_BD_NAME[PORT_BD_N] = { "PLAZA DAWN", "FROZEN CACHE", "KERNEL BADLANDS",
                                               "CABLE JUNGLE", "THE BUG MALL", "WALK OF FAME" };
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
// v1.19 (Daniel): the buddy earns XP from SUCCESSES inside the
// Arcade cabinets too — a Run finish, a Cart placement, a Castle
// escape, a Tank mission, a finished Craft, an Entrances run, a
// WAR! victory. One shared door so every cabinet pays through the
// same rule: no buddy adopted, no award (silently). Callers hook
// their mode's one-shot completion event and surface the amount
// in the mode's own note/banner idiom.
static void buddy_award_xp(int pi, int amt){
    if (amt > 0 && G.bud[pi].active) G.bud[pi].xp += amt;
}
// FIRST SHIFT (v1.25) shared data — the objective table, the XP
// table, and the step-done banner state. Defined here, with the
// design data, because the chain core (save.c), the HUD prompt
// (world.c) and the pause menu (mall.c) all read them. The chain
// itself lives in save.c; see v125-spec.md.
static const char *FS_TEXT[6] = {
    "SQUASH YOUR FIRST BUG",
    "WEAR A 10 KG HAT",
    "BUILD A CARCASS WALL",
    "BUY SOMETHING AT THE COUNTER",
    "BANK CARCASSES AT THE BUG BANK",
    "STEP INTO BANZAI WORLD, OR ANY CABINET"
};
static const int FS_XP[6] = { 5, 5, 10, 10, 15, 20 };
