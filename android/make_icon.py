#!/usr/bin/env python3
"""The Survival Game — launcher/app icon art.

Draws the icon from scratch in the game's flat cartoon style: DEV with
his towering carcass hat, flanked by two live bugs, on the Epicenter
plaza. One 1024 master feeds every target:
  - Android legacy icons at all five mipmap densities
  - Android adaptive icon (anydpi-v26): grass/plaza background layer +
    padded transparent foreground layer
  - macOS app icon source (src/macapp/app-icon-512.png)
  - Windows icon (src/winres/app.ico)
Run: python3 android/make_icon.py
"""
from PIL import Image, ImageDraw, ImageFilter
import math, os, random

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
S = 2                      # supersample: draw at 2048, ship at 1024
W = 1024

OUT   = (26, 24, 40, 255)
GRASS_L = (112, 198, 62, 255)
GRASS_D = (100, 184, 52, 255)
PLAZA   = (239, 233, 209, 255)
PLAZA_S = (221, 212, 184, 255)
SKIN    = (246, 199, 150, 255)
HAIR    = (98, 63, 38, 255)
BLUE    = (62, 110, 222, 255)
BLUE_L  = (98, 144, 238, 255)
BLUE_D  = (44, 80, 172, 255)
LENS    = (170, 226, 248, 255)
BUG_BLUE = (86, 170, 228, 255); BUG_BLUE_D = (52, 128, 198, 255)
BUG_ORNG = (234, 130, 46, 255); BUG_ORNG_D = (198, 100, 30, 255)
BUG_PURP = (150, 98, 198, 255)
BUG_GRN  = (98, 192, 92, 255)

def sc(box):  # logical -> device coords
    return [v * S for v in box]

def new_layer():
    return Image.new("RGBA", (W * S, W * S), (0, 0, 0, 0))

