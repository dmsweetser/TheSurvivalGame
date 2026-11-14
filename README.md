# THE SURVIVAL GAME

A cartoony top-down survival shooter in the spirit of COD Black Ops
Zombies arcade mode — except the zombies are **software bugs**, your
weapons are keyboards, mice, rubber ducks, lobbed project managers and
The Debugger, and every bug you squash lands on your head as a carcass.
Your hat is your wallet. It also slows you down.

## The loop

- You start at the **Epicenter**, home of the **Bug Bounty** store.
  It is *not* a safe zone — bugs wander right in. Near home they are
  few, tiny and slow, though: pocket change. The one exception: while
  **any** player is in the store, a shield seals the Epicenter and
  bugs can't cross the ring until the last shopper leaves.
- Bugs scale with distance from the Epicenter, on a stretched curve:
  you have to travel far out before they get big, mean and worth real
  kilograms of carcass — and out there there are far more of them, and
  even the little ones are fast.
- Haul your carcass hat back to the store and **pay by weight** — the
  carcasses are pulled off your head at the till — for weapons,
  Firewall shields, Hoodie armor, Espresso legs and Rollback tokens.
  Hurt? A **Hotfix** patches you up on the spot (+50 HP, 20 kg) —
  it's the only way to get health back besides dying less.
- Spend 3 carcasses to **throw down a carcass wall** bugs must chew
  through.
- The world is infinite and procedurally generated, and auto-saves as
  you go. If every player dies, the run is over.
- The constant leaderboard ranks one thing: **heaviest hat**.

## Playing

- **Native builds** — `TheSurvivalGame.exe` (Windows),
  `TheSurvivalGame-linux` (X11 + ALSA), `TheSurvivalGame-mac`
  (Apple Silicon). One file each, no dependencies. Press START and
  play. **The native builds are also the co-op hosts:** they embed a
  small web server (port 8379) that serves the web build and relays
  browser guests into the host's game. The menu shows the join
  address (`http://<this-machine>:8379`); friends on the same network
  open it in any browser and drop straight in — no installs, no
  accounts, no mode to pick. Up to 4 players total.
- `the-survival-game.html` — the whole game in one file, open it in a
  browser. Single player anywhere; when a native host serves it, it
  joins that host's game automatically.

## Controls

In the spirit of the DOS classic Cyberdogs, **you face the way you
move, and you fire the way you face**.

WASD/arrows move · Space fire · 1–5 weapons · F build wall
· E interact · Space select/buy · Shift sprint · Esc back/leave store
· **P or Esc pauses in single player** (or while hosting with no
guests connected yet).

On touch screens there is no keyboard, so: **drag to move + shoot,
tap to select + buy**. Desktop remains keyboard-only by design.

## Building from source

`survival.c` is the entire game (core + renderers + all platform
backends, including the embedded HTTP/WebSocket server in C).
`mac_main.c` is the pure-C Cocoa host for macOS (AppKit driven through
the Objective-C runtime — no SDK headers needed). `build.py` rebuilds
everything: it compiles the wasm build, embeds it into `webbuild.h`
for the native servers, then builds the exe, Linux and Mac binaries;
on Windows, `build.bat` compiles just the exe with MinGW or MSVC.
Music is a chiptune sequencer in C — WebAudio in the browser, real
MIDI (winmm) in the exe, a software synth on Linux/Mac.

## This repository

- `src/` — the game (`survival.c` is the whole game; `shell.html` is
  the web shell). `BUILDING.md` has the toolchain notes.
- `tests/` — the Playwright regression suites.
- `android/` — the Android wrapper (APK + Play AAB packaging).
- `releases/v1.4-ready/` — the current finished builds and their
  devlog (Tank Rescue rework, the WALK OF FAME, and the co-op menu
  fix); `releases/play-live-v1.3/` is the currently-live itch pack.
- `packets/` — Google Play and Steam store packets, plus the Whales
  Only Stripe product/payment-link IDs (public identifiers only).
- `HANDOVER.md` — the operator's guide: architecture, release
  checklists, channel status, house rules.

Play it in the browser: https://dmsweetser.github.io/TheSurvivalGame/
Also on itch: https://dmsweetser.itch.io/the-survival-game

The Whales Only vanity items are verified with signed codes; the
verification (public) key ships in the game, and the signing key lives
in the update server's secret store — it is not in this repository.

**SQUASH BUGS. WEAR THE EVIDENCE.**
