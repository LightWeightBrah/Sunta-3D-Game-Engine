"""
Builds res/Scenes/Main.scene of the sample project 'Strongest in History Vs Today': one boss fight in several phases.

The arena, the effects and (for now) the characters are taken from the sample project MyGame, so the scene is made
by a script instead of by hand: run it again after changing the numbers below.

    python Projects/StrongestInHistoryVsToday/Tools/build_scene.py

The characters are the project's own models (Tools/characters_generator.py): they fight with bare hands and spells.
Their components start as copies of MyGame's player and enemies, then the model, the limbs that hurt and the spells
are set here.
"""

import copy
import json
from pathlib import Path

import scene_effects

PROJECT = Path(__file__).resolve().parents[1]
SOURCE_SCENE = PROJECT.parent / "MyGame" / "res" / "Scenes" / "Main.scene"
TARGET_SCENE = PROJECT / "res" / "Scenes" / "Main.scene"

# ========== Who plays whom (the components come from these characters of MyGame) ==========

PLAYER_MODEL = "sih_player"
BOSS_MODEL = "sih_boss"
SUMMON_MODEL = "sih_summon"

# The collision layers of the limbs that hurt (bit masks, as MyGame's weapons have them)
PLAYER_HITBOX_LAYER = 8
ENEMY_HITBOX_LAYER = 16

SOURCE_PLAYER = "Player"
SOURCE_BOSS = "Ash Cultist"
SOURCE_SUMMON = "Executioner"

PLAYER_NAME = "Player"
BOSS_NAME = "Boss"
SUMMON_NAME = "Summon"

BOSS_DISPLAY_NAME = "Jeffrey Epstein, Strongest in History"
SUMMON_DISPLAY_NAME = "Donald J. Trump, the Summoned"

# The temple of the boss's domain: behind the far wall, under the ground until the script raises it
TEMPLE_Z = -122.0
TEMPLE_SUNKEN_Y = -48.0
TEMPLE_SCALE = 1.2
TEMPLE_LIGHT = {"attenuation": {"attenuation_constant": 1.0, "attenuation_linear": 0.022, "attenuation_quadratic": 0.0019},
                "color": {"ambient": [0.06, 0.03, 0.01], "diffuse": [1.0, 0.7, 0.3], "specular": [1.0, 0.8, 0.4]}}

# ========== The arena ==========

ARENA_CENTER = [0.0, 0.0, -50.0]
# (high: the boss rides the dome of his temple 21 meters up and must still count as inside, or the fog gate takes
# him for gone - it clears itself, plays the victory and stops the music)
ARENA_HALF_EXTENTS = [30.0, 60.0, 29.6]
ARENA_STARTS_AT_Z = -19.5

# Outside the arena, on the street that leads to it: the player walks up to the fog and enters with E
PLAYER_START = [0.0, 0.0, 62.0]
BOSS_START = [0.0, 0.0, -58.0]
# A dormant enemy still has a body: it waits on its own piece of floor far outside the arena
SUMMON_WAITING_PLACE = [0.0, 0.0, -160.0]

# What is taken from MyGame's arena: only what can't be seen as a castle. Its floor's colliders, its walls as
# unseen borders, its fires and their light. The street, the heaps of rubble and the ruins are this project's own
ARENA_PIECES = {"Floor Arena", "Wall", "Arena Light", "Arena Cinders", "Arena Fire", "Outer Ground", "Floor Slab"}

# The models of the ruined city (Tools/props_generator.py)
GROUND_MODEL = "sih_ground"
RUBBLE_MODEL = "sih_rubble"
TOWER_MODELS = ("sih_tower_a", "sih_tower_b", "sih_tower_c")

# Heaps of rubble lie along the border of the arena this far apart (meters), and the ruins of the city stand around it:
# (x, z, turned by degrees, size, which tower)
RUBBLE_EVERY = 9.0
# Over the street to the arena: close on both sides and at its end behind the player
STREET_RUINS = [(-20.0, 22.0, 160.0, 1.15, 0), (18.0, 26.0, 20.0, 1.1, 1)]


def skyline_ruins(placed):
    """Two rings of burnt out towers around the arena, close enough together that no gap opens onto empty land:
    the inner one between the arena and the horizon, the outer one taller behind it. Not on the street to the arena,
    not in front of the temple (it stands behind the far wall), not on a tower already placed"""
    import math
    ruins = []
    for ring, (radius, step, base_size) in enumerate(((78.0, 17.0, 1.15), (135.0, 13.0, 1.6))):
        angle = 0.0
        number = 0
        while angle < 360.0:
            number += 1
            wobble = ((number * 37 + ring * 11) % 9) - 4.0
            distance = radius + ((number * 23 + ring * 7) % 13) - 6.0
            x = ARENA_CENTER[0] + math.sin(math.radians(angle + wobble)) * distance
            z = ARENA_CENTER[2] + math.cos(math.radians(angle + wobble)) * distance
            on_street = abs(x) < 16.0 and -30.0 < z < STREET_FAR_Z + 12.0
            before_temple = abs(x) < 26.0 and z < -90.0 and ring == 0
            crowded = any((x - other[0]) ** 2 + (z - other[1]) ** 2 < 18.0 ** 2 for other in placed + ruins)
            if not (on_street or before_temple or crowded):
                size = base_size + ((number * 31) % 6) * 0.07
                ruins.append((x, z, (number * 67) % 360, size, (number + ring) % 3))
            angle += step
    return ruins
CITY_RUINS = [
    (-52.0, -34.0, 20.0, 1.0, 0), (-60.0, -62.0, 110.0, 1.15, 1), (-48.0, -92.0, 200.0, 0.9, 2), (-78.0, -18.0, 70.0, 1.3, 2),
    (54.0, -30.0, 250.0, 1.1, 1), (62.0, -58.0, 300.0, 0.95, 0), (50.0, -90.0, 150.0, 1.2, 2), (82.0, -84.0, 40.0, 1.35, 0),
    (-22.0, 4.0, 180.0, 1.05, 2), (26.0, 8.0, 10.0, 1.2, 0), (0.0, 96.0, 90.0, 1.4, 1), (-58.0, 14.0, 130.0, 1.25, 0),
    (64.0, 10.0, 220.0, 1.1, 2), (-90.0, -70.0, 15.0, 1.5, 1), (96.0, -40.0, 190.0, 1.45, 1), (-36.0, -150.0, 60.0, 1.4, 0),
    (40.0, -155.0, 280.0, 1.5, 2), (-88.0, -125.0, 330.0, 1.6, 2), (92.0, -130.0, 100.0, 1.6, 0),
]

EFFECTS = [
    "Lost Souls", "Hit Sparks", "Hit Embers", "Hit Blood", "Bone Dust", "Spell Charge", "Fireball Trail",
    "Fireball Burst", "Pillar Warning", "Pillar Burst", "Cultist Fireball", "Cultist Fire Pillar",
]

IDENTITY = [1.0, 0.0, 0.0, 0.0]


class SceneBuilder:
    def __init__(self, source_entities):
        self.source = source_entities
        self.entities = []

    # ----- finding things in the source scene -----

    def find_source(self, name):
        for index, entity in enumerate(self.source):
            if entity["tag"]["name"] == name:
                return index
        raise KeyError(name)

    def source_children(self, parent_index):
        return [i for i, entity in enumerate(self.source) if entity.get("parent", {}).get("index") == parent_index]

    # ----- adding to the new scene -----

    def add(self, entity, parent=None, bone=None):
        if parent is not None:
            entity["parent"] = {"index": parent, "bone_name": bone or ""}
        self.entities.append(entity)
        return len(self.entities) - 1

    def copy_one(self, source_index, parent=None):
        entity = copy.deepcopy(self.source[source_index])
        if parent is None:
            entity.pop("parent", None)
        else:
            entity["parent"]["index"] = parent
        self.entities.append(entity)
        return len(self.entities) - 1

    def copy_tree(self, source_index, parent=None, skip=()):
        """The entity with all its children (a character with its weapon and effects). Returns its new index"""
        new_index = self.copy_one(source_index, parent)
        for child in self.source_children(source_index):
            if self.source[child]["tag"]["name"] not in skip:
                self.copy_tree(child, new_index, skip)
        return new_index

    def child_named(self, parent_index, name):
        for index, entity in enumerate(self.entities):
            if entity.get("parent", {}).get("index") == parent_index and entity["tag"]["name"] == name:
                return index
        raise KeyError(name)

    def get(self, index):
        return self.entities[index]


def transform(position, rotation=(0.0, 0.0, 0.0), scale=(1.0, 1.0, 1.0)):
    # Only rotations around the vertical axis are used below
    import math
    half = math.radians(rotation[1]) * 0.5
    quaternion = [math.cos(half), 0.0, math.sin(half), 0.0] if rotation[1] else IDENTITY
    return {"position": list(position), "rotation": list(rotation), "rotation_quaternion": quaternion, "scale": list(scale)}


def named(name, position=(0.0, 0.0, 0.0), **components):
    entity = {"tag": {"name": name}, "transform": transform(position)}
    entity.update(components)
    return entity


def emitter(name, start_color, end_color, **overrides):
    """A template of specks (not emitting): bursts are thrown from it by name"""
    settings = {
        "is_emitting": False, "particles_per_second": 0.0, "lifetime_seconds": 0.6,
        "spawn_half_extents": [0.1, 0.1, 0.1], "velocity": [0.0, 0.0, 0.0], "velocity_randomness": 2.5,
        "gravity": [0.0, 0.5, 0.0], "start_size": 0.4, "end_size": 0.05,
        "start_color": start_color, "end_color": end_color, "intensity": 3.0, "max_particles": 1500,
    }
    settings.update(overrides)
    return named(name, (0.0, -100.0, 0.0), particle_emitter=settings)


def projectile(name, **settings):
    return named(name, (0.0, -100.0, 0.0), projectile=settings)


def add_projectile(builder, name, body_model=None, bodies=0, body_scale=1.0, body_rotation=None, **settings):
    """A Projectile template, with the models its projectiles carry as they fly (one child for each that can be
    in the air at once, see ProjectileComponent)"""
    template = builder.add(projectile(name, **settings))
    for index in range(bodies):
        body = named(f"{name} Body {index + 1}", model={"is_visible": False, "model_name": body_model})
        body["transform"]["scale"] = [body_scale] * 3
        if body_rotation:
            body["transform"]["rotation"] = list(body_rotation)
            body["transform"]["rotation_quaternion"] = None   # filled by fix_quaternions
        builder.add(body, parent=template)
    return template


