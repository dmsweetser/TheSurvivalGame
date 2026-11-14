// questtest.js — the expanded quests: Mop's Lost Tools (side), the
// Sunken Cache bridge (Patchina grants it with the funeral; the Cache
// Hoarder holds the GATE CRANK), and the minimap target chain.
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
  const bits = async () => call('dbg_quest');
  const qx = async q => { const b = await bits(); return q < 4 ? (b >> (q * 2)) & 3 : (b >> (18 + (q - 4) * 2)) & 3; };
  const bit = async n => ((await bits()) >> n) & 1;
  const item = async i => Math.floor((await call('dbg_items')) / Math.pow(10, i)) % 10;
  const tgt = async () => [await call('dbg_bwqtarget', 0), await call('dbg_bwqtarget', 1), await call('dbg_bwqtarget', 2)];
  async function winBattle() {
    const t0 = Date.now();
    while (Date.now() - t0 < 60000) {
      const ph = await call('dbg_bwbattle');
      if (ph === 0) return true;
      if (ph === 1) { await call('dbg_bwchoose', 0); await sleep(320); }
      else await sleep(280);
    }
    return false;
  }
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_budup', 60);
  // park the DEV inside the mall first (see bosstest: an exposed DEV dies
  // over a long suite and ends the run mid-BW)
  await call('dbg_tp', 0, 80); await sleep(300); await key('KeyE', 250); await sleep(400);
  await call('dbg_bwstory');
  await sleep(300);
  // meet the elder, then Mop hands out the side quest
  await call('dbg_bwtele_npc', 0); await sleep(250); await key('KeyE', 200); await sleep(300);
  T('elder met', (await qx(0)) === 2);
  await call('dbg_bwtele_npc', 1); await sleep(250); await key('KeyE', 200); await sleep(300);
  T("Mop's Lost Tools granted", (await qx(4)) === 1, await bits());
  // the minimap target: the farm — a main quest outranks a side quest
  let t = await tgt();
  T('target is the farm while it stands (main outranks side)', t[0] === 3400 && t[1] === -2400 && t[2] === 1, t);
  // fetch all three
  const TOOLS = [[1000, 800], [-3600, 400], [2000, 3000]];
  for (let i = 0; i < 3; i++) {
    await call('dbg_bwtp', TOOLS[i][0], TOOLS[i][1] + 20); await sleep(250);
    await key('KeyE', 200); await sleep(250);
    T('tool ' + i + ' picked up', (await bit(24 + i)) === 1, await bits());
  }
  T('all three tools found', (await bit(24)) === 1 && (await bit(25)) === 1 && (await bit(26)) === 1, await bits());
  // the bridge: farm reported done -> Patchina grants funeral + cache
  await call('dbg_setq', 1, 2);
  await call('dbg_bwtele_npc', 0); await sleep(250); await key('KeyE', 200); await sleep(300);
  T('funeral granted', (await qx(3)) === 1, await bits());
  T('sunken cache granted alongside', (await qx(5)) === 1, await bits());
  t = await tgt();
  T('target is the ruins (crank before Zed)', t[0] === -5200 && t[1] === 1800 && t[2] === 1, t);
  await key('KeyF', 200); await sleep(300);
  await page.screenshot({ path: __dirname + '/story-log2.png' });
  await key('KeyF', 200); await sleep(200);
  // the Hoarder (stand INSIDE its 30px temper radius)
  await call('dbg_bwtp', -5200, 1820); await sleep(400);
  if ((await call('dbg_bwbattle')) > 0 && (await call('dbg_boss')) === 0) await winBattle();
  await page.screenshot({ path: __dirname + '/boss-hoarder.png' });
  T('HOARDER battle starts on approach', (await call('dbg_boss')) === 3, await call('dbg_boss'));
  T('HOARDER won', await winBattle());
  await sleep(400);
  T('hoarder marked defeated', (await bit(22)) === 1, await bits());
  T('crank carried', (await bit(23)) === 1, await bits());
  T('cache quest done', (await qx(5)) === 2, await bits());
  t = await tgt();
  T('target moves on to Zed', t[0] === 5800 && t[1] === 3400 && t[2] === 1, t);
  // with the story's spine done, the side quest finally owns the diamond:
  // tools are all found, so it points home to Mop
  await call('dbg_setq', 3, 2);
  t = await tgt();
  T('side quest surfaces: target is Mop (tools all found)', t[0] === -2540 && t[1] === -1320 && t[2] === 1, t);
  await call('dbg_bwtele_npc', 1); await sleep(250); await key('KeyE', 200); await sleep(300);
  T("Mop's quest done", (await qx(4)) === 2, await bits());
  T('Mop paid in cookies', (await item(2)) >= 2, await call('dbg_items'));
  t = await tgt();
  T('last one standing: target is Glitch (house call)', t[0] === -1400 && t[1] === 4340 && t[2] === 1, t);
  console.log(`questtest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
