"""
Generates Grave Crown's own pieces of the world (instead of Scrap Souls' castle kit): a gothic crypt - dark slabs,
tall walls with blind pointed arches, clustered columns, ribbed vaults, a stained glass window behind the throne,
sarcophagi with stone knights on their lids, candles - and the marks of the gods' chains on the ash plains: ruined
arches, rune stones, huge chains staked into the ground, graves

Run from the repository root:
    blender --background --factory-startup --python Projects/GraveCrown/Tools/gothic_kit_generator.py -- --out Projects/GraveCrown/res/Models/Gothic
    (--preview <folder> renders every piece, --only <name> makes one)
    then: python Projects/GraveCrown/Tools/glow_maps.py   (the glass, the runes and the flames shine by themselves)

The same machinery as Scrap Souls' level kit (Tools/Blender/level_kit_generator.py): pieces made of simple shapes, a
palette of colors given the light and dark spots of CC0 photos (Poly Haven, Tools/Blender/Textures/CREDITS.md), baked
into one texture per piece. Every piece stands on its origin; Blender axes (Z up), the inner face of a wall is +Y
"""

import argparse
import math
import os
import random
import sys

import bpy

BLENDER = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "Tools", "Blender"))
sys.path.insert(0, BLENDER)

from sunta_blender import (  # noqa: E402
    Palette,
    PartBuilder,
    SurfaceTexture,
    bake_textures,
    clear_scene,
    export_gltf,
    make_matrix,
    place_preview_camera,
    setup_bake_render,
    setup_preview_render,
    show_only,
)

# A cold violet-grey stone of the dead king's realm; glowing colors saturated (glow_maps.py makes them shine)
COLORS = {
    "stone":       ((0.30, 0.285, 0.31), 0.12),
    "stone_light": ((0.40, 0.38, 0.40), 0.12),
    "stone_dark":  ((0.17, 0.16, 0.19), 0.08),
    "mortar":      ((0.09, 0.085, 0.10), 0.04),
    "ash":         ((0.36, 0.34, 0.32), 0.04),
    "iron":        ((0.16, 0.15, 0.15), 0.55),
    "bone":        ((0.62, 0.58, 0.50), 0.15),
    "wax":         ((0.72, 0.66, 0.55), 0.20),
    "flame":       ((1.00, 0.62, 0.22), 0.0, True),
    "glass_violet": ((0.55, 0.20, 0.95), 0.6, True),
    "glass_red":   ((0.85, 0.12, 0.18), 0.6, True),
    "glass_gold":  ((0.95, 0.68, 0.20), 0.6, True),
    "rune":        ((0.70, 0.30, 1.00), 0.0, True),
    "cloth":       ((0.30, 0.05, 0.07), 0.05),
}

DARK_STONE = SurfaceTexture("rock_08_diff_1k.jpg", meters=1.2, strength=1.4)
LIGHT_STONE = SurfaceTexture("rock_boulder_dry_diff_1k.jpg", meters=1.0, strength=1.3)
SURFACES = {
    "stone":       DARK_STONE,
    "stone_light": LIGHT_STONE,
    "stone_dark":  DARK_STONE,
    "mortar":      SurfaceTexture("gravel_concrete_03_diff_1k.jpg", meters=1.0),
    "ash":         SurfaceTexture("burned_ground_01_diff_1k.jpg", meters=1.0),
    "iron":        SurfaceTexture("rust_coarse_01_diff_1k.jpg", meters=0.8, strength=0.8),
    "cloth":       SurfaceTexture("brown_leather_diff_1k.jpg", meters=0.5, strength=0.6),
}

TEXTURE_PIXELS = 1024
BIG_TEXTURE_PIXELS = 2048
RANDOM_SEED = 23

WALL_LENGTH = 4.0
WALL_HEIGHT = 8.0
WALL_THICKNESS = 1.0
BLOCK_HEIGHT = 0.5
COLUMN_HEIGHT = 8.0
VAULT_SIZE = 8.0


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--preview", default="")
    parser.add_argument("--only", default="")
    return parser.parse_args(argv)


def pick_stone(generator):
    return generator.choice(("stone", "stone", "stone_light", "stone_dark"))


# ========== Shapes ==========


