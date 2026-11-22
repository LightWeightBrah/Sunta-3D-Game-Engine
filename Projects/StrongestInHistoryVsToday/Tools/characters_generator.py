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
import math
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
from motion_retarget import MIXAMO_NATIVE, MotionClip, QUATERNIUS, SOMA  # noqa: E402
from sunta_blender import aim_camera_at, clear_scene, export_gltf, place_preview_camera, setup_preview_render, show_only  # noqa: E402

SOURCE_FOLDER = os.path.join(BLENDER_FOLDER, "Generated", "StrongestInHistory")
CONCEPTS = os.path.join(BLENDER_FOLDER, "Concepts", "StrongestInHistory")
# (SUNTA_CHARACTERS_OUT: somewhere else, to try clips without touching the game's models)
OUT_FOLDER = os.environ.get("SUNTA_CHARACTERS_OUT") or os.path.join(PROJECT, "res", "Models", "Characters")

MAGIC = "Mixamo/ProMagic/"
AXE = "Mixamo/ProMeleeAxe/"
SWORD = "Mixamo/ProSwordShield/sword and shield "
GREAT_SWORD = "Mixamo/GreatSword/great sword "
# Motion capture of boxing and of a mutant, downloaded one by one from mixamo.com into Tools/Blender/Mixamo/Brawl
BRAWL = "Mixamo/Brawl/"
BASE = "Mesh2Motion/human-base-animations.glb"
ADDON = "Mesh2Motion/human-addon-animations.glb"
# Motions made from text by NVIDIA Kimodo (run locally, see SUNTA_CONTEXT tools table), kept in Tools/Blender/Kimodo
KIMODO_KNOCK = "Kimodo/knocked_down_get_up.bvh"
KIMODO_GET_UP = "Kimodo/get_up_raise_hand.bvh"
KIMODO_ORB = "Kimodo/orb_hold_push.bvh"


def diddy(name, first_frame=None, last_frame=None, **settings):
    """A clip Mixamo made for the player's own rig (Tools/Blender/Mixamo/Diddy, downloaded by mixamo_fetch_named.py)"""
    return MotionClip("Mixamo/Diddy/{}.fbx".format(name), first_frame, last_frame, rig=MIXAMO_NATIVE, **settings)


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

# The player: a man in a dinner jacket who can fight. Every clip is Mixamo's motion capture made for his own rig
# (Tools/Blender/Mixamo/Diddy): he stands at ease and walks with a swagger, and boxes only when he attacks. Each blow
# is cut to its wind-up, the strike and the way back to the guard (the clips' first and last frames stand around)
PLAYER = {
    "Idle":           diddy("Idle_Standing"),
    "Walk":           diddy("Walk_Swagger"),
    "Run":            diddy("Run_Intention"),
    # (Mixamo's own walk back for this rig keeps looking over its shoulder: wrong while facing a locked-on enemy)
    "Walk_Back":      MotionClip(AXE + "unarmed walk back.fbx"),
    "Strafe_Left":    diddy("Strafe_Left_Walk"),
    "Strafe_Right":   diddy("Strafe_Right_Walk"),
    # Locked on, he stands in a boxer's stance bouncing on his feet (the punches flow out of it)
    "Idle_Fight":     diddy("Fight_Idle_Bounce"),
    # A quick sprinting roll (the dive roll was slow and long)
    "Roll":           MotionClip("Mixamo/Sprinting Forward Roll.fbx", 4, 36, speed=1.3),
    "Backstep":       diddy("Backstep_Jump", 6, 56, speed=1.3),
    # The combo: the quick jab, cross and hook of the fight before (short cuts of boxing motion capture, played fast),
    # ended by an uppercut
    "Attack_Light_1": MotionClip(BRAWL + "Boxing Jab.fbx", speed=1.35, **JAB),
    "Attack_Light_2": MotionClip(BRAWL + "Cross Punch.fbx", speed=1.35, **CROSS),
    "Attack_Light_3": MotionClip(BRAWL + "Hook Punch.fbx", speed=1.3, **HOOK),
    "Attack_Light_4": diddy("Uppercut_Back", 8, 42, speed=1.3),
    # Heavy: a kick, then (pressed again) a spinning back kick with his whole weight behind it
    "Attack_Heavy":   MotionClip(BRAWL + "Mma Kick.fbx", speed=1.2, **KICK),
    "Attack_Heavy_2": diddy("Kick_SpinBack", 4, 56, speed=1.1),
    # From a run: chasing the enemy down into an uppercut; out of a dodge: a rear hook; at a parried enemy: an elbow,
    # then an uppercut
    "Attack_Running": diddy("Run_Uppercut", 14, 70, speed=1.15),
    "Attack_Riposte": diddy("Combo_ElbowUppercut", 8, 64),
    # Staggers: a straight to the head, one to the stomach (at random), a big one to the head after a heavy blow
    "Hit":            diddy("Hit_HeadMedium"),
    "Hit_2":          diddy("Hit_Stomach"),
    "Hit_Heavy":      diddy("Hit_HeadBig"),
    "Parried":        diddy("Parried_Rocked", 1, 60),
    "Death":          diddy("Knocked_Back"),
    # The guard: both forearms up before the face; a blow on it rocks him back; the parry swats it aside inward
    # Held still (one frame of the guard: a looping take kept restarting); the oil guard of his script shows it
    "Block":          diddy("Block_BodyArms", 41, 42, speed=0.05),
    "Block_Impact":   diddy("Block_HighCenter", 1, 40, speed=1.3),
    # From the guard, the lead forearm sweeps the blow aside (the arm is posed by GESTURES), 0.6 s
    "Parry":          diddy("Block_BodyArms", 41, 42, speed=0.055),
    # Thrown onto his back by the stars of the boss's domain, lying there until the player answers, getting up
    "Knocked_Down":   diddy("Knocked_Back", 20, 75),
    "Lying":          diddy("Knocked_Back", 140, 146, speed=0.05),
    # (quick: in the film he is up and making the sign of his technique in a second and a half)
    "Get_Up":         diddy("GetUp_Back", speed=2.1),
    "Rest":           MotionClip("Mixamo/Kneel Campfire.fbx", 50, 110, speed=1.2),
    "Stand_Up":       MotionClip("Mixamo/Kneel Campfire.fbx", 188, 238, speed=1.3),
    "Drink":          MotionClip("Mixamo/Drinking.fbx", 60, 180, speed=1.6),
    # Solid Baby Oil: the ball over his open palm at his side (GESTURES), then thrown overhand at the enemy
    "Cast_Orb":       diddy("Idle_Standing", 1, 66),
    # His two great techniques are signs of the hands held still (posed below, see GESTURES): both hands pressed
    # together with two fingers up, and a triangle of both hands before the chest
    # (about three seconds: the sign made, held for a breath and the technique gone, as in the film 1:47.5-1:49 - the
    # whole clip held him still for five and a half seconds before it left his hands)
    "Cast_Great_Orb": diddy("Idle_Standing", 1, 90, speed=1.0),
    # (short: the sign is made and held for the close-up of his hands, then the domain opens - release_at 0.9)
    "Cast_Domain":    diddy("Idle_Standing", 1, 66, speed=1.1),
    # The jump (Space): the flight is the engine's physics, so the take's own rise is taken out of the hips (the tuck
    # of the legs stays); from it a flying side kick, or (heavy) both fists raised and driven down into the ground
    "Jump":           diddy("Jump_InPlace", 20, 50, hips_rise_limit=0.02, feet_on_ground=True),
    # (the take kicks along its own forward: turned by 90 degrees the leg went out to his left side)
    "Jump_Attack":    diddy("Jump_FlyingKick", 4, 34, speed=1.15, hips_rise_limit=0.02, feet_on_ground=True),
    "Jump_Plunge":    diddy("Jump_GiantDown", 30, 72, hips_rise_limit=0.02, feet_on_ground=True),
}

