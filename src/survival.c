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
//  UNITY BUILD (Nov 17, 2026): the game source is split into per-system
//  piece files, #included below in their original order. The translation
//  unit — and the compiled game — is unchanged; this is the same code,
//  filed by system. Edit the pieces; build survival.c as always.
//    prelude.c    orig lines     39-   544  platform interface, libc-free helpers, framebuffer renderer, bitmap font
//    buddies.c    orig lines    545-  2066  game design data, entity helpers, buddy species & powers, world props, bug stats, difficulty, combat, update logic, interlocked walls
//    world.c      orig lines   2067-  5591  the Bug Mall and Arcade rooms, cartoony world rendering, EDDIE MODE sprites, HUD / leaderboard / store UI
//    music.c      orig lines   5592-  6344  the chiptune sequencer + sfx engine and song data — plus, because it sits here in the original layout, the Ed25519 verifier for Whales Only unlock codes
//    save.c       orig lines   6345-  7458  save / load, run setup + continue, save slots, the networking packet format, store purchases, mall station panels
//    mall.c       orig lines   7459-  9762  WHALES ONLY, Bug Bowl, menus / splash / help, settings + key rebinding, the controls panel, the frame state machine, local co-op, game-over portrait
//    banzai.c     orig lines   9763- 12517  BANZAI WORLD — the hub and Entrances (bw_modes 0-2)
//    run.c        orig lines  12518- 13242  SUPER BUDDY RUN (Banzai World mode 3)
//    craft.c      orig lines  13243- 14710  BUDDY CRAFT (Banzai World mode 4)
//    tank.c       orig lines  14711- 15443  TANK RESCUE (Banzai World mode 5) + the mission chain
//    castle.c     orig lines  15444- 15825  CASTLE (bw_mode 6)
//    cart.c       orig lines  15826- 16976  SUPER BUDDY CART (bw_mode 7)
//    war.c        orig lines  16977- 19564  WAR! (bw_mode 8), the tap-first RTS
//    platform.c   orig lines  19565- 21488  PLATFORM hosts — WebAssembly, Windows, POSIX shared, Linux, macOS interface, and the embedded web server + game networking
// ===========================================================================
// [/UNITY-SPLIT MAP]
#include "prelude.c"
#include "buddies.c"
#include "world.c"
#include "music.c"
#include "save.c"
#include "mall.c"
#include "banzai.c"
#include "run.c"
#include "craft.c"
#include "tank.c"
#include "castle.c"
#include "cart.c"
#include "war.c"
#include "platform.c"