def pointed_arch(parts, generator, center_x, spring, span, depth, y=0.0, stones_per_side=6, stone_size=0.38, color=None):
    """A gothic (equilateral) arch over 'span': each side an arc of radius 'span' around the other side's springing
    point, wedge stones along it, from the springing line at 'spring' up to the apex"""
    half = span * 0.5
    for side in (-1.0, 1.0):
        # the left side's arc is centered at the right springing point and the other way round
        arc_center = (center_x - side * half, spring)
        start = math.pi if side < 0 else 0.0
        end = math.radians(120.0) if side < 0 else math.radians(60.0)
        for index in range(stones_per_side):
            angle = start + (end - start) * (index + 0.5) / stones_per_side
            x = arc_center[0] + math.cos(angle) * span
            z = arc_center[1] + math.sin(angle) * span
            chord = span * abs(end - start) / stones_per_side
            parts.box(None, color or pick_stone(generator), (x, y, z), (chord * 1.02, depth, stone_size),
                      rotation=(0, -math.degrees(angle) + 90, 0), bevel=0.02)
    return spring + span * math.sin(math.radians(60.0))


def block_course(parts, generator, z, x_from, x_to, depth, height, offset, y=0.0):
    x = x_from
    first = True
    while x < x_to - 0.05:
        length = offset if first else generator.uniform(0.6, 1.2)
        first = False
        length = min(length, x_to - x)
        parts.box(None, pick_stone(generator), (x + length * 0.5, y + generator.uniform(-0.015, 0.015), z + height * 0.5),
                  (length - 0.03, depth + generator.uniform(-0.03, 0.03), height - 0.03), bevel=0.03)
        x += length


def oval(parts, color, center, along, across, length_radius, width_radius, thickness, segments=12):
    """A ring stretched along 'along' (unit vector) and 'across' (unit vector): a chain link"""
    points = []
    for index in range(segments + 1):
        angle = 2.0 * math.pi * index / segments
        a, b = math.cos(angle) * length_radius, math.sin(angle) * width_radius
        points.append(tuple(center[i] + along[i] * a + across[i] * b for i in range(3)))
    for head, tail in zip(points, points[1:]):
        parts.limb(None, color, head, tail, thickness, thickness, segments=8)


def ring(parts, color, center, radius, thickness, axis="y", segments=12, scale=(1.0, 1.0)):
    """A ring of tube segments (a chain link, an iron ring) in the plane across 'axis'"""
    points = []
    for index in range(segments + 1):
        angle = 2.0 * math.pi * index / segments
        a, b = math.cos(angle) * radius * scale[0], math.sin(angle) * radius * scale[1]
        if axis == "y":
            points.append((center[0] + a, center[1], center[2] + b))
        elif axis == "x":
            points.append((center[0], center[1] + a, center[2] + b))
        else:
            points.append((center[0] + a, center[1] + b, center[2]))
    for head, tail in zip(points, points[1:]):
        parts.limb(None, color, head, tail, thickness, thickness, segments=8)


# ========== The crypt ==========


def build_floor(parts, generator):
    size, slab = 16.0, 1.33
    parts.box(None, "mortar", (0.0, 0.0, -0.17), (size, size, 0.3), bevel=0.0)
    count = int(size / slab)
    for row in range(count):
        for column in range(count):
            x = -size * 0.5 + (column + 0.5) * slab
            y = -size * 0.5 + (row + 0.5) * slab
            height = 0.12 + generator.uniform(-0.02, 0.02)
            # (mostly one dark stone: light and dark slabs side by side read as a chessboard)
            color = "ash" if generator.random() < 0.06 else ("stone" if generator.random() < 0.15 else "stone_dark")
            parts.box(None, color, (x, y, -height * 0.5), (slab - 0.05, slab - 0.05, height),
                      rotation=(generator.uniform(-0.8, 0.8), generator.uniform(-0.8, 0.8), generator.uniform(-1.5, 1.5)), bevel=0.025)