# Every downloaded clip whole, to compare them and read their strikes (--candidates)
PLAYER_CANDIDATES = {name: diddy(name) for name in (
    "Diddy_Skin_WeightShiftIdle", "Idle_Neutral", "Idle_Standing", "Idle_Male01", "Walk_Swagger", "Walk_Standard",
    "Run_Intention", "Run_Standard", "Walk_Back", "Strafe_Left_Male", "Strafe_Right_Male", "Strafe_Left_Walk",
    "Strafe_Right_Walk", "Roll_Dive", "Roll_Quick", "Backstep_Boxing", "Backstep_Jump", "Jab_Lead", "Cross_Back",
    "Hook_Lead", "Hook_Back", "Uppercut_Back", "Uppercut_Lead", "Elbow_Cross", "Elbow_Step", "Kick_Roundhouse",
    "Kick_Thrust", "Run_Uppercut", "Combo_ElbowUppercut", "Combo_JabElbow", "Block_Guard", "Block_Reaction",
    "Parry_Outward", "Hit_HeadMedium", "Hit_Stomach", "Hit_HeadBig", "Parried_Rocked", "Knocked_Back", "Cast_Throw", "Throw_Standard", "Block_HighCenter", "Block_BodyArms", "Block_Inward", "Dodge_KneePunch", "Fight_Idle_Bounce",
    "GetUp_Back", "Death_FallBack", "Jump_InPlace", "Jump_Landing", "Jump_Up", "Jump_FlyingKick", "Jump_BicycleKick",
    "Jump_MutantSmash", "Jump_GiantDown")}
# (and the boxing clips of the other rig the combo uses, to compare their stance)
PLAYER_CANDIDATES.update({"Brawl_Jab": MotionClip(BRAWL + "Boxing Jab.fbx"), "Brawl_Cross": MotionClip(BRAWL + "Cross Punch.fbx"),
                          "Brawl_Hook": MotionClip(BRAWL + "Hook Punch.fbx")})

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
    # Up off the ground: the hand rises into the sign (then Sign_Hold keeps it there)
    "Sign_Raise":     MotionClip(AXE + "unarmed idle.fbx", speed=0.5),
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
    # The stars he shoots in the air fight (film 0:42-0:46): two fingers raised before his face, then both hands
    # pointing at the player as the stars fly (GESTURES)
    "Cast_Stars":     MotionClip(AXE + "unarmed idle.fbx", speed=0.75),
    # Hit like a man in a street fight: taken in the gut, thrown back by the blow
    "Hit":            MotionClip(AXE + "standing react large gut.fbx", speed=1.2),
    "Parried":        MotionClip(AXE + "standing react large from left.fbx", speed=0.8),
    "Death":          MotionClip(MAGIC + "Standing React Death Backward.fbx"),
    # Hurled back by the player's last blow before his domain (film 1:15-1:21): falling, skidding on his back,
    # getting up with his hand already raised for the sign (the script carries him through the air and along the
    # ground). Two takes made by NVIDIA Kimodo from text (Tools/Blender/Kimodo, 30 frames a second): thrown onto
    # his back and lying there (the first take, frames 1-34 and 35-101), then sitting up, rising to his full height
    # (the second take; the first one's rising ended in a crouch). Cut before the second take's own raising of the hand
    # (it came out as a hand pressed to the chest): the sign is made by GESTURES ('Sign_Hold') right after
    "Knocked_Down":   MotionClip(KIMODO_KNOCK, 1, 34, speed=0.8, rig=SOMA),
    # (only the throw backwards, slowed over the flight: he comes down on his feet into Skid_Crouch, not on his back)
    "Knock_Flight":   MotionClip(KIMODO_KNOCK, 1, 18, speed=0.45, rig=SOMA),
    "Lying":          MotionClip(KIMODO_KNOCK, 60, 63, speed=0.05, rig=SOMA),
    "Get_Up":         MotionClip(KIMODO_GET_UP, 1, 100, speed=1.6, rig=SOMA),
    # His ascension (the last phase): his own private jet comes down on him and he rises out of the fire wearing it
    # on his back. Powering up, hovering on its engines, diving at the player and landing on one knee and a fist,
    # pounding the ground with both fists
    "Power_Up":       library(ADDON, "Power Up", speed=0.5),
    "Hover":          library(ADDON, "Levitate Idle"),
    "Dive":           library(ADDON, "Flying Forward Super"),
    "Land_Slam":      library(ADDON, "Land_Three_Point", first_frame=14, speed=1.1),
    # Thrown back by the player's last blow before his domain he lands on his feet, crouched, a hand on the ground,
    # and skids on (film 1:19-1:21): the end of the same landing, held
    "Skid_Crouch":    library(ADDON, "Land_Three_Point", first_frame=22, speed=0.3),
    "Ground_Pound":   library(ADDON, "Attack_Ground_Pound", speed=1.0),
}

