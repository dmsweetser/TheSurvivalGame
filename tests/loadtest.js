// Save/load spawn (Daniel's rule, Nov 2): a loaded run ALWAYS
// resumes at the centre of the Bug Mall, wherever the save was made.
const { chromium } = require('playwright-core');
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms) => { await page.keyboard.down(code); await sleep(ms || 150); await page.keyboard.up(code); await sleep(150); };
  const fails = [];
  const T = (name, ok) => { console.log((ok ? 'PASS' : 'FAIL') + ' ' + name); if (!ok) fails.push(name); };
  const atMallCentre = async () => {
    const px = await call('dbg_px'), py = await call('dbg_py');
    return Math.abs(px) <= 8 && Math.abs(py - (-6000)) <= 8 && (await call('dbg_mall')) === 1;
  };
  const saveQuitContinue = async () => {
    await key('KeyP', 500);
    await key('ArrowDown', 250);
    await key('ArrowDown', 250);        // rows: CONTINUE / CONTROLS / SAVE + QUIT
    await key('Space', 700);            // SAVE + QUIT TO TITLE
    await key('Space', 600);            // CONTINUE row
    await key('Enter', 900);            // slot 1
    await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  };
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400); await key('Enter', 600);
  await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  // scenario 1: save way out in the world
  await call('dbg_tp', 2000, 1500); await sleep(300);
  await saveQuitContinue();
  T('world save resumes at the mall centre', await atMallCentre());
  // scenario 2: save while inside the mall, off in a corner of it
  await call('dbg_tp', 150, -5950); await sleep(300);
  await saveQuitContinue();
  T('mall-corner save resumes at the mall centre', await atMallCentre());

  // scenario 3 (v1.25 E2E regression): the pause-menu held-key race.
  // A single deliberate 250ms ArrowDown hold must step the selection
  // EXACTLY once. The web shell's old fixed 120ms keyup latch used to
  // stretch the perceived hold past the menu auto-repeat delay, and
  // the repeat stepped a second time up to ~200ms after the keyup —
  // intermittently overshooting SAVE + QUIT in saveQuitContinue above
  // (this suite and nametest reded on it; proven against shipped
  // v1.24, so the game was never the regression — the input seam was).
  // The fix releases keys on the frame boundary in the shell and sets
  // the menu repeat delay to 0.50s in derive_input. A genuine long
  // hold must still auto-repeat: fast navigation is a feature.
  const countSteps = async (holdMs) => {
    await key('KeyP', 300);
    let last = await call('dbg_pausesel'), steps = 0;
    const watch = async (until) => {
      while (Date.now() < until) {
        const s = await call('dbg_pausesel');
        if (s !== last) { steps++; last = s; }
        await sleep(25);
      }
    };
    await page.keyboard.down('ArrowDown');
    await watch(Date.now() + holdMs);
    await page.keyboard.up('ArrowDown');
    await watch(Date.now() + 450);   // a phantom repeat lands in this tail if it exists
    await key('Escape', 200);
    return steps;
  };
  for (let i = 0; i < 3; i++) {
    const n = await countSteps(250);
    T('250ms pause-menu hold #' + (i + 1) + ' steps exactly once', n === 1, 'steps=' + n);
  }
  const rep = await countSteps(1300);
  T('a 1.3s pause-menu hold still auto-repeats', rep >= 3, 'steps=' + rep);
  console.log(fails.length ? 'LOADTEST FAILURES: ' + fails.join(', ') : 'LOADTEST ALL PASS');
  await browser.close(); process.exit(fails.length ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
