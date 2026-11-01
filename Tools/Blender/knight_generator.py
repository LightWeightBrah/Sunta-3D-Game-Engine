"""
Generates the Knight player model, its animations, its sword and its shield for the Sunta engine.

Run (from the repository root):
    blender --background --factory-startup --python Tools/Blender/knight_generator.py -- --out Sunta/res/Models

Writes:
    <out>/Knight/knight.gltf (+ .bin, knight_diffuse.png, knight_specular.png)                   the player
    <out>/RustedKnight/rusted_knight.gltf (+ .bin, rusted_knight_diffuse.png, ...)               the enemy: same body, rusted armor
    <out>/KnightSword/knight_sword.gltf (+ .bin, knight_sword_diffuse.png, knight_sword_specular.png)
    <out>/KnightShield/knight_shield.gltf (+ .bin, knight_shield_diffuse.png, knight_shield_specular.png)
Optional: --preview <folder> renders pictures of the model and of every animation, --blend <file> saves the .blend,
--icons <folder> renders the editor icons (souls_player_icon.png, rusted_knight_icon.png: pictures of the helmets)

The armor is built from simple shapes, every color is a cell of a small palette. Cycles then bakes the final textures:
the palette color with fine metal grain, large stains, darker crevices (ambient occlusion) and brighter worn edges

Conventions (Blender axes): X = the knight's right, Y = forward, Z = up
The glTF exporter turns this into the engine's axes: Y up and forward along -Z

Every armor piece follows exactly one bone (rigid skinning), the joints are hidden under overlapping plates
The animations are Mixamo motion capture moved onto this skeleton (see ANIMATIONS): the Mixamo files must be downloaded first
The sword and the shield are separate models. The bones 'Weapon_R' (right hand) and 'Shield_L' (left hand) are placed
so that the model attached to them without any offset sits in the hand (in the engine: a child entity on that bone)
"""

import argparse
import math
import os
import sys

import bpy
from mathutils import Matrix, Quaternion, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from mixamo_retarget import MixamoClip, retarget  # noqa: E402
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

# ========== Colors ==========

# name: (diffuse RGB, specular intensity)
KNIGHT_COLORS = {
    "steel":        ((0.40, 0.41, 0.43), 0.75),
    "steel_dark":   ((0.20, 0.21, 0.22), 0.55),
    "steel_worn":   ((0.33, 0.33, 0.34), 0.55),
    "mail":         ((0.24, 0.24, 0.25), 0.45),
    "leather":      ((0.22, 0.14, 0.08), 0.12),
    "leather_dark": ((0.12, 0.08, 0.05), 0.08),
    "brass":        ((0.46, 0.35, 0.16), 0.70),
    "black":        ((0.01, 0.01, 0.01), 0.00),
}

# The enemy: the same armor, rusted and dark, with rotten leather
RUSTED_COLORS = {
    "steel":        ((0.27, 0.24, 0.21), 0.45),
    "steel_dark":   ((0.12, 0.11, 0.10), 0.30),
    "steel_worn":   ((0.33, 0.21, 0.12), 0.25),
    "mail":         ((0.17, 0.15, 0.13), 0.30),
    "leather":      ((0.14, 0.10, 0.07), 0.08),
    "leather_dark": ((0.08, 0.06, 0.04), 0.05),
    "brass":        ((0.26, 0.19, 0.09), 0.35),
    "black":        ((0.01, 0.01, 0.01), 0.00),
}

# Photo surfaces (Poly Haven, CC0): leather straps on both, the enemy's armor eaten by rust
LEATHER_SURFACE = SurfaceTexture("brown_leather_diff_1k.jpg", meters=0.3)
RUST_SURFACE = SurfaceTexture("rust_coarse_01_diff_1k.jpg", meters=0.5)

KNIGHT_SURFACES = {
    "leather":      LEATHER_SURFACE,
    "leather_dark": LEATHER_SURFACE,
}

RUSTED_SURFACES = {
    "steel":        RUST_SURFACE,
    "steel_dark":   RUST_SURFACE,
    "steel_worn":   RUST_SURFACE,
    "mail":         SurfaceTexture("rust_coarse_01_diff_1k.jpg", meters=0.5, strength=0.6),
    "leather":      LEATHER_SURFACE,
    "leather_dark": LEATHER_SURFACE,
}

SHIELD_COLORS = {
    "wood":       ((0.30, 0.21, 0.13), 0.10),
    "paint":      ((0.30, 0.07, 0.05), 0.15),
    "iron":       ((0.24, 0.24, 0.25), 0.60),
    "steel":      ((0.40, 0.41, 0.43), 0.75),
    "steel_dark": ((0.20, 0.21, 0.22), 0.55),
    "leather":    ((0.22, 0.14, 0.08), 0.12),
}

