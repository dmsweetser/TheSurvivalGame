// bwtest.js — Banzai World (Nov 3): enter through the mall gate as
// your buddy, wander a procedural world, fight a hacker turn-by-turn
// (attacks cost energy), sleep at a tent to recover, and leave —
// re-entering puts you exactly where you left off.
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

  ok((await call('dbg_state')) === 0, 'boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) { await key('Enter', 400); }
  ok((await call('dbg_state')) === 1, 'run started');
  await call('dbg_adopt', 0);                        // puppy, 95 HP
  await call('dbg_teach', 1); await call('dbg_teach', 2);
  await call('dbg_teach', 4); await call('dbg_teach', 8);
  await call('dbg_setbudxp', 0);

  // --- entering
  await call('dbg_bwenter');
  ok((await call('dbg_bwstate')) === 1, 'inside Banzai World');
  ok((await call('dbg_bwseed')) !== 0, 'the world has a seed');
  ok((await call('dbg_bwx')) === 0 && (await call('dbg_bwy')) === 56, 'first visit starts at the gate',
     (await call('dbg_bwx')) + ',' + (await call('dbg_bwy')));

  // --- wandering
  await page.keyboard.down('ArrowRight'); await sleep(600); await page.keyboard.up('ArrowRight');
  ok((await call('dbg_bwx')) > 30, 'the buddy walks the world', await call('dbg_bwx'));

  // --- find a hacker and fight
  let roam = await call('dbg_bwroam');
  for (let step = 0; step < 6 && !(roam > 0); step++) {
    await page.keyboard.down('ArrowDown'); await sleep(900); await page.keyboard.up('ArrowDown');
    roam = await poll('dbg_bwroam', v => v > 0, 3000);
  }
  ok(roam > 0, 'hackers roam within reach', roam);
  await call('dbg_setbudenergy', 100);
  await call('dbg_bwtele_hack');
  ok((await poll('dbg_bwbattle', v => v > 0, 6000)) > 0, 'walking into a hacker starts a battle');
  ok((await poll('dbg_bwbattle', v => v === 1, 6000)) === 1, 'battle reaches the choose-an-attack phase');
  const hhp0 = await call('dbg_bwhhp');
  ok(hhp0 > 0, 'hacker has HP', hhp0);
  await call('dbg_bwchoose', 0);                     // basic attack (free)
  const hhp1 = await poll('dbg_bwhhp', v => v < hhp0, 5000);
  ok(hhp1 < hhp0, 'the basic attack lands', hhp0 + ' -> ' + hhp1);
  ok((await call('dbg_budenergy')) === 100, 'the basic attack costs no energy');

  ok((await poll('dbg_bwbattle', v => v === 1, 8000)) === 1, 'back to choosing after the hacker strikes');
  await call('dbg_bwchoose', 1);                     // move 2: 20 energy
  ok((await poll('dbg_budenergy', v => v === 80, 5000)) === 80, 'a taught move spends its 20 energy');

  // finish this fight with free basics (move 2 may already have won it)
  await call('dbg_setbudenergy', 100);
  for (let i = 0; i < 40 && (await call('dbg_bwbattle')) !== 0; i++) {
    if ((await call('dbg_bwbattle')) === 1) await call('dbg_bwchoose', 0);
    await sleep(350);
  }
  ok((await call('dbg_bwbattle')) === 0, 'the battle ends');
  const xp1 = await call('dbg_buddyxp');
  ok(xp1 > 0, 'winning paid buddy XP', xp1);
  ok((await call('dbg_buddyhp')) < 95, 'the hacker got some hits in', await call('dbg_buddyhp'));

  // a second battle, for the energy-starvation rule. The post-win
  // grace means the roamer can stroll off before contact counts, so
  // keep stepping onto it until the battle actually starts.
  for (let i = 0; i < 20 && (await call('dbg_bwbattle')) === 0; i++) {
    await call('dbg_bwtele_hack');
    await sleep(500);
  }
  ok((await poll('dbg_bwbattle', v => v === 1, 10000)) === 1, 'second battle reaches the choose phase');
  await call('dbg_setbudenergy', 10);
  const hhp2 = await call('dbg_bwhhp');
  await call('dbg_bwchoose', 4);                     // move 4 wants 55
  await sleep(600);
  ok((await call('dbg_bwbattle')) === 1 && (await call('dbg_budenergy')) === 10 && (await call('dbg_bwhhp')) === hhp2,
     'a move you cannot afford is refused');
  await call('dbg_setbudenergy', 100);
  for (let i = 0; i < 40 && (await call('dbg_bwbattle')) !== 0; i++) {
    if ((await call('dbg_bwbattle')) === 1) await call('dbg_bwchoose', 0);
    await sleep(350);
  }
  ok((await call('dbg_bwbattle')) === 0, 'the second battle ends too');
  ok((await call('dbg_buddyxp')) > xp1, 'the second win paid more XP', await call('dbg_buddyxp'));

  // --- sleeping at a tent restores everything
  await call('dbg_setbudenergy', 30);
  await call('dbg_bwtele_tent');
  await sleep(200);
  await key('KeyE', 200);
  ok((await poll('dbg_budenergy', v => v === 100, 5000)) === 100, 'sleeping restores full energy');
  ok((await call('dbg_buddyhp')) === 95, 'sleeping restores full HP', await call('dbg_buddyhp'));

  // --- leaving and returning: exactly where you were
  const lx = await call('dbg_bwx'), ly = await call('dbg_bwy');
  await call('dbg_bwexit');
  ok((await call('dbg_bwstate')) === 0 && (await call('dbg_state')) === 1, 'back in the mall run');
  ok((await call('dbg_mall')) === 1, 'and inside the mall');
  const saved = await page.evaluate(() => {
    for (let i = 0; i < localStorage.length; i++) {
      const v = localStorage.getItem(localStorage.key(i));
      if (v && v.indexOf(' gx=') >= 0 && v.indexOf(' gz=') >= 0) return true;
    }
    return false;
  });
  ok(saved, 'the save carries the Banzai World position + seed');
  await call('dbg_bwenter');
  ok((await call('dbg_bwstate')) === 1, 're-entered Banzai World');
  await sleep(300);
  ok((await call('dbg_bwx')) === lx && (await call('dbg_bwy')) === ly,
     'returned exactly where you left off', (await call('dbg_bwx')) + ',' + (await call('dbg_bwy')) + ' vs ' + lx + ',' + ly);
  await key('Escape', 300);
  ok((await call('dbg_bwstate')) === 0 && (await call('dbg_state')) === 1, 'ESC leaves Banzai World');

  console.log(`\nbwtest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(2); });
