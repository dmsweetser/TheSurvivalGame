const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
const PADSCRIPT = npads => {
  function mkpad(index) {
    const buttons = [];
    for (let i = 0; i < 17; i++) buttons.push({ pressed: false, touched: false, value: 0 });
    return { id: 'Fake Pad ' + index, index, connected: true, mapping: 'standard', buttons, axes: [0, 0, 0, 0] };
  }
  window.__pads = [];
  for (let i = 0; i < npads; i++) window.__pads.push(mkpad(i));
  navigator.getGamepads = () => window.__pads;
  window.__btn = (pi, bi, on) => { const b = window.__pads[pi].buttons[bi]; b.pressed = on; b.value = on ? 1 : 0; };
};
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(PADSCRIPT, 2);
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  const key = async (k, ms = 160) => { await page.keyboard.press(k); await sleep(ms); };
  await sleep(600);
  await page.screenshot({ path: __dirname + '/probe-menu.png' });
  for (let i = 0; i < 4; i++) await key('ArrowDown', 160);
  await page.screenshot({ path: __dirname + '/probe-menu2.png' });
  await key('Space', 300);
  console.log('after 4dn+Space: state=', await call('dbg_state'), 'sel?', await call('dbg_menusel').catch(() => 'noexport'));
  for (let i = 0; i < 4; i++) await key('ArrowUp', 160);
  await key('Space', 500);
  console.log('after 4up+Space: state=', await call('dbg_state'));
  await browser.close();
})();