def spell(name, **settings):
    entity = named(name, (0.0, 0.1, 0.0), spell_caster=settings)
    return entity


def add_limb(builder, character, name, bone, damage, layer, poise_damage=20.0, size=0.14, camera_shake=0.2):
    """A limb that hurts during the hit window of an attack (as a weapon does): a Hitbox on a bone of the character"""
    builder.add(named(name, hitbox={
        "damage": damage, "half_extents": [size, size, size], "layer": layer, "local_offset": [0.0, 0.08, 0.0],
        "poise_damage": poise_damage, "show_gizmos": False, "hit_stop_seconds": 0.07, "camera_shake": camera_shake}),
        parent=character, bone=bone)


def add_orb_projectile(builder, name, bodies, size, **settings):
    """A Projectile whose body is a ball of clear liquid (a Liquid Orb): the player's baby oil"""
    template = builder.add(projectile(name, **settings))
    for index in range(bodies):
        body = named(f"{name} Body {index + 1}", liquid_orb=dict(OIL_ORB, is_visible=False))
        body["transform"]["scale"] = [size] * 3
        builder.add(body, parent=template)
    return template


def add_eyes(builder, character, name, eyes, color, size):
    """Two spots of light on the head bone where the eyes are (EYE lines of Tools/characters_generator.py): turned on
    by the scripts when a technique is called"""
    for side, position in eyes.items():
        builder.add(named(f"{name} Eye {side}", position, glow_sprite={"is_visible": False, "color": color, "size": size,
                                                                     "intensity": 9.0}), parent=character, bone="Head")


def add_fists_and_foot(builder, character, damage, layer, **settings):
    add_limb(builder, character, "Right Fist", "Hand_R", damage, layer, **settings)
    add_limb(builder, character, "Left Fist", "Hand_L", damage, layer, **settings)
    add_limb(builder, character, "Right Foot", "Foot_R", damage, layer, **settings)


# A fight with bare hands sounds different from one with swords: the air of a punch, knuckles on a body, on forearms
BRAWL_SOUNDS = {"attack_sound": "@game/Sounds/Brawl/Whoosh", "hurt_sound": "@game/Sounds/Brawl/Punch",
                "block_sound": "@game/Sounds/Brawl/Block", "parry_sound": "@game/Sounds/Brawl/Parry"}

SPELL_SOUNDS = "@game/Sounds/Spells/"


def script(file_name):
    return [{"script_path": f"@game/Scripts/{file_name}"}]


# ========== The parts of the scene ==========

def add_sky_and_camera(builder):
    sun = builder.copy_one(builder.find_source("Moonlight"))
    entity = builder.get(sun)
    entity["tag"]["name"] = "Sky"
    # A burning dusk over the ruins (the scripts change it during the fight)
    entity["directional_light"].update({"ambient": [0.2, 0.15, 0.13], "diffuse": [0.95, 0.66, 0.45], "specular": [0.5, 0.4, 0.3]})
    # The sky of the film: heavy clouds lit from below by a city on fire (Poly Haven 'the_sky_is_on_fire', CC0,
    # made into a skybox by Tools/Skybox/panorama_to_cubemap.py)
    entity["environment"].update({"skybox_folder": "@game/Skyboxes/BurningDusk", "fog_color": [0.2, 0.12, 0.09],
                                  "fog_density": 0.012, "sky_brightness": 0.85,
                                  # Heavy blows kick the picture for a blink (an impact frame, see PostProcessing::Kick)
                                  "hit_impact_frame": 0.7})
    entity["transform"] = transform((0.0, 30.0, 0.0), (-38.0, 150.0, 0.0))
    entity["transform"]["rotation_quaternion"] = None  # filled below

    camera = builder.copy_one(builder.find_source("Camera"))
    builder.get(camera)["transform"]["position"] = [PLAYER_START[0], 2.2, PLAYER_START[2] + 4.5]
    builder.get(camera)["camera"]["far_plane"] = 220.0
    # Further back than in MyGame: one of the enemies is three meters tall
    builder.get(camera)["follow_camera"]["distance"] = 5.6


def prop(name, model, position, turned_by=0.0, size=1.0):
    entity = named(name, model={"is_visible": True, "model_name": model})
    entity["transform"] = transform(position, (0.0, turned_by, 0.0), (size, size, size))
    return entity


def add_arena(builder):
    """A street of a city in ruins, as in the film: broken ground, heaps of rubble around the place of the fight
    (the unseen walls of MyGame's arena behind them keep everyone inside), burnt out towers all around"""
    for index, entity in enumerate(builder.source):
        if "parent" in entity or entity["tag"]["name"] not in ARENA_PIECES:
            continue
        if entity["transform"]["position"][2] >= ARENA_STARTS_AT_Z:
            continue

        copied = builder.get(builder.copy_one(index))
        if copied["tag"]["name"] == "Wall":
            # Only its collider is kept: an unseen border
            copied.pop("model", None)
            copied["tag"]["name"] = "Arena Border"
        elif copied.get("model", {}).get("model_name") == "kit_floor":
            copied["model"]["model_name"] = GROUND_MODEL

    add_approach(builder)
    extend_street(builder)

    # Rubble along the four sides, each heap turned a little differently and of its own size
    left, right = ARENA_CENTER[0] - ARENA_HALF_EXTENTS[0] - 1.5, ARENA_CENTER[0] + ARENA_HALF_EXTENTS[0] + 1.5
    near, far = ARENA_CENTER[2] + ARENA_HALF_EXTENTS[2] + 1.5, ARENA_CENTER[2] - ARENA_HALF_EXTENTS[2] - 1.5
    number = 0

    def heap(x, z, along_x):
        nonlocal number
        number += 1
        wobble = ((number * 37) % 21) - 10.0
        size = 1.0 + ((number * 53) % 7) * 0.06
        builder.add(prop("Rubble Heap", RUBBLE_MODEL, (x, -0.2, z), (0.0 if along_x else 90.0) + wobble, size))

    x = left
    while x <= right:
        heap(x, near, True)
        heap(x, far, True)
        x += RUBBLE_EVERY
    z = far + RUBBLE_EVERY * 0.5
    while z < near:
        heap(left, z, False)
        heap(right, z, False)
        z += RUBBLE_EVERY

    for index, (x, z, turned_by, size, tower) in enumerate(CITY_RUINS):
        builder.add(prop("City Ruin", TOWER_MODELS[tower], (x, -1.0, z), turned_by, size))

    floor = copy.deepcopy(builder.source[builder.find_source("Floor Arena")])
    floor.pop("parent", None)
    floor["tag"]["name"] = "Summon Waiting Floor"
    floor["transform"] = transform(SUMMON_WAITING_PLACE)
    builder.add(floor)


def add_approach(builder):
    """The street to the arena (MyGame's courtyard before its boss): the ground of the ruined city, unseen walls on
    both sides hidden behind heaps of rubble, burnt out towers over it, and the fog of the arena at its end"""
    for index, entity in enumerate(builder.source):
        name = entity["tag"]["name"]
        z = entity["transform"]["position"][2]
        if "parent" in entity or not (ARENA_STARTS_AT_Z <= z <= 16.0):
            continue
        if name in ("Floor Courtyard", "Floor Corridor"):
            builder.copy_one(index)
        elif name == "Floor Slab":
            copied = builder.get(builder.copy_one(index))
            copied["model"]["model_name"] = GROUND_MODEL
        elif name == "Wall" and abs(entity["transform"]["position"][0]) == 8.0:
            copied = builder.get(builder.copy_one(index))
            copied.pop("model", None)
            copied["tag"]["name"] = "Street Border"
    # The end of the street behind the player
    back = builder.get(builder.copy_one(builder.find_source("Ruined Gate Blocker")))
    back["tag"]["name"] = "Street End"
    back["transform"]["position"] = [0.0, 0.0, STREET_FAR_Z]
    back["box_collider"]["half_extents"] = [8.0, 3.0, 0.3]
    back.pop("model", None)

    number = 0
    for side in (-1.0, 1.0):
        z = 10.0
        while z > ARENA_STARTS_AT_Z + 2.0:
            number += 1
            wobble = ((number * 41) % 23) - 11.0
            size = 0.9 + ((number * 29) % 5) * 0.08
            builder.add(prop("Street Rubble", RUBBLE_MODEL, (side * 9.5, -0.2, z), 90.0 + wobble, size))
            z -= 7.0
    builder.add(prop("Rubble Heap", RUBBLE_MODEL, (0.0, -0.3, STREET_FAR_Z + 1.5), 4.0, 1.1))
    for x, z, turned_by, size, tower in STREET_RUINS + skyline_ruins(CITY_RUINS + STREET_RUINS):
        builder.add(prop("City Ruin", TOWER_MODELS[tower], (x, -1.0, z), turned_by, size))


