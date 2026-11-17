"""
Generates the three characters of 'Strongest in History Vs Today' from their generated, rigged models: they fight with
bare hands and spells, so there are no weapons and no fists to close around grips (compare Tools/Blender/enemies_generator.py)

Run (from the repository root; the Mixamo packs and the Quaternius libraries of Tools/Blender must be downloaded):
    blender --background --factory-startup --python Projects/StrongestInHistoryVsToday/Tools/characters_generator.py
    (optional: -- --only sih_boss --preview <folder>)

Writes res/Models/Characters/<Folder>/<name>.gltf (+ .bin, _diffuse.png, _specular.png) and prints the striking part of
every attack for each limb that can hurt (STRIKE lines: the hit windows of the scene)

Where they come from (see Tools/Blender/Generated/meshy_log.json): the user's A-pose pictures in
Tools/Blender/Concepts/StrongestInHistory, textured models of those (Meshy Image to 3D) and Meshy's rigging:
Tools/Blender/Generated/StrongestInHistory/<name>_rigged.glb. The moves: Mixamo's packs (walking, magic, reactions)
and Quaternius' Universal Animation Library (CC0: punches, hooks, the fighting stance)
"""

import argparse
import os
import sys
from dataclasses import dataclass

import bpy
from mathutils import Matrix, Vector

PROJECT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLENDER_FOLDER = os.path.join(os.path.dirname(os.path.dirname(PROJECT)), "Tools", "Blender")
sys.path.insert(0, BLENDER_FOLDER)

import character_from_mesh  # noqa: E402
import finger_rig  # noqa: E402
import hand_pinning  # noqa: E402
import held_item_check  # noqa: E402
import knight_generator  # noqa: E402
import motion_retarget  # noqa: E402
import strike_windows  # noqa: E402
from motion_retarget import MotionClip, QUATERNIUS  # noqa: E402
from sunta_blender import aim_camera_at, clear_scene, export_gltf, place_preview_camera, setup_preview_render, show_only  # noqa: E402

SOURCE_FOLDER = os.path.join(BLENDER_FOLDER, "Generated", "StrongestInHistory")
CONCEPTS = os.path.join(BLENDER_FOLDER, "Concepts", "StrongestInHistory")
OUT_FOLDER = os.path.join(PROJECT, "res", "Models", "Characters")

MAGIC = "Mixamo/ProMagic/"
AXE = "Mixamo/ProMeleeAxe/"
SWORD = "Mixamo/ProSwordShield/sword and shield "
GREAT_SWORD = "Mixamo/GreatSword/great sword "
# Motion capture of boxing and of a mutant, downloaded one by one from mixamo.com into Tools/Blender/Mixamo/Brawl
BRAWL = "Mixamo/Brawl/"
BASE = "Mesh2Motion/human-base-animations.glb"
ADDON = "Mesh2Motion/human-addon-animations.glb"


def library(file_name, action, **settings):
    """A clip of Quaternius' library (one file holds many)"""
    return MotionClip(file_name, rig=QUATERNIUS, action=action, **settings)


# Walking with empty hands (the axe pack has a set without the axe)
UNARMED_WALKING = {
    "Walk":         MotionClip(AXE + "unarmed walk forward.fbx"),
    "Run":          MotionClip(AXE + "unarmed run forward.fbx"),
    "Walk_Back":    MotionClip(AXE + "unarmed walk back.fbx"),
    "Strafe_Left":  MotionClip(AXE + "standing walk left.fbx"),
    "Strafe_Right": MotionClip(AXE + "standing walk right.fbx"),
}

# The punches and kicks of the motion capture take 1.5 to 2 seconds each with their long wind-ups and their walk back
# into the stance: far too slow for a fight played with buttons. Each is cut to the blow itself (the frames around
# its strike, see the STRIKE lines) and played a little faster, so a jab takes half a second and a combo flows
JAB      = dict(first_frame=0,  last_frame=24)
CROSS    = dict(first_frame=14, last_frame=40)
HOOK     = dict(first_frame=15, last_frame=42)
UPPERCUT = dict(first_frame=3,  last_frame=28)
KICK     = dict(first_frame=5,  last_frame=40)
SWIPE    = dict(first_frame=20, last_frame=56)

