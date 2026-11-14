"""Extract the 420 hero from hero420c_full.png (2M geometry: clean flood,
stall clear). The pill occludes the hat's top-center band (y~55..84 under
the pill) — rebuilt by period-cloning the weave between the two visible
sliver tops. Also saves this capture's orange + purple flanker bugs."""
from PIL import Image
from collections import deque
import os

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "art", "hero420c_full.png")
OUT = os.path.join(HERE, "art")
im = Image.open(SRC).convert("RGB")
px0 = im.load()

PILL = (262, 24, 413, 85)   # measured from the capture (navy runs at y=30/50)
print("pill:", PILL)

CROP = (150, 15, 490, 310)
crop = im.crop(CROP)
W, H = crop.size
px = crop.load()
OX, OY = CROP[0], CROP[1]

bg = set()
for x in range(W):
    bg.add(px[x, 0]); bg.add(px[x, H - 1])
for y in range(H):
    bg.add(px[0, y]); bg.add(px[W - 1, y])
seen2 = [[False] * W for _ in range(H)]
dq = deque()
for x in range(W):
    dq.append((x, 0)); dq.append((x, H - 1))
for y in range(H):
    dq.append((0, y)); dq.append((W - 1, y))
while dq:
    x, y = dq.popleft()
    if x < 0 or y < 0 or x >= W or y >= H or seen2[y][x]:
        continue
    seen2[y][x] = True
    if px[x, y] in bg:
        dq.append((x + 1, y)); dq.append((x - 1, y)); dq.append((x, y + 1)); dq.append((x, y - 1))

fg = [[not seen2[y][x] for x in range(W)] for y in range(H)]
comp = [[-1] * W for _ in range(H)]
comps = []
for y in range(H):
    for x in range(W):
        if fg[y][x] and comp[y][x] < 0:
            cid = len(comps); pts = []
            dq = deque([(x, y)]); comp[y][x] = cid
            while dq:
                cx, cy = dq.popleft(); pts.append((cx, cy))
                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if 0 <= nx < W and 0 <= ny < H and fg[ny][nx] and comp[ny][nx] < 0:
                        comp[ny][nx] = cid; dq.append((nx, ny))
            comps.append(pts)
comps.sort(key=len, reverse=True)
for i, c in enumerate(comps[:8]):
    xs = [q[0] for q in c]; ys = [q[1] for q in c]
    print(f"comp{i}: n={len(c)} img bbox=({min(xs)+OX},{min(ys)+OY})-({max(xs)+OX},{max(ys)+OY})")

hero = {(x, y) for (x, y) in comps[0] if 228 <= x + OX <= 428 and 26 <= y + OY <= 222}
print("hero sanitized:", len(hero))
pc = (PILL[0] - OX, PILL[1] - OY, PILL[2] - OX, PILL[3] - OY)
hero = {(x, y) for (x, y) in hero if not (pc[0] <= x <= pc[2] and pc[1] <= y <= pc[3])}
def col_top(x):
    ys = [y for (xx, y) in hero if xx == x]
    return min(ys) if ys else None
lt, rt = col_top(pc[0] - 3), col_top(pc[2] + 3)
print("sliver tops (img y):", None if lt is None else lt + OY, None if rt is None else rt + OY)
if lt is None: lt = rt
if rt is None: rt = lt
colx = 330 - OX
colpix = [px[colx, y] for y in range(120 - OY, 220 - OY)]
best_p, best_s = 4, -1
for p in range(3, 16):
    sc_ = sum(1 for i in range(len(colpix) - p) if colpix[i] == colpix[i + p])
    if sc_ > best_s: best_s, best_p = sc_, p
shift = best_p * max(1, round(52 / best_p))
print("period:", best_p, "shift:", shift)
over = {}
for x in range(pc[0], pc[2] + 1):
    if not any((x, yy) in hero for yy in range(pc[3] + 1, pc[3] + 7)):
        continue
    t = int(lt + (rt - lt) * (x - pc[0]) / max(1, pc[2] - pc[0]))
    for yy in range(t, pc[3] + 1):
        sy = yy + shift
        if 0 <= sy < H:
            over[(x, yy)] = px[x, sy]
            hero.add((x, yy))
print("notch filled:", len(over))

# body bottom from robe-blue pixels in the hero
blue_ys = [y + OY for (x, y) in hero
           if abs(px[x, y][0] - 62) < 26 and abs(px[x, y][1] - 110) < 26 and abs(px[x, y][2] - 222) < 32]
body_bottom = 211   # measured robe-blue bbox bottom
print("body bottom img y:", body_bottom)

def pale(c):
    r, g, b = c
    return 185 <= r <= 248 and 175 <= g <= 240 and 150 <= b <= 220 and 8 < (r - b) < 62
def is_grey(c):
    r, g, b = c
    return abs(r - g) < 12 and abs(g - b) < 12 and 88 < r < 192
kill = set()
for (x, y) in hero:
    c = over.get((x, y), px[x, y])
    r, g, b = c
    ix, iy = x + OX, y + OY
    if g > 110 and g > r + 35 and g > b + 35 and iy < 92: kill.add((x, y))
    if r > 228 and g > 178 and b < 98: kill.add((x, y))
    if pale(c) and not (294 <= ix <= 348 and body_bottom - 46 <= iy <= body_bottom + 4): kill.add((x, y))
    if iy > body_bottom + 4: kill.add((x, y))
hero -= kill
print("kills:", len(kill))
repainted = 0
for (x, y) in list(hero):
    c = over.get((x, y), px[x, y])
    if is_grey(c):
        for dx in (-7, 7, -11, 11):
            sx = x + dx
            if (sx, y) in hero:
                sc = over.get((sx, y), px[sx, y])
                if not is_grey(sc):
                    over[(x, y)] = sc; repainted += 1; break
print("grey repainted:", repainted)

xs = [p[0] for p in hero]; ys = [p[1] for p in hero]
x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
out = Image.new("RGBA", (x1 - x0 + 1, y1 - y0 + 1), (0, 0, 0, 0))
op = out.load()
for (x, y) in hero:
    c = over.get((x, y), px[x, y])
    op[x - x0, y - y0] = (c[0], c[1], c[2], 255)
out.save(os.path.join(OUT, "hero420_cutout.png"))
print("hero420_cutout.png:", out.size)

def save_comp(pts, name):
    xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
    a, b, c_, d_ = min(xs), max(xs), min(ys), max(ys)
    o = Image.new("RGBA", (b - a + 1, d_ - c_ + 1), (0, 0, 0, 0))
    op2 = o.load()
    for (x, y) in pts:
        cc = px[x, y]
        op2[x - a, y - c_] = (cc[0], cc[1], cc[2], 255)
    o.save(os.path.join(OUT, name))
    print(name, o.size)
for c in comps[1:]:
    if len(c) < 120: continue
    xs = [q[0] + OX for q in c]; ys = [q[1] + OY for q in c]
    cx, cy = sum(xs) / len(xs), sum(ys) / len(ys)
    if 205 < cx < 240 and 198 < cy < 228: save_comp(c, "bug_orange2.png")
    if False: save_comp(c, "bug_purple.png")
