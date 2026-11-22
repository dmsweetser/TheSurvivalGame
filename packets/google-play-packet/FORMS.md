# Google Play Console forms — pre-answered for The Survival Game

Prepared 2026-11-07 from the verified properties of the Android
build (AAB target API 36, game HTML byte-identical to the web
build, checked Nov 7; re-verified against the v1.14 AAB
2026-11-16 — byte-identical to the GitHub v1.14 release AAB,
versionCode 14 / versionName "1.14"). Every answer below is a fact about the build,
not a guess — re-verify only if the build changes.

## Data safety

- Data collected: **None**
- Data shared: **None**
- The app declares **zero permissions**, has no analytics, no ads
  SDK, no accounts, and works fully offline. (Verified Nov 7 from
  inside the build.)
- Data deletion / account deletion sections: not applicable — no
  accounts, no data.

## Ads declaration

- Contains ads: **No.** No ad SDKs, no network calls for ads.

## In-app purchases

- The Android build contains **no purchase links at all** — Whales
  Only buying exists only in the web build (Stripe). Codes still
  verify offline on-device. Nothing to declare for Play Billing in
  this build, and the listing copy must not direct users to buy
  outside Play (it doesn't — see LISTING.md).

## Content rating (IARC questionnaire — expected answers)

- Violence: cartoon / fantasy violence only — bugs squash into
  cartoon goo; no blood, no gore, no human targets.
- Sexual content: none. Profanity: none. Substances: none.
  Gambling: none (Bug Bowl is bowling scoring, no wagering).
- User interaction: the Android build has no chat, no public
  servers, no user-generated content sharing.
- Expected outcome: a mild cartoon-violence rating (Everyone 10+
  territory). The questionnaire itself is answered in Console at
  submission.

## Target audience — DANIEL'S DECISION (flagged, not pre-answered)

- Recommending **13+**: declaring an under-13 audience pulls the app
  into Google Play's Families policies (teacher-approved-style
  review, stricter ads/design rules) even though the game itself is
  kid-friendly. His own kids play via GitHub Pages/Fire regardless of what
  the Play listing declares. If Daniel would rather declare a
  younger audience, the forms need a second pass — don't submit on
  the 13+ answer without his word.

## Privacy policy

Play expects a privacy policy URL on the listing. `PRIVACY-POLICY.md`
in this packet is a complete, accurate policy (it can be this short
*because* the app collects nothing). It still needs a public URL —
host it as a static page (the game's GitHub Pages homepage, a
devlog page, or any free static host) at submission time and paste the URL into
Console. No account or signup is needed to prepare the text.

## Pricing — DANIEL'S DECISION (open)

Main agent already recommended **free** for the first Play release
(Nov 7 message). The AAB and forms work either way; nothing in this
packet assumes a price. Do not submit until Daniel answers
free-vs-paid — that answer was still outstanding when this packet
was prepared.