# The player: a boxer's stance, a jab, a cross and a hook as the combo, a kick as the heavy blow, a guard with the
# forearms, and two spells: an orb thrown with one hand and the great one called with both
PLAYER = dict(UNARMED_WALKING, **{
    "Idle":           MotionClip(BRAWL + "Bouncing Fight Idle.fbx"),
    "Roll":           MotionClip("Mixamo/Sprinting Forward Roll.fbx", 4, 36, speed=1.1),
    "Backstep":       MotionClip("Mixamo/Standing Dodge Backward.fbx", 4, 40, speed=1.2),
    "Attack_Light_1": MotionClip(BRAWL + "Boxing Jab.fbx", speed=1.35, **JAB),
    "Attack_Light_2": MotionClip(BRAWL + "Cross Punch.fbx", speed=1.35, **CROSS),
    "Attack_Light_3": MotionClip(BRAWL + "Hook Punch.fbx", speed=1.3, **HOOK),
    "Attack_Heavy":   MotionClip(BRAWL + "Mma Kick.fbx", speed=1.2, **KICK),
    # A man in a street fight, not the mage's pack (the cultist of the first game moves with it)
    "Hit":            library(BASE, "Hit_Chest", speed=1.2),
    "Parried":        MotionClip(AXE + "standing react large from left.fbx", speed=0.8),
    "Death":          library(ADDON, "Death_A"),
    "Block":          MotionClip(AXE + "standing block idle.fbx"),
    "Block_Impact":   MotionClip(AXE + "standing block react large.fbx", speed=1.2),
    # Slow enough to be a move one can time: about half a second, the first half of it turns blows away
    "Parry":          MotionClip(AXE + "standing block react large.fbx", 0, 18, speed=0.9),
    # Thrown to the ground (a rain of the boss's domain) and getting up again
    "Knocked_Down":   MotionClip(MAGIC + "Standing React Death Backward.fbx", speed=1.1),
    "Get_Up":         library(BASE, "LayToIdle", speed=1.0),
    # Lying on his back where he fell, still (the first frames of getting up, played very slowly)
    "Lying":          library(BASE, "LayToIdle", first_frame=0, last_frame=2, speed=0.03),
    "Rest":           MotionClip("Mixamo/Kneel Campfire.fbx", 50, 110, speed=1.2),
    "Stand_Up":       MotionClip("Mixamo/Kneel Campfire.fbx", 188, 238, speed=1.3),
    "Drink":          MotionClip("Mixamo/Drinking.fbx", 60, 180, speed=1.6),
    "Cast_Orb":       library(BASE, "OverhandThrow", speed=1.3),
    # His two great techniques are signs of the hands held still (posed below, see GESTURES): both hands pressed
    # together with two fingers up, and a triangle of both hands before the chest
    "Cast_Great_Orb": MotionClip(AXE + "unarmed idle.fbx", speed=0.9),
    "Cast_Domain":    MotionClip(AXE + "unarmed idle.fbx", speed=0.9),
})

