// craft3test.js — BUDDY CRAFT, third batch (Nov 5): sharks in the
// water, ranged weapons (boomerang, storm zapper), fishing, the dock,
// the bedroll nap, the scarecrow — plus the scrolling icon menu.
const { chromium } = require('playwright-core');
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = (ms) => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

(async () => {
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1300, height: 800 } });
  page.on('pageerror', e => console.log('PAGEERROR:', String(e).slice(0, 200)));
  await page.goto(URL, { waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  // delay:70 keeps the key physically DOWN across at least one frame —
  // under load an instant press can fall between frames and vanish,
  // which desyncs every menu navigation after it.
  const key = async (code, ms = 160) => { await page.keyboard.press(code, { delay: 70 }); await sleep(ms); };
  const poll = async (n, pred, ms = 9000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(n); if (pred(v)) return v; await sleep(120); } return v; };
  const gear = () => call('dbg_craftgear');
  const aimKey = { E: 'ArrowRight', W: 'ArrowLeft', S: 'ArrowDown', N: 'ArrowUp' };
  async function aimAt(dir) { await page.keyboard.down(aimKey[dir]); await sleep(200); await page.keyboard.up(aimKey[dir]); await sleep(120); }
  // ONE segmented list (Nov 6): walk DOWN by row id via dbg_craftrowid,
  // verifying after every press; the list wraps, so a doubled press
  // just costs one more lap. There are no categories to switch to.
  async function gotoRow(id) {
    for (let i = 0; i < 48; i++) { if ((await call('dbg_craftrowid')) === id) return true; await key('ArrowDown', 100); }
    return (await call('dbg_craftrowid')) === id;
  }
  // craft the row and confirm the world changed; a dropped keypress
  // mid-navigation otherwise fails the craft silently and cascades.
  async function craftRow(id, check) {
    for (let r = 0; r < 3; r++) {
      await gotoRow(id);
      await key('Space', 250); await sleep(300);
      if (await check()) return true;
    }
    return false;
  }
  async function openMenu() { await key('KeyF', 250); await sleep(300); }
  async function closeMenu() { await key('Escape', 250); await sleep(300); }

  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  await call('dbg_adopt', 0);
  await call('dbg_bwentm', 4);
  T('entered BUDDY CRAFT (mode 4)', (await poll('dbg_bwmode', v => v === 4)) === 4, await call('dbg_bwmode'));
  await call('dbg_craftsettod', 40);
  await call('dbg_craftsetm', 90 + 90 * 1000 + 90 * 1000000);
  await call('dbg_craftsetcar', 40);
  T('pack stocked', (await call('dbg_craftmat')) === 90090090, await call('dbg_craftmat'));

  // --- tools chain: axe, stone pick, carapace pick, rod ---
  await openMenu();
  T('menu opens on TOOLS', (await call('dbg_craftcat')) === 0, await call('dbg_craftcat'));
  await key('Space', 250); await sleep(250);                       // first row: axe
  T('axe crafted', ((await gear()) & 1) === 1, await gear());
  await gotoRow(1); await key('Space', 250); await sleep(250);     // stone pick
  T('stone pick crafted', ((await gear()) & 2) === 2, await gear());
  await gotoRow(3); await key('Space', 250); await sleep(250);     // carapace pick
  T('carapace pick crafted', ((await gear()) & 32) === 32, await gear());
  await gotoRow(6); await key('Space', 250); await sleep(250);     // fishing rod
  T('fishing rod crafted', ((await gear()) & 16384) === 16384, await gear());

  // --- ranged weapons: boomerang FIRST (the zapper outranks it once
  // owned, so its flight gets checked while it's the only ranged arm) ---
  await gotoRow(11); await key('Space', 250); await sleep(250);     // boomerang
  T('boomerang crafted', ((await gear()) & 32768) === 32768, await gear());
  await closeMenu();
  await aimAt('E');
  await key('Space', 200);
  T('boomerang leaves the hand', (await poll('dbg_craftbmr', v => v === 1, 3000)) === 1, await call('dbg_craftbmr'));
  T('boomerang comes back', (await poll('dbg_craftbmr', v => v === 0, 5000)) === 0, await call('dbg_craftbmr'));
  await openMenu();
  await gotoRow(10); await key('Space', 250); await sleep(250);     // hotfix zapper
  T('zapper crafted', ((await gear()) & 128) === 128, await gear());
  await gotoRow(12); await key('Space', 250); await sleep(250);     // storm zapper
  T('storm zapper crafted', ((await gear()) & 65536) === 65536, await gear());
  await closeMenu();

  // --- find the water: a water tile with a land neighbour ---
  let water = null, shore = null;
  outer:
  for (let tx = -4; tx < 60; tx++) for (let ty = -6; ty < 10; ty++) {
    if ((await call('dbg_craftbiome', tx, ty)) === 5) {
      for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
        const b2 = await call('dbg_craftbiome', tx + dx, ty + dy);
        if (b2 !== 5) { water = { tx, ty }; shore = { tx: tx + dx, ty: ty + dy, dx, dy }; break outer; }
      }
    }
  }
  T('found a shoreline', !!water, water && (water.tx + ',' + water.ty));
  // standing on the shore tile (water + offset), face back INTO the water
  const faceKey = shore.dx === 1 ? 'ArrowLeft' : shore.dx === -1 ? 'ArrowRight' : shore.dy === 1 ? 'ArrowUp' : 'ArrowDown';
  // face the water — and PROVE it: the tile ahead must be water biome
  // before any casting, or the E presses just gather a bush somewhere.
  const fOff = faceKey === 'ArrowLeft' ? [-1, 0] : faceKey === 'ArrowRight' ? [1, 0] : faceKey === 'ArrowUp' ? [0, -1] : [0, 1];
  let aimWater = false;
  for (let r = 0; r < 5 && !aimWater; r++) {
    await call('dbg_bwtp', shore.tx * 48 + 24, shore.ty * 48 + 24); await sleep(250);
    await page.keyboard.down(faceKey); await sleep(200); await page.keyboard.up(faceKey); await sleep(120);
    await call('dbg_bwtp', shore.tx * 48 + 24, shore.ty * 48 + 24); await sleep(200);   // cancel the aim-hold's walk; the aim survives
    aimWater = (await call('dbg_craftbiome', shore.tx + fOff[0], shore.ty + fOff[1])) === 5;
  }
  T('facing the water', aimWater, shore.tx + ',' + shore.ty);

  // --- fishing: cast until a FISH bites. The lake also gives fiber and
  // carapace (an earlier loop stopped at ANY catch and then demanded a
  // fish — a 40% flake). Only the fish ends the loop now. ---
  const mat0 = await call('dbg_craftmat');
  let caught = false;
  for (let i = 0; i < 40 && !caught; i++) {
    await key('KeyE', 200); await sleep(450);
    if ((await call('dbg_craftfish')) >= 1) caught = true;
  }
  T('fishing yields (fiber, shell, or fish)', caught || (await call('dbg_craftmat')) !== mat0, 'mat moved: ' + ((await call('dbg_craftmat')) !== mat0));
  T('a lake fish in the pack', (await call('dbg_craftfish')) >= 1, await call('dbg_craftfish'));

  // --- the dock: the one build that wants the water ---
  const builds0 = await call('dbg_craftbuilds');
  await openMenu();
  await craftRow(21, async () => (await call('dbg_craftbuilds')) === builds0 + 1);   // DOCK
  T('dock built on the water', (await call('dbg_craftbuilds')) === builds0 + 1, await call('dbg_craftbuilds'));
  T('dock is kind 7', (await call('dbg_craftbuildk', builds0)) === 7, await call('dbg_craftbuildk', builds0));
  await closeMenu();
  // walk out onto it (no raft yet): the dock holds him, open water
  // wouldn't. Pulse the key and poll the tile — a single long hold
  // overshoots the 48px tile whenever headless frame timing runs fast.
  let onDock = false;
  for (let i = 0; i < 16 && !onDock; i++) {
    await page.keyboard.down(faceKey); await sleep(150); await page.keyboard.up(faceKey); await sleep(120);
    const bx2 = await call('dbg_bwx'), by2 = await call('dbg_bwy');
    onDock = Math.floor(bx2 / 48) === water.tx && Math.floor(by2 / 48) === water.ty;
  }
  const bx = await call('dbg_bwx'), by = await call('dbg_bwy');
  T('buddy stands on the dock tile', onDock, bx + ',' + by);

  // --- back at the clearing: campfire, raft, sail ---
  await call('dbg_bwtp', 0, 56); await sleep(300);
  await openMenu();
  await craftRow(18, async () => ((await gear()) & 2048) === 2048);   // campfire at his feet
  T('campfire built', ((await gear()) & 2048) === 2048, await gear());
  await craftRow(4, async () => ((await gear()) & 512) === 512);     // raft
  T('raft crafted', ((await gear()) & 512) === 512, await gear());
  await craftRow(7, async () => ((await gear()) & 262144) === 262144);   // sail raft
  T('sail raft crafted', ((await gear()) & 262144) === 262144, await gear());
  await closeMenu();

  // --- sharks: take the raft out and wait for teeth ---
  await call('dbg_bwtp', water.tx * 48 + 24, water.ty * 48 + 24); await sleep(400);
  const hpFull = await call('dbg_buddyhp');
  let sharks = await poll('dbg_craftsharks', v => v >= 1, 16000);
  if (sharks < 1) {   // a pond too small to spawn in: find open water
    let deep = null;
    for (let tx = -4; tx < 80 && !deep; tx++) for (let ty = -8; ty < 14 && !deep; ty++) {
      if ((await call('dbg_craftbiome', tx, ty)) === 5 &&
          (await call('dbg_craftbiome', tx + 3, ty)) === 5 && (await call('dbg_craftbiome', tx - 3, ty)) === 5 &&
          (await call('dbg_craftbiome', tx, ty + 3)) === 5 && (await call('dbg_craftbiome', tx, ty - 3)) === 5)
        deep = { tx, ty };
    }
    if (deep) { await call('dbg_bwtp', deep.tx * 48 + 24, deep.ty * 48 + 24); await sleep(400); sharks = await poll('dbg_craftsharks', v => v >= 1, 16000); }
  }
  T('a shark finds the raft', sharks >= 1, sharks);
  const hpAfter = await poll('dbg_buddyhp', v => v < hpFull, 25000);
  T('the shark BITES (hp drops)', hpAfter < hpFull, hpFull + ' -> ' + hpAfter);

  // --- the bedroll nap: time passes, the night does not skip ---
  // stance notes: (1) aimAt's key-hold WALKS the buddy (headless dt
  // makes the walk long), and placement refuses within 23px of the
  // target tile — so aim first, then teleport onto the tile-centre
  // stance; the aim survives the teleport and the menu freezes drift.
  // (2) the stance is FAR from the suite's own campfire: a fire within
  // 84px outranks the bedroll in the rest chain and dawn-skips instead
  // (that mistake cost a run: tod read 239, the dawn-skip, not ~50).
  // stance, THIRD version: the world seed is random per run, so the old
  // hardcoded stance (-264,72) rotted — some worlds put water or a node
  // on its west tile and the bedroll could never land (two straight
  // failures, Nov 6). Scan for a stance like the fishing section does:
  // land, with a clear land tile to the WEST, well west of the campfire
  // (a fire within 84px outranks the bedroll in the rest chain).
  let stance = null;
  for (let tx = -14; tx < -3 && !stance; tx++) for (let ty = -4; ty < 6 && !stance; ty++) {
    if ((await call('dbg_craftbiome', tx, ty)) !== 5 && (await call('dbg_craftbiome', tx - 1, ty)) !== 5 &&
        (await call('dbg_craftnode', tx - 1, ty)) === 0) stance = { tx, ty };
  }
  T('found a bedroll stance', !!stance, stance && (stance.tx + ',' + stance.ty));
  await aimAt('W');
  await call('dbg_bwtp', stance.tx * 48 + 24, stance.ty * 48 + 24); await sleep(200);
  const builds1 = await call('dbg_craftbuilds');
  await openMenu();
  await craftRow(22, async () => (await call('dbg_craftbuilds')) === builds1 + 1);   // BEDROLL
  T('bedroll placed', (await call('dbg_craftbuilds')) === builds1 + 1, await call('dbg_craftbuilds'));
  await closeMenu();
  await call('dbg_craftsettod', 200);
  await key('KeyE', 250);
  const todNap = await poll('dbg_crafttod', v => v > 20 && v < 120, 9000);
  T('nap wakes mid-morning (tod ~50, not dawn-skipped)', todNap > 20 && todNap < 120, todNap);

  // --- the scarecrow stands watch (DEFENSE) ---
  // the scarecrow gets the same adaptive treatment, east side: land
  // with a clear land tile to the EAST, clear of bedroll and fire.
  let stance2 = null;
  for (let tx = 4; tx < 14 && !stance2; tx++) for (let ty = -4; ty < 6 && !stance2; ty++) {
    if ((await call('dbg_craftbiome', tx, ty)) !== 5 && (await call('dbg_craftbiome', tx + 1, ty)) !== 5 &&
        (await call('dbg_craftnode', tx + 1, ty)) === 0) stance2 = { tx, ty };
  }
  T('found a scarecrow stance', !!stance2, stance2 && (stance2.tx + ',' + stance2.ty));
  await aimAt('E');
  await call('dbg_bwtp', stance2.tx * 48 + 24, stance2.ty * 48 + 24); await sleep(200);
  const builds2 = await call('dbg_craftbuilds');
  await openMenu();
  await craftRow(27, async () => (await call('dbg_craftbuilds')) === builds2 + 1);   // SCARECROW
  T('scarecrow built', (await call('dbg_craftbuilds')) === builds2 + 1, await call('dbg_craftbuilds'));
  T('scarecrow is kind 9', (await call('dbg_craftbuildk', builds2)) === 9, await call('dbg_craftbuildk', builds2));
  await closeMenu();

  console.log('craft3test: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('ERR', e); process.exit(1); });