def add_effects(builder):
    for name in EFFECTS:
        builder.copy_one(builder.find_source(name))

    # The player's technique is baby oil: everything he throws is amber and glistening, and it drips
    oil, deep_oil = [1.0, 0.78, 0.3], [0.45, 0.22, 0.02]
    builder.add(emitter("Oil Trail", oil, deep_oil, lifetime_seconds=0.4, start_size=0.42, velocity_randomness=0.5,
                        gravity=[0.0, -3.0, 0.0], intensity=3.2))
    builder.add(emitter("Oil Splash", [0.95, 0.97, 1.0], [0.5, 0.5, 0.45], lifetime_seconds=0.55, start_size=0.1, end_size=0.02,
                        velocity_randomness=5.5, gravity=[0.0, -12.0, 0.0], intensity=1.6))
    builder.add(emitter("Oil Charge", oil, deep_oil, lifetime_seconds=0.45, start_size=0.2, velocity_randomness=1.2,
                        gravity=[0.0, -1.5, 0.0]))
    builder.add(emitter("Domain Flash", [1.0, 0.95, 0.75], [0.8, 0.5, 0.1], lifetime_seconds=1.4, start_size=1.6, end_size=0.2,
                        spawn_half_extents=[10.0, 2.0, 10.0], velocity_randomness=9.0, gravity=[0.0, -4.0, 0.0],
                        intensity=4.0, max_particles=2500))
    builder.add(emitter("Summon Burst", [1.0, 0.82, 0.3], [0.5, 0.2, 0.0], lifetime_seconds=1.6, start_size=0.5,
                        spawn_half_extents=[0.6, 1.2, 0.6], velocity=[0.0, 2.5, 0.0], velocity_randomness=3.5, intensity=4.0))
    builder.add(emitter("Temple Dust", [0.75, 0.6, 0.45], [0.1, 0.05, 0.03], lifetime_seconds=2.5, start_size=1.6, end_size=0.4,
                        spawn_half_extents=[9.0, 0.5, 9.0], velocity=[0.0, 1.2, 0.0], velocity_randomness=2.0, intensity=1.2))

    paper, old_paper = [1.0, 0.96, 0.85], [0.5, 0.42, 0.3]
    builder.add(emitter("Page Trail", paper, old_paper, lifetime_seconds=0.25, start_size=0.3, velocity_randomness=0.35, intensity=3.2))
    builder.add(emitter("Page Burst", paper, old_paper, lifetime_seconds=0.6, start_size=0.3, velocity_randomness=5.0,
                        gravity=[0.0, -2.5, 0.0], intensity=3.0))
    builder.add(emitter("Blink Burst", [0.9, 0.2, 0.2], [0.05, 0.0, 0.0], lifetime_seconds=0.5, start_size=0.5, end_size=0.02,
                        spawn_half_extents=[0.35, 0.9, 0.35], velocity_randomness=3.0, intensity=3.5))
    builder.add(emitter("Detonation Trail", [1.0, 0.85, 0.45], [0.6, 0.25, 0.0], lifetime_seconds=0.45, start_size=0.8, end_size=0.1,
                        velocity_randomness=0.7, gravity=[0.0, -2.0, 0.0], intensity=1.5, max_particles=2500))
    builder.add(emitter("Detonation Burst", [1.0, 0.9, 0.6], [0.7, 0.3, 0.0], lifetime_seconds=1.0, start_size=0.55, end_size=0.05,
                        velocity_randomness=10.0, gravity=[0.0, -7.0, 0.0], intensity=1.6, max_particles=2500))
    builder.add(emitter("Detonation Charge", [1.0, 0.85, 0.45], [0.6, 0.25, 0.0], lifetime_seconds=0.5, start_size=0.3, velocity_randomness=1.6))

    # The boss throws what he was known for: hush money (green bills), pages of his black book (pale, sharp),
    # the flashes of his hidden cameras (white rings) and his private jet (fire from above)
    builder.add(emitter("Money Trail", [0.45, 0.95, 0.4], [0.05, 0.3, 0.05], lifetime_seconds=0.3, start_size=0.22, velocity_randomness=0.5, intensity=1.3))
    builder.add(emitter("Money Burst", [0.75, 1.0, 0.5], [0.4, 0.3, 0.0], lifetime_seconds=0.6, start_size=0.2, velocity_randomness=5.5,
                        gravity=[0.0, -3.0, 0.0], intensity=1.4))
    builder.add(emitter("Money Charge", [0.5, 1.0, 0.45], [0.05, 0.3, 0.05], lifetime_seconds=0.4, start_size=0.14, velocity_randomness=1.2, intensity=1.5))
    builder.add(emitter("Flash Trail", [1.0, 1.0, 1.0], [0.5, 0.6, 0.8], lifetime_seconds=0.25, start_size=0.22, velocity_randomness=0.3, intensity=1.6))
    builder.add(emitter("Flash Burst", [1.0, 1.0, 1.0], [0.5, 0.6, 0.8], lifetime_seconds=0.4, start_size=0.3, velocity_randomness=5.0, intensity=2.0))
    builder.add(emitter("Adapt Burst", [1.0, 0.9, 0.5], [0.4, 0.25, 0.0], lifetime_seconds=1.0, start_size=0.45,
                        spawn_half_extents=[0.9, 1.6, 0.9], velocity=[0.0, 1.5, 0.0], velocity_randomness=2.5, intensity=4.0))

    # The fire borrowed from MyGame burns softer here: overlapping, its specks added up to white blobs
    for index, entity in enumerate(builder.entities):
        name = entity["tag"]["name"]
        if name in ("Pillar Warning", "Pillar Burst", "Fireball Trail", "Fireball Burst") and "particle_emitter" in entity:
            entity["particle_emitter"]["intensity"] = 1.1 if name != "Pillar Warning" else 0.9
        if name == "Cultist Fire Pillar":
            entity["projectile"].update({"hit_effect": "Impact Smoke", "hit_burst": 30, "trail_particles_per_second": 40.0,
                                         "hit_sound": "@game/Sounds/Spells/deep_boom.ogg"})
    # Where the strikes from the sky land: a puff of dark smoke and dust that covers what is behind it, not light
    builder.add(named("Impact Smoke", (0.0, -100.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 0.0, "lifetime_seconds": 1.1, "spawn_half_extents": [0.6, 0.2, 0.6],
        "velocity": [0.0, 2.2, 0.0], "velocity_randomness": 1.6, "gravity": [0.0, 0.3, 0.0], "start_size": 0.7,
        "end_size": 1.4, "start_color": [0.13, 0.1, 0.08], "end_color": [0.22, 0.18, 0.15], "intensity": 0.55,
        "max_particles": 350, "is_dark": True}))

    # Where a fist lands: a puff of dust and sweat off the clothes, small and short, not embers or blood
    builder.add(emitter("Punch Dust", [0.75, 0.7, 0.62], [0.25, 0.22, 0.2], lifetime_seconds=0.3, start_size=0.07,
                        end_size=0.02, velocity_randomness=2.2, gravity=[0.0, -3.0, 0.0], intensity=0.7))

    # The swarm that pours out of the temple and gathers into what falls from the sky: dark specks, not light
    builder.add(named("Domain Swarm", (0.0, -100.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 0.0, "lifetime_seconds": 3.2, "spawn_half_extents": [5.0, 1.0, 5.0],
        "velocity": [0.0, 13.0, 4.0], "velocity_randomness": 9.0, "gravity": [0.0, -1.5, 0.0], "start_size": 0.5, "end_size": 0.2,
        "start_color": [0.04, 0.03, 0.03], "end_color": [0.12, 0.08, 0.07], "intensity": 0.9, "max_particles": 3000, "is_dark": True}))
    builder.add(named("Swarm Specks", (0.0, -100.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 0.0, "lifetime_seconds": 0.7, "spawn_half_extents": [0.9, 0.9, 0.9],
        "velocity": [0.0, 0.0, 0.0], "velocity_randomness": 1.2, "gravity": [0.0, 0.0, 0.0], "start_size": 0.14, "end_size": 0.03,
        "start_color": [0.05, 0.04, 0.04], "end_color": [0.1, 0.07, 0.06], "intensity": 0.85, "max_particles": 2500, "is_dark": True}))

    # Red ash falling over the whole arena while the boss's domain stands (turned on by its script)
    ash = named("Domain Ash", (ARENA_CENTER[0], 9.0, ARENA_CENTER[2]), particle_emitter={
        "is_emitting": False, "particles_per_second": 140.0, "lifetime_seconds": 7.0,
        "spawn_half_extents": [28.0, 0.5, 28.0], "velocity": [0.0, -1.3, 0.0], "velocity_randomness": 0.4,
        "gravity": [0.0, 0.0, 0.0], "start_size": 0.09, "end_size": 0.03,
        "start_color": [1.0, 0.18, 0.08], "end_color": [0.25, 0.0, 0.0], "intensity": 2.2, "max_particles": 1200})
    builder.add(ash)

    # A bottle of baby oil, tumbling
    add_orb_projectile(builder, "Player Orb", 4, 0.8, speed=24.0, damage=38.0, poise_damage=35.0, radius=0.55,
                       homing_degrees_per_second=70.0, lifetime_seconds=3.0, fuse_seconds=0.0,
                       trail_effect="", trail_particles_per_second=0.0,
                       hit_effect="Oil Splash", hit_burst=26, hit_sound="@game/Sounds/Spells/glass_break.ogg",
                       hit_stop_seconds=0.08, camera_shake=0.35)
    # The player's domain: one blast under the target that reaches the whole arena
    builder.add(projectile("Player Domain Blast", speed=0.0, damage=130.0, poise_damage=500.0, radius=10.0,
                           homing_degrees_per_second=0.0, lifetime_seconds=2.0, fuse_seconds=0.15,
                           trail_effect="Oil Trail", trail_particles_per_second=300.0,
                           hit_effect="", hit_burst=0, hit_sound="@game/Sounds/Spells/deep_boom.ogg",
                           hit_stop_seconds=0.15, camera_shake=1.0))
    # The player's great spell: slow, wide, and it hits like a truck
    add_projectile(builder, "Player Great Orb", "sih_oil_bottle", 1, 9.0, body_rotation=(-90.0, 0.0, 0.0),
                           body_faces_flight=False, launch_delay_seconds=0.9, spin_degrees_per_second=0.0, speed=7.5,
                           damage=150.0, poise_damage=400.0, radius=3.4,
                           homing_degrees_per_second=0.0, lifetime_seconds=4.0, fuse_seconds=0.0,
                           trail_effect="", trail_particles_per_second=0.0,
                           hit_effect="Oil Cube", hit_burst=60, hit_sound="@game/Sounds/Spells/deep_boom.ogg",
                           hit_stop_seconds=0.2, camera_shake=1.0)
    # The boss's cleave: a fan of blades of light that fly straight and fast (step between them or roll through)
    add_projectile(builder, "Boss Slash", "sih_black_book", 12, 1.5, spin_degrees_per_second=900.0, speed=27.0, damage=18.0, poise_damage=16.0, radius=0.45,
                           homing_degrees_per_second=0.0, lifetime_seconds=1.6, fuse_seconds=0.0,
                           trail_effect="", trail_particles_per_second=0.0,
                           hit_effect="Page Burst", hit_burst=50, hit_sound="@game/Sounds/Brawl/Punch",
                           hit_stop_seconds=0.05, camera_shake=0.25)
    # What the script throws itself: slow orbs in rings around the boss, and lines of fire across the domain
    add_projectile(builder, "Boss Ring Orb", "sih_tape", 48, 1.7, spin_degrees_per_second=500.0, speed=8.5, damage=20.0, poise_damage=18.0, radius=0.45,
                           homing_degrees_per_second=0.0, lifetime_seconds=3.6, fuse_seconds=0.0,
                           trail_effect="", trail_particles_per_second=0.0,
                           hit_effect="Flash Burst", hit_burst=60, hit_sound="@game/Sounds/Brawl/Punch",
                           hit_stop_seconds=0.05, camera_shake=0.25)
    builder.add(projectile("Domain Line Fire", speed=0.0, damage=30.0, poise_damage=30.0, radius=1.15,
                           homing_degrees_per_second=0.0, lifetime_seconds=3.0, fuse_seconds=1.25,
                           trail_effect="Pillar Warning", trail_particles_per_second=28.0,
                           hit_effect="Impact Smoke", hit_burst=22, hit_sound="@game/Sounds/Spells/deep_boom.ogg",
                           hit_stop_seconds=0.06, camera_shake=0.12))
    # The jet that flies over before the fire comes down: it hurts nobody itself (far above everyone's heads)
    add_projectile(builder, "Private Jet", "sih_jet", 3, 1.6, spin_degrees_per_second=0.0, speed=46.0,
                   damage=0.0, poise_damage=0.0, radius=0.1, homing_degrees_per_second=0.0, lifetime_seconds=2.4,
                   fuse_seconds=0.0, trail_effect="Fireball Trail", trail_particles_per_second=140.0,
                   hit_effect="", hit_burst=0, hit_sound="", hit_stop_seconds=0.0, camera_shake=0.0)
    # What rains from the temple when the domain opens: bundles of money, faster than the tapes
    add_projectile(builder, "Domain Rain", "sih_money", 40, 2.0, spin_degrees_per_second=480.0, speed=17.0,
                   launch_delay_seconds=2.4, damage=18.0, poise_damage=14.0, radius=0.5, homing_degrees_per_second=0.0,
                   lifetime_seconds=4.5, fuse_seconds=0.0, trail_effect="Swarm Specks", trail_particles_per_second=26.0,
                   hit_effect="Money Burst", hit_burst=8, hit_sound="@game/Sounds/Spells/coins.ogg", hit_stop_seconds=0.04, camera_shake=0.06)
    # The rain of the domain's opening (film 1:31-1:46): formed high in the sky over the whole arena, hanging there,
    # then falling straight down where they were (not at the player), glowing like meteors (the script lights them)
    add_projectile(builder, "Boss Orb", "sih_money", 24, 1.6, spin_degrees_per_second=540.0, speed=13.0, damage=22.0, poise_damage=18.0, radius=0.5,
                           homing_degrees_per_second=34.0, lifetime_seconds=4.0, fuse_seconds=0.0,
                           trail_effect="", trail_particles_per_second=0.0,
                           hit_effect="Money Burst", hit_burst=10, hit_sound="@game/Sounds/Spells/coins.ogg",
                           hit_stop_seconds=0.05, camera_shake=0.12)


