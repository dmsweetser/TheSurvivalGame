// ============================================================================
//  THE SURVIVAL GAME — a top-down bug-squashing survival shooter.
//  Written in C99. One game source file. Build targets:
//
//    1) Windows (single .exe, Win32 only — no external libraries):
//         gcc -O2 -std=c99 -DPLAT_WIN32 -o TheSurvivalGame.exe survival.c ^
//             -lgdi32 -lwinmm -lws2_32 -mwindows
//       (MinGW; MSVC works too: cl /DPLAT_WIN32 survival.c gdi32.lib
//        winmm.lib ws2_32.lib user32.lib)
//
//    2) WebAssembly (playable in a browser, single HTML file):
//         clang --target=wasm32 -nostdlib -O2 -DPLAT_WASM ... (see build.py)
//       The JS shell provides canvas blit, input, WebAudio, localStorage.
//       (The web menu is just START — LAN co-op lives in the native
//       builds, which talk UDP directly.)
//
//    3) Linux (X11 window + ALSA software synth + BSD UDP):
//         gcc -O2 -std=c99 -DPLAT_LINUX -o TheSurvivalGame-linux survival.c \
//             -lX11 -lasound -lm -lpthread
//
//    4) macOS, Apple Silicon (pure-C Cocoa host in mac_main.c — no SDK
//       headers needed; AppKit is driven through the Objective-C runtime
//       and every framework symbol resolves at runtime):
//         clang -DPLAT_MAC -o TheSurvivalGame-mac survival.c mac_main.c \
//             -Wl,-undefined,dynamic_lookup -lm
//       (build.py cross-compiles this from any host with zig cc.)
//
//  PREMISE: You are a developer. The codebase is infested. Bugs pour in from
//  the Unmaintained Depths — the further from the Epicenter, the bigger,
//  meaner and more valuable they get. Every bug you squash lands on your
//  head as a carcass. Your hat IS your wallet: haul it back to the Bug
//  Bounty store at the Epicenter and pay by weight — carcasses literally
//  pulled off your head at the till. Spend weight on better weapons
//  (Keyboards, thrown Mice, lobbed Project Managers, Rubber Ducks, and the
//  dreaded Debugger), Firewall shields, Hoodie armor and Espresso. Or spend
//  carcasses to build carcass walls that bugs must chew through.
//  Survive. The leaderboard only cares about one thing: heaviest hat wins.
// ============================================================================
// [UNITY-SPLIT MAP]
// ===========================================================================
//  UNITY BUILD: survival.c #includes every piece below, in this exact
//  order, forming one translation unit. Pieces live in category folders
//  under src/ (core, data, input, player, entities, buddies, enemies,
//  world, sim, combat, game, music, crypto, save, net, mall, menus,
//  coop, ui, render, banzai, modes/<game>, war, platform). Edit the
//  pieces; build survival.c as always. Order is load-bearing: do not
//  rearrange includes without rebuilding and re-verifying.
// ===========================================================================
// [/UNITY-SPLIT MAP]
// --- core/ ---
#include "core/platform_iface.c"
#include "core/libc.c"
#include "core/math.c"
#include "core/rng.c"
#include "core/render.c"
#include "core/color.c"
#include "core/font.c"
// --- data/ ---
#include "data/design.c"
#include "data/medals.c"
#include "data/toppers.c"
// --- mall/ ---
#include "mall/bowl_state.c"
// --- input/ ---
#include "input/bindings.c"
#include "input/hints.c"
#include "input/touch.c"
// --- data/ ---
#include "data/palette.c"
// --- world/ ---
#include "world/regions.c"
// --- data/ ---
#include "data/hat_tiers.c"
// --- player/ ---
#include "player/speed.c"
// --- entities/ ---
#include "entities/fx.c"
// --- buddies/ ---
#include "buddies/species.c"
#include "buddies/draft.c"
// --- enemies/ ---
#include "enemies/elites.c"
// --- buddies/ ---
#include "buddies/adopt.c"
#include "buddies/items.c"
// --- world/ ---
#include "world/props.c"
// --- enemies/ ---
#include "enemies/bugs.c"
// --- player/ ---
#include "player/revive.c"
// --- game/ ---
#include "game/difficulty.c"
// --- game/ ---
#include "game/src_blob.c"
#include "game/srcedit.c"
#include "data/livedata.c"
// --- combat/ ---
#include "combat/combat.c"
// --- game/ ---
#include "game/update.c"
// --- combat/ ---
#include "combat/walls.c"
// --- world/ ---
#include "world/mall_geo.c"
#include "world/srcroom_geo.c"
#include "world/sky.c"
#include "world/arcade_geo.c"
// --- combat/ ---
#include "combat/fire.c"
// --- sim/ ---
#include "sim/players.c"
#include "sim/projectiles.c"
#include "sim/bugs.c"
#include "sim/fx.c"
#include "sim/buddies_ai.c"
#include "sim/sim.c"
// --- render/ ---
#include "render/render_core.c"
#include "render/eddie.c"
// --- enemies/ ---
#include "enemies/draw_bugs.c"
// --- render/ ---
#include "render/hats.c"
#include "render/buddies_draw.c"
#include "render/players_draw.c"
#include "render/props_draw.c"
#include "render/mall_draw.c"
#include "render/arcade_draw.c"
#include "render/srcroom_draw.c"
#include "render/render_world.c"
// --- ui/ ---
#include "ui/hud.c"
#include "ui/first_shift.c"
// --- music/ ---
#include "music/songs.c"
#include "music/pieces.c"
#include "music/adaptive.c"
#include "music/engine.c"
// --- crypto/ ---
#include "crypto/sha512.c"
#include "crypto/ed25519.c"
// --- save/ ---
#include "save/saveio.c"
#include "save/run_setup.c"
// --- game/ ---
#include "game/first_shift.c"
#include "game/weekly.c"
// --- save/ ---
#include "save/bounty.c"
#include "save/start.c"
#include "save/slots.c"
// --- net/ ---
#include "net/packets.c"
#include "net/host.c"
#include "net/client.c"
// --- mall/ ---
#include "mall/store.c"
#include "mall/whales.c"
#include "mall/trophy_case.c"
#include "mall/stations.c"
#include "mall/station_input.c"
#include "mall/bowl.c"
#include "mall/bowl_draw.c"
#include "mall/panel.c"
#include "mall/booth.c"
#include "mall/panels.c"
#include "mall/srcterm.c"
// --- menus/ ---
#include "menus/menu.c"
#include "menus/menu_render.c"
#include "menus/help.c"
#include "menus/settings.c"
#include "menus/pause.c"
// --- coop/ ---
#include "coop/input.c"
// --- mall/ ---
#include "mall/frame.c"
// --- banzai/ ---
#include "banzai/tiles.c"
#include "banzai/story.c"
#include "banzai/battle.c"
#include "banzai/gates.c"
#include "banzai/cabinet.c"
#include "banzai/frame.c"
#include "banzai/input.c"
#include "banzai/entrances.c"
#include "banzai/entrances_draw.c"
#include "banzai/draw.c"
#include "banzai/minimap.c"
// --- modes/run/ ---
#include "modes/run/run_levels.c"
#include "modes/run/run.c"
#include "modes/run/run_render.c"
// --- modes/craft/ ---
#include "modes/craft/craft_world.c"
#include "modes/craft/craft.c"
#include "modes/craft/craft_update.c"
#include "modes/craft/craft_render.c"
// --- modes/tank/ ---
#include "modes/tank/tank.c"
#include "modes/tank/tank_render.c"
// --- modes/castle/ ---
#include "modes/castle/castle.c"
#include "modes/castle/castle_play.c"
#include "modes/castle/castle_render.c"
// --- modes/cart/ ---
#include "modes/cart/cart_track.c"
#include "modes/cart/cart_race.c"
#include "modes/cart/cart_render.c"
// --- modes/mordor/ ---
#include "modes/mordor/mordor.c"
#include "modes/mordor/mordor_render.c"
#include "modes/scorch/scorch.c"
#include "modes/scorch/scorch_render.c"
// --- war/ ---
#include "war/units.c"
#include "war/coop.c"
#include "war/fog.c"
#include "war/terrain.c"
#include "war/hive.c"
#include "war/path.c"
#include "war/snap.c"
#include "war/pause.c"
#include "war/enter.c"
#include "war/build.c"
#include "war/orders.c"
#include "war/map.c"
#include "war/pad.c"
#include "war/update.c"
#include "war/render.c"
#include "war/modes_render.c"
#include "war/touch.c"
// --- platform/ ---
#include "platform/wasm.c"
#include "platform/native_common.c"
#include "platform/win32.c"
#include "platform/posix.c"
#include "platform/linux.c"
#include "platform/mac.c"
#include "platform/srcbuild.c"
#include "platform/server.c"
