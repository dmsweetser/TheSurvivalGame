// craft2test.js — BUDDY CRAFT OVERHAUL (Nov 5, second batch): day/night,
// crafting with prerequisites (raft behind campfire),
// campfire rest sleeping through the night, persistence of builds /
// gear / materials across leaving AND a full reload + Continue,
// demolish with refund, and the wall-can't-entomb-you fix.
// (Nov 6: the menu is ONE segmented list — navigation below walks it
// by row id via dbg_craftrowid; left/right no longer switch anything.)
const { chromium } = require('playwright-core');
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = (ms) => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR:', String(e).slice(0, 200)));
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms = 160) => { await page.keyboard.press(code); await sleep(ms); };
  const poll = async (n, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(n); if (pred(v)) return v; await sleep(120); } return v; };
  const gear = () => call('dbg_craftgear');
  // walk DOWN the one segmented list (verify after every press; the
  // list wraps, so a doubled press just costs one more lap)
  const gotoRow = async (id) => {
    for (let i = 0; i < 48; i++) { if ((await call('dbg_craftrowid')) === id) return true; await key('ArrowDown', 110); }
    return (await call('dbg_craftrowid')) === id;
  };

  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_bwentm', 4);
  T('entered BUDDY CRAFT (mode 4)', (await poll('dbg_bwmode', v => v === 4)) === 4, await call('dbg_bwmode'));

  // --- day/night clock ---
  T('starts in daylight', (await call('dbg_craftnight')) < 55, await call('dbg_craftnight'));
  await call('dbg_craftsettod', 200);
  T('deep night reads dark', (await poll('dbg_craftnight', v => v > 80, 3000)) > 80, await call('dbg_craftnight'));
  // (bugs spawn on land in a ring around the buddy; if this world's
  // spawn clearing is an island, hop inland and keep waiting)
  let bugsSeen = await poll('dbg_craftbugs', v => v >= 1, 6000);
  if (bugsSeen < 1) { await call('dbg_bwtp', 1500, 0); await sleep(300); bugsSeen = await poll('dbg_craftbugs', v => v >= 1, 14000); }
  T('night bugs show up', bugsSeen >= 1, bugsSeen);
  await call('dbg_bwtp', 0, 56); await sleep(300);   // back to the clearing for the rest of the suite
  await call('dbg_craftsettod', 40);

  // --- materials + the axe ---
  await call('dbg_craftsetm', 60 + 60 * 1000 + 60 * 1000000);
  await call('dbg_craftsetcar', 30);
  T('pack stocked', (await call('dbg_craftmat')) === 60060060, await call('dbg_craftmat'));
  await key('KeyF', 250); await sleep(300);
  T('menu opens on TOOLS', (await call('dbg_craftmenu')) === 1 && (await call('dbg_craftcat')) === 0,
    'menu=' + (await call('dbg_craftmenu')) + ' cat=' + (await call('dbg_craftcat')));
  await key('Space', 250); await sleep(300);
  T('WOODEN AXE crafted', ((await gear()) & 1) === 1, await gear());

  // --- the raft is gated behind a built campfire ---
  await gotoRow(4);
  T('raft row selected', (await call('dbg_craftrowid')) === 4, await call('dbg_craftrowid'));
  await key('Space', 250); await sleep(300);
  T('raft refuses without a campfire first', ((await gear()) & 512) === 0, await gear());
  await key('Escape', 250); await sleep(300);

  // --- build the campfire (in the BUILDING segment of the one list) ---
  await key('KeyF', 250); await sleep(300);
  await gotoRow(18);
  T('campfire row selected', (await call('dbg_craftrowid')) === 18 && (await call('dbg_craftcat')) === 2,
    'row=' + (await call('dbg_craftrowid')) + ' cat=' + (await call('dbg_craftcat')));
  await key('Space', 250); await sleep(300);
  T('campfire built', (await call('dbg_craftbuilds')) === 1 && (await call('dbg_craftbuildk', 0)) === 1,
    'n=' + (await call('dbg_craftbuilds')) + ' k=' + (await call('dbg_craftbuildk', 0)));
  T('campfire marks the raft prerequisite met', ((await gear()) & 2048) === 2048, await gear());
  await key('Escape', 250); await sleep(300);

  // --- rest by the fire at night: sleep through to dawn ---
  await call('dbg_craftsettod', 200);
  await sleep(300);
  await key('KeyE', 250);
  const tod = await poll('dbg_crafttod', v => v >= 224, 5000);
  T('resting by the fire sleeps through the night', tod >= 224, tod);
  await page.screenshot({ path: __dirname + '/craft2-dawn.png' });

  // --- now the raft crafts (back up the list to the TOOLS segment) ---
  await key('KeyF', 250); await sleep(300);
  await gotoRow(4);
  T('raft row selected again', (await call('dbg_craftrowid')) === 4 && (await call('dbg_craftcat')) === 0,
    'row=' + (await call('dbg_craftrowid')) + ' cat=' + (await call('dbg_craftcat')));
  await key('Space', 250); await sleep(300);
  T('RAFT crafted once its prerequisite stands', ((await gear()) & 512) === 512, await gear());
  await key('Escape', 250); await sleep(300);
  const matsBefore = await call('dbg_craftmat');
  T('materials as expected after crafting', matsBefore === 47 + 58 * 1000 + 57 * 1000000, matsBefore);

  // --- persistence: leave, let it save, reload, Continue ---
  await key('Escape', 300); await sleep(400);
  T('Escape returns to the mall', (await call('dbg_bwmode')) === -1, await call('dbg_bwmode'));
  await sleep(5600);   // autosave ticks in the mall
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 500);
  T('slot picker opened', (await call('dbg_state')) === 7, await call('dbg_state'));
  await key('Space', 600);
  T('run resumed', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_bwentm', 4);
  T('back in BUDDY CRAFT', (await poll('dbg_bwmode', v => v === 4)) === 4, await call('dbg_bwmode'));
  T('the campfire survived the reload', (await call('dbg_craftbuilds')) === 1 && (await call('dbg_craftbuildk', 0)) === 1,
    'n=' + (await call('dbg_craftbuilds')));
  T('gear survived the reload', ((await gear()) & (1 | 512 | 2048)) === (1 | 512 | 2048), await gear());
  T('materials survived the reload', (await call('dbg_craftmat')) === matsBefore, await call('dbg_craftmat'));

  // --- demolish: the BUILDING segment's last row arms it, E removes, half refund ---
  const woodBefore = (await call('dbg_craftmat')) % 1000;
  await key('KeyF', 250); await sleep(300);
  await gotoRow(102);
  T('demolish row selected', (await call('dbg_craftrowid')) === 102, await call('dbg_craftrowid'));
  await key('Space', 250); await sleep(300);
  T('demolish armed, menu closed', (await call('dbg_craftdemolish')) === 1 && (await call('dbg_craftmenu')) === 0,
    'dem=' + (await call('dbg_craftdemolish')) + ' menu=' + (await call('dbg_craftmenu')));
  await key('KeyE', 250); await sleep(400);
  T('campfire demolished', (await call('dbg_craftbuilds')) === 0, await call('dbg_craftbuilds'));
  T('half the wood came back', ((await call('dbg_craftmat')) % 1000) === woodBefore + 2,
    woodBefore + ' -> ' + ((await call('dbg_craftmat')) % 1000));

  // --- a wall can no longer entomb the builder ---
  await call('dbg_craftsetm', 60 + 60 * 1000 + 60 * 1000000);
  await call('dbg_bwtp', -14, 24); await sleep(250);  // a tile west of the clearing: room to build eastward
  await page.keyboard.down('ArrowRight'); await sleep(250); await page.keyboard.up('ArrowRight'); await sleep(150);   // face east for real (the mall follow left his aim wherever he walked in from)
  await key('KeyF', 250); await sleep(300);
  await gotoRow(16);
  T('wall row selected', (await call('dbg_craftcat')) === 2 && (await call('dbg_craftrowid')) === 16,
    'cat=' + (await call('dbg_craftcat')) + ' row=' + (await call('dbg_craftrowid')));
  await key('Space', 250); await sleep(300);
  T('wooden wall built', (await call('dbg_craftbuilds')) === 1 && (await call('dbg_craftbuildk', 0)) === 0,
    'n=' + (await call('dbg_craftbuilds')) + ' k=' + (await call('dbg_craftbuildk', 0)));
  await key('Escape', 250); await sleep(300);
  const x0 = await call('dbg_bwx');
  await page.keyboard.down('ArrowLeft'); await sleep(600); await page.keyboard.up('ArrowLeft');
  const x1 = await call('dbg_bwx');
  await page.keyboard.down('ArrowRight'); await sleep(400); await page.keyboard.up('ArrowRight');
  const x2 = await call('dbg_bwx');
  T('builder still moves freely after building', x1 < x0 - 40 && x2 > x1 + 30, x0 + ' -> ' + x1 + ' -> ' + x2);
  await page.screenshot({ path: __dirname + '/craft2-wall.png' });

  console.log('craft2test: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