# The film's six-pointed stars (the user's Meshy model) that burst out of the temple's dome, turn around it as a
# sphere, spread over the whole sky and rain down glowing (film 1:28-1:46): one Swarm, drawn all at once
DOMAIN_STARS = 3500

# From the STRIKE and STRIDE lines of Tools/characters_generator.py
# The quick combo (jab, cross, hook, uppercut) and the first kick: generous windows, as the fight was liked
PLAYER_HIT_WINDOWS = {"light_hit_from": 0.24, "light_hit_to": 0.72, "heavy_hit_from": 0.3, "heavy_hit_to": 0.74}
# The other blows each land at their own moment (the STRIKE lines of the fist or foot that hits)
# 'strike_height': where the blow goes by itself (m above his feet, measured with SUNTA_COMBAT_TRACE) - the Strike
# Aim raises the jab and the cross from his crouch to the face, but the hook and the uppercut already rise to it
PLAYER_ATTACK_MOVES = [
    {"animation": "Attack_Light_1", "hit_from": 0.24, "hit_to": 0.72, "strike_height": 1.3},
    {"animation": "Attack_Light_2", "hit_from": 0.24, "hit_to": 0.72, "strike_height": 1.25},
    {"animation": "Attack_Light_3", "hit_from": 0.24, "hit_to": 0.72, "strike_height": 1.5},
    {"animation": "Attack_Light_4", "hit_from": 0.12, "hit_to": 0.48, "damage_multiplier": 1.3, "strike_height": 1.55},
    # (the kicks end late: two heavies pressed fast follow each other instead of blending)
    {"animation": "Attack_Heavy",   "hit_from": 0.30, "hit_to": 0.74, "damage_multiplier": 1.8, "cancel_from": 0.8},
    {"animation": "Attack_Heavy_2", "hit_from": 0.36, "hit_to": 0.66, "damage_multiplier": 2.2, "cancel_from": 0.85},
    {"animation": "Attack_Running", "hit_from": 0.26, "hit_to": 0.52, "damage_multiplier": 1.8},
    {"animation": "Attack_Riposte", "hit_from": 0.18, "hit_to": 0.62, "damage_multiplier": 2.0},
]
PLAYER_MORE_ATTACKS = {
    "extra_light_attack_animations": ["Attack_Light_4"],
    "heavy_attack_2_animation": "Attack_Heavy_2",
    "running_attack_animation": "Attack_Running", "running_attack_speed": 6.5,
    "dodge_attack_animation": "",
    "riposte_animation": "Attack_Riposte", "riposte_reach": 2.6,
    "extra_hit_reaction_animations": ["Hit_2"], "heavy_hit_reaction_animation": "Hit_Heavy",
    "attack_moves": PLAYER_ATTACK_MOVES,
    # A punch steps in to reach, but never slides more than a step
    "attack_lunge_max_distance": 0.9,
    # A roll runs to its end (walking away cut it short and snapped him back to standing)
    "dodge_cancel_from": 0.85,
}
# Aiming the blows at the face / chest of the enemy (the engine's Strike Aim)
PLAYER_STRIKE_AIM = {"spine_bones": "Spine_Low,Spine,Chest", "clip_strike_height": 1.3, "target_height": 0.86,
                     "max_pitch_degrees": 20.0, "max_yaw_degrees": 25.0, "fade_seconds": 0.1, "search_distance": 4.0}
BOSS_STRIKE_AIM = {"spine_bones": "Spine02,Spine,Chest", "clip_strike_height": 1.3, "target_height": 0.8,
                   "max_pitch_degrees": 16.0, "max_yaw_degrees": 25.0, "fade_seconds": 0.1, "search_distance": 4.0}
BOSS_HIT_WINDOWS = {"light_hit_from": 0.25, "light_hit_to": 0.72, "heavy_hit_from": 0.28, "heavy_hit_to": 0.75}
SUMMON_HIT_WINDOWS = {"light_hit_from": 0.15, "light_hit_to": 0.65, "heavy_hit_from": 0.32, "heavy_hit_to": 0.48,
                      "leap_attack_animation": "Attack_Leap", "leap_hit_from": 0.35, "leap_hit_to": 0.55, "leap_speed": 13.0}
# Baby oil: almost clear, a little warm, bright enough to read against a burning sky
OIL_ORB = {"tint": [1.0, 0.97, 0.9], "brightness": 1.9, "wobble": 0.06, "wobble_speed": 2.4, "clearness": 0.2}

# Where the eyes are on the head bone (the EYE lines of Tools/characters_generator.py)
PLAYER_EYES = {"R": (-0.0330, 0.1532, 0.0858), "L": (0.0330, 0.1531, 0.0857)}
BOSS_EYES = {"R": (-0.0297, 0.1647, 0.0917), "L": (0.0359, 0.1696, 0.0873)}

# When the spells leave the hands, as parts of their casting animations (looked up in the previews of the generator)
GREAT_ORB_RELEASE_AT = 0.6
CLEAVE_RELEASE_AT = 0.48

# The speed of the walking clips (meters per second) times the scale of the character
PLAYER_STRIDE = {"walk_animation_speed": 1.62, "run_animation_speed": 2.86}
BOSS_STRIDE = {"walk_animation_speed": 1.36, "run_animation_speed": 2.57}
SUMMON_STRIDE = {"walk_animation_speed": 1.92, "run_animation_speed": 2.82}


