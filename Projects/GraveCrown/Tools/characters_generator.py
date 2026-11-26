"""
Generates the characters of Grave Crown from their rigged Meshy models (Tools/Blender/Generated/GraveCrown), with their
weapons and Mixamo animations retargeted onto them - the same machinery as Scrap Souls' enemies (enemies_generator.py)

Run from the repository root:
    blender --background --factory-startup --python Projects/GraveCrown/Tools/characters_generator.py -- --out Projects/GraveCrown/res/Models/Characters
    (--only grave_king, --preview <folder>, --hands <folder> for close-ups of the fists)
    then: python Projects/GraveCrown/Tools/glow_maps.py   (the glowing parts of the textures)

Where they come from: the A-pose concepts in Tools/Blender/Concepts/ThirdGame/Variants (picked by the user), Meshy Image
to 3D and Rigging (all in Tools/Blender/Generated/meshy_log.json). Mixamo animation packs are downloaded by hand into
Tools/Blender/Mixamo (not in the repository: their license allows only the characters with the motion baked in)
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
BLENDER = os.path.normpath(os.path.join(HERE, "..", "..", "..", "Tools", "Blender"))
sys.path.insert(0, BLENDER)

import enemies_generator  # noqa: E402
from enemies_generator import Enemy, Weapon, TWO_HANDED, ONE_HANDED  # noqa: E402
import motion_retarget  # noqa: E402
from motion_retarget import MotionClip  # noqa: E402

# The models of this game live in their own folder
enemies_generator.ENEMIES_FOLDER = os.path.join(BLENDER, "Generated", "GraveCrown")

GREAT_SWORD = "Mixamo/GreatSword/great sword "

# The dead king: a great sword in both hands (Mixamo's Great Sword Pack), the dodges, the bonfire and the flask of the
# Scrap Souls knight. A block is the great sword held across (no shield: the posture takes the blows), the parry a short
# beat of it
KING = dict(TWO_HANDED)
KING.update({
    "Idle":           MotionClip(GREAT_SWORD + "idle.fbx"),
    "Roll":           MotionClip("Mixamo/Sprinting Forward Roll.fbx", 4, 36, speed=1.1),
    "Backstep":       MotionClip("Mixamo/Standing Dodge Backward.fbx", 4, 40, speed=1.2),
    "Attack_Running": MotionClip(GREAT_SWORD + "jump attack.fbx", speed=1.1),
    "Attack_Heavy_2": MotionClip(GREAT_SWORD + "high spin attack.fbx"),
    "Block":          MotionClip(GREAT_SWORD + "blocking.fbx"),
    "Block_Impact":   MotionClip(GREAT_SWORD + "impact (2).fbx"),
    "Parry":          MotionClip(GREAT_SWORD + "blocking (2).fbx", speed=1.4),
    "Hit":            MotionClip(GREAT_SWORD + "impact.fbx"),
    "Parried":        MotionClip(GREAT_SWORD + "impact (3).fbx", speed=0.6),
    "Rest":           MotionClip("Mixamo/Kneel Campfire.fbx", 50, 110, speed=1.2),
    "Stand_Up":       MotionClip("Mixamo/Kneel Campfire.fbx", 188, 238, speed=1.3),
    "Drink":          MotionClip("Mixamo/Drinking.fbx", 60, 180, speed=1.6),
    "Taunt":          MotionClip(GREAT_SWORD + "power up.fbx"),
    # In the saddle (Mesh2Motion's riding clips, CC0; baked by bake_mount_clips.py): the sword in the right fist, the
    # left hand on the reins
    "Ride_Idle":      MotionClip("Mesh2Motion/human/Mount_Idle.glb", rig=motion_retarget.QUATERNIUS, action="Mount_Idle"),
    "Ride_Walk":      MotionClip("Mesh2Motion/human/Mount_Walk.glb", rig=motion_retarget.QUATERNIUS, action="Mount_Walk"),
    "Ride_Gallop":    MotionClip("Mesh2Motion/human/Mount_Gallop.glb", rig=motion_retarget.QUATERNIUS, action="Mount_Gallop"),
})
RIDING = ("Ride_Idle", "Ride_Walk", "Ride_Gallop")

CHARACTERS = [
    Enemy("player_rigged.glb", "GraveKing", "grave_king", 2.05, KING,
          Weapon("weapon_greatsword_upright.glb", "grave_greatsword", 1.75, 0.1),
          is_left_fist=True, is_two_handed=True, brightness=1.6, one_hand_actions=RIDING),
    # The hollow knight of the gods: a long sword of the same forge as the king's (the great sword's model, shorter)
    Enemy("enemy_knight_rigged.glb", "HollowKnight", "hollow_knight", 1.95, ONE_HANDED,
          Weapon("weapon_greatsword_upright.glb", "hollow_knight_sword", 1.15, 0.1), brightness=1.5),
    # The masked cultist of the gods: a staff upright in the left hand (Scrap Souls' ash cultist staff), spells from the right
    Enemy("enemy_cultist_b_rigged.glb", "GodsCultist", "gods_cultist", 1.85, enemies_generator.CULTIST,
          Weapon("../Enemies/cultist_staff.glb", "gods_cultist_staff", 1.85, 0.42), is_staff_in_left=True, brightness=1.7),
    # A knight of the dead king's court, violet flame in the helm: fights at his side when called
    Enemy("court_knight_rigged.glb", "CourtKnight", "court_knight", 1.95, ONE_HANDED,
          Weapon("weapon_greatsword_upright.glb", "court_knight_sword", 1.2, 0.1), brightness=1.5),
]


def main():
    arguments = enemies_generator.parse_arguments()
    for character in CHARACTERS:
        if not arguments.only or arguments.only == character.name:
            enemies_generator.make_enemy(character, arguments)
    print("GRAVE_CROWN_CHARACTERS_DONE")


if __name__ == "__main__":
    main()
