// oceantest.js — the Nov 4 world edge: ocean bounds hold in Story and
// Free Play on all four sides, the interior still walks fine, and the
// minimap (screenshot-checked) rides with the buddy.
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
  async function walk(code, ms) {
    // walk, winning any fight that interrupts (hunters near the shore)
    const t0 = Date.now();
    while (Date.now() - t0 < ms) {
      if ((await call('dbg_bwbattle')) > 0) {
        for (let i = 0; i < 60 && (await call('dbg_bwbattle')) > 0; i++) {
          if ((await call('dbg_bwbattle')) === 1) { await call('dbg_bwchoose', 0); await sleep(280); }
          else await sleep(280);
        }
        continue;
      }
      await page.keyboard.down(code); await sleep(400); await page.keyboard.up(code);
    }
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
  await call('dbg_bwstory');
  T('story mode entered', (await poll('dbg_bwstate', v => v === 1)) === 1);

  // the interior is untouched: open meadow still walks (the village
  // itself is full of huts — solid long before the ocean existed)
  await call('dbg_bwtp', 0, 900); await sleep(250);
  const ix0 = await call('dbg_bwx');
  await walk('ArrowRight', 6000);
  T('interior movement unaffected', (await call('dbg_bwx')) > ix0 + 40, ix0 + '->' + (await call('dbg_bwx')));

  // east edge: start 200 inside it, walk east — never cross 6800
  await call('dbg_bwtp', 6600, 0); await sleep(250);
  await walk('ArrowRight', 2200);
  const ex = await call('dbg_bwx');
  T('east shore holds (x stays <= 6800)', ex <= 6800 && ex > 6400, ex);
  await page.screenshot({ path: __dirname + '/ocean-east.png' });

  // north edge
  await call('dbg_bwtp', 0, -3000); await sleep(250);
  await walk('ArrowUp', 2200);
  const ny = await call('dbg_bwy');
  T('north shore holds (y stays >= -3200)', ny >= -3200 && ny < -2800, ny);

  // west + south corner
  await call('dbg_bwtp', -6000, 5200); await sleep(250);
  await walk('ArrowLeft', 1500);
  const wx2 = await call('dbg_bwx');
  T('west shore holds (x stays >= -6200)', wx2 >= -6200 && wx2 < -5800, wx2);
  await walk('ArrowDown', 1500);
  const sy2 = await call('dbg_bwy');
  T('south shore holds (y stays <= 5400)', sy2 <= 5400 && sy2 > 5000, sy2);
  await page.screenshot({ path: __dirname + '/ocean-corner.png' });

  // free play: same sea
  await call('dbg_bwexit'); await sleep(300);
  await call('dbg_bwenter');
  T('free play entered', (await poll('dbg_bwstate', v => v === 1)) === 1);
  await call('dbg_bwtp', 6600, 0); await sleep(250);
  await walk('ArrowRight', 2200);
  const fx = await call('dbg_bwx');
  T('free-play east shore holds too', fx <= 6800 && fx > 6400, fx);

  console.log(`\n${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('SUITE ERROR', e); process.exit(2); });
