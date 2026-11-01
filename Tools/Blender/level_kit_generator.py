"""
Generates a small kit of ruined castle pieces for building levels: floor, walls, pillars, a gate, stairs and rubble

Run (from the repository root):
    blender --background --factory-startup --python Tools/Blender/level_kit_generator.py -- --out Projects/MyGame/res/Models/Level

Writes one glTF per piece into <out> (castle_floor.gltf, castle_wall.gltf, ...), each with its baked textures
Optional: --preview <folder> renders a picture of every piece

Every piece stands on its origin (the ground, Blender axes: Z up), so in the engine its Y = 0 is the floor
The sizes (meters) are listed in PIECES, a level gives each piece a Box Collider of that size
"""

import argparse
import math
import os
import random
import sys

import bpy

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

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

# name: (diffuse RGB, specular intensity)
STONE_COLORS = {
    "stone":       ((0.46, 0.44, 0.40), 0.15),
    "stone_light": ((0.55, 0.53, 0.49), 0.15),
    "stone_dark":  ((0.32, 0.31, 0.29), 0.10),
    "mortar":      ((0.20, 0.19, 0.17), 0.05),
    "moss":        ((0.20, 0.25, 0.12), 0.05),
    "iron":        ((0.22, 0.21, 0.20), 0.50),
}

# Photo surfaces (Poly Haven, CC0): rough limestone for the blocks, gravelly concrete for the mortar
LIGHT_STONE_SURFACE = SurfaceTexture("rock_boulder_dry_diff_1k.jpg", meters=1.0, strength=1.3)
DARK_STONE_SURFACE = SurfaceTexture("rock_08_diff_1k.jpg", meters=1.0, strength=1.3)

STONE_SURFACES = {
    "stone":       LIGHT_STONE_SURFACE,
    "stone_light": LIGHT_STONE_SURFACE,
    "stone_dark":  DARK_STONE_SURFACE,
    "mortar":      SurfaceTexture("gravel_concrete_03_diff_1k.jpg", meters=1.0),
    "moss":        SurfaceTexture("forest_ground_04_diff_1k.jpg", meters=0.8),
    "iron":        SurfaceTexture("rust_coarse_01_diff_1k.jpg", meters=1.0, strength=0.8),
}

TEXTURE_PIXELS = 1024
BIG_TEXTURE_PIXELS = 2048
RANDOM_SEED = 11

FLOOR_SIZE = 16.0
SLAB_SIZE = 1.0
FLOOR_THICKNESS = 0.3

WALL_LENGTH = 4.0
WALL_HEIGHT = 3.5
WALL_THICKNESS = 0.8
BLOCK_HEIGHT = 0.5

PILLAR_HEIGHT = 4.0
PILLAR_RADIUS = 0.35

GATE_OPENING = 3.2
GATE_HEIGHT = 4.2


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--preview", default="")
    return parser.parse_args(argv)


def pick_stone(generator):
    return generator.choice(("stone", "stone", "stone_light", "stone_dark"))


# ========== The pieces ==========


def build_floor(parts, generator):
    """Square slabs, each a little lower or higher, with dark mortar showing between them"""
    parts.box(None, "mortar", (0.0, 0.0, -FLOOR_THICKNESS * 0.5 - 0.02), (FLOOR_SIZE, FLOOR_SIZE, FLOOR_THICKNESS), bevel=0.0)

    slabs = int(FLOOR_SIZE / SLAB_SIZE)
    for row in range(slabs):
        for column in range(slabs):
            center_x = -FLOOR_SIZE * 0.5 + (column + 0.5) * SLAB_SIZE
            center_y = -FLOOR_SIZE * 0.5 + (row + 0.5) * SLAB_SIZE
            height = 0.12 + generator.uniform(-0.015, 0.015)
            color = "moss" if generator.random() < 0.06 else pick_stone(generator)
            parts.box(None, color, (center_x, center_y, -height * 0.5), (SLAB_SIZE - 0.04, SLAB_SIZE - 0.04, height),
                      rotation=(generator.uniform(-0.6, 0.6), generator.uniform(-0.6, 0.6), generator.uniform(-1.5, 1.5)), bevel=0.02)


