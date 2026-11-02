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
import os
import sys

import bpy
from mathutils import Matrix

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import character_from_mesh  # noqa: E402
import hand_pinning  # noqa: E402
import held_item_check  # noqa: E402
import strike_windows  # noqa: E402
import knight_generator  # noqa: E402
import motion_retarget  # noqa: E402
from motion_retarget import MotionClip  # noqa: E402
from sunta_blender import (clear_scene, export_gltf, place_preview_camera, render_hand_closeups,  # noqa: E402
                           setup_preview_render, show_only)

BLENDER_FOLDER = os.path.dirname(os.path.abspath(__file__))
# Rigged by Meshy (its own bones and weights), from the first concept of the boss
BODY_SOURCE = os.path.join(BLENDER_FOLDER, "Generated", "boss_meshy_rigged.glb")
AXE_SOURCE = os.path.join(BLENDER_FOLDER, "Generated", "boss_great_axe_meshy.glb")

# Meters, before the engine scales the boss up
BODY_HEIGHT = 2.0
AXE_LENGTH = 2.0
BODY_BRIGHTNESS = 1.3

# Where the right fist holds the haft: this part of the axe's length from the pommel (the left hand holds it higher up).
# Both hands are closed into fists around the haft (character_from_mesh.make_fist), the haft goes through the right one
AXE_GRIP_FROM_BOTTOM = 0.14

# ========== The animations ==========

# Mixamo's Great Sword Pack (a two-handed weapon)
GREAT_SWORD = "Mixamo/GreatSword/great sword "

# The jump at the player, made by Meshy's Text to Motion from a description (see Generated/meshy_log.json)
LEAP_SLAM = "Generated/Motions/leap_slam.fbx"


BOSS_ANIMATIONS = {
    "Idle":           MotionClip(GREAT_SWORD + "idle.fbx"),
    "Walk":           MotionClip(GREAT_SWORD + "walk.fbx"),
    # Runs forward carrying the axe at the shoulder ("run.fbx" of the pack runs backwards: the body went one way, the legs
    # the other). The scene fades walking and running into each other slowly, so the axe moves up instead of jumping
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
    # Without its last frames: getting up after the landing flipped a hand around (Text to Motion)
    "Attack_Leap":    MotionClip(LEAP_SLAM, 1, 108, rig=motion_retarget.SMPL),

    # Phase two starts with this (BossComponent.phaseTwoAnimation): it raises the axe and bursts into embers
    "Phase_Two":      MotionClip(GREAT_SWORD + "power up.fbx"),

    # The boss shows itself before the fight (a Cutscene plays it): it raises the axe high over its head, then kneels and
    # drives the blade into the ground (motion capture; Text to Motion's intro turned the wrists at random)
    "Intro":          MotionClip(GREAT_SWORD + "casting.fbx"),

    # Phase two fights differently (BossComponent swaps these in): a wide low sweep sliding on one knee, a leap with
    # a spinning cleave, and three great cuts ending in a fire shockwave. Motion capture (Mixamo's Great Sword Pack):
    # the moves Meshy's Text to Motion made turned the wrists at random, the hands flipped half around in single frames
    "Fury_Sweep":     MotionClip(GREAT_SWORD + "slide attack.fbx"),
    "Fury_Combo":     MotionClip(GREAT_SWORD + "jump attack.fbx", speed=1.1),
    "Fury_Slam":      MotionClip(GREAT_SWORD + "slash (2).fbx", speed=0.95),

    "Hit":            MotionClip(GREAT_SWORD + "impact.fbx"),
    "Parried":        MotionClip(GREAT_SWORD + "impact (3).fbx", speed=0.6),
    "Death":          MotionClip("Mixamo/GreatSword/two handed sword death.fbx"),
}


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--preview", default="")
    parser.add_argument("--hands", default="", help="a folder for close-ups of the hands holding the axe")
    parser.add_argument("--hands-actions", default="Idle,Attack_Heavy,Attack_Leap,Fury_Sweep,Fury_Combo,Fury_Slam")
    return parser.parse_args(argv)


