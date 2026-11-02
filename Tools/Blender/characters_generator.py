"""
Generates the knights of the example game from their generated meshes: the player (Knight) and the enemy (Rusted Knight)

Run (from the repository root, the Mixamo files of knight_generator.py must be downloaded):
    blender --background --factory-startup --python Tools/Blender/characters_generator.py -- --out Sunta/res/Models

Writes <out>/Knight/knight.gltf and <out>/RustedKnight/rusted_knight.gltf (+ .bin, _diffuse.png, _specular.png)
Optional: --icons <folder> renders the editor icons, --preview <folder> renders pictures of some animations

Where the bodies come from: the concepts Concepts/knight_concept.png and Concepts/rusted_knight_concept.png
were made by Meshy (Text to Image, A pose) and turned into textured meshes by Meshy (Image to 3D, A pose),
see Generated/meshy_log.json for every setting. The player is rigged by Meshy too (its bones and weights bend the armor
much better than automatic weights); for the others character_from_mesh.py fits the skeleton. Then come the animations,
and the sword and the shield (knight_generator.py) hang on bones placed at the hands
"""

import argparse
import math
import os
import sys
from dataclasses import dataclass

import bpy
from mathutils import Quaternion, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import character_from_mesh  # noqa: E402
import hand_pinning  # noqa: E402
import held_item_check  # noqa: E402
import strike_windows  # noqa: E402
import knight_generator  # noqa: E402
import motion_retarget  # noqa: E402
from hand_pinning import HandPin, ShieldPose  # noqa: E402
from sunta_blender import (clear_scene, export_gltf, place_preview_camera, render_hand_closeups,  # noqa: E402
                           setup_preview_render, show_only)

BLENDER_FOLDER = os.path.dirname(os.path.abspath(__file__))

# A little brighter than the generated base color: the engine has no reflections, so dark steel would look flat
KNIGHT_BRIGHTNESS = 1.5

# A round shield is held by a grip behind its boss: its middle is at the left fist, its face this far outside the fist
# (parts of the character's height), so the hand stays behind the face
SHIELD_OUT_OF_FIST = 0.04

# Rigged characters close their hands into fists: between the back of the left fist and the back of the shield (meters)
SHIELD_GAP = 0.008

# How far the shield may be turned around its grip to keep the forearm behind it (degrees, both ways)
SHIELD_MOST_TILT = 45

# How far the hand holding the shield may bend from its forearm in any animation (degrees)
SHIELD_WRIST_MOST_BEND = 8.0

# How far an arm bone may turn between two frames (degrees): faster than that is a jump, not a swing
MOST_ARM_TURN_PER_FRAME = 75.0


@dataclass
class Character:
    source: str        # in Generated
    folder: str        # in the output folder
    name: str          # the files and the model in the engine
    height: float      # meters, to the top of the head
    icon: str          # the editor icon's file name
    brightness: float = KNIGHT_BRIGHTNESS
    is_rigged: bool = False  # rigged by Meshy (its bones and weights are kept), otherwise the skeleton is fitted here


CHARACTERS = [
    Character("knight_meshy_rigged.glb", "Knight", "knight", 1.85, "souls_player_icon.png", is_rigged=True),
    Character("rusted_knight_meshy.glb", "RustedKnight", "rusted_knight", 1.85, "rusted_knight_icon.png"),
]


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--icons", default="")
    parser.add_argument("--preview", default="")
    parser.add_argument("--preview-actions", default="Idle,Walk,Roll,Attack_Light_1,Attack_Heavy,Block")
    parser.add_argument("--preview-steps", type=int, default=3)
    parser.add_argument("--hands", default="", help="a folder for close-ups of the hands in the preview actions")
    parser.add_argument("--only", default="", help="the name of one character to make")
    return parser.parse_args(argv)


def sword_and_shield(skeleton):
    """The sword in the right fist, the shield held by the left fist, for a skeleton (fitted or rigged)"""
    height = max(head[2] for name, parent, head, tail, roll in skeleton if name == "Head") / character_from_mesh.HEAD_HEIGHT
    shield_center = character_from_mesh.fist_center(skeleton, "_L") + Vector((-SHIELD_OUT_OF_FIST * height, 0.0, 0.0))

    return [(knight_generator.WEAPON_BONE, knight_generator.WEAPON_PARENT_BONE,
             knight_generator.get_sword_rest_matrix(character_from_mesh.fist_center(skeleton, "_R"))),
            (knight_generator.SHIELD_BONE, knight_generator.SHIELD_PARENT_BONE,
             knight_generator.get_shield_rest_matrix(shield_center))]