def add_block_course(parts, generator, z, length, thickness, height, offset):
    """One row of stone blocks along X, from -length/2 to length/2, starting 'offset' meters in (rows are staggered)"""
    x = -length * 0.5
    first_block = True
    while x < length * 0.5 - 0.05:
        block_length = generator.uniform(0.6, 1.1)
        if first_block:
            block_length = offset
            first_block = False
        block_length = min(block_length, length * 0.5 - x)

        depth = thickness + generator.uniform(-0.03, 0.04)
        parts.box(None, pick_stone(generator), (x + block_length * 0.5, generator.uniform(-0.015, 0.015), z + height * 0.5),
                  (block_length - 0.03, depth, height - 0.03), bevel=0.03)
        x += block_length


def build_wall(parts, generator, height=WALL_HEIGHT, broken=False):
    # The mortar between the blocks: in a ruined wall only as high as the rows that are still whole
    mortar_height = BLOCK_HEIGHT * 3 if broken else height
    parts.box(None, "mortar", (0.0, 0.0, mortar_height * 0.5), (WALL_LENGTH - 0.06, WALL_THICKNESS - 0.08, mortar_height), bevel=0.0)

    courses = int(height / BLOCK_HEIGHT)
    for course in range(courses):
        length = WALL_LENGTH
        if broken:
            # A ruined top: every row is shorter, from a random side
            length = WALL_LENGTH * max(0.25, 1.0 - max(0, course - 2) * 0.22 * generator.uniform(0.7, 1.3))
        add_block_course(parts, generator, course * BLOCK_HEIGHT, length, WALL_THICKNESS, BLOCK_HEIGHT, 0.3 + 0.35 * (course % 2))

    if not broken:
        parts.box(None, "stone_dark", (0.0, 0.0, height + 0.08), (WALL_LENGTH, WALL_THICKNESS + 0.12, 0.16), bevel=0.03)
        parts.box(None, "moss", (generator.uniform(-1.2, 1.2), 0.0, height + 0.17), (generator.uniform(0.6, 1.4), WALL_THICKNESS * 0.8, 0.03), bevel=0.01)


def build_pillar(parts, generator, height=PILLAR_HEIGHT, broken=False):
    parts.box(None, "stone_dark", (0.0, 0.0, 0.15), (1.0, 1.0, 0.3), bevel=0.04)
    parts.box(None, "stone", (0.0, 0.0, 0.38), (0.85, 0.85, 0.16), bevel=0.03)

    shaft_top = height - 0.35 if not broken else height
    parts.cylinder(None, "stone_light", make_matrix((0.0, 0.0, (0.46 + shaft_top) * 0.5)), PILLAR_RADIUS + 0.02, PILLAR_RADIUS,
                   shaft_top - 0.46, segments=16)

    if broken:
        # A slanted break at the top and a piece lying next to it
        parts.cylinder(None, "stone_dark", make_matrix((0.04, 0.0, height + 0.05), (14, 6, 0)), PILLAR_RADIUS, PILLAR_RADIUS * 0.7, 0.18, segments=12)
        parts.cylinder(None, "stone_light", make_matrix((1.1, 0.4, PILLAR_RADIUS), (90, 0, 35)), PILLAR_RADIUS, PILLAR_RADIUS, 1.2, segments=16)
        return

    parts.box(None, "stone", (0.0, 0.0, height - 0.25), (0.85, 0.85, 0.2), bevel=0.03)
    parts.box(None, "stone_dark", (0.0, 0.0, height - 0.07), (1.0, 1.0, 0.16), bevel=0.04)