# ========== The glow of phase two ==========

# Cracks with embers in the armor and the red-hot head of the axe (the look of Concepts/boss_phase_two_concept.png):
# baked into boss_emissive.png and boss_axe_emissive.png, they shine when the Boss sets the glow in phase two
EMBER_COLOR = (1.0, 0.36, 0.05)
RED_HOT_COLOR = (1.0, 0.2, 0.02)
CRACK_SCALE = 7.0            # cracks per meter (a 3D pattern in the boss's own coordinates)
CRACK_WIDTH = 0.035
# The darkest gaps between armor plates glow too: fully below this brightness (linear), not at all above the second
# (about the darkest 5% of the boss's texture)
GLOWING_GAP_FULL = 0.012
GLOWING_GAP_NONE = 0.02
AXE_HEAD_FROM = 1.1          # meters above the fist where the axe head starts to glow


def add_node(tree, kind, **settings):
    node = tree.nodes.new(kind)
    for name, value in settings.items():
        if hasattr(node, name):
            setattr(node, name, value)
        else:
            node.inputs[name].default_value = value
    return node


def map_range(tree, socket, from_min, from_max, to_min, to_max):
    node = add_node(tree, "ShaderNodeMapRange", clamp=True)
    node.inputs["From Min"].default_value = from_min
    node.inputs["From Max"].default_value = from_max
    node.inputs["To Min"].default_value = to_min
    node.inputs["To Max"].default_value = to_max
    tree.links.new(socket, node.inputs["Value"])
    return node.outputs["Result"]


def combine(tree, first, second, operation="MULTIPLY"):
    node = add_node(tree, "ShaderNodeMath", operation=operation, use_clamp=True)
    tree.links.new(first, node.inputs[0])
    tree.links.new(second, node.inputs[1])
    return node.outputs["Value"]


def tinted(tree, strength, color):
    node = add_node(tree, "ShaderNodeVectorMath", operation="SCALE")
    node.inputs[0].default_value = color
    tree.links.new(strength, node.inputs["Scale"])
    return node.outputs["Vector"]


def ember_cracks(tree, diffuse_image):
    """Thin cracks (edges of 3D Voronoi cells) in some parts of the armor, and the darkest gaps between its plates"""
    coordinates = tree.nodes.new("ShaderNodeTexCoord").outputs["Object"]

    cells = add_node(tree, "ShaderNodeTexVoronoi", feature="DISTANCE_TO_EDGE", Scale=CRACK_SCALE)
    tree.links.new(coordinates, cells.inputs["Vector"])
    cracks = map_range(tree, cells.outputs["Distance"], 0.0, CRACK_WIDTH, 1.0, 0.0)

    patches = add_node(tree, "ShaderNodeTexNoise", Scale=2.5, Detail=2.0)
    tree.links.new(coordinates, patches.inputs["Vector"])
    cracks = combine(tree, cracks, map_range(tree, patches.outputs["Fac"], 0.45, 0.6, 0.0, 1.0))

    image = tree.nodes.new("ShaderNodeTexImage")
    image.image = diffuse_image
    brightness = tree.nodes.new("ShaderNodeRGBToBW")
    tree.links.new(image.outputs["Color"], brightness.inputs["Color"])
    gaps = map_range(tree, brightness.outputs["Val"], GLOWING_GAP_FULL, GLOWING_GAP_NONE, 0.5, 0.0)

    return tinted(tree, combine(tree, cracks, gaps, operation="ADD"), EMBER_COLOR)