# The player with his sword of baby oil (his answer to the boss's armor, boss_fight.lua swaps his model to it): the same
# man and the same names of clips, but he fights with a one-handed sword now (Mixamo's Sword And Shield pack, played
# without the shield): quick slashes, a heavy downward cut, the blade raised to block
SWORD_SHIELD = "Mixamo/ProSwordShield/sword and shield "
PLAYER_SWORD = dict(PLAYER, **{
    # (Mixamo's Sword And Shield Idle made for his own rig: the blade held low before him, not over the shoulder)
    "Idle":           diddy("Sword_Idle"),
    "Idle_Fight":     diddy("Sword_Idle"),
    "Walk":           MotionClip(SWORD_SHIELD + "walk.fbx"),
    "Run":            MotionClip(SWORD_SHIELD + "run.fbx"),
    "Walk_Back":      MotionClip(SWORD_SHIELD + "walk (2).fbx"),
    "Strafe_Left":    MotionClip(SWORD_SHIELD + "strafe (2).fbx"),
    "Strafe_Right":   MotionClip(SWORD_SHIELD + "strafe.fbx"),
    "Attack_Light_1": MotionClip(SWORD_SHIELD + "slash.fbx", 4, 40, speed=1.45),
    "Attack_Light_2": MotionClip(SWORD_SHIELD + "slash (3).fbx", 10, 45, speed=1.45),
    "Attack_Light_3": MotionClip(SWORD_SHIELD + "attack (2).fbx", 6, 34, speed=1.25),
    "Attack_Light_4": MotionClip(SWORD_SHIELD + "attack (3).fbx", speed=1.2),
    "Attack_Heavy":   MotionClip(SWORD_SHIELD + "slash (2).fbx", 4, 36, speed=1.0),
    "Attack_Heavy_2": MotionClip(SWORD_SHIELD + "attack (4).fbx", speed=1.0),
    "Attack_Running": MotionClip(SWORD_SHIELD + "attack.fbx", speed=1.15),
    "Attack_Riposte": MotionClip(SWORD_SHIELD + "slash (2).fbx", 4, 36, speed=1.1),
    "Block":          MotionClip(SWORD_SHIELD + "block idle.fbx"),
    "Block_Impact":   MotionClip(SWORD_SHIELD + "impact.fbx"),
    "Parry":          MotionClip(SWORD_SHIELD + "block.fbx", speed=1.4),
    "Hit":            MotionClip(SWORD_SHIELD + "impact (2).fbx"),
    "Hit_2":          MotionClip(SWORD_SHIELD + "impact (2).fbx"),
    # Drawing the sword out of the oil (the transformation's last shot)
    "Draw_Sword":     MotionClip("Mixamo/ProSwordShield/draw sword 1.fbx"),
})

# The boss in his armor (the last two phases, after the transformation of boss_fight.lua): the same names as BOSS -
# the engine swaps his model and the clips go on by name - but he fights with a great sword in both hands now
# (Mixamo's Great Sword pack). Everything the fight's staging plays (his falls, signs, the summoning) stays as BOSS
GREAT_SWORD_PACK = "Mixamo/GreatSword/"
BOSS_ARMORED = dict(BOSS, **{
    "Idle":           MotionClip(GREAT_SWORD + "idle.fbx"),
    "Walk":           MotionClip(GREAT_SWORD + "walk.fbx"),
    "Run":            MotionClip(GREAT_SWORD + "run (2).fbx"),
    "Strafe_Left":    MotionClip(GREAT_SWORD + "strafe.fbx"),
    "Strafe_Right":   MotionClip(GREAT_SWORD + "strafe (2).fbx"),
    "Attack_Light_1": MotionClip(GREAT_SWORD + "slash.fbx", speed=1.15),
    "Attack_Light_2": MotionClip(GREAT_SWORD + "slash (2).fbx", speed=1.15),
    "Attack_Light_3": MotionClip(GREAT_SWORD + "slash (3).fbx", speed=1.1),
    "Attack_Heavy":   MotionClip(GREAT_SWORD + "high spin attack.fbx", speed=1.05),
    "Attack_Leap":    MotionClip(GREAT_SWORD + "jump attack.fbx", speed=1.1),
    "Block":          MotionClip(GREAT_SWORD + "blocking.fbx"),
    "Block_Impact":   MotionClip(GREAT_SWORD + "impact (2).fbx", speed=1.3),
    "Hit":            MotionClip(GREAT_SWORD + "impact.fbx", speed=1.2),
    "Parried":        MotionClip(GREAT_SWORD + "impact (3).fbx", speed=0.8),
    "Death":          MotionClip(GREAT_SWORD_PACK + "two handed sword death.fbx"),
    # His own great moves (played by boss_fight.lua): the leap that splits the ground, the slide, the kick, the
    # spinning slash, the war cry, drawing the sword out of the ground
    "Sword_Slam":     MotionClip(GREAT_SWORD + "jump attack.fbx", speed=0.95),
    "Sword_Slide":    MotionClip(GREAT_SWORD + "slide attack.fbx", speed=1.1),
    "Sword_Kick":     MotionClip(GREAT_SWORD + "kick.fbx", speed=1.1),
    "Sword_Spin":     MotionClip(GREAT_SWORD + "high spin attack.fbx", speed=1.25),
    "Sword_Power_Up": MotionClip(GREAT_SWORD + "power up.fbx"),
    "Sword_Draw":     MotionClip(GREAT_SWORD_PACK + "draw a great sword 1.fbx"),
    "Sword_Cast":     MotionClip(GREAT_SWORD + "casting.fbx"),
})
ARMORED_CANDIDATES = {name[:-4].replace(" ", "_").replace("(", "").replace(")", ""): MotionClip(GREAT_SWORD_PACK + name)
                      for name in sorted(os.listdir(os.path.join(BLENDER_FOLDER, "Mixamo", "GreatSword")))
                      if name.endswith(".fbx") and name != "X Bot.fbx"}

