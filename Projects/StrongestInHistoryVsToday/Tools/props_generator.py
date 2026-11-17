"""
Generates the props of 'Strongest in History Vs Today': the bodies of the projectiles each fighter throws (a spell's
Projectile shows a model, see ProjectileComponent) and the temple of the boss's domain. Each comes from its generated
model when there is one (Meshy, from a concept picture), otherwise it is built out of simple shapes as a stand-in

Run (from the repository root):
    blender --background --factory-startup --python Projects/StrongestInHistoryVsToday/Tools/props_generator.py
    (optional: -- --only temple --preview <folder>)

Writes res/Models/Props/<Folder>/<name>.gltf (+ .bin, _diffuse.png, _specular.png)

The player's technique is baby oil: a bottle. The boss throws what he was known for: bundles of hush money,
his black book, the tapes of his hidden cameras, and his private jet flies over the arena. The temple is the one of
his island: a square of white stone with blue stripes under a golden dome (the concept:
Tools/Blender/Concepts/StrongestInHistory/temple_concept.webp)

Blender axes: Z up. A projectile flies towards -Y here (the engine's +Z, the front of a model), its middle at the origin.
The temple stands on the ground with its door towards -Y
"""

import argparse
import math
import os
import random
import sys
from dataclasses import dataclass

import bpy
from mathutils import Matrix, Vector

PROJECT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLENDER_FOLDER = os.path.join(os.path.dirname(os.path.dirname(PROJECT)), "Tools", "Blender")
sys.path.insert(0, BLENDER_FOLDER)

import character_from_mesh  # noqa: E402
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
)

OUT_FOLDER = os.path.join(PROJECT, "res", "Models", "Props")

RANDOM_SEED = 11

# A cylinder is built along Z: lying along Y (the way a projectile flies) or along X
ALONG_Y = (90.0, 0.0, 0.0)
ALONG_X = (0.0, 90.0, 0.0)


# ========== The bottle of baby oil ==========

BOTTLE_COLORS = {
    "clear":     ((0.93, 0.95, 0.98), 0.95, True),
    "cap":       ((0.98, 0.72, 0.83), 0.45, True),
    "cap_ridge": ((0.93, 0.60, 0.74), 0.45),
    "drop":      ((0.97, 0.62, 0.77), 0.30, True),
    "drop_dark": ((0.90, 0.36, 0.56), 0.30),
}

# The bottle is flatter than it is wide, as the real ones are
BOTTLE_FLATNESS = 0.62


def build_bottle(parts):
    """A clear bottle with a waist, a wide pink cap and a pink drop on its label, as baby oil is sold.
    Lying along its flight: the cap first"""
    def ring(color, y_back, y_front, radius_back, radius_front, flatness=BOTTLE_FLATNESS):
        middle, depth = (y_back + y_front) * 0.5, y_back - y_front
        parts.cylinder(None, color, make_matrix((0.0, middle, 0.0), ALONG_Y), radius_front, radius_back, depth,
                       segments=20, scale=(1.0, flatness, 1.0))

    # From the bottom to the shoulders: wide, a waist in the middle, wide again
    ring("clear", 0.29, 0.27, 0.095, 0.115)
    ring("clear", 0.27, 0.16, 0.115, 0.118)
    ring("clear", 0.16, 0.06, 0.118, 0.100)
    ring("clear", 0.06, -0.04, 0.100, 0.116)
    ring("clear", -0.04, -0.12, 0.116, 0.108)
    ring("clear", -0.12, -0.17, 0.108, 0.070)
    # The cap: round, wide, with a darker ring where it screws on
    ring("cap_ridge", -0.17, -0.185, 0.078, 0.078, flatness=1.0)
    ring("cap", -0.185, -0.27, 0.075, 0.072, flatness=1.0)
    ring("cap", -0.27, -0.28, 0.072, 0.060, flatness=1.0)

    # The label: a pink drop on the front and on the back, its tip towards the cap
    for side in (-1.0, 1.0):
        surface = side * 0.118 * BOTTLE_FLATNESS
        parts.sphere(None, "drop", (0.0, 0.10, surface * 0.93), (0.078, 0.085, 0.012), segments=18, rings=8)
        parts.cylinder(None, "drop", make_matrix((0.0, -0.01, surface * 0.9), ALONG_Y), 0.0, 0.066, 0.13,
                       segments=18, scale=(1.0, 0.16, 1.0))
        parts.sphere(None, "drop_dark", (0.0, 0.135, surface * 0.97), (0.05, 0.022, 0.012), segments=14, rings=6)