def red_hot_head(tree, diffuse_image):
    """The axe head glows (the haft stays cold), unevenly like heated metal"""
    coordinates = tree.nodes.new("ShaderNodeTexCoord").outputs["Object"]
    along = tree.nodes.new("ShaderNodeSeparateXYZ")
    tree.links.new(coordinates, along.inputs["Vector"])
    head = map_range(tree, along.outputs["Z"], AXE_HEAD_FROM, AXE_HEAD_FROM + 0.3, 0.0, 1.0)

    heat = add_node(tree, "ShaderNodeTexNoise", Scale=6.0, Detail=3.0)
    tree.links.new(coordinates, heat.inputs["Vector"])
    unevenness = map_range(tree, heat.outputs["Fac"], 0.3, 0.7, 0.5, 1.0)

    return tinted(tree, combine(tree, head, unevenness), RED_HOT_COLOR)


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
    character_from_mesh.bake_emission(axe, folder, "boss_axe_emissive", red_hot_head, size=1024)
    print("AXE_HITBOX local_offset (0, {:.3f}, 0) half_extents (0.12, {:.3f}, 0.12) for the whole length above the fist".format(
        (AXE_LENGTH - grip) * 0.5, (AXE_LENGTH - grip) * 0.5))
    return axe


def axe_in_fists(armature, body):
    """Both hands closed into fists; the axe (built with the haft along +Z, the blade towards -X) goes through the right
    one towards the thumb, its blade where the straight fingers pointed. Returns the axe's rest matrix and both fists"""
    right = character_from_mesh.make_fist(armature, body, "_R")
    left = character_from_mesh.make_fist(armature, body, "_L")
    return right.item_matrix(-right.fingers, right.fingers.cross(right.along), right.along), right, left


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
    character_from_mesh.bake_emission(body, boss_folder, "boss_emissive", ember_cracks, size=2048)

    axe_matrix, right, left = axe_in_fists(armature, body)
    character_from_mesh.add_held_item_bones(armature, [(knight_generator.WEAPON_BONE, knight_generator.WEAPON_PARENT_BONE, axe_matrix)])
    actions = knight_generator.create_animations(armature, BOSS_ANIMATIONS,
                                                 character_from_mesh.hand_frames(armature, {"_R": right, "_L": left}))
    # Both fists stay on the haft, also in the motions made by Text to Motion (they swing it with one hand)
    spacing = hand_pinning.FIST_SPACING * BODY_HEIGHT / 1.8
    made_by_text = [name for name, clip in BOSS_ANIMATIONS.items() if clip.rig is motion_retarget.SMPL]
    hand_pinning.straight_hands(armature, [action for action in actions if action.name in made_by_text])
    hand_pinning.smooth_jerks(armature, [action for action in actions if action.name in made_by_text])
    # The leap keeps the axe as its straight right wrist holds it: aimed at the left hand it turned half around mid-air
    made_by_text = [name for name in made_by_text if name != "Attack_Leap"]
    hand_pinning.hold_with_both_hands(armature, knight_generator.WEAPON_BONE, actions, left, right.along,
                                      right.center, spacing, aimed_actions=made_by_text)
    hand_pinning.spread_twist(armature, actions)
    held_item_check.check_actions(armature, body, actions)
    held_item_check.check_arms(armature, actions)
    strike_windows.print_strike_windows(armature, knight_generator.WEAPON_BONE, axe_matrix,
                                        AXE_LENGTH * (1.0 - AXE_GRIP_FROM_BOTTOM), actions)
    strike_windows.print_stride_speeds(armature, actions)
    knight_generator.reset_to_rest_pose(armature)

    axe = load_axe(axe_folder)

    export_gltf(os.path.join(boss_folder, "boss.gltf"), [armature, body], with_animations=True)
    export_gltf(os.path.join(axe_folder, "boss_axe.gltf"), [axe], with_animations=False)

    if arguments.preview:
        knight_generator.hang_on_bone(axe, armature, knight_generator.WEAPON_BONE, axe_matrix)
        render_previews(os.path.abspath(arguments.preview), armature, body, axe)
        if arguments.hands:
            render_hand_closeups(os.path.abspath(arguments.hands), "boss_", armature, arguments.hands_actions.split(","),
                                 steps=4, distance=1.1)

    print("BOSS_GENERATOR_DONE")


if __name__ == "__main__":
    main()
