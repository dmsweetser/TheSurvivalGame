// WHALES ONLY (Nov 2): signed unlock codes verify in-game against the
// production public key; entitlements persist in the save data; the
// mall counter's code-entry row accepts a pasted code; owned rows
// toggle on/off. Codes come from gen-whale-codes.mjs (issuer's signer).
const { chromium } = require('playwright-core');
const fs = require('fs');
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const { codes, tampered, wrongVersion } = JSON.parse(fs.readFileSync(__dirname + '/whale-codes.json', 'utf8'));
  const browser = await chromium.launch({ executablePath: process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome', args: ['--autoplay-policy=no-user-gesture-required'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  await page.goto('http://localhost:8903/the-survival-game.html');
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const call = (n, ...a) => page.evaluate(([n, a]) => window.__TSG[n](...a), [n, a]);
  const key = async (code, ms) => { await page.keyboard.down(code); await sleep(ms || 150); await page.keyboard.up(code); await sleep(150); };
  const fails = [];
  const T = (name, ok, extra) => { console.log((ok ? 'PASS' : 'FAIL') + ' ' + name + (extra !== undefined ? ' [' + extra + ']' : '')); if (!ok) fails.push(name); };
  const redeemVia = code => page.evaluate(c => {
    const ptr = window.__TSG.dbg_scratch();
    const mem = new Uint8Array(window.__TSG.memory.buffer);
    for (let i = 0; i < c.length; i++) mem[ptr + i] = c.charCodeAt(i);
    mem[ptr + c.length] = 0;
    return window.__TSG.dbg_redeem();
  }, code);
  const walkTo = async (tx, ty, tol) => {
    tol = tol || 12;
    for (let i = 0; i < 60; i++) {
      const x = await call('dbg_px'), y = await call('dbg_py');
      if (Math.abs(x - tx) <= tol && Math.abs(y - ty) <= tol) return true;
      const keys = [];
      if (x < tx - tol) keys.push('ArrowRight'); else if (x > tx + tol) keys.push('ArrowLeft');
      if (y < ty - tol) keys.push('ArrowDown'); else if (y > ty + tol) keys.push('ArrowUp');
      for (const k of keys) await page.keyboard.down(k);
      await sleep(90);
      for (const k of keys) await page.keyboard.up(k);
    }
    return false;
  };

  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  T('starts with no whale unlocks', (await call('dbg_whale')) === 0);

  // --- the crypto path: every sku redeems, junk does not ---
  for (let sku = 0; sku < 15; sku++)
    T('code for sku ' + sku + ' redeems', (await redeemVia(codes[sku])) === sku);
  T('all fifteen owned after redeem', (await call('dbg_whale')) === 32767, await call('dbg_whale'));
  T('all fifteen switched on', (await call('dbg_whaleon')) === 32767);
  T('tampered code rejected', (await redeemVia(tampered)) === -1);
  T('wrong-version code rejected', (await redeemVia(wrongVersion)) === -1);
  T('plain garbage rejected', (await redeemVia('HELLO-WHALE-NOT-A-CODE')) === -1);
  T('lowercase + spaces accepted', (await redeemVia(codes[3].toLowerCase().replace(/-/g, ' '))) === 3);

  // --- persistence: entitlements live in the save data ---
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(600);
  T('unlocks survive a reload (save data)', (await call('dbg_whale')) === 32767, await call('dbg_whale'));

  // --- the shop itself, on a fresh profile: paste a code at the counter ---
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1);
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  T('entered the mall', (await call('dbg_mall')) === 1);
  T('walked to the Whales Only counter', await walkTo(-105, -6148, 14));
  await key('KeyE', 350);
  T('whales panel open', (await call('dbg_state')) === 2 && (await call('dbg_station')) === 1,
    (await call('dbg_state')) + '/' + (await call('dbg_station')));
  for (let i = 0; i < 15; i++) await key('ArrowDown', 110);  // down to ENTER UNLOCK CODE
  await key('Space', 400);                                    // code-entry mode
  await page.evaluate(c => {
    const dt = new DataTransfer(); dt.setData('text/plain', c);
    document.dispatchEvent(new ClipboardEvent('paste', { clipboardData: dt, bubbles: true, cancelable: true }));
  }, codes[0]);
  await sleep(700);                                           // the shell drips chars a few per frame
  await key('Enter', 300);
  await sleep(300);
  T('pasted code unlocked sku 0', ((await call('dbg_whale')) & 1) === 1, await call('dbg_whale'));
  // toggle it off and back on from the counter
  for (let i = 0; i < 15; i++) await key('ArrowUp', 110);
  await key('Space', 400);
  T('owned row toggles OFF', ((await call('dbg_whaleon')) & 1) === 0, await call('dbg_whaleon'));
  await key('Space', 400);
  T('owned row toggles back ON', ((await call('dbg_whaleon')) & 1) === 1, await call('dbg_whaleon'));
  await key('Escape', 300);
  T('panel closed cleanly', (await call('dbg_state')) === 1);

  // --- v1.32: CONFETTI SQUASH must never fire unowned ---------------
  // Daniel's playtest: confetti burst on bug kills he never bought.
  // Root cause found by crafted-save repro: the loader trusted a
  // stored whaleon mask verbatim, so an on-bit with no matching own-
  // bit (a legacy/hand-edited save) fired forever and re-saved
  // itself, and the counter shows unowned rows as buy prompts — no
  // switch exists to turn such a bit off. The loader now clamps
  // on &= own at every border (globals, pl-line, pt token, net join).
  const pollv = async (fn, want, timeout = 6000, ...args) => {
    const t0 = Date.now();
    while (Date.now() - t0 < timeout) { if ((await call(fn, ...args)) === want) return true; await sleep(150); }
    return false;
  };
  const killOne132 = async () => {
    await call('dbg_tp', 0, 58); await sleep(250);
    const px = await call('dbg_px'), py = await call('dbg_py');
    await call('dbg_spawnbug', px + 45, py + 10);
    await sleep(300);
    let bi = 0, best = 1e9;
    for (let i = 0; i < 64; i++) {
      const bx = await call('dbg_bugx', i), by = await call('dbg_bugy', i);
      if (!isFinite(bx) || !isFinite(by)) continue;
      const d = Math.abs(bx - (px + 45)) + Math.abs(by - (py + 10));
      if (d < best) { best = d; bi = i; }
    }
    const k0 = await call('dbg_kills');
    for (let i = 0; i < 14 && (await call('dbg_kills')) === k0; i++) {
      const bx = await call('dbg_bugx', bi), by = await call('dbg_bugy', bi);
      const x = await call('dbg_px'), y = await call('dbg_py');
      const dx = bx - x, dy = by - y;
      const dir = Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? 'ArrowRight' : 'ArrowLeft') : (dy > 0 ? 'ArrowDown' : 'ArrowUp');
      await page.keyboard.down(dir); await sleep(200);
      await key('Space', 110); await key('Space', 110);
      await page.keyboard.up(dir);
    }
    return (await call('dbg_kills')) > k0;
  };
  const exitMall132 = async () => {
    await call('dbg_tp', 0, -5810); await sleep(300);   // the exit mat (MALL_CY+168..+212)
    await key('KeyE', 350);
    return pollv('dbg_mall', 0, 4000);
  };
  const continue132 = async () => {
    for (let i = 0; i < 40; i++) { const a = await call('dbg_menucount'); await sleep(250); const b = await call('dbg_menucount'); if (a === b && a >= 6) break; }
    await key('Space', 400);
    if (await pollv('dbg_state', 7, 4000)) { await key('Enter', 500); await pollv('dbg_state', 1, 8000); }
    return (await call('dbg_state')) === 1;
  };

  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400);
  if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('v132: fresh run started', (await call('dbg_state')) === 1);
  T('v132: fresh-profile kill lands', await killOne132());
  T('v132: second fresh-profile kill lands', await killOne132());
  T('v132: NO confetti burst on a fresh profile', (await call('dbg_party')) === 0, await call('dbg_party'));
  T('v132: player on-mask is 0 on a fresh profile', (await call('dbg_pwon')) === 0, await call('dbg_pwon'));

  // the crafted save: whaleon carries bit 10, whale (owned) does not
  await page.evaluate(() => { const t0 = Date.now(); return new Promise(res => { const iv = setInterval(() => { const s = localStorage.getItem('tsg_save') || ''; if (s.includes('whaleon=') || Date.now() - t0 > 9000) { clearInterval(iv); res(); } }, 300); }); });
  let glob132 = await page.evaluate(() => localStorage.getItem('tsg_save') || '');
  T('v132: autosave wrote the globals blob', /whaleon=\d+/.test(glob132));
  glob132 = glob132.replace(/whaleon=\d+/, 'whaleon=1024');
  await page.evaluate(s => localStorage.setItem('tsg_save', s), glob132);
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(600);
  T('v132: crafted unowned on-bit is clamped at load', (await call('dbg_whaleon')) === 0, await call('dbg_whaleon'));
  T('v132: continue after the crafted save', await continue132());
  T('v132: exited the mall', await exitMall132());
  T('v132: kill after the crafted save lands', await killOne132());
  T('v132: still NO burst with the crafted save', (await call('dbg_party')) === 0, await call('dbg_party'));

  // grant CONFETTI SQUASH (sku 10) for real: the burst must fire
  T('v132: confetti code redeems as sku 10', (await redeemVia(codes[10])) === 10);
  T('v132: grant lands on the player mask', ((await call('dbg_pwon')) & 1024) === 1024, await call('dbg_pwon'));
  const partyBefore = await call('dbg_party');
  T('v132: owned kill lands', await killOne132());
  T('v132: burst fires once owned + on', (await call('dbg_party')) > partyBefore, await call('dbg_party'));

  // switch it off at the counter: the burst must stop
  await call('dbg_tp', 0, 80); await sleep(150);
  await key('KeyE', 300);
  T('v132: back in the mall', (await call('dbg_mall')) === 1);
  T('v132: walked to the Whales Only counter', await walkTo(-105, -6148, 14));
  await key('KeyE', 350);
  T('v132: whales panel open', (await call('dbg_state')) === 2 && (await call('dbg_station')) === 1);
  for (let i = 0; i < 10; i++) await key('ArrowDown', 110);   // row 10 = CONFETTI SQUASH
  await key('Space', 400);
  T('v132: counter toggles confetti OFF', ((await call('dbg_whaleon')) & 1024) === 0, await call('dbg_whaleon'));
  await key('Space', 400);
  T('v132: counter toggles confetti back ON', ((await call('dbg_whaleon')) & 1024) === 1024, await call('dbg_whaleon'));
  await key('Space', 400);
  T('v132: counter toggles confetti OFF again', ((await call('dbg_whaleon')) & 1024) === 0, await call('dbg_whaleon'));
  await key('Escape', 300);
  T('v132: exited the mall for the off-check', await exitMall132());
  const partyOff = await call('dbg_party');
  T('v132: kill with confetti off lands', await killOne132());
  T('v132: NO burst while switched off at the counter', (await call('dbg_party')) === partyOff, await call('dbg_party'));

  // save/load preserves the masks bit-for-bit (own=1024, on=0 here)
  await sleep(6000);   // autosave
  const ownB = await call('dbg_whale'), onB = await call('dbg_whaleon');
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await sleep(600);
  T('v132: save/load preserves the masks bit-for-bit',
    (await call('dbg_whale')) === ownB && (await call('dbg_whaleon')) === onB && ownB === 1024 && onB === 0,
    ownB + '/' + onB);

  console.log(fails.length ? 'WHALETEST FAILURES: ' + fails.join(', ') : 'WHALETEST ALL PASS');
  await browser.close(); process.exit(fails.length ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
