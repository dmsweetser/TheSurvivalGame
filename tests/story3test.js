// story3test.js — THE REBUILDING (v1.25 feat 14): the Banzai World
// post-game chapter. Patchina's door stays shut until the story is
// 10/10; then her new list runs — the Cider Run (a race to the Frozen
// Lake), the Long Walk (Duster escorted to the Server Farm), Raid,
// Again (the BROKEN RAID rematch), and Patch Day (the festival).
// The completed story is staged with dbg_setq — bosstest's own
// staging door — and everything the CHAPTER does is earned through
// its real doors: talks at the real NPCs, the buddy's own feet, and
// a real boss fight. No export sets a chapter quest state.
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
  async function poll(fn, want, timeout = 9000, step = 200) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(step); }
    return call(fn);
  }
  async function winBattle() {
    for (let i = 0; i < 120 && (await call('dbg_bwbattle')) > 0; i++) {
      const ph = await call('dbg_bwbattle');
      if (ph === 1) { await call('dbg_bwchoose', 0); await sleep(250); }
      else await sleep(250);
    }
  }
  // story hackers hunt: an E press can be eaten by a chase battle.
  // Win any fight that interrupts, then press again (story2test's tool).
  async function interactUntil(cond, tries = 6) {
    for (let i = 0; i < tries; i++) {
      if (await cond()) return true;
      if ((await call('dbg_bwbattle')) > 0) { await winBattle(); continue; }
      await key('KeyE', 200); await sleep(350);
      if ((await call('dbg_bwbattle')) > 0) await winBattle();
    }
    return cond();
  }
  // a poll that wins any battle which interrupts the waiting
  async function pollCh2(fn, want, timeout = 30000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      if ((await call('dbg_bwbattle')) > 0) { await winBattle(); continue; }
      const v = await call(fn);
      if (want(v)) return v;
      await sleep(250);
    }
    return call(fn);
  }
  const ch2 = async () => call('dbg_ch2q');
  const cq = async n => { const v = await ch2(); return (v >> ((n - 10) * 2)) & 3; };

  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_budup', 60);   // a veteran buddy: the rematch is a boss fight

  // ---- into Story Mode through the real gate (storytest's road) ----
  await call('dbg_tp', 0, 80);
  await key('KeyE', 200); await sleep(400);
  await call('dbg_tp', 215, -5868);
  await sleep(250);
  await key('KeyE', 200);
  T('gate panel opens', (await poll('dbg_gate', v => v === 1)) === 1);
  await key('Space', 250);
  T('story mode entered', (await poll('dbg_bwstate', v => v === 1)) === 1);

  // ---- locked at zero: a fresh story hears nothing of a chapter ----
  await call('dbg_bwtele_npc', 0);
  await sleep(250);
  T('meeting Patchina completes quest 0', await interactUntil(async () => (((await call('dbg_quest')) >> 0) & 3) === 2), await call('dbg_quest'));
  T('story stands at 1/10', (await call('dbg_story10')) === 1, await call('dbg_story10'));
  T('the chapter is not a rumour yet', (await ch2()) === 0, await ch2());

  // ---- locked at nine: stage every quest but Zed's own, then knock ----
  for (const q of [1, 2, 4, 5, 6, 7, 8, 9]) await call('dbg_setq', q, 2);
  T('story staged at 9/10', (await call('dbg_story10')) === 9, await call('dbg_story10'));
  await call('dbg_bwtele_npc', 0);
  await sleep(250);
  const offeredEarly = await interactUntil(async () => (await ch2()) !== 0, 4);
  T('Patchina offers no chapter at 9/10', !offeredEarly && (await ch2()) === 0, await ch2());
  T('still short of ten after the knock', (await call('dbg_story10')) < 10, await call('dbg_story10'));

  // ---- the story completes; snapshot the shipped story's footprint ----
  await call('dbg_setq', 3, 2);
  await call('dbg_setq', 5, 2);   // her 9/10 arm may have re-opened the crank errand; close it again
  T('story complete at 10/10', (await call('dbg_story10')) === 10, await call('dbg_story10'));
  const bitsBefore = await call('dbg_quest');
  const sq2LowBefore = (await call('dbg_sq2v')) & 0x1FFFF;   // bits 0-16: the shipped story's whole sq2 footprint

  // ---- quest 10: THE CIDER RUN ----
  await call('dbg_bwtele_npc', 0);
  await sleep(250);
  T('Patchina offers the chapter: quest 10 active', await interactUntil(async () => (await cq(10)) === 1), await ch2());
  const clock = await call('dbg_ch2race');
  T('the cider clock is running (~120s)', clock > 100 && clock <= 120, clock);
  await page.screenshot({ path: __dirname + '/story3-offer.png' });
  await call('dbg_bwtele_site', 2);   // the Frozen Lake — the race's far end
  await sleep(300);
  T('arriving at the lake completes the race', (await pollCh2('dbg_ch2q', v => (v & 3) === 2, 12000)) !== undefined && (await cq(10)) === 2, await ch2());
  T('the clock stops at the lake', (await call('dbg_ch2race')) === -1, await call('dbg_ch2race'));

  // ---- quest 11: THE LONG WALK ----
  await call('dbg_bwtele_npc', 0);
  await sleep(250);
  T('the escort is offered: quest 11 active', await interactUntil(async () => (await cq(11)) === 1), await ch2());
  T('Duster is placed at its post', (await call('dbg_ch2escd')) >= 0, await call('dbg_ch2escd'));
  // walk it in heel-hops along the road, as a player would: village ->
  // mid-meadow -> the pass's west shoulder -> the farm (the last leg
  // rides the taught waypoint through the pass). Battles pause Duster
  // by design and a pass hunter carries ~970 hp, so each ambush costs
  // real seconds — the leashes are sized for them.
  await call('dbg_bwtp', -700, -1650);
  await sleep(250);
  const hopA = await pollCh2('dbg_ch2escd', v => v >= 0 && v < 90, 90000);
  T('Duster follows across the meadow', hopA >= 0 && hopA < 90, hopA);
  await call('dbg_bwtp', 1100, -1800);
  await sleep(250);
  const atPass = await pollCh2('dbg_ch2escd', v => v >= 0 && v < 90, 90000);
  T('Duster follows to the farm pass', atPass >= 0 && atPass < 90, atPass);
  await page.screenshot({ path: __dirname + '/story3-pass.png' });
  await call('dbg_bwtp', 3150, -2400);   // just inside the farm's west aisle: the player waits at the gate they came through, and Duster's last yards are the aisle, not the whole rack field (under battery load the centre-wait outran the leash — the story2test ridge-walk family)
  await sleep(250);
  T('Duster arrives at the farm: quest 11 done', (await pollCh2('dbg_ch2q', v => ((v >> 2) & 3) === 2, 150000)) !== undefined && (await cq(11)) === 2, await ch2());
  T('the rematch arms itself (quest 12 active)', (await cq(12)) === 1, await ch2());

  // ---- quest 12: RAID, AGAIN ----
  await call('dbg_budup', 60);   // tent-top-up before the boss, as a player would
  await call('dbg_bwtp', 3400, -2400);   // the farm's centre aisle, inside the rematch's temper radius
  await sleep(400);
  if ((await call('dbg_bwbattle')) > 0 && (await call('dbg_boss')) === 0) await winBattle();   // a roamer may butt in first
  T('the rematch starts on approach (boss 4)', (await poll('dbg_boss', v => v === 4, 15000)) === 4, await call('dbg_boss'));
  await page.screenshot({ path: __dirname + '/story3-rematch.png' });
  await winBattle();
  T('the rematch is won: quest 12 done', (await cq(12)) === 2, await ch2());
  T('the festival report arms itself (quest 13 active)', (await cq(13)) === 1, await ch2());

  // ---- quest 13: PATCH DAY ----
  await call('dbg_bwtele_npc', 0);
  await sleep(250);
  T('reporting to Patchina completes quest 13', await interactUntil(async () => (await cq(13)) === 2), await ch2());
  T('the festival is playing', (await call('dbg_ch2fest')) === 1, await call('dbg_ch2fest'));
  await page.screenshot({ path: __dirname + '/story3-festival.png' });
  T('the whole chapter reads done', (await ch2()) === 170, await ch2());   // 2 + 2*4 + 2*16 + 2*64

  // ---- separation: the shipped story's records never moved ----
  T('story_bits is byte-identical to before the chapter', (await call('dbg_quest')) === bitsBefore, (await call('dbg_quest')) + ' vs ' + bitsBefore);
  T('sq2 below the chapter bits is unchanged', ((await call('dbg_sq2v')) & 0x1FFFF) === sq2LowBefore, await call('dbg_sq2v'));

  // ---- persistence + the festival replays, unpaid ----
  await key('Escape', 250);
  T('left Banzai World', (await poll('dbg_bwstate', v => v === 0)) === 0);
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400); await sleep(300); await key('Space', 500);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); await sleep(300); }
  T('run resumed', (await call('dbg_state')) === 1);
  T('chapter states survived the reload', (await ch2()) === 170, await ch2());
  T('story still complete after the reload', (await call('dbg_story10')) === 10);
  // back through the real gate: the panel's STORY MODE line now tells
  // of Patch Day (screenshot evidence; the rows are the shipped three)
  await call('dbg_tp', 215, -5868);
  await sleep(250);
  await key('KeyE', 200);
  T('gate panel opens for a finished story', (await poll('dbg_gate', v => v === 1)) === 1);
  await page.screenshot({ path: __dirname + '/story3-gate.png' });
  await key('Space', 250);
  T('story mode re-entered', (await poll('dbg_bwstate', v => v === 1)) === 1);
  await call('dbg_bwtele_npc', 0);
  await sleep(250);
  T('Patchina replays the festival', await interactUntil(async () => (await call('dbg_ch2fest')) === 1), await call('dbg_ch2fest'));
  T('the replay moves no quest state', (await ch2()) === 170, await ch2());

  console.log('\nstory3test: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