# ========== A bundle of hush money ==========

MONEY_COLORS = {
    "bill":  ((0.35, 0.62, 0.32), 0.10),
    "edge":  ((0.80, 0.86, 0.72), 0.10),
    "strap": ((0.95, 0.90, 0.60), 0.20),
    "ink":   ((0.16, 0.34, 0.16), 0.10),
}


def build_money(parts):
    parts.box(None, "edge", (0.0, 0.0, 0.0), (0.20, 0.44, 0.10))
    parts.box(None, "bill", (0.0, 0.0, 0.051), (0.20, 0.44, 0.006))
    parts.box(None, "bill", (0.0, 0.0, -0.051), (0.20, 0.44, 0.006))
    # The paper strap around the middle and the oval where the face would be
    parts.box(None, "strap", (0.0, 0.0, 0.0), (0.21, 0.09, 0.115))
    for side in (-1.0, 1.0):
        parts.cylinder(None, "ink", make_matrix((0.0, side * 0.13, 0.055)), 0.05, 0.05, 0.004, segments=14, scale=(1.0, 1.3, 1.0))


# ========== The black book ==========

BOOK_COLORS = {
    "cover": ((0.05, 0.05, 0.06), 0.35),
    "pages": ((0.90, 0.86, 0.74), 0.05),
    "gold":  ((0.90, 0.70, 0.25), 0.90),
}


def build_book(parts):
    parts.box(None, "pages", (0.012, 0.0, 0.0), (0.30, 0.42, 0.065))
    parts.box(None, "cover", (0.0, 0.0, 0.037), (0.33, 0.45, 0.012))
    parts.box(None, "cover", (0.0, 0.0, -0.037), (0.33, 0.45, 0.012))
    parts.box(None, "cover", (-0.16, 0.0, 0.0), (0.012, 0.45, 0.086))   # the spine
    # Gold lines where a title would be, and the corners
    for y, width in ((-0.10, 0.18), (-0.06, 0.12)):
        parts.box(None, "gold", (0.0, y, 0.044), (width, 0.016, 0.003))
    for x in (-0.145, 0.145):
        for y in (-0.205, 0.205):
            parts.box(None, "gold", (x, y, 0.0), (0.03, 0.03, 0.09))


# ========== A tape of the hidden cameras ==========

TAPE_COLORS = {
    "shell":  ((0.07, 0.07, 0.08), 0.40),
    "label":  ((0.93, 0.92, 0.86), 0.10),
    "window": ((0.25, 0.27, 0.30), 0.80),
    "reel":   ((0.85, 0.85, 0.85), 0.30),
    "record": ((1.00, 0.10, 0.08), 0.00, True),
}


def build_tape(parts):
    parts.box(None, "shell", (0.0, 0.0, 0.0), (0.42, 0.24, 0.06))
    parts.box(None, "label", (0.0, 0.07, 0.031), (0.34, 0.07, 0.003))
    parts.box(None, "window", (0.0, -0.03, 0.031), (0.26, 0.09, 0.003))
    for x in (-0.075, 0.075):
        parts.cylinder(None, "reel", make_matrix((x, -0.03, 0.033)), 0.032, 0.032, 0.004, segments=14)
    parts.sphere(None, "record", (0.17, -0.09, 0.03), (0.018, 0.018, 0.012), segments=10, rings=6)