# The ordinary enemies of the street before the arena (Tools/meshy_enemies.py). His bodyguards box: a jab, a cross,
# an uppercut, a kick as the heavy blow; they stagger and fall like men in a street fight
GUARD = dict(UNARMED_WALKING, **{
    "Idle":           MotionClip(BRAWL + "Bouncing Fight Idle.fbx"),
    "Attack_Light_1": MotionClip(BRAWL + "Boxing Jab.fbx", speed=1.0, **JAB),
    "Attack_Light_2": MotionClip(BRAWL + "Cross Punch.fbx", speed=1.0, **CROSS),
    "Attack_Light_3": MotionClip(BRAWL + "Uppercut.fbx", speed=1.0, **UPPERCUT),
    "Attack_Heavy":   MotionClip(BRAWL + "Mma Kick.fbx", speed=0.95, **KICK),
    "Backstep":       MotionClip("Mixamo/Standing Dodge Backward.fbx", 4, 40, speed=1.2),
    "Hit":            MotionClip(AXE + "standing react large gut.fbx", speed=1.2),
    "Parried":        MotionClip(AXE + "standing react large from left.fbx", speed=0.8),
    "Death":          MotionClip(MAGIC + "Standing React Death Backward.fbx"),
})
# The photographers: they keep their distance and flash their cameras in the player's face (a spell of the scene,
# thrown with the arm like the boss's), and shove when the player comes close
PAPARAZZO = dict(UNARMED_WALKING, **{
    "Idle":           MotionClip(AXE + "unarmed idle.fbx"),
    "Attack_Light_1": MotionClip(BRAWL + "Cross Punch.fbx", speed=0.9, **CROSS),
    "Attack_Light_2": MotionClip(BRAWL + "Hook Punch.fbx", speed=0.9, **HOOK),
    "Attack_Heavy":   library(ADDON, "Throw Object", speed=1.0),
    "Cast_Spell":     library(ADDON, "Throw Object", speed=1.2),
    "Backstep":       MotionClip("Mixamo/Standing Dodge Backward.fbx", 4, 40, speed=1.2),
    "Hit":            MotionClip(AXE + "standing react large gut.fbx", speed=1.2),
    "Parried":        MotionClip(AXE + "standing react large from left.fbx", speed=0.8),
    "Death":          MotionClip(MAGIC + "Standing React Death Backward.fbx"),
})

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
    ("Idle_Fight", ("fist", "fist")),
    ("Idle", ("relaxed", "relaxed")),
    ("Block", ("fist", "fist")),
    ("Parry", ("fist", "open")),
    ("Sign_Domain", ("two", "relaxed")),
    ("Sign_Hold", ("two", "relaxed")),
    ("Sign_Raise", ("two", "relaxed")),
    ("Cast_Great_Orb", ("open", "open")),
    ("Cast_Domain", ("steeple", "steeple")),
    ("Cast_Orb", ("open", "relaxed")),
    ("Cast_Pillars", ("two", "relaxed")),
    ("Summon", ("two", "relaxed")),
    ("Cast_Stars", ("two", "two")),
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
    # The part before which the clip's hand is left alone, and by which the hand is back in the clip's pose: two
    # gestures one after another (hold, then push) make one move
    start_at: float = 0.0
    end_at: float = 1.0


GESTURES = {
    "sih_boss": {
        # Beside his face, the fingers up, the palm turned half towards his own face
        # (he holds the sign already when its shot starts: the hand is there from the first frame, no drop and rise)
        "Sign_Domain": [Gesture("_R", (0.30, 0.20, 0.05), (0.0, 0.08, 1.0), (-0.6, 0.8, 0.0), reached_at=0.0)],
        "Sign_Raise":  [Gesture("_R", (0.30, 0.20, 0.05), (0.0, 0.08, 1.0), (-0.6, 0.8, 0.0), reached_at=0.3)],
        # The same sign held high and forward over the battlefield: his jet comes, his domain answers, his summon rises
        "Cast_Pillars": [Gesture("_R", (0.26, 0.38, 0.30), (0.0, 0.2, 1.0), (-0.3, 0.95, 0.0), reached_at=0.3, left_from=0.85)],
        "Cast_Domain":  [Gesture("_R", (0.30, 0.20, 0.05), (0.0, 0.08, 1.0), (-0.6, 0.8, 0.0), reached_at=0.25)],
        "Sign_Hold":    [Gesture("_R", (0.30, 0.20, 0.05), (0.0, 0.08, 1.0), (-0.6, 0.8, 0.0), reached_at=0.0)],
        "Summon":       [Gesture("_R", (0.26, 0.38, 0.30), (0.0, 0.2, 1.0), (-0.3, 0.95, 0.0), reached_at=0.3, left_from=0.9)],
        # Film 0:42: two fingers up before his face, the palm towards the player; 0:46: both arms out, the fingers
        # pointing at the player (the stars leave at 0.68 of it, see boss_fight.lua)
        "Cast_Stars":   [Gesture("_R", (0.10, 0.34, 0.20), (0.0, 0.15, 1.0), (-0.2, 1.0, 0.0),
                                 reached_at=0.15, left_from=0.42, end_at=0.55),
                         Gesture("_R", (0.16, 0.56, 0.10), (0.0, 1.0, 0.08), (-0.25, 0.0, -1.0),
                                 start_at=0.42, reached_at=0.58, left_from=0.9),
                         Gesture("_L", (0.16, 0.56, 0.10), (0.0, 1.0, 0.08), (-0.25, 0.0, -1.0),
                                 start_at=0.42, reached_at=0.58, left_from=0.9)],
    },
    "sih_player": {
        # As in the film (1:16): standing calm, the open palm turned up at his side, a little forward, the ball of oil
        # floating over it; then the palm pushes it straight at the enemy (the spell leaves at 0.66) and comes back
        # (the palm in front of him a little under the chest, the arm reaching forward, as in the film's close shot)
        "Cast_Orb":       [Gesture("_R", (0.24, 0.42, -0.16), (0.2, 1.0, 0.0), (0.0, 0.0, 1.0),
                                   reached_at=0.16, left_from=0.52, end_at=0.64),
                           Gesture("_R", (0.10, 0.62, -0.02), (0.0, 0.25, 1.0), (0.0, 1.0, 0.0),
                                   start_at=0.52, reached_at=0.64, left_from=0.80, end_at=0.98)],
        # The parry: the lead (left) forearm leaves the guard and sweeps out and forward, the open palm turning the
        # blow away, then comes back to the guard
        "Parry":          [Gesture("_L", (0.42, 0.40, 0.12), (0.15, 0.35, 1.0), (0.75, 0.65, 0.0),
                                   reached_at=0.16, left_from=0.5, end_at=0.9)],
        # As in the film (1:47): a diamond of both hands before the chest, the fingertips meeting above and the
        # thumbs below, the palms looking forward
        # (high on the chest, the wrists close: the diamond under his chin, as in the film's close shot)
        "Cast_Great_Orb": [Gesture("_R", (0.11, 0.31, 0.04), (-0.75, 0.0, 0.7), (0.0, 1.0, 0.1), reached_at=0.3, left_from=0.9),
                           Gesture("_L", (0.11, 0.31, 0.04), (-0.75, 0.0, 0.7), (0.0, 1.0, 0.1), reached_at=0.3, left_from=0.9)],
        # As in the film (2:19): both hands clasped before the chin, the index fingers straight up together
        "Cast_Domain":    [Gesture("_R", (0.02, 0.30, 0.12), (0.0, 0.25, 1.0), (-1.0, 0.0, 0.0), reached_at=0.25, left_from=0.92),
                           Gesture("_L", (0.02, 0.30, 0.12), (0.0, 0.25, 1.0), (-1.0, 0.0, 0.0), reached_at=0.25, left_from=0.92)],
    },
}


