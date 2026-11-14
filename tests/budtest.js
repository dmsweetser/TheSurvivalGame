// budtest.js — the extended Banzai Buddies: adopt a PET ROCK through
// the species -> name -> color picker (AEGNOR, neon rainbow), rename
// and recolor it afterwards, check it follows its owner through the
// mall door both ways, then train it, faint it, and revive it —
// training and skills must survive the faint, at a price that
// reflects the trained buddy.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k); await sleep(ms); };
  async function poll(fn, want, timeout = 15000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(250); }
    return call(fn);
  }
  async function walkTo(tx, ty, tol = 10, timeout = 20000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) return true;
      const keys = [];
      if (x < tx - tol) keys.push('ArrowRight'); else if (x > tx + tol) keys.push('ArrowLeft');
      if (y < ty - tol) keys.push('ArrowDown'); else if (y > ty + tol) keys.push('ArrowUp');
      for (const k of keys) await page.keyboard.down(k);
      await sleep(90);
      for (const k of keys) await page.keyboard.up(k);
    }
    return false;
  }

  ok((await call('dbg_state')) === 0, 'boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  ok((await call('dbg_state')) === 1, 'run started');
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  ok((await call('dbg_mall')) === 1, 'entered the mall');
  ok(await walkTo(225, -6108, 14), 'walked to the buddies counter');
  for (let i = 0; i < 6; i++) await call('dbg_feed');   // 60 kg: exactly a rock's price

  await key('KeyE', 350);
  ok((await call('dbg_station')) === 3, 'buddies panel open');
  for (let i = 0; i < 4; i++) await key('ArrowDown', 130);   // row 4: PET ROCK
  await key('Space', 350);
  ok((await call('dbg_budstage')) === 1, 'species picked: name picker opens', await call('dbg_budstage'));
  await key('Space', 350);                                    // row 0: AEGNOR
  ok((await call('dbg_budstage')) === 2, 'name picked: color picker opens', await call('dbg_budstage'));
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/bud-colors.png' });
  for (let i = 0; i < 9; i++) await key('ArrowDown', 120);   // row 9: NEON RAINBOW
  await key('Space', 400);
  const bud = await poll('dbg_buddy', v => v !== 0, 20000);
  ok(bud === 5, 'pet rock adopted after payment', bud);
  ok((await call('dbg_budname')) === 0, 'its name is AEGNOR', await call('dbg_budname'));
  ok((await call('dbg_budcolor')) === 9, 'its coat is NEON RAINBOW', await call('dbg_budcolor'));
  ok((await call('dbg_budstage')) === 0, 'picker closed after adoption');
  await key('Escape', 300);

  // the buddy follows through the door, both ways (the retired
  // robot clones used to fill this role — the buddies own it now)
  ok((await call('dbg_budy')) < -5700, 'buddy beside owner inside the mall', await call('dbg_budy'));
  ok(await walkTo(0, -5850, 10), 'walked to the exit mat');
  await key('KeyE', 300);
  ok((await call('dbg_mall')) === 0, 'left the mall');
  const yOut = await poll('dbg_budy', v => v > -1000, 5000);
  ok(yOut > -1000, 'buddy followed OUT of the mall', yOut);
  await key('KeyE', 300);   // back in through the hut
  ok((await call('dbg_mall')) === 1, 'back in the mall');
  const yIn = await poll('dbg_budy', v => v < -5700, 5000);
  ok(yIn < -5700, 'buddy followed back IN', yIn);

  // rename + recolor the rock at the counter
  ok(await walkTo(225, -6108, 14), 'back at the buddies counter');
  await key('KeyE', 350);
  for (let i = 0; i < 9; i++) await key('ArrowDown', 120);   // row 9: COLOR
  await key('Space', 350);
  ok((await call('dbg_budstage')) === 4, 'recolor picker opens');
  await key('ArrowDown', 130);                                // row 1: RED
  await key('Space', 350);
  ok((await call('dbg_budcolor')) === 1, 'coat is now RED', await call('dbg_budcolor'));
  for (let i = 0; i < 8; i++) await key('ArrowDown', 120);   // row 8: NAME
  await key('Space', 350);
  ok((await call('dbg_budstage')) === 3, 'rename picker opens');
  for (let i = 0; i < 3; i++) await key('ArrowDown', 120);   // row 3: ANGROD
  await key('Space', 350);
  ok((await call('dbg_budname')) === 3, 'name is now ANGROD', await call('dbg_budname'));
  await key('Escape', 300);
  await cv.screenshot({ path: __dirname + '/bud-rock.png' });

  // train it, faint it, revive it — at the counter the whole time
  await call('dbg_setbudxp', 500);
  await key('KeyE', 350);
  await key('Space', 350);                                    // row 0: TRAIN HEALTH (12 XP)
  await key('ArrowDown', 130);
  await key('Space', 350);                                    // row 1: TRAIN ATTACK (12 XP)
  await key('ArrowDown', 130); await key('ArrowDown', 130);
  await key('Space', 350);                                    // row 3: TEACH move 2 (40 XP)
  ok((await call('dbg_budups')) === 1101, 'trained: +1 HP, +1 ATK, move 2 taught', await call('dbg_budups'));
  ok((await call('dbg_budrevive')) === 49, 'revive priced off the trained rock: 49 KG', await call('dbg_budrevive'));
  await key('Escape', 300);
  await call('dbg_budfaint');
  ok((await call('dbg_buddyhp')) === 0, 'buddy fainted');
  ok((await call('dbg_budups')) === 1101, 'the faint cost it nothing: upgrades intact');
  for (let i = 0; i < 6; i++) await call('dbg_feed');         // 60 kg hat
  await key('KeyE', 350);
  await key('Space', 400);
  const hp2 = await poll('dbg_buddyhp', v => v > 0, 20000);
  ok(hp2 === 445, 'revived at full trained HP (420 base + 25 training)', hp2);
  ok((await call('dbg_budups')) === 1101, 'the revive kept every upgrade');
  const left = await call('dbg_carried');
  ok(Math.abs(left - 110) <= 25, 'charged exactly the 49 KG revive price', left);
  await key('Escape', 300);

  // XP: the rock earns from its own kills (doubled rate)
  ok(await walkTo(0, -5850, 10), 'walked to the exit mat again');
  await key('KeyE', 300);
  await call('dbg_tp', 0, 300); await sleep(300);
  await call('dbg_spawnbug', 20, 290);
  await call('dbg_spawnbug', 0, 322);
  const xp = await poll('dbg_buddyxp', v => v > 0, 20000);
  ok(xp > 0, 'buddy earned XP from the fight', xp);

  console.log(`${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERROR', e); process.exit(2); });
