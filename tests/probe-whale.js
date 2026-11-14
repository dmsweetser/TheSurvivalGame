const { chromium } = require('playwright-core');
(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome' });
  const page = await browser.newPage();
  page.on('console', m => console.log('CONSOLE', m.type(), m.text().slice(0,200)));
  page.on('pageerror', e => console.log('PAGEERROR', String(e).slice(0,300)));
  await page.goto('http://localhost:8903/the-survival-game.html');
  await new Promise(r => setTimeout(r, 6000));
  console.log('TSG?', await page.evaluate(() => typeof window.__TSG));
  console.log('err el:', await page.evaluate(() => { const e = document.getElementById('err'); return e ? e.textContent : '(none)'; }));
  await browser.close();
})().catch(e => console.error('ERR', e.message));
