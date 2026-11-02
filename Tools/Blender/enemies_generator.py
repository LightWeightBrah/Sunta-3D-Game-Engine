"""
Generates the enemies of the example game (MyGame) from their generated, rigged models: the hollow ghoul, the ash cultist,
the executioner and the skeleton spearman, each with its weapons as separate models

Run (from the repository root, the Mixamo files of knight_generator.py must be downloaded):
    blender --background --factory-startup --python Tools/Blender/enemies_generator.py -- --out Projects/MyGame/res/Models/Enemies

Writes <out>/<Folder>/<name>.gltf for every enemy and weapon (+ .bin, _diffuse.png, _specular.png). They are the game's
models, not the engine's: the engine loads every model in the game's folder by its file name, so a scene uses them
with "model_name": "<name>". Prints the hitbox of every weapon (for its entity in the scene)
Optional: --preview <folder> renders pictures, --only <name> makes one enemy

Where they come from (see Generated/meshy_log.json): the concepts Concepts/Enemies/enemy_<name>.png (Meshy Text to Image,
approved by the user), the same character with empty hands and every weapon alone (Meshy Image to Image), textured models
of those (Meshy Image to 3D) and the bodies rigged by Meshy: Generated/Enemies/<name>_rigged.glb and <weapon>.glb.
The hands are closed into fists around the grips (character_from_mesh.make_fist), the weapons hang on bones in the fists
"""

import argparse
import os
import sys
from dataclasses import dataclass, field

import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import boss_generator  # noqa: E402
import character_from_mesh  # noqa: E402
import hand_pinning  # noqa: E402
import held_item_check  # noqa: E402
import strike_windows  # noqa: E402
import knight_generator  # noqa: E402
import motion_retarget  # noqa: E402
from hand_pinning import HandPin  # noqa: E402
from motion_retarget import MotionClip  # noqa: E402
from sunta_blender import (clear_scene, export_gltf, place_preview_camera, render_hand_closeups,  # noqa: E402
                           setup_preview_render, show_only)

BLENDER_FOLDER = os.path.dirname(os.path.abspath(__file__))
ENEMIES_FOLDER = os.path.join(BLENDER_FOLDER, "Generated", "Enemies")

# Mixamo's sword-and-shield moves (the knights') for one-handed weapons, the Great Sword Pack's (the boss's) for two hands
ONE_HANDED = knight_generator.ANIMATIONS
TWO_HANDED = {name: clip for name, clip in boss_generator.BOSS_ANIMATIONS.items()
              if name not in ("Attack_Leap", "Phase_Two", "Intro") and not name.startswith("Fury_")}

# The executioner: a butcher sure of himself, the cleaver in one hand, the other free (Mixamo's Pro Melee Axe Pack: one
# axe, so one hand). Chops down, across and back up, a heavy combo of two blows, and thumping his chest with the cleaver
# (a taunt a script can play). Quaternius' Universal Animation Library (CC0, from Mesh2Motion, see below) stays for
# other enemies
AXE = "Mixamo/ProMeleeAxe/"
# Download (not in the repository, 13 MB): https://app.mesh2motion.org/animations/human-base-animations.glb and
# .../human-addon-animations.glb into Tools/Blender/Mesh2Motion/
QUATERNIUS_BASE = "Mesh2Motion/human-base-animations.glb"
QUATERNIUS_ADDON = "Mesh2Motion/human-addon-animations.glb"
EXECUTIONER = {
    "Idle":           MotionClip(AXE + "standing idle.fbx"),
    "Walk":           MotionClip(AXE + "standing walk forward.fbx"),
    "Run":            MotionClip(AXE + "standing run forward.fbx"),
    "Walk_Back":      MotionClip(AXE + "standing walk back.fbx"),
    "Strafe_Left":    MotionClip(AXE + "standing walk left.fbx"),
    "Strafe_Right":   MotionClip(AXE + "standing walk right.fbx"),
    "Attack_Light_1": MotionClip(AXE + "standing melee attack downward.fbx"),
    "Attack_Light_2": MotionClip(AXE + "standing melee attack horizontal.fbx"),
    "Attack_Light_3": MotionClip(AXE + "standing melee attack backhand.fbx"),
    # Three blows: a cut from the side, back across, and a great chop down from over the head ("ver. 1" threw the
    # whole body forward and the free arm back: it looked odd)
    "Attack_Heavy":   MotionClip(AXE + "standing melee combo attack ver. 2.fbx"),
    "Taunt":          MotionClip(AXE + "standing taunt chest thump.fbx"),
    "Hit":            MotionClip(AXE + "standing react large gut.fbx"),
    "Parried":        MotionClip(AXE + "standing react large from left.fbx", speed=0.7),
    "Death":          TWO_HANDED["Death"],
}


