"""
Grave Crown's plants for the engine's Foliage (res/Models/Foliage/): tufts of ash grass, tall dead grass, pale grave
flowers with violet heads, a thistle - each a few crossed cards with a picture of blades on a transparent ground - and
the density mask that keeps them off the old road, the camps, the bonfire and the arena (res/Terrain/grass_mask.png)

    python Projects/GraveCrown/Tools/foliage_generator.py

No Blender: the pictures are drawn with Pillow (four times bigger, scaled down) and the glTF files written directly
"""

import base64
import json
import math
import os
import struct
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
RES = os.path.normpath(os.path.join(HERE, "..", "res"))
FOLIAGE = os.path.join(RES, "Models", "Foliage")
sys.path.insert(0, HERE)
import terrain_map  # noqa: E402

SS = 4


# ========== Pictures ==========

def blades(name, size, count, height_range, colors, lean=0.25, width=(3, 6), seed=1, heads=None, curl=0.0):
    """Blades growing from the bottom edge of the picture, each a tapered curved stroke; 'heads' adds a flower or a
    seed head on some of them: (chance, color, radius)"""
    w, h = size
    W, H = w * SS, h * SS
    rng = np.random.default_rng(seed)
    image = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    for _ in range(count):
        x0 = rng.uniform(0.15, 0.85) * W
        height = rng.uniform(*height_range) * H
        bend = rng.uniform(-lean, lean) * height
        base_w = rng.uniform(*width) * SS
        color = np.array(colors[rng.integers(len(colors))], dtype=float) * rng.uniform(0.75, 1.15)
        steps = 14
        left, right = [], []
        for k in range(steps + 1):
            t = k / steps
            x = x0 + bend * t * t + math.sin(t * 3.0) * curl * SS
            y = H - t * height
            half = base_w * (1.0 - t) * 0.5 + 0.4 * SS
            left.append((x - half, y))
            right.append((x + half, y))
        tip = right[-1]
        shade = tuple(int(c) for c in np.clip(color, 0, 255)) + (255,)
        draw.polygon(left + right[::-1], fill=shade)
        if heads and rng.random() < heads[0]:
            r = heads[2] * SS * rng.uniform(0.8, 1.2)
            hc = tuple(int(c) for c in np.clip(np.array(heads[1]) * rng.uniform(0.85, 1.15), 0, 255)) + (255,)
            draw.ellipse([tip[0] - r, tip[1] - r * 1.4, tip[0] + r, tip[1] + r * 0.6], fill=hc)
    # darker at the roots (ground shade), lighter at the tips
    a = np.asarray(image, dtype=np.float32)
    ys = np.linspace(0.55, 1.1, H)[::-1][:, None]
    a[..., :3] *= ys[..., None] if a.ndim == 3 else 1.0
    # Scaled down with the colors weighted by their alpha (a plain resize of an RGBA picture tints the edges)
    alpha = a[..., 3:4] / 255.0
    premultiplied = np.concatenate([a[..., :3] * alpha, a[..., 3:4]], axis=2)
    small = np.asarray(Image.fromarray(np.clip(premultiplied, 0, 255).astype(np.uint8), "RGBA").resize((w, h), Image.BOX), dtype=np.float32)
    small_alpha = small[..., 3:4] / 255.0
    color = small[..., :3] / np.maximum(small_alpha, 1e-3)
    # Bleed the colors into the transparent pixels (no dark fringe when the texture is filtered)
    rgb = Image.fromarray(np.clip(color, 0, 255).astype(np.uint8), "RGB").filter(ImageFilter.MaxFilter(5))
    covered = small_alpha[..., 0] > 0.02
    rgb_a = np.asarray(rgb, dtype=np.float32)
    rgb_a[covered] = np.clip(color, 0, 255)[covered]
    out = Image.fromarray(np.dstack([rgb_a, small[..., 3]]).astype(np.uint8), "RGBA")
    path = os.path.join(FOLIAGE, name, name + "_diffuse.png")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    out.save(path)
    return name + "_diffuse.png"


# ========== Crossed cards as glTF ==========