# The boss: a mage who also brawls. Punches up close, a kick as the heavy blow, spells with one hand, the great
# spells (his domain, the summoning) with both
BOSS = {
    # He walks and stands like a man in a street, not like a wizard (the magic pack's stance had him creeping
    # with his hands spread)
    "Idle":           MotionClip(AXE + "unarmed idle.fbx"),
    "Walk":           MotionClip(AXE + "unarmed walk forward.fbx"),
    "Run":            MotionClip(AXE + "unarmed run forward.fbx"),
    "Walk_Back":      MotionClip(AXE + "unarmed walk back.fbx"),
    "Strafe_Left":    MotionClip(AXE + "standing walk left.fbx"),
    "Strafe_Right":   MotionClip(AXE + "standing walk right.fbx"),
    # The sign of his domain: standing still, the right hand raised beside his face with two fingers up
    # (the arm and the fingers are posed below, see GESTURES)
    "Sign_Domain":    MotionClip(AXE + "unarmed idle.fbx", speed=0.5),
    # The same sign held from the first frame on: played again and again while his temple rises under him
    "Sign_Hold":      MotionClip(AXE + "unarmed idle.fbx", speed=0.5),
    "Attack_Light_1": MotionClip(BRAWL + "Boxing Jab.fbx", speed=1.15, **JAB),
    "Attack_Light_2": MotionClip(BRAWL + "Cross Punch.fbx", speed=1.15, **CROSS),
    "Attack_Light_3": MotionClip(BRAWL + "Uppercut.fbx", speed=1.1, **UPPERCUT),
    "Attack_Heavy":   MotionClip(BRAWL + "Mma Kick.fbx", **KICK),
    # Out of the way of a blow or an orb: a hop back and a quick step through
    "Backstep":       MotionClip("Mixamo/Standing Dodge Backward.fbx", 4, 40, speed=1.3),
    "Roll":           MotionClip("Mixamo/Standing Dodge Forward.fbx", speed=1.3),
    # The forearms up against the player's punches
    "Block":          MotionClip(AXE + "standing block idle.fbx"),
    "Block_Impact":   MotionClip(AXE + "standing block react large.fbx", speed=1.3),
    # What he throws, he throws like a man: from the shoulder (the bundles of money, the pages of his book), a sweep
    # of the arm (a fan of pages); his air strikes and his summoning are the sign of two fingers held up (GESTURES)
    "Cast_Spell":     MotionClip(BRAWL + "Cross Punch.fbx", speed=1.0, **CROSS),
    "Cast_Cleave":    MotionClip(BRAWL + "Hook Punch.fbx", speed=0.95, **HOOK),
    "Cast_Pillars":   MotionClip(AXE + "unarmed idle.fbx", speed=0.9),
    "Cast_Domain":    MotionClip(AXE + "unarmed idle.fbx", speed=0.5),
    "Summon":         MotionClip(AXE + "unarmed idle.fbx", speed=0.6),
    # Hit like a man in a street fight: taken in the gut, thrown back by the blow
    "Hit":            MotionClip(AXE + "standing react large gut.fbx", speed=1.2),
    "Parried":        MotionClip(AXE + "standing react large from left.fbx", speed=0.8),
    "Death":          MotionClip(MAGIC + "Standing React Death Backward.fbx"),
    # Hurled back by the player's last blow before his domain (film 1:15-1:21): falling, skidding on his back,
    # getting up (the script carries him through the air and along the ground)
    "Knocked_Down":   MotionClip(MAGIC + "Standing React Death Backward.fbx", speed=1.1),
    "Lying":          library(BASE, "LayToIdle", first_frame=0, last_frame=2, speed=0.03),
    "Get_Up":         library(BASE, "LayToIdle", speed=1.25),
}

# The summon: a monster three meters tall. Swipes of its claws, slow wide hooks, both fists into the ground as the
# heavy blow (a shockwave in the scene), and a roar when it is called
SUMMON = dict(UNARMED_WALKING, **{
    "Idle":           MotionClip(BRAWL + "Mutant Breathing Idle.fbx"),
    "Walk":           MotionClip(BRAWL + "Mutant Walking.fbx"),
    "Run":            MotionClip(BRAWL + "Mutant Run.fbx"),
    "Attack_Light_1": MotionClip(BRAWL + "Mutant Swiping.fbx", speed=1.1, **SWIPE),
    "Attack_Light_2": MotionClip(BRAWL + "Mutant Punch.fbx"),
    "Attack_Light_3": MotionClip(BRAWL + "Mutant Swiping.fbx", speed=1.3, **SWIPE),
    "Attack_Heavy":   library(ADDON, "Attack_Ground_Pound", speed=0.9),
    # A jump at a player who keeps away (the engine's leap attack)
    "Attack_Leap":    MotionClip(BRAWL + "Mutant Jump Attack.fbx"),
    "Taunt":          MotionClip(BRAWL + "Mutant Roaring.fbx"),
    "Hit":            MotionClip(AXE + "standing react large gut.fbx"),
    "Parried":        MotionClip(AXE + "standing react large from left.fbx", speed=0.7),
    "Death":          MotionClip(BRAWL + "Mutant Dying.fbx"),
})


