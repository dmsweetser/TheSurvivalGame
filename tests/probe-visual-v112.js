// Visual verification for the v1.12 fixes (Nov 15): the CART
// grandstand after its clamp/move (track 1 at the grid, where the
// gray wall filled the screen, and FAR cup track 4), and SUPER
// BUDDY RUN on the touch stick scheme, where the world must ride
// on its deck clear of the joystick and JUMP button.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function bootPage(browser, opts) {
  const ctx = await browser.newContext(opts);
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  const poll = async (fn, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  return { ctx, page, call, key, poll };
}

async function enterCart({ call, key, poll }) {
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  await poll('dbg_mall', v => v === 1);
  await call('dbg_tp', 230, -5976); await sleep(200);
  await key('KeyE', 250);
  await poll('dbg_inarcade', v => v === 1, 4000);
  await call('dbg_tp', 256, -11898); await sleep(200);
  await key('KeyE', 250);
  await poll('dbg_arcgate', v => v === 1, 3000);
  await key('Space', 300);
  await poll('dbg_bwmode', v => v === 7, 4000);
  await poll('dbg_crtstate', v => v === 3, 4000);
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });

  // CART: HOME cup race 2 = track 1 (Spaghetti Fields), the grid
  // where the grandstand wall loomed.
  {
    const s = await bootPage(browser, { viewport: { width: 1000, height: 640 } });
    const cv = await s.page.$('canvas');
    await enterCart(s);
    await s.key('Space', 300);
    await s.poll('dbg_crtstate', v => v === 0, 4000);
    await s.call('dbg_crtskip');
    await s.poll('dbg_crtstate', v => v === 4, 15000);
    await s.key('Space', 300);
    await s.poll('dbg_crtstate', v => v === 0, 6000);
    console.log('race2 track', await s.call('dbg_crttrack'));
    await sleep(600);
    await cv.screenshot({ path: __dirname + '/probe2-cart-t1.png' });
    await s.ctx.close();
  }

  // CART: FAR cup race 1 = track 4, entered fresh so the waiting
  // room's DOWN really lands on the FAR cup this time.
  {
    const s = await bootPage(browser, { viewport: { width: 1000, height: 640 } });
    const cv = await s.page.$('canvas');
    await enterCart(s);
    let cup = await s.call('dbg_crtcup');
    for (let i = 0; i < 3 && cup !== 1; i++) { await s.key('ArrowDown', 250); cup = await s.call('dbg_crtcup'); }
    console.log('far cup selected:', cup);
    await s.key('Space', 300);
    await s.poll('dbg_crtstate', v => v === 0, 6000);
    console.log('far race1 track', await s.call('dbg_crttrack'));
    await sleep(600);
    await cv.screenshot({ path: __dirname + '/probe2-cart-t4.png' });
    await s.ctx.close();
  }

  // RUN on the touch stick scheme: the level must sit on its deck,
  // clear of the joystick (bottom-left) and JUMP (bottom-right).
  {
    const s = await bootPage(browser, {
      viewport: { width: 1000, height: 640 }, hasTouch: true, isMobile: true,
      userAgent: 'Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.0 Mobile/15E148 Safari/604.1',
    });
    await s.call('set_mobile', 1);
    await s.call('dbg_touchctl', 1);
    await s.call('dbg_bwentm', 3);
    await s.poll('dbg_bwmode', v => v === 3, 4000);
    await s.page.keyboard.down('ArrowRight'); await sleep(900); await s.page.keyboard.up('ArrowRight');
    await sleep(300);
    const cv = await s.page.$('canvas');
    await cv.screenshot({ path: __dirname + '/probe2-run-mobile.png' });
    console.log('run shot taken, bwmode', await s.call('dbg_bwmode'));
    await s.ctx.close();
  }

  await browser.close();
})();