SHIELD_SURFACES = {
    "wood":    SurfaceTexture("old_wood_floor_diff_1k.jpg", meters=0.6),
    "paint":   SurfaceTexture("old_wood_floor_diff_1k.jpg", meters=0.6, strength=0.6),
    "iron":    SurfaceTexture("rust_coarse_01_diff_1k.jpg", meters=0.4, strength=0.5),
    "leather": LEATHER_SURFACE,
}

SWORD_COLORS = {
    "blade":       ((0.62, 0.63, 0.65), 1.00),
    "blade_edge":  ((0.78, 0.79, 0.80), 1.00),
    "fuller":      ((0.34, 0.35, 0.37), 0.70),
    "guard":       ((0.26, 0.27, 0.28), 0.70),
    "brass":       ((0.46, 0.35, 0.16), 0.70),
    "grip":        ((0.13, 0.08, 0.05), 0.08),
    "wrap":        ((0.22, 0.14, 0.08), 0.12),
}

KNIGHT_TEXTURE_PIXELS = 2048

SWORD_TEXTURE_PIXELS = 1024

# ========== Skeleton ==========

# name, parent, head, tail, the direction the bone's Z axis should point to (roll)
FORWARD = (0.0, 1.0, 0.0)
UP = (0.0, 0.0, 1.0)

SKELETON = [
    ("Hips",       None,         (0.00, 0.00, 0.98), (0.00, 0.00, 1.10), FORWARD),
    ("Spine",      "Hips",       (0.00, 0.00, 1.10), (0.00, 0.00, 1.28), FORWARD),
    ("Chest",      "Spine",      (0.00, 0.00, 1.28), (0.00, 0.00, 1.50), FORWARD),
    ("Neck",       "Chest",      (0.00, 0.00, 1.50), (0.00, 0.00, 1.60), FORWARD),
    ("Head",       "Neck",       (0.00, 0.00, 1.60), (0.00, 0.00, 1.82), FORWARD),

    ("Shoulder_R", "Chest",      (0.04, 0.00, 1.45), (0.19, 0.00, 1.45), UP),
    ("UpperArm_R", "Shoulder_R", (0.19, 0.00, 1.45), (0.23, 0.00, 1.17), FORWARD),
    ("LowerArm_R", "UpperArm_R", (0.23, 0.00, 1.17), (0.25, 0.03, 0.92), FORWARD),
    ("Hand_R",     "LowerArm_R", (0.25, 0.03, 0.92), (0.25, 0.04, 0.82), FORWARD),

    ("Shoulder_L", "Chest",      (-0.04, 0.00, 1.45), (-0.19, 0.00, 1.45), UP),
    ("UpperArm_L", "Shoulder_L", (-0.19, 0.00, 1.45), (-0.23, 0.00, 1.17), FORWARD),
    ("LowerArm_L", "UpperArm_L", (-0.23, 0.00, 1.17), (-0.25, 0.03, 0.92), FORWARD),
    ("Hand_L",     "LowerArm_L", (-0.25, 0.03, 0.92), (-0.25, 0.04, 0.82), FORWARD),

    ("Thigh_R",    "Hips",       (0.10, 0.00, 0.97), (0.11, 0.00, 0.54), FORWARD),
    ("Shin_R",     "Thigh_R",    (0.11, 0.00, 0.54), (0.11, -0.01, 0.10), FORWARD),
    ("Foot_R",     "Shin_R",     (0.11, -0.01, 0.10), (0.11, 0.14, 0.03), UP),

    ("Thigh_L",    "Hips",       (-0.10, 0.00, 0.97), (-0.11, 0.00, 0.54), FORWARD),
    ("Shin_L",     "Thigh_L",    (-0.11, 0.00, 0.54), (-0.11, -0.01, 0.10), FORWARD),
    ("Foot_L",     "Shin_L",     (-0.11, -0.01, 0.10), (-0.11, 0.14, 0.03), UP),
]

WEAPON_BONE = "Weapon_R"
WEAPON_PARENT_BONE = "Hand_R"
SHIELD_BONE = "Shield_L"
SHIELD_PARENT_BONE = "Hand_L"
HELD_ITEM_BONE_LENGTH = 0.15

# Where the sword grip sits in the rest pose (the middle of the right fist) and where its blade points:
# forward and 55 degrees down, the edges up and down
SWORD_GRIP_POSITION = Vector((0.25, 0.045, 0.855))
SWORD_BLADE_DOWN_DEGREES = 55.0

