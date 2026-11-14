@echo off
REM ============================================================
REM  THE SURVIVAL GAME - upload a build to Steam (SteamPipe)
REM
REM  Before running:
REM   1. Download the Steamworks SDK from Steamworks (Tools section)
REM      and copy this whole steamcmd folder next to steamcmd.exe,
REM      i.e.  sdk\tools\ContentBuilder\
REM             content\      <- game files (already has the .exe)
REM             scripts\      <- put the two .vdf files here if you
REM                              prefer the SDK's default layout
REM   2. In app_build_TEMPLATE.vdf replace YOUR_APP_ID and
REM      YOUR_DEPOT_ID (the depot ID is normally your AppID + 1;
REM      both are shown on your app's Steamworks page).
REM   3. Double-click this file. You will be asked for YOUR Steam
REM      password and a Steam Guard code - they go to Valve's
REM      steamcmd only, never anywhere else.
REM ============================================================
set /p STEAMUSER=Your Steam username:
steamcmd.exe +login %STEAMUSER% +run_app_build app_build_TEMPLATE.vdf +quit
echo.
echo If the log above ends with "Success." your build is uploaded.
echo Set it live on the default branch in Steamworks when ready.
pause