# ========== The private jet ==========

JET_COLORS = {
    "body":   ((0.93, 0.93, 0.95), 0.60),
    "stripe": ((0.08, 0.12, 0.30), 0.40),
    "glass":  ((0.06, 0.08, 0.12), 0.90),
    "metal":  ((0.45, 0.46, 0.50), 0.80),
    "light":  ((1.00, 0.20, 0.15), 0.00, True),
}


def build_jet(parts):
    """Nine meters from nose to tail, flying towards -Y"""
    parts.limb(None, "body", (0.0, 3.2, 0.0), (0.0, -2.6, 0.0), 0.62, 0.62, segments=18)
    parts.limb(None, "body", (0.0, -2.6, 0.0), (0.0, -4.5, -0.12), 0.62, 0.06, segments=18)      # the nose
    parts.limb(None, "body", (0.0, 3.2, 0.0), (0.0, 4.6, 0.25), 0.62, 0.16, segments=18)         # the tail cone
    parts.box(None, "glass", (0.0, -3.05, 0.28), (0.72, 0.6, 0.26), rotation=(-14.0, 0.0, 0.0))  # the cockpit
    parts.box(None, "stripe", (0.0, 0.2, 0.05), (1.27, 5.6, 0.14))                               # the stripe with the windows
    for index in range(7):
        for side in (-1.0, 1.0):
            parts.box(None, "glass", (side * 0.62, -1.8 + index * 0.62, 0.2), (0.05, 0.3, 0.2))

    for side in (-1.0, 1.0):
        # Swept wings with a little winglet, the tailplane, an engine on each side of the tail
        parts.box(None, "body", (side * 2.3, 0.75, -0.22), (4.0, 1.25, 0.1), rotation=(0.0, side * -4.0, side * 24.0))
        parts.box(None, "stripe", (side * 4.15, 1.62, 0.02), (0.08, 0.7, 0.55), rotation=(0.0, 0.0, side * 24.0))
        parts.box(None, "body", (side * 1.25, 4.35, 1.45), (2.2, 0.7, 0.07), rotation=(0.0, 0.0, side * 20.0))
        parts.limb(None, "metal", (side * 1.0, 2.2, 0.35), (side * 1.0, 3.9, 0.35), 0.3, 0.26, segments=14)
        parts.limb(None, "glass", (side * 1.0, 2.14, 0.35), (side * 1.0, 2.2, 0.35), 0.24, 0.24, segments=14)
        parts.sphere(None, "light", (side * 4.2, 1.7, 0.3), (0.07, 0.07, 0.07), segments=8, rings=5)

    parts.box(None, "stripe", (0.0, 4.0, 0.9), (0.1, 1.5, 1.5), rotation=(-28.0, 0.0, 0.0))      # the fin


# ========== The temple of the island ==========

TEMPLE_COLORS = {
    "white": ((0.86, 0.84, 0.78), 0.15),
    "blue":  ((0.12, 0.33, 0.78), 0.20),
    "gold":  ((0.92, 0.68, 0.20), 0.95),
    "wood":  ((0.42, 0.24, 0.12), 0.15),
    "stone": ((0.55, 0.53, 0.50), 0.15),
    "rock":  ((0.30, 0.28, 0.27), 0.10),
    "dark":  ((0.05, 0.04, 0.05), 0.05),
}

TEMPLE_SURFACES = {
    "white": SurfaceTexture("gravel_concrete_03_diff_1k.jpg", meters=3.0, strength=0.9),
    "blue":  SurfaceTexture("gravel_concrete_03_diff_1k.jpg", meters=3.0, strength=0.7),
    "stone": SurfaceTexture("rock_08_diff_1k.jpg", meters=3.0, strength=0.9),
    "rock":  SurfaceTexture("rock_boulder_dry_diff_1k.jpg", meters=5.0, strength=1.0),
    "wood":  SurfaceTexture("old_wood_floor_diff_1k.jpg", meters=2.0, strength=0.9),
}

