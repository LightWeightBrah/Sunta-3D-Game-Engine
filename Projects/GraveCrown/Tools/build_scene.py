"""
Builds res/Scenes/Main.scene of the sample game Grave Crown: the crypt-citadel of the dead king (the base: a bonfire,
his throne, the knights of his court), its gate onto the ash plains (ruins, camps of the gods' faithful, a second
bonfire) and the arena of the Chain Warden behind a fog gate at the far end.

    python Projects/GraveCrown/Tools/build_scene.py

The souls-like core (bonfires, a currency dropped on death, stamina, a flask, level ups, a fog gate and a boss bar) is the
engine's own components; their settings start as copies of Scrap Souls's (the first sample game) and are changed here.
The scene is generated: change this file and run it again rather than editing Main.scene by hand.
It also copies the shared level kit (walls, pillars, braziers...) of Scrap Souls into this project's res folder, since a
game loads only its own models.
"""

import copy
import json
import sys
import math
import random
import shutil
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))
import terrain_map  # noqa: E402
SOURCE_PROJECT = PROJECT.parent / "ScrapSouls"
SOURCE_SCENE = SOURCE_PROJECT / "res" / "Scenes" / "Main.scene"
TARGET_SCENE = PROJECT / "res" / "Scenes" / "Main.scene"

# Shared assets of Scrap Souls this game uses as they are (all made for the project or CC0, see their CREDITS)
SHARED_FOLDERS = ["Models/Kit", "Models/Arena", "Models/Level", "Skyboxes/AshenNight", "UI/HUD",
                  "Sounds/Ambience", "Sounds/Boss", "Sounds/Combat", "Sounds/Movement", "Sounds/Music"]
SHARED_FILES = ["Sounds/CREDITS.md", "App/window_icon.png"]

PLAYER_HITBOX_LAYER = 8
ENEMY_HITBOX_LAYER = 16

# ========== The map (meters; the crypt's throne in the north, +Z; the plains to the south, -Z) ==========

CRYPT_HALF_WIDTH = 12.0
CRYPT_FROM_Z, CRYPT_TO_Z = 0.0, 40.0
THRONE_Z = 36.5
CRYPT_BONFIRE = (0.0, 0.0, 22.0)
PLAYER_START = (0.0, 0.0, 26.0)
PLAINS_HALF_WIDTH = 64.0
PLAINS_TO_Z = -120.0
PLAINS_BONFIRE = (-10.0, 0.0, -48.0)
FOG_Z = -122.0
ARENA_CENTER = (0.0, 0.0, -146.0)
ARENA_HALF = 22.0

# ========== The look: a dark violet dusk over ash, painted ==========

ENVIRONMENT = {
    "skybox_folder": "@game/Skyboxes/VioletStorm", "sky_brightness": 1.0, "is_fog_enabled": True,
    # An ash-violet haze that takes the far hills into layers of silhouettes (the color of the storm's horizon),
    # ink on the near edges fading with it, colors a little drained and warm: one look for the whole world
    "fog_color": [0.22, 0.13, 0.19], "fog_density": 0.022,
    "outline_strength": 0.45, "outline_color": [0.06, 0.03, 0.07],
    "paint_radius": 3, "saturation": 0.85, "contrast": 1.18, "color_tint": [1.04, 0.96, 0.93], "vignette": 0.38, "exposure": 1.0,
}
MOONLIGHT = {"ambient": [0.2, 0.18, 0.21], "diffuse": [0.9, 0.82, 0.8], "specular": [0.35, 0.32, 0.42],
             "casts_shadows": True, "shadow_distance": 45.0, "shadow_strength": 1.0, "shadow_softness": 1,
             "shadow_resolution": 2048}

# ========== The characters (Tools/characters_generator.py; STRIKE / STRIDE / WEAPON_HITBOX lines of its log) ==========

KING_MOVES = [
    {"animation": "Attack_Light_1", "hit_from": 0.38, "hit_to": 0.58, "damage_multiplier": 1.0},
    {"animation": "Attack_Light_2", "hit_from": 0.2, "hit_to": 0.42, "damage_multiplier": 1.0},
    {"animation": "Attack_Light_3", "hit_from": 0.38, "hit_to": 0.58, "damage_multiplier": 1.15},
    {"animation": "Attack_Heavy", "hit_from": 0.43, "hit_to": 0.58, "damage_multiplier": 1.0},
    {"animation": "Attack_Heavy_2", "hit_from": 0.12, "hit_to": 0.32, "damage_multiplier": 1.0},
    {"animation": "Attack_Running", "hit_from": 0.44, "hit_to": 0.63, "damage_multiplier": 1.2},
    # the finisher: the first cleave of the pack's "slash (2)" (frames 0-42), the blade comes down at 0.48-0.71
    {"animation": "Riposte", "hit_from": 0.42, "hit_to": 0.72, "damage_multiplier": 1.0},
]
KING_SWORD_HITBOX = {"local_offset": [0.0, 0.79, 0.0], "half_extents": [0.17, 0.8, 0.07]}
KNIGHT_SWORD_HITBOX = {"local_offset": [0.0, 0.52, 0.0], "half_extents": [0.12, 0.53, 0.05]}
COURT_SWORD_HITBOX = {"local_offset": [0.0, 0.54, 0.0], "half_extents": [0.12, 0.55, 0.05]}
STAFF_HITBOX = {"local_offset": [0.0, 0.54, 0.0], "half_extents": [0.16, 0.54, 0.09]}
# The knight pack's clips (one-handed): light 0.33-0.56, heavy 0.46-0.80
KNIGHT_WINDOWS = {"light_hit_from": 0.33, "light_hit_to": 0.56, "heavy_hit_from": 0.46, "heavy_hit_to": 0.78}
CULTIST_WINDOWS = {"light_hit_from": 0.38, "light_hit_to": 0.52, "heavy_hit_from": 0.24, "heavy_hit_to": 0.46}