# The ash cultist: a mage (Mixamo's Pro Magic Pack) with a staff upright in its left hand. Its blows push the staff
# forward with both hands and slam it into the ground (a burst of fire there: the shockwave of its heavy attack);
# Cast_Spell throws a spell with the right hand
MAGIC = "Mixamo/ProMagic/"
CULTIST = {
    "Idle":           MotionClip(MAGIC + "standing idle.fbx"),
    "Walk":           MotionClip(MAGIC + "Standing Walk Forward.fbx"),
    "Run":            MotionClip(MAGIC + "Standing Run Forward.fbx"),
    "Walk_Back":      MotionClip(MAGIC + "Standing Walk Back.fbx"),
    "Strafe_Left":    MotionClip(MAGIC + "Standing Walk Left.fbx"),
    "Strafe_Right":   MotionClip(MAGIC + "Standing Walk Right.fbx"),
    "Attack_Light_1": MotionClip(MAGIC + "Standing 2H Magic Attack 01.fbx"),
    "Attack_Light_2": MotionClip(MAGIC + "Standing 2H Magic Attack 02.fbx"),
    "Attack_Light_3": MotionClip(MAGIC + "Standing 2H Magic Attack 01.fbx", speed=1.2),
    "Attack_Heavy":   MotionClip(MAGIC + "Standing 2H Magic Area Attack 01.fbx"),
    "Cast_Spell":     MotionClip(MAGIC + "Standing 1H Magic Attack 01.fbx"),
    "Hit":            MotionClip(MAGIC + "Standing React Small From Front.fbx"),
    "Parried":        MotionClip(MAGIC + "Standing React Large From Front.fbx", speed=0.8),
    "Death":          MotionClip(MAGIC + "Standing React Death Backward.fbx"),
}

# Where the cultist holds its staff at rest (meters for a 1.9 m cultist, the character's space): at the left side,
# the forearm forward. In its blows the fist goes this part of the way to where the clip has it
STAFF_HAND_POINT = (-0.27, 0.33, 1.13)
STAFF_FOLLOWS_BLOWS = 0.65
# The slam into the ground: the clip's hands go down to the knees, the staff only plants itself before the cultist
STAFF_FOLLOWS_SLAM = 0.3
STAFF_BONE = "Weapon_L"


@dataclass
class Weapon:
    """A weapon model held in a fist. Meshy made it standing upright, its edge (or face) towards -X"""
    source: str               # in Generated/Enemies
    name: str                 # the files and the model in the engine
    length: float             # meters, end to end
    grip: float               # where the fist holds it: this part of the length from the handle's end
    is_handle_on_top: bool = False
    is_shield: bool = False   # held on the back of the fist, its face outwards (instead of through the fist)


@dataclass
class Enemy:
    source: str               # in Generated/Enemies
    folder: str
    name: str
    height: float             # meters, to the top of the head
    animations: dict
    right_hand: Weapon
    left_hand: Weapon = None
    is_left_fist: bool = False  # two-handed weapons: the left hand closes around the haft too
    is_two_handed: bool = False  # the left hand stays on the haft in every animation (where the Idle holds it)
    is_staff_in_left: bool = False  # the weapon is a staff held upright in the left fist (the right hand casts spells)
    brightness: float = 1.3
    weapons: list = field(default_factory=list)


