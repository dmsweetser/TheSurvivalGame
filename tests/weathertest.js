// weathertest.js — v1.25 THE LIVING SKY (feature #12): a day/night
// tint over the overworld + Banzai World outdoors, signature weather
// in three regions, Run/Cart skies tinting. The driver is the device
// clock (plat_minutes/plat_daynum); the suite pins the perceived
// clock with dbg_todset/dbg_todday (runtime-only, never saved — the
// feat-6 seam idiom) and holds every pure function to an independent
// JS replica:
//   Phase 0 (title): the tint curve and the sky mix, minute by
//     minute, against the replica; the weather table and wx_state
//     against the replica across regions x epochs, incl. a 40-epoch
//     sweep proving weather changes over time in the signature
//     regions and never appears anywhere else.
//   Phase 1 (fresh run, the plaza): canvas pixels — the meadow at
//     midnight is darker and bluer than at noon, dusk sits between.
//     Unpinning hands the sky back to the device clock.
//   Phase 2: reload agreement — same pinned clock, same weather,
//     same epoch, same darkf (no per-load randomness in the path).
//   Phase 3 (the Mall, entered through the hut door): a floor pixel
//     is IDENTICAL at noon and midnight pins — interiors are
//     structurally untouched.
//   Phase 4 (Entrances, entered through its cabinet): a room pixel
//     is identical across pins — its lighting is its own.
//   Phase 5 (SUPER BUDDY RUN, through its cabinet): the sky pixel
//     darkens at the midnight pin; a ground-tile pixel (median of
//     three) does not move — skies only.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

