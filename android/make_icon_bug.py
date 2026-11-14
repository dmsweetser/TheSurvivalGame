#!/usr/bin/env python3
"""The Survival Game — ANDROID launcher icon: ONE ANGRY NORMAL BUG.

Daniel (Nov 3, 2026): the first icon used the orange Eddie-mode
puffball; he corrected it — "make it one of the normal bug." So this is
the game's ordinary code-bug (kind 0, the roach) drawn the way
draw_bug_sprite draws it: purple shell COL(150,90,200) with two dark
bands, pale belly COL(190,150,230), round head, six jointed legs,
ball-tipped antennae — but ANGRY: slanted brow bars, glaring googly
eyes, and a gritted-teeth snarl, on the game's grass checker.

Outputs: android/icon-master-1024.png, res/mipmap-*/ic_launcher.png,
res/mipmap-xxxhdpi/ic_launcher_{foreground,background}.png, and the
chat preview at ../tests/android-icon-preview.png.
Run: python3 android/make_icon_bug.py
"""
from PIL import Image, ImageDraw
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
S = 3                      # supersample
W = 1024

OUT    = (26, 24, 40, 255)
GRASS_L = (112, 198, 62, 255)
GRASS_D = (100, 184, 52, 255)
BODY   = (150, 90, 200, 255)
BODY_D = (108, 65, 144, 255)
BELLY  = (190, 150, 230, 255)
WHITE  = (255, 255, 255, 255)

def sc(box):
    return [v * S for v in box]

def draw_background(img):
    d = ImageDraw.Draw(img)
    d.rectangle(sc([0, 0, W, W]), fill=GRASS_D)
    sq = W // 8
    for gy in range(9):
        for gx in range(9):
            if (gx + gy) % 2 == 0:
                d.rectangle(sc([gx * sq, gy * sq, (gx + 1) * sq, (gy + 1) * sq]), fill=GRASS_L)
    for (tx, ty) in [(80, 850), (170, 960), (880, 880), (790, 965), (60, 690), (950, 700),
                     (120, 180), (905, 150), (480, 60), (50, 420), (960, 430)]:
        for k in (-1, 0, 1):
            d.line(sc([tx + k * 12, ty, tx + k * 20, ty - 30]),
                   fill=(74, 148, 40, 255), width=6 * S)

def thick(d, pts, fill, wdt):
    d.line([ (x * S, y * S) for (x, y) in pts ], fill=fill, width=wdt * S, joint='curve')