IDENTITY = [1.0, 0.0, 0.0, 0.0]


def transform(position, yaw=0.0, scale=1.0):
    half = math.radians(yaw) * 0.5
    size = list(scale) if isinstance(scale, (list, tuple)) else [scale] * 3
    return {"position": list(position), "rotation": [0.0, yaw, 0.0],
            "rotation_quaternion": [math.cos(half), 0.0, math.sin(half), 0.0], "scale": size}


def named(name, position=(0.0, 0.0, 0.0), yaw=0.0, scale=1.0, **components):
    entity = {"tag": {"name": name}, "transform": transform(position, yaw, scale)}
    entity.update(components)
    return entity


def collider(half, offset=None, trigger=False):
    return {"half_extents": list(half), "is_trigger": trigger, "layer": 1,
            "local_offset": list(offset if offset is not None else (0.0, half[1], 0.0)), "show_gizmos": False}


class SceneBuilder:
    def __init__(self, source):
        self.source = source
        self.entities = []

    def find_source(self, name):
        return next(i for i, e in enumerate(self.source) if e["tag"]["name"] == name)

    def source_children(self, index):
        return [i for i, e in enumerate(self.source) if e.get("parent", {}).get("index") == index]

    def add(self, entity, parent=None, bone=None):
        if parent is not None:
            entity["parent"] = {"index": parent, "bone_name": bone or ""}
        self.entities.append(entity)
        return len(self.entities) - 1

    def copy_tree(self, index, parent=None, skip=()):
        entity = copy.deepcopy(self.source[index])
        if parent is None:
            entity.pop("parent", None)
        else:
            entity["parent"]["index"] = parent
        self.entities.append(entity)
        new = len(self.entities) - 1
        for child in self.source_children(index):
            if self.source[child]["tag"]["name"] not in skip:
                self.copy_tree(child, new, skip)
        return new

    def copy_named(self, name, parent=None, skip=()):
        return self.copy_tree(self.find_source(name), parent, skip)

    def get(self, index):
        return self.entities[index]


# ========== Pieces of the level ==========

def piece(builder, name, model, position, yaw=0.0, half=None, offset=None, scale=1.0):
    entity = named(name, position, yaw, scale, model={"is_visible": True, "model_name": model})
    if half:
        entity["box_collider"] = collider(half, offset)
    return builder.add(entity)


def wall_line(builder, start, end, model="kit_wall", height=3.0, name="Wall"):
    """4 m wall pieces from 'start' to 'end' (x, z), turned along the line"""
    (x0, z0), (x1, z1) = start, end
    length = math.hypot(x1 - x0, z1 - z0)
    count = max(1, round(length / 4.0))
    yaw = math.degrees(math.atan2(x1 - x0, z1 - z0)) - 90.0
    for i in range(count):
        t = (i + 0.5) / count
        piece(builder, name, model, (x0 + (x1 - x0) * t, 0.0, z0 + (z1 - z0) * t), yaw, (2.0, height, 0.5))


def floor_slabs(builder, x_from, x_to, z_from, z_to):
    x = x_from + 4.0
    while x < x_to:
        z = z_from + 4.0
        while z < z_to:
            piece(builder, "Floor Slab", "kit_floor", (x, 0.0, z))
            z += 8.0
        x += 8.0


def invisible_wall(builder, center, half):
    builder.add(named("Border", center, box_collider=collider(half, (0.0, 0.0, 0.0))))


def brazier(builder, position):
    piece(builder, "Brazier", "arena_brazier", position, half=(0.4, 0.85, 0.4))
    fire = copy.deepcopy(builder.source[builder.find_source("Arena Fire")])
    fire["transform"] = transform((position[0], 1.6, position[2]))
    builder.add(fire)


def violet_flame(builder, position, name="Violet Flame"):
    """The dead king's cold fire: a violet brazier flame with its light"""
    builder.add(named(name, (position[0], 1.6, position[2]), particle_emitter={
        "is_emitting": True, "particles_per_second": 60.0, "lifetime_seconds": 0.8, "spawn_half_extents": [0.2, 0.05, 0.2],
        "velocity": [0.0, 1.4, 0.0], "velocity_randomness": 0.3, "gravity": [0.0, 0.9, 0.0], "start_size": 0.34,
        "end_size": 0.04, "start_color": [0.75, 0.4, 1.0], "end_color": [0.15, 0.02, 0.3], "intensity": 1.8, "max_particles": 120}))
    builder.add(named(name + " Light", (position[0], 2.0, position[2]), point_light={
        "attenuation": {"attenuation_constant": 1.0, "attenuation_linear": 0.09, "attenuation_quadratic": 0.032},
        "color": {"ambient": [0.02, 0.01, 0.03], "diffuse": [0.62, 0.35, 1.0], "specular": [0.5, 0.3, 0.8]}}))
    piece(builder, "Brazier", "arena_brazier", position, half=(0.4, 0.85, 0.4))


def add_sky_and_camera(builder):
    moon = builder.copy_named("Moonlight")
    builder.get(moon)["environment"].update(ENVIRONMENT)
    builder.get(moon)["directional_light"].update(MOONLIGHT)
    camera = builder.copy_named("Camera")
    builder.get(camera)["camera"]["far_plane"] = 160.0
    builder.get(camera)["transform"] = transform((PLAYER_START[0], 2.4, PLAYER_START[2] + 4.5), 0.0)
    builder.get(camera)["follow_camera"].update({"distance": 5.2, "pivot_height": 1.75})


