@echo off
rem Build THE SURVIVAL GAME as a single Windows executable.
rem Works with MinGW gcc (in PATH) — no other dependencies.
gcc -O2 -std=c99 -DPLAT_WIN32 -o TheSurvivalGame.exe survival.c -lgdi32 -lwinmm -lws2_32 -mwindows
if %ERRORLEVEL% NEQ 0 (
  echo gcc failed — trying MSVC...
  cl /O2 /DPLAT_WIN32 survival.c /Fe:TheSurvivalGame.exe gdi32.lib winmm.lib ws2_32.lib user32.lib
)
echo Done: TheSurvivalGame.exe
