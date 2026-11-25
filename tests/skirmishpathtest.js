// skirmishpathtest.js — v1.25 SKIRMISH FIX: WAR! skirmish must be
// reachable on the REAL player path for every device class.
// wartest's skirmish checks enter through evt_tap (a debug tap
// injected at game coordinates), which bypasses the shell's real
// input paths entirely — and on a gamepad there was NO door at
// all: the setup panel opened only on a typed 'S' or a tap, the
// pad cursor never runs on the world map, and A/B on the map are
// the region launch. A pad player could invade every region and
// never once skirmish (Daniel's report, Nov 18). The fix makes
// the SKIRMISH button the map selection's 9th stop: the stick,
// dpad and arrows land on it like any region, and the confirm
// verb opens the panel. This suite drives REAL events only —
// fake-pad buttons, physical key presses, real mouse clicks at
// the button's true screen position. dbg exports only OBSERVE.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function newPage(browser) {
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 }, hasTouch: true });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.addInitScript(() => {
    const buttons = [];
    for (let i = 0; i < 18; i++) buttons.push({ pressed: false, touched: false, value: 0 });
    window.__FAKEPAD = { id: 'Fake Standard Pad', index: 0, connected: true, mapping: 'standard',
                         axes: [0, 0, 0, 0], buttons, timestamp: 0 };
    navigator.getGamepads = () => [window.__FAKEPAD, null, null, null];
  });
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  return page;
}
const call = (page, fn, ...args) => page.evaluate(([f, a]) => window.__TSG[f](...a), [fn, args]);
async function poll(page, fn, want, timeout = 6000) {
  const t0 = Date.now();
  while (Date.now() - t0 < timeout) { const v = await call(page, fn); if (want(v)) return v; await sleep(150); }
  return call(page, fn);
}
async function key(page, k, ms = 150) { await page.keyboard.down(k); await sleep(170); await page.keyboard.up(k); await sleep(ms); }

