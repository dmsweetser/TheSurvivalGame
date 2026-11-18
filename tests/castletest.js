// castletest.js — CASTLE (Nov 14): the fifth arcade cabinet, a clone
// of Castle Adventure (1984), the DOS game Krista loves. Trapped in a
// castle: 20 rooms, 13 treasures, bump combat with a sword, a gate
// key in the dungeon, and the gate in the starting courtyard.
// Max score 1550, same as the original.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
const sleep = ms => new Promise(r => setTimeout(r, ms));
let pass = 0, fail = 0;
function T(name, cond, extra) { if (cond) { pass++; } else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); } }

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });
  const page = await browser.newPage({ viewport: { width: 1000, height: 640 } });
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  const call = (fn, ...a) => page.evaluate(([f, x]) => window.__TSG[f](...x), [fn, a]);
  const key = async (code, ms = 150) => { await page.keyboard.down(code); await sleep(170); await page.keyboard.up(code); await sleep(ms); };
  const poll = async (fn, pred, ms = 8000) => { const t0 = Date.now(); let v; while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) return v; await sleep(120); } return v; };
  // hold a direction until a dbg value satisfies pred (the real player
  // path: the buddy walks, doors trigger on crossing)
  const walkUntil = async (code, fn, pred, ms = 9000) => {
    await page.keyboard.down(code);
    const t0 = Date.now(); let v;
    while (Date.now() - t0 < ms) { v = await call(fn); if (pred(v)) break; await sleep(100); }
    await page.keyboard.up(code); await sleep(150);
    return v;
  };
  // stand on the E/W door row (ly ~194) / the N/S door column (lx ~320),
  // approaching from whichever side the buddy is on. For the column,
  // drop to the south lane first so an east/west walk cannot blunder
  // out of a side door.
  const gotoRow = async () => {
    const y = await call('dbg_casy');
    if (y < 180) await walkUntil('ArrowDown', 'dbg_casy', v => v >= 184, 5000);
    else if (y > 208) await walkUntil('ArrowUp', 'dbg_casy', v => v <= 204, 5000);
  };
  const gotoCol = async () => {
    const x = await call('dbg_casx');
    if (x >= 300 && x <= 340) return;
    await walkUntil('ArrowDown', 'dbg_casy', v => v >= 292, 6000);
    if (x < 300) await walkUntil('ArrowRight', 'dbg_casx', v => v >= 306, 6000);
    else await walkUntil('ArrowLeft', 'dbg_casx', v => v <= 334, 6000);
  };
  await page.goto(URL);
  await page.evaluate(() => localStorage.clear());
  await page.reload({ waitUntil: 'load' });
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  await key('Space', 400); if ((await call('dbg_state')) !== 1) await key('Enter', 400);
  T('run started', (await call('dbg_state')) === 1, await call('dbg_state'));
  await call('dbg_adopt', 0);
  T('buddy adopted', (await call('dbg_buddy')) !== 0, await call('dbg_buddy'));

  // mall -> arcade -> the CASTLE cabinet (the new fifth box)
  await call('dbg_tp', 0, 80); await sleep(200);
  await key('KeyE', 300);
  T('in the mall', (await poll('dbg_mall', v => v === 1)) === 1, await call('dbg_mall'));
  await call('dbg_tp', 230, -5976); await sleep(200);
  await key('KeyE', 250);
  T('entered the arcade', (await poll('dbg_inarcade', v => v === 1, 4000)) === 1, await call('dbg_inarcade'));
  await call('dbg_tp', 0, -11898); await sleep(200);
  T('standing at the CASTLE cabinet', (await call('dbg_cabinet')) === 4, await call('dbg_cabinet'));
  await key('KeyE', 250);
  T('castle cabinet panel opened', (await poll('dbg_arcgate', v => v === 1, 3000)) === 1, await call('dbg_arcgate'));
  await key('Space', 300);
  T('CASTLE launched (bw_mode 6)', (await poll('dbg_bwmode', v => v === 6, 4000)) === 6, await call('dbg_bwmode'));
  T('starts in the courtyard, empty-handed', (await call('dbg_casroom')) === 17 && (await call('dbg_casitems')) === 0 && (await call('dbg_casscore')) === 0,
    'room=' + (await call('dbg_casroom')) + ' items=' + (await call('dbg_casitems')));

  // north through the door into THE CROSSING (room 12, two snakes)
  await walkUntil('ArrowUp', 'dbg_casroom', v => v === 12);
  T('walked north into the crossing', (await call('dbg_casroom')) === 12, await call('dbg_casroom'));
  T('two snakes in the crossing', (await call('dbg_casmon')) === 2, await call('dbg_casmon'));
  // back south to the courtyard
  await walkUntil('ArrowDown', 'dbg_casroom', v => v === 17);
  T('back in the courtyard', (await call('dbg_casroom')) === 17, await call('dbg_casroom'));

  // west to THE WELL ROOM (16) for its treasure
  await gotoRow();
  await walkUntil('ArrowLeft', 'dbg_casroom', v => v === 16);
  T('west into the well room', (await call('dbg_casroom')) === 16, await call('dbg_casroom'));
  // its treasure sits at (470, 262): walk to it and take it by contact
  await walkUntil('ArrowLeft', 'dbg_casx', v => v <= 480, 5000);
  await walkUntil('ArrowDown', 'dbg_castreasures', v => v === 1, 6000);
  T('first treasure taken (+100)', (await call('dbg_castreasures')) === 1 && (await call('dbg_casscore')) === 100,
    'tr=' + (await call('dbg_castreasures')) + ' score=' + (await call('dbg_casscore')));

  // east back to the courtyard; the sword comes via the armory in real
  // play — the suite takes the dbg shortcut for the combat checks
  await gotoRow();
  await walkUntil('ArrowRight', 'dbg_casroom', v => v === 17);
  T('back in the courtyard again', (await call('dbg_casroom')) === 17, await call('dbg_casroom'));
  await call('dbg_casgive', 1);
  T('sword in hand (dbg)', ((await call('dbg_casitems')) & 1) === 1, await call('dbg_casitems'));

  // combat: north to the crossing and let a snake come to the sword
  await gotoCol();
  await walkUntil('ArrowUp', 'dbg_casroom', v => v === 12);
  const s0 = await call('dbg_casscore');
  await poll('dbg_casscore', v => v >= s0 + 10, 15000);
  T('a snake died on the sword (+10)', (await call('dbg_casscore')) >= s0 + 10, 'score=' + (await call('dbg_casscore')));
  T('one snake left in the crossing', (await poll('dbg_casmon', v => v === 1, 4000)) === 1, await call('dbg_casmon'));

  // the gate without the key: locked. with the key (dbg): freedom.
  await gotoCol();
  await walkUntil('ArrowDown', 'dbg_casroom', v => v === 17);
  await walkUntil('ArrowDown', 'dbg_casy', v => v >= 316, 6000);
  await sleep(500);
  T('the gate stays locked without the key', ((await call('dbg_casitems')) & 4) === 0 && (await call('dbg_bwmode')) === 6,
    'items=' + (await call('dbg_casitems')) + ' mode=' + (await call('dbg_bwmode')));
  const xpCas = await call('dbg_buddyxp');   // v1.19: the buddy's XP at the brink of the escape
  await call('dbg_casgive', 2);
  await sleep(700);
  T('the key opens the gate (escape!)', ((await call('dbg_casitems')) & 4) === 4,
    'items=' + (await call('dbg_casitems')));
  T('escape ends the run', (await poll('dbg_bwmode', v => v === -1, 6000)) === -1, await call('dbg_bwmode'));
  T('back in the arcade at the castle cabinet', (await call('dbg_inarcade')) === 1 && Math.abs((await call('dbg_px')) - 0) < 60,
    'arc=' + (await call('dbg_inarcade')) + ' px=' + (await call('dbg_px')));
  const best = await call('dbg_casbest');
  T('the escape score is the saved best', best >= 210, 'best=' + best);
  T('v1.19: the escape paid the buddy +50 XP', (await call('dbg_buddyxp')) === xpCas + 50, 'xp ' + xpCas + ' -> ' + (await call('dbg_buddyxp')));

  // relaunch: v1.22 (Daniel, Nov 17) — the exit was never meant to
  // be key-gated. Escape leaves the castle with NO key at all; the
  // key gates only the courtyard gate (the win, covered above).
  await call('dbg_tp', 0, -11898); await sleep(200);
  await key('KeyE', 250);
  await key('Space', 300);
  T('CASTLE relaunched', (await poll('dbg_bwmode', v => v === 6, 4000)) === 6, await call('dbg_bwmode'));
  await key('Escape', 400);
  T('v1.22: Escape leaves WITHOUT the key', (await poll('dbg_bwmode', v => v === -1, 4000)) === -1, await call('dbg_bwmode'));
  T('v1.22: ...and lands back in the arcade', (await call('dbg_inarcade')) === 1, await call('dbg_inarcade'));
  // and with the key in hand, Escape still leaves, as it always did
  await call('dbg_tp', 0, -11898); await sleep(200);
  await key('KeyE', 250);
  await key('Space', 300);
  T('CASTLE relaunched again', (await poll('dbg_bwmode', v => v === 6, 4000)) === 6, await call('dbg_bwmode'));
  await call('dbg_casgive', 2);
  await key('Escape', 400);
  T('with the key in hand, Escape leaves', (await poll('dbg_bwmode', v => v === -1, 4000)) === -1, await call('dbg_bwmode'));
  T('...and the buddy did not faint', (await call('dbg_inarcade')) === 1, await call('dbg_inarcade'));

  console.log('castletest: ' + pass + ' passed, ' + fail + ' failed');
  await browser.close();
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('ERR', String(e).slice(0, 300)); process.exit(1); });
