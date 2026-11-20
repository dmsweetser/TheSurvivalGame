// story2test.js — the Nov 4 Story Mode expansion: hill ridges block,
// hackers chase on sight/proximity in Story (and only wander in Free
// Play), fewer tents in Story, help signs + the sign errand, Bitling,
// the lake commission, and the parcel run.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 140) : ''); }
}
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 120, wait = 0) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  async function poll(fn, want, timeout = 9000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(200); }
    return call(fn);
  }
  const q = async n => n < 6 ? (((await call('dbg_quest')) >> (n < 4 ? n * 2 : 18 + (n - 4) * 2)) & 3)
                             : (((await call('dbg_sq2v')) >> ((n - 6) * 2)) & 3);
  const sq2 = async () => call('dbg_sq2v');
  const frame = async () => page.evaluate(() => new Promise(r => requestAnimationFrame(() => requestAnimationFrame(r))));
  // Story hackers hunt now: an E press can be eaten by a chase battle.
  // Win any fight that interrupts, then press again, until cond() holds.
  async function winBattle() {
    for (let i = 0; i < 80 && (await call('dbg_bwbattle')) > 0; i++) {
      const ph = await call('dbg_bwbattle');
      if (ph === 1) { await call('dbg_bwchoose', 0); await sleep(300); }
      else await sleep(300);
    }
  }
  async function interactUntil(cond, tries = 6) {
    for (let i = 0; i < tries; i++) {
      if (await cond()) return true;
      if ((await call('dbg_bwbattle')) > 0) { await winBattle(); continue; }
      await key('KeyE', 200); await sleep(350);
      if ((await call('dbg_bwbattle')) > 0) await winBattle();
    }
    return cond();
  }

  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_budup', 40);

  // ---- ridges are pure geometry: solid on the wall, open at the pass ----
  T('ridge wall is solid (farm road, mid-wall)', (await call('dbg_ridge', 1700, -1200)) === 1);
  T('ridge pass is open (farm road pass)', (await call('dbg_ridge', 1700, -1900)) === 0);
  T('ridge wall is solid (lake road)', (await call('dbg_ridge', -200, 2200)) === 1);
  T('ridge pass is open (lake road pass)', (await call('dbg_ridge', -1700, 2200)) === 0);
  T('open meadow is not a ridge', (await call('dbg_ridge', 0, 900)) === 0);

  // ---- enter Story Mode through the real gate -----------------------------
  await call('dbg_tp', 0, 80);
  await key('KeyE', 200); await sleep(400);
  await call('dbg_tp', 215, -5868);
  await sleep(250);
  await key('KeyE', 200);
  T('gate panel opens', (await poll('dbg_gate', v => v === 1)) === 1);
  await key('Space', 250);
  T('story mode entered', (await poll('dbg_bwstate', v => v === 1)) === 1);

  // fewer tents in Story Mode (deterministic count over cells -10..10)
  const tentsStory = await call('dbg_tentcount');
  T('story-mode tent count is low (< 24 of 441 cells, ~3%)', tentsStory > 0 && tentsStory < 24, tentsStory);

  // walking east from the gate, the farm ridge stops you cold
  await call('dbg_bwtp', 1500, -1200);
  await frame();
  const sx0 = await call('dbg_bwx');
  for (let burst = 0; burst < 8; burst++) {
    if ((await call('dbg_bwbattle')) > 0) { await winBattle(); continue; }
    const before = await call('dbg_bwx');
    await page.keyboard.down('ArrowRight');
    await sleep(400);
    await page.keyboard.up('ArrowRight');
    if ((await call('dbg_bwx')) <= before + 2 && burst >= 2) break;   // up against the ridge
  }
  const sx1 = await call('dbg_bwx');
  T('the ridge blocks the straight walk east', sx1 > sx0 && sx1 < 1690, sx0 + '->' + sx1);
  // ...but the pass lets you through (start inside the pass mouth:
  // the meadow can grow a tree on a teleport spot, and a hunter can
  // end the walk in a faint — recover and retry the crossing once)
  async function crossPass() {
    if ((await call('dbg_bwstate')) !== 1) { await call('dbg_bwstory'); await sleep(400); }
    if ((await call('dbg_bwbattle')) > 0) await winBattle();
    await call('dbg_bwtp', 1620, -1900);
    await frame();
    for (let burst = 0; burst < 6 && (await call('dbg_bwx')) <= 1750; burst++) {
      if ((await call('dbg_bwstate')) !== 1) break;
      if ((await call('dbg_bwbattle')) > 0) { await winBattle(); continue; }
      await page.keyboard.down('ArrowRight');
      await sleep(550);
      await page.keyboard.up('ArrowRight');
    }
    return call('dbg_bwx');
  }
  let px1 = await crossPass();
  if (px1 <= 1750) px1 = await crossPass();
  T('the pass lets you through the ridge', px1 > 1750, px1);

  // ---- chase: stand in a hacker's face and it hunts ------------------------
  await call('dbg_bwtele_hack');
  await sleep(300);
  const d0 = await call('dbg_roamdist');
  const alerted = await poll('dbg_alertmax', v => v > 40, 3000);
  T('a nearby hacker spots you and hunts (alert rises)', alerted > 40, 'd0=' + d0 + ' alert=' + alerted);
  // it closes the distance even while you stand still
  const d1 = await call('dbg_roamdist');
  await sleep(700);
  const d2 = await call('dbg_roamdist');
  T('the hunter closes in', d2 < d1 || (await call('dbg_bwbattle')) > 0, d1 + '->' + d2);
  await winBattle();
  await call('dbg_bwexit');
  T('left Banzai World after the chase', (await poll('dbg_bwstate', v => v === 0)) === 0);

  // ---- Free Play: no hunting, and tents are plentiful again ----------------
  await call('dbg_bwenter');
  T('free play entered', (await poll('dbg_bwstate', v => v === 1)) === 1);
  const tentsFree = await call('dbg_tentcount');
  T('free-play tent count exceeds story count', tentsFree > tentsStory, tentsFree + ' vs ' + tentsStory);
  await call('dbg_bwtele_hack');
  await sleep(1500);
  T('free-play hackers never hunt (alert stays 0)', (await call('dbg_alertmax')) === 0, await call('dbg_alertmax'));
  await call('dbg_bwexit');
  await sleep(300);

  // ---- the errands: Patchina, signs, Bitling, Mop, Glitch -------------------
  await call('dbg_bwstory');
  T('story mode re-entered', (await poll('dbg_bwstate', v => v === 1)) === 1);
  await call('dbg_bwtele_npc', 0);
  await sleep(250);
  T('meeting Patchina completes quest 0', await interactUntil(async () => (await q(0)) === 2), await call('dbg_quest'));
  T('the sign errand (quest 9) is offered', (await q(9)) === 1, await sq2());

  // read any three signs
  for (const si of [5, 0, 2]) {
    await call('dbg_hsigntp', si);
    await sleep(250);
    await interactUntil(async () => (((await sq2()) >> (11 + si)) & 1) === 1);
  }
  T('three signs read completes quest 9', (await q(9)) === 2, await sq2());
  T('sign read bits are recorded', (((await sq2()) >> 11) & 0x3f) !== 0, await sq2());

  // Bitling: Patchina offers the stray after the farm; simulate by talking
  // twice more later — first set the farm done via real battles is long;
  // instead verify Bitling exists, is findable, and chirps without the quest
  await call('dbg_bittp');
  await sleep(200);
  T('Bitling is right where the teleport put us', (await call('dbg_bitdist')) < 30, await call('dbg_bitdist'));
  await key('KeyE', 200); await sleep(300);
  T('Bitling chirps but quest 8 stays unoffered', (await q(8)) === 0, await sq2());

  // Mop: tools quest first (existing flow), via teleports to the real tools
  await call('dbg_bwtele_npc', 1);
  await sleep(250);
  T("Mop's tools quest offered", await interactUntil(async () => (await q(4)) === 1), await call('dbg_quest'));
  for (let ti = 0; ti < 3; ti++) {
    await call('dbg_bwtp', [1000, -3600, 2000][ti], [800, 400, 3000][ti]);
    await sleep(250);
    const got = await interactUntil(async () => (((await call('dbg_quest')) >> (24 + ti)) & 1) === 1);
    T('tool ' + ti + ' picked up', got, await call('dbg_quest'));
  }
  await call('dbg_bwtele_npc', 1);
  await sleep(250);
  T('tools returned complete quest 4', await interactUntil(async () => (await q(4)) === 2), await call('dbg_quest'));
  T('Mop offers the parcel run (quest 7)', await interactUntil(async () => (await q(7)) === 1), await sq2());
  T('the parcel is carried', (((await sq2()) >> 10) & 1) === 1, await sq2());

  // Glitch: house call completes, parcel delivers, lake commission offered
  await call('dbg_bwtele_npc', 2);
  await sleep(250);
  T('house call completes (quest 2)', await interactUntil(async () => (await q(2)) === 2), await call('dbg_quest'));
  T('the parcel is delivered (quest 7 done)', await interactUntil(async () => (await q(7)) === 2), await sq2());
  T('Glitch offers the lake commission (quest 6)', await interactUntil(async () => (await q(6)) === 1), await sq2());

  // two lake battles through the real fight path
  for (let round = 0; round < 2; round++) {
    await call('dbg_budup', 40);   // tent-top-up between fights: foes scale with capability now (Nov 5)
    await call('dbg_bwtele_site', 2);
    await sleep(250);
    await call('dbg_bwhackhere');
    const inB = await poll('dbg_bwbattle', v => v > 0, 9000);
    T('lake battle ' + (round + 1) + ' starts', inB > 0, inB);
    await winBattle();
    T('lake battle ' + (round + 1) + ' won', (await call('dbg_bwbattle')) === 0);
  }
  T('two lake kills recorded', (((await sq2()) >> 8) & 3) === 2, await sq2());
  await call('dbg_bwtele_npc', 2);
  await sleep(250);
  T('reporting to Glitch completes quest 6', await interactUntil(async () => (await q(6)) === 2), await sq2());

  console.log(`\n${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('SUITE ERROR', e); process.exit(2); });