# The shield is held by a grip behind its middle: in the rest pose (arm hanging) it covers the outside of the left forearm,
# its face looking away from the body and its top forward. Raising the forearm across the chest (Mixamo's block)
# then turns the face forward
SHIELD_CENTER = Vector((-0.32, 0.02, 1.04))
SHIELD_FACE = Vector((-1.0, 0.0, 0.0))
SHIELD_TOP = Vector((0.0, 1.0, 0.0))

# ========== Shield size (meters) ==========

SHIELD_RADIUS = 0.30
SHIELD_THICKNESS = 0.03

# ========== Sword size (meters) ==========

SWORD_BLADE_LENGTH = 0.86
SWORD_BLADE_HALF_WIDTH = 0.026
SWORD_BLADE_HALF_THICKNESS = 0.007
SWORD_GRIP_HALF_LENGTH = 0.10
SWORD_GUARD_HALF_WIDTH = 0.12

# ========== Small helpers ==========


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--preview", default="")
    parser.add_argument("--blend", default="")
    parser.add_argument("--icons", default="")
    return parser.parse_args(argv)


# ========== The knight ==========


def build_skeleton():
    armature_data = bpy.data.armatures.new("KnightArmature")
    armature = bpy.data.objects.new("Armature", armature_data)
    bpy.context.scene.collection.objects.link(armature)
    bpy.context.view_layer.objects.active = armature

    bpy.ops.object.mode_set(mode="EDIT")
    edit_bones = armature_data.edit_bones

    for name, parent, head, tail, roll_axis in SKELETON:
        bone = edit_bones.new(name)
        bone.head, bone.tail = head, tail
        bone.align_roll(Vector(roll_axis))
        if parent:
            bone.parent = edit_bones[parent]
            bone.use_connect = False

    add_held_item_bone(edit_bones, WEAPON_BONE, WEAPON_PARENT_BONE, get_sword_rest_matrix())
    add_held_item_bone(edit_bones, SHIELD_BONE, SHIELD_PARENT_BONE, get_shield_rest_matrix())

    bpy.ops.object.mode_set(mode="OBJECT")
    return armature


def add_held_item_bone(edit_bones, name, parent_name, item_rest_matrix):
    """A bone that carries a held model: the model attached to it without any offset stands at 'item_rest_matrix'"""
    bone = edit_bones.new(name)
    # A new bone has no length and no direction, it takes the matrix only after it got some
    bone.head, bone.tail = (0.0, 0.0, 0.0), (0.0, HELD_ITEM_BONE_LENGTH, 0.0)
    bone.matrix = get_held_item_bone_matrix(item_rest_matrix)
    bone.parent = edit_bones[parent_name]
    bone.use_deform = False


def get_sword_rest_matrix():
    """Where the sword object stands in the rest pose (Blender axes, sword built with the blade along +Z)"""
    angle = math.radians(SWORD_BLADE_DOWN_DEGREES)
    blade = Vector((0.0, math.cos(angle), -math.sin(angle)))
    edge = Vector((0.0, math.sin(angle), math.cos(angle)))
    flat = blade.cross(edge)
    rotation = Matrix((edge, flat, blade)).transposed()
    return Matrix.Translation(SWORD_GRIP_POSITION) @ rotation.to_4x4()


def get_shield_rest_matrix():
    """Where the shield object stands in the rest pose (Blender axes, shield built facing +Z with its top along +Y)"""
    side = SHIELD_TOP.cross(SHIELD_FACE)
    rotation = Matrix((side, SHIELD_TOP, SHIELD_FACE)).transposed()
    return Matrix.Translation(SHIELD_CENTER) @ rotation.to_4x4()


# The glTF exporter turns Blender axes (Z up) into glTF axes (Y up): (x, y, z) -> (x, z, -y)
BLENDER_TO_GLTF = Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, -1, 0, 0), (0, 0, 0, 1)))


def get_held_item_bone_matrix(item_rest_matrix):
    """The held model's file is converted to glTF axes on its own, the bone must carry the same frame:
    bone frame in glTF = model placement in glTF, so in Blender the bone frame is model frame * (Blender->glTF)^-1"""
    return item_rest_matrix @ BLENDER_TO_GLTF.inverted()