def add_effects(builder):
    for name in ("Lost Souls", "Hit Sparks", "Hit Embers", "Hit Blood", "Bone Dust", "Spell Charge", "Fireball Trail",
                 "Fireball Burst", "Pillar Warning", "Pillar Burst", "Cultist Fireball", "Cultist Fire Pillar",
                 "Shockwave Burst"):
        try:
            builder.copy_named(name)
        except StopIteration:
            pass
    # The lost ash waits where the king fell: grey-violet specks instead of green souls
    lost = next(e for e in builder.entities if e["tag"]["name"] == "Lost Souls")
    lost["particle_emitter"].update({"start_color": [0.75, 0.6, 1.0], "end_color": [0.12, 0.08, 0.2]})
    # Posture breaking and the finisher: a ring of white-gold sparks, a burst of the king's violet fire
    builder.add(named("Posture Break", (0.0, -100.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 0.0, "lifetime_seconds": 0.5, "spawn_half_extents": [0.2, 0.3, 0.2],
        "velocity": [0.0, 0.5, 0.0], "velocity_randomness": 4.0, "gravity": [0.0, -2.0, 0.0], "start_size": 0.16,
        "end_size": 0.02, "start_color": [1.0, 0.95, 0.75], "end_color": [1.0, 0.5, 0.1], "intensity": 3.5, "max_particles": 400}))
    builder.add(named("Finisher Burst", (0.0, -100.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 0.0, "lifetime_seconds": 0.9, "spawn_half_extents": [0.3, 0.5, 0.3],
        "velocity": [0.0, 1.5, 0.0], "velocity_randomness": 5.0, "gravity": [0.0, 1.0, 0.0], "start_size": 0.4,
        "end_size": 0.05, "start_color": [0.8, 0.45, 1.0], "end_color": [0.2, 0.02, 0.35], "intensity": 3.0, "max_particles": 600}))
    builder.add(named("Perilous Glint", (0.0, -100.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 0.0, "lifetime_seconds": 0.45, "spawn_half_extents": [0.05, 0.05, 0.05],
        "velocity": [0.0, 0.0, 0.0], "velocity_randomness": 2.5, "gravity": [0.0, 0.0, 0.0], "start_size": 0.5,
        "end_size": 0.05, "start_color": [1.0, 0.12, 0.05], "end_color": [0.4, 0.0, 0.0], "intensity": 4.0, "max_particles": 200}))
    # Ash falling over the plains (around the middle of the map)
    builder.add(named("Falling Ash", (0.0, 8.0, -50.0), particle_emitter={
        "is_emitting": True, "particles_per_second": 60.0, "lifetime_seconds": 14.0, "spawn_half_extents": [60.0, 6.0, 70.0],
        "velocity": [0.4, -0.5, 0.0], "velocity_randomness": 0.3, "gravity": [0.0, 0.0, 0.0], "start_size": 0.05,
        "end_size": 0.03, "start_color": [0.55, 0.5, 0.55], "end_color": [0.2, 0.18, 0.2], "intensity": 0.8, "max_particles": 900}))


# ========== The player: the dead king ==========

def add_player(builder):
    player = builder.copy_named("Player", skip=("Sword", "Shield"))
    entity = builder.get(player)
    entity["model"]["model_name"] = "grave_king"
    entity["model"]["emissive_strength"] = 1.6
    entity["transform"] = transform(PLAYER_START, 180.0)
    actions = entity["character_actions"]
    actions.update({"attack_moves": KING_MOVES, "running_attack_animation": "Attack_Running",
                    "heavy_attack_2_animation": "Attack_Heavy_2", "attack_speed": 1.05,
                    "heavy_damage_multiplier": 1.7, "riposte_damage_multiplier": 3.0, "parry_from": 0.0, "parry_to": 0.35,
                    # the deflect: the block raised just before a blow turns it away (Sekiro's main defence)
                    "deflect_seconds": 0.22, "riposte_animation": "Riposte",
                    "swing_effect": "", "heavy_swing_effect": ""})
    entity["character_animation"].update({"walk_animation_speed": 0.95, "run_animation_speed": 3.3})
    entity["healing_flask"].update({"display_name": "Grave Draught", "hidden_while_drinking": "", "max_charges": 4,
                                    "heal_amount": 70.0})
    entity["currency"].update({"display_name": "Ash", "retrieve_prompt": "Gather your ash"})
    entity["level_up"].update({"health_attribute": "Vigor", "stamina_attribute": "Endurance", "damage_attribute": "Wrath"})
    entity["player_hud"]["death_message"] = "YOU FELL AGAIN"
    entity["health"]["max_health"] = 140.0
    entity["health"]["hit_effect"] = "Hit Embers"
    # The rally (Bloodborne): the health a blow took can be won back by striking back within 3 seconds
    entity["health"].update({"rally_window_seconds": 3.0, "rally_per_damage": 0.5})
    entity["stamina"].update({"max_stamina": 110.0, "light_attack_cost": 18.0, "heavy_attack_cost": 32.0})
    entity["box_collider"]["half_extents"] = [0.34, 1.0, 0.34]
    entity["box_collider"]["local_offset"] = [0.0, 1.0, 0.0]
    entity["lock_on"]["max_distance"] = 20.0
    entity["inventory"] = {}
    # A king's guard holds a long time, but a reckless one breaks: blocks wear his posture, a broken guard is a
    # quarter of his life to a finisher
    entity["posture"] = {"max_posture": 120.0, "from_hits": 0.4, "from_blocks": 0.7, "from_being_parried": 30.0,
                         "recover_per_second": 20.0, "recover_delay_seconds": 1.0, "low_health_recover_factor": 0.6,
                         "broken_seconds": 1.4, "finisher_damage": 0.25, "finisher_slow_motion": 0.0,
                         "finisher_slow_seconds": 0.0, "break_effect": "Posture Break", "finisher_effect": "Hit Blood"}
    # The great sword: no shield - his guard is the blade across (an unseen shield on the left fist takes the blows)
    sword = named("Grave Greatsword", model={"is_visible": True, "model_name": "grave_greatsword"})
    sword["hitbox"] = dict(KING_SWORD_HITBOX, damage=34.0, poise_damage=30.0, layer=PLAYER_HITBOX_LAYER,
                           show_gizmos=False, hit_stop_seconds=0.09, camera_shake=0.25)
    sword["weapon_trail"] = {"base_offset": [0.0, 0.45, 0.0], "tip_offset": [0.0, 1.55, 0.0], "color": [0.72, 0.45, 1.0],
                             "intensity": 1.1, "lifetime_seconds": 0.16, "only_while_striking": True}
    builder.add(sword, parent=player, bone="Weapon_R")
    builder.add(named("Blade Guard", shield={"damage_through": 0.15, "guard_angle_degrees": 120.0, "stamina_per_damage": 1.2,
                                             "block_effect": "Hit Sparks"}), parent=player, bone="Hand_L")
    return player