def build_gate(parts, generator):
    """Two massive piers and a stone arch over a dark opening (the way into the boss arena)"""
    pier_width = 1.4
    pier_x = GATE_OPENING * 0.5 + pier_width * 0.5
    depth = WALL_THICKNESS + 0.4

    for side in (-1, 1):
        for course in range(int((GATE_HEIGHT + 1.2) / BLOCK_HEIGHT)):
            parts.box(None, pick_stone(generator), (side * pier_x, 0.0, course * BLOCK_HEIGHT + BLOCK_HEIGHT * 0.5),
                      (pier_width - 0.04, depth + generator.uniform(-0.03, 0.03), BLOCK_HEIGHT - 0.03), bevel=0.03)

    # The arch: wedge stones on a half circle above the opening
    arch_radius = GATE_OPENING * 0.5 + 0.35
    spring = GATE_HEIGHT - GATE_OPENING * 0.5
    stones = 11
    for index in range(stones):
        angle = math.pi * (index + 0.5) / stones
        x = math.cos(angle) * arch_radius
        z = spring + math.sin(angle) * arch_radius
        color = "stone_dark" if index == stones // 2 else pick_stone(generator)
        parts.box(None, color, (x, 0.0, z), (0.5, depth, 0.7), rotation=(0, -math.degrees(angle) + 90, 0), bevel=0.03)

    parts.box(None, "stone_dark", (0.0, 0.0, GATE_HEIGHT + 1.35), (GATE_OPENING + pier_width * 2 + 0.2, depth + 0.15, 0.3), bevel=0.04)


def build_stairs(parts, generator):
    steps = 5
    for step in range(steps):
        height = 0.2 * (step + 1)
        parts.box(None, pick_stone(generator), (0.0, -step * 0.4, height * 0.5), (3.0, 0.4 + 0.02, height), bevel=0.02)


def build_rubble(parts, generator):
    for _ in range(14):
        size = generator.uniform(0.2, 0.6)
        location = (generator.uniform(-1.0, 1.0), generator.uniform(-0.8, 0.8), size * 0.35)
        rotation = (generator.uniform(-25, 25), generator.uniform(-25, 25), generator.uniform(0, 90))
        parts.box(None, pick_stone(generator), location, (size, size * generator.uniform(0.6, 1.0), size * 0.7), rotation, bevel=0.04)


# Every piece: its builder and the size of its texture
PIECES = {
    "castle_floor":         (build_floor, BIG_TEXTURE_PIXELS),
    "castle_wall":          (lambda parts, generator: build_wall(parts, generator), TEXTURE_PIXELS),
    "castle_wall_broken":   (lambda parts, generator: build_wall(parts, generator, broken=True), TEXTURE_PIXELS),
    "castle_pillar":        (lambda parts, generator: build_pillar(parts, generator), TEXTURE_PIXELS),
    "castle_pillar_broken": (lambda parts, generator: build_pillar(parts, generator, height=1.8, broken=True), TEXTURE_PIXELS),
    "castle_gate":          (build_gate, TEXTURE_PIXELS),
    "castle_stairs":        (build_stairs, TEXTURE_PIXELS),
    "castle_rubble":        (build_rubble, TEXTURE_PIXELS),
}


def render_preview(folder, piece):
    camera = setup_preview_render(500, 500)
    place_preview_camera(camera, 35, distance=9.0, height=4.5, target_height=1.5)
    bpy.context.scene.render.filepath = os.path.join(folder, piece.name + ".png")
    bpy.ops.render.render(write_still=True)


def main():
    arguments = parse_arguments()
    folder = os.path.abspath(arguments.out)
    os.makedirs(folder, exist_ok=True)

    clear_scene()
    setup_bake_render()

    palette = Palette(STONE_COLORS, STONE_SURFACES)
    pieces = []

    for name, (build, pixels) in PIECES.items():
        parts = PartBuilder(palette)
        build(parts, random.Random(RANDOM_SEED + len(pieces)))
        piece = parts.to_object(name)

        bake_textures(piece, palette, name, folder, pixels)
        export_gltf(os.path.join(folder, name + ".gltf"), [piece], with_animations=False)
        pieces.append(piece)

    if arguments.preview:
        os.makedirs(arguments.preview, exist_ok=True)
        for piece in pieces:
            show_only([piece])
            render_preview(os.path.abspath(arguments.preview), piece)

    print("LEVEL_KIT_GENERATOR_DONE")


if __name__ == "__main__":
    main()
