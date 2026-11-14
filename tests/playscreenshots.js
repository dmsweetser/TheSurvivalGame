// playscreenshots.js — Google Play packet phone screenshots (1920x1080)
// from the current build. Scenes: title, free-play combat w/ big hat,
// Bug Mall, stacked wall pile, Banzai World, Story Mode (Patch Village),
// Far Country region (Neon RAM Flats). Outputs to tests/playscreens/.
const { chromium } = require('playwright-core');
const fs = require('fs');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const OUT = __dirname + '/playscreens';
const sleep = ms => new Promise(r => setTimeout(r, ms));
fs.mkdirSync(OUT, { recursive: true });

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 160, wait = 220) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
  const shot = async name => { await page.screenshot({ path: OUT + '/' + name }); console.log('SHOT', name); };
  async function poll(fn, args, want, timeout = 9000, step = 200) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn, ...args); if (want(v)) return v; await sleep(step); }
    return call(fn, ...args);
  }

  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await sleep(1200);

  // canvas geometry probe
  const box = await page.evaluate(() => { const c = document.getElementById('cv'); const r = c.getBoundingClientRect(); return { w: r.width, h: r.height, x: r.x, y: r.y }; });
  console.log('canvas box', JSON.stringify(box));

  // 1) TITLE
  await shot('screenshot-01-title.png');

  // start run (menu -> slots -> run), verify-and-retry
  await key('Enter', 300, 500);
  await poll('dbg_state', [], v => v === 7, 6000);
  await key('Enter', 300, 600);
  const st = await poll('dbg_state', [], v => v === 1, 10000);
  console.log('run state', st);
  await call('dbg_adopt', 0); // moose buddy, photogenic

  // 2) COMBAT — big hat out in the meadow, bugs closing in
  for (let i = 0; i < 7; i++) await call('dbg_feed'); // 35 carcasses on the hat
  console.log('hat stack_n', await call('dbg_carried'));
  await call('dbg_tp', 0, 300); await sleep(300);
  await page.evaluate(() => {
    const s = window.__TSG;
    s.dbg_spawnbug(-110, 235); s.dbg_spawnbug(95, 250); s.dbg_spawnbug(-70, 385);
    s.dbg_spawnbug(120, 380); s.dbg_spawnbug(0, 205); s.dbg_spawnbug(-150, 320);
    s.dbg_spawnbug(150, 300); s.dbg_spawnbug(40, 400);
  });
  await key('ArrowDown', 90, 100); // face south
  await sleep(1200);               // let them close in a little
  await key('Space', 150, 120);    // a shot for life
  await key('Space', 150, 60);
  console.log('combat bugs', await call('dbg_bugs'));
  await shot('screenshot-02-combat.png');

  // 4) WALL PILE — quiet spot, feed up, stack F on one spot
  await call('dbg_tp', 500, 500); await sleep(400);
  for (let i = 0; i < 3; i++) await call('dbg_feed');
  await key('ArrowDown', 90, 150); // face south
  await key('KeyF', 200, 300);
  console.log('walls', await poll('dbg_walls', [], v => v === 1, 8000), 'stack', await call('dbg_wallstack'));
  for (let want = 2; want <= 5; want++) {
    await key('KeyF', 200, 300);
    const s = await poll('dbg_wallstack', [], v => v >= want, 6000);
    console.log('stack now', s);
    if (s < want) break;
  }
  // step back south and face the pile
  await call('dbg_tp', 500, 660); await sleep(250);
  await key('ArrowUp', 90, 200);
  await sleep(300);
  await shot('screenshot-04-wall.png');

  // 3) BUG MALL interior
  await call('dbg_enter');
  await call('dbg_tp', -75, -6060); await sleep(800);
  console.log('in_mall', await call('dbg_mall'), 'px', await call('dbg_px'), 'py', await call('dbg_py'));
  await shot('screenshot-03-mall.png');

  // 5) BANZAI WORLD — enter and stroll into the trees
  await call('dbg_bwenter'); await sleep(600);
  console.log('bwstate', await call('dbg_bwstate'), 'bwmode', await call('dbg_bwmode'));
  await page.keyboard.down('KeyW'); await page.keyboard.down('KeyD');
  await sleep(2400);
  await page.keyboard.up('KeyW'); await page.keyboard.up('KeyD');
  await sleep(300);
  console.log('bw pos', await call('dbg_bwx'), await call('dbg_bwy'));
  await shot('screenshot-05-banzai.png');

  // 6) STORY MODE — Patch Village at the elder
  await call('dbg_bwstory'); await sleep(600);
  await call('dbg_bwtele_npc', 0); await sleep(500);
  console.log('bwstate', await call('dbg_bwstate'), 'bwmode', await call('dbg_bwmode'), 'site', await call('dbg_site'));
  await shot('screenshot-06-story.png');

  // 7) FAR COUNTRY — Neon RAM Flats
  await call('dbg_bwexit'); await sleep(500);
  await poll('dbg_state', [], v => v === 1, 8000);
  await call('dbg_tp', 7500, 0); await sleep(500);
  console.log('region', await call('dbg_region'));
  await page.evaluate(() => {
    const s = window.__TSG;
    s.dbg_spawnbug(7390, -60); s.dbg_spawnbug(7610, 40); s.dbg_spawnbug(7500, 130); s.dbg_spawnbug(7420, 110);
  });
  await sleep(900);
  await shot('screenshot-07-region.png');

  console.log('DONE');
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