def build_head(parts):
    """A close helm: rounded skull, a chin plate, a narrow eye slit and breathing holes"""
    parts.sphere("Head", "steel", (0.0, 0.0, 1.725), (0.108, 0.118, 0.128), segments=28, rings=16)
    parts.sphere("Head", "steel_worn", (0.0, 0.03, 1.662), (0.098, 0.1, 0.082), segments=28, rings=14)
    parts.cylinder("Head", "steel_dark", make_matrix((0.0, 0.005, 1.612)), 0.1, 0.098, 0.03, segments=28)

    # The eye slit follows the curve of the face in four short pieces
    for angle_degrees in (-36, -12, 12, 36):
        angle = math.radians(angle_degrees)
        location = (math.sin(angle) * 0.106, math.cos(angle) * 0.116, 1.738)
        parts.box("Head", "black", location, (0.048, 0.02, 0.011), rotation=(0, 0, -angle_degrees), bevel=0.0)

    # Breathing holes on the right cheek
    for row in range(3):
        for column in range(2):
            angle_degrees = 22 + column * 12
            angle = math.radians(angle_degrees)
            location = (math.sin(angle) * 0.098, math.cos(angle) * 0.1, 1.675 - row * 0.016)
            parts.box("Head", "black", location, (0.008, 0.016, 0.006), rotation=(0, 0, -angle_degrees), bevel=0.0)


def build_torso(parts):
    # Gorget around the neck
    parts.cylinder("Neck", "steel_worn", make_matrix((0, 0.0, 1.565)), 0.105, 0.088, 0.08)
    parts.cylinder("Chest", "steel", make_matrix((0, 0.0, 1.515)), 0.15, 0.11, 0.05)

    # Breastplate with a ridge down the middle and a dark rim at the bottom
    parts.sphere("Chest", "steel", (0.0, 0.02, 1.37), (0.2, 0.152, 0.19), segments=28, rings=16)
    parts.box("Chest", "steel_worn", (0.0, 0.158, 1.40), (0.024, 0.024, 0.2), rotation=(0, 0, 45))
    parts.cylinder("Chest", "steel_dark", make_matrix((0, 0.015, 1.255)), 0.178, 0.186, 0.028, scale=(1.0, 0.8, 1.0))

    # Belly under the plate, belt with a buckle
    parts.cylinder("Spine", "mail", make_matrix((0, 0.0, 1.18)), 0.16, 0.17, 0.18, scale=(1.0, 0.8, 1.0))
    parts.cylinder("Hips", "leather", make_matrix((0, 0.0, 1.05)), 0.175, 0.175, 0.05, scale=(1.0, 0.8, 1.0))
    parts.box("Hips", "brass", (0.0, 0.145, 1.05), (0.05, 0.016, 0.045))

    # Fauld (the plates over the hips) and a mail skirt that stays short, so the legs never cut through it
    parts.cylinder("Hips", "steel", make_matrix((0, 0.0, 0.975)), 0.212, 0.18, 0.10, scale=(1.0, 0.82, 1.0))
    parts.cylinder("Hips", "mail", make_matrix((0, 0.0, 0.90)), 0.20, 0.205, 0.07, scale=(1.0, 0.82, 1.0))


def build_arm(parts, side):
    suffix = "_R" if side > 0 else "_L"

    def mirrored(point):
        return (point[0] * side, point[1], point[2])

    shoulder, upper_arm, lower_arm, hand = ("Shoulder" + suffix, "UpperArm" + suffix, "LowerArm" + suffix, "Hand" + suffix)

    # Pauldron: three overlapping plates with a dark rim
    for index, (radius, z) in enumerate(((0.1, 1.47), (0.092, 1.425), (0.084, 1.39))):
        parts.half_sphere(shoulder, "steel" if index == 0 else "steel_worn", mirrored((0.20, 0.0, z)), (radius, radius * 0.95, radius * 0.7),
                          rotation=(0, side * 25, 0))
    parts.cylinder(shoulder, "steel_dark", make_matrix(mirrored((0.20, 0.0, 1.466)), (0, side * 25, 0)), 0.102, 0.102, 0.012)

    # Upper arm: mail with a plate on the outside
    parts.limb(upper_arm, "mail", mirrored((0.19, 0.0, 1.42)), mirrored((0.23, 0.0, 1.17)), 0.058, 0.05)
    parts.limb(upper_arm, "steel_worn", mirrored((0.20, 0.0, 1.36)), mirrored((0.23, 0.0, 1.20)), 0.062, 0.055)

    # Elbow cop with a small round wing
    parts.sphere(lower_arm, "steel", mirrored((0.23, 0.0, 1.17)), (0.055, 0.055, 0.055))
    parts.cylinder(lower_arm, "steel_worn", make_matrix(mirrored((0.265, -0.005, 1.17)), (0, 90, 0)), 0.045, 0.045, 0.012)

    # Vambrace and gauntlet
    parts.limb(lower_arm, "steel", mirrored((0.23, 0.0, 1.15)), mirrored((0.25, 0.03, 0.95)), 0.05, 0.042)
    parts.limb(hand, "steel_worn", mirrored((0.25, 0.03, 0.96)), mirrored((0.25, 0.034, 0.90)), 0.05, 0.062)
    parts.box(hand, "steel_dark", mirrored((0.25, 0.04, 0.86)), (0.05, 0.08, 0.09))
    parts.box(hand, "steel_dark", mirrored((0.25, 0.075, 0.835)), (0.048, 0.04, 0.06))


