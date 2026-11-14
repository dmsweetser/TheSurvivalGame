#!/usr/bin/env python3
"""The Survival Game — ANDROID launcher icon: ONE ANGRY BUG.

Daniel (Nov 3, 2026): the Android logo/frontmatter becomes a single
angry bug. Drawn from scratch in the game's flat cartoon style (same
shapes/palette as android/make_icon.py's draw_bug): a round orange bug
filling the frame on the game's grass checker, with slanted brow bars,
narrowed glaring eyes, and a gritted-teeth grimace.

Outputs (Android only — mac/win icon sources still come from
make_icon.py; note that re-running make_icon.py will overwrite these
Android files with the old hero scene):
  - android/icon-master-1024.png          (new master)
  - android/res/mipmap-*/ic_launcher.png  (legacy, all 5 densities)
  - android/res/mipmap-xxxhdpi/ic_launcher_foreground.png  (adaptive)
  - android/res/mipmap-xxxhdpi/ic_launcher_background.png  (adaptive)
Run: python3 android/make_icon_android.py
"""
from PIL import Image, ImageDraw
import math, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
S = 2                      # supersample

OUT    = (26, 24, 40, 255)
GRASS_L = (112, 198, 62, 255)
GRASS_D = (100, 184, 52, 255)
BODY   = (237, 125, 45, 255)    # the game's orange bug
BODY_D = (198, 100, 30, 255)
WHITE  = (255, 255, 255, 255)

def sc(box):  # logical -> device coords
    return [v * S for v in box]