def draw_bug(d, cx=512, cy=512, scale=1.0):
    # Daniel (Nov 3): the game's kind-2 SPIDER — main shell, eight
    # splayed legs, yellow warning chevrons. (Wings and the rear
    # abdomen both tried and removed at his word, same afternoon.)
    ORANGE   = (220, 120, 50, 255)
    ORANGE_D = (158, 86, 36, 255)
    O_BELLY  = (245, 170, 100, 255)
    CHEV     = (255, 220, 90, 255)
    R = 262 * scale
    bx, by = cx, cy + 26 * scale          # main shell
    hx, hy = cx, by - R / 3               # head
    hr = R / 2 + 3 * scale
    img = d._image
    def E_(dd, x0, y0, x1, y1, **kw): dd.ellipse(sc([x0, y0, x1, y1]), **kw)
    def L_(dd, pts, fill, wdt): thick(dd, pts, fill, max(1, int(wdt * scale)))
    d = ImageDraw.Draw(img)
    def E(x0, y0, x1, y1, **kw): E_(d, x0, y0, x1, y1, **kw)
    def L(pts, fill, wdt): L_(d, pts, fill, wdt)
    # ---- eight legs, fanned wide so every one reads -------------------------------
    for side in (-1, 1):
        fans = [(-0.62, -0.95), (-0.22, -0.55), (0.22, 0.05), (0.62, 0.62)]
        for row, lift in fans:
            sx = bx + side * (R - 10 * scale)
            sy = by + row * R * 0.92
            kx = sx + side * R * 0.40
            ky = sy + lift * R * 0.35
            fx = sx + side * R * 0.78
            fy = ky + R * 0.30
            L([(sx, sy), (kx, ky), (fx, fy)], OUT, 24)
            L([(sx, sy), (kx, ky), (fx, fy)], ORANGE_D, 12)
    # ---- main shell ------------------------------------------------------------------
    E(bx - R - 4 * scale, by - (R * 3 / 4 + 4 * scale), bx + R + 4 * scale, by + (R * 3 / 4 + 4 * scale), fill=OUT)
    E(bx - R - 2 * scale, by - (R * 3 / 4 + 2 * scale), bx + R + 2 * scale, by + (R * 3 / 4 + 2 * scale), fill=ORANGE)
    d.pieslice(sc([bx - R - 2 * scale, by - (R * 3 / 4 + 2 * scale), bx + R + 2 * scale, by + (R * 3 / 4 + 2 * scale)]),
               start=20, end=160, fill=ORANGE_D)
    E(bx - R - 2 * scale, by - (R * 3 / 4 + 2 * scale), bx + R + 2 * scale, by + (R * 3 / 4 - 12 * scale), fill=ORANGE)
    # belly
    E(bx - R * 3 / 5, by + 2 * scale - R / 2, bx + R * 3 / 5, by + 2 * scale + R / 2, fill=O_BELLY)
    # warning chevrons on the shell, over the belly so they read
    for dy in (-R * 0.20, R * 0.10):
        L([(bx - 25 * scale, by + dy - 9 * scale), (bx, by + dy + 8 * scale), (bx + 25 * scale, by + dy - 9 * scale)], CHEV, 9)
    # gloss
    E(bx - R * 0.72, by - R * 0.60, bx - R * 0.20, by - R * 0.30, fill=(250, 205, 150, 255))
    # ---- head ---------------------------------------------------------------------------
    E(hx - hr - 2 * scale, hy - hr - 2 * scale, hx + hr + 2 * scale, hy + hr + 2 * scale, fill=OUT)
    E(hx - hr, hy - hr, hx + hr, hy + hr, fill=ORANGE)
    E(hx - hr * 0.55, hy - hr * 0.78, hx + hr * 0.05, hy - hr * 0.38, fill=(250, 205, 150, 255))
    for side in (-1, 1):
        x0, y0 = hx + side * 10 * scale, hy - hr * 0.60
        x1, y1 = hx + side * 26 * scale, hy - hr * 0.60 - 22 * scale
        x2, y2 = hx + side * 44 * scale, hy - hr * 0.60 - 40 * scale
        L([(x0, y0), (x1, y1), (x2, y2)], OUT, 10)
        E(x2 - 8 * scale, y2 - 8 * scale, x2 + 8 * scale, y2 + 8 * scale, fill=OUT)
        E(x2 - 5 * scale, y2 - 5 * scale, x2 + 5 * scale, y2 + 5 * scale, fill=ORANGE_D)
    for side in (-1, 1):
        ex, ey = hx + side * hr / 2, hy - 3 * scale
        er = hr / 2 + 1 * scale
        E(ex - er - 2 * scale, ey - er - 2 * scale, ex + er + 2 * scale, ey + er + 2 * scale, fill=OUT)
        E(ex - er, ey - er, ex + er, ey + er, fill=WHITE)
        pr = er / 2 + 1 * scale
        E(ex - pr + 3 * scale, ey - pr, ex + pr + 3 * scale, ey + pr, fill=OUT)
        E(ex - pr + 1 * scale, ey - pr - 2 * scale, ex - pr + 7 * scale, ey - pr + 4 * scale, fill=WHITE)
    my = hy + hr / 2 + 2 * scale
    L([(hx - 10 * scale, my + 5 * scale), (hx, my), (hx + 10 * scale, my + 5 * scale)], ORANGE_D, 7)
    return d

def render(with_bg):
    img = Image.new('RGBA', (W * S, W * S), (0, 0, 0, 0))
    if with_bg:
        draw_background(img)
    d = ImageDraw.Draw(img)
    draw_bug(d)
    return img.resize((W, W), Image.LANCZOS)

def save(img, path, size):
    img.resize((size, size), Image.LANCZOS).save(path)
    print('wrote', path, size)

master = render(True)
save(master, os.path.join(ROOT, 'android', 'icon-master-1024.png'), 1024)
save(master, os.path.join(ROOT, 'tests', 'android-icon-preview.png'), 512)
res = os.path.join(ROOT, 'android', 'res')
for name, size in [('mdpi', 48), ('hdpi', 72), ('xhdpi', 96), ('xxhdpi', 144), ('xxxhdpi', 192)]:
    save(master, os.path.join(res, 'mipmap-' + name, 'ic_launcher.png'), size)
# adaptive: grass background + bug-only foreground (bug scaled into the safe zone)
bg = Image.new('RGBA', (W * S, W * S), (0, 0, 0, 0))
draw_background(bg)
save(bg.resize((W, W), Image.LANCZOS), os.path.join(res, 'mipmap-xxxhdpi', 'ic_launcher_background.png'), 192)
fg = Image.new('RGBA', (W * S, W * S), (0, 0, 0, 0))
draw_bug(ImageDraw.Draw(fg), scale=0.94)
save(fg.resize((W, W), Image.LANCZOS), os.path.join(res, 'mipmap-xxxhdpi', 'ic_launcher_foreground.png'), 192)
print('done')
