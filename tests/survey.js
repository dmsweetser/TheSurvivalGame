// offline difficulty survey: for each seed x level, summarise the
// generated course's hard features from runmap.js output
const { execFileSync } = require('child_process');
const seeds = [11126, 8689, 58220, 14876, 57829, 37579];
for (let lvl = 1; lvl <= 18; lvl++) {
  let worstDouble = 0, maxGap = 0, gapCount = 0, vaults = 0, kinds = new Set(), tightCombo = 0;
  for (const seed of seeds) {
    const out = execFileSync('node', ['runmap.js', String(seed), String(lvl), String(lvl)], { encoding: 'utf8' });
    const gaps = [...out.matchAll(/GAP at tile (\d+) width (\d+)/g)].map(m => ({ x: +m[1], w: +m[2] }));
    gapCount += gaps.length;
    for (const g of gaps) maxGap = Math.max(maxGap, g.w);
    for (let i = 1; i < gaps.length; i++) if (gaps[i].x - (gaps[i - 1].x + gaps[i - 1].w) <= 2) worstDouble = Math.max(worstDouble, gaps[i - 1].w + gaps[i].w);
    if (/vaultTx=[0-9]/.test(out) && !/vaultTx=-1/.test(out)) vaults++;
    const em = out.match(/enemies: (.*)/);
    if (em) for (const m of em[1].matchAll(/kind(\d+)@/g)) kinds.add(+m[1]);
    // pipe immediately before a gap (<=3 tiles between pipe end and gap start)
    const segs = [...out.matchAll(/x=(\d+) \(px \d+\) \{"x":\d+,"type":"(pipe|gap)","h":(\d+)?,?"?(piranha|vault)?/g)];
    const segLines = out.split('\n').filter(l => l.includes('"type":"pipe"') || l.includes('"type":"gap"'));
    const parsed = segLines.map(l => { const x = +l.match(/x=(\d+)/)[1]; const t = l.includes('"type":"pipe"') ? 'p' : 'g'; const w = t === 'g' ? +l.match(/"w":(\d+)/)[1] : 2; return { x, t, w }; }).sort((a, b) => a.x - b.x);
    for (let i = 1; i < parsed.length; i++) if (parsed[i - 1].t === 'p' && parsed[i].t === 'g' && parsed[i].x - (parsed[i - 1].x + 2) <= 3 && parsed[i].w >= 3) tightCombo++;
  }
  console.log(`L${String(lvl).padStart(2)} gaps/seed=${(gapCount / seeds.length).toFixed(1)} maxGap=${maxGap} worstDoubleSum=${worstDouble} pipe->gapCombos=${tightCombo} vaultSeeds=${vaults}/6 enemyKinds=${[...kinds].sort().join(',')}`);
}