ENEMIES = [
    Enemy("hollow_ghoul_rigged.glb", "HollowGhoul", "hollow_ghoul", 1.8, ONE_HANDED,
          Weapon("ghoul_cleaver.glb", "ghoul_cleaver", 0.55, 0.18)),
    Enemy("ash_cultist_rigged.glb", "AshCultist", "ash_cultist", 1.9, CULTIST,
          Weapon("cultist_staff.glb", "cultist_staff", 1.9, 0.42), is_staff_in_left=True, brightness=1.7),
    Enemy("executioner_rigged.glb", "Executioner", "executioner", 2.1, EXECUTIONER,
          Weapon("executioner_cleaver.glb", "executioner_cleaver", 1.6, 0.1, is_handle_on_top=True)),
    Enemy("skeleton_spearman_rigged.glb", "SkeletonSpearman", "skeleton_spearman", 1.8, ONE_HANDED,
          Weapon("skeleton_spear.glb", "skeleton_spear", 2.2, 0.4),
          left_hand=Weapon("skeleton_shield.glb", "skeleton_shield", 0.62, 0.5, is_shield=True)),
]

SHIELD_GAP = 0.008

# How far the hand holding a shield may bend from its forearm in any animation (degrees)
SHIELD_WRIST_MOST_BEND = 8.0

# How far an arm bone may turn between two frames (degrees): faster than that is a jump, not a swing
MOST_ARM_TURN_PER_FRAME = 75.0


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--preview", default="")
    parser.add_argument("--only", default="")
    parser.add_argument("--preview-actions", default="Idle,Walk,Attack_Light_1,Attack_Heavy")
    parser.add_argument("--preview-steps", type=int, default=3)
    parser.add_argument("--hands", default="", help="a folder for close-ups of the hands in the preview actions")
    return parser.parse_args(argv)


# ========== Weapons ==========


def load_weapon(weapon, folder, brightness):
    """The weapon with its grip at the origin: along +Z (the handle below), its edge towards -X.
    A shield: its middle at the origin, its face towards -Y and its top along +Z"""
    objects_before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(ENEMIES_FOLDER, weapon.source))
    imported = [obj for obj in bpy.data.objects if obj not in objects_before]
    model = next(obj for obj in imported if obj.type == "MESH")
    world_matrix = model.matrix_world.copy()
    model.parent = None
    for obj in imported:
        if obj is not model:
            bpy.data.objects.remove(obj)
    model.data.transform(world_matrix)
    model.matrix_world = Matrix.Identity(4)

    if weapon.is_handle_on_top:
        model.data.transform(Matrix.Rotation(3.14159265, 4, "X"))

    points = [vertex.co for vertex in model.data.vertices]
    lowest, highest = min(p.z for p in points), max(p.z for p in points)
    middle_x = (min(p.x for p in points) + max(p.x for p in points)) * 0.5
    middle_y = (min(p.y for p in points) + max(p.y for p in points)) * 0.5
    scale = weapon.length / (highest - lowest)
    grip = weapon.length * weapon.grip
    model.data.transform(Matrix.Translation((0.0, 0.0, -grip)) @ Matrix.Scale(scale, 4)
                         @ Matrix.Translation((-middle_x, -middle_y, -lowest)))

    model.name = weapon.name
    os.makedirs(folder, exist_ok=True)
    character_from_mesh.make_engine_textures(model, folder, weapon.name, brightness)
    export_gltf(os.path.join(folder, weapon.name + ".gltf"), [model], with_animations=False)

    # The hitbox in the engine (glTF axes: +Z here is +Y there): from the fist to the tip
    points = [vertex.co for vertex in model.data.vertices]
    reach = max(p.z for p in points)
    half_x = (max(p.x for p in points) - min(p.x for p in points)) * 0.5
    half_y = (max(p.y for p in points) - min(p.y for p in points)) * 0.5
    print("WEAPON_HITBOX {} local_offset (0, {:.3f}, 0) half_extents ({:.3f}, {:.3f}, {:.3f})".format(
        weapon.name, reach * 0.5, half_x, reach * 0.5, half_y))
    return model


