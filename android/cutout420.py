"""Extract the 420-carcass (max stack) hero from hero420_full.png.
The HUD pill occludes a band of the hat's top-center; that notch is rebuilt
by period-cloning the hat's own weave upward to the top line visible at the
pill's sides. Posts over the hat are repainted with hat texture."""
from PIL import Image
from collections import deque
import os

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "art", "hero420_full.png")
OUT = os.path.join(HERE, "art")
CROP = (170, 8, 500, 320)          # in the capture's 666x378 coords
PILL = (288, 25, 478, 87)         # img coords of the HUD pill (measured)

im = Image.open(SRC).convert("RGB")
crop = im.crop(CROP)
W, H = crop.size
px = crop.load()
OX, OY = CROP[0], CROP[1]

# region-growing flood from the borders: this capture was resampled by the
# browser (666x378 for a 640x360 framebuffer), so flat colours became gentle
# gradients — grow through neighbours that are close to the pixel we came
# from (sharp sprite edges stop the growth).
def close(c1, c2, tol=14):
    return max(abs(c1[0] - c2[0]), abs(c1[1] - c2[1]), abs(c1[2] - c2[2])) <= tol
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
    c = px[x, y]
    for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
        if 0 <= nx < W and 0 <= ny < H and not seen[ny][nx] and close(c, px[nx, ny]):
            dq.append((nx, ny))

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
for i, c in enumerate(comps[:6]):
    xs = [q[0] for q in c]; ys = [q[1] for q in c]
    print(f"comp{i}: n={len(c)} bbox crop=({min(xs)},{min(ys)})-({max(xs)},{max(ys)})")

hero = set(comps[0])
# --- delete the pill rectangle from the hero ---
pill_c = (PILL[0] - OX, PILL[1] - OY, PILL[2] - OX, PILL[3] - OY)
before = len(hero)
hero = {(x, y) for (x, y) in hero if not (pill_c[0] <= x <= pill_c[2] and pill_c[1] <= y <= pill_c[3])}
print("pill deleted:", before - len(hero))

# --- measure the weave period on a clean column (img x=350) ---
colx = 350 - OX
def pix(x, y):
    return px[x, y]
best_p, best_score = 4, -1
colpix = [pix(colx, y) for y in range(130, 230)]
for p in range(3, 16):
    score = sum(1 for i in range(len(colpix) - p) if colpix[i] == colpix[i + p])
    if score > best_score:
        best_score, best_p = score, p
print("weave period:", best_p)
shift = best_p * max(1, round(52 / best_p))

# --- clone-fill the notch up to the top line (img y ~63) ---
top_img_y = 63
filled = {}
for x in range(pill_c[0], pill_c[2] + 1):
    # only columns where the hat exists just below the pill
    yb = pill_c[3] + 1
    if (x, yb) not in hero and (x, yb + 2) not in hero:
        continue
    for yy in range(top_img_y - OY, pill_c[3] + 1):
        src_y = yy + shift
        if (x, src_y) in hero or True:
            filled[(x, yy)] = pix(x, src_y) if 0 <= src_y < H else (120, 70, 30)
    hero |= {(x, yy) for yy in range(top_img_y - OY, pill_c[3] + 1) if (x, yy) in filled}
print("notch filled px:", len(filled))

# --- color kills: perch green (top), sign gold, crate red (right side) ---
kill = set()
for (x, y) in hero:
    r, g, b = px[x, y] if (x, y) not in filled else filled[(x, y)]
    ix, iy = x + OX, y + OY
    if g > 110 and g > r + 35 and g > b + 35 and iy < 84: kill.add((x, y))          # perch bug
    if r > 228 and g > 178 and b < 98: kill.add((x, y))                            # BUG BOUNTY gold
    if ix > 392 and 68 < iy < 122 and r > 168 and g < 92 and b < 72: kill.add((x, y))  # red crate
hero -= kill
print("color kills:", len(kill))

# --- grey posts -> repaint from hat texture 7px aside ---
def is_grey(c):
    r, g, b = c
    return abs(r - g) < 12 and abs(g - b) < 12 and 88 < r < 192
over = dict(filled)
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

# --- save ---
xs = [p[0] for p in hero]; ys = [p[1] for p in hero]
x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
out = Image.new("RGBA", (x1 - x0 + 1, y1 - y0 + 1), (0, 0, 0, 0))
op = out.load()
for (x, y) in hero:
    c = over.get((x, y), px[x, y])
    op[x - x0, y - y0] = (c[0], c[1], c[2], 255)
out.save(os.path.join(OUT, "hero420_cutout.png"))
print("hero420_cutout.png size:", out.size)