def add_domain_spectacle(builder):
    """What the domain is made of (moved and woken by boss_fight.lua): the hill of fire that swells out of the ground
    and turns into the temple (film 1:22-1:25), its smoke, the stars, the dust and the rubble of the boss's flight"""
    builder.add(named("Domain Star Swarm", (0.0, -100.0, 0.0), swarm={
        "is_visible": False, "model_name": "sih_hex_bullet", "count": DOMAIN_STARS, "seed": 7,
        "piece_size": 2.2, "size_variation": 0.5, "radius": 2.0, "shell_thickness": 0.55,
        "spin_degrees_per_second": 26.0, "tumble_degrees_per_second": 70.0,
        "sky_center": [0.0, 36.0, -20.0], "sky_half_extents": [60.0, 12.0, 60.0],
        "fall_speed": 34.0, "fall_spread_seconds": 6.5, "ground_height": 0.0, "stuck_seconds": 6.0,
        "glow_color": [1.3, 0.34, 0.05], "trail_length": 11.0, "hit_effect": "Star Impact", "hit_burst": 2}))

    # Tens of thousands of tongues of fire on a dome that grows (its radius set by the script)
    builder.add(named("Domain Sun", (0.0, -100.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 16000.0, "lifetime_seconds": 1.0,
        "spawn_half_extents": [0.1, 0.1, 0.1], "velocity": [0.0, 1.5, 0.0], "velocity_randomness": 1.2,
        "gravity": [0.0, 2.5, 0.0], "start_size": 2.4, "end_size": 1.1,
        "start_color": [1.0, 0.24, 0.02], "end_color": [0.4, 0.02, 0.0], "intensity": 0.07, "max_particles": 30000,
        "style": 1, "spawn_shape": 2, "spawn_radius": 3.0, "outward_speed": 2.0, "size_randomness": 0.5,
        "spin_degrees_per_second": 50.0}))
    # The smoke rolls out around the foot of the temple, thin: the temple must be seen rising through it
    builder.add(named("Domain Sun Smoke", (0.0, -100.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 80.0, "lifetime_seconds": 3.5,
        "spawn_half_extents": [0.1, 0.1, 0.1], "velocity": [0.0, 2.5, 0.0], "velocity_randomness": 1.5,
        "gravity": [0.0, 0.3, 0.0], "start_size": 4.0, "end_size": 9.0,
        "start_color": [0.09, 0.07, 0.06], "end_color": [0.16, 0.14, 0.13], "intensity": 0.4, "max_particles": 800,
        "style": 2, "spawn_shape": 2, "spawn_radius": 17.0, "outward_speed": 3.0, "size_randomness": 0.4,
        "spin_degrees_per_second": 20.0}))

    # Where a star strikes the ground: a short burst of fire
    builder.add(emitter("Star Impact", [1.0, 0.5, 0.1], [0.4, 0.05, 0.0], lifetime_seconds=0.7, start_size=2.2,
                        end_size=0.8, velocity=[0.0, 3.0, 0.0], velocity_randomness=3.0, gravity=[0.0, 1.0, 0.0],
                        intensity=0.9, max_particles=6000, style=1, size_randomness=0.5,
                        spin_degrees_per_second=90.0))

    # The boss's flight: dust where he skids along the ground (light smoke) and the rubble of what he smashes through
    builder.add(emitter("Skid Dust", [0.62, 0.56, 0.48], [0.5, 0.46, 0.41], lifetime_seconds=1.5, start_size=0.8,
                        end_size=2.2, velocity=[0.0, 1.0, 0.0], velocity_randomness=2.6, gravity=[0.0, 0.2, 0.0],
                        intensity=0.3, max_particles=4000, style=2, size_randomness=0.5,
                        spin_degrees_per_second=30.0, spawn_half_extents=[0.8, 0.2, 0.8]))
    builder.add(named("Crash Rubble", (0.0, -100.0, 0.0), swarm={
        "is_visible": False, "model_name": "sih_rubble", "count": 400, "seed": 3,
        "piece_size": 0.03, "size_variation": 0.7, "radius": 5.0, "shell_thickness": 0.9,
        "spin_degrees_per_second": 0.0, "tumble_degrees_per_second": 400.0,
        "sky_center": [0.0, 0.0, 0.0], "sky_half_extents": [1.0, 1.0, 1.0],
        "fall_speed": 14.0, "fall_spread_seconds": 1.2, "ground_height": 0.0, "stuck_seconds": 3.0,
        "glow_color": [0.0, 0.0, 0.0], "trail_length": 0.0, "hit_effect": "Skid Dust", "hit_burst": 1}))

    # The camera flies through the sphere of stars and out among them (placed by the script where the temple is)
    flight = builder.add(named("Star Flight", cutscene={
        "duration_seconds": 3.6, "look_at_entity": "Star Flight Focus", "look_at_height": 0.0, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0, "can_be_skipped": False,
        "video_file": ""}))
    for index, point in enumerate(((-62.0, 20.0, 52.0), (-38.0, 30.0, 34.0), (-14.0, 42.0, 18.0), (16.0, 48.0, 2.0),
                                   (44.0, 52.0, -16.0))):
        builder.add(named(f"Camera Point {index + 1}", point), parent=flight)
    builder.add(named("Star Flight Focus", (0.0, 26.0, 0.0)))


def add_domain_moments(builder, player):
    """Two moments of the boss's domain seen from where the player stands (the script plays them on the film's beat):
    the sky filling with what is about to fall, then the rain itself throwing the player to the ground"""
    # (the player's front is his own -Z)
    builder.add(named("Domain Sky Focus", (0.0, 24.0, -16.0)), parent=player)
    sky = builder.add(named("Domain Sky", cutscene={
        "duration_seconds": 3.8, "look_at_entity": "Domain Sky Focus", "look_at_height": 0.0, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0, "can_be_skipped": False,
        "video_file": ""}), parent=player)
    # (behind him and to his right, low: the line up to the sky passes beside him, not through his head)
    for index, point in enumerate(((3.2, 1.0, 3.0), (2.4, 0.8, 2.0), (1.8, 0.6, 1.3))):
        builder.add(named(f"Camera Point {index + 1}", point), parent=sky)

    fall = builder.add(named("Meteor Fall", cutscene={
        "duration_seconds": 9.6, "look_at_entity": PLAYER_NAME, "look_at_height": 1.6, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0, "can_be_skipped": False,
        "video_file": ""}), parent=player)
    for index, point in enumerate(((5.5, 1.9, -6.5), (4.8, 1.5, -5.8), (4.2, 1.2, -5.2), (3.8, 1.0, -4.6))):
        builder.add(named(f"Camera Point {index + 1}", point), parent=fall)


def add_player(builder):
    player = builder.copy_tree(builder.find_source(SOURCE_PLAYER), skip=("Sword", "Shield"))
    entity = builder.get(player)
    entity["tag"]["name"] = PLAYER_NAME
    entity["model"]["model_name"] = PLAYER_MODEL
    entity["healing_flask"]["hidden_while_drinking"] = ""
    entity["character_actions"].update(PLAYER_HIT_WINDOWS)
    entity["character_actions"].update(PLAYER_MORE_ATTACKS)
    # Quick hands: the next punch may be asked for early and cuts the end of the one before, and the parry turns
    # blows away for its first third of a second
    entity["character_actions"].update({"input_buffer_from": 0.1, "cancel_from": 0.55, "parry_from": 0.0, "parry_to": 0.5,
                                        "riposte_damage_multiplier": 2.5, "attack_step_speed": 1.4})
    # A boxer who closes in: locked on, each punch steps in to reach (a lunge), and the hands are quicker than a
    # knight's. Every swing cuts an arc of light (see scene_effects.py)
    # The boxing clips are slow motion capture: played faster, so the fist flies as fast as its arc of light
    # (reach 0.85: the step in ends where the fist really touches him - from 1.2 the blows stopped short of his face)
    entity["character_actions"].update({"attack_lunge_speed": 4.5, "attack_reach": 0.85, "attack_speed": 1.45,
                                        "swing_effect": "Player Swing", "heavy_swing_effect": "Player Heavy Swing",
                                        "swing_effect_height": 1.25, "swing_effect_distance": 0.1, "swing_effect_at": 0.3})
    # A souls-like stamina: five or six punches, then the player has to back off, guard or roll and breathe
    # (cheaper since the playtest: a whole combo and a roll fit in one bar, and it comes back quickly)
    entity["stamina"].update({"light_attack_cost": 11.0, "heavy_attack_cost": 22.0, "roll_cost": 12.0, "backstep_cost": 9.0,
                              "regeneration_per_second": 50.0, "regeneration_delay_seconds": 0.5})
    entity["character_animation"].update(PLAYER_STRIDE)
    entity["character_animation"]["locked_on_idle_animation"] = "Idle_Fight"
    # His boxing clips punch from a deep crouch, at the chest of a man of his size: while he attacks the spine
    # straightens and turns a little towards the boss, so jabs and crosses meet the face (measured with
    # SUNTA_COMBAT_TRACE: the fists strike 1.2-1.35 m above his feet, the boss's face is ~1.65 m)
    entity["strike_aim"] = PLAYER_STRIKE_AIM
    entity["character_sounds"].update(BRAWL_SOUNDS)
    entity["script"] = script("player_techniques.lua")
    add_eyes(builder, player, "Player", PLAYER_EYES, [1.0, 0.75, 0.1], 0.045)
    add_domain_moments(builder, player)
    # Down on his back after the stars of the boss's domain: the camera comes over him and waits for the player's
    # answer, [3] (a moment of the cutscene the player plays; boss_fight.lua goes on with 'Technique Rise')
    down = builder.add(named("Down Wait", cutscene={
        "duration_seconds": 3.5, "look_at_entity": PLAYER_NAME, "look_at_height": 0.3, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0,
        "can_be_skipped": False, "video_file": "", "wait_for_key": "2",
        "wait_prompt": "[2]   Imaginary Technique: Baby Oil Detonation"}), parent=player)
    for index, point in enumerate(((3.2, 3.4, 2.4), (2.4, 2.6, 0.2), (1.6, 2.1, -1.6))):
        builder.add(named(f"Camera Point {index + 1}", point), parent=down)
    # Rising from the ground with his hand up for his great technique (started by boss_fight.lua when the player
    # presses [3] lying down): the camera low in front of him, coming up with him (his front is his own -Z)
    rise = builder.add(named("Technique Rise", cutscene={
        "duration_seconds": 2.6, "look_at_entity": PLAYER_NAME, "look_at_height": 1.1, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": "Imaginary Technique: Baby Oil Detonation",
        "title_seconds": 1.6, "can_be_skipped": False, "video_file": ""}), parent=player)
    for index, point in enumerate(((2.4, 0.5, -2.6), (1.6, 1.0, -2.6), (0.7, 1.5, -2.3))):
        builder.add(named(f"Camera Point {index + 1}", point), parent=rise)
    add_domain_spectacle(builder)
    # The ball of oil he holds over his palm while he calls it (shown by his script)
    held = named("Player Held Orb", (0.0, 0.2, 0.0), liquid_orb=dict(OIL_ORB, is_visible=False))
    held["transform"]["scale"] = [0.34] * 3
    builder.add(held, parent=player, bone="Hand_R")
    # His guard: a wall of baby oil before his chest while he blocks, flaring up in the live part of a parry
    # (shown by his script; his front is his own -Z)
    # (mostly see-through: the player watches the boss through it; it brightens only for the parry)
    guard = named("Player Oil Guard", (0.0, 1.3, -0.55), liquid_orb=dict(OIL_ORB, is_visible=False, clearness=0.85,
                                                                          brightness=0.5, wobble=0.1))
    guard["transform"]["scale"] = [1.0, 1.15, 0.22]
    builder.add(guard, parent=player)
    # (a little bigger than a fist: a blow that looks like it lands, lands)
    add_fists_and_foot(builder, player, 26.0, PLAYER_HITBOX_LAYER, size=0.2)
    # The guard of bare forearms: an unseen shield on the left hand (blocking and parrying need one)
    # Bare forearms are no shield: some of every blow comes through, and three or four of his punches break the guard
    builder.add(named("Guard", shield={"damage_through": 0.3, "guard_angle_degrees": 110.0, "stamina_per_damage": 2.4,
                                       "block_effect": "Guard Impact"}), parent=player, bone="Hand_L")
    entity["transform"] = transform(PLAYER_START)
    # Bodies wide enough that two boxers don't stand inside each other
    entity["box_collider"]["half_extents"] = [0.36, 0.9, 0.36]
    entity["health"]["max_health"] = 200.0
    entity["health"]["hit_effect"] = "Punch Impact"
    # A boxer keeps his feet under a jab: only a heavy blow or a few quick ones stagger him
    entity["poise"] = {"max_poise": 30.0, "recover_after_seconds": 3.0, "stagger_immunity_seconds": 1.2}
    entity["healing_flask"].update({"display_name": "Champagne", "max_charges": 4, "heal_amount": 80.0})
    entity["currency"]["display_name"] = "Clout"
    entity["lock_on"]["max_distance"] = 40.0

    builder.add(spell("Player Orb Spell", cast_animation="Cast_Orb", projectile="Player Orb", projectile_count=1,
                      spread_degrees=0.0, release_at=0.66, at_targets_feet=False, min_distance=0.0, max_distance=60.0,
                      cooldown_seconds=2.5, charge_effect="", charge_particles_per_second=0.0,
                      cast_sound="@game/Sounds/Spells/throw.ogg", cast_key="1", display_name="Solid Baby Oil"), parent=player, bone="Hand_R")
    builder.add(spell("Player Great Orb Spell", cast_animation="Cast_Great_Orb", projectile="Player Great Orb", projectile_count=1,
                      spread_degrees=0.0, release_at=GREAT_ORB_RELEASE_AT, at_targets_feet=False, drop_height=13.0,
                      min_distance=0.0, max_distance=60.0,
                      cooldown_seconds=16.0, charge_effect="", charge_particles_per_second=0.0,
                      cast_sound="@game/Sounds/Spells/magic_whoosh.ogg", cast_key="2", display_name="Imaginary Technique: Baby Oil Detonation"), parent=player, bone="Hand_R")
    builder.add(spell("Player Domain Spell", cast_animation="Cast_Domain", projectile="Player Domain Blast", projectile_count=1,
                      spread_degrees=0.0, release_at=0.6, at_targets_feet=True, min_distance=0.0, max_distance=60.0,
                      cooldown_seconds=45.0, charge_effect="", charge_particles_per_second=0.0,
                      cast_sound="@game/Sounds/Boss/roar.ogg", cast_key="3", display_name="Domain Expansion"), parent=player)
    return player


