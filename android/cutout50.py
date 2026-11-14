"""Extract the 50-carcass hero + flankers from hero50_full.png (Daniel's spec:
tiny, like in the game, ~50-bug hat). The short hat clears the stall and the
HUD, so this is flood fill + components + a green trim for the perch bug."""
from PIL import Image
from collections import deque
import os

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "art", "hero50_full.png")
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
for i, c in enumerate(comps[:8]):
    xs = [q[0] for q in c]; ys = [q[1] for q in c]
    print(f"comp{i}: n={len(c)} bbox crop=({min(xs)},{min(ys)})-({max(xs)},{max(ys)})")

def save_comp(pts, name):
    xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    out = Image.new("RGBA", (x1 - x0 + 1, y1 - y0 + 1), (0, 0, 0, 0))
    op = out.load()
    for (x, y) in pts:
        r, g, b = px[x, y]
        op[x - x0, y - y0] = (r, g, b, 255)
    out.save(os.path.join(OUT, name))
    print(name, "size:", out.size)

hero = []
for (x, y) in comps[0]:
    r, g, b = px[x, y]
    if g > 110 and g > r + 35 and g > b + 35:
        continue  # perch bug green
    hero.append((x, y))
print("hero trimmed:", len(comps[0]), "->", len(hero))
save_comp(hero, "hero50_cutout.png")
