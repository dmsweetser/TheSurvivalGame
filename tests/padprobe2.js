// padprobe2.js — replicate padui's exact tap() and watch the highlight.
const { chromium } = require('playwright-core');
const URL = 'http://127.0.0.1:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', String(e).slice(0, 200)));
  await page.addInitScript(() => {
    const mkpad = (idx) => ({ index: idx, id: 'Fake Pad', connected: true, mapping: 'standard',
      axes: [0, 0, 0, 0], buttons: Array.from({ length: 17 }, () => ({ pressed: false, value: 0 })) });
    window.__pads = [];
    navigator.getGamepads = () => window.__pads;
    window.__mkpad = mkpad;
    window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
  });
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_menun, null, { timeout: 20000 });
  const ev = (fn, arg) => page.evaluate(fn, arg);
  const tap = async (bi) => { await ev(b => window.__btn(0, b, true), bi); await sleep(260); await ev(b => window.__btn(0, b, false), bi); await sleep(240); };
  await ev(() => { window.__pads = [window.__mkpad(0)]; });
  await page.waitForFunction(() => window.__TSG.dbg_pads() === 1, null, { timeout: 5000 });
  await tap(13);
  await page.screenshot({ path: 'padprobe2-a.png' });
  await tap(13);
  await page.screenshot({ path: 'padprobe2-b.png' });
  await tap(0);
  await sleep(200);
  console.log('diff after A:', await ev(() => window.__TSG.dbg_diff()), 'state:', await ev(() => window.__TSG.dbg_state()));
  await page.screenshot({ path: 'padprobe2-c.png' });
  await browser.close();
})().catch(e => { console.error('ERROR', e); process.exit(2); });
