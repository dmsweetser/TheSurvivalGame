// playscreenshots2.js — recaptures: wall pile (small hat, offset framing),
// Story Mode Patch Village (no battle overlay), Neon RAM Flats (fresh
// session, the regiontest-proven path). Overwrites playscreens/04, 06, 07.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const OUT = __dirname + '/playscreens';
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function newRun(browser) {
  const page = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 160, wait = 220) => { await page.keyboard.down(code); await sleep(ms); await page.keyboard.up(code); await sleep(wait); };
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
  await sleep(600);
  await key('Enter', 300, 500);
  await poll('dbg_state', [], v => v === 7, 6000);
  await key('Enter', 300, 600);
  await poll('dbg_state', [], v => v === 1, 10000);
  return { page, call, key, poll };
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });

  // ---- session 1: wall + story ----
  {
    const { page, call, key, poll } = await newRun(browser);
    await call('dbg_adopt', 0);

    // WALL: 20 carcasses in, stack of 5 costs 15 -> small hat remains
    for (let i = 0; i < 4; i++) await call('dbg_feed');
    await call('dbg_tp', 500, 500); await sleep(300);
    await key('ArrowDown', 90, 150);
    await key('KeyF', 200, 400);
    console.log('walls', await poll('dbg_walls', [], v => v === 1, 8000));
    for (let want = 2; want <= 5; want++) {
      await key('KeyF', 200, 400);
      const s = await poll('dbg_wallstack', [], v => v >= want, 6000);
      console.log('stack now', s);
      if (s < want) break;
    }
    // stand off to the south-east so the pile stands alone in frame
    await call('dbg_tp', 618, 648); await sleep(250);
    await key('ArrowUp', 90, 200);
    await sleep(300);
    await page.screenshot({ path: OUT + '/screenshot-04-wall.png' });
    console.log('SHOT wall');

    // STORY: veteran buddy so any wild battle is winnable, then Patch Village
    await call('dbg_budup', 40);
    await call('dbg_bwenter'); await sleep(600);
    await call('dbg_bwstory'); await sleep(600);
    await call('dbg_bwtele_site', 0); await sleep(500);
    // fight through any wild battle that grabbed us, then re-teleport
    for (let round = 0; round < 40 && (await call('dbg_bwbattle')) > 0; round++) {
      if ((await call('dbg_bwbattle')) === 1) { await call('dbg_bwchoose', 0); }
      await sleep(350);
    }
    console.log('bwbattle after clear', await call('dbg_bwbattle'));
    await call('dbg_bwtele_site', 0); await sleep(600);
    console.log('story: bwmode', await call('dbg_bwmode'), 'site', await call('dbg_site'), 'battle', await call('dbg_bwbattle'));
    await page.screenshot({ path: OUT + '/screenshot-06-story.png' });
    console.log('SHOT story');
    await page.close();
  }

  // ---- session 2: region (fresh run, teleport straight out) ----
  {
    const { page, call, poll } = await newRun(browser);
    await call('dbg_adopt', 0);
    for (let i = 0; i < 3; i++) await call('dbg_feed');
    await call('dbg_tp', 7500, 0); await sleep(600);
    console.log('region', await poll('dbg_region', [], v => v === 5, 6000), 'mall', await call('dbg_mall'));
    await page.evaluate(() => {
      const s = window.__TSG;
      s.dbg_spawnbug(7390, -70); s.dbg_spawnbug(7615, 50); s.dbg_spawnbug(7505, 140); s.dbg_spawnbug(7415, 120); s.dbg_spawnbug(7590, -110);
    });
    await sleep(1000);
    console.log('region bugs', await call('dbg_bugs'));
    await page.screenshot({ path: OUT + '/screenshot-07-region.png' });
    console.log('SHOT region');
    await page.close();
  }

  console.log('DONE');
  await browser.close();
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
