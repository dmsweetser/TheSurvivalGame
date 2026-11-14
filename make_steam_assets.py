#!/usr/bin/env python3
"""Build all Steam store/library assets for THE SURVIVAL GAME from real
gameplay captures. Title treatment (Daniel, Oct 29): Comic Sans style —
Comic Neue Bold (SIL OFL, the open Comic Sans revival) — thickened with a
dark outline, sitting on a clean dark banner like the game's splash
screen, instead of fighting the screenshot's HUD text."""
import os
from PIL import Image, ImageDraw, ImageEnhance, ImageFont

RAW = os.path.expanduser("~/workspace/survival-game/shots-1080")
OUT = os.path.expanduser("~/workspace/your_files/steam-publish-kit/assets")
FONT = os.path.expanduser("~/workspace/survival-game/fonts/ComicNeue-Bold.ttf")
os.makedirs(OUT, exist_ok=True)

GOLD = (255, 214, 64)
DARK = (10, 8, 20)
PANEL = (16, 18, 32)

def fit_font(text, max_w, start):
    """Largest Comic Neue Bold size (<= start) that fits text in max_w."""
    size = start
    while size > 10:
        f = ImageFont.truetype(FONT, size)
        if f.getlength(text) <= max_w: return f, size
        size -= 2
    return ImageFont.truetype(FONT, 10), 10

def draw_title(img, lines, band_h, max_size):
    """Dark banner across the top with a gold rule, title in thick
    outlined Comic Sans centered on it."""
    d = ImageDraw.Draw(img, "RGBA")
    d.rectangle([0, 0, img.width, band_h], fill=PANEL + (242,))
    d.rectangle([0, band_h, img.width, band_h + max(3, band_h // 28)], fill=GOLD + (255,))
    text = max(lines, key=len)
    f, size = fit_font(text, img.width * 0.92, max_size)
    line_h = size * 1.12
    total = line_h * len(lines)
    y = (band_h - total) / 2 - size * 0.08
    for ln in lines:
        d.text((img.width / 2, y), ln, font=f, fill=GOLD, anchor="ma",
               stroke_width=max(2, size // 13), stroke_fill=DARK)
        y += line_h
    return img

def cover(img, W, H, cx_frac=0.5, cy_frac=0.5, zoom=1.0):
    s = max(W / img.width, H / img.height) * zoom
    im = img.resize((round(img.width * s), round(img.height * s)), Image.LANCZOS)
    cx = int(im.width * cx_frac - W / 2); cy = int(im.height * cy_frac - H / 2)
    cx = max(0, min(cx, im.width - W)); cy = max(0, min(cy, im.height - H))
    return im.crop((cx, cy, cx + W, cy + H))

def load(name): return Image.open(os.path.join(RAW, name)).convert("RGB")
def save(img, name):
    img.save(os.path.join(OUT, name))
    print(f"{name}: {img.width}x{img.height}")

TITLE = "THE SURVIVAL GAME"

# ---- screenshots: 1920x1080, straight gameplay captures (no text added) ----
shots = [("raw-trek-13.png", "screenshot-1-legacy-lands-swarm.png"),
         ("raw-trek-5.png",  "screenshot-2-spaghetti-fields.png"),
         ("raw-trek-11.png", "screenshot-3-the-trek-east.png"),
         ("raw-store.png",   "screenshot-4-bug-bounty-store.png"),
         ("raw-spawn.png",   "screenshot-5-the-epicenter.png"),
         ("raw-trek-7.png",  "screenshot-6-bug-cap.png")]
for raw, name in shots:
    save(load(raw).resize((1920, 1080), Image.LANCZOS), name)

# ---- main capsule 1232x706 ----
im = cover(load("raw-trek-13.png"), 1232, 706, 0.47, 0.45, zoom=1.05)
save(draw_title(im, [TITLE], 148, 118), "capsule-main-1232x706.png")

# ---- header capsule 920x430 ----
im = cover(load("raw-trek-5.png"), 920, 430, 0.5, 0.42, zoom=1.05)
save(draw_title(im, [TITLE], 104, 84), "capsule-header-920x430.png")

# ---- small capsule 462x174 ----
im = cover(load("raw-trek-5.png"), 462, 174, 0.5, 0.5, zoom=1.05)
save(draw_title(im, [TITLE], 66, 44), "capsule-small-462x174.png")

# ---- vertical capsule 748x896 ----
im = cover(load("raw-trek-13.png"), 748, 896, 0.47, 0.5, zoom=1.05)
save(draw_title(im, ["THE SURVIVAL", "GAME"], 216, 96), "capsule-vertical-748x896.png")

# ---- library capsule 600x900 ----
im = cover(load("raw-trek-5.png"), 600, 900, 0.52, 0.5, zoom=1.05)
save(draw_title(im, ["THE SURVIVAL", "GAME"], 188, 80), "library-capsule-600x900.png")

# ---- library hero 3840x1240: artwork only, NO text ----
save(cover(load("raw-trek-13.png"), 3840, 1240, 0.47, 0.45, zoom=1.05), "library-hero-3840x1240.png")

# ---- library logo 1280x720: the title alone on transparency ----
logo = Image.new("RGBA", (1280, 720), (0, 0, 0, 0))
d = ImageDraw.Draw(logo)
f, size = fit_font(TITLE, 1280 * 0.92, 150)
d.text((640, 360), TITLE, font=f, fill=GOLD, anchor="mm",
       stroke_width=max(3, size // 13), stroke_fill=DARK)
save(logo, "library-logo-1280x720.png")

# ---- icons from the big spider in trek-13 (bottom-left) ----
base = load("raw-trek-13.png")
spider = base.crop((110, 660, 290, 840)).resize((256, 256), Image.LANCZOS)
save(spider, "icon-256.png")
save(spider.resize((184, 184), Image.LANCZOS), "icon-community-184.png")

# ---- optional page background 1438x810 ----
bg = ImageEnhance.Brightness(load("raw-menu.png").resize((1438, 810), Image.LANCZOS)).enhance(0.5)
save(bg, "page-background-1438x810.png")
print("done")