def build_leg(parts, side):
    suffix = "_R" if side > 0 else "_L"

    def mirrored(point):
        return (point[0] * side, point[1], point[2])

    thigh, shin, foot = "Thigh" + suffix, "Shin" + suffix, "Foot" + suffix

    # Cuisse (thigh plate) with a tasset hanging over it
    parts.limb(thigh, "mail", mirrored((0.10, 0.0, 0.97)), mirrored((0.11, 0.0, 0.56)), 0.085, 0.065)
    parts.limb(thigh, "steel", mirrored((0.10, 0.012, 0.88)), mirrored((0.11, 0.012, 0.58)), 0.08, 0.066)
    parts.box(thigh, "steel_worn", mirrored((0.105, 0.075, 0.86)), (0.14, 0.025, 0.14), rotation=(8, 0, 0))

    # Knee cop with a pointed front
    parts.sphere(shin, "steel", mirrored((0.11, 0.015, 0.54)), (0.065, 0.06, 0.065))
    parts.box(shin, "steel_worn", mirrored((0.11, 0.07, 0.545)), (0.045, 0.04, 0.045), rotation=(0, 45, 0))

    # Greave
    parts.limb(shin, "steel", mirrored((0.11, 0.0, 0.52)), mirrored((0.11, -0.01, 0.13)), 0.062, 0.048)

    # Sabaton
    parts.cylinder(foot, "steel_worn", make_matrix(mirrored((0.11, -0.005, 0.11))), 0.055, 0.05, 0.07)
    parts.box(foot, "steel_dark", mirrored((0.11, 0.04, 0.05)), (0.10, 0.20, 0.08))
    parts.box(foot, "steel_worn", mirrored((0.11, 0.155, 0.04)), (0.085, 0.08, 0.06), rotation=(-12, 0, 0))
    parts.box(foot, "leather_dark", mirrored((0.11, 0.03, 0.006)), (0.10, 0.22, 0.012), bevel=0.002)


def build_knight_mesh(armature, palette, name):
    parts = PartBuilder(palette)
    build_head(parts)
    build_torso(parts)
    for side in (1, -1):
        build_arm(parts, side)
        build_leg(parts, side)
    return parts.to_object(name, armature)


# ========== The shield ==========


def build_shield_mesh(palette):
    """A round wooden shield: painted planks in an iron rim, a steel boss and rivets on the face, a leather grip behind.
    Built with the grip at the origin, the face towards +Z and the top along +Y"""
    parts = PartBuilder(palette)
    face_z = SHIELD_THICKNESS * 0.5

    # The rim is thinner than the planks, so only its edge shows: no two surfaces may lie in one plane,
    # the graphics card would draw one or the other from frame to frame (flickering)
    parts.cylinder(None, "iron", make_matrix((0.0, 0.0, 0.0)), SHIELD_RADIUS + 0.015, SHIELD_RADIUS + 0.015, SHIELD_THICKNESS - 0.01, segments=40)
    parts.cylinder(None, "wood", make_matrix((0.0, 0.0, 0.0)), SHIELD_RADIUS, SHIELD_RADIUS, SHIELD_THICKNESS, segments=40)
    parts.cylinder(None, "paint", make_matrix((0.0, 0.0, face_z + 0.002)), SHIELD_RADIUS - 0.012, SHIELD_RADIUS - 0.012, 0.006, segments=40)

    # A pale band across the paint, under the boss
    parts.box(None, "wood", (0.0, 0.0, face_z + 0.004), (0.09, SHIELD_RADIUS * 1.9, 0.006), bevel=0.002)

    parts.half_sphere(None, "steel", (0.0, 0.0, face_z + 0.004), (0.075, 0.075, 0.06), segments=24, rings=10)
    parts.cylinder(None, "steel_dark", make_matrix((0.0, 0.0, face_z + 0.006)), 0.095, 0.095, 0.008, segments=24)

    for index in range(12):
        angle = 2.0 * math.pi * index / 12
        location = (math.cos(angle) * (SHIELD_RADIUS - 0.03), math.sin(angle) * (SHIELD_RADIUS - 0.03), face_z + 0.006)
        parts.sphere(None, "steel_dark", location, (0.011, 0.011, 0.008), segments=8, rings=6)

    # The grip behind the boss, across the shield, and the strap over the forearm
    parts.box(None, "leather", (0.0, 0.0, -face_z - 0.035), (0.13, 0.03, 0.025))
    parts.box(None, "iron", (-0.08, 0.0, -face_z - 0.018), (0.02, 0.035, 0.035))
    parts.box(None, "iron", (0.08, 0.0, -face_z - 0.018), (0.02, 0.035, 0.035))
    parts.box(None, "leather", (0.0, -0.12, -face_z - 0.03), (0.16, 0.035, 0.012))

    return parts.to_object("KnightShield")