def build_wall(parts, generator):
    """Blocks to the full height, a blind pointed arch on the inner face (+Y) between two shafts, a cornice"""
    half = WALL_LENGTH * 0.5
    parts.box(None, "mortar", (0.0, 0.0, WALL_HEIGHT * 0.5), (WALL_LENGTH - 0.06, WALL_THICKNESS - 0.1, WALL_HEIGHT), bevel=0.0)
    for course in range(int(WALL_HEIGHT / BLOCK_HEIGHT)):
        block_course(parts, generator, course * BLOCK_HEIGHT, -half, half, WALL_THICKNESS, BLOCK_HEIGHT, 0.35 + 0.4 * (course % 2))

    face = WALL_THICKNESS * 0.5
    # The recess of the blind arch: a dark panel just in front of the blocks
    parts.box(None, "stone_dark", (0.0, face + 0.04, 3.4), (2.5, 0.08, 6.4), bevel=0.01)
    for side in (-1.0, 1.0):
        x = side * 1.55
        parts.box(None, "stone_dark", (x, face + 0.2, 0.3), (0.62, 0.42, 0.6), bevel=0.03)
        parts.cylinder(None, "stone_light", make_matrix((x, face + 0.2, 2.75)), 0.17, 0.16, 4.3, segments=12)
        parts.box(None, "stone", (x, face + 0.2, 5.0), (0.5, 0.38, 0.22), bevel=0.03)
    apex = pointed_arch(parts, generator, 0.0, 5.1, 3.1, 0.34, y=face + 0.2, stones_per_side=6, stone_size=0.34)
    # A small round window of tracery under the apex (dark)
    ring(parts, "stone_light", (0.0, face + 0.12, apex - 0.95), 0.42, 0.06, axis="y", segments=14)
    parts.box(None, "stone_dark", (0.0, face + 0.3, WALL_HEIGHT - 0.15), (WALL_LENGTH, 0.5, 0.3), bevel=0.03)
    parts.box(None, "stone", (0.0, face + 0.15, 0.15), (WALL_LENGTH, 0.3, 0.3), bevel=0.03)


def build_column(parts, generator):
    """A clustered gothic column: a core and four shafts, an octagonal plinth, a flaring capital"""
    parts.cylinder(None, "stone_dark", make_matrix((0.0, 0.0, 0.3)), 0.95, 0.9, 0.6, segments=8)
    parts.cylinder(None, "stone", make_matrix((0.0, 0.0, 0.72)), 0.8, 0.72, 0.24, segments=8)
    top = COLUMN_HEIGHT - 1.0
    parts.cylinder(None, "stone", make_matrix((0.0, 0.0, (0.84 + top) * 0.5)), 0.46, 0.44, top - 0.84, segments=16)
    for index in range(4):
        angle = math.radians(45.0 + 90.0 * index)
        x, y = math.cos(angle) * 0.47, math.sin(angle) * 0.47
        parts.cylinder(None, "stone_light", make_matrix((x, y, (0.84 + top) * 0.5)), 0.17, 0.16, top - 0.84, segments=10)
    parts.cylinder(None, "stone_light", make_matrix((0.0, 0.0, top + 0.3)), 0.62, 0.95, 0.6, segments=8)
    parts.cylinder(None, "stone_dark", make_matrix((0.0, 0.0, top + 0.75)), 1.0, 1.0, 0.3, segments=8)


def build_vault(parts, generator):
    """One bay of the ceiling (8 x 8 m) set on the tops of four columns: diagonal and transverse ribs arching from the
    corners up to a boss stone, a dark web above them"""
    size = VAULT_SIZE
    corner = size * 0.5
    rise = 1.6
    parts.box(None, "stone_dark", (0.0, 0.0, rise + 0.35), (size + 0.2, size + 0.2, 0.4), bevel=0.0)

    def rib(color, start, end, height, radius):
        steps = 12
        points = []
        for t in range(steps + 1):
            a = t / steps
            x = start[0] + (end[0] - start[0]) * a
            y = start[1] + (end[1] - start[1]) * a
            points.append((x, y, height * math.sin(math.pi * a) + (rise - height) * 0.0))
        for head, tail in zip(points, points[1:]):
            parts.limb(None, color, head, tail, radius, radius, segments=8)

    rib("stone_light", (-corner, -corner), (corner, corner), rise, 0.17)
    rib("stone_light", (-corner, corner), (corner, -corner), rise, 0.17)
    for sx, sy in ((1, 0), (0, 1)):
        # the transverse and wall ribs: from one column to the next along the bay's sides and through its middle
        for offset in (-corner, 0.0, corner):
            if sx:
                rib("stone", (-corner, offset), (corner, offset), rise * (0.7 if offset == 0.0 else 0.55), 0.13)
            else:
                rib("stone", (offset, -corner), (offset, corner), rise * (0.7 if offset == 0.0 else 0.55), 0.13)
    parts.cylinder(None, "stone_light", make_matrix((0.0, 0.0, rise + 0.05)), 0.42, 0.32, 0.3, segments=10)
    parts.sphere(None, "iron", (0.0, 0.0, rise - 0.12), (0.16, 0.16, 0.16))