def sword_and_shield_in_fists(armature, mesh):
    """For a rigged character: both hands closed into fists, the sword's grip in the right one (the blade towards the thumb,
    the edge where the straight fingers pointed) and the shield's grip in the left one (its face on the back of the hand).
    Returns the held items and the two fists"""
    right = character_from_mesh.make_fist(armature, mesh, "_R")
    left = character_from_mesh.make_fist(armature, mesh, "_L")

    sword = right.item_matrix(right.fingers, right.along.cross(right.fingers), right.along)
    # The shield lies along the forearm (so the forearm stays behind it, not through it), its face on the side
    # of the back of the fist, the grip through the fist's hole
    forearm = (armature.data.bones["Hand_L"].head_local - armature.data.bones["LowerArm_L"].head_local).normalized()
    face = left.along.cross(forearm).normalized()
    if face.dot(-left.palm) < 0.0:
        face = -face
    # The grip goes through the fist's hole (knight_generator.build_shield_mesh puts it on posts, far enough behind
    # the planks for the back of the fist)
    grip_in_shield = knight_generator.SHIELD_THICKNESS * 0.5 + knight_generator.SHIELD_GRIP_BEHIND_FACE
    if left.back_of_fist + SHIELD_GAP > knight_generator.SHIELD_GRIP_BEHIND_FACE:
        print("WARNING: the back of the left fist reaches {:.3f} m, into the shield's planks".format(left.back_of_fist))
    shield_middle = Vector((0.0, 0.0, -grip_in_shield))

    # Turned around the grip until the forearm (its armor is thicker than the gap behind the planks) goes away
    # from the shield towards the elbow instead of through it
    def shield_turned(degrees):
        turned_face = Quaternion(left.along, math.radians(degrees)) @ face
        return left.item_matrix(left.along, turned_face.cross(left.along), turned_face, shield_middle)

    def depth(matrix):
        return character_from_mesh.depth_through_disc(mesh, ("Hand_L", "LowerArm_L"), matrix.translation,
                                                      matrix.to_3x3().col[2].normalized(), knight_generator.SHIELD_RADIUS,
                                                      knight_generator.SHIELD_THICKNESS * 0.5)

    tilt = min(range(-SHIELD_MOST_TILT, SHIELD_MOST_TILT + 1, 3), key=lambda degrees: (round(max(depth(shield_turned(degrees)), 0.0), 3), abs(degrees)))
    shield = shield_turned(tilt)
    print("Shield turned {} degrees around the grip, the arm goes {:.3f} m into it".format(tilt, max(depth(shield), 0.0)))

    return [(knight_generator.WEAPON_BONE, knight_generator.WEAPON_PARENT_BONE, sword),
            (knight_generator.SHIELD_BONE, knight_generator.SHIELD_PARENT_BONE, shield)], {"_R": right, "_L": left}


# ========== The shield held up ==========

# Where the shield is while blocking (meters for a 1.85 m knight, the character's space: X right, Y forward, Z up):
# before the chest a little to the left, its top at the chin, its face forward and a little to the left and up.
# Mixamo's block holds it sideways at the shoulder, which covers nothing from the front
BLOCK_SHIELD_CENTER = (-0.08, 0.40, 1.27)
BLOCK_SHIELD_FACE = (-0.15, 1.0, 0.08)

# A blow on the shield pushes it back and tips its top towards the knight, then it comes forward again
BLOCK_IMPACT_POSES = [
    ShieldPose(0.0, BLOCK_SHIELD_CENTER, BLOCK_SHIELD_FACE),
    ShieldPose(0.15, (-0.06, 0.29, 1.31), (-0.1, 1.0, 0.35)),
    ShieldPose(0.55, BLOCK_SHIELD_CENTER, BLOCK_SHIELD_FACE),
    ShieldPose(1.0, BLOCK_SHIELD_CENTER, BLOCK_SHIELD_FACE),
]