# ========== The sword ==========


def build_sword_mesh(palette):
    """Built with the grip in the middle of the origin and the blade along +Z"""
    parts = PartBuilder(palette)

    guard_z = SWORD_GRIP_HALF_LENGTH
    blade_start = guard_z + 0.012
    blade_end = blade_start + SWORD_BLADE_LENGTH

    add_blade(parts, blade_start, blade_end)

    # Crossguard, slightly bent towards the blade, with rounded tips
    for side in (-1, 1):
        parts.box(None, "guard", (side * SWORD_GUARD_HALF_WIDTH * 0.5, 0.0, guard_z + 0.004),
                  (SWORD_GUARD_HALF_WIDTH, 0.022, 0.022), rotation=(0, side * -8, 0))
        parts.sphere(None, "brass", (side * SWORD_GUARD_HALF_WIDTH, 0.0, guard_z + 0.012), (0.017, 0.017, 0.017), segments=12, rings=8)
    parts.box(None, "brass", (0.0, 0.0, guard_z + 0.004), (0.05, 0.03, 0.034))

    # Grip wrapped in leather, pommel
    parts.cylinder(None, "grip", make_matrix((0, 0, 0)), 0.016, 0.016, SWORD_GRIP_HALF_LENGTH * 2, segments=12)
    for index in range(5):
        z = -SWORD_GRIP_HALF_LENGTH + 0.02 + index * 0.04
        parts.cylinder(None, "wrap", make_matrix((0, 0, z), (12, 0, 0)), 0.018, 0.018, 0.012, segments=12)
    parts.sphere(None, "brass", (0, 0, -SWORD_GRIP_HALF_LENGTH - 0.022), (0.03, 0.03, 0.026), segments=16, rings=8)
    parts.cylinder(None, "guard", make_matrix((0, 0, -SWORD_GRIP_HALF_LENGTH - 0.002)), 0.02, 0.022, 0.012, segments=12)

    return parts.to_object("KnightSword")


def add_blade(parts, blade_start, blade_end):
    """A diamond cross section (edges along X, ridge along Y) that narrows into a point"""
    blade_mesh = parts.mesh
    uv_blade = parts.palette.uv("blade")
    uv_edge = parts.palette.uv("blade_edge")
    uv_fuller = parts.palette.uv("fuller")

    point_start = blade_end - 0.14
    sections = [
        (blade_start, 1.0, 1.0),
        (blade_start + 0.25, 0.96, 1.0),
        (point_start, 0.82, 0.95),
        (blade_end - 0.05, 0.38, 0.7),
    ]

    rings = []
    for z, width_factor, thickness_factor in sections:
        width = SWORD_BLADE_HALF_WIDTH * width_factor
        thickness = SWORD_BLADE_HALF_THICKNESS * thickness_factor
        rings.append([blade_mesh.verts.new(point) for point in
                      ((width, 0, z), (0, thickness, z), (-width, 0, z), (0, -thickness, z))])
    tip = blade_mesh.verts.new((0, 0, blade_end))

    faces = []
    for lower, upper in zip(rings, rings[1:]):
        for corner in range(4):
            next_corner = (corner + 1) % 4
            faces.append(blade_mesh.faces.new((lower[corner], lower[next_corner], upper[next_corner], upper[corner])))
    for corner in range(4):
        faces.append(blade_mesh.faces.new((rings[-1][corner], rings[-1][(corner + 1) % 4], tip)))
    faces.append(blade_mesh.faces.new(list(reversed(rings[0]))))

    for face in faces:
        center = face.calc_center_median()
        uv = uv_edge if center.z > point_start else uv_blade
        for loop in face.loops:
            loop[parts.uv_layer].uv = uv

    # A darker groove (fuller) along both flats of the blade
    for side in (-1, 1):
        parts.box(None, "fuller", (0.0, side * SWORD_BLADE_HALF_THICKNESS * 0.62, (blade_start + point_start) * 0.5 - 0.03),
                  (SWORD_BLADE_HALF_WIDTH * 0.45, 0.002, point_start - blade_start - 0.12))
    del uv_fuller