def write_tuft(name, texture, width, height, cards=3):
    """'cards' upright quads through the middle, turned evenly, double sided by the engine (no culling); the normals
    point up and a little out so the tuft is lit like the ground, not like flat boards"""
    positions, normals, uvs, indices = [], [], [], []
    for c in range(cards):
        a = math.pi * c / cards
        dx, dz = math.cos(a) * width * 0.5, math.sin(a) * width * 0.5
        base = len(positions)
        for (x, z, u), (y, v) in [((-dx, -dz, 0.0), (0.0, 1.0)), ((dx, dz, 1.0), (0.0, 1.0)),
                                  ((dx, dz, 1.0), (height, 0.0)), ((-dx, -dz, 0.0), (height, 0.0))]:
            positions.append((x, y, z))
            out = np.array([-math.sin(a), 1.6, math.cos(a)])
            out /= np.linalg.norm(out)
            normals.append(tuple(out))
            uvs.append((u, v))
        indices += [base, base + 1, base + 2, base, base + 2, base + 3]

    blob = b"".join(struct.pack("<3f", *p) for p in positions) + b"".join(struct.pack("<3f", *n) for n in normals) + \
        b"".join(struct.pack("<2f", *t) for t in uvs) + b"".join(struct.pack("<I", i) for i in indices)
    n = len(positions)
    p_len, n_len, t_len, i_len = n * 12, n * 12, n * 8, len(indices) * 4
    gltf = {
        "asset": {"version": "2.0", "generator": "Grave Crown foliage_generator.py"},
        "scene": 0, "scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0, "name": name}],
        "meshes": [{"name": name, "primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2}, "indices": 3, "material": 0}]}],
        "materials": [{"name": name, "doubleSided": True, "alphaMode": "MASK",
                       "pbrMetallicRoughness": {"baseColorTexture": {"index": 0}, "metallicFactor": 0.0, "roughnessFactor": 1.0}}],
        "textures": [{"source": 0}], "images": [{"uri": texture}],
        "buffers": [{"byteLength": len(blob), "uri": name + ".bin"}],
        "bufferViews": [{"buffer": 0, "byteOffset": 0, "byteLength": p_len, "target": 34962},
                        {"buffer": 0, "byteOffset": p_len, "byteLength": n_len, "target": 34962},
                        {"buffer": 0, "byteOffset": p_len + n_len, "byteLength": t_len, "target": 34962},
                        {"buffer": 0, "byteOffset": p_len + n_len + t_len, "byteLength": i_len, "target": 34963}],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": n, "type": "VEC3",
             "min": [min(p[i] for p in positions) for i in range(3)], "max": [max(p[i] for p in positions) for i in range(3)]},
            {"bufferView": 1, "componentType": 5126, "count": n, "type": "VEC3"},
            {"bufferView": 2, "componentType": 5126, "count": n, "type": "VEC2"},
            {"bufferView": 3, "componentType": 5125, "count": len(indices), "type": "SCALAR"}],
    }
    folder = os.path.join(FOLIAGE, name)
    with open(os.path.join(folder, name + ".bin"), "wb") as out:
        out.write(blob)
    with open(os.path.join(folder, name + ".gltf"), "w", encoding="utf-8") as out:
        json.dump(gltf, out, indent=1)
    print("FOLIAGE", name, n, "vertices")


# ========== The mask ==========

def write_mask(path, pixels_x=400, pixels_z=440):
    """White where grass may grow: the plains' rolling ground; black on the old road, in the camps, around the bonfire,
    in the Warden's court and on the mountains' steep sides (the same flat places terrain_map.py flattens)"""
    mask = np.zeros((pixels_z, pixels_x), dtype=np.float32)
    for row in range(pixels_z):
        z = terrain_map.CENTER_Z - terrain_map.SIZE_Z * 0.5 + terrain_map.SIZE_Z * (row + 0.5) / pixels_z
        for column in range(pixels_x):
            x = terrain_map.CENTER_X - terrain_map.SIZE_X * 0.5 + terrain_map.SIZE_X * (column + 0.5) / pixels_x
            value = terrain_map.smooth(terrain_map.ROAD_HALF_WIDTH - 1.0, terrain_map.ROAD_HALF_WIDTH + 3.0, abs(x))
            if z > terrain_map.CRYPT_APRON_Z - 4.0:
                value = 0.0
            for fx, fz, inner, outer in terrain_map.FLAT:
                value = min(value, terrain_map.smooth(inner * 0.6, inner + 2.0, math.hypot(x - fx, z - fz)))
            # steep mountain sides stay bare
            slope = abs(terrain_map.height(x + 1.0, z) - terrain_map.height(x - 1.0, z)) + abs(terrain_map.height(x, z + 1.0) - terrain_map.height(x, z - 1.0))
            value *= 1.0 - terrain_map.smooth(0.8, 1.6, slope)
            mask[row, column] = value
    # patches: thicker in hollows, thinner on rises (a slow noise)
    xs = np.linspace(0, 1, pixels_x)[None, :]
    zs = np.linspace(0, 1, pixels_z)[:, None]
    patches = 0.55 + 0.45 * np.sin(xs * 23.0 + zs * 7.0) * np.sin(zs * 19.0 - xs * 5.0)
    mask *= np.clip(patches, 0.15, 1.0)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    Image.fromarray((mask * 255).astype(np.uint8)).save(path)
    print("MASK", path, "{:.0%} covered".format(float(mask.mean())))


def main():
    ash = [(118, 108, 92), (96, 88, 80), (132, 118, 96), (84, 78, 74)]
    tex = blades("grass_ash", (256, 256), 46, (0.35, 0.95), ash, lean=0.3, width=(4, 8), seed=3)
    write_tuft("grass_ash", tex, 0.9, 0.55)
    dead = [(92, 74, 52), (110, 90, 60), (70, 58, 46)]
    tex = blades("grass_dead", (256, 512), 30, (0.5, 0.98), dead, lean=0.35, width=(4, 7), seed=7, heads=(0.45, (150, 128, 92), 4), curl=3)
    write_tuft("grass_dead", tex, 0.8, 1.1)
    stalk = [(88, 92, 78), (76, 82, 70)]
    tex = blades("grave_flowers", (256, 384), 16, (0.45, 0.95), stalk, lean=0.2, width=(3, 4), seed=11, heads=(0.85, (176, 112, 228), 9))
    write_tuft("grave_flowers", tex, 0.6, 0.7)
    thistle = [(70, 74, 66), (60, 66, 60)]
    tex = blades("thistle", (256, 384), 10, (0.6, 0.98), thistle, lean=0.15, width=(5, 8), seed=19, heads=(0.9, (120, 70, 110), 12))
    write_tuft("thistle", tex, 0.7, 0.9)
    write_mask(os.path.join(RES, "Terrain", "grass_mask.png"))


if __name__ == "__main__":
    main()