# ========== Hands ==========

# What the fingers do in each action (the first name that the action's name starts with counts): right hand, left hand.
# Everything not listed keeps relaxed hands. Rigged models come with stiff open hands: a punch needs a fist
FINGER_POSES = (
    ("Attack_Heavy", ("fist", "fist")),      # a kick: the guard stays closed
    ("Attack", ("fist", "fist")),
    ("Idle", ("relaxed", "relaxed")),
    ("Block", ("fist", "fist")),
    ("Parry", ("fist", "fist")),
    ("Sign_Domain", ("two", "relaxed")),
    ("Sign_Hold", ("two", "relaxed")),
    ("Cast_Great_Orb", ("two", "two")),
    ("Cast_Domain", ("open", "open")),
    ("Cast_Orb", ("cupped", "relaxed")),
    ("Cast_Pillars", ("two", "relaxed")),
    ("Summon", ("two", "relaxed")),
    ("Cast_Spell", ("open", "fist")),
    ("Cast_Cleave", ("open", "fist")),
)

# The monster's hands are claws: half closed whatever it does
MONSTER_POSE = {"Index": (30, 40), "Middle": (32, 42), "Ring": (34, 44), "Little": (36, 46), "Thumb": (15, 20)}


def finger_poses_of(action_name):
    for prefix, poses in FINGER_POSES:
        if action_name.startswith(prefix):
            return poses
    return ("relaxed", "relaxed")


@dataclass
class Gesture:
    """A sign of the hands held through an action: where each wrist is (meters from the middle of the chest at rest:
    to the hand's own side, forward, up), where its fingers point and where its palm looks (the same axes; 'side'
    flips the first number for the left hand). The arms reach there from the clip's pose and stay"""
    side: str
    wrist: tuple
    fingers: tuple
    palm: tuple
    reached_at: float = 0.2   # the part of the action by which the hand is in place
    left_from: float = 1.0    # the part after which it goes back to the clip's pose


GESTURES = {
    "sih_boss": {
        # Beside his face, the fingers up, the palm turned half towards his own face
        "Sign_Domain": [Gesture("_R", (0.30, 0.20, 0.05), (0.0, 0.08, 1.0), (-0.6, 0.8, 0.0), reached_at=0.25)],
        # The same sign held high and forward over the battlefield: his jet comes, his domain answers, his summon rises
        "Cast_Pillars": [Gesture("_R", (0.26, 0.38, 0.30), (0.0, 0.2, 1.0), (-0.3, 0.95, 0.0), reached_at=0.3, left_from=0.85)],
        "Cast_Domain":  [Gesture("_R", (0.30, 0.20, 0.05), (0.0, 0.08, 1.0), (-0.6, 0.8, 0.0), reached_at=0.25)],
        "Sign_Hold":    [Gesture("_R", (0.30, 0.20, 0.05), (0.0, 0.08, 1.0), (-0.6, 0.8, 0.0), reached_at=0.0)],
        "Summon":       [Gesture("_R", (0.26, 0.38, 0.30), (0.0, 0.2, 1.0), (-0.3, 0.95, 0.0), reached_at=0.3, left_from=0.9)],
    },
    "sih_player": {
        # Both hands pressed together before the chest, the fingers up and a little forward
        "Cast_Great_Orb": [Gesture("_R", (0.03, 0.30, 0.10), (0.0, 0.35, 1.0), (-1.0, 0.0, 0.0), reached_at=0.3, left_from=0.9),
                           Gesture("_L", (0.03, 0.30, 0.10), (0.0, 0.35, 1.0), (-1.0, 0.0, 0.0), reached_at=0.3, left_from=0.9)],
        # A triangle: the fingertips of both hands meet above, the palms look forward
        "Cast_Domain":    [Gesture("_R", (0.12, 0.33, -0.06), (-0.75, 0.0, 0.7), (0.0, 1.0, 0.1), reached_at=0.3, left_from=0.9),
                           Gesture("_L", (0.12, 0.33, -0.06), (-0.75, 0.0, 0.7), (0.0, 1.0, 0.1), reached_at=0.3, left_from=0.9)],
    },
}