# ========== Animations ==========

# The motion comes from Mixamo (motion capture), moved onto the knight's skeleton by mixamo_retarget.py
# Every animation is a part of one Mixamo file (Mixamo frames at 30 fps, inclusive) played at some speed
# The engine finds the animations by these names (Character Animation, Character Actions), so they stay the same
#
# Files (downloaded as "FBX Binary, Without Skin" into Tools/Blender/Mixamo, see MIXAMO_FOLDER):
#   ProSwordShield/   the "Pro Sword And Shield Pack", unzipped
#   Sprinting Forward Roll.fbx, Standing Dodge Backward.fbx, Kneel Campfire.fbx ("Kneel", warming hands over a campfire)
#
# The attacks are cut so that the blade strikes inside the hit windows of Character Actions
# (light: 0.28..0.56 of the animation, heavy: 0.45..0.66), check with check_attack_reach.py

PACK = "ProSwordShield/sword and shield "

ANIMATIONS = {
    # Walking and standing, looped by the engine
    "Idle":           MixamoClip(PACK + "idle (4).fbx"),
    "Walk":           MixamoClip(PACK + "walk.fbx"),
    "Run":            MixamoClip(PACK + "run.fbx"),
    "Walk_Back":      MixamoClip(PACK + "walk (2).fbx"),
    "Strafe_Left":    MixamoClip(PACK + "strafe (2).fbx"),
    "Strafe_Right":   MixamoClip(PACK + "strafe.fbx"),

    # Actions, played once. The roll and the backstep without the running before and after them
    "Roll":           MixamoClip("Sprinting Forward Roll.fbx", 4, 36, speed=1.1),
    "Backstep":       MixamoClip("Standing Dodge Backward.fbx", 4, 40, speed=1.2),
    "Attack_Light_1": MixamoClip(PACK + "slash.fbx", 4, 40, speed=1.3),
    "Attack_Light_2": MixamoClip(PACK + "slash (3).fbx", 10, 45, speed=1.3),
    "Attack_Light_3": MixamoClip(PACK + "attack (2).fbx", 6, 34, speed=1.1),
    "Attack_Heavy":   MixamoClip(PACK + "slash (2).fbx", 4, 36, speed=0.9),
    "Hit":            MixamoClip(PACK + "impact (2).fbx"),
    "Parried":        MixamoClip(PACK + "impact (3).fbx", speed=0.6),
    "Death":          MixamoClip(PACK + "death.fbx"),

    # The shield: held up (the engine fades into it, the last frame is held while blocking), a hit taken on it,
    # and a parry (a fast raise of the shield)
    "Block":          MixamoClip(PACK + "block idle.fbx"),
    "Block_Impact":   MixamoClip(PACK + "impact.fbx"),
    "Parry":          MixamoClip(PACK + "block.fbx", speed=1.5),

    # Resting at a bonfire: kneeling down (the last frame is held), and getting up again
    "Rest":           MixamoClip("Kneel Campfire.fbx", 50, 110, speed=1.2),
    "Stand_Up":       MixamoClip("Kneel Campfire.fbx", 188, 238, speed=1.3),
}

MIXAMO_FOLDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Mixamo")


def create_animations(armature):
    armature.animation_data_create()
    actions = []

    for name, clip in ANIMATIONS.items():
        action = bpy.data.actions.new(name)
        action.use_fake_user = True

        last_frame = retarget(armature, clip, MIXAMO_FOLDER, action)
        action.frame_range = (0, last_frame)
        actions.append(action)

    armature.animation_data.action = actions[0]
    return actions


# ========== Preview pictures ==========

ICON_PIXELS = 256
ICON_CAMERA_ANGLE = 30
ICON_CAMERA_DISTANCE = 1.0


def reset_to_rest_pose(armature):
    armature.animation_data.action = None
    for pose_bone in armature.pose.bones:
        pose_bone.rotation_quaternion = Quaternion()
        pose_bone.location = Vector()
    bpy.context.view_layer.update()


def render_icon(path, armature):
    """The toolbar icon of the editor: the helmet and the shoulders, on a transparent background"""
    reset_to_rest_pose(armature)
    camera = setup_preview_render(ICON_PIXELS, ICON_PIXELS, is_transparent=True)
    place_preview_camera(camera, ICON_CAMERA_ANGLE, distance=ICON_CAMERA_DISTANCE, height=1.62, target_height=1.56)

    scene = bpy.context.scene
    scene.render.filepath = os.path.abspath(path)
    bpy.ops.render.render(write_still=True)


