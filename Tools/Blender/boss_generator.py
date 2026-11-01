"""
Generates the boss of the example game: a giant horned knight in blackened plate armor, and its two-handed great axe

Run (from the repository root, the Mixamo files must be downloaded: knight_generator.py and BOSS_ANIMATIONS below):
    blender --background --factory-startup --python Tools/Blender/boss_generator.py -- --out Sunta/res/Models

Writes:
    <out>/Boss/boss.gltf (+ .bin, boss_diffuse.png, boss_specular.png)              the body with its animations
    <out>/BossAxe/boss_axe.gltf (+ .bin, boss_axe_diffuse.png, boss_axe_specular.png)
Optional: --preview <folder> renders pictures of the boss with its axe

Where they come from: the body is Meshy's model (Image to 3D, A pose) of the first concept Concepts/boss_concept.png
(Z-Image-Turbo), rigged by Meshy; the axe is Meshy's model of Concepts/boss_great_axe_concept.png (Meshy Text to Image).
See Generated/meshy_log.json for every setting. Its bones are renamed to ours, the axe hangs on a bone at the right fist

The boss walks and fights with Mixamo's Great Sword Pack (two-handed cuts and chops; it holds the axe like the fist
it hangs on) and jumps at the player with a slam at the end (Leap Attack in the engine), made by Meshy's Text to Motion.
Meshy's animation library was tried too: its clips hold a weapon in one hand, the axe looked wrong in them
In the engine the boss entity is scaled up (1.5: a 3 m giant), its axe and collider grow with it
"""

import argparse
import math
import os
import sys

import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import character_from_mesh  # noqa: E402
import knight_generator  # noqa: E402
import motion_retarget  # noqa: E402
from motion_retarget import MotionClip  # noqa: E402
from sunta_blender import clear_scene, export_gltf, place_preview_camera, setup_preview_render, show_only  # noqa: E402

BLENDER_FOLDER = os.path.dirname(os.path.abspath(__file__))
# Rigged by Meshy (its own bones and weights), from the first concept of the boss
BODY_SOURCE = os.path.join(BLENDER_FOLDER, "Generated", "boss_meshy_rigged.glb")
AXE_SOURCE = os.path.join(BLENDER_FOLDER, "Generated", "boss_great_axe_meshy.glb")

# Meters, before the engine scales the boss up
BODY_HEIGHT = 2.0
AXE_LENGTH = 2.0
BODY_BRIGHTNESS = 1.3

# Where the right fist holds the haft: this part of the axe's length from the pommel (the left hand holds it higher up)
AXE_GRIP_FROM_BOTTOM = 0.14

# The haft points forward from the fist and this many degrees down, like the knight's sword (Mixamo's sword holding hand)
AXE_DOWN_DEGREES = 55.0

# ========== The animations ==========

# Mixamo's Great Sword Pack (a two-handed weapon)
GREAT_SWORD = "Mixamo/GreatSword/great sword "

# The jump at the player, made by Meshy's Text to Motion from a description (see Generated/meshy_log.json)
LEAP_SLAM = "Generated/Motions/leap_slam.fbx"


BOSS_ANIMATIONS = {
    "Idle":           MotionClip(GREAT_SWORD + "idle.fbx"),
    "Walk":           MotionClip(GREAT_SWORD + "walk.fbx"),
    "Run":            MotionClip(GREAT_SWORD + "run (2).fbx"),
    "Walk_Back":      MotionClip(GREAT_SWORD + "walk (2).fbx"),
    "Strafe_Left":    MotionClip(GREAT_SWORD + "strafe.fbx"),
    "Strafe_Right":   MotionClip(GREAT_SWORD + "strafe (2).fbx"),

    # Two-handed cuts (striking at about half of the animation), three great chops in a row (strikes at 0.25, 0.5
    # and 0.75 of it) and the jump at the player (lands at 0.35); the boss's hit windows in Main.scene follow these
    "Attack_Light_1": MotionClip(GREAT_SWORD + "slash.fbx", speed=0.9),
    "Attack_Light_2": MotionClip(GREAT_SWORD + "slash (3).fbx", speed=0.9),
    "Attack_Light_3": MotionClip(GREAT_SWORD + "slash.fbx", speed=0.8),
    "Attack_Heavy":   MotionClip(GREAT_SWORD + "slash (2).fbx", speed=0.8),
    "Attack_Leap":    MotionClip(LEAP_SLAM, rig=motion_retarget.SMPL),

    "Hit":            MotionClip(GREAT_SWORD + "impact.fbx"),
    "Parried":        MotionClip(GREAT_SWORD + "impact (3).fbx", speed=0.6),
    "Death":          MotionClip("Mixamo/GreatSword/two handed sword death.fbx"),
}


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--preview", default="")
    return parser.parse_args(argv)


# ========== The axe ==========


