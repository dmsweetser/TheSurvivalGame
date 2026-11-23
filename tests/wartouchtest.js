// wartouchtest.js — v1.27 (Daniel: "can't tap skirmish on mobile
// in war game"): the SKIRMISH button through the REAL touch path.
// wartest's skirmish checks tap through evt_tap — a debug tap
// injected straight into IN.tapped, bypassing the shell's finger
// routing entirely. The bug lived in that routing: in the STICK
// scheme the SKIRMISH button's drawn rect ([500,628]x[292,320] in
// the 640x360 view, centre (564,306) — war_skirmish_btn) sits
// under the invisible main-game FIRE zone (centre (582,278),
// r38), the zone claimed the touch at finger-down, and a zone-5/6
// touch never synthesizes IN.tapped on release, so the tap
// vanished. The engine fix (platform/wasm.c, exp_touch_point)
// claims no main-game zones while the local player is in WAR! —
// bw_mode 8 draws no touch buttons (bw_draw_touch early-returns),
// so its touches stay plain taps/drags. This suite drives
// touch_point, the exact export the shell calls for real fingers
// (the touchaudit path), with the STICK scheme active, and
// observes only dbg state: a finger tap on the SKIRMISH button
// must open the setup panel (war_state 4), a finger tap on
// LAUNCH must start the skirmish battle (war_state 1, skirmish
// 1), and the battle pause menu's QUIT row takes a finger tap
// back to the map.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (k, ms = 150) => { await page.keyboard.down(k); await sleep(170); await page.keyboard.up(k); await sleep(ms); };
  async function poll(fn, want, timeout = 6000) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { const v = await call(fn); if (want(v)) return v; await sleep(150); }
    return call(fn);
  }
  // a real finger tap (game coords 640x360): down, a short hold
  // well inside the 0.4s tap window, up at the same spot — the
  // touch_point phases the shell's pointer handler sends
  const finger = (id, x, y, phase) => call('touch_point', id, x, y, phase);
  const ftap = async (x, y) => { await finger(9, x, y, 0); await sleep(120); await finger(9, x, y, 2); await sleep(300); };

  await call('set_mobile', 1);   // the touch device class (touchaudit's setup)
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  ok((await call('dbg_state')) === 1, 'run started');
  await call('dbg_adopt', 0);
  ok((await call('dbg_buddy')) !== 0, 'buddy adopted');
  await call('dbg_touchctl', 1);   // the STICK scheme — the scheme the bug lived in
  ok((await call('dbg_touchctl', -1)) === 1, 'control scheme is STICK');

  // the established real path to the WAR! cabinet (wartest's
  // route): mall -> arcade -> cabinet panel -> WAR! map. Entry is
  // keyboard; everything inside WAR! below is finger taps.
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  ok((await poll('dbg_mall', v => v === 1)) === 1, 'in the mall');
  await call('dbg_tp', 350, -5976); await sleep(200);
  await key('KeyE', 250);
  ok((await poll('dbg_inarcade', v => v === 1)) === 1, 'in the arcade');
  await call('dbg_tp', 0, -11758); await sleep(200);   // v1.27: WAR! stand point = (0, -12000+184+58) — the cabinet rose 70px (ARC_ROWY[6] 254 -> 184)
  ok((await call('dbg_cabinet')) === 6, 'standing at the WAR! cabinet', await call('dbg_cabinet'));
  await key('KeyE', 250);
  ok((await poll('dbg_arcgate', v => v === 1)) === 1, 'WAR! cabinet panel opened');
  await key('Space', 300);
  ok((await poll('dbg_bwmode', v => v === 8)) === 8, 'WAR! launched (bw_mode 8)');
  ok((await poll('dbg_warstate', v => v === 0)) === 0, 'on the WAR! world map');
  await sleep(800);   // entry grace, so the map reads taps

  // the SKIRMISH button's drawn rect is [500,628]x[292,320]; its
  // centre (564,306) sits INSIDE the old FIRE zone (dist ~33 < 38)
  // — before the fix this exact tap was claimed by the zone and
  // never landed. (wartest check 26 taps the same centre through
  // evt_tap and passes either way; only the finger path proves it.)
  await ftap(564, 306);
  ok((await poll('dbg_warstate', v => v === 4)) === 4, 'touch: finger tap on the SKIRMISH button opened the setup panel', await call('dbg_warstate'));
  const cv = await page.$('canvas');
  if (cv) await cv.screenshot({ path: __dirname + '/war-touch-setup.png' });

  // LAUNCH is setup row 2: the panel (px=122, py=58, pw=396) puts
  // its rows at ry = py+58+n*38 with h 32 — LAUNCH centre (320,208),
  // the same point wartest's check 26 taps through evt_tap
  await ftap(320, 208);
  ok((await poll('dbg_warstate', v => v === 1)) === 1, 'touch: finger tap on LAUNCH started the battle', await call('dbg_warstate'));
  ok((await call('dbg_warskirmish')) === 1, 'touch: the battle is a skirmish');
  ok((await call('dbg_warhives')) >= 1 && (await call('dbg_warbits')) > 0, 'touch: the skirmish field is live (hive + purse)',
     'hives=' + (await call('dbg_warhives')) + ' bits=' + (await call('dbg_warbits')));

  // the pause menu's rows take finger taps too (same IN.tapped
  // path): QUIT BATTLE is the skirmish pause's third row (320,234),
  // the point wartest's check 28 taps through evt_tap
  await key('Escape', 300);
  ok((await poll('dbg_warpause', v => v === 1)) === 1, 'touch: pause menu up in the skirmish');
  await ftap(320, 234);
  ok((await poll('dbg_warstate', v => v === 0)) === 0, 'touch: finger tap on QUIT BATTLE returned to the map', await call('dbg_warstate'));

  console.log(`\nwartouchtest: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