def render_previews(folder, armature, actions):
    os.makedirs(folder, exist_ok=True)
    camera = setup_preview_render()
    scene = bpy.context.scene

    reset_to_rest_pose(armature)

    for label, angle in (("front", 0), ("side", 90), ("back", 180), ("three_quarter", 35)):
        place_preview_camera(camera, angle)
        scene.render.filepath = os.path.join(folder, "rest_" + label + ".png")
        bpy.ops.render.render(write_still=True)

    place_preview_camera(camera, 60)
    for action in actions:
        armature.animation_data.action = action
        start, end = (int(frame) for frame in action.frame_range)
        for step in range(4):
            frame = start + round((end - start) * step / 3)
            scene.frame_set(frame)
            scene.render.filepath = os.path.join(folder, "{}_{:02d}.png".format(action.name, frame))
            bpy.ops.render.render(write_still=True)


# ========== Main ==========


def hang_on_bone(held_object, armature, bone_name, rest_matrix):
    held_object.parent = armature
    held_object.parent_type = "BONE"
    held_object.parent_bone = bone_name
    held_object.matrix_world = rest_matrix



def main():
    arguments = parse_arguments()
    out_folder = os.path.abspath(arguments.out)
    knight_folder = os.path.join(out_folder, "Knight")
    rusted_folder = os.path.join(out_folder, "RustedKnight")
    sword_folder = os.path.join(out_folder, "KnightSword")
    shield_folder = os.path.join(out_folder, "KnightShield")
    for folder in (knight_folder, rusted_folder, sword_folder, shield_folder):
        os.makedirs(folder, exist_ok=True)

    clear_scene()

    knight_palette = Palette(KNIGHT_COLORS, KNIGHT_SURFACES)
    rusted_palette = Palette(RUSTED_COLORS, RUSTED_SURFACES)
    sword_palette = Palette(SWORD_COLORS)
    shield_palette = Palette(SHIELD_COLORS, SHIELD_SURFACES)

    armature = build_skeleton()
    knight = build_knight_mesh(armature, knight_palette, "Knight")
    rusted_knight = build_knight_mesh(armature, rusted_palette, "RustedKnight")
    actions = create_animations(armature)
    sword = build_sword_mesh(sword_palette)
    shield = build_shield_mesh(shield_palette)

    # Baked in the rest pose: the armor must not be bent by an animation while its texture is made
    reset_to_rest_pose(armature)
    setup_bake_render()
    bake_textures(knight, knight_palette, "knight", knight_folder, KNIGHT_TEXTURE_PIXELS)
    bake_textures(rusted_knight, rusted_palette, "rusted_knight", rusted_folder, KNIGHT_TEXTURE_PIXELS)
    bake_textures(sword, sword_palette, "knight_sword", sword_folder, SWORD_TEXTURE_PIXELS)
    bake_textures(shield, shield_palette, "knight_shield", shield_folder, SWORD_TEXTURE_PIXELS)

    # Both characters share the skeleton and the animations, each file has its own armor
    export_gltf(os.path.join(knight_folder, "knight.gltf"), [armature, knight], with_animations=True)
    export_gltf(os.path.join(rusted_folder, "rusted_knight.gltf"), [armature, rusted_knight], with_animations=True)
    export_gltf(os.path.join(sword_folder, "knight_sword.gltf"), [sword], with_animations=False)
    export_gltf(os.path.join(shield_folder, "knight_shield.gltf"), [shield], with_animations=False)

    # For the pictures (and the .blend) the sword and the shield hang on their bones, exactly as the engine attaches them
    reset_to_rest_pose(armature)
    hang_on_bone(sword, armature, WEAPON_BONE, get_sword_rest_matrix())
    hang_on_bone(shield, armature, SHIELD_BONE, get_shield_rest_matrix())

    if arguments.icons:
        icons_folder = os.path.abspath(arguments.icons)
        show_only([knight, sword, shield])
        render_icon(os.path.join(icons_folder, "souls_player_icon.png"), armature)
        show_only([rusted_knight, sword, shield])
        render_icon(os.path.join(icons_folder, "rusted_knight_icon.png"), armature)

    show_only([knight, sword, shield])

    if arguments.blend:
        bpy.ops.wm.save_as_mainfile(filepath=os.path.abspath(arguments.blend))

    if arguments.preview:
        render_previews(os.path.abspath(arguments.preview), armature, actions)

    print("KNIGHT_GENERATOR_DONE")


if __name__ == "__main__":
    main()
