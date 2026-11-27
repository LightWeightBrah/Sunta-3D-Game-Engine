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

# The dead king: Mixamo's Auto-Rigger rigged him (2026-11-27; Meshy's rig twisted his shoulders and wrists in every
# blow), and every clip is Mixamo's own for that skeleton - the Great Sword Pack's moves, a roll and a dodge, the bonfire
# and the flask - exported for it (Tools/Blender/Mixamo/GraveKing/, not in the repository). A block is the great sword
# held across (no shield: the posture takes the blows), the parry a short beat of it


def king(name, first_frame=None, last_frame=None, **settings):
    """A clip Mixamo exported for the king's own skeleton"""
    return MotionClip("Mixamo/GraveKing/{}.fbx".format(name), first_frame, last_frame, rig=motion_retarget.MIXAMO_NATIVE, **settings)


KING = {
    "Idle":           king("GS_Idle"),
    "Walk":           king("GS_Walk"),
    "Run":            king("GS_Run"),
    "Walk_Back":      king("GS_Walk_Back"),
    "Strafe_Left":    king("GS_Strafe_Left"),
    "Strafe_Right":   king("GS_Strafe_Right"),
    "Attack_Light_1": king("GS_Downward_Slash", speed=0.95),
    "Attack_Light_2": king("GS_Low_Slash", speed=0.95),
    "Attack_Light_3": king("GS_Combo_Slash", speed=0.9),
    "Attack_Heavy":   king("GS_Power_Slash", speed=0.85),
    "Attack_Heavy_2": king("GS_High_Spin"),
    "Attack_Running": king("GS_Jump_Attack", speed=1.1),
    "Riposte":        king("GS_Power_Slash", speed=0.9),
    "Roll":           king("Roll", 4, 36, speed=1.1),
    "Backstep":       king("Dodge_Back", speed=1.2),
    "Block":          king("GS_Block_Idle"),
    "Block_Impact":   king("GS_Blocked_Impact"),
    "Parry":          king("GS_To_Block", speed=1.4),
    "Hit":            king("GS_Unblocked_Impact"),
    "Parried":        king("GS_Head_Impact", speed=0.6),
    "Death":          king("Death_Forward"),
    "Rest":           king("Campfire", 50, 110, speed=1.2),
    "Stand_Up":       king("Campfire", 188, 238, speed=1.3),
    "Drink":          king("Drinking", 60, 180, speed=1.6),
    "Taunt":          king("GS_Power_Up"),
    # In the saddle (Mesh2Motion's riding clips, CC0; baked by bake_mount_clips.py): the sword in the right fist, the
    # left hand on the reins
    "Ride_Idle":      MotionClip("Mesh2Motion/human/Mount_Idle.glb", rig=motion_retarget.QUATERNIUS, action="Mount_Idle"),
    "Ride_Walk":      MotionClip("Mesh2Motion/human/Mount_Walk.glb", rig=motion_retarget.QUATERNIUS, action="Mount_Walk"),
    "Ride_Gallop":    MotionClip("Mesh2Motion/human/Mount_Gallop.glb", rig=motion_retarget.QUATERNIUS, action="Mount_Gallop"),
}
RIDING = ("Ride_Idle", "Ride_Walk", "Ride_Gallop")

# The Chain Warden, the gods' hand at the edge of the plains: the horned black knight (the rider's model, on foot), half a
# head taller than the king, with the boss's great sword cuts; he guards (Sekiro) and answers with a great spinning blow
WARDEN = dict(TWO_HANDED)
WARDEN.update({
    "Block":          MotionClip(GREAT_SWORD + "blocking.fbx"),
    "Block_Impact":   MotionClip(GREAT_SWORD + "impact (2).fbx"),
    "Attack_Heavy_2": MotionClip(GREAT_SWORD + "high spin attack.fbx", speed=0.9),
    "Taunt":          MotionClip(GREAT_SWORD + "power up.fbx"),
    # (the pack's "slash (3)" bends the right wrist 92 degrees, as it did the king's)
    "Attack_Light_2": MotionClip(GREAT_SWORD + "slash (5).fbx", speed=0.9),
})