def gesture_pin(armature, rig, gesture, size):
    """A HandPin that holds the hand where the gesture says, following the chest as the clip sways"""
    chest_at_rest = (armature.matrix_world @ armature.data.bones["Chest"].matrix_local).translation.copy()
    # Which way is this hand's own side (the character faces +Y)
    own_side = 1.0 if rig[gesture.side].wrist.x >= 0.0 else -1.0

    def mirrored(vector):
        return Vector((vector[0] * own_side, vector[1], vector[2]))

    def target(armature, progress):
        chest = hand_pinning.world_matrix(armature, "Chest").translation
        wrist = chest_at_rest + mirrored(gesture.wrist) * size + (chest - chest_at_rest)
        return finger_rig.hand_target(armature, rig, gesture.side, wrist, mirrored(gesture.fingers), mirrored(gesture.palm))

    return hand_pinning.HandPin(gesture.side, target, hand_pinning.fade(gesture.reached_at, gesture.left_from))


def pose_hands(character, armature, rig, actions):
    size = character.height / 1.83
    gestures = GESTURES.get(character.name, {})

    for action in actions:
        if action.name in gestures:
            hand_pinning.pin_hands(armature, action, [gesture_pin(armature, rig, gesture, size) for gesture in gestures[action.name]])

        if character.name == "sih_summon":
            poses = {"_R": MONSTER_POSE, "_L": MONSTER_POSE}
        else:
            right, left = finger_poses_of(action.name)
            poses = {"_R": right, "_L": left}
        finger_rig.key_poses(armature, rig, action, {side: pose for side, pose in poses.items() if rig[side].bones})


def print_eyes(character, armature, body):
    """Where the eyes are, in the space of the head bone: the scene puts a Glow Sprite on each (EYE lines)"""
    points = [body.matrix_world @ vertex.co for vertex in body.data.vertices]
    top = max(point.z for point in points)
    size = character.height / 1.83
    eye_height = top - 0.118 * size
    head = armature.matrix_world @ armature.data.bones["Head"].matrix_local

    for name, side in (("R", 1.0), ("L", -1.0)):
        across = side * 0.033 * size * (1.0 if armature.data.bones["Hand_R"].head_local.x >= 0.0 else -1.0)
        near = [point for point in points if abs(point.z - eye_height) < 0.012 * size and abs(point.x - across) < 0.012 * size]
        if not near:
            continue
        # The front of the face there (the character looks along +Y), a little in front of the skin
        eye = Vector((across, max(point.y for point in near) + 0.012 * size, eye_height))
        local = head.inverted() @ eye
        print("EYE {} {} {:.4f} {:.4f} {:.4f}".format(character.name, name, local.x, local.y, local.z))