# The cube of the temple (meters): its walls, where it starts above the terrace, how many blue stripes
TEMPLE_WIDTH = 12.0
TEMPLE_WALL_HEIGHT = 10.0
TEMPLE_FLOOR = 1.2
TEMPLE_STRIPES = 5
TEMPLE_DOME_RADIUS = 4.9


def build_arch(parts, center, facing_degrees, width, height, depth, panel_color):
    """A doorway with a round top in a blue frame: 'center' is the middle of its threshold, on the wall"""
    def place(color, local, size, bevel=0.02):
        turn = make_matrix((0.0, 0.0, 0.0), (0.0, 0.0, facing_degrees))
        point = turn @ Vector(local)
        parts.box(None, color, (center[0] + point.x, center[1] + point.y, center[2] + point.z), size, (0.0, 0.0, facing_degrees), bevel)

    def disc(color, local, radius, thickness):
        turn = make_matrix((0.0, 0.0, 0.0), (0.0, 0.0, facing_degrees))
        point = turn @ Vector(local)
        matrix = make_matrix((center[0] + point.x, center[1] + point.y, center[2] + point.z), (90.0, 0.0, facing_degrees))
        parts.cylinder(None, color, matrix, radius, radius, thickness, segments=24)

    straight = height - width * 0.5
    frame = 0.45
    # The frame behind, the panel (a door, or the dark of an opening) in front of it
    place("blue", (0.0, -depth * 0.5, straight * 0.5), (width + frame * 2.0, depth, straight))
    disc("blue", (0.0, -depth * 0.5, straight), width * 0.5 + frame, depth)
    place(panel_color, (0.0, -depth * 0.75, straight * 0.5), (width, depth, straight))
    disc(panel_color, (0.0, -depth * 0.75, straight), width * 0.5, depth)
    if panel_color == "wood":
        place("dark", (0.0, -depth * 1.26, straight * 0.5), (0.06, 0.02, straight))        # the gap between the two leaves
        for side in (-1.0, 1.0):
            place("gold", (side * 0.22, -depth * 1.3, straight * 0.5), (0.1, 0.08, 0.34))  # the handles