def weapon_in_fist(weapon, grip):
    """Where the weapon stands in the rest pose: through the fist towards the thumb, its edge where the fingers pointed"""
    return grip.item_matrix(-grip.fingers, grip.fingers.cross(grip.along), grip.along)


def shield_on_fist(armature, body, grip, weapon, thickness):
    """Its back just outside the back of the fist, its face outwards, lying along the forearm (turned around the grip
    until the forearm stays behind it, see character_from_mesh.shield_along_forearm)"""
    # The fist is behind the shield (towards +Y of the shield, whose face looks at -Y)
    middle = Vector((0.0, grip.back_of_fist + SHIELD_GAP + thickness * 0.5, 0.0))

    def make_matrix(face):
        top = face.cross(grip.along).normalized()
        return grip.item_matrix(grip.along, -face, top, middle)

    return character_from_mesh.shield_along_forearm(armature, body, grip, "_L", make_matrix, Vector((0.0, -1.0, 0.0)),
                                                    weapon.length * 0.5, thickness * 0.5)


# ========== Enemies ==========


def make_enemy(enemy, arguments):
    clear_scene()
    out_folder = os.path.join(os.path.abspath(arguments.out), enemy.folder)
    os.makedirs(out_folder, exist_ok=True)

    our_names = {theirs: ours for ours, theirs in motion_retarget.MESHY.bone_of.items()}
    armature, body = character_from_mesh.load_rigged_character(os.path.join(ENEMIES_FOLDER, enemy.source), enemy.height, our_names)
    body.name = enemy.name
    character_from_mesh.make_engine_textures(body, out_folder, enemy.name, enemy.brightness)

    if enemy.is_staff_in_left:
        # The right hand stays open (it casts), the staff goes through the left fist
        weapon_bone = STAFF_BONE
        staff_fist = character_from_mesh.make_fist(armature, body, "_L")
        held = [(STAFF_BONE, "Hand_L", weapon_in_fist(enemy.right_hand, staff_fist))]
    else:
        weapon_bone = knight_generator.WEAPON_BONE
        right = character_from_mesh.make_fist(armature, body, "_R")
        held = [(knight_generator.WEAPON_BONE, knight_generator.WEAPON_PARENT_BONE, weapon_in_fist(enemy.right_hand, right))]
    weapons = [(enemy.right_hand, held[0])]

    left = None
    if enemy.left_hand or enemy.is_left_fist:
        left = character_from_mesh.make_fist(armature, body, "_L")
        if enemy.left_hand:
            shield = load_weapon(enemy.left_hand, out_folder, enemy.brightness)
            thickness = max(v.co.y for v in shield.data.vertices) - min(v.co.y for v in shield.data.vertices)
            held.append((knight_generator.SHIELD_BONE, knight_generator.SHIELD_PARENT_BONE, shield_on_fist(armature, body, left, enemy.left_hand, thickness)))
            weapons.append((enemy.left_hand, held[1]))
            bpy.data.objects.remove(shield)

    character_from_mesh.add_held_item_bones(armature, held)
    # The fists turn as the clips' hands do (the weapons lie in them as the motion capture held its props)
    grips = {"_R": None if enemy.is_staff_in_left else right, "_L": staff_fist if enemy.is_staff_in_left else left}
    actions = knight_generator.create_animations(armature, enemy.animations, character_from_mesh.hand_frames(armature, grips))
    if enemy.is_two_handed:
        # The left fist one fist from the right one on the haft (on the side the Idle holds it, see hold_with_both_hands)
        spacing = hand_pinning.FIST_SPACING * enemy.height / 1.8
        made_by_text = [name for name, clip in enemy.animations.items() if clip.rig is motion_retarget.SMPL]
        hand_pinning.straight_hands(armature, [action for action in actions if action.name in made_by_text])
        hand_pinning.hold_with_both_hands(armature, knight_generator.WEAPON_BONE, actions, left, right.along,
                                          right.center, spacing, aimed_actions=made_by_text)
    if enemy.is_staff_in_left:
        scale = enemy.height / 1.9
        point = tuple(value * scale for value in STAFF_HAND_POINT)
        free_turn = hand_pinning.staff_free_turn(armature, staff_fist, "_L")
        for action in actions:
            follow = STAFF_FOLLOWS_BLOWS if action.name.startswith(("Attack", "Cast")) else 0.0
            if action.name == "Attack_Heavy":
                follow = STAFF_FOLLOWS_SLAM
            weight = hand_pinning.fade(0.0, 0.35) if action.name == "Death" else (lambda progress: 1.0)
            hand_pinning.pin_hands(armature, action, [HandPin("_L", hand_pinning.staff_upright(staff_fist, "_L", point, follow),
                                                              weight, free_turn=free_turn)])
    shield = None
    if enemy.left_hand:
        # The shield lies along the forearm and never cuts through the body (see characters_generator, the knight)
        shield = (knight_generator.SHIELD_BONE, held[1][2], enemy.left_hand.length * 0.5, thickness * 0.5, Vector((0.0, -1.0, 0.0)))
        hand_pinning.straighten_wrist(armature, actions, "_L", SHIELD_WRIST_MOST_BEND)
        held_item_check.push_shield_out_of_body(armature, body, actions, shield)
    # Last: the twists of the hands spread up the arms (after every pin, which set the hands), and no arm bone jumps
    # further than a fast swing turns it in one frame
    hand_pinning.spread_twist(armature, actions)
    hand_pinning.smooth_jerks(armature, actions, MOST_ARM_TURN_PER_FRAME)
    held_item_check.check_actions(armature, body, actions, shield=shield)
    held_item_check.check_arms(armature, actions)
    weapon = enemy.right_hand
    reach = weapon.length * (1.0 - weapon.grip)
    strike_windows.print_strike_windows(armature, weapon_bone, held[0][2], reach, actions)
    strike_windows.print_stride_speeds(armature, actions)
    knight_generator.reset_to_rest_pose(armature)
    export_gltf(os.path.join(out_folder, enemy.name + ".gltf"), [armature, body], with_animations=True)

    weapon_object = load_weapon(enemy.right_hand, out_folder, enemy.brightness)
    if arguments.preview:
        shown = [body, weapon_object]
        knight_generator.hang_on_bone(weapon_object, armature, weapon_bone, held[0][2])
        if enemy.left_hand:
            shield = load_weapon(enemy.left_hand, out_folder, enemy.brightness)
            knight_generator.hang_on_bone(shield, armature, knight_generator.SHIELD_BONE, held[1][2])
            shown.append(shield)
        render_previews(os.path.abspath(arguments.preview), armature, shown, enemy, arguments.preview_actions.split(","),
                        arguments.preview_steps)
        if arguments.hands:
            render_hand_closeups(os.path.abspath(arguments.hands), enemy.name + "_", armature,
                                 arguments.preview_actions.split(","), arguments.preview_steps)


def render_previews(folder, armature, shown, enemy, actions, steps):
    os.makedirs(folder, exist_ok=True)
    show_only(shown)
    camera = setup_preview_render(360, 440)
    scene = bpy.context.scene
    size = enemy.height / 1.8
    for action_name in actions:
        armature.animation_data.action = bpy.data.actions[action_name]
        start, end = (int(frame) for frame in armature.animation_data.action.frame_range)
        for step in range(steps):
            scene.frame_set(start + round((end - start) * step / max(steps - 1, 1)))
            place_preview_camera(camera, 35, distance=4.2 * size, height=1.2 * size, target_height=1.0 * size)
            scene.render.filepath = os.path.join(folder, "{}_{}_{}.png".format(enemy.name, action_name, step))
            bpy.ops.render.render(write_still=True)


def main():
    arguments = parse_arguments()
    for enemy in ENEMIES:
        if not arguments.only or arguments.only == enemy.name:
            make_enemy(enemy, arguments)
    print("ENEMIES_GENERATOR_DONE")


if __name__ == "__main__":
    main()