# ---------------------------------------------------------------- background
def draw_background(img):
    d = ImageDraw.Draw(img)
    # the previous background: the top-down game world itself — grass
    # checker over the whole frame with the Epicenter plaza in the middle.
    # No sky band (Daniel: "we don't need the space one").
    d.rectangle(sc([0, 0, W, W]), fill=GRASS_D)
    sq = 128
    for gy in range(0, W // sq + 1):
        for gx in range(0, W // sq + 1):
            if (gx + gy) % 2 == 0:
                d.rectangle(sc([gx * sq, gy * sq, (gx + 1) * sq, (gy + 1) * sq]), fill=GRASS_L)
    # the Epicenter plaza the hero stands on
    d.ellipse(sc([67, 390, 957, 920]), fill=PLAZA)
    d.ellipse(sc([87, 407, 937, 903]), outline=PLAZA_S, width=8 * S)
    # a few grass tufts around the edges
    for (tx, ty) in [(90, 830), (170, 930), (880, 880), (800, 960), (60, 700), (940, 720),
                     (120, 200), (900, 160), (480, 90), (60, 420), (950, 430)]:
        for k in (-1, 0, 1):
            d.line(sc([tx + k * 12, ty, tx + k * 20, ty - 30]), fill=(74, 148, 40, 255), width=6 * S)

# ---------------------------------------------------------------- one bug
def draw_bug(d, cx, cy, r, body, dark, flip=1, dead=False):
    # legs first (behind the body)
    for i, a in enumerate((-0.9, -0.45, 0.0, 0.45, 0.9)):
        for sgn in (-1, 1):
            x0 = cx + sgn * r * 0.72
            y0 = cy + r * 0.28 + i * 2
            x1 = cx + sgn * (r * 1.12)
            y1 = cy + r * (0.62 + 0.16 * abs(a))
            d.line(sc([x0, y0, x1, y1]), fill=OUT, width=int(r * 0.13))
    # antennae
    for sgn in (-1, 1):
        x0 = cx + sgn * r * 0.34; y0 = cy - r * 0.78
        x1 = cx + sgn * r * 0.62; y1 = cy - r * 1.24
        d.line(sc([x0, y0, x1, y1]), fill=OUT, width=int(r * 0.11))
        d.ellipse(sc([x1 - r * 0.11, y1 - r * 0.11, x1 + r * 0.11, y1 + r * 0.11]), fill=dark, outline=OUT, width=int(r * 0.07))
    # segmented body: rear segment, main dome, face plate
    d.ellipse(sc([cx - r * 1.02, cy - r * 0.52, cx + r * 0.30, cy + r * 0.78]), fill=dark, outline=OUT, width=int(r * 0.10))
    d.ellipse(sc([cx - r * 0.86, cy - r * 0.88, cx + r * 0.86, cy + r * 0.82]), fill=body, outline=OUT, width=int(r * 0.10))
    d.arc(sc([cx - r * 0.62, cy - r * 0.66, cx + r * 0.62, cy + r * 0.55]), 300, 60, fill=dark, width=int(r * 0.09))
    if dead:
        # X eyes
        for ex in (cx - r * 0.34, cx + r * 0.34):
            ey = cy - r * 0.18; e = r * 0.15
            d.line(sc([ex - e, ey - e, ex + e, ey + e]), fill=OUT, width=int(r * 0.10))
            d.line(sc([ex - e, ey + e, ex + e, ey - e]), fill=OUT, width=int(r * 0.10))
        d.arc(sc([cx - r * 0.26, cy + r * 0.28, cx + r * 0.26, cy + r * 0.62]), 200, 340, fill=OUT, width=int(r * 0.08))
    else:
        # big eyes, pupils glancing toward center stage
        for ex in (cx - r * 0.34, cx + r * 0.34):
            d.ellipse(sc([ex - r * 0.21, cy - r * 0.44, ex + r * 0.21, cy - r * 0.02]), fill=(255, 255, 255, 255), outline=OUT, width=int(r * 0.07))
            px = ex + flip * r * 0.06
            d.ellipse(sc([px - r * 0.09, cy - r * 0.33, px + r * 0.09, cy - r * 0.15]), fill=OUT)
        # grin with two fangs
        d.arc(sc([cx - r * 0.36, cy + r * 0.02, cx + r * 0.36, cy + r * 0.52]), 15, 165, fill=OUT, width=int(r * 0.08))
        for fx in (cx - r * 0.20, cx + r * 0.20):
            d.polygon(sc([fx - r * 0.07, cy + r * 0.30, fx + r * 0.07, cy + r * 0.30, fx, cy + r * 0.48]), fill=(255, 255, 255, 255), outline=OUT)

# ---------------------------------------------------------------- one carcass (hat layer)
def draw_carcass(w, body, dark, angle, dx):
    lay = new_layer()
    d = ImageDraw.Draw(lay)
    cx, cy = 512, 512
    # stubby legs sticking up/out (it's upside down up there)
    for sgn in (-1, 1):
        for k in range(3):
            x = cx + sgn * (w * 0.16 + k * w * 0.13)
            d.line(sc([x, cy - 26, x + sgn * 14, cy - 58]), fill=OUT, width=9 * S)
    d.ellipse(sc([cx - w / 2, cy - 34, cx + w / 2, cy + 34]), fill=body, outline=OUT, width=10 * S)
    d.arc(sc([cx - w / 2 + 26, cy - 30, cx + w / 2 - 26, cy + 30]), 300, 60, fill=dark, width=8 * S)
    # X eyes on the belly
    for ex in (cx - w * 0.16, cx + w * 0.16):
        e = 13
        d.line(sc([ex - e, cy - e, ex + e, cy + e]), fill=OUT, width=8 * S)
        d.line(sc([ex - e, cy + e, ex + e, cy - e]), fill=OUT, width=8 * S)
    lay = lay.rotate(angle, resample=Image.BICUBIC, center=(512 * S, 512 * S))
    return lay, dx

# ---------------------------------------------------------------- the hero
# The hero and his flanking bugs are the game's ACTUAL sprites, captured
# from a live run (android/art/, see cutout.py): the real DEV at his real
# in-game size, wearing a 240-carcass hat. Everything is pasted at ONE
# shared scale so the proportions are exactly the game's own.
ART = os.path.join(ROOT, "android", "art")
_sprites = {}
def sprite(name):
    if name not in _sprites:
        _sprites[name] = Image.open(os.path.join(ART, name)).convert("RGBA")
    return _sprites[name]

def paste_sprite(img, name, cx, feet, scale):
    """Paste a captured sprite: center x = cx, bottom = feet (logical
    1024-space), magnified by `scale` with nearest-neighbour (chunky pixels
    are the point — it's the real sprite)."""
    sp = sprite(name)
    tw = max(1, int(sp.width * scale * S)); th = max(1, int(sp.height * scale * S))
    img.alpha_composite(sp.resize((tw, th), Image.NEAREST),
                        (int(cx * S - tw / 2), int(feet * S - th)))

def soft_shadow(img, cx, cy, w, h):
    lay = new_layer()
    ImageDraw.Draw(lay).ellipse(sc([cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2]),
                                fill=(16, 26, 12, 130))
    img.alpha_composite(lay.filter(ImageFilter.GaussianBlur(7 * S)))

SPR_SCALE = 2.05   # the hero stays tiny, like he is in the actual game

def draw_art(img):
    d = ImageDraw.Draw(img)
    # a giant bug looming in from the right edge, behind everything
    draw_bug(d, 940, 520, 120, BUG_PURP, (116, 74, 162, 255), flip=-1)
    # mid-size bugs out in the world
    draw_bug(d, 150, 600, 38, BUG_ORNG, BUG_ORNG_D, flip=1)
    draw_bug(d, 868, 672, 46, BUG_GRN, (66, 156, 62, 255), flip=-1)
    # soft ground shadows
    soft_shadow(img, 512, 794, 270, 22)
    soft_shadow(img, 300, 784, 90, 14)
    soft_shadow(img, 720, 792, 70, 13)
    soft_shadow(img, 940, 636, 250, 28)
    # the actual in-game character, tiny, with his 240-bug hat
    paste_sprite(img, "hero_cutout.png", 512, 800, SPR_SCALE)
    # the two real bugs that were flanking him in the capture
    paste_sprite(img, "bug_blue.png", 300, 788, SPR_SCALE)
    paste_sprite(img, "bug_orange.png", 700, 796, SPR_SCALE)
    # one already-squashed bug by his feet
    draw_bug(d, 428, 806, 30, BUG_GRN, (66, 156, 62, 255), flip=1, dead=True)

def compose():
    bg = Image.new("RGBA", (W * S, W * S), (0, 0, 0, 255))
    draw_background(bg)
    art = new_layer()
    draw_art(art)
    full = bg.copy()
    full.alpha_composite(art)
    return full.resize((W, W), Image.LANCZOS), bg.resize((W, W), Image.LANCZOS), art.resize((W, W), Image.LANCZOS)

def main():
    full, bg, art = compose()
    master = os.path.join(ROOT, "android", "icon-master-1024.png")
    full.save(master)
    # Android legacy densities
    dens = {"mdpi": 48, "hdpi": 72, "xhdpi": 96, "xxhdpi": 144, "xxxhdpi": 192}
    for name, px in dens.items():
        path = os.path.join(ROOT, "android", "res", f"mipmap-{name}", "ic_launcher.png")
        os.makedirs(os.path.dirname(path), exist_ok=True)
        full.resize((px, px), Image.LANCZOS).save(path)
    # Android adaptive icon layers (432 = xxxhdpi anydpi foreground size)
    fg = Image.new("RGBA", (432, 432), (0, 0, 0, 0))
    art432 = art.resize((432, 432), Image.LANCZOS)
    inner = art432.resize((int(432 * 0.78), int(432 * 0.78)), Image.LANCZOS)
    fg.alpha_composite(inner, ((432 - inner.width) // 2, (432 - inner.height) // 2))
    fg_path = os.path.join(ROOT, "android", "res", "mipmap-xxxhdpi", "ic_launcher_foreground.png")
    fg.save(fg_path)
    bg.resize((432, 432), Image.LANCZOS).save(
        os.path.join(ROOT, "android", "res", "mipmap-xxxhdpi", "ic_launcher_background.png"))
    anydpi = os.path.join(ROOT, "android", "res", "mipmap-anydpi-v26")
    os.makedirs(anydpi, exist_ok=True)
    with open(os.path.join(anydpi, "ic_launcher.xml"), "w") as f:
        f.write('<?xml version="1.0" encoding="utf-8"?>\n'
                '<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">\n'
                '    <background android:drawable="@mipmap/ic_launcher_background" />\n'
                '    <foreground android:drawable="@mipmap/ic_launcher_foreground" />\n'
                '</adaptive-icon>\n')
    # macOS + Windows sources
    full.resize((512, 512), Image.LANCZOS).save(os.path.join(ROOT, "src", "macapp", "app-icon-512.png"))
    ico = full.resize((256, 256), Image.LANCZOS)
    ico.save(os.path.join(ROOT, "src", "winres", "app.ico"), format="ICO",
             sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    print("icon art written:", master)

if __name__ == "__main__":
    main()