def build_temple(parts):
    generator = random.Random(RANDOM_SEED)
    half = TEMPLE_WIDTH * 0.5
    wall_top = TEMPLE_FLOOR + TEMPLE_WALL_HEIGHT

    # The rock it stands on: boulders under and around a terrace of stone
    for _ in range(26):
        angle = generator.uniform(0.0, math.tau)
        radius = generator.uniform(4.0, 11.5)
        size = generator.uniform(2.6, 5.0)
        parts.sphere(None, "rock", (math.cos(angle) * radius, math.sin(angle) * radius, generator.uniform(-3.2, -1.2)),
                     (size * generator.uniform(0.9, 1.4), size * generator.uniform(0.9, 1.4), size * generator.uniform(0.7, 1.0)),
                     (generator.uniform(-20, 20), generator.uniform(-20, 20), generator.uniform(0, 180)), segments=10, rings=7)
    parts.box(None, "stone", (0.0, 0.0, 0.3), (19.0, 19.0, 1.8), bevel=0.08)
    parts.box(None, "stone", (0.0, 0.0, TEMPLE_FLOOR - 0.2), (14.6, 14.6, 0.5), bevel=0.06)

    # The stairs down the front, between two low walls with a ball on each end
    for step in range(9):
        parts.box(None, "stone", (0.0, -9.6 - step * 0.62, 0.95 - step * 0.36), (4.4, 0.66, 0.4), bevel=0.03)
    for side in (-1.0, 1.0):
        parts.box(None, "stone", (side * 2.5, -12.1, -0.2), (0.6, 5.6, 1.2), rotation=(-30.0, 0.0, 0.0), bevel=0.05)
        parts.sphere(None, "white", (side * 2.5, -9.5, 1.75), (0.36, 0.36, 0.36), segments=12, rings=8)

    # The cube: white stone, blue stripes standing a hand out of it, a pillar of stone on every corner
    parts.box(None, "white", (0.0, 0.0, TEMPLE_FLOOR + TEMPLE_WALL_HEIGHT * 0.5), (TEMPLE_WIDTH, TEMPLE_WIDTH, TEMPLE_WALL_HEIGHT), bevel=0.05)
    stripe_step = TEMPLE_WALL_HEIGHT / (TEMPLE_STRIPES * 2 + 1)
    for stripe in range(TEMPLE_STRIPES):
        height = TEMPLE_FLOOR + stripe_step * (stripe * 2 + 1.5)
        parts.box(None, "blue", (0.0, 0.0, height), (TEMPLE_WIDTH + 0.12, TEMPLE_WIDTH + 0.12, stripe_step), bevel=0.02)
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            parts.box(None, "stone", (x * half, y * half, TEMPLE_FLOOR + TEMPLE_WALL_HEIGHT * 0.5), (1.1, 1.1, TEMPLE_WALL_HEIGHT), bevel=0.06)
            parts.box(None, "stone", (x * half, y * half, TEMPLE_FLOOR + 0.5), (1.5, 1.5, 1.0), bevel=0.06)

    # The door in front, an arched opening on each side and at the back
    build_arch(parts, (0.0, -half, TEMPLE_FLOOR), 0.0, 3.0, 7.2, 0.5, "wood")
    build_arch(parts, (half, 0.0, TEMPLE_FLOOR), 90.0, 2.4, 6.4, 0.5, "dark")
    build_arch(parts, (-half, 0.0, TEMPLE_FLOOR), -90.0, 2.4, 6.4, 0.5, "dark")
    build_arch(parts, (0.0, half, TEMPLE_FLOOR), 180.0, 2.4, 6.4, 0.5, "dark")

    # The cornice, a blue band under it and the teeth of a battlement on top
    parts.box(None, "blue", (0.0, 0.0, wall_top - 0.15), (TEMPLE_WIDTH + 0.5, TEMPLE_WIDTH + 0.5, 0.5), bevel=0.03)
    parts.box(None, "stone", (0.0, 0.0, wall_top + 0.45), (TEMPLE_WIDTH + 1.5, TEMPLE_WIDTH + 1.5, 0.8), bevel=0.06)
    parts.box(None, "white", (0.0, 0.0, wall_top + 1.0), (TEMPLE_WIDTH + 0.9, TEMPLE_WIDTH + 0.9, 0.5), bevel=0.04)
    teeth = 11
    edge = (TEMPLE_WIDTH + 0.9) * 0.5 - 0.3
    for index in range(teeth):
        along = -edge + index * (edge * 2.0 / (teeth - 1))
        for x, y in ((along, -edge), (along, edge), (-edge, along), (edge, along)):
            parts.box(None, "white", (x, y, wall_top + 1.55), (0.62, 0.62, 0.7), bevel=0.04)

    # The drum with its little windows and the golden dome with a spire
    drum_bottom = wall_top + 1.2
    parts.cylinder(None, "white", make_matrix((0.0, 0.0, drum_bottom + 0.9)), TEMPLE_DOME_RADIUS, TEMPLE_DOME_RADIUS, 1.8, segments=32)
    parts.cylinder(None, "blue", make_matrix((0.0, 0.0, drum_bottom + 0.2)), TEMPLE_DOME_RADIUS + 0.12, TEMPLE_DOME_RADIUS + 0.12, 0.3, segments=32)
    for index in range(12):
        angle = math.tau * index / 12
        position = (math.cos(angle) * TEMPLE_DOME_RADIUS, math.sin(angle) * TEMPLE_DOME_RADIUS, drum_bottom + 1.0)
        parts.box(None, "dark", position, (0.25, 0.7, 0.95), rotation=(0.0, 0.0, math.degrees(angle)), bevel=0.02)
    dome_bottom = drum_bottom + 1.8
    parts.cylinder(None, "gold", make_matrix((0.0, 0.0, dome_bottom + 0.1)), TEMPLE_DOME_RADIUS + 0.3, TEMPLE_DOME_RADIUS + 0.3, 0.25, segments=32)
    parts.half_sphere(None, "gold", (0.0, 0.0, dome_bottom + 0.15), (TEMPLE_DOME_RADIUS + 0.1, TEMPLE_DOME_RADIUS + 0.1, TEMPLE_DOME_RADIUS * 0.95),
                      segments=32, rings=16)
    top = dome_bottom + 0.15 + TEMPLE_DOME_RADIUS * 0.95
    parts.sphere(None, "gold", (0.0, 0.0, top + 0.25), (0.42, 0.42, 0.42), segments=14, rings=8)
    parts.sphere(None, "gold", (0.0, 0.0, top + 0.85), (0.26, 0.26, 0.26), segments=12, rings=8)
    parts.cylinder(None, "gold", make_matrix((0.0, 0.0, top + 1.7)), 0.14, 0.0, 1.6, segments=10)


