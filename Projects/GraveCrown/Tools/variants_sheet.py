"""Puts the concept variants side by side with their names, one sheet per group, for the user to choose from"""
import os, sys
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
VARIANTS = os.path.normpath(os.path.join(HERE, "..", "..", "..", "Tools", "Blender", "Concepts", "ThirdGame", "Variants"))
GROUPS = {"1_player_horse": ["player", "horse"], "2_enemies": ["enemy"], "3_court_companion": ["court", "companion"],
          "4_titan_weapons": ["titan", "weapon"]}
CELL = 360

def sheet(name, prefixes):
    files = sorted(f for f in os.listdir(VARIANTS) if f.endswith(".png") and any(f.startswith(p) for p in prefixes))
    columns = min(5, len(files))
    rows = (len(files) + columns - 1) // columns
    out = Image.new("RGB", (columns * CELL, rows * (CELL + 34)), (28, 26, 30))
    draw = ImageDraw.Draw(out)
    font = ImageFont.truetype("arial.ttf", 22)
    for i, f in enumerate(files):
        picture = Image.open(os.path.join(VARIANTS, f)).convert("RGB")
        picture.thumbnail((CELL - 8, CELL - 8))
        x, y = (i % columns) * CELL, (i // columns) * (CELL + 34)
        out.paste(picture, (x + (CELL - picture.width) // 2, y + 4))
        draw.text((x + 8, y + CELL + 4), f[:-4], fill=(235, 220, 190), font=font)
    path = os.path.join(VARIANTS, "sheet_" + name + ".jpg")
    out.save(path, quality=88)
    print(path)

for group, prefixes in GROUPS.items():
    sheet(group, prefixes)