# (in his armor he makes the same signs)
GESTURES["sih_boss_armored"] = GESTURES["sih_boss"]
# (with his sword the player makes the signs of his techniques the same way; he parries with the blade, not the arm)
# (with his sword in the right fist only the left hand makes the sign: the right one keeps the blade where the clip
# holds it, instead of bringing it up under his chin)
GESTURES["sih_player_sword"] = {name: [g for g in gestures if g.side == "_L"] for name, gestures in GESTURES["sih_player"].items()
                                if name != "Parry"}
# (and in his armor of baby oil: the same signs - without this his techniques were made with the clip's idle hands)
GESTURES["sih_player_armored"] = GESTURES["sih_player_sword"]


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

    def weight(progress):
        if progress <= gesture.start_at or progress >= gesture.end_at:
            return 0.0
        if progress < gesture.reached_at:
            return hand_pinning.smooth((progress - gesture.start_at) / max(gesture.reached_at - gesture.start_at, 1e-4))
        if progress > gesture.left_from:
            return hand_pinning.smooth((gesture.end_at - progress) / max(gesture.end_at - gesture.left_from, 1e-4))
        return 1.0

    return hand_pinning.HandPin(gesture.side, target, weight)


# Actions of a character with motion-captured fingers whose fingers are still posed (the signs of the techniques)
GESTURE_FINGERS_ONLY = ("Cast_", "Sign_", "Summon")


def take_materials(body, model_path):
    """The materials of another model of the same mesh (the same texture coordinates) put on 'body'"""
    objects_before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=model_path)
    imported = [obj for obj in bpy.data.objects if obj not in objects_before]
    donor = next(obj for obj in imported if obj.type == "MESH")
    body.data.materials.clear()
    for material in donor.data.materials:
        body.data.materials.append(material)
    for obj in imported:
        bpy.data.objects.remove(obj)


def pose_hands(character, armature, rig, actions):
    size = character.height / 1.83
    gestures = GESTURES.get(character.name, {})

    for action in actions:
        if action.name in gestures:
            hand_pinning.pin_hands(armature, action, [gesture_pin(armature, rig, gesture, size) for gesture in gestures[action.name]])

        if character.name == "sih_summon":
            poses = {"_R": MONSTER_POSE, "_L": MONSTER_POSE}
        elif character.mixamo_rigged:
            # Motion capture made for this rig moves the fingers itself: posed only for the signs of the hands
            # (clips of other characters have no fingers for this rig: posed as everywhere else)
            clip = character.animations.get(action.name)
            own_fingers = clip is not None and clip.rig is MIXAMO_NATIVE
            is_sign = action.name in gestures or action.name.startswith(GESTURE_FINGERS_ONLY)
            right, left = finger_poses_of(action.name)
            if character.fists_always:
                # (a held sword: the right fist stays closed around its grip in every clip, his own takes too; the left
                # hand still makes the signs of his techniques)
                right = "fist"
                left = left if is_sign else "fist"
            elif own_fingers and not is_sign:
                continue
            poses = {"_R": right, "_L": left}
        else:
            right, left = finger_poses_of(action.name)
            if character.fists_always and not action.name.startswith(GESTURE_FINGERS_ONLY):
                right, left = "fist", "fist"
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
    # How glossy (times the generated shine): below 1 for clothes that came out like plastic
    shine: float = 1.0
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
    # Rigged by Mixamo's auto-rigger (the source is its FBX with the skin, in Tools/Blender): its weights and its
    # fingers are kept, the clips are Mixamo's own for this rig, and 'textures' (a model in SOURCE_FOLDER) lends the
    # materials (the upload carried only the colour)
    mixamo_rigged: bool = False
    textures: str = ""
    # The plates of an armored character also cut out as props of their own (export_armor_pieces)
    armor_pieces: bool = False
    # Both hands closed in every clip (they hold a great sword)
    fists_always: bool = False
    # A held model (a prop's gltf in res/Models/Props) on a bone of its own in the right fist: 'Weapon_R'
    weapon: str = ""
    # Rigid pieces of armor (CRYSTAL_ARMOR: Meshy models) fitted around his body and moved by one bone each
    crystal_armor: bool = False
    # A cold smile as the expression shape of the head (the engine's model.expression): the corners of the mouth up
    smirk: bool = False


