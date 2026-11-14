const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  const key = async (code, ms = 160, wait = 220) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  await page.goto(process.env.URL || 'http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Enter', 300, 500);
  await page.waitForFunction(() => window.__TSG.dbg_state() === 7, null, { timeout: 10000 });
  await key('Enter', 300, 600);
  await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  await sleep(4000);
  const keys = await page.evaluate(() => Object.keys(localStorage));
  console.log('keys:', keys.join(', '));
  for (const k of keys) {
    const v = await page.evaluate((k) => localStorage.getItem(k), k);
    console.log(`--- ${k} (${v.length} chars) ---`);
    console.log(v.slice(0, 900));
  }
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