def load_axe(folder):
    """The axe standing on its pommel at the origin, its haft along +Z, its blade towards -X (as Meshy made it),
    then moved down so that the grip of the right fist is at the origin"""
    clear_selection_imports = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=AXE_SOURCE)
    imported = [obj for obj in bpy.data.objects if obj not in clear_selection_imports]
    axe = next(obj for obj in imported if obj.type == "MESH")
    world_matrix = axe.matrix_world.copy()
    axe.parent = None
    for obj in imported:
        if obj is not axe:
            bpy.data.objects.remove(obj)
    axe.data.transform(world_matrix)
    axe.matrix_world = Matrix.Identity(4)

    lowest = min(vertex.co.z for vertex in axe.data.vertices)
    highest = max(vertex.co.z for vertex in axe.data.vertices)
    scale = AXE_LENGTH / (highest - lowest)
    grip = AXE_LENGTH * AXE_GRIP_FROM_BOTTOM
    axe.data.transform(Matrix.Translation((0.0, 0.0, -grip)) @ Matrix.Scale(scale, 4) @ Matrix.Translation((0.0, 0.0, -lowest)))

    axe.name = "BossAxe"
    character_from_mesh.make_engine_textures(axe, folder, "boss_axe", BODY_BRIGHTNESS)
    print("AXE_HITBOX local_offset (0, {:.3f}, 0) half_extents (0.12, {:.3f}, 0.12) for the whole length above the fist".format(
        (AXE_LENGTH - grip) * 0.5, (AXE_LENGTH - grip) * 0.5))
    return axe


def axe_rest_matrix(grip_position):
    """Where the axe stands in the rest pose: built with the haft along +Z and the blade towards -X"""
    angle = math.radians(AXE_DOWN_DEGREES)
    haft = Vector((0.0, math.cos(angle), -math.sin(angle)))
    toward_back_of_blade = Vector((0.0, math.sin(angle), math.cos(angle)))
    side = haft.cross(toward_back_of_blade)
    rotation = Matrix((toward_back_of_blade, side, haft)).transposed()
    return Matrix.Translation(grip_position) @ rotation.to_4x4()


def held_axe(skeleton):
    return [(knight_generator.WEAPON_BONE, knight_generator.WEAPON_PARENT_BONE,
             axe_rest_matrix(character_from_mesh.fist_center(skeleton, "_R")))]


# ========== Pictures ==========


def render_previews(folder, armature, body, axe):
    os.makedirs(folder, exist_ok=True)
    camera = setup_preview_render(360, 400)
    show_only([body, axe])
    scene = bpy.context.scene

    for action_name in ("Idle", "Attack_Light_1", "Attack_Heavy", "Attack_Leap", "Walk"):
        armature.animation_data.action = bpy.data.actions[action_name]
        start, end = (int(frame) for frame in armature.animation_data.action.frame_range)
        for step in range(5):
            frame = start + round((end - start) * step / 4)
            scene.frame_set(frame)
            place_preview_camera(camera, 60, distance=7.5, height=1.6, target_height=1.2)
            scene.render.filepath = os.path.join(folder, "{}_{:03d}.png".format(action_name, frame))
            bpy.ops.render.render(write_still=True)


# ========== Main ==========


def main():
    arguments = parse_arguments()
    out_folder = os.path.abspath(arguments.out)
    boss_folder = os.path.join(out_folder, "Boss")
    axe_folder = os.path.join(out_folder, "BossAxe")
    for folder in (boss_folder, axe_folder):
        os.makedirs(folder, exist_ok=True)

    clear_scene()

    our_names = {theirs: ours for ours, theirs in motion_retarget.MESHY.bone_of.items()}
    armature, body = character_from_mesh.load_rigged_character(BODY_SOURCE, BODY_HEIGHT, our_names)
    body.name = "Boss"
    character_from_mesh.make_engine_textures(body, boss_folder, "boss", BODY_BRIGHTNESS)

    skeleton = character_from_mesh.skeleton_of(armature)
    character_from_mesh.add_held_item_bones(armature, held_axe(skeleton))
    knight_generator.create_animations(armature, BOSS_ANIMATIONS)
    knight_generator.reset_to_rest_pose(armature)

    axe = load_axe(axe_folder)

    export_gltf(os.path.join(boss_folder, "boss.gltf"), [armature, body], with_animations=True)
    export_gltf(os.path.join(axe_folder, "boss_axe.gltf"), [axe], with_animations=False)

    if arguments.preview:
        knight_generator.hang_on_bone(axe, armature, knight_generator.WEAPON_BONE, axe_rest_matrix(character_from_mesh.fist_center(skeleton, "_R")))
        render_previews(os.path.abspath(arguments.preview), armature, body, axe)

    print("BOSS_GENERATOR_DONE")


if __name__ == "__main__":
    main()