# His ascension (the last phase, boss_fight.lua): his private jet comes down on him and he rises wearing it on his
# back, nose up, its two engines burning under his shoulders. Hidden until then
JET_PACK = {"position": [0.0, 0.05, -0.22], "rotation": [-90.0, 0.0, 0.0], "scale": 0.12}
JET_ENGINES = [[-0.16, -0.45, -0.28], [0.16, -0.45, -0.28]]


def add_jet_pack(builder, boss):
    pack = named("Boss Jet Pack", tuple(JET_PACK["position"]), model={"is_visible": False, "model_name": "sih_jet"})
    pack["transform"]["rotation"] = list(JET_PACK["rotation"])
    pack["transform"]["rotation_quaternion"] = None   # filled by fix_quaternions
    pack["transform"]["scale"] = [JET_PACK["scale"]] * 3
    builder.add(pack, parent=boss, bone="Chest")
    # The flames of the engines: always burning once he has it (a steady jet of fire down, and a glow)
    for index, place in enumerate(JET_ENGINES):
        builder.add(named(f"Boss Jet Flame {index + 1}", tuple(place), particle_emitter={
            "is_emitting": False, "particles_per_second": 120.0, "lifetime_seconds": 0.22,
            "spawn_half_extents": [0.03, 0.03, 0.03], "velocity": [0.0, -5.0, 0.0], "velocity_randomness": 0.4,
            "gravity": [0.0, 0.0, 0.0], "start_size": 0.16, "end_size": 0.03,
            "start_color": [1.0, 0.6, 0.25], "end_color": [0.8, 0.1, 0.02], "intensity": 0.9, "max_particles": 400,
            "style": 1, "size_randomness": 0.3}), parent=boss, bone="Chest")
    # Embers rising off him: the last stand burns
    builder.add(named("Boss Ascension Aura", (0.0, 0.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 70.0, "lifetime_seconds": 1.4,
        "spawn_half_extents": [0.35, 0.8, 0.35], "velocity": [0.0, 1.6, 0.0], "velocity_randomness": 0.8,
        "gravity": [0.0, 0.6, 0.0], "start_size": 0.09, "end_size": 0.01,
        "start_color": [1.0, 0.45, 0.1], "end_color": [0.6, 0.05, 0.0], "intensity": 1.5, "max_particles": 400,
        "style": 0, "size_randomness": 0.5}), parent=boss, bone="Hips")


# ========== The street before the arena: a walk through the ruined city, his men in the way ==========

# The street runs from behind the player's start to the fog of the arena; its floor, its unseen walls and its rubble
# go on past the part copied from MyGame's courtyard
STREET_FAR_Z = 74.0
STREET_FLOORS_Z = (20.0, 36.0, 52.0, 68.0)

# His men (Tools/meshy_enemies.py, characters_generator.py): bodyguards who box, photographers who flash their cameras
# in the player's face and shove. Groups along the street, each a little harder than the one before; the last ones
# stand just before the fog. (name, kind, x, z, turned towards +Z = 180)
STREET_ENEMIES = [
    ("Bodyguard 1", "guard", -1.5, 46.0),
    ("Bodyguard 2", "guard", 2.0, 43.0),
    ("Paparazzo 1", "paparazzo", 4.5, 30.0),
    ("Bodyguard 3", "guard", -2.5, 28.0),
    ("Paparazzo 2", "paparazzo", -5.0, 15.0),
    ("Paparazzo 3", "paparazzo", 5.0, 14.0),
    ("Bodyguard 4", "guard", 0.0, 17.0),
    ("Bodyguard 5", "guard", -2.0, 7.0),
    ("Bodyguard 6", "guard", 2.5, 4.0),
]
GUARD_STRIDE = {"walk_animation_speed": 0.82, "run_animation_speed": 2.07}
ENEMY_HIT_WINDOWS = {"light_hit_from": 0.28, "light_hit_to": 0.7, "heavy_hit_from": 0.3, "heavy_hit_to": 0.72}


def extend_street(builder):
    floor = builder.find_source("Floor Corridor")
    for z in STREET_FLOORS_Z:
        builder.get(builder.copy_one(floor))["transform"]["position"] = [0.0, 0.0, z]
    slab = builder.find_source("Floor Slab")
    z = 24.0
    while z <= STREET_FAR_Z:
        for x in (-4.0, 4.0):
            copied = builder.get(builder.copy_one(slab))
            copied["model"]["model_name"] = GROUND_MODEL
            copied["transform"]["position"] = [x, 0.0, z]
        z += 8.0
    # Unseen walls along the new part (behind the heaps of rubble), and the end of the street behind the start
    wall = builder.find_source("Ruined Gate Blocker")
    for side in (-1.0, 1.0):
        border = builder.get(builder.copy_one(wall))
        border["tag"]["name"] = "Street Border"
        border.pop("model", None)
        border["transform"]["position"] = [side * 8.5, 0.0, 16.0 + (STREET_FAR_Z - 16.0) * 0.5]
        border["box_collider"]["half_extents"] = [0.5, 3.0, (STREET_FAR_Z - 16.0) * 0.5]
    # Fires burning in the rubble along the way, each with its light: the street is lit like the arena
    source_fire, source_light = builder.find_source("Arena Fire"), builder.find_source("Arena Light")
    for x, z in ((-6.0, 57.0), (6.5, 47.0), (-6.5, 33.0), (6.0, 22.0), (-6.0, 9.0)):
        fire = builder.get(builder.copy_one(source_fire))
        fire["tag"]["name"] = "Street Fire Pit"
        fire["transform"]["position"] = [x, fire["transform"]["position"][1], z]
        light = builder.get(builder.copy_one(source_light))
        light["tag"]["name"] = "Street Light"
        light["transform"]["position"] = [x, 2.4, z]
    number = 0
    for side in (-1.0, 1.0):
        z = STREET_FAR_Z - 2.0
        while z > 14.0:
            number += 1
            wobble = ((number * 37) % 23) - 11.0
            size = 0.9 + ((number * 31) % 5) * 0.08
            builder.add(prop("Street Rubble", RUBBLE_MODEL, (side * 9.5, -0.2, z), 90.0 + wobble, size))
            z -= 7.0


def add_street_enemy(builder, name, kind, x, z):
    enemy = builder.copy_tree(builder.find_source("Hollow Ghoul"), skip=("Cleaver",))
    entity = builder.get(enemy)
    entity["tag"]["name"] = name
    entity["model"]["model_name"] = "sih_guard" if kind == "guard" else "sih_paparazzo"
    entity["transform"] = transform((x, 0.0, z), (0.0, 180.0, 0.0))
    entity["character_actions"].update(ENEMY_HIT_WINDOWS)
    entity["character_actions"].update({"attack_lunge_speed": 4.0, "attack_reach": 1.0, "attack_lunge_max_distance": 1.2,
                                        "swing_effect": "Boss Swing", "heavy_swing_effect": "Boss Heavy Swing",
                                        "swing_effect_height": 1.3, "swing_effect_distance": 0.1, "swing_effect_at": 0.3})
    entity["character_animation"].update(GUARD_STRIDE)
    entity["character_sounds"].update(BRAWL_SOUNDS)
    entity["enemy"].update({"sight_distance": 14.0, "hearing_distance": 4.0, "eye_height": 1.7})
    entity["strike_aim"] = dict(BOSS_STRIKE_AIM)
    if kind == "guard":
        entity["health"]["max_health"] = 140.0
        entity["melee_enemy"].update({"attack_distance": 1.6, "attack_cooldown_seconds": 1.1, "heavy_attack_chance": 0.25,
                                      "max_light_combo": 3, "give_up_distance": 15.0})
        entity["currency_reward"] = {"amount": 600}
        add_fists_and_foot(builder, enemy, 16.0, ENEMY_HITBOX_LAYER, poise_damage=18.0, camera_shake=0.2)
    else:
        # Keeps away and flashes the camera (the boss's own "Hidden Cameras" flash, one at a time)
        entity["health"]["max_health"] = 80.0
        entity["melee_enemy"].update({"attack_distance": 1.4, "attack_cooldown_seconds": 1.6, "heavy_attack_chance": 0.1,
                                      "max_light_combo": 2, "give_up_distance": 15.0})
        entity["currency_reward"] = {"amount": 450}
        add_fists_and_foot(builder, enemy, 10.0, ENEMY_HITBOX_LAYER, poise_damage=10.0, camera_shake=0.1)
        hand = builder.add(named(f"{name} Camera Hand"), parent=enemy, bone="Hand_R")
        builder.add(spell(f"{name} Flash Spell", charge_effect="", spread_degrees=0.0, at_targets_feet=False,
                          cast_animation="Cast_Spell", projectile="Boss Ring Orb", projectile_count=1, release_at=0.55,
                          min_distance=3.5, max_distance=16.0, cooldown_seconds=3.5, charge_particles_per_second=0.0,
                          cast_sound="@game/Sounds/Spells/camera_shutter.ogg"), parent=hand)