# ========== The ground of the ruined city ==========

GROUND_COLORS = {
    "asphalt": ((0.52, 0.49, 0.45), 0.08),
    "scorch":  ((0.33, 0.30, 0.28), 0.04),
    "dust":    ((0.62, 0.57, 0.50), 0.05),
    "chunk":   ((0.58, 0.55, 0.52), 0.10),
}

GROUND_SURFACES = {
    "asphalt": SurfaceTexture("gravel_concrete_03_diff_1k.jpg", meters=3.0, strength=1.2),
    "scorch":  SurfaceTexture("burned_ground_01_diff_1k.jpg", meters=2.0, strength=1.0),
    "dust":    SurfaceTexture("burned_ground_01_diff_1k.jpg", meters=2.5, strength=0.8),
    "chunk":   SurfaceTexture("rock_08_diff_1k.jpg", meters=1.0, strength=1.0),
}

# One tile of the street, as wide as the tiles of the level kit (they are laid side by side)
GROUND_TILE = 8.0


def build_ground(parts):
    """Broken asphalt: scorched and dusty patches lying on it, small pieces of concrete scattered about. The pieces
    stay low (a boot's height): the fight goes over them"""
    generator = random.Random(RANDOM_SEED)
    half = GROUND_TILE * 0.5
    parts.box(None, "asphalt", (0.0, 0.0, -0.25), (GROUND_TILE, GROUND_TILE, 0.5), bevel=0.0)

    # Low flat slabs of broken paving, no two alike, instead of anything round: a street after an explosion
    for _ in range(14):
        color = generator.choice(("scorch", "dust", "dust"))
        size = generator.uniform(0.7, 1.9)
        position = (generator.uniform(-half + 1.2, half - 1.2), generator.uniform(-half + 1.2, half - 1.2), 0.012)
        parts.box(None, color, position, (size, size * generator.uniform(0.4, 0.9), 0.03),
                  rotation=(generator.uniform(-2, 2), generator.uniform(-2, 2), generator.uniform(0, 180)), bevel=0.01)

    for _ in range(70):
        size = generator.uniform(0.06, 0.24)
        position = (generator.uniform(-half + 0.3, half - 0.3), generator.uniform(-half + 0.3, half - 0.3), size * 0.25)
        parts.box(None, "chunk", position, (size * generator.uniform(0.8, 1.8), size, size * 0.6),
                  rotation=(generator.uniform(-15, 15), generator.uniform(-15, 15), generator.uniform(0, 180)), bevel=0.01)


# ========== Making them ==========

GENERATED_FOLDER = os.path.join(BLENDER_FOLDER, "Generated", "StrongestInHistory")

# A little brighter than generated: the engine has no reflections, dark surfaces would look flat
GENERATED_BRIGHTNESS = 1.25


