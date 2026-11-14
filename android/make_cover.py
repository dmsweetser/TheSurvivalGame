#!/usr/bin/env python3
"""itch.io storefront art for The Survival Game: a designed cover
(630x500) and a wide description banner, both built from the icon
scene with a gradient-and-glow title treatment, plus a subtle
page-background grass tile. Reuses the icon painter from make_icon.py.
Run: python3 android/make_cover.py
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import make_icon as ic

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "android", "storefront")
os.makedirs(OUT, exist_ok=True)
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
DARK = (20, 18, 31)
GOLD_STOPS = [(0.0, (255, 248, 205)), (0.45, (255, 214, 64)), (1.0, (224, 138, 26))]
ICY = (170, 226, 248)

def scene():
    bg = Image.new("RGBA", (2048, 2048), (0, 0, 0, 255))
    ic.draw_background(bg)
    art = ic.new_layer()
    ic.draw_art(art)
    bg.alpha_composite(art)
    return bg.resize((1024, 1024), Image.LANCZOS)

def darken_left(img, width, strength=238):
    """Fade the left `width` px of img into the page dark, for title text."""
    w, h = img.size
    grad = Image.new("L", (width, 1))
    for x in range(width):
        grad.putpixel((x, 0), int(strength * max(0.0, 1 - x / width)))
    grad = grad.resize((width, h))
    black = Image.new("RGB", (width, h), DARK)
    img.paste(black, (0, 0), grad)
    return img

def _grad_color(stops, t):
    for i in range(len(stops) - 1):
        p0, c0 = stops[i]; p1, c1 = stops[i + 1]
        if p0 <= t <= p1:
            u = 0 if p1 == p0 else (t - p0) / (p1 - p0)
            return tuple(int(c0[k] + (c1[k] - c0[k]) * u) for k in range(3))
    return stops[-1][1]

def fancy_text(img, xy, s, size, stops=GOLD_STOPS, glow=(255, 180, 40)):
    """Display text with a hard shadow, a soft gold glow, and a vertical
    gradient fill — the storefront echo of the game's shimmering title."""
    f = ImageFont.truetype(FONT, size)
    x, y = xy
    d = ImageDraw.Draw(img)
    d.text((x + size // 18 + 2, y + size // 16 + 2), s, font=f, fill=(14, 12, 26))
    lay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    ImageDraw.Draw(lay).text((x, y), s, font=f, fill=glow + (255,))
    img.alpha_composite(lay.filter(ImageFilter.GaussianBlur(max(2, size // 12))))
    img.alpha_composite(lay.filter(ImageFilter.GaussianBlur(max(2, size // 12))))
    m = Image.new("L", img.size, 0)
    ImageDraw.Draw(m).text((x, y), s, font=f, fill=255)
    bbox = m.getbbox()
    if not bbox: return
    x0, y0, x1, y1 = bbox
    grad = Image.new("RGB", (1, max(1, y1 - y0)))
    for yy in range(y1 - y0):
        grad.putpixel((0, yy), _grad_color(stops, yy / max(1, y1 - y0 - 1)))
    grad = grad.resize((x1 - x0, y1 - y0))
    img.paste(grad, (x0, y0), m.crop((x0, y0, x1, y1)))

def plain_text(img, xy, s, size, fill):
    d = ImageDraw.Draw(img)
    f = ImageFont.truetype(FONT, size)
    d.text((xy[0] + 2, xy[1] + 3), s, font=f, fill=(14, 12, 26))
    d.text(xy, s, font=f, fill=fill)

def flourish(img, x, y, w):
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([x, y - 3, x + w, y + 3], radius=3, fill=(255, 205, 40))
    cx = x + w + 16
    d.polygon([(cx, y - 8), (cx + 8, y), (cx, y + 8), (cx - 8, y)], fill=(255, 205, 40))

def main():
    sc = scene()
    # ---- cover (built at 2x, shipped 630x500)
    cover = Image.new("RGBA", (1260, 1000), DARK + (255,))
    cover.paste(sc.resize((960, 960), Image.LANCZOS), (430, 40))
    cover = darken_left(cover.convert("RGB"), 900, 205).convert("RGBA")
    plain_text(cover, (64, 88), "T H E", 72, (255, 205, 40))
    fancy_text(cover, (58, 172), "SURVIVAL", 148)
    fancy_text(cover, (58, 332), "GAME", 148)
    flourish(cover, 64, 512, 340)
    plain_text(cover, (64, 548), "SQUASH BUGS.", 54, ICY)
    plain_text(cover, (64, 616), "WEAR THE EVIDENCE.", 54, ICY)
    plain_text(cover, (64, 706), "HOW HEAVY CAN YOU GO?", 44, (255, 205, 40))
    cover.convert("RGB").resize((630, 500), Image.LANCZOS).save(os.path.join(OUT, "cover-630x500.png"))

    # ---- wide banner for the top of the description
    ban = Image.new("RGBA", (1240, 620), DARK + (255,))
    ban.paste(sc.resize((620, 620), Image.LANCZOS), (700, 0))
    ban = darken_left(ban.convert("RGB"), 860, 215).convert("RGBA")
    plain_text(ban, (56, 50), "T H E", 48, (255, 205, 40))
    fancy_text(ban, (52, 104), "SURVIVAL", 102)
    fancy_text(ban, (52, 218), "GAME", 102)
    flourish(ban, 56, 344, 250)
    plain_text(ban, (56, 368), "SQUASH BUGS - WEAR THE EVIDENCE", 33, ICY)
    plain_text(ban, (56, 440), "A tiny arcade survival game", 29, (205, 208, 224))
    plain_text(ban, (56, 484), "about the world's most dangerous", 29, (205, 208, 224))
    plain_text(ban, (56, 528), "fashion accessory.", 29, (205, 208, 224))
    ban.convert("RGB").save(os.path.join(OUT, "banner-1240x620.png"))

    # ---- page background tile: the game's grass, darkened way down
    tile = Image.new("RGB", (256, 256), (23, 34, 17))
    dt = ImageDraw.Draw(tile)
    for gy in range(2):
        for gx in range(2):
            if (gx + gy) % 2 == 0:
                dt.rectangle([gx * 128, gy * 128, (gx + 1) * 128, (gy + 1) * 128], fill=(27, 40, 20))
    tile.save(os.path.join(OUT, "bg-tile-256.png"))
    print("storefront art written to", OUT)

if __name__ == "__main__":
    main()
