// Firewall shield: does it actually recharge? (Nov 2 regression —
// the old per-frame (int)(14*dt+0.5) regen truncated to 0 at any
// normal framerate, so a drained shield never came back.)
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
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400); await key('Enter', 600);
  await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  await call('dbg_setshield', 0);
  T('shield drained to 0', (await call('dbg_shield')) === 0);
  T('shield_max headroom set', (await call('dbg_shieldmax')) >= 60);
  await sleep(3000);
  const s3 = await call('dbg_shield');
  T('shield recharges over time (' + s3 + ' after 3s, expect ~42)', s3 >= 25);
  await sleep(2500);
  const s5 = await call('dbg_shield');
  T('shield keeps climbing toward max (' + s5 + ')', s5 > s3 && s5 <= 60);
  console.log(fails.length ? 'FIREWALLTEST FAILURES: ' + fails.join(', ') : 'FIREWALLTEST ALL PASS');
  await browser.close(); process.exit(fails.length ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
