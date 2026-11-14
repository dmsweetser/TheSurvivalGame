> **CORRECTION (Nov 11, 2026, ~20:10 EST, proactive research):** The upload-set section at the bottom of this file as originally drafted was WRONG about the browser file. Do NOT upload `survival-game-browser-itch.zip` — that zip format was retired in the Nov 10 v1.3 publish (itch deduplicated it; the single HTML has served as the browser build since, confirmed in ~/memory/2026-11-10.md and ~/memory/2026-11-11.md). The scheduled retry cron `survival-game-v14-publish-retry` (Thu Nov 12, 9:47 AM America/New_York) has the correct set and its own inline devlog text, and governs. The corrected upload set is listed at the bottom of this file. The zip in this folder is obsolete — leave it on disk, do not upload it.

# Devlog — v1.4 Tank Rescue rework (prepared Nov 11, 2026; publish with the v1.4 upload)

Title: FOLLOW THE YELLOW ARROW

Tank Rescue had a problem: you were a tank among toys, and the road went nowhere in particular. Not any more.

The mission now runs in order, and the game tells you where the order goes:

- A big yellow objective arrow points the way, the whole way — rescue the captives, deliver 3 of them to the helipad, break the Blockade Gun, crack the fortress core, and cross the finish line. Bosses come in sequence, the way they should.
- The enemies got their teeth back. Tougher across the board — rolling in is a decision now, not a victory lap.
- Your cannon got shorter legs, on purpose. Like the NES original this one salutes, you have to roll INTO their fire to make your shots count. Standing off at safe range and plinking is over.

Closer to Jackal, harder to coast, impossible to get lost.

Also in this build: the Arcade cabinets in the Bug Mall grew a little wider, so the nameplates fit — SUPER BUDDY RUN finally gets its whole name over its door.

And one more thing, small and important: the WALK OF FAME. Hauling a monster hat through the Bug Mall used to be a slow, silent trudge. Now, when your hat is heavy enough that you're down to a crawl, the mall celebrates you — confetti, and WALK OF FAME flashing in lights over the red carpet while you creep. A big hat is an achievement. Now it has a parade.

Tested before it ships: tanktest 20/20 (full mission chain, both gates, boss sequence), arcadetest 24/24, woftest 12/12 (the parade fires only mid-crawl, only in the mall), full regression green.

SQUASH BUGS. WEAR THE EVIDENCE.

## Upload set (v1.4, CORRECTED Nov 11, 2026 ~20:10 EST — matches the scheduled retry cron and the Nov 10–11 publish record)
Stage from ~/workspace/goals/the-survival-game/files/, ~/workspace/your_files, and ~/workspace/survival-game/tests/ — NOT from files/survival-game-share/ (those copies are Nov 9, v1.3-era), and NOT the browser zip in this folder.
- the-survival-game.html — 1,456,327 bytes, Nov 14 2026 rebuild (WALK OF FAME + title-menu layout latch fix), md5 ffe6a211aaa69c078cdd1ab5f5bd3340 — the BROWSER-PLAYABLE file; re-check its "This file will be played in the browser" box after replacing (re-upload clears it)
- the-survival-game-standalone.html — 1,456,327 bytes, same md5, staged at ~/workspace/survival-game/tests/the-survival-game-standalone.html — download-only; browser-play and Hide boxes stay UNCHECKED
- TheSurvivalGame.exe — 2,635,776 bytes, Nov 14 2026 (via ~/workspace/your_files), md5 ee31a1c9776637dd822b4a02033b5457
- TheSurvivalGame-linux — 3,504,904 bytes, Nov 14 2026 (via ~/workspace/your_files), md5 602495851ab126269403e074a98c03fe
- TheSurvivalGame-mac.zip — 1,597,606 bytes, Nov 14 2026, md5 8e975ba0ca5ed2f4ffd45cf7fd1dd64b
- TheSurvivalGame.apk — 1,225,964 bytes, Nov 14 2026, md5 80a0cd87925c61ef7f559b739eacc063, versionCode 5 / versionName 1.4

## After the itch publish
- Redeploy the Cloudflare tsg-version worker (locally edited worker.js, versionCode 5 / "1.4") and verify the live endpoint https://tsg-version.dmsweetser.workers.dev/version.json actually serves 1.4 before reporting success — it still served 1.3 on Nov 11.