# ========== Enemies ==========

def enemy_from(builder, source, name, model, position, yaw, display, health, reward, weapon=None):
    skip = {"Cleaver", "Staff", "Spear", "Bronze Shield", "Staff Fire Burst", "Executioner Cleaver", "Cleaver Slam Burst"}
    index = builder.copy_named(source, skip=skip)
    entity = builder.get(index)
    entity["tag"]["name"] = name
    entity["model"]["model_name"] = model
    entity["transform"] = transform(position, yaw)
    entity["enemy_health_bar"]["display_name"] = display
    entity["health"]["max_health"] = health
    entity["currency_reward"]["amount"] = reward
    if weapon:
        weapon_model, hitbox, damage, poise = weapon
        blade = named(name + " Weapon", model={"is_visible": True, "model_name": weapon_model})
        blade["hitbox"] = dict(hitbox, damage=damage, poise_damage=poise, layer=ENEMY_HITBOX_LAYER, show_gizmos=False,
                               hit_stop_seconds=0.07, camera_shake=0.25)
        builder.add(blade, parent=index, bone="Weapon_R")
    return index


def hollow_knight(builder, position, yaw, name="Hollow Knight"):
    index = enemy_from(builder, "Hollow Ghoul", name, "hollow_knight", position, yaw, "Hollow Knight of the Gods", 150.0, 120,
                       ("hollow_knight_sword", KNIGHT_SWORD_HITBOX, 22.0, 22.0))
    entity = builder.get(index)
    entity["character_actions"].update(KNIGHT_WINDOWS)
    entity["character_animation"].update({"walk_animation_speed": 1.37, "run_animation_speed": 3.2})
    entity["box_collider"].update({"half_extents": [0.32, 0.97, 0.32], "local_offset": [0.0, 0.97, 0.0]})
    entity["posture"] = {"max_posture": 70.0, "from_hits": 1.0, "from_being_parried": 25.0, "recover_per_second": 10.0,
                         "broken_seconds": 2.5, "finisher_damage": 1.0, "break_effect": "Posture Break",
                         "finisher_effect": "Finisher Burst"}
    return index


def ash_wolf(builder, position, yaw, name="Ash Wolf"):
    """A gaunt wolf of the ash (the fox's clips of Mesh2Motion): quick bites, a leap from afar, no guard"""
    index = enemy_from(builder, "Hollow Ghoul", name, "ash_wolf", position, yaw, "Ash Wolf", 70.0, 50)
    entity = builder.get(index)
    actions = entity["character_actions"]
    actions.update({"light_attack_1_animation": "Attack_Light_1", "light_attack_2_animation": "Attack_Light_1",
                    "light_attack_3_animation": "Attack_Light_1", "heavy_attack_animation": "Attack_Heavy",
                    "roll_animation": "", "backstep_animation": "", "block_animation": "", "block_impact_animation": "",
                    "parry_animation": "", "parried_animation": "Hit", "hit_reaction_animation": "Hit",
                    "death_animation": "Death", "rest_animation": "", "stand_up_animation": "",
                    "light_hit_from": 0.3, "light_hit_to": 0.62, "heavy_hit_from": 0.35, "heavy_hit_to": 0.65,
                    "attack_reach": 1.3, "attack_lunge_speed": 6.0})
    entity["character_animation"].update({"idle_animation": "Idle", "walk_animation": "Walk", "run_animation": "Run",
                                          "walk_back_animation": "Walk", "strafe_left_animation": "Walk",
                                          "strafe_right_animation": "Walk", "walk_animation_speed": 1.2,
                                          "run_animation_speed": 6.5})
    entity["character_controller"].update({"run_speed": 7.5, "walk_speed": 2.2})
    entity["melee_enemy"].update({"attack_distance": 1.7, "run_distance": 12.0, "max_light_combo": 1,
                                  "heavy_attack_chance": 0.35, "attack_cooldown_seconds": 1.2})
    entity["box_collider"].update({"half_extents": [0.3, 0.55, 0.75], "local_offset": [0.0, 0.55, 0.0]})
    entity["lock_on_target"]["aim_height"] = 0.7
    entity["enemy"]["eye_height"] = 0.9
    entity["posture"] = {"max_posture": 35.0, "from_hits": 1.5, "recover_per_second": 10.0, "broken_seconds": 2.0,
                         "finisher_damage": 1.0, "break_effect": "Posture Break", "finisher_effect": "Finisher Burst"}
    jaws = named(name + " Jaws")
    jaws["hitbox"] = {"damage": 14.0, "poise_damage": 12.0, "local_offset": [0.0, 0.1, 0.0], "half_extents": [0.18, 0.2, 0.25],
                      "layer": ENEMY_HITBOX_LAYER, "show_gizmos": False, "hit_stop_seconds": 0.05, "camera_shake": 0.15}
    builder.add(jaws, parent=index, bone="head")
    return index


