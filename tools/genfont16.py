#!/usr/bin/env python3
"""Rasterize Atkinson Hyperlegible Bold into the game's 16x16 FONT16 table.

Accessibility pick: Atkinson was designed for low-vision readers —
distinct I/l/1, open apertures, unambiguous letterforms. Bold face for
stroke weight at 8px. Output: font16_new.inc (C table) + preview.png.
"""
from PIL import Image, ImageDraw, ImageFont
import sys

TTF = sys.argv[1] if len(sys.argv) > 1 else "fonts/Atkinson-Bold.ttf"
SIZE = 220                      # render big, downsample by area
K = 14.7                      # 2x the 8px cell tuning (same letterforms, finer grid)                        # cell px per ascent px * SIZE scale (tuned; bumped Nov 2 2026 — Daniel: letters sat too small in their cells)
THRESH = 0.52                   # ink coverage needed to set a cell pixel (high threshold
# erodes the Bold face to a regular visual weight; full bold at 8px
# was too heavy to read — Daniel, Nov 1 2026)

font = ImageFont.truetype(TTF, SIZE)
ascent, descent = font.getmetrics()
scale = K / ascent              # cell rows per rendered px above baseline
BASE = 14.0                     # baseline sits at cell row 14 (descenders -> rows 14-15)

glyphs = []
for code in range(32, 127):
    ch = chr(code)
    AX, AY = SIZE * 2, SIZE * 2     # anchor point: left origin, baseline
    img = Image.new("L", (SIZE * 4, SIZE * 4), 0)
    d = ImageDraw.Draw(img)
    d.text((AX, AY), ch, font=font, fill=255, anchor="ls")
    bbox = img.getbbox()
    cell = [[0] * 16 for _ in range(16)]
    if bbox:
        # Wide glyphs (M, W, %, @) get squeezed individually so no
        # glyph's ink exceeds ~5.6 cells: face ink then lives in
        # cols 1..6 and the 1px outline halo never bleeds into the
        # neighbouring character (a merged black band reads worse
        # than the thin font we are replacing).
        ink_cells = (bbox[2] - bbox[0]) * scale
        xf = min(1.0, 11.2 / ink_cells) if ink_cells > 0 else 1.0
        ink_cx = (bbox[0] + bbox[2]) / 2.0     # ink centre, rendered px
        for cy in range(16):
            for cx in range(16):
                # cell pixel covers a rect in rendered space; sample it
                rx0 = ink_cx + (cx - 7.5) / (scale * xf)
                rx1 = ink_cx + (cx + 1 - 7.5) / (scale * xf)
                ry0 = (cy - BASE) / scale + AY
                ry1 = (cy + 1 - BASE) / scale + AY
                box = img.crop((int(rx0), int(ry0), int(rx1), int(ry1)))
                avg = sum(box.getdata()) / max(1, box.width * box.height) / 255.0
                cell[cy][cx] = 1 if avg >= THRESH else 0
    # despeckle (Nov 2, Daniel: "weird extra pixels"): a set cell
    # with no neighbour at all — not even a diagonal — is a stray
    # coverage artifact, not letterform. Clear it unless the whole
    # glyph is just a dot or two (periods, commas, quote marks).
    # 8-connectivity matters: at 8x8, real strokes often connect
    # only diagonally, and a 4-neighbour rule eats them.
    total = sum(sum(r) for r in cell)
    if total > 3:
        for cy2 in range(16):
            for cx2 in range(16):
                if not cell[cy2][cx2]: continue
                nb = 0
                for dy2 in (-1,0,1):
                    for dx2 in (-1,0,1):
                        if dx2==0 and dy2==0: continue
                        nx, ny = cx2+dx2, cy2+dy2
                        if 0<=nx<16 and 0<=ny<16 and cell[ny][nx]: nb+=1
                if nb==0: cell[cy2][cx2] = 0
    glyphs.append(cell)

# emit C table
out = []
for i, cell in enumerate(glyphs):
    rows = []
    for r in cell:
        b = 0
        for c in r:
            b = (b << 1) | c
        rows.append("0x%04X" % b)
    out.append(" /* %s */ {%s}," % (chr(32 + i).replace("*/", "* /"), ",".join(rows)))
with open("font16_new.inc", "w") as f:
    f.write("\n".join(out) + "\n")

# preview: charset grid + sample lines, face + outline halo like the engine
prev = Image.new("RGB", (1120, (6 * 16 + 8) * 2 + 130), (24, 22, 34))
pd = ImageDraw.Draw(prev)
def draw_glyph(img, cell, ox, oy, s, color):
    d = ImageDraw.Draw(img)
    for r in range(16):
        for c in range(16):
            if cell[r][c]:
                d.rectangle([ox + c * s, oy + r * s, ox + (c + 1) * s - 1, oy + (r + 1) * s - 1], fill=color)
S = 2
for i, cell in enumerate(glyphs):
    gx = 8 + (i % 16) * 16 * S
    gy = 8 + (i // 16) * 16 * S
    draw_glyph(prev, cell, gx, gy, S, (240, 238, 220))
lines = ["THE SURVIVAL GAME", "Squash bugs. Wear the evidence.", "0123456789 Il1 O0 $4.99 +10 XP",
         "Banzai Buddies: puppy, kitty, moose!", "Wegmans $80.27 — Chewy (adj) 37.39"]
y = (6 * 16 + 8) * S + 10
for ln in lines:
    x = 8
    for ch in ln:
        idx = ord(ch) - 32
        if 0 <= idx < 95:
            draw_glyph(prev, glyphs[idx], x, y, 2, (255, 240, 205))
        x += 16 * 2
    y += 16 * 2 + 4
prev.save("preview.png")
print("wrote font16_new.inc + preview.png")
