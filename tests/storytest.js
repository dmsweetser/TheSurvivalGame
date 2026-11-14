// storytest.js — Story Mode (Phase 2a): the gate panel, Patch Village and
// Elder Patchina, the quest log + backpack, the Server Farm quest, the
// hermit's house call, and story progress surviving a save/load.
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
  const qstate = async q => ((await call('dbg_quest')) >> (q * 2)) & 3;
  const farmkills = async () => ((await call('dbg_quest')) >> 8) & 15;
  const item = async i => Math.floor((await call('dbg_items')) / Math.pow(10, i)) % 10;

  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_budup', 40);   // a veteran buddy: farm hackers are level ~5

  // the gate panel, for real: mall, gate, INTERACT, pick STORY MODE
  await call('dbg_tp', 0, 80);
  await key('KeyE', 200); await sleep(400);
  await call('dbg_tp', 185, -5908);
  await sleep(250);
  await key('KeyE', 200);
  T('gate panel opens', (await poll('dbg_gate', v => v === 1)) === 1);
  await page.screenshot({ path: __dirname + '/story-gate.png' });
  await key('Space', 250);
  T('story mode entered', (await poll('dbg_bwstate', v => v === 1)) === 1);
  T('quest 0 is active (meet the elder)', (await qstate(0)) === 1, await call('dbg_quest'));

  // Patch Village + Elder Patchina
  await call('dbg_bwtele_npc', 0);
  await sleep(300);
  T('standing in Patch Village', (await call('dbg_site')) === 0, await call('dbg_site'));
  await page.screenshot({ path: __dirname + '/story-village.png' });
  await key('KeyE', 200); await sleep(300);
  T('elder met: quest 0 done', (await qstate(0)) === 2, await call('dbg_quest'));
  T('quest 1 active (server farm)', (await qstate(1)) === 1);
  T('quest 2 active (house call)', (await qstate(2)) === 1);
  T('elder gifted 2 Patch Notes', (await item(0)) === 2, await call('dbg_items'));

  // the quest log + backpack
  for (let tries = 0; tries < 5 && (await call('dbg_log')) !== 1; tries++) {
    if ((await call('dbg_bwbattle')) > 0) break;
    await key('KeyF', 200); await sleep(250);
  }
  T('quest log opens (wall key)', (await poll('dbg_log', v => v === 1)) === 1);
  await page.screenshot({ path: __dirname + '/story-log.png' });
  await key('KeyF', 200);
  T('quest log closes', (await poll('dbg_log', v => v === 0)) === 0);

  // the Adrenal Cookie, used from the log
  await call('dbg_giveitem', 2);
  T('cookie in the pack', (await item(2)) === 1);
  await key('KeyF', 200); await sleep(200);
  await key('ArrowDown', 150); await key('ArrowDown', 150);   // sel -> cookie row
  await key('Space', 250);
  T('cookie eaten from the log', (await item(2)) === 0, await call('dbg_items'));
  await key('KeyF', 200); await sleep(200);

  // the Server Farm quest: three wins on site
  await call('dbg_bwtele_site', 1);
  await sleep(300);
  T('standing at the Server Farm', (await call('dbg_site')) === 1, await call('dbg_site'));
  await page.screenshot({ path: __dirname + '/story-farm.png' });
  for (let round = 1; round <= 3; round++) {
    await call('dbg_bwhackhere');   // the fight happens HERE, on site
    const inBattle = await poll('dbg_bwbattle', v => v > 0, 9000);
    T('farm battle ' + round + ' started', inBattle > 0, inBattle);
    const t0 = Date.now();
    while (Date.now() - t0 < 30000) {
      const ph = await call('dbg_bwbattle');
      if (ph === 0) break;
      if (ph === 1) { await call('dbg_bwchoose', 0); await sleep(350); }
      else await sleep(300);
    }
    T('farm battle ' + round + ' won (battle over, buddy alive)', (await call('dbg_bwbattle')) === 0 && (await call('dbg_buddy')) !== 0);
    T('farm kills = ' + round, (await farmkills()) === round, await call('dbg_quest'));
    await sleep(1500);   // post-battle grace
  }
  T('quest 1 done after the third clear', (await qstate(1)) === 2, await call('dbg_quest'));

  // the hermit's house call — since Nov 4 the lake's hackers HUNT, so
  // a lone E press can be eaten by a chase battle; win it and re-press
  await call('dbg_bwtele_npc', 2);
  await sleep(300);
  T('standing at the Frozen Lake', (await call('dbg_site')) === 2, await call('dbg_site'));
  for (let tries = 0; tries < 6 && (await qstate(2)) !== 2; tries++) {
    if ((await call('dbg_bwbattle')) > 0) {
      for (let i = 0; i < 60 && (await call('dbg_bwbattle')) > 0; i++) {
        if ((await call('dbg_bwbattle')) === 1) { await call('dbg_bwchoose', 0); await sleep(300); }
        else await sleep(300);
      }
      await call('dbg_bwtele_npc', 2); await sleep(250);
      continue;
    }
    await key('KeyE', 200); await sleep(350);
  }
  T('quest 2 done (house call)', (await qstate(2)) === 2, await call('dbg_quest'));
  T('hermit gifted 3 Energy Drinks', (await item(1)) === 3, await call('dbg_items'));

  // story progress survives leaving + a full reload
  await key('Escape', 250);
  T('left Banzai World', (await poll('dbg_bwstate', v => v === 0)) === 0);
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400); await sleep(300); await key('Space', 500);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); await sleep(300); }
  T('run resumed', (await call('dbg_state')) === 1);
  T('quests survived the reload', (await qstate(0)) === 2 && (await qstate(1)) === 2 && (await qstate(2)) === 2, await call('dbg_quest'));
  T('items survived the reload', (await item(0)) >= 2 && (await item(1)) === 3, await call('dbg_items'));

  // free play has no sites: the village is just meadow there
  await call('dbg_bwenter');
  T('free play entered', (await poll('dbg_bwstate', v => v === 1)) === 1);
  await call('dbg_bwtele_site', 0);
  await sleep(300);
  T('no village in free play', (await call('dbg_site')) === -1, await call('dbg_site'));
  await key('Escape', 250);

  console.log('\nstorytest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