# ---------------------------------------------------------------- background
def draw_background(img, W):
    d = ImageDraw.Draw(img)
    d.rectangle(sc([0, 0, W, W]), fill=GRASS_D)
    sq = W // 8
    for gy in range(0, 9):
        for gx in range(0, 9):
            if (gx + gy) % 2 == 0:
                d.rectangle(sc([gx * sq, gy * sq, (gx + 1) * sq, (gy + 1) * sq]), fill=GRASS_L)
    # a few grass tufts near the edges (kept clear of the bug's face)
    for (tx, ty) in [(70, 830), (150, 950), (880, 880), (800, 960), (60, 680), (945, 700),
                     (120, 190), (905, 150), (480, 70), (55, 420), (955, 430)]:
        tx, ty = tx * W // 1024, ty * W // 1024
        for k in (-1, 0, 1):
            d.line(sc([tx + k * 12 * W // 1024, ty, tx + k * 20 * W // 1024, ty - 30 * W // 1024]),
                   fill=(74, 148, 40, 255), width=max(2, 6 * S * W // 1024))

# ---------------------------------------------------------------- the angry bug
def draw_angry_bug(d, cx, cy, r):
    """One front-facing bug, FURIOUS. (cx, cy) = body center, r = body radius."""
    w_out = max(2, int(r * 0.09))
    # legs (behind the body): three per side, splayed down and out
    for sgn in (-1, 1):
        for i, ang in enumerate((28, 55, 82)):
            a = math.radians(ang)
            x0 = cx + sgn * math.cos(a) * r * 0.88
            y0 = cy + math.sin(a) * r * 0.80
            x1 = cx + sgn * math.cos(a) * r * 1.22
            y1 = cy + math.sin(a) * r * 1.06
            d.line(sc([x0, y0, x1, y1]), fill=OUT, width=int(r * 0.13))
    # antennae, swept up and out, ball tips
    for sgn in (-1, 1):
        x0 = cx + sgn * r * 0.34; y0 = cy - r * 0.78
        x1 = cx + sgn * r * 0.58; y1 = cy - r * 1.12
        d.line(sc([x0, y0, x1, y1]), fill=OUT, width=int(r * 0.11))
        br = r * 0.105
        d.ellipse(sc([x1 - br, y1 - br, x1 + br, y1 + br]), fill=BODY_D, outline=OUT, width=int(r * 0.06))
    # rear segment peeking out below the dome
    d.ellipse(sc([cx - r * 1.00, cy - r * 0.30, cx + r * 1.00, cy + r * 1.02]),
              fill=BODY_D, outline=OUT, width=w_out)
    # main dome
    d.ellipse(sc([cx - r, cy - r * 0.94, cx + r, cy + r * 0.94]),
              fill=BODY, outline=OUT, width=w_out)
    # shell segment arc
    d.arc(sc([cx - r * 0.62, cy - r * 0.70, cx + r * 0.62, cy + r * 0.52]),
          300, 60, fill=BODY_D, width=int(r * 0.07))
    # --- ANGRY eyes: squashed whites, pupils glaring up under the brows
    ey = cy - r * 0.16
    for sgn in (-1, 1):
        ex = cx + sgn * r * 0.36
        d.ellipse(sc([ex - r * 0.205, ey - r * 0.185, ex + r * 0.205, ey + r * 0.165]),
                  fill=WHITE, outline=OUT, width=int(r * 0.06))
        px = ex - sgn * r * 0.035          # pupils pulled toward center…
        py = ey - r * 0.045                # …and up: a proper glare
        pr = r * 0.088
        d.ellipse(sc([px - pr, py - pr, px + pr, py + pr]), fill=OUT)
    # --- brow bars: thick, slamming down toward the center
    bw_ = int(r * 0.155)
    for sgn in (-1, 1):
        x0 = cx + sgn * r * 0.66; y0 = cy - r * 0.545
        x1 = cx + sgn * r * 0.08; y1 = cy - r * 0.315
        d.line(sc([x0, y0, x1, y1]), fill=OUT, width=bw_)
        for (exx, eyy) in ((x0, y0), (x1, y1)):
            d.ellipse(sc([exx - bw_ / 2, eyy - bw_ / 2, exx + bw_ / 2, eyy + bw_ / 2]), fill=OUT)
    # --- gritted teeth: white grimace slab, tooth seams, two up-fangs
    mx0, my0 = cx - r * 0.42, cy + r * 0.30
    mx1, my1 = cx + r * 0.42, cy + r * 0.56
    d.rounded_rectangle(sc([mx0, my0, mx1, my1]), radius=r * 0.075, fill=WHITE)
    midy = (my0 + my1) / 2
    d.line(sc([mx0 + r * 0.03, midy, mx1 - r * 0.03, midy]), fill=OUT, width=int(r * 0.05))
    for fx in (cx - r * 0.14, cx, cx + r * 0.14):
        d.line(sc([fx, my0 + r * 0.02, fx, my1 - r * 0.02]), fill=OUT, width=int(r * 0.05))
    for sgn in (-1, 1):   # fangs biting UP from the bottom jaw
        fx = cx + sgn * r * 0.29
        d.polygon(sc([fx - r * 0.055, my1 - r * 0.01, fx + r * 0.055, my1 - r * 0.01,
                      fx, my1 - r * 0.115]), fill=WHITE, outline=OUT, width=int(r * 0.035))
    d.rounded_rectangle(sc([mx0, my0, mx1, my1]), radius=r * 0.075, outline=OUT, width=int(r * 0.065))

# ---------------------------------------------------------------- compose
def compose(W, bug_r, with_bg=True):
    img = Image.new("RGBA", (W * S, W * S), (0, 0, 0, 0) if not with_bg else (0, 0, 0, 255))
    if with_bg:
        draw_background(img, W)
    d = ImageDraw.Draw(img)
    draw_angry_bug(d, W / 2, W / 2 + bug_r * 0.06, bug_r)
    return img.resize((W, W), Image.LANCZOS)

def main():
    # master + legacy: bug big and bold (body = 64% of the frame)
    full = compose(1024, 330)
    master = os.path.join(ROOT, "android", "icon-master-1024.png")
    full.save(master)
    dens = {"mdpi": 48, "hdpi": 72, "xhdpi": 96, "xxhdpi": 144, "xxxhdpi": 192}
    for name, px in dens.items():
        path = os.path.join(ROOT, "android", "res", f"mipmap-{name}", "ic_launcher.png")
        os.makedirs(os.path.dirname(path), exist_ok=True)
        full.resize((px, px), Image.LANCZOS).save(path)
    # adaptive background: the grass checker, full bleed at xxxhdpi anydpi size
    bg = Image.new("RGBA", (432 * S, 432 * S), (0, 0, 0, 255))
    draw_background(bg, 432)
    bg.resize((432, 432), Image.LANCZOS).save(
        os.path.join(ROOT, "android", "res", "mipmap-xxxhdpi", "ic_launcher_background.png"))
    # adaptive foreground: bug alone on transparency, sized so the whole
    # silhouette (antenna tips included) stays inside the safe zone
    fg = compose(432, 112, with_bg=False)
    fg.save(os.path.join(ROOT, "android", "res", "mipmap-xxxhdpi", "ic_launcher_foreground.png"))
    # preview for a human verdict
    full.resize((512, 512), Image.LANCZOS).save(
        os.path.join(ROOT, "tests", "android-icon-preview.png"))
    print("angry bug icon written:", master)

if __name__ == "__main__":
    main()
