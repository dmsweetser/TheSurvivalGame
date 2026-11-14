// WALK OF FAME (Nov 14, 2026): crawling through the Bug Mall under a
// big hat triggers confetti + the flashing marquee; walking normally,
// standing still, or crawling outside the mall does not.
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
  const key = async (k, ms = 120) => { await page.keyboard.press(k); await sleep(ms); };

  ok((await call('dbg_state')) === 0, 'boots to title');
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  ok((await call('dbg_state')) === 1, 'run started');

  // heavy hat: 14 feeds x 10 kg = 140 kg -> crawl speed ~42 px/s
  for (let i = 0; i < 14; i++) await call('dbg_feed');
  ok((await call('dbg_carried')) >= 130, 'hat is heavy', await call('dbg_carried'));
  ok((await call('dbg_speed')) < 640, 'speed is a crawl', await call('dbg_speed'));

  // into the mall
  await call('dbg_tp', 0, 80);
  await sleep(150);
  await key('KeyE', 300);
  ok((await call('dbg_mall')) === 1, 'entered the mall');

  // strut across the open mid-mall floor: the parade should start
  await call('dbg_tp', -60, -5980);
  await sleep(200);
  await page.keyboard.down('ArrowRight');
  await sleep(900);
  ok((await call('dbg_wof')) === 1, 'WALK OF FAME active while crawling in the mall');
  const cv = await page.$('canvas');
  await cv.screenshot({ path: __dirname + '/wof-on.png' });
  await sleep(400);
  await cv.screenshot({ path: __dirname + '/wof-on2.png' });
  await page.keyboard.up('ArrowRight');

  // stand still: the parade ends
  await sleep(700);
  ok((await call('dbg_wof')) === 0, 'parade ends when you stop moving');

  // leave the mall, keep the hat: crawling outside is just crawling
  await call('dbg_tp', 0, -5850);
  await sleep(150);
  await key('KeyE', 300);
  ok((await call('dbg_mall')) === 0, 'left the mall');
  await page.keyboard.down('ArrowRight');
  await sleep(700);
  ok((await call('dbg_wof')) === 0, 'no parade outside the mall');
  await page.keyboard.up('ArrowRight');
  await page.close();

  // page 2: light hat in the mall — a normal walk gets no parade
  const page2 = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page2.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page2.goto(URL);
  await page2.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T2 = await page2.evaluateHandle(() => window.__TSG);
  const call2 = async (fn, ...args) => page2.evaluate(([t, f, a]) => t[f](...a), [T2, fn, args]);
  const key2 = async (k, ms = 120) => { await page2.keyboard.press(k); await sleep(ms); };
  await key2('Space', 400);
  if ((await call2('dbg_state')) !== 1) await key2('Enter', 400);
  await call2('dbg_tp', 0, 80);
  await sleep(150);
  await key2('KeyE', 300);
  ok((await call2('dbg_mall')) === 1, 'page2 entered the mall');
  ok((await call2('dbg_speed')) > 1000, 'page2 speed is a normal walk', await call2('dbg_speed'));
  await page2.keyboard.down('ArrowRight');
  await sleep(800);
  ok((await call2('dbg_wof')) === 0, 'no parade at normal speed');
  await page2.keyboard.up('ArrowRight');
  await page2.close();

  await browser.close();
  console.log(`\nwof test: ${pass} pass, ${fail} fail`);
  process.exit(fail ? 1 : 0);
})();