// the established real path to the WAR! cabinet (wartest's route):
// title -> run -> mall -> arcade -> cabinet panel -> WAR! map
async function enterWarMap(page) {
  await key(page, 'Space', 400);
  if ((await call(page, 'dbg_state')) !== 1) await key(page, 'Enter', 400);
  if ((await poll(page, 'dbg_state', v => v === 1)) !== 1) return false;
  await call(page, 'dbg_adopt', 0);
  await call(page, 'dbg_tp', 0, 80); await sleep(200);
  await key(page, 'KeyE', 300);
  if ((await poll(page, 'dbg_mall', v => v === 1)) !== 1) return false;
  await call(page, 'dbg_tp', 350, -5976); await sleep(200);
  await key(page, 'KeyE', 250);
  if ((await poll(page, 'dbg_inarcade', v => v === 1)) !== 1) return false;
  await call(page, 'dbg_tp', 0, -11758); await sleep(200);   // v1.27: WAR! stand point = (0, -12000+184+58) — the cabinet rose 70px (ARC_ROWY[6] 254 -> 184)
  if ((await call(page, 'dbg_cabinet')) !== 6) return false;
  await key(page, 'KeyE', 250);
  if ((await poll(page, 'dbg_arcgate', v => v === 1)) !== 1) return false;
  await key(page, 'Space', 300);
  if ((await poll(page, 'dbg_bwmode', v => v === 8)) !== 8) return false;
  if ((await poll(page, 'dbg_warstate', v => v === 0)) !== 0) return false;
  await sleep(700);   // entry grace
  return true;
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });

  // ============ PAD: the device class that had no door ============
  {
    const page = await newPage(browser);
    const padBtn = (i, down) => page.evaluate(([n, d]) => { const b = window.__FAKEPAD.buttons[n]; b.pressed = d; b.value = d ? 1 : 0; }, [i, down]);
    const padTap = async (i, holdMs = 130) => { await padBtn(i, true); await sleep(holdMs); await padBtn(i, false); await sleep(150); };
    ok(await enterWarMap(page), 'pad: reached the WAR! world map on the real path');
    // selection starts on region 1 (stop 0); one UP edge wraps it
    // to the 9th stop — the SKIRMISH button
    await padTap(12);   // DUp
    await page.screenshot({ path: 'tsg-v125-fskirm-pad-map.png' });
    await padTap(0);    // A: confirm the stop
    ok((await poll(page, 'dbg_warstate', v => v === 4)) === 4, 'pad: SKIRMISH stop + A opened the setup panel');
    await padTap(1);    // B: backs out of the panel
    ok((await poll(page, 'dbg_warstate', v => v === 0)) === 0, 'pad: B backed out of the panel to the map');
    await sleep(500);   // map grace after backing out
    await padTap(0);    // A again: the selection is still on SKIRMISH
    ok((await poll(page, 'dbg_warstate', v => v === 4)) === 4, 'pad: A reopened the panel from the SKIRMISH stop');
    await padTap(13); await padTap(13); await padTap(13);   // DDown x3 -> LAUNCH row (v1.33: CLANS joined the panel — LAUNCH is row 3 now, was 2)
    await padTap(0);    // A: LAUNCH
    ok((await poll(page, 'dbg_warstate', v => v === 1)) === 1, 'pad: LAUNCH started the battle');
    ok((await call(page, 'dbg_warskirmish')) === 1, 'pad: the battle is a skirmish');
    ok((await call(page, 'dbg_warbits')) > 0 && (await call(page, 'dbg_warhives')) >= 1,
       'pad: the skirmish field is live (purse + hive)',
       (await call(page, 'dbg_warbits')) + '/' + (await call(page, 'dbg_warhives')));
    await page.close();
  }

  // ============ KEYBOARD: the new stop + the old 'S' door ============
  {
    const page = await newPage(browser);
    ok(await enterWarMap(page), 'kb: reached the WAR! world map on the real path');
    await page.keyboard.press('KeyS');
    ok((await poll(page, 'dbg_warstate', v => v === 4)) === 4, "kb: 'S' still opens the setup panel");
    await page.keyboard.press('Escape');
    ok((await poll(page, 'dbg_warstate', v => v === 0)) === 0, 'kb: Esc closed the panel back to the map');
    await sleep(500);
    // a fresh map's selection sits on region 1 (stop 0) — the 'S'
    // door never moved it. One UP edge wraps it backward onto the
    // 9th stop, the SKIRMISH button; Space confirms it.
    await key(page, 'ArrowUp');     // 0 -> 8 (SKIRMISH, wrapping)
    await page.keyboard.press('Space');
    ok((await poll(page, 'dbg_warstate', v => v === 4)) === 4, 'kb: arrows land on the SKIRMISH stop, Space opens the panel');
    await key(page, 'ArrowDown'); await key(page, 'ArrowDown'); await key(page, 'ArrowDown');   // -> LAUNCH (row 3 since v1.33's CLANS row)
    await page.keyboard.press('Space');
    ok((await poll(page, 'dbg_warstate', v => v === 1)) === 1, 'kb: LAUNCH started the battle');
    ok((await call(page, 'dbg_warskirmish')) === 1, 'kb: the battle is a skirmish');
    await page.close();
  }

  // ============ MOUSE: the real click path (guard) ============
  {
    const page = await newPage(browser);
    ok(await enterWarMap(page), 'mouse: reached the WAR! world map on the real path');
    const box = await page.evaluate(() => {
      const r = document.querySelector('canvas').getBoundingClientRect();
      return { left: r.left, top: r.top, width: r.width, height: r.height };
    });
    const gameXY = (gx, gy) => [box.left + gx / 640 * box.width, box.top + gy / 360 * box.height];
    const [bx, by] = gameXY(564, 306);   // the SKIRMISH button's centre, where it is drawn
    await page.mouse.click(bx, by);
    ok((await poll(page, 'dbg_warstate', v => v === 4)) === 4, 'mouse: a real click on the SKIRMISH button opened the panel');
    const [lx, ly] = gameXY(320, 217);   // LAUNCH row centre (panel grew a row in v1.33: centre 208 -> 217)
    await page.mouse.click(lx, ly);
    ok((await poll(page, 'dbg_warstate', v => v === 1)) === 1, 'mouse: LAUNCH click started the battle');
    ok((await call(page, 'dbg_warskirmish')) === 1, 'mouse: the battle is a skirmish');
    await page.close();
  }

  console.log(`\nSKIRMISHPATHTEST: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 400)); process.exit(1); });
