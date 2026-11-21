// THE SURVIVAL GAME — data/toppers.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 233-244 in the Nov 2026 fine-grained refactor; code unchanged.

// HAT TOPPERS (v1.25 feat 5): cabinet trophies worn on top of the
// carcass hat. Ids 1..8 (0 = none); the earned set is a bitmask on
// Buddy (bit id-1), derived from the same records the Trophy Case
// reads — see topper_refresh in world.c and the feat-5 spec. Pure
// vanity: no weight, not a carcass, never spendable or bankable.
enum { TOP_CROWN=1, TOP_CAP, TOP_FLAG, TOP_MEDAL, TOP_PIN, TOP_GOLD, TOP_ANVIL, TOP_HARDHAT, TOP_N=8 };
#define TOP_BIT(id) (1<<((id)-1))
#define TOP_ALL ((1<<TOP_N)-1)
#define TOP_ANVIL_KG 160.0f   // the weight where a hat's tier name becomes THE LEGACY SYSTEM
static const char *TOP_NAME[TOP_N+1] = { "NONE", "CROWN", "GENERAL CAP", "CHECKER FLAG",
                                         "STORY MEDAL", "BOWL PIN", "GOLD STAR", "ANVIL", "HARD HAT" };

