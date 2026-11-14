const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150, wait = 200) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 4);                    // species 4 (platypus), draft name idx 0
  console.log('buddy name idx:', await call('dbg_budname'));
  await key('Escape', 350); await sleep(300);    // pause (state 5)
  await key('ArrowDown', 200); await sleep(200); // SAVE + QUIT TO TITLE
  await key('Space', 400); await sleep(700);
  if ((await call('dbg_state')) !== 0) { console.log('state after save+quit:', await call('dbg_state')); }
  const raw = await page.evaluate(() => localStorage.getItem('tsg_save1'));
  console.log('RAW SLOT:', JSON.stringify((raw || '').slice(0, 120)));
  await call('dbg_peek');
  console.log('peek: species', await call('dbg_pkbuddy', 1), 'nameidx', await call('dbg_pkbname', 1));
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
