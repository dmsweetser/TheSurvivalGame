# Closed test + launch sequence — Google Play

Prepared 2026-11-07, after the account check settled the big
unknown: Daniel's Google account (dmsweetser@gmail.com) previously
had a Play developer account named "Morgan Road," but Google
**closed it on Sep 13, 2026 for inactivity** (warning sent Jul 7,
deadline Sep 5; the $25 fee was non-refundable). So there is no
pre-Nov-2023 account to inherit — a **fresh personal account** is
required, and the new-account testing rule applies in full.

## The rule (verified against Play Console Help summaries, Nov 7)

A personal developer account created after Nov 13, 2023 cannot go
to production until it has run a **closed test with at least 12
testers opted in continuously for the preceding 14 days**, and then
**applied** for production access. Meeting the test makes him
eligible to apply — it does not auto-approve; Google reviews the
application (typically ~7 days) and can ask for more testing.

Two mechanics that decide the timeline:

1. **The 14-day clock starts when the 12th tester opts in** — not
   when the track is created or the first tester joins. A tester
   who opts out and back in restarts their own continuity.
2. Testers must actually **install the app on a real Android
   device** from the closed track; merely being on the email list
   is not opted in.

Practical (not official) guidance from tester-community sources:
recruit **14–15 testers as buffer**, so one dropout doesn't break
the run of 12. Family, friends and church folks with Android
phones cover this — Daniel's own household plus a handful of
Grace Church / co-op contacts.

## Sequence

### Daniel-only steps (cannot be delegated)
1. Create the new Play developer account: **$25 one-time** +
   identity verification with his ID.
2. Verify access to an Android device in the Play Console app
   (device verification step for new personal accounts).
3. Answer free-vs-paid (open — see FORMS.md).
4. Recruit the testers (names/Gmail addresses go in TESTERS.md).

### Prepared / delegable steps
5. Generate a **dedicated Play upload keystore** — the current AAB
   (1.12 MB, API 36, bundletool-validated Nov 7) is debug-key
   signed and cannot be the submission signing key.
6. **Pre-launch code fixes** (identified Nov 7, do before the
   closed test starts so testers test the real thing):
   - Predictive Back on Android 16: legacy onBackPressed won't
     fire at API 36 — Back would exit instead of pausing.
   - Edge-to-edge layout check at the new target API.
   Both need a pass on a real Android 15/16 device — Daniel's
   A17 (Android) can serve for the device test.
7. Create the app in Console, paste LISTING.md copy + assets,
   answer forms from FORMS.md, host PRIVACY-POLICY.md and paste
   its URL.
8. Phone screenshots: DONE for the current build — all seven
   captures in `assets/` are current as of the v1.5 build (v1.6 changed the Arcade interior and added the kart, and v1.7 rebuilt the kart view and Tank Rescue — neither is depicted in a shot; the title and mall shots are unaffected; v1.8/v1.9 only renumbered the v1.7 build, changing no scene; v1.10 changed the kart race's controls and visuals — still not depicted in any shot; v1.11 changed the Run's level themes, the kart format and CASTLE's exit rule — none depicted in a shot either; v1.12 changed the Run's speed and touch layout, the kart grandstand's size and placement, and the wall health bookkeeping — also none depicted in a shot; v1.13 expanded the Arcade room for a seventh cabinet, WAR!, and added its battles — the Arcade interior is not depicted in a shot either; v1.14 changed WAR!'s battles — property-line building, terrain, connected firewalls, vehicles, fog of war, grab-the-world panning — none depicted in a shot)
   (title shot recaptured 2026-11-14; see LISTING.md). Only
   re-check this step if a later build visibly changes a
   depicted scene.
9. Upload the AAB to the **closed testing** track, add tester
   emails, send opt-in links.
10. **Day 0 = the day the 12th tester opts in.** Track opt-ins
    daily in TESTERS.md; collect real feedback (the production
    application asks what feedback was received and what changed —
    the Back-button fix and tester playtest notes are exactly the
    material for those answers).
11. Day 14+: apply for production access from the Console
    dashboard; expect ~7 days of review. Realistic public launch:
    **~3 weeks after the tester roster is full**, consistent with
    the estimate Daniel was given on Nov 7.

## What would change this plan

- If Daniel instead registers as an **organization** account, the
  12-tester rule doesn't apply — but that needs a D-U-N-S number
  and an organization identity he doesn't currently publish under;
  not recommended, noted only for completeness.
- The trashed Amazon / Game Jolt packets stay trashed (Daniel's
  Nov 7 order). The Steam kit was NOT named in that order and is
  untouched, awaiting his separate word.