def gods_cultist(builder, position, yaw):
    index = builder.copy_named("Ash Cultist", skip={"Staff", "Staff Fire Burst"})
    entity = builder.get(index)
    entity["tag"]["name"] = "Gods' Cultist"
    entity["model"]["model_name"] = "gods_cultist"
    entity["transform"] = transform(position, yaw)
    entity["enemy_health_bar"]["display_name"] = "Masked Cultist"
    entity["character_actions"].update(CULTIST_WINDOWS)
    entity["character_animation"].update({"walk_animation_speed": 1.38, "run_animation_speed": 2.56})
    staff = named("Cultist Staff", model={"is_visible": True, "model_name": "gods_cultist_staff"})
    staff["hitbox"] = dict(STAFF_HITBOX, damage=18.0, poise_damage=15.0, layer=ENEMY_HITBOX_LAYER, show_gizmos=False,
                           hit_stop_seconds=0.06, camera_shake=0.2)
    builder.add(staff, parent=index, bone="Weapon_L")
    entity["posture"] = {"max_posture": 45.0, "from_hits": 1.2, "recover_per_second": 8.0, "broken_seconds": 2.5,
                         "finisher_damage": 1.0, "break_effect": "Posture Break", "finisher_effect": "Finisher Burst"}
    return index


def add_court(builder):
    """The knights of the court standing guard along the hall (not fighting yet: a model and its idle)"""
    for side in (-1.0, 1.0):
        for z in (10.0 - 4.0, 26.0 + 4.0):
            builder.add(named("Court Knight", (side * 3.2, 0.0, z), -90.0 * side,
                              model={"is_visible": True, "model_name": "court_knight", "emissive_strength": 1.0},
                              animator={"current_animation_name": "Idle"}))


def add_boss(builder):
    boss = builder.copy_named("Boss", skip={"Great Axe", "Armor Embers", "Axe Sparks", "Warden Defeat"})
    entity = builder.get(boss)
    entity["tag"]["name"] = "Chain Warden"
    entity["model"]["model_name"] = "hollow_knight"
    entity["transform"] = transform((ARENA_CENTER[0], 0.0, ARENA_CENTER[2] - 10.0), 0.0, 1.45)
    entity["enemy_health_bar"]["display_name"] = "Chain Warden, Hand of the Gods"
    entity["health"]["max_health"] = 1100.0
    entity["currency_reward"]["amount"] = 6000
    entity["character_actions"].update(KNIGHT_WINDOWS)
    # His heavy blows can't be blocked or parried: a red glint warns of them, only a dodge is safe
    entity["character_actions"].update({"heavy_attacks_are_perilous": True, "perilous_warning_effect": "Perilous Glint",
                                        "leap_attack_animation": ""})
    entity["melee_enemy"].update({"leap_attack_distance": 0.0, "attack_distance": 3.2})
    entity["boss"].update({"phase_two_animation": "", "phase_two_light_attack_1_animation": "Attack_Light_1",
                           "phase_two_light_attack_2_animation": "Attack_Light_2",
                           "phase_two_light_attack_3_animation": "Attack_Light_3",
                           "phase_two_heavy_attack_animation": "Attack_Heavy", "defeat_cutscene": "",
                           "phase_two_light_hit_from": 0.33, "phase_two_light_hit_to": 0.56,
                           "phase_two_heavy_hit_from": 0.46, "phase_two_heavy_hit_to": 0.78})
    entity["character_animation"].update({"walk_animation_speed": 1.37 * 1.45, "run_animation_speed": 3.2 * 1.45})
    entity["posture"] = {"max_posture": 260.0, "from_hits": 1.0, "from_blocks": 0.5, "from_being_parried": 45.0,
                         "recover_per_second": 14.0, "recover_delay_seconds": 1.8, "low_health_recover_factor": 0.35,
                         "broken_seconds": 3.0, "finisher_damage": 0.3, "finisher_slow_motion": 0.25,
                         "finisher_slow_seconds": 0.7, "break_effect": "Posture Break", "finisher_effect": "Finisher Burst"}
    blade = named("Warden Blade", model={"is_visible": True, "model_name": "hollow_knight_sword"})
    blade["hitbox"] = dict(KNIGHT_SWORD_HITBOX, damage=34.0, poise_damage=40.0, layer=ENEMY_HITBOX_LAYER, show_gizmos=False,
                           hit_stop_seconds=0.1, camera_shake=0.45)
    builder.add(blade, parent=boss, bone="Weapon_R")
    return boss


# ========== The black horse (Tools/beasts_generator.py) ==========

HORSE_START = (3.0, 0.0, -5.0)


def add_horse(builder):
    """The dead king's black horse waiting by the gate of the crypt: E mounts it (the engine's Mount component)"""
    template = builder.source[builder.find_source("Hollow Ghoul")]
    horse = named("Black Horse", HORSE_START, 180.0,
                  model={"is_visible": True, "model_name": "black_horse", "emissive_strength": 0.0},
                  animator={"current_animation_name": "Idle"},
                  physics_body=copy.deepcopy(template["physics_body"]),
                  box_collider={"half_extents": [0.45, 0.85, 1.15], "is_trigger": False, "layer": 4,
                                "local_offset": [0.0, 0.85, 0.0], "show_gizmos": False},
                  character_controller={"acceleration": 7.0, "facing_offset_degrees": 0.0, "is_player_controlled": False,
                                        "run_speed": 11.0, "turn_speed": 150.0, "walk_speed": 3.2},
                  character_animation={"blend_seconds": 0.3, "idle_animation": "Idle", "run_animation": "Gallop",
                                       "walk_animation": "Walk", "walk_back_animation": "Walk", "strafe_left_animation": "Walk",
                                       "strafe_right_animation": "Walk", "walk_animation_speed": 1.7,
                                       "run_animation_speed": 7.5},
                  mount={"seat_offset": [0.0, 0.62, 0.05], "mount_distance": 2.8, "mount_prompt": "Ride   [E]",
                         "dismount_prompt": "Dismount   [E]", "rider_idle_animation": "Ride_Idle",
                         "rider_walk_animation": "Ride_Walk", "rider_run_animation": "Ride_Gallop",
                         "run_animation_from_speed": 5.0, "camera_rise": 1.0, "camera_pull_back": 2.5})
    horse["physics_body"]["mass"] = 450.0
    return builder.add(horse)


