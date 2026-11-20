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

  console.log(fails.length ? 'WHALETEST FAILURES: ' + fails.join(', ') : 'WHALETEST ALL PASS');
  await browser.close(); process.exit(fails.length ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
