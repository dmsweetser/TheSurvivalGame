# Building The Survival Game

Everything is one C99 file — `src/survival.c` — plus a web shell
(`src/shell.html`), a macOS host shim (`src/mac_main.c`), and an
Android WebView wrapper (`android/`). `build.py` drives all of it.

## Prerequisites

- Python 3.10+
- **zig 0.14.1** — used as the cross-compiler for the WebAssembly,
  Windows, and macOS targets. build.py looks for it at
  `~/workspace/tools/zig-x86_64-linux-0.14.1/zig`; override with the
  `TSG_ZIG` environment variable.
- **Linux native build**: a system C compiler plus X11 and ALSA
  development files (`libx11-dev`, `libasound2-dev` on Debian/Ubuntu).
- **Android APK**: JDK 17 and the Android SDK (build-tools 34,
  platform android-34). `android/build_apk.sh` defaults to
  `~/workspace/tools/jdk17` and `~/workspace/tools/android-sdk`;
  override with `JAVA_HOME` / `ANDROID_HOME`.
- No Windows or Mac machine is needed: those binaries are
  cross-compiled by zig on Linux. (The Mac build is unsigned —
  first launch needs right-click → Open.)

## Build everything

```
python3 build.py
```

Outputs (written to `~/workspace/your_files/`):

- `the-survival-game.html` — the complete web game: the wasm is
  embedded in the single HTML file. This exact file is also what
  the native builds embed and serve to LAN guests, what the APK
  wraps, and what goes into the itch browser zip (as `index.html`).
- `TheSurvivalGame.exe` — Windows (icon + version info compiled
  from `src/winres/survival.rc`).
- `TheSurvivalGame-linux` — Linux (X11 + ALSA).
- `TheSurvivalGame-mac.zip` — the macOS .app bundle (zipped).
- `TheSurvivalGame.apk` — Android (also installs on Fire TV;
  signed with the bundled debug keystore).
- `survival-game-source/` — a small source bundle build.py copies
  alongside the binaries.

`build.bat` runs the same build from Windows (wasm + exe only)
if zig is on PATH there.

## Running

- Native: just launch the binary. It opens the game AND hosts the
  LAN server on port 8379 — the title screen shows the join
  address; friends on the same network open it in a browser and
  drop in as guests (1–4 players).
- Web: serve `the-survival-game.html` from any static file server
  and open it in a browser. For browser-hosted LAN play instead,
  `node server.js` runs a zero-dependency relay on port 8379.
- Android: install the APK (sideloading on Fire TV works too).

## Tests

The Playwright suites in `tests/` drive the web build in headless
Chromium and poke it through the game's `dbg_*` hooks:

```
cd tests && npm install        # playwright-core
# serve the web build on :8903 first, e.g.
python3 -m http.server 8903 --directory ~/workspace/your_files
node mallfix.js                # mall + co-op regressions
node bowltest.js               # bowling + music
node bowlpad.js                # bowling on real gamepads (both players)
node splittest.js              # split-screen behaviour
node walltest.js               # wall snapping + stacking
node revivetest.js             # co-op death + paid revive
```

The Chromium executable path in each suite points at
`~/.cache/ms-playwright/` — adjust if yours lives elsewhere.

## Layout notes

- The native builds embed the freshly built web page (build.py
  generates `src/webbuild.h` from the HTML); never hand-edit it.
- Saves, records, and settings live in the browser's localStorage
  (web/APK) or the native build's profile storage. The save format
  is versioned (TSG3; TSG1/TSG2 still load).