# The parry: a quick flick of the shield from across the front outwards to the left, turning the blow aside,
# then back down to the guard. Attacks that land between 0.1 and 0.35 of it are parried (parry_from/to in the scene)
PARRY_POSES = [
    ShieldPose(0.0, (-0.30, 0.15, 1.05), (-1.0, 0.3, 0.0)),
    ShieldPose(0.12, (-0.02, 0.42, 1.24), (0.45, 1.0, 0.1)),
    ShieldPose(0.32, (-0.42, 0.36, 1.22), (-0.9, 1.0, 0.15)),
    ShieldPose(0.55, (-0.38, 0.26, 1.12), (-0.8, 1.0, 0.0)),
]


# The upper body twists to the left with the flick (degrees for each of the spine and the chest) and the hips drop
# a little (meters for a 1.85 m knight): the whole knight parries, not only its arm
PARRY_TWIST_DEGREES = [(0.0, 0.0), (0.12, -4.0), (0.32, 9.0), (0.55, 5.0), (0.85, 0.0), (1.0, 0.0)]
PARRY_HIPS_DROP = [(0.0, 0.0), (0.25, 0.04), (0.6, 0.03), (1.0, 0.0)]


def pin_the_shield(armature, held, height):
    """Block, Block_Impact and Parry move the shield where it really guards (hand_pinning)"""
    scale = height / 1.85
    shield_rest = held[1][2]
    # The shield is built facing +Z (knight_generator.build_shield_mesh), its middle at the bone
    face_in_bone = hand_pinning.item_face_in_bone(armature, knight_generator.SHIELD_BONE, shield_rest.col[2].xyz.normalized())

    def scaled(poses):
        return [ShieldPose(pose.progress, tuple(value * scale for value in pose.center), pose.face) for pose in poses]

    center = tuple(value * scale for value in BLOCK_SHIELD_CENTER)
    actions = bpy.data.actions
    hand_pinning.pin_hands(armature, actions["Block"], [
        HandPin("_L", hand_pinning.shield_in_front(knight_generator.SHIELD_BONE, face_in_bone, center, BLOCK_SHIELD_FACE))])
    hand_pinning.pin_hands(armature, actions["Block_Impact"], [
        HandPin("_L", hand_pinning.shield_path(knight_generator.SHIELD_BONE, face_in_bone, scaled(BLOCK_IMPACT_POSES)))])
    # The body turns into the flick (to the left, with the shield), the knees give a little; the arms follow the body,
    # the shield is pinned afterwards so it still goes its way
    parry = actions["Parry"]
    hand_pinning.add_turn(armature, parry, "Spine", (0.0, 0.0, 1.0), hand_pinning.curve(PARRY_TWIST_DEGREES))
    hand_pinning.add_turn(armature, parry, "Chest", (0.0, 0.0, 1.0), hand_pinning.curve(PARRY_TWIST_DEGREES))
    hand_pinning.add_hips_drop(armature, parry, hand_pinning.curve([(p, d * scale) for p, d in PARRY_HIPS_DROP]))
    hand_pinning.pin_hands(armature, actions["Parry"], [
        HandPin("_L", hand_pinning.shield_path(knight_generator.SHIELD_BONE, face_in_bone, scaled(PARRY_POSES)),
                hand_pinning.fade(0.12, 0.55))])


def render_icon(path, armature, mesh, height):
    """The editor icon: the helmet and the shoulders on a transparent background"""
    show_only([mesh])
    camera = setup_preview_render(knight_generator.ICON_PIXELS, knight_generator.ICON_PIXELS, is_transparent=True)
    place_preview_camera(camera, knight_generator.ICON_CAMERA_ANGLE, distance=1.0 * height / 1.82,
                         height=0.89 * height, target_height=0.86 * height)
    bpy.context.scene.render.filepath = os.path.abspath(path)
    bpy.ops.render.render(write_still=True)


# The sword and the shield the previews show (made by knight_generator.py)
HELD_MODELS = os.path.join(BLENDER_FOLDER, "..", "..", "Sunta", "res", "Models")


def load_held_model(file_name):
    objects_before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(HELD_MODELS, file_name))
    return next(obj for obj in bpy.data.objects if obj not in objects_before and obj.type == "MESH")