def add_street_enemies(builder):
    for name, kind, x, z in STREET_ENEMIES:
        add_street_enemy(builder, name, kind, x, z)
    # A fire to rest at halfway (resting brings his men back, as in a souls-like), and one just before the fog
    for name, place in (("Street Fire", (6.0, 0.0, 36.0)), ("Fog Fire", (5.5, 0.0, -10.0))):
        fire = builder.get(builder.copy_tree(builder.find_source("Gate Bonfire")))
        fire["tag"]["name"] = name
        fire["transform"]["position"] = list(place)


def add_boss(builder):
    boss = builder.copy_tree(builder.find_source(SOURCE_BOSS), skip=("Staff", "Staff Fire Burst"))
    entity = builder.get(boss)
    entity["tag"]["name"] = BOSS_NAME
    entity["model"]["model_name"] = BOSS_MODEL
    entity["character_actions"].update(BOSS_HIT_WINDOWS)
    # He steps in to reach with his punches, now and then holds one back for a moment, hops out of the way of blows
    # and orbs and takes punches on his forearms (the script raises his guard)
    entity["character_actions"].update({"shockwave_radius": 0.0, "shockwave_effect": "", "attack_reach": 1.3,
                                        "attack_lunge_speed": 7.0, "attack_delay_chance": 0.15, "attack_delay_seconds": 0.35,
                                        "attack_turn_degrees_per_second": 200.0, "cancel_from": 0.7, "input_buffer_from": 0.1,
                                        "backstep_animation": "Backstep", "roll_animation": "Roll", "backstep_speed": 5.0,
                                        "roll_speed": 6.5, "block_animation": "Block", "block_impact_animation": "Block_Impact"})
    builder.add(named("Guard", shield={"damage_through": 0.2, "guard_angle_degrees": 150.0, "stamina_per_damage": 0.0,
                                       "block_effect": "Guard Impact"}), parent=boss, bone="Hand_L")
    # His blows go through the player's: a punch doesn't stop his swing (he takes it and hits back)
    # (armoured through his heavy blows only: a combo of the player's breaks his light ones)
    entity["character_actions"].update({"hyper_armor": 1, "swing_effect": "Boss Swing", "heavy_swing_effect": "Boss Heavy Swing",
                                        "swing_effect_height": 1.3, "swing_effect_distance": 0.1, "swing_effect_at": 0.3})
    entity["character_animation"].update(BOSS_STRIDE)
    entity["character_sounds"].update(BRAWL_SOUNDS)
    entity["melee_enemy"]["attack_distance"] = 1.7
    entity["strike_aim"] = BOSS_STRIKE_AIM
    add_fists_and_foot(builder, boss, 17.0, ENEMY_HITBOX_LAYER, poise_damage=22.0, camera_shake=0.3)
    entity["transform"] = transform(BOSS_START, (0.0, 180.0, 0.0))
    entity["box_collider"]["half_extents"] = [0.38, 0.95, 0.38]
    entity["health"].update({"max_health": 1500.0, "death_slow_motion_seconds": 3.0, "death_slow_motion_scale": 0.25,
                             "hit_effect": "Punch Impact"})
    # Staggered, he can't be staggered again for 2.5 s: a string of punches doesn't pin him (see also boss_fight.lua)
    entity["poise"] = {"max_poise": 70.0, "recover_after_seconds": 4.0, "stagger_immunity_seconds": 1.6}
    entity["enemy"].update({"sight_distance": 80.0, "hearing_distance": 3.0, "eye_height": 1.7})
    entity["enemy_health_bar"].update({"is_boss": True, "display_name": BOSS_DISPLAY_NAME})
    # Pressure: short breaths between his combos, longer combos (boss_fight.lua speeds him up in later phases)
    entity["melee_enemy"].update({"give_up_distance": 120.0, "run_distance": 7.0, "attack_cooldown_seconds": 1.25,
                                  "heavy_attack_chance": 0.25, "max_light_combo": 3, "finisher_chance": 0.35,
                                  "recover_seconds": 0.7, "dodge_chance": 0.3, "dodge_distance": 3.0,
                                  "dodge_cooldown_seconds": 3.0})
    entity["character_controller"].update({"walk_speed": 2.2, "run_speed": 5.6})
    entity["currency_reward"] = {"amount": 20000}
    # The engine's own second phase is not used: the phases of this fight are in the script
    entity["boss"] = {"phase_two_health_fraction": 0.0, "phase_two_damage_multiplier": 1.0, "phase_two_attack_speedup": 1.0,
                      "defeat_cutscene": "Boss Defeat", "defeat_cutscene_delay": 3.5}
    entity["script"] = script("boss_fight.lua")
    add_eyes(builder, boss, "Boss", BOSS_EYES, [1.0, 1.0, 1.0], 0.1)
    add_jet_pack(builder, boss)

    # The spells leave from his right hand
    staff = builder.add(named("Spell Hand"), parent=boss, bone="Hand_R")
    # No glow gathering in his hand: what he throws is seen leaving it
    common = {"charge_effect": "", "spread_degrees": 0.0, "at_targets_feet": False}
    # What the boss chooses by itself between its blows
    builder.add(spell("Boss Orb Volley Spell", **{**common, "cast_animation": "Cast_Spell", "projectile": "Boss Orb",
                      "projectile_count": 3, "spread_degrees": 13.0, "release_at": 0.5, "min_distance": 4.5,
                      "max_distance": 30.0, "cooldown_seconds": 6.0, "charge_particles_per_second": 90.0,
                      "cast_sound": "@game/Sounds/Spells/cash_register.ogg"}), parent=staff)
    builder.add(spell("Boss Cleave Spell", **{**common, "cast_animation": "Cast_Cleave", "projectile": "Boss Slash",
                      "projectile_count": 5, "spread_degrees": 8.0, "release_at": CLEAVE_RELEASE_AT, "min_distance": 5.0,
                      "max_distance": 30.0, "cooldown_seconds": 9.0, "charge_particles_per_second": 160.0,
                      "cast_sound": "@game/Sounds/Spells/throw.ogg"}), parent=staff)
    builder.add(spell("Boss Fire Pillar Spell", **{**common, "cast_animation": "Cast_Pillars", "projectile": "Cultist Fire Pillar",
                      "projectile_count": 3, "release_at": 0.36, "at_targets_feet": True, "min_distance": 5.0,
                      "max_distance": 30.0, "cooldown_seconds": 12.0, "charge_particles_per_second": 140.0,
                      "cast_sound": ""}), parent=staff)
    # What only the script casts: the great spells of the later phases
    builder.add(spell("Boss Domain Pillars Spell", **{**common, "cast_animation": "Cast_Pillars", "projectile": "Cultist Fire Pillar",
                      "projectile_count": 7, "release_at": 0.36, "at_targets_feet": True, "min_distance": 0.0,
                      "max_distance": 60.0, "cooldown_seconds": 5.0, "charge_particles_per_second": 260.0,
                      "cast_sound": "@game/Sounds/Combat/Shockwave", "cast_on_request_only": True}), parent=staff)
    builder.add(spell("Boss Orb Storm Spell", **{**common, "cast_animation": "Cast_Spell", "projectile": "Boss Orb",
                      "projectile_count": 7, "spread_degrees": 11.0, "release_at": 0.42, "min_distance": 0.0,
                      "max_distance": 60.0, "cooldown_seconds": 5.5, "charge_particles_per_second": 220.0,
                      "cast_sound": "@game/Sounds/Spells/cash_register.ogg", "cast_on_request_only": True}), parent=staff)

    # The sign of his domain, seen from close: the camera comes in on his raised hand
    sign = builder.add(named("Domain Sign", cutscene={
        "duration_seconds": 3.4, "look_at_entity": BOSS_NAME, "look_at_height": 1.55, "animation_entity": BOSS_NAME,
        "animation_name": "Sign_Domain", "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0,
        "can_be_skipped": False, "video_file": ""}), parent=boss)
    # (in front of him: his front is his own -Z)
    for index, point in enumerate(((-1.5, 1.75, -1.9), (-1.1, 1.7, -1.4), (-0.8, 1.68, -1.05))):
        builder.add(named(f"Camera Point {index + 1}", point), parent=sign)

    # The bottle of the player's great technique coming down on him, seen from the ground looking up (film 2:27-2:30):
    # started by the player's script the moment the bottle is called
    fall = builder.add(named("Bottle Fall", cutscene={
        "duration_seconds": 2.3, "look_at_entity": BOSS_NAME, "look_at_height": 4.2, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0,
        "can_be_skipped": False, "video_file": ""}), parent=boss)
    for index, point in enumerate(((-4.5, 0.8, -15.0), (-3.5, 0.7, -14.0), (-2.6, 0.6, -13.0))):
        builder.add(named(f"Camera Point {index + 1}", point), parent=fall)

    # Thrown by the player's last blow before his domain (film 1:15-1:21): the camera flies beside him as he is
    # hurled back through the ruins, skids along the ground and rises with the sign (the points move with him; his
    # back, where he flies, is his own +Z)
    knock = builder.add(named("Knockback Flight", cutscene={
        "duration_seconds": 5.4, "look_at_entity": BOSS_NAME, "look_at_height": 1.0, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0,
        "can_be_skipped": False, "video_file": ""}), parent=boss)
    for index, point in enumerate(((5.5, 1.6, -3.5), (6.5, 1.3, 0.5), (6.0, 1.0, 3.0), (4.5, 0.9, 1.0), (2.2, 1.2, -3.2))):
        builder.add(named(f"Camera Point {index + 1}", point), parent=knock)

    # The camera circles the fallen boss (the points come from MyGame's cutscene, moved to this boss)
    defeat = builder.copy_tree(builder.find_source("Warden Defeat"), parent=boss)
    builder.get(defeat)["tag"]["name"] = "Boss Defeat"
    builder.get(defeat)["cutscene"].update({"look_at_entity": BOSS_NAME, "title": ""})
    return boss