# ========== Books and the court (Tools/books.py, Tools/narrate_books.py) ==========

def wisp(position, color=(1.0, 0.92, 0.7)):
    """The glow of an item lying on the ground, as in souls-likes: a small rising swirl of light"""
    return {"is_emitting": True, "particles_per_second": 28.0, "lifetime_seconds": 1.1, "spawn_half_extents": [0.08, 0.05, 0.08],
            "velocity": [0.0, 0.55, 0.0], "velocity_randomness": 0.25, "gravity": [0.0, 0.3, 0.0], "start_size": 0.12,
            "end_size": 0.02, "start_color": list(color), "end_color": [0.3, 0.2, 0.5], "intensity": 2.4, "max_particles": 60}


def voice_seconds(book_id):
    import wave
    path = PROJECT / "res" / "Sounds" / "Voice" / (book_id + ".wav")
    if not path.exists():
        return 0.0
    with wave.open(str(path)) as audio:
        return audio.getnframes() / audio.getframerate() + 1.0


def pickup(name, position, book_id, title, text):
    return named(name, position, particle_emitter=wisp(position), pickup={
        "item_id": book_id, "display_name": title, "prompt": "Pick up   [E]", "interact_distance": 1.8,
        "read_text": text, "voice_sound": "@game/Sounds/Voice/" + book_id + ".wav", "read_seconds": voice_seconds(book_id),
        "pickup_sound": ""})


def add_books(builder):
    import sys
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from books import BOOKS
    for book_id, (title, where, text) in BOOKS.items():
        if where == "throne":
            builder.add(pickup(title, (1.6, 0.15, THRONE_Z - 2.2), book_id, title, text))
        elif where.startswith("drop:"):
            # A template far below the world, copied where its enemy falls (the engine's Loot Drop)
            template = "Loot " + title
            builder.add(pickup(template, (0.0, -100.0, 0.0), book_id, title, text))
            enemy = next(e for e in builder.entities if e["tag"]["name"] == where[5:] and "health" in e)
            enemy["loot_drop"] = {"pickup_template": template, "chance": 1.0, "only_once": True}


def add_court_npcs(builder):
    """The lich (who reads the found books aloud) and the butler, by the throne"""
    for name, model, position, yaw in (("Court Lich", "court_lich", (-3.2, 0.0, THRONE_Z - 2.5), 200.0),
                                       ("Court Butler", "court_butler", (3.4, 0.0, THRONE_Z - 2.8), 160.0)):
        builder.add(named(name, position, yaw, model={"is_visible": True, "model_name": model},
                          animator={"current_animation_name": "Idle"}))


# ========== The level ==========

NAVE_HALF = 9.0        # the crypt's walls (gothic_kit_generator.py pieces: walls 4 m long, 8 m high)
COLUMN_X = 4.5
BAY_ROWS = (2.0, 10.0, 18.0, 26.0, 34.0)
VAULT_TOP = 7.9        # where the columns' capitals end and the ribs of the vault start


def gothic_wall_line(builder, start, end, yaw, name="Crypt Wall", skip=()):
    """4 m gothic wall pieces from 'start' to 'end' (x, z), their inner face (the blind arches) turned by 'yaw'
    (0: facing -Z, 180: +Z, -90: +X, 90: -X)"""
    (x0, z0), (x1, z1) = start, end
    count = max(1, round(math.hypot(x1 - x0, z1 - z0) / 4.0))
    for i in range(count):
        if i in skip:
            continue
        t = (i + 0.5) / count
        half = (2.0, 4.0, 0.5)
        piece(builder, name, "gc_crypt_wall", (x0 + (x1 - x0) * t, 0.0, z0 + (z1 - z0) * t), yaw, half, (0.0, 4.0, 0.0))


def candles(builder, position, light=True):
    piece(builder, "Candles", "gc_candles", position)
    if light:
        builder.add(named("Candle Light", (position[0], position[1] + 0.9, position[2]), point_light={
            "attenuation": {"attenuation_constant": 1.0, "attenuation_linear": 0.14, "attenuation_quadratic": 0.09},
            "color": {"ambient": [0.01, 0.006, 0.003], "diffuse": [0.75, 0.45, 0.22], "specular": [0.3, 0.2, 0.1]}}))


