"""
Generates the bonfire model of the engine: a ring of stones, ashes with embers, charred logs,
a rusted sword planted in the middle and the flames around it

Run (from the repository root):
    blender --background --factory-startup --python Tools/Blender/bonfire_generator.py -- --out Sunta/res/Models --icon Sunta/res/Textures/Icons/souls_bonfire_icon.png

Writes <out>/Bonfire/bonfire.gltf (+ .bin, bonfire_diffuse.png, bonfire_specular.png)
Optional: --icon <file> renders the editor icon, --preview <file> renders a picture of the model

The origin is on the ground in the middle of the fire, Blender axes: Z up
"""

import argparse
import math
import os
import random
import sys

import bpy
from mathutils import Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from sunta_blender import (  # noqa: E402
    Palette,
    PartBuilder,
    bake_textures,
    clear_scene,
    export_gltf,
    make_matrix,
    place_preview_camera,
    setup_bake_render,
    setup_preview_render,
)

# name: (diffuse RGB, specular intensity)
BONFIRE_COLORS = {
    "stone":        ((0.50, 0.47, 0.43), 0.20),
    "stone_dark":   ((0.36, 0.34, 0.31), 0.15),
    "ash":          ((0.30, 0.28, 0.26), 0.05),
    "charcoal":     ((0.09, 0.08, 0.07), 0.05),
    "ember":        ((1.00, 0.42, 0.08), 0.00, True),
    "flame_outer":  ((1.00, 0.50, 0.10), 0.00, True),
    "flame_inner":  ((1.00, 0.88, 0.45), 0.00, True),
    "rusted_steel": ((0.40, 0.33, 0.27), 0.45),
    "rust":         ((0.36, 0.19, 0.09), 0.25),
    "leather":      ((0.13, 0.09, 0.06), 0.08),
}

TEXTURE_PIXELS = 1024

# The same pseudo random stones and flames every time the model is made
RANDOM_SEED = 7

STONE_COUNT = 13
STONE_RING_RADIUS = 0.58

LOG_COUNT = 5
EMBER_COUNT = 14

# The sword leans a little, planted in the ashes up to a third of its blade
SWORD_TILT_DEGREES = (8.0, -5.0, 0.0)
SWORD_BLADE_BOTTOM = -0.15
SWORD_BLADE_TOP = 0.62

ICON_PIXELS = 256


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--icon", default="")
    parser.add_argument("--preview", default="")
    return parser.parse_args(argv)


def build_stone_ring(parts, generator):
    for index in range(STONE_COUNT):
        angle = math.tau * index / STONE_COUNT + generator.uniform(-0.12, 0.12)
        radius = STONE_RING_RADIUS + generator.uniform(-0.04, 0.05)
        size = generator.uniform(0.09, 0.13)

        location = (math.cos(angle) * radius, math.sin(angle) * radius, size * 0.45)
        radii = (size * generator.uniform(1.0, 1.4), size * generator.uniform(0.8, 1.1), size * generator.uniform(0.6, 0.85))
        rotation = (generator.uniform(-12, 12), generator.uniform(-12, 12), math.degrees(angle) + generator.uniform(-20, 20))

        color = "stone" if index % 3 else "stone_dark"
        parts.sphere(None, color, location, radii, rotation, segments=9, rings=6)


def build_ashes(parts, generator):
    parts.sphere(None, "ash", (0.0, 0.0, 0.0), (0.46, 0.46, 0.09), segments=24, rings=10)
    parts.sphere(None, "charcoal", (0.0, 0.0, 0.03), (0.3, 0.3, 0.07), segments=20, rings=8)

    for _ in range(EMBER_COUNT):
        angle = generator.uniform(0.0, math.tau)
        radius = generator.uniform(0.05, 0.36)
        size = generator.uniform(0.02, 0.04)
        location = (math.cos(angle) * radius, math.sin(angle) * radius, 0.06)
        parts.sphere(None, "ember", location, (size, size, size * 0.6), segments=6, rings=4)


def build_logs(parts, generator):
    """Charred logs lying in the ashes, their ends pointing into the fire"""
    for index in range(LOG_COUNT):
        angle = math.tau * index / LOG_COUNT + generator.uniform(-0.2, 0.2)
        inner = Vector((math.cos(angle) * 0.08, math.sin(angle) * 0.08, 0.16))
        outer = Vector((math.cos(angle) * 0.5, math.sin(angle) * 0.5, 0.05))
        radius = generator.uniform(0.045, 0.06)

        parts.limb(None, "charcoal", outer, inner, radius, radius * 0.85, segments=8)
        parts.sphere(None, "ember", inner, (radius * 0.9, radius * 0.9, radius * 0.9), segments=8, rings=5)