def add_summon(builder):
    summon = builder.copy_tree(builder.find_source(SOURCE_SUMMON), skip=("Executioner Cleaver",))
    entity = builder.get(summon)
    entity["tag"]["name"] = SUMMON_NAME
    entity["model"]["model_name"] = SUMMON_MODEL
    entity["character_actions"].update(SUMMON_HIT_WINDOWS)
    # Both fists into the ground: a shockwave around where they land
    entity["character_actions"].update({"shockwave_radius": 4.5, "shockwave_damage": 42.0, "shockwave_distance": 1.8,
                                        "shockwave_effect": "Cleaver Slam Burst", "shockwave_camera_shake": 0.9,
                                        "attack_reach": 2.8, "attack_turn_degrees_per_second": 120.0,
                                        "hyper_armor": 2, "heavy_swing_effect": "Ground Slam", "swing_effect": "Boss Heavy Swing",
                                        "swing_effect_height": 0.15, "swing_effect_distance": 1.8})
    entity["character_animation"].update(SUMMON_STRIDE)
    entity["character_sounds"].update(BRAWL_SOUNDS)
    entity["character_sounds"]["shockwave_sound"] = "@game/Sounds/Combat/Shockwave"
    entity["melee_enemy"]["attack_distance"] = 3.3
    # It jumps at a player who keeps away
    entity["melee_enemy"]["leap_attack_distance"] = 11.0
    entity["enemy"]["eye_height"] = 2.6
    add_fists_and_foot(builder, summon, 30.0, ENEMY_HITBOX_LAYER, poise_damage=50.0, size=0.38, camera_shake=0.5)
    entity["transform"] = transform(SUMMON_WAITING_PLACE, (0.0, 180.0, 0.0))
    # Three meters tall: a body, senses and blows of that size
    entity["box_collider"].update({"half_extents": [1.05, 1.45, 1.05], "local_offset": [0.0, 1.45, 0.0]})
    entity["lock_on_target"].update({"aim_height": 1.9})
    entity["enemy_health_bar"]["height_above_feet"] = 3.3
    entity["character_sounds"].update({"footstep_meters": 2.4, "footstep_camera_shake": 0.16, "footstep_shake_radius": 12.0, "volume": 1.5})
    entity["health"].update({"max_health": 900.0, "death_slow_motion_seconds": 1.5, "death_slow_motion_scale": 0.35,
                             "hit_effect": "Punch Impact"})
    entity["poise"] = {"max_poise": 140.0, "recover_after_seconds": 4.0, "stagger_immunity_seconds": 3.0}
    entity["enemy"].update({"sight_distance": 80.0, "hearing_distance": 80.0, "starts_dormant": True})
    entity["enemy_health_bar"].update({"is_boss": True, "display_name": SUMMON_DISPLAY_NAME})
    entity["melee_enemy"].update({"give_up_distance": 120.0, "run_distance": 5.0, "attack_cooldown_seconds": 1.3,
                                  "heavy_attack_chance": 0.4, "max_light_combo": 3, "finisher_chance": 0.4})
    entity["character_controller"].update({"walk_speed": 2.0, "run_speed": 5.6})
    entity["currency_reward"] = {"amount": 8000}
    # It adapts to what it is hit with (see the script)
    entity["script"] = script("summon_adaptation.lua")
    return summon


def add_fight(builder):
    # The fog in the gateway, as in a soulslike: the player walks up the street, presses E and steps through; the
    # film's opening plays as it closes behind him
    fog = builder.copy_tree(builder.find_source("Arena Fog"))
    builder.get(fog)["fog_gate"].update({
        "arena_center": ARENA_CENTER, "arena_half_extents": ARENA_HALF_EXTENTS, "is_crossed_with_key": True,
        "cutscene": "Film Intro", "battle_music": "", "victory_message": "STRONGEST OF TODAY",
        "victory_sound": "@game/Sounds/Boss/victory.ogg"})
    builder.get(fog)["transform"]["position"] = [0.0, 0.0, -20.6]

    builder.add(named("Film Intro", cutscene={
        "duration_seconds": 6.0, "look_at_entity": BOSS_NAME, "look_at_height": 1.6, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": BOSS_DISPLAY_NAME, "title_seconds": 3.0,
        "can_be_skipped": True, "video_file": "@game/Videos/intro.mpg"}))

    # The sound of the film's opening: stopped by its script when the video ends or is skipped
    builder.add(named("Film Intro Sound", script=script("film_intro_sound.lua"), audio_source={
        "sound": "@game/Sounds/Film/intro.ogg", "volume": 1.0, "is_looping": False, "play_on_start": False,
        "is_spatial": False, "min_distance": 2.0, "max_distance": 30.0}))

    # The temple of the boss's domain: under the ground behind the far wall until the script raises it
    # (the project's own model, Tools/props_generator.py: its door looks at the arena, it towers over the far wall)
    temple = builder.add(named("Domain Temple", (0.0, TEMPLE_SUNKEN_Y, TEMPLE_Z)))
    building = named("Domain Temple Building", model={"is_visible": True, "model_name": "sih_temple"})
    building["transform"]["scale"] = [TEMPLE_SCALE] * 3
    builder.add(building, parent=temple)
    # Its dome glows over the arena while it stands
    builder.add(named("Domain Temple Light", (0.0, 18.0, 20.0), point_light=TEMPLE_LIGHT), parent=temple)
    # What the camera looks at while it rises: the middle of its walls
    builder.add(named("Domain Temple Heart", (0.0, 15.0, 0.0)), parent=temple)

    # The moment the domain opens: the camera leaves the fight and watches the temple come out of the ground
    # (the script puts it where the temple comes up, turned like the temple: the points are around the temple)
    reveal = builder.add(named("Domain Reveal", cutscene={
        "duration_seconds": 6.2, "look_at_entity": "Domain Reveal Focus", "look_at_height": 0.0, "animation_entity": BOSS_NAME,
        "animation_name": "Sign_Domain", "animation_at_seconds": 0.0, "title": "Domain Expansion: Epstein Island Temple",
        "title_seconds": 3.5, "can_be_skipped": False, "video_file": ""}))

    # Two moments taken from the film itself, as it can't be made in the engine (a face that smiles): his smile as his
    # eyes turn white and he turns black against the blast; and the wheel that turns before the summoning
    builder.add(named("Domain Smile", cutscene={
        "duration_seconds": 2.4, "look_at_entity": "", "look_at_height": 0.0, "animation_entity": "", "animation_name": "",
        "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0, "can_be_skipped": False,
        "video_file": "@game/Videos/domain_smile.mpg"}))
    builder.add(named("Summon Wheel", cutscene={
        "duration_seconds": 3.7, "look_at_entity": "", "look_at_height": 0.0, "animation_entity": "", "animation_name": "",
        "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0, "can_be_skipped": False,
        "video_file": "@game/Videos/summon_wheel.mpg"}))
    # Far and high enough to hold the whole temple (24 m tall) and the boss on its dome in one shot, sweeping slowly
    # around it as it rises (the script aims it between the boss and the middle of the temple)
    for index, point in enumerate(((17.0, 7.0, 41.0), (8.0, 10.0, 40.0), (-1.0, 13.0, 38.0), (-10.0, 16.0, 35.0))):
        builder.add(named(f"Camera Point {index + 1}", point), parent=reveal)
    # What it looks at: where the dome stands once risen (not a part of the temple, which starts deep underground,
    # nor a child of the cutscene, whose children are all points of the camera's path): placed by the script
    builder.add(named("Domain Reveal Focus", (0.0, 13.0, TEMPLE_Z)))


# The shots of the domain's opening and of the player's answer flow into each other as one sequence (film 1:15-1:49):
# each flies in from the shot before ('blend_in_seconds') and lasts until the next one starts (no game camera between)
SEQUENCE_SHOTS = {
    "Knockback Flight": {"duration_seconds": 6.1},
    "Domain Sign":      {"blend_in_seconds": 0.9},
    "Domain Reveal":    {"blend_in_seconds": 1.4, "duration_seconds": 6.5},
    "Star Flight":      {"blend_in_seconds": 1.2, "duration_seconds": 4.2},
    "Meteor Fall":      {"duration_seconds": 10.0},
    "Down Wait":        {"blend_in_seconds": 1.8},
    # (lasts until the bottle's shot takes over: no game camera between his rising and the bottle coming down)
    "Technique Rise":   {"blend_in_seconds": 1.0, "duration_seconds": 4.6},
    "Bottle Fall":      {"blend_in_seconds": 0.6},
}


def join_sequence_shots(entities):
    for entity in entities:
        settings = SEQUENCE_SHOTS.get(entity["tag"]["name"])
        if settings and "cutscene" in entity:
            entity["cutscene"].update(settings)


def fix_quaternions(entities):
    """Euler angles (degrees, as in the Inspector) to the quaternion the engine reads, for the transforms written above"""
    import math
    for entity in entities:
        t = entity["transform"]
        if t.get("rotation_quaternion") is not None:
            continue
        x, y, z = (math.radians(angle) * 0.5 for angle in t["rotation"])
        cx, sx, cy, sy, cz, sz = math.cos(x), math.sin(x), math.cos(y), math.sin(y), math.cos(z), math.sin(z)
        t["rotation_quaternion"] = [cx * cy * cz + sx * sy * sz, sx * cy * cz - cx * sy * sz,
                                    cx * sy * cz + sx * cy * sz, cx * cy * sz - sx * sy * cz]


def main():
    source = json.loads(SOURCE_SCENE.read_text(encoding="utf-8"))["entities"]
    builder = SceneBuilder(source)

    add_sky_and_camera(builder)
    add_player(builder)
    add_arena(builder)
    add_effects(builder)
    scene_effects.add_effect_library(builder)
    scene_effects.upgrade_effects(builder)
    add_boss(builder)
    add_summon(builder)
    add_street_enemies(builder)
    add_fight(builder)
    join_sequence_shots(builder.entities)
    fix_quaternions(builder.entities)

    TARGET_SCENE.parent.mkdir(parents=True, exist_ok=True)
    TARGET_SCENE.write_text(json.dumps({"entities": builder.entities, "scene_name": "Main"}, indent=1), encoding="utf-8")
    print(f"{len(builder.entities)} entities -> {TARGET_SCENE}")


if __name__ == "__main__":
    main()