def add_crypt(builder):
    """The dead king's crypt: a gothic nave of clustered columns under ribbed vaults, aisles of sarcophagi lit by
    candles, the throne on its dais under a stained glass window, the gate south to the plains"""
    builder.add(named("Floor Crypt", (0.0, 0.0, 20.0), box_collider=collider((CRYPT_HALF_WIDTH, 0.15, 20.0), (0.0, -0.15, 0.0))))
    for x in (-8.0, 8.0):
        for z in (8.0, 24.0, 40.0):
            piece(builder, "Crypt Floor", "gc_crypt_floor", (x, 0.0, z))
    w, z0, z1 = NAVE_HALF, CRYPT_FROM_Z, CRYPT_TO_Z
    gothic_wall_line(builder, (-w, z0), (-w, z1), -90.0)
    gothic_wall_line(builder, (w, z0), (w, z1), 90.0)
    # The north wall: the stained glass window in its middle, behind the throne
    for x in (-7.0, -3.5, 3.5, 7.0):
        piece(builder, "Crypt Wall", "gc_crypt_wall", (x, 0.0, z1), 0.0, (2.0, 4.0, 0.5), (0.0, 4.0, 0.0))
    piece(builder, "Stained Glass Window", "gc_stained_window", (0.0, 0.0, z1 - 0.1), 0.0)
    # The window's colored light on the throne and the dais
    builder.add(named("Window Light", (0.0, 5.0, z1 - 3.0), point_light={
        "attenuation": {"attenuation_constant": 1.0, "attenuation_linear": 0.05, "attenuation_quadratic": 0.012},
        "color": {"ambient": [0.03, 0.01, 0.03], "diffuse": [0.85, 0.35, 0.75], "specular": [0.6, 0.3, 0.6]}}))
    # The south wall with the gate: a wall piece over the opening
    for x in (-7.0, -4.0, 4.0, 7.0):
        piece(builder, "Crypt Wall", "gc_crypt_wall", (x, 0.0, z0), 180.0, (2.0, 4.0, 0.5), (0.0, 4.0, 0.0))
    builder.add(named("Gate Lintel", (0.0, 4.6, z0), 180.0, (1.0, 0.45, 1.0), model={"is_visible": True, "model_name": "gc_crypt_wall"}))
    for side in (-1.0, 1.0):
        builder.add(named("Gate Pier", (side * 3.0, 0.0, z0), box_collider=collider((1.0, 4.0, 0.5))))
    # The nave: clustered columns and the ribbed vault over the nave and the aisles
    for z in BAY_ROWS:
        for side in (-1.0, 1.0):
            piece(builder, "Crypt Column", "gc_crypt_column", (side * COLUMN_X, 0.0, z), 0.0, (0.55, 4.0, 0.55), (0.0, 4.0, 0.0))
    bays = list(zip(BAY_ROWS, BAY_ROWS[1:])) + [(BAY_ROWS[-1], z1)]
    for z_from, z_to in bays:
        depth = (z_to - z_from) / 8.0
        middle = (z_from + z_to) * 0.5
        builder.add(named("Crypt Vault", (0.0, VAULT_TOP, middle), 0.0, (COLUMN_X * 2.0 / 8.0, 1.0, depth),
                          model={"is_visible": True, "model_name": "gc_crypt_vault"}))
        for side in (-1.0, 1.0):
            builder.add(named("Aisle Vault", (side * (COLUMN_X + w) * 0.5, VAULT_TOP, middle), 0.0, ((w - COLUMN_X) / 8.0, 0.8, depth),
                              model={"is_visible": True, "model_name": "gc_crypt_vault"}))
    builder.add(named("Gate Vault", (0.0, VAULT_TOP, (z0 + BAY_ROWS[0]) * 0.5), 0.0, (w * 2.0 / 8.0, 1.0, 0.3),
                      model={"is_visible": True, "model_name": "gc_crypt_vault"}))
    # The aisles: the king's fallen knights in their sarcophagi, candles at their heads
    for side in (-1.0, 1.0):
        for z in (6.0, 14.0, 22.0, 30.0):
            piece(builder, "Sarcophagus", "gc_sarcophagus", (side * 6.9, 0.0, z), 90.0, (0.55, 0.6, 1.25), (0.0, 0.6, 0.0))
            candles(builder, (side * 6.9, 0.0, z + 1.75))
    # The throne on its dais, candles and violet fire at its sides
    piece(builder, "Throne Dais", "gc_throne_dais", (0.0, 0.0, THRONE_Z), 180.0, (2.4, 0.3, 1.9), (0.0, 0.3, 0.0))
    piece(builder, "Throne of the Dead King", "arena_throne", (0.0, 0.6, THRONE_Z + 0.4), 180.0, (1.55, 1.7, 1.53))
    for side in (-1.0, 1.0):
        violet_flame(builder, (side * 3.6, 0.0, THRONE_Z - 1.5))
        candles(builder, (side * 2.0, 0.6, THRONE_Z - 1.4), light=False)
        piece(builder, "Knight Statue", "arena_knight_statue", (side * 7.3, 0.0, 37.0), -90.0 * side, (1.22, 1.9, 1.33))
        violet_flame(builder, (side * 6.5, 0.0, 3.0))
    bonfire = builder.copy_named("Shrine Bonfire")
    builder.get(bonfire)["tag"]["name"] = "Crypt Bonfire"
    builder.get(bonfire)["transform"] = transform(CRYPT_BONFIRE)
    builder.get(bonfire)["bonfire"]["respawn_offset"] = [0.0, 0.0, 1.6]
    add_court(builder)


