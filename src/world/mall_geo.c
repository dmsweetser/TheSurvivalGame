// THE SURVIVAL GAME — world/mall_geo.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 1-34 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// THE BUG MALL — the old single-panel Bug Bounty grew into a walk-in
// mall (Daniel's design): a shop counter at the entrance with everything
// the Bounty always sold, then the Gallery (Psychic Hat Stand), the Bug
// Bank, and the Banzai Buddies store. The interior lives far south of
// the world at (0, MALL_CY); walking in is a step through the hut door.
// No bugs inside. Ever. House rules.
// ---------------------------------------------------------------------------
#define MALL_CY (-6000)
static const int MALL_SX[4] = { -315, -105, 105, 315 };   // station counter centres
static const char *MALL_STATION[10] = { "THE COUNTER", "WHALES ONLY", "BUG BANK", "BANZAI BUDDIES", "BUG BOWL", "TROPHY CASE", "CHALLENGE BOARD", "BOUNTY BOARD", "PHOTO BOOTH", "THE SOURCE" };   // station 9 (v1.26): the Source terminal, inside the Source Room (world/srcroom_geo.c), not in the Mall proper
#define BOWL_SX (-180)                                   // the lanes counter, south-west
#define CASE_SX 340                                       // TROPHY CASE (v1.25 feat 4): the glass case at the south wall's east end, past the Banzai gate — clear of the entry-mat <-> gate corridor
#define BOARD_SX (-330)                                  // CHALLENGE BOARD (v1.25 feat 6): the south wall's west end, the case's mirror bookend — west of the bowl there is no door, mat or through-route to block
#define BOUNTY_WX (-366)                                 // BOUNTY BOARD (v1.25 feat 8): a WANTED board flush on the WEST wall (the south and north walls are full; the east carries the Arcade) — no floor footprint, no collision box, nothing in any corridor
#define BOUNTY_WY (MALL_CY+8)
/* v1.25 MALL ENLARGEMENT (Daniel, Nov 19): the interior grew +100 a side and
 * +40 north/south, symmetric about (0, MALL_CY), so the room centre — and the
 * save/load resume point — never moved. Station 0 is THE COUNTER now (the
 * static-text audit's naming collision: the elite BOUNTY BOARD is the only
 * live "Bounty" left). The Hat Portrait Booth left the Banzai Buddies panel
 * for its own station (index 8): a freestanding island fixture west-centre.
 * BOOTH_SX/BOOTH_CY are the body centre; the interact disc sits 20 px south
 * of the body face, like the other fixtures. Disc separations were checked
 * against every other disc — no overlaps. */
#define BOOTH_SX (-232)
#define BOOTH_CY (MALL_CY - 30)
#define BOOTH_DY (MALL_CY + 20)
