// bosstest.js — Story Mode Phase 3: the farm terminal puzzle (read the
// scratched order, press in sequence, wrong press resets), BROKEN RAID
// in the inner aisle, the firewalled compound gate (shut until the
// GATE CRANK from the Sunken Cache), and ZERO-DAY ZED's boss fight.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + String(extra).slice(0, 140) : ''); }
}
const TERM = [[3230, -2460], [3570, -2460], [3400, -2210]];
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
  const bits = async () => call('dbg_quest');
  const prog = async () => ((await bits()) >> 12) & 7;
  const puzdone = async () => ((await bits()) >> 15) & 1;
  const bit = async n => ((await bits()) >> n) & 1;
  const qstate = async q => ((await bits()) >> (q * 2)) & 3;
  async function winBattle(label) {
    const t0 = Date.now();
    while (Date.now() - t0 < 90000) {
      const ph = await call('dbg_bwbattle');
      if (ph === 0) return true;
      if (ph === 1) { await call('dbg_bwchoose', 0); await sleep(320); }
      else await sleep(280);
    }
    return false;
  }
  // one REGISTERED press per call: since Nov 4 the farm's hackers hunt,
  // and a press can be eaten by a chase battle — win it and press again
  // until the puzzle state actually moves
  const pressTerm = async i => {
    const before = await prog(), doneBefore = await puzdone();
    for (let tries = 0; tries < 8; tries++) {
      if ((await prog()) !== before || (await puzdone()) !== doneBefore) return;
      if ((await call('dbg_bwbattle')) > 0) { await winBattle('stray'); continue; }
      await call('dbg_bwtp', TERM[i][0], TERM[i][1] + 26); await sleep(200);
      await key('KeyE', 200); await sleep(250);
    }
  };

  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_budup', 60);
  // park the DEV inside the mall before the BW shortcut: dbg_bwstory enters
  // BW directly, and a DEV left standing in the open field gets nibbled to
  // death over a long suite, ending the run mid-battle (the Nov 5 freeze)
  await call('dbg_tp', 0, 80); await sleep(300); await key('KeyE', 250); await sleep(400);
  T('DEV parked in the mall', (await call('dbg_mall')) === 1, await call('dbg_mall'));
  await call('dbg_bwstory');
  T('story mode', (await poll('dbg_bwstate', v => v === 1)) === 1);

  // the puzzle: read the order off the sign (the hook mirrors it), fumble
  // once, then play it straight
  await call('dbg_bwtp', 3400, -2074);
  await sleep(250);
  await key('KeyE', 200); await sleep(250);   // read the scratched note
  const packed = await call('dbg_termord');
  const ord = [packed % 3, Math.floor(packed / 3) % 3, Math.floor(packed / 9) % 3];
  T('an order exists', new Set(ord).size === 3, packed);
  await pressTerm(ord[1]);   // wrong first press (the second note first)
  T('wrong press leaves progress at 0', (await prog()) === 0, await prog());
  await pressTerm(ord[0]);
  T('first right press: 1/3', (await prog()) === 1, await prog());
  await pressTerm(ord[2]);   // wrong (third before second)
  T('out-of-order press resets to 0', (await prog()) === 0, await prog());
  await pressTerm(ord[0]); await pressTerm(ord[1]);
  T('two right: 2/3', (await prog()) === 2, await prog());
  await page.screenshot({ path: __dirname + '/boss-terminals.png' });
  await pressTerm(ord[2]);
  T('puzzle solved', (await puzdone()) === 1, await bits());

  // BROKEN RAID, in the inner aisle the puzzle opened
  await call('dbg_budup', 60);   // tent-top-up before the boss (capability-scaled foes, Nov 5)
  await call('dbg_bwtp', 3400, -2380);   // inside RAID's 27px temper radius
  await sleep(400);
  if ((await call('dbg_bwbattle')) > 0 && (await call('dbg_boss')) === 0) await winBattle('stray');   // a roamer may butt in first
  await page.screenshot({ path: __dirname + '/boss-raid.png' });
  T('RAID battle starts on approach', (await poll('dbg_boss', v => v === 1, 6000)) === 1, await call('dbg_boss'));
  T('RAID won', await winBattle('raid'));
  await sleep(400);
  T('RAID marked defeated', (await bit(16)) === 1, await bits());
  T('buddy alive after RAID', (await call('dbg_buddy')) !== 0);

  // the compound gate: shut without the GATE CRANK... (clear any
  // roamer battle first — a fight eats the walk and fakes a "hold")
  // walk at the gate until the walk is CLEAN (no roamer fight stole
  // it) and the outcome is real: in, or refused AT the fence (y<3745)
  const walkIn = async (wantIn) => {
    for (let a = 0; a < 8; a++) {
      if ((await call('dbg_bwbattle')) > 0) await winBattle('gate');
      await call('dbg_bwtp', 5800, 3760);
      await sleep(200);
      // Pulse-and-poll the walk instead of one fixed 1200 ms hold: under
      // battery load a single hold can end before the buddy reaches the
      // gate, faking the outcome (the v1.26 battery flake: "gate opens
      // with the crank" failed at site -1 solo-clean). Outcomes are the
      // original ones — site 3 for entry; for a hold, site !== 3 with
      // y < 3745 AND y stable across two polls (stopped at the fence,
      // not merely passing the line mid-walk).
      const t0 = Date.now();
      let prevY = null, outcome = false, fought = false;
      while (Date.now() - t0 < 12000) {
        await page.keyboard.down('ArrowUp'); await sleep(250); await page.keyboard.up('ArrowUp');
        await sleep(120);
        if ((await call('dbg_bwbattle')) > 0) { fought = true; break; }
        const site = await call('dbg_site'), y = await call('dbg_bwy');
        if (wantIn) { if (site === 3) { outcome = true; break; } }
        else if (site !== 3 && y < 3745 && prevY !== null && Math.abs(y - prevY) < 2) { outcome = true; break; }
        prevY = y;
      }
      if (outcome) return true;
      if (fought) continue;
      const site = await call('dbg_site'), y = await call('dbg_bwy');
      if (wantIn) { if (site === 3) return true; }
      else if (site !== 3 && y < 3745) return true;
    }
    return wantIn ? (await call('dbg_site')) === 3 : (await call('dbg_site')) !== 3;
  };
  T('firewall holds without the crank', await walkIn(false), await call('dbg_site'));
  // ...the farm clear alone no longer suffices (the crank is the key now)
  await call('dbg_setq', 1, 2);
  T('firewall still holds with the farm done but no crank', await walkIn(false), await call('dbg_site'));
  // ...open with the crank from the Sunken Cache
  await call('dbg_bwgivecrank');
  T('gate opens with the crank', await walkIn(true), await call('dbg_site'));

  // ZERO-DAY ZED
  await call('dbg_budup', 60);   // tent-top-up: foes now scale with capability (Nov 5), so arrive at the boss healed, as a player would
  await call('dbg_bwtp', 5800, 3420);
  await sleep(400);
  if ((await call('dbg_bwbattle')) > 0 && (await call('dbg_boss')) === 0) await winBattle('stray');
  T('quest 4 granted on approaching ZED', (await qstate(3)) === 1, await bits());
  T('ZED battle starts on approach', (await poll('dbg_boss', v => v === 2, 15000)) === 2, await call('dbg_boss'));
  await page.screenshot({ path: __dirname + '/boss-zed.png' });
  T('ZED won', await winBattle('zed'));
  await sleep(400);
  T('ZED marked defeated', (await bit(17)) === 1, await bits());
  T('quest 4 done: the funeral is held', (await qstate(3)) === 2, await bits());
  T('buddy alive after ZED', (await call('dbg_buddy')) !== 0);

  console.log('\nbosstest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
