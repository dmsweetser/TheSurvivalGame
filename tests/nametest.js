// Elven name expansion (Nov 2): 60 names, and saves written by the
// OLD 20-name build must keep the buddy's name — the stored index
// is remapped through the legacy list when the save has no bnv=2.
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
  const surgery = (bnVal) => page.evaluate((bnVal) => {
    for (let i = 0; i < localStorage.length; i++) {
      const k = localStorage.key(i), v = localStorage.getItem(k);
      if (v && v.indexOf('bnv=') >= 0) {
        let nv = v.replace(/ bn=\d+/, ' bn=' + bnVal).replace(/ bnv=2/, '');
        localStorage.setItem(k, nv);
        return nv.indexOf('bnv=') < 0 && nv.indexOf(' bn=' + bnVal) >= 0;
      }
    }
    return false;
  }, bnVal);
  const saveQuitContinue = async () => {
    await key('KeyP', 500);
    await key('ArrowDown', 250);
    await key('Space', 700);
    await key('Space', 600);
    await key('Enter', 900);
    await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  };
  await page.evaluate(() => localStorage.clear());
  await page.reload();
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
  await key('Space', 400); await key('Enter', 600);
  await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  await call('dbg_adopt', 0);   // puppy; dbg_adopt takes the picker draft name (0 = AEGNOR in both lists)
  T('fresh puppy adopts with draft name index 0', (await call('dbg_budname')) === 0);
  await saveQuitContinue();
  T('new-format save round-trips (bnv=2 path)', (await call('dbg_budname')) === 0);
  // forge a LEGACY save: bn=10 meant BELEG in the old list; no bnv.
  // (Reload to the title rather than pause-saving, which would
  // overwrite the forged save with a fresh new-format one.)
  const reloadContinue = async () => {
    await page.reload();
    await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state() === 0, null, { timeout: 20000 });
    await key('Space', 600);
    await key('Enter', 900);
    await page.waitForFunction(() => window.__TSG.dbg_state() === 1, null, { timeout: 10000 });
  };
  T('save surgery applied (bn=10, bnv stripped)', await surgery(10));
  await reloadContinue();
  T('legacy bn=10 (BELEG) migrates to new index 7, not CELEBRIAN', (await call('dbg_budname')) === 7);
  // forge a legacy save naming a Maia (old 13 = SALMAR): not an elf,
  // gone from the list, so the species default applies. A fresh
  // pause-save first, so there is a bnv=2 save to forge from.
  await saveQuitContinue();
  T('save surgery applied (bn=13)', await surgery(13));
  await reloadContinue();
  T('legacy bn=13 (SALMAR, a Maia) falls back to the puppy default', (await call('dbg_budname')) === 7);
  console.log(fails.length ? 'NAMETEST FAILURES: ' + fails.join(', ') : 'NAMETEST ALL PASS');
  await browser.close(); process.exit(fails.length ? 1 : 0);
})().catch(e => { console.error('ERR', e.message); process.exit(1); });