def render_previews(folder, armature, mesh, name, held, actions, steps):
    """Pictures of the knight with its sword and shield, from the front-left and the front-right"""
    os.makedirs(folder, exist_ok=True)
    shown = [mesh]
    if held:
        for (bone_name, _, rest_matrix), file_name in zip(held, ("KnightSword/knight_sword.gltf", "KnightShield/knight_shield.gltf")):
            item = load_held_model(file_name)
            item.parent = None
            knight_generator.reset_to_rest_pose(armature)
            knight_generator.hang_on_bone(item, armature, bone_name, rest_matrix)
            shown.append(item)
    show_only(shown)
    camera = setup_preview_render(300, 380)
    scene = bpy.context.scene

    for action_name in actions:
        armature.animation_data.action = bpy.data.actions[action_name]
        start, end = (int(frame) for frame in armature.animation_data.action.frame_range)
        for step in range(steps):
            scene.frame_set(start + round((end - start) * step / max(steps - 1, 1)))
            for angle in (35, -40):
                place_preview_camera(camera, angle, distance=3.6, height=1.1, target_height=0.95)
                scene.render.filepath = os.path.join(folder, "{}_{}_{}_{}.png".format(name, action_name, step, angle))
                bpy.ops.render.render(write_still=True)
    return shown


def make_character(character, arguments):
    clear_scene()
    folder = os.path.join(os.path.abspath(arguments.out), character.folder)
    os.makedirs(folder, exist_ok=True)

    source = os.path.join(BLENDER_FOLDER, "Generated", character.source)
    if character.is_rigged:
        our_names = {theirs: ours for ours, theirs in motion_retarget.MESHY.bone_of.items()}
        armature, mesh = character_from_mesh.load_rigged_character(source, character.height, our_names)
    else:
        mesh = character_from_mesh.load_mesh(source, character.height)
    mesh.name = character.name
    character_from_mesh.make_engine_textures(mesh, folder, character.name, character.brightness)

    held = None
    if character.is_rigged:
        held, grips = sword_and_shield_in_fists(armature, mesh)
        character_from_mesh.add_held_item_bones(armature, held)
        actions = knight_generator.create_animations(armature, hand_frames=character_from_mesh.hand_frames(armature, grips))
        pin_the_shield(armature, held, character.height)
        # The shield lies along the forearm: a bent wrist would push the forearm through it
        # Drinking keeps its hand at the mouth; the block, a blow on it and the parry hold the shield where it guards
        # (pin_the_shield): a straightened wrist would turn it sideways again
        guarded = [action for action in actions if action.name not in ("Drink", "Block", "Block_Impact", "Parry")]
        actions_by_name = {action.name: action for action in actions}
        hand_pinning.straighten_wrist(armature, guarded, "_L", SHIELD_WRIST_MOST_BEND)
        shield = (knight_generator.SHIELD_BONE, held[1][2], knight_generator.SHIELD_RADIUS, knight_generator.SHIELD_THICKNESS * 0.5,
                  Vector((0.0, 0.0, 1.0)))
        held_item_check.push_shield_out_of_body(armature, mesh, guarded, shield)
        held_item_check.turn_shield_off_the_body(armature, mesh, [actions_by_name["Drink"]], shield)
        # The twists of the hands spread up the arms, no arm bone jumps further than a fast swing turns it in a frame
        hand_pinning.spread_twist(armature, actions)
        hand_pinning.smooth_jerks(armature, actions, MOST_ARM_TURN_PER_FRAME)
        held_item_check.check_actions(armature, mesh, actions, shield=shield)
        held_item_check.check_arms(armature, actions)
        strike_windows.print_stride_speeds(armature, actions)
        knight_generator.reset_to_rest_pose(armature)
    else:
        armature, skeleton = character_from_mesh.build_character(mesh, sword_and_shield)
        # The shield held up and the parry move the shield where it guards, as for the rigged knight
        pin_the_shield(armature, sword_and_shield(skeleton), character.height)
        knight_generator.reset_to_rest_pose(armature)
    export_gltf(os.path.join(folder, character.name + ".gltf"), [armature, mesh], with_animations=True)

    if arguments.icons:
        render_icon(os.path.join(os.path.abspath(arguments.icons), character.icon), armature, mesh, character.height)
    if arguments.preview:
        render_previews(os.path.abspath(arguments.preview), armature, mesh, character.name, held,
                        arguments.preview_actions.split(","), arguments.preview_steps)
        if arguments.hands:
            render_hand_closeups(os.path.abspath(arguments.hands), character.name + "_", armature,
                                 arguments.preview_actions.split(","), arguments.preview_steps)


def main():
    arguments = parse_arguments()
    for character in CHARACTERS:
        if not arguments.only or arguments.only == character.name:
            make_character(character, arguments)
    print("CHARACTERS_GENERATOR_DONE")


if __name__ == "__main__":
    main()