CHARACTERS = [
    # From the user's second A-pose sheet (front, side, back): its face is good enough as generated, so no photo is
    # painted over it (the photo on v2 left white patches at the mouth and beard)
    Character("Mixamo/Diddy/Diddy_Skin_WeightShiftIdle.fbx", "Player", "sih_player", 1.83, PLAYER,
              smooth_head=1, shine=0.25, mixamo_rigged=True, textures="player_v3.glb",
              heat_weights=False, smooth_weights=0),
        # His generated face already looks like him; the film's still (1:03.5, smiling, tilted, other light) painted over
    # it looked worse - tried and left out:
    #   FacePhoto(os.path.join(CONCEPTS, "boss_face_film.png"), (549, 353), (671, 319), (649, 462))
    Character("boss_rigged.glb", "Boss", "sih_boss", 1.83, BOSS, smirk=True),
    # In his armor (Concepts/boss_armor_apose.png, from the user's sheet): the same skeleton; his pieces (the plates
    # of the torso with the pauldrons, both bracers, both greaves) are also cut out as props that fly onto him in
    # the transformation's cutscene
    Character("boss_armored_rigged.glb", "BossArmored", "sih_boss_armored", 1.95, BOSS_ARMORED, armor_pieces=True,
              fists_always=True, weapon="BossSword/sih_boss_sword.gltf", smirk=True),
    # The player with his sword of baby oil (the last phases): the same model, the sword in his right fist
    Character("Mixamo/Diddy/Diddy_Skin_WeightShiftIdle.fbx", "PlayerSword", "sih_player_sword", 1.83, PLAYER_SWORD,
              smooth_head=1, shine=0.25, mixamo_rigged=True, textures="player_v3.glb",
              heat_weights=False, smooth_weights=0, fists_always=True, weapon="DiddySword/sih_diddy_sword.gltf"),
    # ...and in his armor of baby oil (WORK IN PROGRESS, not in the scene yet: the pieces are fitted, but the glass of
    # their textures is see-through and the fit on the arms needs checking): the same, with crystal pieces on his bones
    Character("Mixamo/Diddy/Diddy_Skin_WeightShiftIdle.fbx", "PlayerArmored", "sih_player_armored", 1.83, PLAYER_SWORD,
              smooth_head=1, shine=0.25, mixamo_rigged=True, textures="player_v3.glb",
              heat_weights=False, smooth_weights=0, fists_always=True, weapon="DiddySword/sih_diddy_sword.gltf",
              crystal_armor=True),
    Character("summon_monster_rigged.glb", "Summon", "sih_summon", 3.0, SUMMON),
    Character("enemy_guard_rigged.glb", "Guard", "sih_guard", 1.9, GUARD, shine=0.12),
    Character("enemy_paparazzo_rigged.glb", "Paparazzo", "sih_paparazzo", 1.76, PAPARAZZO, shine=0.4,
              ),
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
    parser.add_argument("--candidates", action="store_true", help="the player with every downloaded clip whole")
    return parser.parse_args(argv)


def make_character(character, arguments):
    clear_scene()
    out_folder = os.path.join(OUT_FOLDER, character.folder)
    os.makedirs(out_folder, exist_ok=True)

    if character.mixamo_rigged:
        armature, body = character_from_mesh.load_rigged_character(
            os.path.join(BLENDER_FOLDER, character.source), character.height, motion_retarget.mixamo_native_names())
        take_materials(body, os.path.join(SOURCE_FOLDER, character.textures))
    else:
        our_names = {theirs: ours for ours, theirs in motion_retarget.MESHY.bone_of.items()}
        armature, body = character_from_mesh.load_rigged_character(os.path.join(SOURCE_FOLDER, character.source), character.height, our_names)
    body.name = character.name
    character_from_mesh.make_engine_textures(body, out_folder, character.name, character.brightness, character.shine)

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

    if character.armor_pieces:
        export_armor_pieces(character, body)

    # Fingers before the animations: every clip then keys what the hands do
    rig = finger_rig.hands_from_skeleton(armature) if character.mixamo_rigged else finger_rig.add_fingers(armature, body)
    print_eyes(character, armature, body)

    # The head with a texture of its own (on the generated atlas the face is many small islands: blocky up close),
    # with the photo of the person's face painted over its front (a generated face is only a guess at it)
    photo = character.face_photo if not arguments.no_face else None
    head = character_from_mesh.separate_head(body, armature, out_folder, character.name, character.height, photo, character.eye_offset)

    if character.smirk:
        add_smirk(character, head, arguments.preview)

    weapon = add_weapon_bone(character, armature, rig) if character.weapon else None
    armor = add_crystal_armor(character, armature, body, out_folder) if character.crystal_armor else []

    actions = knight_generator.create_animations(armature, character.animations)
    pose_hands(character, armature, rig, actions)

    # The twists of the hands spread up the arms, and no arm bone jumps further than a fast punch turns it in one frame
    # (motion capture made for this very rig is kept as it is)
    if not character.mixamo_rigged:
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
    export_gltf(os.path.join(out_folder, character.name + ".gltf"), [armature, body, head] + armor, with_animations=True)

    if arguments.preview:
        if weapon:
            held = load_prop(character.weapon)
            knight_generator.hang_on_bone(held, armature, WEAPON_BONE, weapon)
            head = [head, held]
        else:
            head = [head]
        head = head + armor
        render_previews(os.path.abspath(arguments.preview), armature, [body] + head, character,
                        arguments.preview_actions.split(","), arguments.preview_steps, arguments.hands)


WEAPON_BONE = knight_generator.WEAPON_BONE


# ========== The smile ==========

# Measured from the eyes (find_eyes), times the character's size: the mouth this far under them, its corners this far
# to each side of the middle; how far a corner goes up, out and back at the full smile, and how wide the skin around
# it follows (a gaussian)
# (a cold smile: one corner, his left, higher than the other)
SMIRK = {"mouth_below_eyes": 0.072, "corner_across": 0.034, "up": 0.02, "out": 0.006, "back": 0.004, "radius": 0.017,
         "sides": {1.0: 0.6, -1.0: 1.0}}


def add_smirk(character, head, preview_folder=""):
    """The shape key 'Smirk' on the head: both corners of the mouth pulled up and a little out, the skin around them
    following - exported as the head's first morph target, the engine's expression"""
    size = character.height / 1.83
    right_eye, left_eye = character_from_mesh.find_eyes(head, character.height)
    mouth_z = (right_eye.z + left_eye.z) * 0.5 - SMIRK["mouth_below_eyes"] * size
    world = head.matrix_world
    to_local = world.inverted().to_3x3()
    points = [world @ vertex.co for vertex in head.data.vertices]

    if not head.data.shape_keys:
        head.shape_key_add(name="Basis", from_mix=False)
    smirk = head.shape_key_add(name="Smirk", from_mix=False)
    moved = 0
    for side in (1.0, -1.0):
        across = side * SMIRK["corner_across"] * size
        near = [p for p in points if abs(p.z - mouth_z) < 0.015 * size and abs(p.x - across) < 0.015 * size]
        if not near:
            print("SMIRK no mouth corner found on side", side)
            continue
        corner = Vector((across, max(p.y for p in near), mouth_z))
        offset = Vector((side * SMIRK["out"], -SMIRK["back"], SMIRK["up"])) * size * SMIRK["sides"][side]
        radius = SMIRK["radius"] * size
        for index, point in enumerate(points):
            distance = (point - corner).length
            if distance > radius * 2.5:
                continue
            weight = math.exp(-(distance / radius) ** 2)
            smirk.data[index].co = smirk.data[index].co + to_local @ (offset * weight)
            moved += 1
        print("SMIRK corner {:.3f} {:.3f} {:.3f}".format(corner.x, corner.y, corner.z))
    largest = max(((smirk.data[i].co - head.data.vertices[i].co).length for i in range(len(points))), default=0.0)
    print("SMIRK {} vertices moved, the most {:.4f} (local), matrix scale {}".format(moved, largest, tuple(round(v, 3) for v in world.to_scale())))

    if preview_folder:
        camera = setup_preview_render(360, 360)
        face = (right_eye + left_eye) * 0.5 - Vector((0.0, 0.0, SMIRK["mouth_below_eyes"] * 0.5 * size))
        show_only([head])
        for value in (0.0, 1.0):
            smirk.value = value
            for angle in (0, 30):
                aim_camera_at(camera, face, angle, 0.3 * size, rise=0.0)
                bpy.context.scene.render.filepath = os.path.join(os.path.abspath(preview_folder),
                                                                 "{}_smirk_{}_{}.png".format(character.name, int(value), angle))
                bpy.ops.render.render(write_still=True)
        smirk.value = 0.0


# ========== The armor of baby oil (Divine Baby Oil: Crystal Vessel) ==========

# piece: (Meshy model in Generated/StrongestInHistory, the bones whose part of the body it covers, the bone that moves
# it, how much bigger than the body it is, its kind: None the torso, "limb" along its bones, "shoulder" over the top
# of the arm; and how far around the bones the body is measured, meters)
CRYSTAL_ARMOR = {
    "chest":      ("diddy_armor_chest.glb", ("Spine", "Chest"), "Chest", 1.2, None, 0.26),
    "pauldron_r": ("diddy_armor_pauldron.glb", ("UpperArm_R",), "UpperArm_R", 1.15, "shoulder", 0.1),
    "bracer_r":   ("diddy_armor_bracer.glb", ("LowerArm_R",), "LowerArm_R", 1.02, "limb", 0.07),
    "greave_r":   ("diddy_armor_greave.glb", ("Shin_R",), "Shin_R", 1.12, "limb", 0.11),
}
MIRRORED = {"pauldron_r": "pauldron_l", "bracer_r": "bracer_l", "greave_r": "greave_l"}


def import_piece(file_name):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(SOURCE_FOLDER, file_name))
    imported = [obj for obj in bpy.data.objects if obj not in before]
    meshes = [obj for obj in imported if obj.type == "MESH"]
    for obj in meshes:
        obj.data.transform(obj.matrix_world)
        obj.parent = None
        obj.matrix_world = Matrix.Identity(4)
    for obj in imported:
        if obj not in meshes:
            bpy.data.objects.remove(obj)
    if len(meshes) > 1:
        bpy.ops.object.select_all(action="DESELECT")
        for obj in meshes:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = meshes[0]
        bpy.ops.object.join()
    return meshes[0]


