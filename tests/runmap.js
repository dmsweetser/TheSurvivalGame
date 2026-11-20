// runmap.js — OFFLINE replica of plat_load_level's generation (run.c)
// for analysis only: given a buddy bw_seed and level, print the tile
// map + a segment report (gaps, run-ups, pipes, enemies). Play never
// uses this; it exists so a death spot found in real play can be
// diagnosed against the exact geometry that generated it.
function hash2(x, y, seed) {
  let h = (seed + Math.imul(x | 0, 374761393) + Math.imul(y | 0, 668265263)) >>> 0;
  h = Math.imul(h ^ (h >>> 13), 1274126177) >>> 0;
  return (h ^ (h >>> 16)) >>> 0;
}
const PL_TILE = 24, PL_ROWS = 15, PL_MAXW = 240, PL_GROUND = 13;
function genLevel(seed0, lvl) {
  const W0 = Math.min(110 + lvl * 14, PL_MAXW - 4);
  const grid = new Uint8Array(PL_MAXW * PL_ROWS);
  const set = (x, y, v) => { if (x >= 0 && y >= 0 && x < W0 && y < PL_ROWS) grid[y * PL_MAXW + x] = v; };
  const tile = (x, y) => (x < 0 || y < 0 || x >= W0 || y >= PL_ROWS) ? 0 : grid[y * PL_MAXW + x];
  const ens = [];
  const enAdd = (kind, x, y) => { if (ens.length < 24) ens.push({ kind, x, y }); };
  let vaultTx = -1;
  const seed = (seed0 ^ (0x9E37 + lvl * 7919)) >>> 0;
  for (let x = 0; x < W0; x++) { set(x, 13, 1); set(x, 14, 1); }
  const segs = [];
  let x = 10, cpdone = 0;
  while (x < W0 - 18) {
    const h = hash2(x, lvl * 3 + 1, seed);
    const seg = hash2(x * 3 + 1, lvl * 7, seed >>> 3) % 6;
    if (!cpdone && x > W0 / 2) { cpdone = 1; segs.push({ x, type: 'checkpoint-skip' }); x += 6; continue; }
    if (seg === 0) {
      const gw = 2 + (h % 3);
      for (let g = 0; g < gw; g++) { set(x + g, 13, 0); set(x + g, 14, 0); }
      const plat = !!(h & 8);
      if (plat) for (let p = 0; p < 3; p++) { set(x + p, 8, 5); set(x + p, 7, 10); }
      segs.push({ x, type: 'gap', w: gw, plat });
      x += gw + 2;
    } else if (seg === 1) {
      const ph = 2 + (h % 3);
      for (let r = 0; r < ph; r++) for (let c = 0; c < 2; c++) set(x + c, 12 - r, r === ph - 1 ? 7 : 6);
      if (lvl >= 2 && (h & 16)) enAdd(3, (x + 1) * PL_TILE, (13 - ph) * PL_TILE);
      if (ph >= 4) vaultTx = x;
      segs.push({ x, type: 'pipe', h: ph, piranha: lvl >= 2 && !!(h & 16), vault: ph >= 4 });
      x += 4;
    } else if (seg === 2) {
      const n = 3 + (h % 3);
      for (let i = 0; i < n; i++) set(x + i, 9, (i === (n >> 1) || (h >> i) & 1) ? 3 : 2);
      if (h & 32) for (let i = 0; i < n; i++) set(x + i, 8, 10);
      segs.push({ x, type: 'crates', n });
      x += n + 3;
    } else if (seg === 3) {
      const sh = 2 + (h % 3);
      for (let s = 0; s < sh; s++) for (let r = 0; r <= s; r++) set(x + s, 12 - r, 5);
      let adv = sh + 2;
      if (h & 64) { for (let s = 0; s < sh; s++) for (let r = 0; r < sh - s; r++) set(x + sh + 2 + s, 12 - r, 5); adv += sh + 1; }
      segs.push({ x, type: 'stairs', h: sh, down: !!(h & 64) });
      x += adv;
    } else if (seg === 4) {
      const ne = 1 + (h % 3);
      const kinds = [];
      for (let e = 0; e < ne; e++) {
        let kind = 0;
        const eh = hash2(x + e * 7, lvl, seed ^ 0xE11);
        if (lvl >= 3 && eh % 5 === 0) kind = 4; else if (lvl >= 2 && eh % 3 === 0) kind = 1;
        enAdd(kind, (x + 2 + e * 3) * PL_TILE, kind === 4 ? 9 * PL_TILE : PL_GROUND * PL_TILE);
        kinds.push(kind);
      }
      for (let b = 0; b < 4; b++) set(x + 1 + b, 11, 10);
      segs.push({ x, type: 'enemies', kinds });
      x += 6 + ne * 2;
    } else {
      for (let p = 0; p < 3; p++) { set(x + 1 + p, 9, 5); set(x + 1 + p, 8, 10); }
      const upper = !!(h & 128);
      if (upper) for (let p = 0; p < 3; p++) { set(x + 3 + p, 5, 5); set(x + 3 + p, 4, 10); }
      segs.push({ x, type: 'platforms', upper });
      x += 7;
    }
  }
  for (let cx = 0; cx < 9; cx++) for (let ry = 0; ry < PL_GROUND; ry++) set(cx, ry, 0);
  { const ct = Math.floor(W0 / 2); for (let cx = ct - 1; cx <= ct + 2; cx++) for (let ry = 0; ry < PL_GROUND; ry++) set(cx, ry, 0); }
  for (const e of ens) if (e.x < 9 * PL_TILE) e.kind = -1;
  const sx = W0 - 13;
  for (let s = 0; s < 8; s++) for (let r = 0; r <= s; r++) set(sx + s, 12 - r, 5);
  set(W0 - 3, 12, 9);
  for (let r = 4; r <= 11; r++) set(W0 - 3, r, 8);
  return { W: W0, grid, tile, ens: ens.filter(e => e.kind >= 0), segs, vaultTx };
}
function render(seed, lvl) {
  const L = genLevel(seed, lvl);
  const chars = { 0: ' ', 1: '#', 2: 'B', 3: '?', 4: 'u', 5: '=', 6: '|', 7: 'T', 8: 'F', 9: 'X', 10: '.' };
  const rows = [];
  for (let y = 0; y < PL_ROWS; y++) {
    let row = '';
    for (let x = 0; x < L.W; x++) row += chars[L.tile(x, y)] || '?';
    rows.push(row);
  }
  for (const e of L.ens) {
    const tx = Math.floor(e.x / PL_TILE), ty = Math.floor(e.y / PL_TILE);
    if (ty >= 0 && ty < PL_ROWS) rows[ty] = rows[ty].substring(0, tx) + 'e' + rows[ty].substring(tx + 1);
  }
  console.log('LEVEL ' + lvl + ' seed=' + seed + ' W=' + L.W + ' (' + L.W * 24 + 'px) vaultTx=' + L.vaultTx);
  rows.forEach((r, i) => console.log(String(i).padStart(2) + ' ' + r));
  console.log('segments:');
  let prevEnd = 10;
  for (const s of L.segs) {
    console.log('  x=' + s.x + ' (px ' + s.x * 24 + ') ' + JSON.stringify(s) + ' runup_from_prev_seg_end=' + (s.x - prevEnd) + ' tiles');
    prevEnd = s.x + 6;
  }
  console.log('enemies: ' + L.ens.map(e => 'kind' + e.kind + '@tile' + Math.floor(e.x / 24)).join(' '));
  // gap audit: every ground hole with the flat run-up before it
  let runup = 0, lastSolidEnd = 0;
  for (let x = 0; x < L.W; x++) {
    if (L.tile(x, 13) === 0) {
      let w = 0; while (x + w < L.W && L.tile(x + w, 13) === 0) w++;
      console.log('  GAP at tile ' + x + ' width ' + w + ' flat-runup-before=' + (x - lastSolidEnd) + ' tiles');
      x += w - 1; lastSolidEnd = x + 1;
    } else lastSolidEnd = x + 1;
  }
}
if (require.main === module) {
  const seed = parseInt(process.argv[2], 10), from = parseInt(process.argv[3] || '1', 10), to = parseInt(process.argv[4] || process.argv[3] || '1', 10);
  for (let l = from; l <= to; l++) render(seed, l);
}
module.exports = { genLevel, hash2 };
