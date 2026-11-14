"""Extract the REAL in-game hero (mega hat) + flanker bugs from hero_clean_full.png.
Repairs: (1) column-top spike trim removes stall bits/perch bug above the hat's arc;
(2) grey-stripe repaint removes the stall posts drawn over the hat's sides."""
from PIL import Image
from collections import deque
import os, statistics

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "art", "hero_clean_full.png")
OUT = os.path.join(HERE, "art")
CROP = (170, 40, 470, 300)

im = Image.open(SRC).convert("RGB")
crop = im.crop(CROP)
W, H = crop.size
px = crop.load()

bg = set()
for x in range(W):
    bg.add(px[x, 0]); bg.add(px[x, H - 1])
for y in range(H):
    bg.add(px[0, y]); bg.add(px[W - 1, y])
seen = [[False] * W for _ in range(H)]
dq = deque()
for x in range(W):
    dq.append((x, 0)); dq.append((x, H - 1))
for y in range(H):
    dq.append((0, y)); dq.append((W - 1, y))
while dq:
    x, y = dq.popleft()
    if x < 0 or y < 0 or x >= W or y >= H or seen[y][x]:
        continue
    seen[y][x] = True
    if px[x, y] in bg:
        dq.append((x + 1, y)); dq.append((x - 1, y)); dq.append((x, y + 1)); dq.append((x, y - 1))

fg = [[not seen[y][x] for x in range(W)] for y in range(H)]
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
print("components:", [len(c) for c in comps[:8]])

hero = set(comps[0])
xs = [p[0] for p in hero]; ys = [p[1] for p in hero]
print("hero raw bbox:", min(xs), min(ys), max(xs), max(ys), "n=", len(hero))

# --- repair 1: column-top spike trim (junk above the hat's arc) ---
for _ in range(3):
    tops = {}
    for (x, y) in hero:
        if x not in tops or y < tops[x]:
            tops[x] = y
    kill = set()
    for x, t in tops.items():
        neigh = [tops[xx] for xx in range(x - 6, x + 7) if xx in tops]
        med = statistics.median(neigh)
        if t < med - 4:
            lim = int(med) - 2
            for (pxx, pyy) in list(hero):
                if pxx == x and pyy < lim:
                    kill.add((pxx, pyy))
    if not kill:
        break
    hero -= kill
    print("spike trim removed", len(kill))

# --- repair 2: grey posts -> repaint with hat texture from 7px to the side ---
def is_grey(c):
    r, g, b = c
    return abs(r - g) < 12 and abs(g - b) < 12 and 90 < r < 190
repainted = 0
newpx = {}
for (x, y) in hero:
    if is_grey(px[x, y]):
        for dx in (-7, 7, -10, 10):
            sx = x + dx
            if (sx, y) in hero and not is_grey(px[sx, y]):
                newpx[(x, y)] = px[sx, y]; repainted += 1; break
print("grey repainted:", repainted)

def save_comp(pts, name, overrides=None):
    pts = list(pts)
    xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    out = Image.new("RGBA", (x1 - x0 + 1, y1 - y0 + 1), (0, 0, 0, 0))
    op = out.load()
    for (x, y) in pts:
        c = (overrides or {}).get((x, y), px[x, y])
        op[x - x0, y - y0] = (c[0], c[1], c[2], 255)
    out.save(os.path.join(OUT, name))
    print(name, "size:", out.size)

save_comp(hero, "hero_cutout.png", newpx)
n = 0
for c in comps[1:]:
    if len(c) > 150:
        n += 1
        save_comp(c, f"bug_flank{n}.png")
print("done")

# ================= SURGERY (appended): hard geometric cuts per grid inspection =========
from PIL import Image as _Im
p = os.path.join(OUT, "hero_cutout.png")
im2 = _Im.open(p).convert("RGBA")
w2, h2 = im2.size
px2 = im2.load()
removed = 0
for y in range(h2):
    for x in range(w2):
        r, g, b, a = px2[x, y]
        if a == 0:
            continue
        kill = False
        if y < 21: kill = True                                   # junk band (stall boards, perch top)
        if y >= 155: kill = True                                 # DEV label / prompt text
        if x < 10 and 70 <= y <= 110: kill = True                # beige blob at hat's left
        if x > 149 and y < 128: kill = True                      # post remnant + right junk
        if x > 149 and y < 55: kill = True
        if y < 34 and g > 110 and g > r + 35 and g > b + 35: kill = True   # perch bug chin (green)
        if y < 60 and r > 215 and g > 205 and b > 175 and abs(r - b) < 70: kill = True  # pale stall board
        if 34 <= y <= 52 and 112 <= x <= 140 and r > 170 and g < 90 and b < 70: kill = True  # red crate
        if kill:
            px2[x, y] = (0, 0, 0, 0); removed += 1
print("surgery removed:", removed)
# re-trim to bbox of remaining opaque pixels
bb = im2.getbbox()
im2 = im2.crop(bb)
im2.save(p)
print("hero final size:", im2.size)
# montage of the other components for flanker picking
mont = _Im.new("RGBA", (6 * 46, 40), (255, 0, 255, 255))
for i, c in enumerate(comps[1:7]):
    xs = [q[0] for q in c]; ys = [q[1] for q in c]
    print(f"comp{i+1}: n={len(c)} bbox crop=({min(xs)},{min(ys)})-({max(xs)},{max(ys)})")