def body_points_of(armature, body, bones, radius):
    """The rest positions of the body's vertices around those bones: closer than 'radius' to the line from the first
    bone's head to the last bone's tail, and between its ends (the part of the body a piece covers)"""
    start = armature.matrix_world @ armature.data.bones[bones[0]].head_local
    end = armature.matrix_world @ armature.data.bones[bones[-1]].tail_local
    along = end - start
    length = along.length
    along.normalize()
    points = []
    for vertex in body.data.vertices:
        point = body.matrix_world @ vertex.co
        t = (point - start).dot(along)
        if 0.0 <= t <= length and (point - (start + along * t)).length < radius:
            points.append(point)
    return points


def frame_along(axis, front):
    """x across, y backwards, z along 'axis' (a piece made by Meshy looks towards its own -Y)"""
    z = axis.normalized()
    y = -(front - z * front.dot(z)).normalized()
    x = y.cross(z).normalized()
    return Matrix((x, y, z)).transposed()


def fit_piece(piece, points, rotation, margin):
    """The piece turned by 'rotation' and stretched to cover the points (in the piece's own frame), a little bigger"""
    import numpy
    local = numpy.array([tuple(rotation.transposed() @ point) for point in points])
    low, high = local.min(axis=0), local.max(axis=0)
    centre = Vector(tuple((low + high) * 0.5))
    size = (high - low) * margin
    vertices = numpy.array([tuple(vertex.co) for vertex in piece.data.vertices])
    p_low, p_high = vertices.min(axis=0), vertices.max(axis=0)
    p_size = numpy.maximum(p_high - p_low, 1e-4)
    p_centre = Vector(tuple((p_low + p_high) * 0.5))
    scale = Matrix.Diagonal(Vector((size[0] / p_size[0], size[1] / p_size[1], size[2] / p_size[2], 1.0)))
    piece.data.transform(Matrix.Translation(rotation @ centre) @ rotation.to_4x4() @ scale @ Matrix.Translation(-p_centre))


ARMOR_TEXTURE_SIZE = 1024


def shrink_textures(piece, folder, base_name, size):
    """A piece's textures scaled down to 'size' pixels (if bigger): the image of its material (written again by the
    export) and its specular texture on the disk (found by the engine by its name)"""
    for node in piece.data.materials[0].node_tree.nodes:
        if node.type == "TEX_IMAGE" and node.image and node.image.size[0] > size:
            node.image.scale(size, size)
    path = os.path.join(folder, base_name + "_specular.png")
    if os.path.exists(path):
        image = bpy.data.images.load(path)
        if image.size[0] > size:
            image.scale(size, size)
            image.save()
        bpy.data.images.remove(image)


def add_crystal_armor(character, armature, body, out_folder):
    """The pieces of CRYSTAL_ARMOR fitted around the body at rest, each moved by one bone (rigid, weight 1), with
    textures of their own. Returns the meshes (exported with the character)"""
    front = Vector((0.0, 1.0, 0.0))   # (the character faces +Y)
    head = lambda name: armature.matrix_world @ armature.data.bones[name].head_local
    tail = lambda name: armature.matrix_world @ armature.data.bones[name].tail_local
    pieces = []
    for name, (file_name, bones, bone, margin, kind, radius) in CRYSTAL_ARMOR.items():
        points = body_points_of(armature, body, bones, radius)
        print("ARMOR {} covers {} points".format(name, len(points)))
        if kind == "limb":
            axis = tail(bones[-1]) - head(bones[0])
        elif kind == "shoulder":
            # (over the top of the shoulder: its own up is the world's up, only the upper third of the arm)
            axis = Vector((0.0, 0.0, 1.0))
            start, arm = head(bones[0]), tail(bones[0]) - head(bones[0])
            points = [point for point in points if (point - start).dot(arm) < arm.length_squared * 0.4]
        else:
            axis = Vector((0.0, 0.0, 1.0))
        piece = import_piece(file_name)
        fit_piece(piece, points, frame_along(axis, front), margin)
        piece.name = f"{character.name}_{name}"
        # (its textures, a fourth of the generated size: a piece is small on the screen)
        character_from_mesh.make_engine_textures(piece, out_folder, piece.name, 1.05, 1.6)
        piece.data.materials[0].name = piece.name + "_crystal"
        shrink_textures(piece, out_folder, piece.name, ARMOR_TEXTURE_SIZE)
        pieces.append((piece, bone))
        if name in MIRRORED:
            copy = piece.copy()
            copy.data = piece.data.copy()
            bpy.context.scene.collection.objects.link(copy)
            copy.data.transform(Matrix.Scale(-1.0, 4, Vector((1.0, 0.0, 0.0))))
            copy.data.flip_normals()
            copy.name = f"{character.name}_{MIRRORED[name]}"
            # (the mirrored piece wears the same material: one set of textures for both sides)
            pieces.append((copy, bone.replace("_R", "_L")))

    out = []
    for piece, bone in pieces:
        piece.vertex_groups.clear()
        group = piece.vertex_groups.new(name=bone)
        group.add(range(len(piece.data.vertices)), 1.0, "REPLACE")
        piece.parent = armature
        modifier = piece.modifiers.new("Armature", "ARMATURE")
        modifier.object = armature
        out.append(piece)
        print("ARMOR {} on {}".format(piece.name, bone))
    return out


