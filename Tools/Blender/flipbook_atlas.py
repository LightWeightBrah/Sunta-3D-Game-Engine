"""Puts the frames of Tools/Blender/flipbook_baker.py into one flipbook picture (left to right, top to bottom):
    python Tools/Blender/flipbook_atlas.py <frames folder> <out.png> [columns] [rows] [cell size]
The edges of every cell fade to nothing, so a puff never shows the square of its frame"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image

folder, out = Path(sys.argv[1]), sys.argv[2]
columns = int(sys.argv[3]) if len(sys.argv) > 3 else 8
rows = int(sys.argv[4]) if len(sys.argv) > 4 else 8
cell = int(sys.argv[5]) if len(sys.argv) > 5 else 128

frames = sorted(folder.glob("frame_*.png"))[: columns * rows]
atlas = Image.new("RGBA", (columns * cell, rows * cell), (0, 0, 0, 0))

# A round mask: full in the middle, nothing at the edge of the cell
y, x = np.mgrid[0:cell, 0:cell]
distance = np.sqrt((x - cell / 2 + 0.5) ** 2 + (y - cell / 2 + 0.5) ** 2) / (cell / 2)
mask = np.clip((1.0 - distance) / 0.18, 0.0, 1.0)

pictures = [np.asarray(Image.open(path).convert("RGBA").resize((cell, cell), Image.LANCZOS)).astype(np.float32) for path in frames]
# The brightest smoke becomes white (its shading kept): the emitter's colors then decide how light it is
solid = np.concatenate([p[..., :3][p[..., 3] > 128] for p in pictures if (p[..., 3] > 128).any()] or [np.ones((1, 3)) * 255])
brightest = max(np.percentile(solid, 99), 1.0)
for index, picture in enumerate(pictures):
    picture[..., :3] = np.clip(picture[..., :3] * (255.0 / brightest), 0, 255)
    picture[..., 3] *= mask
    atlas.paste(Image.fromarray(picture.astype(np.uint8), "RGBA"), ((index % columns) * cell, (index // columns) * cell))

Path(out).parent.mkdir(parents=True, exist_ok=True)
atlas.save(out)
print(out, len(frames), "frames")