def build_sword(parts):
    """A rusted sword planted in the middle: the blade goes into the ashes, the grip stands up"""
    tilt = make_matrix((0.0, 0.0, 0.0), SWORD_TILT_DEGREES)

    def place(location, size, color, rotation=(0, 0, 0)):
        parts.box(None, color, tuple(tilt @ Vector(location)), size, tuple(a + b for a, b in zip(SWORD_TILT_DEGREES, rotation)))

    blade_length = SWORD_BLADE_TOP - SWORD_BLADE_BOTTOM
    place((0.0, 0.0, (SWORD_BLADE_BOTTOM + SWORD_BLADE_TOP) * 0.5), (0.05, 0.012, blade_length), "rusted_steel")
    place((0.0, 0.0, SWORD_BLADE_TOP + 0.015), (0.2, 0.026, 0.03), "rust")
    place((0.0, 0.0, SWORD_BLADE_TOP + 0.11), (0.032, 0.032, 0.16), "leather")
    place((0.0, 0.0, SWORD_BLADE_TOP + 0.215), (0.05, 0.05, 0.05), "rust", rotation=(45, 0, 45))


def build_flame(parts, center_xy, height, lean):
    """One tongue of fire: a round bulb at the bottom narrowing into a tip, yellow inside orange"""
    width = height * 0.32
    bottom = 0.05

    parts.sphere(None, "flame_outer", (center_xy[0], center_xy[1], bottom + width * 0.6), (width, width, width * 0.8), segments=10, rings=6)
    parts.cylinder(None, "flame_outer", make_matrix((center_xy[0], center_xy[1], bottom + height * 0.55), lean), width * 0.95, 0.0, height * 0.9, segments=10)
    parts.cylinder(None, "flame_inner", make_matrix((center_xy[0], center_xy[1], bottom + height * 0.4), lean), width * 0.6, 0.0, height * 0.65, segments=8)


def build_flames(parts, generator):
    """Tall tongues of fire in the middle, shorter ones further out"""
    for ring, (count, radius, height) in enumerate(((3, 0.05, 0.6), (5, 0.16, 0.4), (6, 0.26, 0.24))):
        for index in range(count):
            angle = math.tau * index / count + generator.uniform(-0.3, 0.3) + ring
            center_xy = (math.cos(angle) * radius, math.sin(angle) * radius)
            lean = (generator.uniform(-6, 6), generator.uniform(-6, 6), generator.uniform(0, 90))
            build_flame(parts, center_xy, height * generator.uniform(0.8, 1.15), lean)


def build_bonfire(palette):
    generator = random.Random(RANDOM_SEED)
    parts = PartBuilder(palette)

    build_stone_ring(parts, generator)
    build_ashes(parts, generator)
    build_logs(parts, generator)
    build_sword(parts)
    build_flames(parts, generator)

    return parts.to_object("Bonfire")


def render_picture(path, pixels, is_transparent, angle, distance, height):
    camera = setup_preview_render(pixels, pixels, is_transparent=is_transparent)
    place_preview_camera(camera, angle, distance=distance, height=height, target_height=0.3)

    scene = bpy.context.scene
    scene.render.filepath = os.path.abspath(path)
    bpy.ops.render.render(write_still=True)


def main():
    arguments = parse_arguments()
    folder = os.path.join(os.path.abspath(arguments.out), "Bonfire")
    os.makedirs(folder, exist_ok=True)

    clear_scene()

    palette = Palette(BONFIRE_COLORS)
    bonfire = build_bonfire(palette)

    setup_bake_render()
    bake_textures(bonfire, palette, "bonfire", folder, TEXTURE_PIXELS)
    export_gltf(os.path.join(folder, "bonfire.gltf"), [bonfire], with_animations=False)

    if arguments.icon:
        render_picture(arguments.icon, ICON_PIXELS, True, angle=30, distance=1.9, height=1.2)

    if arguments.preview:
        render_picture(arguments.preview, 600, False, angle=30, distance=2.4, height=1.3)

    print("BONFIRE_GENERATOR_DONE")


if __name__ == "__main__":
    main()