def add_plains(builder, rng):
    # The ground: hills, the old road, mountains at the edges (Tools/terrain_map.py, terrain_generator.py)
    builder.add(named("Ash Plains", (terrain_map.CENTER_X, 0.0, terrain_map.CENTER_Z),
                      model={"is_visible": True, "model_name": "ash_plains"},
                      terrain={"heightmap": "@game/Terrain/plains_height.png", "size_x": terrain_map.SIZE_X,
                               "size_z": terrain_map.SIZE_Z, "height_scale": terrain_map.HEIGHT_SCALE}))
    builder.add(named("Floor Plains", (0.0, 0.0, -60.0), box_collider=collider((PLAINS_HALF_WIDTH, 0.3, 70.0), (0.0, -0.3, 0.0))))
    # The map's edges: unseen walls (the open world comes with the terrain component)
    invisible_wall(builder, (-PLAINS_HALF_WIDTH, 5.0, -60.0), (0.5, 10.0, 70.0))
    invisible_wall(builder, (PLAINS_HALF_WIDTH, 5.0, -60.0), (0.5, 10.0, 70.0))
    invisible_wall(builder, (-38.0, 5.0, 0.0), (26.0, 10.0, 0.5))
    invisible_wall(builder, (38.0, 5.0, 0.0), (26.0, 10.0, 0.5))
    # Ruins of the king's old realm: broken walls and pillars, heaps of rubble, graves of swords
    for _ in range(26):
        x = rng.uniform(-PLAINS_HALF_WIDTH + 6, PLAINS_HALF_WIDTH - 6)
        z = rng.uniform(PLAINS_TO_Z + 8, -8.0)
        if abs(x) < 7.0:  # the old road from the gate south stays clear
            continue
        kind = rng.random()
        yaw = rng.uniform(0.0, 360.0)
        if kind < 0.35:
            piece(builder, "Ruined Wall", "kit_wall_ruined", (x, 0.0, z), yaw, (2.0, 1.3, 0.5))
        elif kind < 0.6:
            piece(builder, "Broken Pillar", "castle_pillar_broken", (x, 0.0, z), yaw, (0.5, 0.9, 0.5))
        elif kind < 0.8:
            piece(builder, "Rubble", "castle_rubble", (x, 0.0, z), yaw)
        else:
            piece(builder, "Sword Graves", "arena_sword_graves", (x, 0.0, z), yaw, (0.99, 0.75, 0.95))
    for x, z, yaw in ((-46.0, -30.0, 30.0), (48.0, -70.0, 200.0), (-40.0, -100.0, 120.0), (52.0, -12.0, 300.0)):
        piece(builder, "Ruined Tower", "arena_corner_tower", (x, 0.0, z), yaw, (3.05, 4.5, 3.64))
    # Camps of the faithful: fires and their guards
    for camp in ((14.0, 0.0, -28.0), (-18.0, 0.0, -78.0), (16.0, 0.0, -96.0)):
        brazier(builder, camp)
    bonfire = builder.copy_named("Gate Bonfire")
    builder.get(bonfire)["tag"]["name"] = "Plains Bonfire"
    builder.get(bonfire)["transform"] = transform(PLAINS_BONFIRE)
    hollow_knight(builder, (12.0, 0.0, -24.0), 180.0)
    hollow_knight(builder, (17.0, 0.0, -31.0), 220.0, "Hollow Knight 2")
    gods_cultist(builder, (10.0, 0.0, -32.0), 160.0)
    hollow_knight(builder, (-16.0, 0.0, -74.0), 40.0, "Hollow Knight 3")
    gods_cultist(builder, (-21.0, 0.0, -81.0), 20.0)
    hollow_knight(builder, (2.0, 0.0, -60.0), 0.0, "Road Knight")
    hollow_knight(builder, (14.0, 0.0, -100.0), 300.0, "Hollow Knight 4")
    gods_cultist(builder, (19.0, 0.0, -93.0), 250.0)
    # A pack of wolves roaming the west of the plains
    for i, (x, z) in enumerate(((-30.0, -40.0), (-33.0, -44.0), (-27.0, -46.0))):
        ash_wolf(builder, (x, 0.0, z), 90.0, "Ash Wolf" if i == 0 else "Ash Wolf {}".format(i + 1))


def add_arena(builder):
    cx, _, cz = ARENA_CENTER
    builder.add(named("Floor Arena", (cx, 0.0, cz), box_collider=collider((ARENA_HALF, 0.15, ARENA_HALF), (0.0, -0.15, 0.0))))
    floor_slabs(builder, cx - 24.0, cx + 24.0, cz - 24.0, cz + 24.0)
    h = ARENA_HALF
    wall_line(builder, (cx - h, cz - h), (cx + h, cz - h))
    wall_line(builder, (cx - h, cz - h), (cx - h, cz + h))
    wall_line(builder, (cx + h, cz - h), (cx + h, cz + h))
    wall_line(builder, (cx - h, cz + h), (cx - 3.6, cz + h))
    wall_line(builder, (cx + 3.6, cz + h), (cx + h, cz + h))
    for sx in (-1.0, 1.0):
        for sz in (-1.0, 1.0):
            piece(builder, "Corner Tower", "arena_corner_tower", (cx + sx * (h + 1.0), 0.0, cz + sz * (h + 1.0)),
                  45.0 + 90.0 * (sx > 0) + 180.0 * (sz > 0), (3.05, 4.5, 3.64))
        brazier(builder, (cx + sx * 12.0, 0.0, cz - 12.0))
        brazier(builder, (cx + sx * 12.0, 0.0, cz + 12.0))
    fog = builder.copy_named("Arena Fog")
    builder.get(fog)["transform"] = transform((cx, 0.0, cz + h))
    builder.get(fog)["fog_gate"].update({"arena_center": list(ARENA_CENTER), "arena_half_extents": [h, 8.0, h],
                                         "cutscene": ""})
    add_boss(builder)


def stand_on_terrain(builder):
    """Everything placed on the plains rises to the ground's height where it stands (the floors, the borders and the
    templates far below the world stay where they are)"""
    keep = ("Floor", "Border", "Ash Plains", "Falling Ash", "Gate Pier", "Gate Lintel")
    half_x, half_z = terrain_map.SIZE_X * 0.5, terrain_map.SIZE_Z * 0.5
    for entity in builder.entities:
        if "parent" in entity or entity["tag"]["name"].startswith(keep):
            continue
        x, y, z = entity["transform"]["position"]
        if z > -0.5 or y < -10.0 or abs(x - terrain_map.CENTER_X) > half_x or abs(z - terrain_map.CENTER_Z) > half_z:
            continue
        entity["transform"]["position"][1] = y + terrain_map.height(x, z)


def copy_shared_assets():
    source = SOURCE_PROJECT / "res"
    target = PROJECT / "res"
    for folder in SHARED_FOLDERS:
        shutil.copytree(source / folder, target / folder, dirs_exist_ok=True)
    for file in SHARED_FILES:
        (target / file).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source / file, target / file)


def main():
    copy_shared_assets()
    source = json.loads(SOURCE_SCENE.read_text(encoding="utf-8"))["entities"]
    builder = SceneBuilder(source)
    rng = random.Random(7)
    add_sky_and_camera(builder)
    add_player(builder)
    add_effects(builder)
    add_crypt(builder)
    add_plains(builder, rng)
    add_arena(builder)
    add_horse(builder)
    add_court_npcs(builder)
    add_books(builder)
    stand_on_terrain(builder)
    TARGET_SCENE.parent.mkdir(parents=True, exist_ok=True)
    TARGET_SCENE.write_text(json.dumps({"entities": builder.entities, "scene_name": "Main"}, indent=1), encoding="utf-8")
    print(f"{len(builder.entities)} entities -> {TARGET_SCENE}")


if __name__ == "__main__":
    main()