// ---- independent replicas of the derivations in world.c ----
function hash2(x, y, seed) {
  let h = (seed + Math.imul(x, 374761393) + Math.imul(y, 668265263)) >>> 0;
  h = Math.imul(h ^ (h >>> 13), 1274126177) >>> 0;
  return (h ^ (h >>> 16)) >>> 0;
}
const smooth = p => p * p * (3 - 2 * p);
function darkf(m) {
  if (m < 270) return 1;
  if (m < 390) return 1 - smooth((m - 270) / 120);
  if (m < 1050) return 0;
  if (m < 1170) return smooth((m - 1050) / 120);
  return 1;
}
function warmf(m) {
  if (m >= 270 && m < 390) return Math.sin(Math.PI * (m - 270) / 120);
  if (m >= 1050 && m < 1170) return Math.sin(Math.PI * (m - 1050) / 120);
  return 0;
}
function mixc(a, b, f) { return a.map((v, i) => Math.floor(v + (b[i] - v) * f)); }
function todsky(c, m) {
  let out = c.slice(); const dk = darkf(m), wf = warmf(m);
  if (wf > 0.001) out = mixc(out, [255, 166, 88], 0.22 * wf);
  if (dk > 0.001) out = mixc(out, [16, 20, 50], 0.62 * dk);
  return out;
}
const WX_KIND = [0, 0, 0, 3, 2, 0, 1, 0];
function wxstate(rg, ep) {
  if (!WX_KIND[rg]) return 0;
  const v = (hash2(ep, rg * 7 + 1, 0x5EED) >>> 8) % 4;
  return v < 2 ? 0 : v - 1;
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = async (fn, ...args) => page.evaluate(([f, a]) => window.__TSG[f](...a), [fn, args]);
  const key = async (k, ms = 150) => { await page.keyboard.press(k, { delay: 70 }); await sleep(ms); };
  const poll = async (fn, pred, ms = 15000) => {
    const t0 = Date.now(); let v;
    while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(200); }
    return v;
  };
  // canvas pixels: the page canvas is the 1280x720 (RS=2) blit of
  // the 640x360 stage; a logical point maps x2. A 5x5 logical patch,
  // averaged; pxStable polls until two consecutive reads agree, so
  // a wandering sprite or a drifting cloud can't fake a colour.
  const px = async (lx, ly) => page.evaluate(([x, y]) => {
    const g = document.getElementById('cv').getContext('2d');
    const d = g.getImageData(x * 2 - 5, y * 2 - 5, 10, 10).data;
    let r = 0, gg = 0, b = 0, n = 0;
    for (let i = 0; i < d.length; i += 4) { r += d[i]; gg += d[i + 1]; b += d[i + 2]; n++; }
    return [Math.round(r / n), Math.round(gg / n), Math.round(b / n)];
  }, [lx, ly]);
  const pxStable = async (lx, ly) => {
    let prev = null;
    for (let i = 0; i < 16; i++) {
      const v = await px(lx, ly);
      if (prev && v[0] === prev[0] && v[1] === prev[1] && v[2] === prev[2]) return v;
      prev = v; await sleep(250);
    }
    return prev;
  };
  const pollSave = async (n, needle, ms = 20000) => {
    const t0 = Date.now(); let s = '';
    while (Date.now() - t0 < ms) { s = (await page.evaluate(k => localStorage.getItem(k), 'tsg_save' + n)) || ''; if (s.includes(needle)) return s; await sleep(300); }
    return s;
  };
  const lum = c => c[0] * 0.4 + c[1] * 0.45 + c[2] * 0.15;
  const same = (a, b) => a && b && a[0] === b[0] && a[1] === b[1] && a[2] === b[2];
  // interior equality with a render-noise allowance: lamplight
  // flicker and rounding wobble a static surface by a unit or two,
  // while a real tint leak at the midnight pin would shift it by
  // ~25 a channel — ±2 separates the two decisively
  const sameish = (a, b) => a && b && Math.abs(a[0] - b[0]) <= 2 && Math.abs(a[1] - b[1]) <= 2 && Math.abs(a[2] - b[2]) <= 2;
  const setMin = async m => { await call('dbg_todset', Math.round(m * 10)); await sleep(120); };

  // ================= PHASE 0: the pure model, at the title =====
  ok((await call('dbg_state')) === 0, 'P0: boots to title');
  let curveBad = '';
  for (const m of [0, 100, 269, 270, 300, 330, 360, 389, 390, 720, 1049, 1050, 1080, 1110, 1140, 1169, 1170, 1300, 1439]) {
    await setMin(m);
    const dk = await call('dbg_toddk'), wf = await call('dbg_todwarm'), mn = await call('dbg_todmin');
    if (Math.abs(dk - darkf(m) * 1000) > 3) curveBad += ` dk@${m}=${dk}!=${Math.round(darkf(m) * 1000)}`;
    if (Math.abs(wf - warmf(m) * 1000) > 16) curveBad += ` wf@${m}=${wf}!=${Math.round(warmf(m) * 1000)}`;
    if (Math.abs(mn - m * 10) > 1) curveBad += ` min@${m}=${mn}`;
  }
  ok(!curveBad, 'P0: tint curve matches the replica at 19 minutes of the day', curveBad);
  ok((await call('dbg_toddk')) !== undefined, 'P0: darkf seam live');

  let skyBad = '';
  for (const m of [720, 330, 1110, 0]) {
    await setMin(m);
    for (const c of [[104, 178, 238], [200, 60, 40], [250, 250, 250], [16, 20, 50]]) {
      const packed = await call('dbg_todsky', c[0], c[1], c[2]);
      const got = [(packed >> 16) & 255, (packed >> 8) & 255, packed & 255];
      const want = todsky(c, m);
      for (let i = 0; i < 3; i++) if (Math.abs(got[i] - want[i]) > 2) skyBad += ` sky@${m} ${c} ch${i}: ${got[i]}!=${want[i]}`;
    }
  }
  ok(!skyBad, 'P0: tod_sky mix matches the replica (4 colours x 4 phases)', skyBad);

  let kindBad = '';
  for (let rg = 0; rg < 8; rg++) if ((await call('dbg_wxkind', rg)) !== WX_KIND[rg]) kindBad += ` rg${rg}`;
  ok(!kindBad, 'P0: weather kinds are the signature table (Badlands shimmer, Frozen snow, Jungle rain)', kindBad);
  const epNow = await call('dbg_wxepoch');
  let wxBad = '';
  for (const ep of [0, 1, 2, 3, 7, 1000, epNow])
    for (let rg = 0; rg < 8; rg++)
      if ((await call('dbg_wxat', rg, ep)) !== wxstate(rg, ep)) wxBad += ` rg${rg}@${ep}`;
  ok(!wxBad, 'P0: wx_state matches the replica (8 regions x 7 epochs)', wxBad);
  let sweepBad = '';
  {
    const seen6 = new Set(), seen4 = new Set(), seen3 = new Set();
    for (let ep = 0; ep < 40; ep++) {
      seen6.add(await call('dbg_wxat', 6, ep)); seen4.add(await call('dbg_wxat', 4, ep)); seen3.add(await call('dbg_wxat', 3, ep));
      if ((await call('dbg_wxat', 0, ep)) !== 0) sweepBad += ` meadow@${ep}`;
      if ((await call('dbg_wxat', 5, ep)) !== 0) sweepBad += ` ram@${ep}`;
    }
    if (!(seen6.has(0) && seen6.has(1) && seen6.has(2))) sweepBad += ' jungle lacks a state: ' + [...seen6];
    if (seen4.size < 2) sweepBad += ' frozen never changes';
    if (seen3.size < 2) sweepBad += ' badlands never changes';
  }
  ok(!sweepBad, 'P0: weather changes over time in the signature regions, never appears elsewhere (40 epochs)', sweepBad);
  let curBad = '';
  for (const rg of [3, 4, 6]) if ((await call('dbg_wx', rg)) !== wxstate(rg, epNow)) curBad += ` rg${rg}`;
  ok(!curBad, 'P0: dbg_wx (current epoch) agrees with the replica', curBad);

  // ================= PHASE 1: the plaza, in play ================
  await setMin(720);
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'P1: fresh run started at the plaza');
  await call('dbg_adopt', 0);
  await call('dbg_budup', 60);   // sturdy: the suite idles for minutes of pixel polling, and arc_gate_try refuses a fainted buddy
  await sleep(600);
  const noon = await pxStable(65, 290);
  await setMin(0);
  const night = await pxStable(65, 290);
  ok(lum(night) < lum(noon) - 20, 'P1: the meadow at midnight is well darker than at noon', `noon=${noon} night=${night}`);
  ok((night[2] / Math.max(1, night[0])) > (noon[2] / Math.max(1, noon[0])), 'P1: the night meadow is bluer (b/r rises)', `noon=${noon} night=${night}`);
  await setMin(1110);
  const dusk = await pxStable(65, 290);
  ok(lum(dusk) < lum(noon) - 5 && lum(dusk) > lum(night) + 5, 'P1: dusk sits between noon and midnight', `noon=${noon} dusk=${dusk} night=${night}`);
  await call('dbg_todset', -1);
  { const devMin = await call('dbg_todmin');
    const real = (() => { const d = new Date(); return (d.getHours() * 60 + d.getMinutes()) * 10; })();
    ok(Math.abs(devMin - real) <= 25, 'P1: unpinning hands the sky back to the device clock', `dev=${devMin} real~${real}`); }

  // ================= PHASE 2: reload agreement ==================
  await call('dbg_todday', 21000); await setMin(1110);
  const before = { ep: await call('dbg_wxepoch'), w3: await call('dbg_wx', 3), w4: await call('dbg_wx', 4), w6: await call('dbg_wx', 6), dk: await call('dbg_toddk') };
  ok(before.ep === 21000 * 8 + 6, 'P2: epoch is daynum*8 + minutes/180 under the pin', before.ep);
  // the reload must continue THIS game, buddy and all — wait for
  // the autosave to actually land (bd=1 in the slot blob) instead
  // of assuming the 5s autosave beat the suite's short P1
  const savedBlob = await pollSave(1, 'bd=1');
  ok(savedBlob.includes('bd=1'), 'P2: the autosave landed with the buddy in it');
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await call('dbg_todday', 21000); await setMin(1110);
  await key('Space', 400); await key('Enter', 400);
  ok((await poll('dbg_state', v => v === 1, 8000)) === 1, 'P2: reloaded into play');
  ok((await call('dbg_buddyhp')) > 0, 'P2: the buddy survived the reload (the cabinet panels will want him)');
  const after = { ep: await call('dbg_wxepoch'), w3: await call('dbg_wx', 3), w4: await call('dbg_wx', 4), w6: await call('dbg_wx', 6), dk: await call('dbg_toddk') };
  ok(JSON.stringify(before) === JSON.stringify(after), 'P2: weather, epoch and darkf identical across a reload', `${JSON.stringify(before)} vs ${JSON.stringify(after)}`);

  // ================= PHASE 3: the Mall interior =================
  // a load resumes at the Mall centre (the game's rule), so step
  // OUT through the exit mat first, then back in through the hut
  // door — a genuine entry, not a state we woke up in
  await call('dbg_tp', 0, -5810); await sleep(250); await key('KeyE', 250); await sleep(400);
  ok((await call('dbg_mall')) === 0, 'P3: stepped out of the Mall through the exit mat');
  await call('dbg_tp', 0, 80); await sleep(300); await key('KeyE', 250); await sleep(400);
  ok((await call('dbg_mall')) === 1, 'P3: entered the Mall through the hut door');
  await setMin(720); const mallNoon = await pxStable(320, 60);
  await setMin(0); const mallNight = await pxStable(320, 60);
  ok(sameish(mallNoon, mallNight), 'P3: a Mall floor pixel is identical at noon and midnight pins', `noon=${mallNoon} night=${mallNight}`);

  // ================= PHASE 4: Entrances interior ================
  await call('dbg_tp', 350, -5976); await sleep(200); await key('KeyE', 250);
  ok((await poll('dbg_inarcade', v => v === 1, 4000)) === 1, 'P4: entered the Arcade');
  await call('dbg_budrevive'); await call('dbg_budup', 60);
  ok((await call('dbg_fainted')) === 0, 'P4: buddy is up (the cabinet panel refuses a fainted buddy)');
  await call('dbg_tp', -256, -12108); await sleep(200);
  // verify-retry (the entrancetest door rule): a single press can be
  // eaten between frames under load; press until the panel answers
  let gateOpen = false;
  for (let t = 0; t < 4 && !gateOpen; t++) {
    await key('KeyE', 250);
    gateOpen = (await poll('dbg_arcgate', v => v === 1, 1200)) === 1;
  }
  if (!gateOpen) console.log('  P4 dump: state=' + await call('dbg_state'), 'mall=' + await call('dbg_mall'), 'arc=' + await call('dbg_inarcade'), 'px=' + await call('dbg_px'), 'py=' + await call('dbg_py'), 'faint=' + await call('dbg_fainted'), 'budhp=' + await call('dbg_buddyhp'));
  ok(gateOpen, 'P4: ENTRANCES panel open');
  await key('Space', 300);
  const entIn = (await poll('dbg_bwmode', v => v === 2, 4000)) === 2;
  ok(entIn, 'P4: PLAY launched Entrances');
  await sleep(600);
  await setMin(720); const entNoon = await pxStable(320, 60);
  await setMin(0); const entNight = await pxStable(320, 60);
  ok(entIn && sameish(entNoon, entNight), 'P4: an Entrances room pixel is identical across pins', `noon=${entNoon} night=${entNight}`);
  await key('Escape', 300);
  await poll('dbg_bwstate', v => v === 0, 4000);

  // ================= PHASE 5: Run sky only ======================
  await call('dbg_tp', 0, -12108); await sleep(200);
  let gateOpen5 = false;
  for (let t = 0; t < 4 && !gateOpen5; t++) {
    await key('KeyE', 250);
    gateOpen5 = (await poll('dbg_arcgate', v => v === 1, 1200)) === 1;
  }
  if (!gateOpen5) console.log('  P5 dump: state=' + await call('dbg_state'), 'mall=' + await call('dbg_mall'), 'arc=' + await call('dbg_inarcade'), 'px=' + await call('dbg_px'), 'py=' + await call('dbg_py'), 'faint=' + await call('dbg_fainted'), 'budhp=' + await call('dbg_buddyhp'));
  ok(gateOpen5, 'P5: SUPER BUDDY RUN panel open');
  await key('Space', 300);
  const runIn = (await poll('dbg_bwmode', v => v === 3, 4000)) === 3 && (await poll('dbg_platlvl', v => v >= 1, 4000)) >= 1;
  ok(runIn, 'P5: PLAY launched the Run');
  await sleep(3600);   // let the "LEVEL 1 - ..." note banner clear: it is a dark box over the top of the view, and it fooled the first sky sample at both pins
  const med = async pts => {
    const cols = [];
    for (const [x, y] of pts) cols.push(await pxStable(x, y));
    const ch = i => cols.map(c => c[i]).sort((a, b) => a - b)[1];
    return [ch(0), ch(1), ch(2)];
  };
  await setMin(720);
  const skyNoon = await pxStable(40, 100);   // band 0, below the cloud deck, above the skyline
  const tileNoon = await med([[100, 330], [300, 330], [520, 330]]);
  await setMin(0);
  const skyNight = await pxStable(40, 100);
  const tileNight = await med([[100, 330], [300, 330], [520, 330]]);
  ok(runIn && lum(skyNight) < lum(skyNoon) - 15, 'P5: the Run sky darkens at the midnight pin', `noon=${skyNoon} night=${skyNight}`);
  ok(runIn && same(tileNoon, tileNight), 'P5: the Run ground tiles do not move (skies only)', `noon=${tileNoon} night=${tileNight}`);

  console.log(`\nWEATHERTEST: ${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.error('SUITE ERROR', e); process.exit(2); });
