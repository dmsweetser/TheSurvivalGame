// codetest.js — one real issuer code (Daniel's Golden Carcass, Nov 3)
// through the game's own verifier, via the dbg scratch buffer.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const CODE = "0409F-4MMFK-6SM5T-W9SR5-WCN8R-KBZ2H-6WJVD-SMZZ7-RFFV7-E1433-3B07A-RV6A2-06YJB-607MX-EX5XW-67709-C68QA-6GSWT-XN397-50BY4-6M0VQ-80NTV-R3CGZ-7W1G";
const sleep = ms => new Promise(r => setTimeout(r, ms));
(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const redeemVia = code => page.evaluate(c => {
    const ptr = window.__TSG.dbg_scratch();
    const mem = new Uint8Array(window.__TSG.memory.buffer);
    for (let i = 0; i < c.length; i++) mem[ptr + i] = c.charCodeAt(i);
    mem[ptr + c.length] = 0;
    return window.__TSG.dbg_redeem();
  }, code);
  const r = await redeemVia(CODE);
  console.log('dbg_redeem(Daniel code) =', r, r === 0 ? '(SKU 0 Golden Carcass: VALID)' : '(REJECTED)');
  // copy-accident forgiveness (Daniel's live paste showed 143/142 chars):
  const extraEnd = await redeemVia(CODE + '7');
  const extraStart = await redeemVia('X' + CODE);
  const extraMid = await redeemVia(CODE.slice(0, 60) + 'Q' + CODE.slice(60));
  console.log('extra char at end/start/middle =', extraEnd, extraStart, extraMid, '(all must be 0)');
  // but a CHANGED character must still fail — the signature is the receipt
  const chars = CODE.split('');
  chars[30] = chars[30] === 'A' ? 'B' : 'A';
  const tampered = await redeemVia(chars.join(''));
  console.log('tampered (one char changed) =', tampered, '(must be -1)');
  await browser.close();
  process.exit(r === 0 && extraEnd === 0 && extraStart === 0 && extraMid === 0 && tampered === -1 ? 0 : 1);
})().catch(e => { console.error('ERR', e.message); process.exit(2); });