def fist_hole(armature, hand):
    """The middle of the hole of the closed fist (where a grip goes), measured on the fingers bent into the fist:
    halfway between the knuckles and the tip of the curled middle finger. None for a hand without three bones a finger"""
    names = hand.bones.get("Middle")
    if not names or len(names) != 3:
        return None
    rotations = finger_rig.pose_rotations(armature, hand, "fist")
    for name, rotation in rotations.items():
        bone = armature.pose.bones[name]
        bone.rotation_mode = "QUATERNION"
        bone.rotation_quaternion = rotation
    bpy.context.view_layer.update()
    knuckles = sum((armature.matrix_world @ armature.pose.bones[hand.bones[finger][0]].head
                    for finger in ("Index", "Middle", "Ring")), Vector()) / 3.0
    tip = armature.matrix_world @ armature.pose.bones[names[2]].tail
    for name in rotations:
        armature.pose.bones[name].rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
    bpy.context.view_layer.update()
    return (knuckles + tip) * 0.5


def add_weapon_bone(character, armature, rig):
    """The bone that carries the held model (its grip at the origin, its blade along +Z): in the middle of the right
    fist, the blade coming out on the side of the thumb, its edges along the fingers. Returns its rest matrix"""
    hand = rig["_R"]
    blade = hand.across.normalized()
    edges = (hand.fingers - blade * hand.fingers.dot(blade)).normalized()
    flat = blade.cross(edges)
    # (hand.fingers is only a direction: the middle of the fist is a part of the hand's length from the wrist)
    center = fist_hole(armature, hand)
    if center is None:
        center = hand.wrist + hand.fingers * (hand.length * 0.45) + hand.palm.normalized() * 0.03 * character.height / 1.83
    matrix = Matrix.Translation(center) @ Matrix((edges, flat, blade)).transposed().to_4x4()
    character_from_mesh.add_held_item_bones(armature, [(WEAPON_BONE, "Hand_R", matrix)])
    return matrix


def load_prop(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(PROJECT, "res", "Models", "Props", path))
    return next(obj for obj in bpy.data.objects if obj not in before and obj.type == "MESH")


# Which piece of the armor a bone's skin belongs to (None: not armor - the head)
def armor_piece_of(bone):
    for side in ("L", "R"):
        if bone.endswith("_" + side):
            if bone.startswith(("LowerArm", "Hand", "Index", "Middle", "Ring", "Little", "Thumb", "Finger")):
                return "bracer_" + side.lower()
            if bone.startswith(("Thigh", "Shin", "Foot", "Toe")):
                return "greave_" + side.lower()
    if bone.startswith("Head"):
        return None
    return "torso"


ARMOR_PIECES_FOLDER = os.path.join(PROJECT, "res", "Models", "Props", "BossArmor")


def export_armor_pieces(character, body):
    """The plates of the armor as props: the body's faces split by the bone that moves them most, each piece with its
    middle at its own origin. Prints PIECE lines: where each piece sits on the character (meters in the engine's space,
    from his feet, his front towards -Z), for the cutscene that puts them on"""
    import bmesh
    os.makedirs(ARMOR_PIECES_FOLDER, exist_ok=True)
    groups = {group.index: group.name for group in body.vertex_groups}
    owner = []
    for vertex in body.data.vertices:
        best = max(vertex.groups, key=lambda g: g.weight, default=None)
        owner.append(armor_piece_of(groups[best.group]) if best else None)

    for piece in ("torso", "bracer_l", "bracer_r", "greave_l", "greave_r"):
        copy = body.copy()
        copy.data = body.data.copy()
        bpy.context.scene.collection.objects.link(copy)
        world = body.matrix_world.copy()
        copy.parent = None
        copy.modifiers.clear()
        copy.matrix_world = world
        mesh = bmesh.new()
        mesh.from_mesh(copy.data)
        mesh.verts.ensure_lookup_table()
        doomed = [face for face in mesh.faces
                  if sum(owner[vertex.index] == piece for vertex in face.verts) * 2 < len(face.verts)]
        bmesh.ops.delete(mesh, geom=doomed, context="FACES")
        loose = [vertex for vertex in mesh.verts if not vertex.link_faces]
        bmesh.ops.delete(mesh, geom=loose, context="VERTS")
        mesh.to_mesh(copy.data)
        mesh.free()
        copy.vertex_groups.clear()
        # (into world space, then its middle to the origin)
        copy.data.transform(world)
        copy.matrix_world = Matrix.Identity(4)
        points = [vertex.co for vertex in copy.data.vertices]
        middle = sum(points, Vector()) / max(len(points), 1)
        copy.data.transform(Matrix.Translation(-middle))
        copy.name = "{}_{}".format(character.name, piece)
        export_gltf(os.path.join(ARMOR_PIECES_FOLDER, copy.name + ".gltf"), [copy], with_animations=False)
        # Blender's +Y (the character's front) is the engine's -Z
        print("PIECE {} {:.3f} {:.3f} {:.3f} faces {}".format(piece, middle.x, middle.z, -middle.y, len(copy.data.polygons)))
        bpy.data.objects.remove(copy)


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
            if arguments.candidates and character.name == "sih_player":
                character.animations = PLAYER_CANDIDATES
            if arguments.candidates and character.name == "sih_boss_armored":
                character.animations = dict(ARMORED_CANDIDATES)
                character.armor_pieces = False
            make_character(character, arguments)
    print("CHARACTERS_GENERATOR_DONE")


if __name__ == "__main__":
    main()