@dataclass
class Prop:
    folder: str
    # The generated model (Meshy, from a concept picture: see Tools/Blender/Generated/meshy_log.json) in GENERATED_FOLDER.
    # While it is not there, the prop is built out of simple shapes instead (a stand-in)
    generated: str
    size: float                 # meters, its longest side
    stands_on_ground: bool      # the temple: its footprint centered at the origin. Otherwise its middle at the origin
    turn_degrees: float         # around the vertical axis, so a generated model's front is towards -Y like the shapes'
    colors: dict
    surfaces: dict
    build: object
    texture_pixels: int
    # A thrown prop is laid along its flight by its longest side. Not one whose longest side isn't its length
    # (a jet's wings are as wide as it is long): 'turn_degrees' alone puts its nose forward
    lies_along_longest: bool = True
    # A flat thing thrown face first (a star): its thinnest side along the flight, not its longest
    face_along_flight: bool = False
    # A generated model with more faces than this is lightened (a projectile that falls by the hundred): 0 keeps them all
    faces: int = 0


PROPS = {
    "sih_oil_bottle": Prop("OilBottle", "prop_oil_bottle.glb", 0.56, False, 0.0, BOTTLE_COLORS, None, build_bottle, 256),
    "sih_money":      Prop("Money", "prop_money.glb", 0.44, False, 0.0, MONEY_COLORS, None, build_money, 256),
    "sih_black_book": Prop("BlackBook", "prop_black_book.glb", 0.45, False, 0.0, BOOK_COLORS, None, build_book, 256),
    "sih_tape":       Prop("Tape", "prop_tape.glb", 0.42, False, 0.0, TAPE_COLORS, None, build_tape, 256),
    # What fills the sky of his domain and rains down (Meshy, made by the user: Hexagon Bullet, FBX with its textures)
    "sih_hex_bullet": Prop("HexBullet", "hexagon_bullet/Meshy_AI_Hexagon_Bullet_20261117_texture.fbx", 0.5, False, 0.0,
                           {}, None, None, 0, face_along_flight=True, faces=1500),
    "sih_jet":        Prop("Jet", "prop_jet.glb", 9.2, False, 90.0, JET_COLORS, None, build_jet, 1024, lies_along_longest=False),
    "sih_temple":     Prop("Temple", "temple.glb", 30.0, True, 0.0, TEMPLE_COLORS, TEMPLE_SURFACES, build_temple, 2048),
    # The street the fight is fought on (built of simple shapes: it has no generated model)
    "sih_ground":     Prop("Ground", "", 8.0, True, 0.0, GROUND_COLORS, GROUND_SURFACES, build_ground, 2048),
    # The ruins around it (Meshy, from concept pictures of the same names)
    "sih_tower_a":    Prop("TowerA", "city_tower_a.glb", 60.0, True, 0.0, {}, None, None, 0),
    "sih_tower_b":    Prop("TowerB", "city_tower_b.glb", 42.0, True, 0.0, {}, None, None, 0),
    "sih_tower_c":    Prop("TowerC", "city_tower_c.glb", 52.0, True, 0.0, {}, None, None, 0),
    "sih_rubble":     Prop("Rubble", "city_rubble.glb", 11.0, True, 0.0, {}, None, None, 0),
}


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--only", default="", help="a part of the names of the props to make, e.g. 'temple'")
    parser.add_argument("--preview", default="")
    return parser.parse_args(argv)


