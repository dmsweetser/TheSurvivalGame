// probe-nov6.js — visual checks for the Nov 6 batch (not a pass/fail suite):
// title bottom, main HUD, bug pupils, buddy flop, craft prompt, story roads.
const { chromium } = require('playwright-core');
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = (ms) => new Promise(r => setTimeout(r, ms));

(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR:', String(e).slice(0, 200)));
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms = 160) => { await page.keyboard.press(code); await sleep(ms); };
  const shot = async (name, clip) => {
    const cv = await page.$('canvas');
    await cv.screenshot({ path: __dirname + '/' + name, ...(clip ? { clip } : {}) });
    console.log('shot', name);
  };

  // 1) title screen bottom
  await sleep(1200);
  await shot('probe-title.png');

  // 2) main game: HUD + bugs with pupils (wait for bugs to close in)
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await sleep(14000);
  await shot('probe-main.png');
  const box = await (await page.$('canvas')).boundingBox();
  await shot('probe-bugs.png', { x: box.x + box.width / 2 - 260, y: box.y + box.height / 2 - 200, width: 520, height: 400 });

  // 3) buddy flop: story mode, walk east, burst of shots
  await call('dbg_bwentm', 1);
  await sleep(600);
  await page.keyboard.down('ArrowRight');
  for (let i = 0; i < 4; i++) { await sleep(170); await shot('probe-flop' + i + '.png'); }
  await page.keyboard.up('ArrowRight');
  await key('Escape', 300); await sleep(400);

  // 4) story terrain: stand on the village -> farm road, east of the village
  await call('dbg_bwentm', 1);
  await sleep(400);
  await call('dbg_bwtp', -2050, -1458);
  await sleep(500);
  await shot('probe-road.png');
  await key('Escape', 300); await sleep(400);

  // 5) craft prompt: face a tree in Buddy Craft
  await call('dbg_bwentm', 4);
  await sleep(500);
  let tree = null;
  for (let r = 3; r <= 16 && !tree; r++)
    for (let tx = -r; tx <= r && !tree; tx++)
      for (let ty = -r; ty <= r && !tree; ty++)
        if ((await call('dbg_craftnode', tx, ty)) === 1) tree = [tx, ty];
  if (tree) {
    await call('dbg_bwtp', tree[0] * 48 + 24 - 62, tree[1] * 48 + 24);
    await sleep(200);
    await page.keyboard.down('ArrowRight'); await sleep(90); await page.keyboard.up('ArrowRight');
    await sleep(250);
    await shot('probe-craftprompt.png');
  } else console.log('no tree found for craft prompt shot');

  await browser.close();
  console.log('probe done');
})().catch(e => { console.error('ERR', e); process.exit(1); });