def build_window(parts, generator):
    """The stained glass window behind the throne: a pointed frame, mullions, colored panes in lead, a rose on top"""
    span, spring, depth = 3.0, 4.4, 0.6
    for side in (-1.0, 1.0):
        for course in range(int(spring / 0.5)):
            parts.box(None, pick_stone(generator), (side * (span * 0.5 + 0.35), 0.0, course * 0.5 + 0.25), (0.7, depth, 0.47), bevel=0.03)
    parts.box(None, "stone_dark", (0.0, 0.0, 0.25), (span + 1.4, depth + 0.1, 0.5), bevel=0.03)
    apex = pointed_arch(parts, generator, 0.0, spring, span + 0.7, depth, stones_per_side=7, stone_size=0.5)
    # The glass: rows of small panes in a lead grid, colors in bands, filling the arch's outline
    pane = 0.25
    half = span * 0.5
    rows = int((apex - 0.6) / pane)
    for row in range(rows):
        z = 0.6 + (row + 0.5) * pane
        if z < spring:
            width = half
        else:
            # inside the pointed top: an equilateral arch of this span is sqrt(span^2 - h^2) - span/2 wide each side
            height = z - spring
            width = math.sqrt(max(span * span - height * height, 0.0)) - half
        columns = int(width * 2 / pane)
        for column in range(columns):
            x = -columns * pane * 0.5 + (column + 0.5) * pane
            band = (row // 3 + abs(int(x / pane))) % 5
            color = ("glass_violet", "glass_red", "glass_violet", "glass_gold", "glass_red")[band]
            if abs(x) < 0.3 and row > rows * 0.55:
                color = "glass_gold"
            parts.box(None, color, (x, 0.0, z), (pane - 0.04, 0.05, pane - 0.04), bevel=0.0)
    # Lead and the stone mullions
    parts.box(None, "iron", (0.0, -0.03, (0.6 + apex) * 0.5 - 0.2), (span, 0.04, 0.03), bevel=0.0)
    for x in (-0.75, 0.0, 0.75):
        parts.box(None, "stone_light", (x, 0.0, 0.6 + (spring - 0.2) * 0.5), (0.12, 0.2, spring - 0.2), bevel=0.02)
    ring(parts, "stone_light", (0.0, 0.0, spring + 1.0), 0.6, 0.07, axis="y", segments=16)
    for index in range(8):
        angle = 2.0 * math.pi * index / 8
        parts.limb(None, "stone_light", (0.0, 0.0, spring + 1.0),
                   (math.cos(angle) * 0.6, 0.0, spring + 1.0 + math.sin(angle) * 0.6), 0.035, 0.035, segments=6)


def build_sarcophagus(parts, generator):
    """A stone coffin with panels of arches on its sides and a knight lying on the lid, his sword on his chest"""
    length, width, height = 2.4, 1.05, 0.85
    parts.box(None, "stone_dark", (0.0, 0.0, 0.1), (length + 0.2, width + 0.2, 0.2), bevel=0.03)
    parts.box(None, "stone", (0.0, 0.0, 0.2 + height * 0.5), (length, width, height), bevel=0.03)
    for side in (-1.0, 1.0):
        for index in range(4):
            x = -length * 0.5 + (index + 0.5) * length / 4
            parts.box(None, "stone_dark", (x, side * (width * 0.5 + 0.01), 0.55), (0.42, 0.03, 0.5), bevel=0.0)
            pointed_arch(parts, generator, x, 0.75, 0.42, 0.05, y=side * (width * 0.5 + 0.02), stones_per_side=2,
                         stone_size=0.06, color="stone_light")
    lid = 0.2 + height
    parts.box(None, "stone_light", (0.0, 0.0, lid + 0.08), (length + 0.1, width + 0.1, 0.16), bevel=0.04)
    top = lid + 0.16
    # The stone knight (head at +X)
    parts.box(None, "stone", (0.95, 0.0, top + 0.06), (0.32, 0.55, 0.12), bevel=0.03)  # cushion
    parts.sphere(None, "stone_light", (0.95, 0.0, top + 0.2), (0.13, 0.12, 0.13))
    parts.box(None, "stone_light", (0.42, 0.0, top + 0.14), (0.7, 0.46, 0.2), bevel=0.06)  # chest
    parts.box(None, "stone_light", (-0.15, 0.0, top + 0.11), (0.45, 0.4, 0.16), bevel=0.05)  # hips
    for side in (-1.0, 1.0):
        parts.limb(None, "stone_light", (-0.3, side * 0.11, top + 0.1), (-1.0, side * 0.12, top + 0.08), 0.09, 0.07)
        parts.box(None, "stone", (-1.06, side * 0.12, top + 0.13), (0.1, 0.1, 0.2), bevel=0.02)
    # The sword along the body, the hands on its hilt
    parts.box(None, "iron", (0.0, 0.0, top + 0.27), (1.5, 0.07, 0.025), bevel=0.0)
    parts.box(None, "iron", (0.74, 0.0, top + 0.27), (0.04, 0.32, 0.04), bevel=0.0)
    parts.sphere(None, "stone_light", (0.84, 0.0, top + 0.28), (0.09, 0.14, 0.06))


def build_candles(parts, generator):
    """A cluster of candles on a pool of old wax, their flames shining"""
    parts.cylinder(None, "wax", make_matrix((0.0, 0.0, 0.02)), 0.5, 0.42, 0.04, segments=14)
    for index in range(9):
        angle = generator.uniform(0.0, 2.0 * math.pi)
        distance = generator.uniform(0.0, 0.38)
        x, y = math.cos(angle) * distance, math.sin(angle) * distance
        height = generator.uniform(0.12, 0.55)
        radius = generator.uniform(0.035, 0.06)
        parts.cylinder(None, "wax", make_matrix((x, y, 0.04 + height * 0.5)), radius * 1.15, radius, height, segments=10)
        parts.sphere(None, "flame", (x, y, 0.04 + height + 0.05), (0.022, 0.022, 0.05), segments=8, rings=6)


def build_throne_dais(parts, generator):
    """Three steps up to the throne, a crimson runner down them"""
    for step in range(3):
        size = 6.0 - step * 1.2
        parts.box(None, pick_stone(generator), (0.0, 0.0, 0.2 * step + 0.1), (size, size * 0.8, 0.2), bevel=0.02)
    parts.box(None, "cloth", (0.0, -1.2, 0.62), (1.3, 2.4, 0.02), bevel=0.0)


def build_portal(parts, generator):
    """The crypt's gate seen from the plains: buttressed piers, three pointed arches stepping in around a 4 m opening,
    a gable with a dark rose over it, the iron gate's two leaves swung open"""
    opening, depth = 4.0, 1.8
    for side in (-1.0, 1.0):
        # the buttresses: stepped back twice on the way up
        for course in range(20):
            z = course * 0.5
            setback = 0.0 if z < 4.0 else (0.25 if z < 7.0 else 0.45)
            parts.box(None, pick_stone(generator), (side * (opening * 0.5 + 1.2), -setback * 0.5, z + 0.25),
                      (1.5 - setback * 0.4, depth - setback, 0.47), bevel=0.03)
        parts.box(None, "stone_dark", (side * (opening * 0.5 + 1.2), -0.4, 10.2), (1.0, 0.9, 0.5), rotation=(0, side * 30, 0), bevel=0.04)
        # the jambs: three steps of shafts
        for step in range(3):
            x = side * (opening * 0.5 + 0.15 + step * 0.22)
            parts.cylinder(None, "stone_light", make_matrix((x, -depth * 0.5 + 0.2 + step * 0.25, 2.4)), 0.1, 0.09, 4.4, segments=10)
    for step in range(3):
        span = opening + 0.3 + step * 0.44
        pointed_arch(parts, generator, 0.0, 4.6, span, 0.3, y=-depth * 0.5 + 0.2 + step * 0.25, stones_per_side=7,
                     stone_size=0.3, color="stone_light" if step % 2 == 0 else "stone")
    # the tympanum's dark stone above the opening, the gable and its rose
    parts.box(None, "stone_dark", (0.0, 0.2, 7.6), (opening + 1.6, 1.0, 3.4), bevel=0.02)
    for row in range(8):
        width = (opening + 3.2) * (1.0 - row / 8.0)
        parts.box(None, pick_stone(generator), (0.0, 0.0, 9.5 + row * 0.42), (width, depth * 0.7, 0.4), bevel=0.03)
    ring(parts, "stone_light", (0.0, -depth * 0.35 - 0.05, 10.7), 0.9, 0.09, axis="y", segments=18)
    parts.cylinder(None, "mortar", make_matrix((0.0, -depth * 0.35, 10.7), (90, 0, 0)), 0.85, 0.85, 0.1, segments=18)
    for index in range(6):
        angle = 2.0 * math.pi * index / 6
        parts.limb(None, "stone_light", (0.0, -depth * 0.35 - 0.06, 10.7),
                   (math.cos(angle) * 0.85, -depth * 0.35 - 0.06, 10.7 + math.sin(angle) * 0.85), 0.05, 0.05, segments=6)
    # the gate's leaves, swung in: iron bars in a frame
    for side in (-1.0, 1.0):
        hinge = (side * opening * 0.5, depth * 0.5)
        angle = math.radians(70.0) * side
        for bar in range(6):
            t = (bar + 0.5) / 6
            x = hinge[0] - side * math.cos(angle * side) * t * opening * 0.5
            y = hinge[1] + math.sin(abs(angle)) * t * opening * 0.5
            parts.box(None, "iron", (x, y, 2.1), (0.07, 0.07, 4.2), bevel=0.0)
        for z in (0.4, 2.0, 3.8):
            mid = (hinge[0] - side * math.cos(abs(angle)) * opening * 0.25, hinge[1] + math.sin(abs(angle)) * opening * 0.25)
            parts.box(None, "iron", (mid[0], mid[1], z), (opening * 0.5, 0.06, 0.1), rotation=(0, 0, -side * 70.0), bevel=0.0)


# ========== The plains: the marks of the gods' chains ==========


def build_arch_ruin(parts, generator):
    """A freestanding pointed arch of a fallen nave: two piers, the arch over them, two of its stones fallen out"""
    span, depth, spring = 4.0, 1.0, 5.5
    for side in (-1.0, 1.0):
        courses = 11 if side < 0 else 10
        for course in range(courses):
            parts.box(None, pick_stone(generator), (side * (span * 0.5 + 0.45), 0.0, course * 0.5 + 0.25),
                      (0.9 + generator.uniform(-0.04, 0.04), depth, 0.47), bevel=0.03)
    half = span * 0.5 + 0.45
    radius = span + 0.9
    for side in (-1.0, 1.0):
        center_x = -side * half
        start = math.pi if side < 0 else 0.0
        end = math.radians(120.0) if side < 0 else math.radians(60.0)
        for index in range(6):
            if side > 0 and index in (3, 4):
                continue  # fallen out: they lie at the foot of the pier
            angle = start + (end - start) * (index + 0.5) / 6
            x = center_x + math.cos(angle) * radius
            z = spring - 0.5 + math.sin(angle) * radius
            parts.box(None, pick_stone(generator), (x, 0.0, z), (0.86, depth, 0.6), rotation=(0, -math.degrees(angle) + 90, 0), bevel=0.03)
    for _ in range(9):
        size = generator.uniform(0.3, 0.75)
        parts.box(None, pick_stone(generator), (generator.uniform(1.0, 3.6), generator.uniform(-1.3, 1.3), size * 0.3),
                  (size, size * 0.8, size * 0.6), rotation=(generator.uniform(-20, 20), generator.uniform(-20, 20), generator.uniform(0, 90)), bevel=0.04)


def build_runestone(parts, generator):
    """A tall slab of the old kings, its runes still burning violet"""
    parts.box(None, "stone_dark", (0.0, 0.0, 1.35), (1.0, 0.45, 2.7), rotation=(0, 3, 0), bevel=0.08)
    parts.box(None, "stone", (0.0, 0.0, 2.78), (0.8, 0.38, 0.25), rotation=(0, 3, 0), bevel=0.08)
    parts.box(None, "ash", (0.0, 0.0, 0.08), (1.6, 1.1, 0.16), bevel=0.05)
    for face in (-0.24, 0.24):
        carve_runes(parts, generator, face)


def carve_runes(parts, generator, face):
    z = 2.3
    while z > 0.5:
        x = generator.uniform(-0.25, 0.1)
        strokes = generator.randint(2, 4)
        for _ in range(strokes):
            length = generator.uniform(0.12, 0.26)
            angle = generator.choice((0, 45, 90, -45))
            parts.box(None, "rune", (x, face, z), (length, 0.03, 0.035), rotation=(0, angle, 0), bevel=0.0)
            x += generator.uniform(0.05, 0.14)
        z -= generator.uniform(0.2, 0.3)


def build_chain_anchor(parts, generator):
    """An iron stake driven into the ash, a ring on it and a chain of huge links rising into the sky (to a god)"""
    parts.cylinder(None, "ash", make_matrix((0.0, 0.0, 0.15)), 1.6, 1.0, 0.3, segments=12)
    parts.cylinder(None, "iron", make_matrix((0.0, 0.0, 0.9)), 0.45, 0.3, 1.5, segments=10)
    parts.cylinder(None, "iron", make_matrix((0.0, 0.0, 1.7)), 0.38, 0.38, 0.15, segments=10)
    ring(parts, "iron", (0.0, 0.0, 2.25), 0.5, 0.1, axis="y", segments=14)
    # Links one after another, every second turned across, rising at 55 degrees (oval, interlocked)
    up = math.radians(55.0)
    along = (math.cos(up), 0.0, math.sin(up))
    side = (0.0, 1.0, 0.0)
    flat = (-math.sin(up), 0.0, math.cos(up))
    for index in range(9):
        step = 0.62 * (index + 1)
        center = (along[0] * step, 0.0, 2.25 + along[2] * step)
        oval(parts, "iron", center, along, side if index % 2 == 0 else flat, 0.46, 0.27, 0.085)


def build_graves(parts, generator):
    """Five headstones with pointed tops, leaning, and a stone cross"""
    for index in range(5):
        x, y = (index - 2) * 1.2 + generator.uniform(-0.2, 0.2), generator.uniform(-0.4, 0.4)
        lean = (generator.uniform(-10, 10), generator.uniform(-12, 12), generator.uniform(-15, 15))
        height = generator.uniform(0.8, 1.25)
        parts.box(None, pick_stone(generator), (x, y, height * 0.5 - 0.1), (0.62, 0.16, height), rotation=lean, bevel=0.04)
        # the pointed top: a square turned on its corner, half of it above the slab
        top = (x + math.sin(math.radians(lean[1])) * height * 0.5, y, height - 0.1)
        parts.box(None, pick_stone(generator), top, (0.44, 0.15, 0.44), rotation=(lean[0], lean[1] + 45.0, lean[2]), bevel=0.03)
        parts.sphere(None, "ash", (x, y - 0.75, 0.0), (0.38, 0.75, 0.12))
    parts.box(None, "stone_light", (3.4, 0.3, 0.9), (0.2, 0.2, 1.9), rotation=(0, 8, 0), bevel=0.03)
    parts.box(None, "stone_light", (3.47, 0.3, 1.4), (0.85, 0.18, 0.18), rotation=(0, 8, 0), bevel=0.03)


def build_banner(parts, generator):
    """A standard of the gods' church over its camps: an iron pole, a pale cloth with torn tails, the chain's ring sewn
    on it in dark thread"""
    parts.cylinder(None, "ash", make_matrix((0.0, 0.0, 0.1)), 0.45, 0.35, 0.2, segments=10)
    parts.cylinder(None, "iron", make_matrix((0.0, 0.0, 2.2)), 0.05, 0.04, 4.4, segments=8)
    parts.box(None, "iron", (0.0, 0.0, 4.2), (1.3, 0.06, 0.06), bevel=0.0)
    parts.sphere(None, "iron", (0.0, 0.0, 4.45), (0.08, 0.08, 0.12))
    # the cloth hangs in a few slightly turned strips, the lower ones torn shorter
    for index in range(5):
        x = -0.48 + index * 0.24
        length = 2.2 - abs(index - 2) * 0.15 - generator.uniform(0.0, 0.35)
        parts.box(None, "bone", (x, 0.03 + generator.uniform(-0.02, 0.02), 4.15 - length * 0.5), (0.25, 0.025, length),
                  rotation=(generator.uniform(-3, 3), 0, generator.uniform(-2, 2)), bevel=0.0)
    ring(parts, "mortar", (0.0, -0.02, 3.3), 0.3, 0.04, axis="y", segments=14)
    for z in (2.7, 2.45):
        ring(parts, "mortar", (0.0, -0.02, z), 0.12, 0.03, axis="y", segments=10)


def build_king_banner(parts, generator):
    """The dead king's banner hung on a column or a wall (its origin is the hook, the cloth hangs down from it): an iron
    bar, a long crimson cloth with a gold edge, his crown sewn in violet, the end cut into two tails"""
    parts.box(None, "iron", (0.0, 0.0, 0.0), (1.3, 0.08, 0.08), bevel=0.0)
    for side in (-1.0, 1.0):
        parts.sphere(None, "iron", (side * 0.68, 0.0, 0.0), (0.07, 0.07, 0.07))
    length = 4.2
    parts.box(None, "cloth", (0.0, -0.06, -length * 0.5), (1.1, 0.03, length), bevel=0.0)
    for side in (-1.0, 1.0):
        parts.box(None, "glass_gold", (side * 0.53, -0.08, -length * 0.5), (0.05, 0.02, length), bevel=0.0)
        # the two tails
        parts.box(None, "cloth", (side * 0.28, -0.06, -length - 0.35), (0.5, 0.03, 0.7), rotation=(0, side * 6.0, 0), bevel=0.0)
    # the crown: a band and five points
    parts.box(None, "rune", (0.0, -0.09, -1.5), (0.6, 0.02, 0.14), bevel=0.0)
    for index in range(5):
        x = -0.26 + index * 0.13
        height = 0.32 if index % 2 == 0 else 0.22
        parts.box(None, "rune", (x, -0.09, -1.43 + height * 0.5), (0.07, 0.02, height), bevel=0.0)


# Every piece: its builder and the size of its texture
PIECES = {
    "gc_crypt_floor":   (build_floor, BIG_TEXTURE_PIXELS),
    "gc_crypt_wall":    (build_wall, TEXTURE_PIXELS),
    "gc_crypt_column":  (build_column, TEXTURE_PIXELS),
    "gc_crypt_vault":   (build_vault, TEXTURE_PIXELS),
    "gc_stained_window": (build_window, TEXTURE_PIXELS),
    "gc_sarcophagus":   (build_sarcophagus, TEXTURE_PIXELS),
    "gc_candles":       (build_candles, TEXTURE_PIXELS),
    "gc_throne_dais":   (build_throne_dais, TEXTURE_PIXELS),
    "gc_portal":        (build_portal, TEXTURE_PIXELS),
    "gc_banner":        (build_banner, TEXTURE_PIXELS),
    "gc_king_banner":   (build_king_banner, TEXTURE_PIXELS),
    "gc_arch_ruin":     (build_arch_ruin, TEXTURE_PIXELS),
    "gc_runestone":     (build_runestone, TEXTURE_PIXELS),
    "gc_chain_anchor":  (build_chain_anchor, TEXTURE_PIXELS),
    "gc_graves":        (build_graves, TEXTURE_PIXELS),
}


def render_preview(folder, piece):
    camera = setup_preview_render(520, 520)
    size = max(piece.dimensions)
    place_preview_camera(camera, 35, distance=size * 1.6 + 1.0, height=size * 0.55, target_height=piece.dimensions.z * 0.45)
    bpy.context.scene.render.filepath = os.path.join(folder, piece.name + ".png")
    bpy.ops.render.render(write_still=True)


def main():
    arguments = parse_arguments()
    folder = os.path.abspath(arguments.out)
    os.makedirs(folder, exist_ok=True)

    clear_scene()
    setup_bake_render()
    palette = Palette(COLORS, SURFACES)
    pieces = []

    for index, (name, (build, pixels)) in enumerate(PIECES.items()):
        if arguments.only and arguments.only != name:
            continue
        parts = PartBuilder(palette)
        build(parts, random.Random(RANDOM_SEED + index))
        piece = parts.to_object(name)
        bake_textures(piece, palette, name, folder, pixels)
        export_gltf(os.path.join(folder, name + ".gltf"), [piece], with_animations=False)
        pieces.append(piece)
        print("GOTHIC_PIECE", name, tuple(round(value, 2) for value in piece.dimensions))

    if arguments.preview:
        os.makedirs(arguments.preview, exist_ok=True)
        for piece in pieces:
            show_only([piece])
            render_preview(os.path.abspath(arguments.preview), piece)

    print("GOTHIC_KIT_DONE")


if __name__ == "__main__":
    main()