# The obsidian golem (an optional giant on the plains): Mixamo's mutant moves - a heavy breathing idle, a lumbering walk,
# punches, a swipe, a jump that crashes down (his perilous blow), a roar, a fall
MUTANT = "Mixamo/Brawl/Mutant "
GOLEM = {
    "Idle":           MotionClip(MUTANT + "Breathing Idle.fbx"),
    "Walk":           MotionClip(MUTANT + "Walking.fbx"),
    "Run":            MotionClip(MUTANT + "Run.fbx"),
    "Walk_Back":      MotionClip(MUTANT + "Walking.fbx"),
    "Strafe_Left":    MotionClip(MUTANT + "Walking.fbx"),
    "Strafe_Right":   MotionClip(MUTANT + "Walking.fbx"),
    "Attack_Light_1": MotionClip(MUTANT + "Punch.fbx"),
    "Attack_Light_2": MotionClip(MUTANT + "Swiping.fbx"),
    "Attack_Heavy":   MotionClip(MUTANT + "Jump Attack.fbx"),
    "Taunt":          MotionClip(MUTANT + "Roaring.fbx"),
    "Hit":            MotionClip(GREAT_SWORD + "impact.fbx"),
    "Parried":        MotionClip(GREAT_SWORD + "impact (3).fbx", speed=0.6),
    "Death":          MotionClip(MUTANT + "Dying.fbx"),
}

# The enemies' own moves (2026-11-27; before they had the Scrap Souls knight's and cultist's): Mixamo clips exported on
# the king's rig (Tools/Blender/Mixamo/GraveEnemies/), retargeted like his
def grave_enemy(name, first_frame=None, last_frame=None, **settings):
    return MotionClip("Mixamo/GraveEnemies/{}.fbx".format(name), first_frame, last_frame, rig=motion_retarget.MIXAMO_NATIVE, **settings)


# The hollow knight: a dead man still walking in his armor - the zombie's shamble, idle sway and stumbles (Mixamo's
# Scary Zombie Pack), his blows wild swipes of the sword and a one-handed combo; the knight's block and parry stay
HOLLOW = dict(ONE_HANDED)
HOLLOW.update({
    "Idle":           grave_enemy("HK_Idle"),
    # (the zombie's own walk reached forward with both arms: a limp keeps the sword down)
    "Walk":           grave_enemy("HK_Limp"),
    "Walk_Back":      grave_enemy("HK_Limp_Back"),
    "Run":            grave_enemy("HK_Limp_Run"),
    "Attack_Light_1": grave_enemy("HK_Swipe", speed=1.1),
    "Attack_Light_2": grave_enemy("HK_Right_Hand", speed=1.1),
    "Attack_Light_3": grave_enemy("HK_Swipe", speed=1.25),
    "Attack_Heavy":   grave_enemy("HK_Combo"),
    "Hit":            grave_enemy("HK_Hit"),
    "Parried":        grave_enemy("HK_Stumble", speed=0.8),
    "Death":          grave_enemy("HK_Death"),
    "Taunt":          grave_enemy("HK_Scream"),
})

# The gods' cultist: a sweep of the hand and an upward sweep for its blows, the slam into the ground, a spell thrown
# forward
GODS_CULTIST = dict(enemies_generator.CULTIST)
GODS_CULTIST.update({
    "Idle":           grave_enemy("GC_Idle"),
    "Walk":           grave_enemy("GC_Walk"),
    "Run":            grave_enemy("GC_Run"),
    "Walk_Back":      grave_enemy("GC_Walk_Back"),
    "Strafe_Left":    grave_enemy("GC_Walk_Left"),
    "Strafe_Right":   grave_enemy("GC_Walk_Right"),
    "Attack_Light_1": grave_enemy("GC_Sweep"),
    "Attack_Light_2": grave_enemy("GC_Upward"),
    "Attack_Light_3": grave_enemy("GC_Sweep", speed=1.2),
    "Attack_Heavy":   grave_enemy("GC_Ground"),
    "Cast_Spell":     grave_enemy("GC_Forwards"),
    "Hit":            grave_enemy("GC_Hit"),
    "Parried":        grave_enemy("GC_Hit_Large", speed=0.8),
    "Death":          grave_enemy("GC_Death"),
})

