// THE SURVIVAL GAME — render/render_core.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 1542-1551 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// Rendering — the cartoony world
// ---------------------------------------------------------------------------
static int SX(float wx){ float z = G.zoom>0.01f? G.zoom : 1.0f; int vw = G.view_w>0? G.view_w : VW; return (int)((wx - G.camx)*z + G.view_x0 + vw/2); }
static int SY(float wy){ float z = G.zoom>0.01f? G.zoom : 1.0f; int vh = G.view_h>0? G.view_h : VH; return (int)((wy - G.camy)*z + G.view_y0 + vh/2); }

// ---------------------------------------------------------------------------
// EDDIE MODE sprites: every bug is a fuzzy orange puffball — round fur,
// cream belly, two shuffling feet and one very big shiny red nose. An
// original generic puff-critter design (Daniel's mode, Daniel's name).
