// Wall shared health pool, asserted exactly (Nov 15): Daniel's rule
// is that connected walls share ONE pool worth 1.1x the sum of their
// bases, INCLUDING all reinforcement — a fresh layer arrives at full
// strength even in a damaged run, and a wall carries its real health
// into a run, never a stale record of it. Walls are placed/damaged
// through the dbg instrumentation (fixed 100hp layers) so the math
// can be checked to the point; the real F-key path is covered by
// walltest.js. Each scenario gets a fresh browser profile.
const { chromium } = require('playwright-core');
const EXE = process.env.HOME + '/.cache/ms-playwright/chromium-1248/chrome-linux64/chrome';
const URL = 'http://localhost:8903/the-survival-game.html';
let pass = 0, fail = 0;
function ok(cond, name, extra) {
  if (cond) { pass++; console.log('PASS', name); }
  else { fail++; console.log('FAIL', name, extra !== undefined ? '-> ' + extra : ''); }
}
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function boot(browser) {
  const ctx = await browser.newContext({ viewport: { width: 1000, height: 640 } });
  const page = await ctx.newPage();
  page.on('pageerror', e => console.log('PAGEERROR', e.message));
  await page.goto(URL);
  await page.waitForFunction(() => window.__TSG && window.__TSG.dbg_state, null, { timeout: 20000 });
  const T = await page.evaluateHandle(() => window.__TSG);
  const call = async (fn, ...args) => page.evaluate(([t, f, a]) => t[f](...a), [T, fn, args]);
  await page.keyboard.press('Space'); await sleep(500);
  if ((await call('dbg_state')) !== 1) { await page.keyboard.press('Enter'); await sleep(500); }
  return { ctx, page, call };
}