@dataclass
class Character:
    source: str        # in Tools/Blender/Generated/StrongestInHistory
    folder: str
    name: str          # the files and the model in the engine
    height: float      # meters, to the top of the head
    animations: dict
    brightness: float = 1.15
    # How far past the bone's head a fist or a foot reaches (meters)
    limb_reach: float = 0.12
    # How many times the skin weights are blurred (0 = as generated), see character_from_mesh.smooth_skin_weights
    smooth_weights: int = 6
    # Skin weights from bone heat instead of the generated ones, and how many times those are blurred
    heat_weights: bool = True
    smooth_after_heat: int = 2
    # How many times the surface of the head is evened out (0 = as generated)
    smooth_head: int = 0
    # A photo of the face painted onto the model's texture (character_from_mesh.project_face_photo), and how far the
    # eyes are from where the shape of the head suggests (meters: across, up)
    face_photo: object = None
    eye_offset: tuple = (0.0, 0.0)


CHARACTERS = [
    # From three views of the user's sheet (front, side, back): closer to the man than the first, front-only model
    Character("player_v2_rigged.glb", "Player", "sih_player", 1.83, PLAYER,
              smooth_head=2,
              face_photo=character_from_mesh.FacePhoto(os.path.join(CONCEPTS, "player_reference_user_2.webp"),
                                                       (525, 451), (729, 451), (627, 666))),
        # His generated face already looks like him; the film's still (1:03.5, smiling, tilted, other light) painted over
    # it looked worse - tried and left out:
    #   FacePhoto(os.path.join(CONCEPTS, "boss_face_film.png"), (549, 353), (671, 319), (649, 462))
    Character("boss_rigged.glb", "Boss", "sih_boss", 1.83, BOSS),
    Character("summon_monster_rigged.glb", "Summon", "sih_summon", 3.0, SUMMON),
]

# The limbs that can hurt (a Hitbox on each in the scene)
STRIKING_LIMBS = ("Hand_R", "Hand_L", "Foot_R")

# How far an arm bone may turn between two frames (degrees): faster than that is a jump, not a swing
MOST_ARM_TURN_PER_FRAME = 75.0


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--only", default="")
    parser.add_argument("--preview", default="")
    parser.add_argument("--preview-actions", default="Idle,Walk,Attack_Light_1,Attack_Light_3,Attack_Heavy,Cast_Spell,Cast_Orb")
    parser.add_argument("--no-face", action="store_true", help="leave the generated face (no photo painted on it)")
    parser.add_argument("--no-heat", action="store_true", help="keep the generated skin weights (no bone heat)")
    parser.add_argument("--smooth", type=int, default=-1, help="times the skin weights are blurred (-1: the character's own)")
    parser.add_argument("--hands", action="store_true", help="close-ups of the upper body instead of the whole figure")
    parser.add_argument("--preview-steps", type=int, default=3)
    return parser.parse_args(argv)