def load_generated(path, prop, name):
    """The generated model as one mesh: 'size' along its longest side, lying along Y when it is thrown (its longest
    side is the way it flies), in its place around the origin"""
    objects_before = set(bpy.data.objects)
    if path.lower().endswith(".fbx"):
        bpy.ops.import_scene.fbx(filepath=path)
    else:
        bpy.ops.import_scene.gltf(filepath=path)
    imported = [scene_object for scene_object in bpy.data.objects if scene_object not in objects_before]
    model = next(scene_object for scene_object in imported if scene_object.type == "MESH")
    world_matrix = model.matrix_world.copy()
    model.parent = None
    for scene_object in imported:
        if scene_object is not model:
            bpy.data.objects.remove(scene_object)
    model.data.transform(Matrix.Rotation(math.radians(prop.turn_degrees), 4, "Z") @ world_matrix)
    model.matrix_world = Matrix.Identity(4)

    def bounds():
        points = [vertex.co for vertex in model.data.vertices]
        return ([min(point[axis] for point in points) for axis in range(3)],
                [max(point[axis] for point in points) for axis in range(3)])

    lowest, highest = bounds()
    sides = [highest[axis] - lowest[axis] for axis in range(3)]
    longest = max(range(3), key=lambda axis: sides[axis])
    thinnest = min(range(3), key=lambda axis: sides[axis])
    if prop.face_along_flight:
        if thinnest != 1:
            model.data.transform(Matrix.Rotation(math.radians(90.0), 4, "X" if thinnest == 2 else "Z"))
            lowest, highest = bounds()
    elif not prop.stands_on_ground and prop.lies_along_longest and longest != 1:
        # Standing up (a bottle) or lying across: laid along its flight
        model.data.transform(Matrix.Rotation(math.radians(90.0), 4, "X" if longest == 2 else "Z"))
        lowest, highest = bounds()

    middle = [(lowest[axis] + highest[axis]) * 0.5 for axis in range(3)]
    down = lowest[2] if prop.stands_on_ground else middle[2]
    model.data.transform(Matrix.Scale(prop.size / max(sides), 4) @ Matrix.Translation((-middle[0], -middle[1], -down)))
    model.name = name

    if prop.faces and len(model.data.polygons) > prop.faces:
        decimate = model.modifiers.new("Lighten", "DECIMATE")
        decimate.ratio = prop.faces / len(model.data.polygons)
        bpy.context.view_layer.objects.active = model
        model.select_set(True)
        bpy.ops.object.modifier_apply(modifier=decimate.name)
    return model


def render_previews(folder, name, view_meters):
    os.makedirs(folder, exist_ok=True)
    camera = setup_preview_render(640, 520)
    for angle in (25, 205):
        # From the front (the door of the temple, the nose of a projectile is towards -Y) and from behind
        place_preview_camera(camera, angle + 180, distance=view_meters * 1.5, height=view_meters * 0.55, target_height=view_meters * 0.18)
        bpy.context.scene.render.filepath = os.path.join(folder, "{}_{}.png".format(name, angle))
        bpy.ops.render.render(write_still=True)


def make_prop(name, arguments):
    prop = PROPS[name]
    folder = os.path.join(OUT_FOLDER, prop.folder)
    os.makedirs(folder, exist_ok=True)

    clear_scene()
    generated = os.path.join(GENERATED_FOLDER, prop.generated)

    if not prop.generated and prop.build is None or (prop.build is None and not os.path.exists(generated)):
        print("PROP {} skipped: its generated model {} is not there".format(name, prop.generated))
        return

    if prop.generated and os.path.exists(generated):
        model = load_generated(generated, prop, name)
        character_from_mesh.make_engine_textures(model, folder, name, GENERATED_BRIGHTNESS)
        print("PROP {} from {}".format(name, prop.generated))
    else:
        palette = Palette(prop.colors, prop.surfaces)
        parts = PartBuilder(palette)
        prop.build(parts)
        model = parts.to_object(name)
        setup_bake_render()
        bake_textures(model, palette, name, folder, prop.texture_pixels)
        print("PROP {} from simple shapes (no generated model yet)".format(name))

    export_gltf(os.path.join(folder, name + ".gltf"), [model], with_animations=False)

    if arguments.preview:
        render_previews(os.path.abspath(arguments.preview), name, prop.size * 1.3)


def main():
    arguments = parse_arguments()
    for name in PROPS:
        if arguments.only in name:
            make_prop(name, arguments)
    print("PROPS_GENERATOR_DONE")


if __name__ == "__main__":
    main()