(async () => {
  const browser = await chromium.launch({ executablePath: EXE, args: ['--no-sandbox'] });

  // S1+S2+S3: a fresh pair pools at 1.1x; damage drains BOTH walls;
  // the chewed segment gives way at its share and the survivor keeps
  // its own share of what was left.
  {
    const { ctx, call } = await boot(browser);
    const A = await call('dbg_wallplace', 0, 400, 1);
    const B = await call('dbg_wallplace', 56, 400, 1);
    ok(A >= 0 && B >= 0 && A !== B, 'S1 two walls placed');
    ok((await call('dbg_grpnof', A)) === 2, 'S1 the pair forms one run of 2', await call('dbg_grpnof', A));
    ok((await call('dbg_capof', A)) === 2200, 'S1 pool cap is 1.1x the sum (220)', await call('dbg_capof', A));
    ok((await call('dbg_poolof', A)) === 2200, 'S1 fresh pool is full (220)', await call('dbg_poolof', A));
    await call('dbg_walldmg', A, 100);
    ok((await call('dbg_poolof', A)) === 1200, 'S2 chewing one wall drains the shared pool (120 left)', await call('dbg_poolof', A));
    const ha = await call('dbg_wallhpof', A), hb = await call('dbg_wallhpof', B);
    ok(ha >= 520 && ha <= 570 && hb >= 520 && hb <= 570, 'S2 both walls wear the damage equally (~54.5 each)', ha + '/' + hb);
    ok((await call('dbg_walls')) === 2, 'S2 nothing breaks before a full share is chewed');
    await call('dbg_walldmg', A, 30);
    ok((await call('dbg_wallhpof', A)) === -1, 'S3 the chewed segment gives way past its share (110)');
    ok((await call('dbg_walls')) === 1, 'S3 the other segment still stands');
    const hb2 = await call('dbg_wallhpof', B);
    ok(hb2 >= 380 && hb2 <= 470, 'S3 the survivor keeps its share of the pool (~41)', hb2);
    await ctx.close();
  }

  // S4: when the run's LEADER breaks, the survivors stay pooled at
  // exactly the remaining fraction (the old code recounted the
  // group first and dropped them out of the share carry).
  {
    const { ctx, call } = await boot(browser);
    const A = await call('dbg_wallplace', 0, 400, 1);
    const B = await call('dbg_wallplace', 56, 400, 1);
    await call('dbg_wallplace', 112, 400, 1);
    ok((await call('dbg_capof', A)) === 3300, 'S4 run of 3 caps at 330', await call('dbg_capof', A));
    await call('dbg_walldmg', A, 115);
    ok((await call('dbg_wallhpof', A)) === -1, 'S4 leader segment breaks at its share');
    ok((await call('dbg_grpnof', B)) === 2, 'S4 survivors remain a run of 2', await call('dbg_grpnof', B));
    const p = await call('dbg_poolof', B);
    ok(p >= 1400 && p <= 1470, 'S4 survivors pool is exactly the remaining share (~143)', p);
    await ctx.close();
  }

  // S5: reinforcing a DAMAGED run — the new layer must land at full
  // strength in the pool, not scaled by the run's damage fraction.
  {
    const { ctx, call } = await boot(browser);
    const A = await call('dbg_wallplace', 0, 400, 1);
    const B = await call('dbg_wallplace', 56, 400, 1);
    await call('dbg_walldmg', A, 100);   // pool 220 -> 120
    ok((await call('dbg_wallreinf', A)) === 2, 'S5 reinforcement stacks (2 layers)');
    ok((await call('dbg_capof', A)) === 3300, 'S5 cap grows by the full layer (330)', await call('dbg_capof', A));
    const p = await call('dbg_poolof', A);
    ok(p >= 2250 && p <= 2350, 'S5 pool gains the FULL layer: 120 + 110 = 230', p);
    const ha = await call('dbg_wallhpof', A);
    ok(ha >= 1350 && ha <= 1440, 'S5 reinforced wall wears its share of the topped pool (~139)', ha);
    const hb = await call('dbg_wallhpof', B);
    ok(hb >= 660 && hb <= 730, 'S5 neighbour shares the topped pool too (~70)', hb);
    await ctx.close();
  }

  // S6: a loner chewed down, THEN joined to a run, brings its real
  // health — not the full pool its stale record claimed.
  {
    const { ctx, call } = await boot(browser);
    const A = await call('dbg_wallplace', 0, 400, 1);
    await call('dbg_walldmg', A, 90);
    ok((await call('dbg_wallhpof', A)) === 100, 'S6 loner chewed down to 10hp', await call('dbg_wallhpof', A));
    const B = await call('dbg_wallplace', 56, 400, 1);
    const p = await call('dbg_poolof', A);
    ok(p >= 1250 && p <= 1350, 'S6 joining a run pools real health (~130, not a healed 220)', p);
    ok((await call('dbg_grpnof', B)) === 2, 'S6 the pair is one run');
    await ctx.close();
  }

  // S7+S8: reinforcement inside a run sheds ONE layer at a time; a
  // lone reinforced pile pools with itself the same way.
  {
    const { ctx, call } = await boot(browser);
    const A = await call('dbg_wallplace', 0, 400, 1);
    await call('dbg_wallplace', 56, 400, 1);
    await call('dbg_wallreinf', A); await call('dbg_wallreinf', A);
    ok((await call('dbg_wallstackof', A)) === 3, 'S7 stack of 3 in the run');
    ok((await call('dbg_capof', A)) === 4400, 'S7 cap counts all reinforcement (440)', await call('dbg_capof', A));
    ok((await call('dbg_poolof', A)) === 4400, 'S7 undamaged reinforced run is full (440)', await call('dbg_poolof', A));
    await call('dbg_walldmg', A, 100);
    ok((await call('dbg_wallstackof', A)) === 3, 'S7 no shed before a full layer share is chewed');
    await call('dbg_walldmg', A, 30);
    ok((await call('dbg_wallstackof', A)) === 2, 'S7 chewing sheds ONE layer (3 -> 2)');
    ok((await call('dbg_walls')) === 2, 'S7 both entries still stand after the shed');
    const p = await call('dbg_poolof', A);
    ok(p >= 2250 && p <= 2400, 'S7 pool after the shed is the survivors share (~232)', p);
    await ctx.close();
  }
  {
    const { ctx, call } = await boot(browser);
    const A = await call('dbg_wallplace', 0, 400, 1);
    await call('dbg_wallreinf', A);
    ok((await call('dbg_grpnof', A)) === 2, 'S8 a lone stack counts its layers as a run of 2', await call('dbg_grpnof', A));
    await call('dbg_walldmg', A, 100);
    ok((await call('dbg_wallstackof', A)) === 2, 'S8 lone stack holds before its layer share');
    await call('dbg_walldmg', A, 30);
    ok((await call('dbg_wallstackof', A)) === 1, 'S8 lone stack sheds to its last layer');
    ok((await call('dbg_wallhpof', A)) > 0, 'S8 last layer still standing');
    await ctx.close();
  }

  console.log(`\n${pass} passed, ${fail} failed`);
  await browser.close();
  process.exit(fail ? 1 : 0);
})();