def make_character(character, arguments):
    clear_scene()
    out_folder = os.path.join(OUT_FOLDER, character.folder)
    os.makedirs(out_folder, exist_ok=True)

    our_names = {theirs: ours for ours, theirs in motion_retarget.MESHY.bone_of.items()}
    armature, body = character_from_mesh.load_rigged_character(os.path.join(SOURCE_FOLDER, character.source), character.height, our_names)
    body.name = character.name
    character_from_mesh.make_engine_textures(body, out_folder, character.name, character.brightness)

    # One piece instead of patches cut along the texture seams (smoothing would tear the seams open), and an even
    # surface on the head (the generated beard and hair are heaps of shards)
    character_from_mesh.weld_seams(body)
    if character.smooth_head:
        character_from_mesh.smooth_shape_of(body, "Head", character.smooth_head)

    # Meshy's weights in hard patches tear the suit in a pose: blurred before anything else uses them
    # Bone heat spreads a shoulder over the chest the way skin stretches; Meshy's patches crumple it when the arms go up
    heated = character.heat_weights and not arguments.no_heat and character_from_mesh.reskin_with_heat(body, armature)
    smoothing = arguments.smooth if arguments.smooth >= 0 else (character.smooth_after_heat if heated else character.smooth_weights)
    if smoothing:
        character_from_mesh.smooth_skin_weights(body, smoothing)

    # Fingers before the animations: every clip then keys what the hands do
    rig = finger_rig.add_fingers(armature, body)
    print_eyes(character, armature, body)

    # The head with a texture of its own (on the generated atlas the face is many small islands: blocky up close),
    # with the photo of the person's face painted over its front (a generated face is only a guess at it)
    photo = character.face_photo if not arguments.no_face else None
    head = character_from_mesh.separate_head(body, armature, out_folder, character.name, character.height, photo, character.eye_offset)

    actions = knight_generator.create_animations(armature, character.animations)
    pose_hands(character, armature, rig, actions)

    # The twists of the hands spread up the arms, and no arm bone jumps further than a fast punch turns it in one frame
    hand_pinning.spread_twist(armature, actions)
    hand_pinning.smooth_jerks(armature, actions, MOST_ARM_TURN_PER_FRAME)
    held_item_check.check_arms(armature, actions)

    for limb in STRIKING_LIMBS:
        reach = character.limb_reach
        print("LIMB {} {}".format(character.name, limb))
        rest = hand_pinning.rest_world_matrix(armature, limb)
        strike_windows.print_strike_windows(armature, limb, rest, reach, actions)
    strike_windows.print_stride_speeds(armature, actions)

    knight_generator.reset_to_rest_pose(armature)
    export_gltf(os.path.join(out_folder, character.name + ".gltf"), [armature, body, head], with_animations=True)

    if arguments.preview:
        render_previews(os.path.abspath(arguments.preview), armature, [body, head], character,
                        arguments.preview_actions.split(","), arguments.preview_steps, arguments.hands)


def render_previews(folder, armature, meshes, character, actions, steps, arguments_hands=False):
    os.makedirs(folder, exist_ok=True)
    show_only(meshes)
    camera = setup_preview_render(360, 440)
    scene = bpy.context.scene
    size = character.height / 1.8
    for action_name in actions:
        if action_name not in bpy.data.actions:
            continue
        armature.animation_data.action = bpy.data.actions[action_name]
        start, end = (int(frame) for frame in armature.animation_data.action.frame_range)
        for step in range(steps):
            scene.frame_set(start + round((end - start) * step / max(steps - 1, 1)))
            if arguments_hands:
                place_preview_camera(camera, 35, distance=2.2 * size, height=1.5 * size, target_height=1.35 * size)
            else:
                place_preview_camera(camera, 35, distance=4.2 * size, height=1.2 * size, target_height=1.0 * size)
            scene.render.filepath = os.path.join(folder, "{}_{}_{}.png".format(character.name, action_name, step))
            bpy.ops.render.render(write_still=True)

        if arguments_hands:
            # The face, from the front and from a little to the side
            scene.frame_set(start)
            head = armature.pose.bones["Head"]
            face = armature.matrix_world @ (head.head + (head.tail - head.head) * 0.45)
            for angle in (0, 35):
                aim_camera_at(camera, face, angle, 0.42 * size, rise=0.02)
                scene.render.filepath = os.path.join(folder, "{}_{}_face_{}.png".format(character.name, action_name, angle))
                bpy.ops.render.render(write_still=True)

            # The hands themselves at the end of the action's hold, from the front and from each side
            scene.frame_set(start + round((end - start) * 0.6))
            for hand in ("Hand_R", "Hand_L"):
                bone = armature.pose.bones[hand]
                center = armature.matrix_world @ (bone.head + (bone.tail - bone.head) * 1.4)
                for angle in (0, 60, -60):
                    aim_camera_at(camera, center, angle, 0.55 * size)
                    scene.render.filepath = os.path.join(folder, "{}_{}_{}_{}.png".format(character.name, action_name, hand, angle))
                    bpy.ops.render.render(write_still=True)


def main():
    arguments = parse_arguments()
    for character in CHARACTERS:
        if not arguments.only or arguments.only == character.name:
            make_character(character, arguments)
    print("CHARACTERS_GENERATOR_DONE")


if __name__ == "__main__":
    main()