CHARACTERS = [
    Enemy("player.glb", "GraveKing", "grave_king", 2.05, KING,
          Weapon("weapon_greatsword_upright.glb", "grave_greatsword", 1.75, 0.1),
          is_left_fist=True, is_two_handed=True, brightness=0.9, one_hand_actions=RIDING, cape=True, skirt=True, cloth_bones=True,
          mixamo_skin="Mixamo/GraveKing/King_Skin_Idle.fbx"),

    # The hollow knight of the gods: a long sword of the same forge as the king's (the great sword's model, shorter)
    Enemy("enemy_knight_rigged.glb", "HollowKnight", "hollow_knight", 1.95, HOLLOW,
          Weapon("weapon_greatsword_upright.glb", "hollow_knight_sword", 1.15, 0.1), brightness=1.5),
    # The masked cultist of the gods: a staff upright in the left hand (Scrap Souls' ash cultist staff), spells from the right
    Enemy("enemy_cultist_b_rigged.glb", "GodsCultist", "gods_cultist", 1.85, GODS_CULTIST,
          Weapon("../Enemies/cultist_staff.glb", "gods_cultist_staff", 1.85, 0.42), is_staff_in_left=True, brightness=1.7),
    # The court in the crypt (they stand by the throne; the lich reads the books found in the world aloud): the magic
    # pack's calm idle; the lich keeps a staff in its left hand, the butler's staff is never shown (its hands rest)
    Enemy("court_lich_rigged.glb", "CourtLich", "court_lich", 1.95, enemies_generator.CULTIST,
          Weapon("../Enemies/cultist_staff.glb", "court_lich_staff", 1.9, 0.42), is_staff_in_left=True, brightness=1.6),
    Enemy("court_butler_rigged.glb", "CourtButler", "court_butler", 1.85, enemies_generator.CULTIST,
          Weapon("../Enemies/cultist_staff.glb", "court_butler_cane", 1.0, 0.5), is_staff_in_left=True, brightness=1.5),
    Enemy("enemy_rider_rigged.glb", "ChainWarden", "chain_warden", 2.55, WARDEN,
          Weapon("weapon_greatsword_upright.glb", "chain_warden_blade", 2.0, 0.1), is_left_fist=True, is_two_handed=True),
    # (no weapon: a sliver in the fist stands for it - the scene hangs only a hitbox on the hand)
    Enemy("enemy_golem_rigged.glb", "ObsidianGolem", "obsidian_golem", 3.3, GOLEM,
          Weapon("weapon_greatsword_upright.glb", "golem_fist", 0.2, 0.5), brightness=1.0),
    # A knight of the dead king's court, violet flame in the helm: fights at his side when called
    Enemy("court_knight_rigged.glb", "CourtKnight", "court_knight", 1.95, ONE_HANDED,
          Weapon("weapon_greatsword_upright.glb", "court_knight_sword", 1.2, 0.1), brightness=1.5),
]


# Every Meshy rig of this game: its head end lies a centimeter from the head (the head tipped forward in every clip),
# and some spines start just above the hips - straightened before the animations go on
for _character in CHARACTERS:
    _character.straight_spine = True
    # (brighter textures went pink and pale under the game's violet light and painted grade)
    _character.brightness = min(_character.brightness, 1.2)


def main():
    arguments = enemies_generator.parse_arguments()
    for character in CHARACTERS:
        if not arguments.only or arguments.only == character.name:
            enemies_generator.make_enemy(character, arguments)
    print("GRAVE_CROWN_CHARACTERS_DONE")


if __name__ == "__main__":
    main()
