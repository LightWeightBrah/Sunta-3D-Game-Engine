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
FOG_GATE_HALF_WIDTH = 3.2

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
# Half of the ground each tower model covers (meters at size 1, x and z of the model: Tools/props_generator.py), and how
# far from the middle of the street nothing of a tower may come (the heaps of rubble along the street end there)
TOWER_HALF_FOOTPRINTS = ((15.8, 15.9), (9.1, 13.2), (16.0, 16.3))
STREET_CLEAR_HALF_WIDTH = 11.0
STREET_CLEAR_FROM_Z, STREET_CLEAR_TO_Z = -22.0, 90.0


# The heaps of rubble along the street: their inner foot (2.55 m from the middle at size 1, up to 1.22 here) ends where
# the unseen walls stop the player (x = +-7.5), so nobody walks into the stones or under them
STREET_HEAP_X = 10.6


def off_the_street(ruin):
    """The tower moved sideways until nothing of it reaches over the street (its rubble at the foot is wide: a tower
    whose middle was off the street still covered the camera at the start and swallowed the enemies of the street)"""
    import math
    x, z, turned_by, size, tower = ruin
    half_x, half_z = TOWER_HALF_FOOTPRINTS[tower]
    angle = math.radians(turned_by)
    reach_x = size * (abs(math.cos(angle)) * half_x + abs(math.sin(angle)) * half_z)
    reach_z = size * (abs(math.sin(angle)) * half_x + abs(math.cos(angle)) * half_z)
    if z + reach_z < STREET_CLEAR_FROM_Z or z - reach_z > STREET_CLEAR_TO_Z or abs(x) - reach_x >= STREET_CLEAR_HALF_WIDTH:
        return ruin
    side = 1.0 if x >= 0.0 else -1.0
    return (side * (STREET_CLEAR_HALF_WIDTH + reach_x), z, turned_by, size, tower)


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

    def find(self, name):
        """The index of an entity already added to the new scene"""
        for index, entity in enumerate(self.entities):
            if entity["tag"]["name"] == name:
                return index
        raise KeyError(name)

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
    # (the player's techniques are uninterruptible: a blow during the wind-up hurts, but the technique goes on)
    entity = named(name, (0.0, 0.1, 0.0), spell_caster=settings)
    return entity


# Meters a kick throws back the one it staggers (the engine's knockback: sliding back on the feet, as in the film's
# fights): the feet throw, the fists only stagger
KICK_KNOCKBACK = 4.0


def add_limb(builder, character, name, bone, damage, layer, poise_damage=20.0, size=0.14, camera_shake=0.2):
    """A limb that hurts during the hit window of an attack (as a weapon does): a Hitbox on a bone of the character"""
    builder.add(named(name, hitbox={
        "damage": damage, "half_extents": [size, size, size], "layer": layer, "local_offset": [0.0, 0.08, 0.0],
        "poise_damage": poise_damage, "show_gizmos": False, "hit_stop_seconds": 0.07, "camera_shake": camera_shake,
        "knockback": KICK_KNOCKBACK if bone.startswith("Foot") else 0.0}),
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


def rubble_heap(position, turned_by, size, name="Rubble Heap"):
    """A heap of rubble with a box the camera can't go through (a trigger: nobody walks into it), so the follow camera
    rises over it instead of coming up to the player's head (the model is 11 x 1.6 x 5.1 m at size 1)"""
    entity = prop(name, RUBBLE_MODEL, position, turned_by, size)
    # (a little bigger than the model: a box smaller than the heap let the camera into its stones)
    entity["box_collider"] = {"local_offset": [0.0, 0.85, 0.0], "half_extents": [5.7, 0.95, 2.75], "is_trigger": True,
                              "layer": 1, "show_gizmos": False, "blocks_camera": True}
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
            # (the camera goes through it into the rubble behind, and rises over the rubble)
            copied["box_collider"]["blocks_camera"] = False
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
        builder.add(rubble_heap((x, -0.2, z), (0.0 if along_x else 90.0) + wobble, size))

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

    for index, (x, z, turned_by, size, tower) in enumerate(map(off_the_street, CITY_RUINS)):
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
            builder.add(rubble_heap((side * STREET_HEAP_X, -0.2, z), 90.0 + wobble, size, "Street Rubble"))
            z -= 7.0
    builder.add(prop("Rubble Heap", RUBBLE_MODEL, (0.0, -0.3, STREET_FAR_Z + 1.5), 4.0, 1.1))
    for x, z, turned_by, size, tower in map(off_the_street, STREET_RUINS + skyline_ruins(CITY_RUINS + STREET_RUINS)):
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
                       hit_effect="Solid Oil Burst", hit_burst=26, hit_sound="@game/Sounds/Spells/glass_break.ogg",
                       hit_stop_seconds=0.1, camera_shake=0.45)
    # The player's domain: one blast under the target that reaches the whole arena
    # (it lands when the two waves of oil meet on the enemy: player_techniques.lua plays them)
    builder.add(projectile("Player Domain Blast", speed=0.0, damage=130.0, poise_damage=500.0, radius=10.0,
                           homing_degrees_per_second=0.0, lifetime_seconds=6.0, fuse_seconds=PLAYER_DOMAIN_CRASH_AT,
                           trail_effect="Oil Trail", trail_particles_per_second=60.0,
                           hit_effect="", hit_burst=0, hit_sound="@game/Sounds/Spells/deep_boom.ogg",
                           hit_stop_seconds=0.15, camera_shake=1.0))
    # The player's great spell: slow, wide, and it hits like a truck
    add_projectile(builder, "Player Great Orb", "sih_oil_bottle", 1, 9.0, body_rotation=(-90.0, 0.0, 0.0),
                           body_faces_flight=False, launch_delay_seconds=0.9, spin_degrees_per_second=0.0, speed=7.5,
                           damage=150.0, poise_damage=400.0, radius=3.4,
                           homing_degrees_per_second=0.0, lifetime_seconds=4.0, fuse_seconds=0.0,
                           trail_effect="", trail_particles_per_second=0.0,
                           # (where it lands the bottle turns into the oil itself: boss_fight.lua 'Detonation Oil')
                           hit_effect="", hit_burst=0, hit_sound="@game/Sounds/Spells/deep_boom.ogg",
                           hit_stop_seconds=0.2, camera_shake=1.0)
    # The bottle become baby oil without its bottle (film 2:28-2:30): a glowing capsule of oil that swells into a ball
    # and bursts like a liquid (shaped and shown by boss_fight.lua)
    detonation = named("Detonation Oil", (0.0, -100.0, 0.0), liquid_orb=dict(OIL_ORB, is_visible=False, clearness=0.12,
                                                                              brightness=2.4, wobble=0.1, wobble_speed=3.5))
    builder.add(detonation)
    # The boss's cleave: a fan of blades of light that fly straight and fast (step between them or roll through)
    add_projectile(builder, "Boss Slash", "sih_black_book", 12, 1.5, spin_degrees_per_second=900.0, speed=27.0, damage=15.0, poise_damage=12.0, radius=0.45,
                           homing_degrees_per_second=0.0, lifetime_seconds=1.6, fuse_seconds=0.0,
                           trail_effect="", trail_particles_per_second=0.0,
                           hit_effect="Page Burst", hit_burst=50, hit_sound="@game/Sounds/Brawl/Punch",
                           hit_stop_seconds=0.05, camera_shake=0.25)
    # What the script throws itself: slow orbs in rings around the boss, and lines of fire across the domain
    add_projectile(builder, "Boss Ring Orb", "sih_tape", 48, 1.7, spin_degrees_per_second=500.0, speed=8.5, damage=15.0, poise_damage=10.0, radius=0.45,
                           homing_degrees_per_second=0.0, lifetime_seconds=3.6, fuse_seconds=0.0,
                           trail_effect="", trail_particles_per_second=0.0,
                           hit_effect="Flash Burst", hit_burst=60, hit_sound="@game/Sounds/Brawl/Punch",
                           hit_stop_seconds=0.05, camera_shake=0.25)
    # The stars of his domain shot straight at the player (film 0:42-0:46): a sign of two fingers, both hands
    # pointing, and a fan of them flies low across the arena, burning (boss_fight.lua O.startStarVolley)
    stars = add_projectile(builder, "Boss Star Bolt", "sih_hex_bullet", 30, 2.0, spin_degrees_per_second=620.0,
                           speed=15.0, damage=13.0, poise_damage=14.0, radius=0.5, homing_degrees_per_second=0.0,
                           lifetime_seconds=3.2, fuse_seconds=0.0, trail_effect="Fireball Trail",
                           trail_particles_per_second=70.0, hit_effect="Star Impact", hit_burst=2,
                           hit_sound="@game/Sounds/Brawl/Punch", hit_stop_seconds=0.04, camera_shake=0.2)
    for entity in builder.entities:
        if entity["tag"]["name"].startswith("Boss Star Bolt Body"):
            entity["model"]["emissive_strength"] = 2.5
    builder.add(projectile("Domain Line Fire", speed=0.0, damage=30.0, poise_damage=30.0, radius=1.15,
                           homing_degrees_per_second=0.0, lifetime_seconds=3.0, fuse_seconds=1.25,
                           trail_effect="Pillar Warning", trail_particles_per_second=28.0,
                           hit_effect="Impact Smoke", hit_burst=22, hit_sound="@game/Sounds/Spells/deep_boom.ogg",
                           hit_stop_seconds=0.06, camera_shake=0.12))
    # The jet that flies over before the fire comes down: it hurts nobody itself (far above everyone's heads)
    add_projectile(builder, "Private Jet", "sih_jet", 3, 2.6, spin_degrees_per_second=0.0, speed=30.0,
                   damage=0.0, poise_damage=0.0, radius=0.1, homing_degrees_per_second=0.0, lifetime_seconds=3.6,
                   fuse_seconds=0.0, trail_effect="Fireball Trail", trail_particles_per_second=140.0,
                   hit_effect="", hit_burst=0, hit_sound="", hit_stop_seconds=0.0, camera_shake=0.0)
    # What rains from the temple when the domain opens: bundles of money, faster than the tapes
    add_projectile(builder, "Domain Rain", "sih_money", 40, 2.0, spin_degrees_per_second=480.0, speed=17.0,
                   launch_delay_seconds=2.4, damage=12.0, poise_damage=6.0, radius=0.5, homing_degrees_per_second=0.0,
                   lifetime_seconds=4.5, fuse_seconds=0.0, trail_effect="Swarm Specks", trail_particles_per_second=26.0,
                   hit_effect="Money Burst", hit_burst=8, hit_sound="@game/Sounds/Spells/coins.ogg", hit_stop_seconds=0.04, camera_shake=0.06)
    # The rain of the domain's opening (film 1:31-1:46): formed high in the sky over the whole arena, hanging there,
    # then falling straight down where they were (not at the player), glowing like meteors (the script lights them)
    add_projectile(builder, "Boss Orb", "sih_money", 24, 1.6, spin_degrees_per_second=540.0, speed=13.0, damage=16.0, poise_damage=10.0, radius=0.5,
                           homing_degrees_per_second=16.0, lifetime_seconds=4.0, fuse_seconds=0.0,
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
    # From a jump: the side kick, and the plunge's fists as they come down (its shockwave hurts on its own)
    {"animation": "Jump_Attack",    "hit_from": 0.2, "hit_to": 0.6, "damage_multiplier": 1.5, "strike_height": 1.2},
    {"animation": "Jump_Plunge",    "hit_from": 0.4, "hit_to": 0.75, "damage_multiplier": 2.0},
]
PLAYER_MORE_ATTACKS = {
    "extra_light_attack_animations": ["Attack_Light_4"],
    "heavy_attack_2_animation": "Attack_Heavy_2",
    "running_attack_animation": "Attack_Running", "running_attack_speed": 6.5,
    "dodge_attack_animation": "",
    "riposte_animation": "Attack_Riposte", "riposte_reach": 2.6,
    "extra_hit_reaction_animations": ["Hit_2"], "heavy_hit_reaction_animation": "Hit_Heavy",
    "knockback_animation": "Knockback", "knockback_seconds": 0.75, "knockback_dust": "Skid Dust",
    "attack_moves": PLAYER_ATTACK_MOVES,
    # A punch steps in to reach, but never slides more than a step
    "attack_lunge_max_distance": 0.9,
    # A roll runs to its end (walking away cut it short and snapped him back to standing)
    "dodge_cancel_from": 0.85,
    # A stagger can be rolled out of after its first third: a blow doesn't pin him for its whole animation
    "hit_reaction_dodge_from": 0.3,
    # Space jumps; the attack button in the air: a flying kick, Ctrl + it: both fists down into the ground with a
    # shockwave around where he lands (the plunge hangs a moment with the fists up, then drops)
    "jump_animation": "Jump", "jump_attack_animation": "Jump_Attack", "jump_heavy_attack_animation": "Jump_Plunge",
    "jump_speed": 5.2, "jump_move_speed": 4.2, "jump_plunge_speed": 9.0, "jump_plunge_from": 0.42,
    "jump_attack_damage_multiplier": 1.4, "jump_plunge_shockwave_radius": 2.8, "jump_plunge_shockwave_damage": 40.0,
    "jump_plunge_effect": "Crash Impact",
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
# Seconds from his domain's release to the waves of oil meeting on the enemy (player_techniques.lua: WaveCrashAt)
PLAYER_DOMAIN_CRASH_AT = 4.0
OIL_ORB = {"tint": [1.0, 0.97, 0.9], "brightness": 1.9, "wobble": 0.06, "wobble_speed": 2.4, "clearness": 0.2}

# Where the eyes are on the head bone (the EYE lines of Tools/characters_generator.py)
PLAYER_EYES = {"R": (-0.0330, 0.1532, 0.0858), "L": (0.0330, 0.1531, 0.0857)}
BOSS_EYES = {"R": (-0.0297, 0.1647, 0.0917), "L": (0.0359, 0.1696, 0.0873)}

# When the spells leave the hands, as parts of their casting animations (looked up in the previews of the generator)
GREAT_ORB_RELEASE_AT = 0.55
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
        "fall_speed": 34.0, "fall_spread_seconds": 6.5, "ground_height": 0.0, "stuck_seconds": 120.0,
        "glow_color": [1.3, 0.34, 0.05], "trail_length": 11.0, "hit_effect": "Star Impact", "hit_burst": 2}))

    # Tens of thousands of tongues of fire on a dome that grows (its radius set by the script)
    builder.add(named("Domain Sun", (0.0, -100.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 16000.0, "lifetime_seconds": 1.0,
        "spawn_half_extents": [0.1, 0.1, 0.1], "velocity": [0.0, 1.5, 0.0], "velocity_randomness": 1.2,
        "gravity": [0.0, 2.5, 0.0], "start_size": 2.4, "end_size": 1.1,
        # (molten orange, not white: tens of thousands of them added up burnt white at the old strength)
        "start_color": [1.0, 0.3, 0.02], "end_color": [0.55, 0.05, 0.0], "intensity": 0.022, "max_particles": 30000,
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
                        spin_degrees_per_second=30.0, spawn_half_extents=[0.8, 0.2, 0.8], **scene_effects.SMOKE_FLIPBOOK))
    builder.add(named("Crash Rubble", (0.0, -100.0, 0.0), swarm={
        "is_visible": False, "model_name": "sih_rubble", "count": 400, "seed": 3,
        "piece_size": 0.03, "size_variation": 0.7, "radius": 5.0, "shell_thickness": 0.9,
        "spin_degrees_per_second": 0.0, "tumble_degrees_per_second": 400.0,
        "sky_center": [0.0, 0.0, 0.0], "sky_half_extents": [1.0, 1.0, 1.0],
        "fall_speed": 14.0, "fall_spread_seconds": 1.2, "ground_height": 0.0, "stuck_seconds": 3.0,
        "glow_color": [0.0, 0.0, 0.0], "trail_length": 0.0, "hit_effect": "Skid Dust", "hit_burst": 1}))

    # The stars burst out of the dome seen from behind the player (placed by the script where he stands, its +Z away
    # from the temple, far enough back for him whole below the temple): he and the whole temple in one shot, the camera drawing slowly back (no flight over the arena)
    flight = builder.add(named("Star Flight", cutscene={
        "duration_seconds": 3.6, "look_at_entity": "Star Flight Focus", "look_at_height": 0.0, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0, "can_be_skipped": False,
        "video_file": ""}))
    for index, point in enumerate(((3.0, 4.0, 19.0), (2.6, 4.4, 21.0), (2.2, 4.8, 23.0))):
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


# The player's domain, Divine Baby Oil Evolve (film 2:19-2:26, played by player_techniques.lua): baby oil over the
# whole floor of the arena (pools of clear liquid, flat, growing out of nothing: x, z, size in meters across), and
# two long waves of it that rise on both sides of the enemy and crash together on him
OIL_POOLS = [(0.0, -50.0, 34.0), (-17.0, -36.0, 18.0), (16.0, -37.0, 20.0), (-18.0, -63.0, 19.0), (17.0, -64.0, 18.0),
             (0.0, -31.0, 16.0), (0.0, -70.0, 18.0), (-23.0, -50.0, 14.0), (23.0, -50.0, 14.0)]
# (clear and glossy, as oil spilt on stone: mostly the floor is seen through it, the sky shines on it)
OIL_DOMAIN_ORB = dict(OIL_ORB, tint=[1.0, 0.95, 0.85], brightness=0.75, clearness=0.86, is_visible=False)
# (flat on the floor it mirrors the sky at every low angle: a dim, warm mirror reads as oil, a bright one as snow)
OIL_POOL_ORB = dict(OIL_DOMAIN_ORB, tint=[1.0, 0.86, 0.66], brightness=0.42, clearness=0.92, wobble=0.02, wobble_speed=0.8)


def add_oil_domain(builder, player):
    for index, (x, z, _) in enumerate(OIL_POOLS):
        pool = named(f"Oil Pool {index + 1}", (x, -0.02, z), liquid_orb=OIL_POOL_ORB)
        pool["transform"]["rotation"] = [0.0, index * 37.0, 0.0]
        pool["transform"]["scale"] = [0.01, 0.01, 0.01]
        builder.add(pool)
    for side in ("L", "R"):
        # (a wall of oil: thin across, tall, long; the script moves it and grows it)
        wave = named(f"Oil Wave {side}", (0.0, -50.0, 0.0), liquid_orb=dict(OIL_DOMAIN_ORB, wobble=0.18, wobble_speed=3.5,
                                                                         clearness=0.35, brightness=1.35))
        wave["transform"]["scale"] = [0.01, 0.01, 0.01]
        index = builder.add(wave)
        # A wall of white foam (puffs of baked smoke, white) and drops flung up out of it
        builder.add(named(f"Oil Wave {side} Spray", (0.0, 0.0, 0.0), particle_emitter={
            "is_emitting": False, "particles_per_second": 700.0, "lifetime_seconds": 1.0,
            "spawn_half_extents": [0.6, 1.4, 6.0], "velocity": [0.0, 5.5, 0.0], "velocity_randomness": 3.0,
            "gravity": [0.0, -9.0, 0.0], "start_size": 0.22, "end_size": 0.05, "start_color": [0.95, 0.97, 1.0],
            "end_color": [0.6, 0.7, 0.85], "intensity": 1.4, "max_particles": 2500}), parent=index)
        builder.add(named(f"Oil Wave {side} Foam", (0.0, 0.0, 0.0), particle_emitter={
            "is_emitting": False, "particles_per_second": 260.0, "lifetime_seconds": 0.7,
            "spawn_half_extents": [0.7, 1.8, 6.5], "velocity": [0.0, 1.5, 0.0], "velocity_randomness": 1.2,
            "gravity": [0.0, 0.0, 0.0], "start_size": 1.6, "end_size": 2.8, "start_color": [0.92, 0.94, 0.97],
            "end_color": [0.75, 0.8, 0.88], "intensity": 0.75, "max_particles": 600, "style": 2,
            "flipbook_texture": "@engine/Textures/Flipbooks/smoke_puff_8x8.png", "flipbook_columns": 8,
            "flipbook_rows": 8, "flipbook_loops": 1.0, "flipbook_random_start": True}), parent=index)

    # The shots (his front is his own -Z): his hands and glowing eyes from close and low, then the whirl of oil
    # around him from afar, then from behind him the waves meeting on the enemy
    shots = (
        ("Domain Gesture", 2.0, PLAYER_NAME, 1.5, 0.4, ((0.6, 0.95, -1.4), (0.45, 1.0, -1.15), (0.32, 1.05, -0.98))),
        ("Domain Vortex", 2.6, PLAYER_NAME, 4.0, 0.5, ((9.0, 1.2, -12.0), (10.5, 2.4, -14.0), (12.0, 3.6, -16.0))),
        # (low behind him and close: the walls of oil rise over the enemy on both sides and crash together)
        ("Domain Waves", 2.8, BOSS_NAME, 2.0, 0.6, ((2.4, 2.0, 6.0), (1.8, 2.6, 7.5), (1.2, 3.2, 9.0))),
        # His answer to the boss's armor (Divine Baby Oil: Crystal Vessel, boss_fight.lua O.startVessel): his hands close
        # and low, beside him as he draws the sword of oil, wide from aside on both of them (the boss before him)
        # (close on his open palm: a bottle of baby oil gathers over it, then the sign and it bursts)
        ("Vessel Bottle", 1.4, PLAYER_NAME, 1.4, 0.3, ((1.1, 1.45, -2.0), (0.8, 1.5, -1.6))),
        ("Vessel Gesture", 1.8, PLAYER_NAME, 1.45, 0.2, ((0.4, 1.45, -1.9), (0.3, 1.5, -1.5))),
        # (the oil crystallizes on him: the camera goes round him by about forty degrees)
        ("Vessel Armor", 2.0, PLAYER_NAME, 1.15, 0.3, ((1.7, 1.2, -2.3), (0.4, 1.3, -2.7))),
        ("Vessel Draw", 1.8, PLAYER_NAME, 1.2, 0.4, ((1.9, 1.25, -1.7), (1.65, 1.35, -1.35))),
        ("Vessel Face-off", 2.2, PLAYER_NAME, 1.3, 0.6, ((6.5, 2.1, -2.4), (7.0, 2.3, -3.0))),
    )
    for name, seconds, target, height, blend, points in shots:
        shot = builder.add(named(name, cutscene={
            "duration_seconds": seconds, "look_at_entity": target, "look_at_height": height, "animation_entity": "",
            "animation_name": "", "animation_at_seconds": 0.0,
            # (the name of his answer over its last shot)
            "title": "Divine Baby Oil: Crystal Vessel" if name == "Vessel Face-off" else "",
            "title_seconds": 1.8 if name == "Vessel Face-off" else 0.0,
            "can_be_skipped": False, "video_file": "", "blend_in_seconds": blend}), parent=player)
        for number, point in enumerate(points):
            builder.add(named(f"Camera Point {number + 1}", point), parent=shot)


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
    # (a fight like the film's: long strings of blows, stamina that comes back fast)
    entity["stamina"].update({"max_stamina": 160.0, "light_attack_cost": 8.0, "heavy_attack_cost": 16.0, "roll_cost": 10.0,
                              "backstep_cost": 7.0, "regeneration_per_second": 80.0, "regeneration_delay_seconds": 0.3})
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
    add_oil_domain(builder, player)
    # Down on his back after the stars of the boss's domain: the camera comes over him and waits for the player's
    # answer, [3] (a moment of the cutscene the player plays; boss_fight.lua goes on with 'Technique Rise')
    down = builder.add(named("Down Wait", cutscene={
        "duration_seconds": 1.6, "look_at_entity": PLAYER_NAME, "look_at_height": 0.3, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0,
        "can_be_skipped": False, "video_file": "", "wait_for_key": "2",
        # (a quick-time event: [2] again and again, quickly, a blow felt with each press, until he is up)
        "wait_presses": 8, "press_window_seconds": 0.5, "press_sound": "@game/Sounds/Brawl/Punch",
        "wait_prompt": "Mash [2] to rise   -   Imaginary Technique: Baby Oil Detonation"}), parent=player)
    # (a cut, high beside him: the stars that threw him down stuck around him, and him; no flight across the arena)
    for index, point in enumerate(((3.4, 3.0, 2.6), (3.0, 2.6, 1.2))):
        builder.add(named(f"Camera Point {index + 1}", point), parent=down)
    # Rising from the ground with his hand up for his great technique (started by boss_fight.lua when the player
    # presses [3] lying down): the camera low in front of him, coming up with him (his front is his own -Z)
    # It ends close on his hands making the sign before his chest and his glowing eyes (film 1:47.5)
    rise = builder.add(named("Technique Rise", cutscene={
        "duration_seconds": 2.6, "look_at_entity": PLAYER_NAME, "look_at_height": 1.0, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": "Imaginary Technique: Baby Oil Detonation",
        "title_seconds": 1.6, "can_be_skipped": False, "video_file": ""}), parent=player)
    # (close and low from the start, rising with him: from farther it looked at the ruins over him lying)
    for index, point in enumerate(((1.6, 1.3, -2.4), (1.4, 1.2, -2.8), (1.1, 1.15, -3.1), (0.8, 1.15, -3.3))):
        builder.add(named(f"Camera Point {index + 1}", point), parent=rise)
    add_domain_spectacle(builder)
    # The bottle of baby oil over his open palm as he answers the boss's armor (boss_fight.lua, Crystal Vessel)
    # (standing upright where his open right palm is, not on the hand bone: it turned with the hand and lay on its side)
    bottle = named("Vessel Bottle", (-0.12, 1.62, -0.36), model={"is_visible": False, "model_name": "sih_oil_bottle"})
    bottle["transform"]["scale"] = [0.42] * 3
    bottle["transform"]["rotation"] = [-90.0, 0.0, 0.0]
    bottle["transform"]["rotation_quaternion"] = None
    builder.add(bottle, parent=player)
    # The ball of oil he holds over his palm while he calls it (shown by his script)
    held = named("Player Held Orb", (0.0, 0.2, 0.0), liquid_orb=dict(OIL_ORB, is_visible=False))
    held["transform"]["scale"] = [0.34] * 3
    builder.add(held, parent=player, bone="Hand_R")
    # His guard: a wall of baby oil before his chest while he blocks, flaring up in the live part of a parry
    # (shown by his script; his front is his own -Z)
    # (mostly see-through: the player watches the boss through it; it brightens only for the parry)
    guard = named("Player Oil Guard", (0.0, 1.3, -0.55), liquid_orb=dict(OIL_ORB, is_visible=False, clearness=0.55,
                                                                          brightness=0.9, wobble=0.03))
    guard["transform"]["scale"] = [1.0, 1.15, 0.5]
    builder.add(guard, parent=player)
    # (a little bigger than a fist: a blow that looks like it lands, lands)
    add_fists_and_foot(builder, player, 26.0, PLAYER_HITBOX_LAYER, size=0.2)
    # His sword of baby oil (Divine Baby Oil: Crystal Vessel, boss_fight.lua O.armPlayer): on the bone in the fist of
    # his sword model, hidden and harmless until then; a narrow trail of warm white light along the outer half of the
    # blade while it strikes (the film's light of his techniques is gold and white, never pink)
    sword = named("Player Sword", (0.0, 0.0, 0.0), model={"is_visible": False, "model_name": "sih_diddy_sword",
                                                         "emissive_strength": 0.0})
    # (the blade from 0.21 to 1.11 m above the grip, boss_items_generator.py; a little wider than the blade)
    sword["hitbox"] = {"damage": 34.0, "poise_damage": 30.0, "local_offset": [0.0, 0.67, 0.0],
                       "half_extents": [0.14, 0.47, 0.1], "layer": PLAYER_HITBOX_LAYER, "show_gizmos": False,
                       "hit_stop_seconds": 0.07, "camera_shake": 0.3, "is_enabled": False,
                       "knockback": 3.0}
    sword["weapon_trail"] = {"base_offset": [0.0, 0.62, 0.0], "tip_offset": [0.0, 1.1, 0.0], "color": [1.0, 0.88, 0.62],
                             "intensity": 1.3, "lifetime_seconds": 0.18, "only_while_striking": True}
    builder.add(sword, parent=player, bone="Weapon_R")
    # The guard of bare forearms: an unseen shield on the left hand (blocking and parrying need one)
    # Bare forearms are no shield: some of every blow comes through, and three or four of his punches break the guard
    builder.add(named("Guard", shield={"damage_through": 0.3, "guard_angle_degrees": 110.0, "stamina_per_damage": 2.4,
                                       "block_effect": "Guard Impact"}), parent=player, bone="Hand_L")
    entity["transform"] = transform(PLAYER_START)
    # Bodies wide enough that two boxers don't stand inside each other
    entity["box_collider"]["half_extents"] = [0.36, 0.9, 0.36]
    # (a long fight in many phases against two bosses at once: room for mistakes, see also the champagne refilled
    # between the phases in boss_fight.lua)
    entity["health"]["max_health"] = 320.0
    entity["health"]["hit_effect"] = "Punch Impact"
    # A boxer keeps his feet under a jab: only a heavy blow or a few quick ones stagger him
    # (a few blows before he staggers, and a long while after it: staggered again and again he could do nothing)
    entity["poise"] = {"max_poise": 110.0, "recover_after_seconds": 1.8, "stagger_immunity_seconds": 3.0}
    # A volley of money or tapes costs one hit, not all of them at once
    entity["health"]["spell_immunity_seconds"] = 0.8
    entity["healing_flask"].update({"display_name": "Champagne", "max_charges": 5, "heal_amount": 110.0})
    entity["currency"]["display_name"] = "Clout"
    entity["lock_on"]["max_distance"] = 40.0

    builder.add(spell("Player Orb Spell", cast_animation="Cast_Orb", projectile="Player Orb", projectile_count=1,
                      spread_degrees=0.0, release_at=0.66, at_targets_feet=False, min_distance=0.0, max_distance=60.0,
                      cooldown_seconds=2.5, charge_effect="", charge_particles_per_second=0.0,
                      cast_sound="@game/Sounds/Spells/throw.ogg", cast_key="1", is_uninterruptible=True, display_name="Solid Baby Oil"), parent=player, bone="Hand_R")
    builder.add(spell("Player Great Orb Spell", cast_animation="Cast_Great_Orb", projectile="Player Great Orb", projectile_count=1,
                      spread_degrees=0.0, release_at=GREAT_ORB_RELEASE_AT, at_targets_feet=False, drop_height=13.0,
                      min_distance=0.0, max_distance=60.0,
                      cooldown_seconds=16.0, charge_effect="", charge_particles_per_second=0.0,
                      cast_sound="@game/Sounds/Spells/magic_whoosh.ogg", cast_key="2", is_uninterruptible=True, display_name="Imaginary Technique: Baby Oil Detonation"), parent=player, bone="Hand_R")
    builder.add(spell("Player Domain Spell", cast_animation="Cast_Domain", projectile="Player Domain Blast", projectile_count=1,
                      spread_degrees=0.0, release_at=0.9, at_targets_feet=True, min_distance=0.0, max_distance=60.0,
                      cooldown_seconds=45.0, charge_effect="", charge_particles_per_second=0.0,
                      cast_sound="@game/Sounds/Boss/roar.ogg", cast_key="3", is_uninterruptible=True, display_name="Domain Expansion"), parent=player)
    return player


# The seal behind his back (from the summoning on, boss_fight.lua): how high its middle is above his feet, how far behind
SEAL_HEIGHT, SEAL_BEHIND = 1.6, 0.45


# His armor and his sword (the transformation at 0 health of his last stand, boss_fight.lua): the pieces cut out of
# the armored model (PIECE lines of Tools/characters_generator.py: where each sits on him, meters from his feet, his
# front -Z), the sword in his fist on its own bone, and the one that falls out of the rift and stands in the ground
ARMOR_PIECES = {
    "torso":    (-0.001, 1.426, 0.005),
    "bracer_l": (-0.551, 1.211, -0.027),
    "bracer_r": (0.554, 1.208, -0.028),
    "greave_l": (-0.202, 0.588, 0.017),
    "greave_r": (0.189, 0.584, 0.025),
}
BOSS_ARMORED_MODEL = "sih_boss_armored"
# The sword's blade over its grip (SWORD_HITBOX of Tools/boss_items_generator.py), a little wider than the steel
SWORD_HITBOX = {"local_offset": [0.0, 0.823, 0.0], "half_extents": [0.18, 0.62, 0.12]}


def add_armor_transformation(builder, boss):
    sword = named("Boss Sword", (0.0, 0.0, 0.0), model={"is_visible": False, "model_name": "sih_boss_sword",
                                                         "emissive_strength": 1.5})
    sword["hitbox"] = dict(SWORD_HITBOX, damage=30.0, poise_damage=45.0, layer=ENEMY_HITBOX_LAYER, show_gizmos=False,
                           hit_stop_seconds=0.1, camera_shake=0.45, is_enabled=False, knockback=3.5)
    # (a trail of crimson fire behind the blade while it strikes, as the swords of the souls bosses that burn)
    sword["weapon_trail"] = {"base_offset": [0.0, 0.25, 0.0], "tip_offset": [0.0, 1.42, 0.0], "color": [1.0, 0.18, 0.06],
                             "intensity": 2.2, "lifetime_seconds": 0.22, "only_while_striking": True}
    held = builder.add(sword, parent=boss, bone="Weapon_R")
    # Fire along the blade (his last phase lights it)
    builder.add(named("Boss Sword Flames", (0.0, 0.85, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 90.0, "lifetime_seconds": 0.45,
        "spawn_half_extents": [0.05, 0.58, 0.03], "velocity": [0.0, 0.6, 0.0], "velocity_randomness": 0.4,
        "gravity": [0.0, 1.5, 0.0], "start_size": 0.14, "end_size": 0.26, "start_color": [1.0, 0.3, 0.15],
        "end_color": [0.5, 0.02, 0.02], "intensity": 1.1, "max_particles": 150, "style": 1, "size_randomness": 0.35,
        "spin_degrees_per_second": 80.0, "flipbook_texture": "@engine/Textures/Flipbooks/fireball_6x4.png",
        "flipbook_columns": 6, "flipbook_rows": 4, "flipbook_loops": 1.0, "flipbook_random_start": False}), parent=held)

    # What the rift drops: the sword that stands in the ground, the pieces that fly onto him (moved by the script)
    builder.add(named("Armor Drop Sword", (0.0, -100.0, 0.0), model={"is_visible": False, "model_name": "sih_boss_sword",
                                                                     "emissive_strength": 1.0}))
    for piece in ARMOR_PIECES:
        builder.add(named("Armor Piece " + piece, (0.0, -100.0, 0.0),
                          model={"is_visible": False, "model_name": BOSS_ARMORED_MODEL + "_" + piece}))

    # The shots of the transformation (his front is his own -Z), boss_fight.lua O.Shots: beside him as he skids back on
    # his boots; close on his face - a beat of silence, then the smile as his health comes back; low behind him under
    # the rift as the pieces rain down around him; low at the sword as it falls into the stone; his body as the plates
    # snap on; beside him as he draws the sword; close on his face as his eyes catch fire and he smiles again; pulled
    # back to the whole of him for the stamp (from his left: the sword in his right hand stays out of the lens)
    shots = (
        ("Armor Fakeout", 1.25, 1.2, 0.5, ((3.4, 1.3, -1.8), (3.0, 1.25, -1.3))),
        ("Armor Face", 3.65, 1.64, 0.35, ((0.22, 1.58, -0.85), (0.12, 1.62, -0.6))),
        # (low beside him looking up: the rift opening over him and the pieces coming out of it)
        ("Armor Rift Sky", 1.75, 9.0, 0.4, ((1.8, 0.6, -2.2), (1.6, 0.5, -2.6))),
        ("Armor Rift Shot", 2.05, 1.9, 0.5, ((2.2, 0.45, 4.2), (2.5, 0.5, 5.0))),
        ("Armor Sword Fall", 1.05, 0.7, 0.25, ((2.6, 0.55, -3.4), (2.4, 0.6, -3.1))),
        # (to the side of the paths of the plates: from straight ahead the torso flew through the lens)
        ("Armor Snap", 3.05, 1.3, 0.4, ((3.0, 1.5, -3.3), (2.5, 1.5, -2.8))),
        ("Armor Draw", 2.55, 1.25, 0.4, ((3.0, 1.2, -2.3), (2.6, 1.3, -1.9))),
        ("Armor Ignite Face", 2.05, 1.87, 0.3, ((-0.25, 1.84, -1.0), (-0.15, 1.87, -0.78))),
        ("Armor Ignite", 1.85, 1.4, 0.5, ((-0.4, 1.6, -1.6), (-1.0, 1.9, -4.4))),
        # (the player's answer starts over the boss's shoulder, looking at the player: see Vessel Gesture)
        ("Vessel Answer", 1.8, 1.4, 0.6, ((0.75, 1.95, 1.7), (0.7, 1.9, 1.35))),
    )
    # The stand-off (film 0:58-1:05, boss_fight.lua O.Standoff): over his shoulder at the player, the other way over the
    # player's shoulder, close on his smile, low and wide as they run at each other (cuts, no flights)
    standoff = (
        ("Standoff Boss Side", 2.8, PLAYER_NAME, 1.3, BOSS_NAME, ((0.55, 1.85, 1.9), (0.45, 1.8, 1.6))),
        ("Standoff Player Side", 2.0, BOSS_NAME, 1.3, PLAYER_NAME, ((-0.5, 1.8, 1.8), (-0.4, 1.75, 1.5))),
        ("Standoff Smile", 1.6, BOSS_NAME, 1.64, BOSS_NAME, ((0.2, 1.6, -0.8), (0.12, 1.62, -0.62))),
        ("Standoff Charge", 1.3, BOSS_NAME, 1.0, BOSS_NAME, ((4.5, 0.7, -5.5), (4.0, 0.8, -6.5))),
    )
    # The fist fight in the sky (film 2:11-2:16, boss_fight.lua O.SkyBrawl): beside them as they leap, then circling
    # them in the clouds while the player mashes [Space] - a blow of his with every press (a quick-time event)
    standoff = standoff + (
        ("Sky Leap", 1.2, BOSS_NAME, 1.2, BOSS_NAME, ((3.5, 0.4, -4.5), (3.0, 2.5, -4.0))),
        # (close beside them, level with them, high over the roofs: the sky, the clouds and the city far below behind them as
        # in the film - from above, the street under them looked like the ground they stood on)
        ("Sky Brawl", 2.0, BOSS_NAME, 1.5, BOSS_NAME, ((4.4, 1.3, -1.8), (4.6, 1.1, -0.7), (4.2, 1.3, 0.4))),
        ("Sky Fall", 1.4, BOSS_NAME, 1.0, BOSS_NAME, ((-3.0, 2.5, -3.5), (-3.5, 1.5, -4.5))),
    )
    # The fight in the air (film 0:40-0:50, boss_fight.lua O.AirFight): low looking up at both rising, beside the player
    # as the ball of oil grows over his palm, wide from the side as they throw and it bursts between them
    standoff = standoff + (
        # (both in the shot: low on the ground between them and to the side, they rise into it)
        ("Air Rise", 1.6, BOSS_NAME, 1.3, BOSS_NAME, ((1.8, 0.3, -2.8), (2.0, 0.1, -3.2))),
        ("Air Diddy", 2.0, PLAYER_NAME, 1.5, PLAYER_NAME, ((-1.3, 1.7, -2.0), (-1.1, 1.75, -1.7))),
        ("Air Clash", 1.8, BOSS_NAME, 1.4, BOSS_NAME, ((7.5, 2.5, -5.5), (8.5, 3.0, -6.0))),
    )
    for name, seconds, target, height, owner, points in standoff:
        quick = {"wait_for_key": " ", "wait_presses": 7, "press_window_seconds": 0.45,
                 "press_sound": "@game/Sounds/Brawl/Punch",
                 "wait_prompt": "Mash [Space] - strike him in the clouds"} if name == "Sky Brawl" else {}
        shot = builder.add(named(name, cutscene=dict({
            "duration_seconds": seconds, "look_at_entity": target, "look_at_height": height, "animation_entity": "",
            "animation_name": "", "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0,
            "can_be_skipped": False, "video_file": "", "blend_in_seconds": 0.0}, **quick)),
            parent=boss if owner == BOSS_NAME else builder.find(PLAYER_NAME))
        for number, point in enumerate(points):
            builder.add(named(f"Camera Point {number + 1}", point), parent=shot)

    # What he says over the shots (the cutscene's title line): the player must know where his armor comes from
    lines = {"Armor Face": ("Did you think it was over?", 2.2),
             "Armor Rift Sky": ("My treasure is not spent yet.", 1.6),
             "Armor Ignite": ("Jeffrey Epstein, Armored in Sin", 1.6)}
    for name, seconds, height, blend, points in shots:
        line, line_seconds = lines.get(name, ("", 0.0))
        shot = builder.add(named(name, cutscene={
            "duration_seconds": seconds,
            "look_at_entity": PLAYER_NAME if name.startswith("Vessel") else BOSS_NAME,
            "look_at_height": height, "animation_entity": "",
            "animation_name": "", "animation_at_seconds": 0.0, "title": line, "title_seconds": line_seconds,
            "can_be_skipped": False, "video_file": "", "blend_in_seconds": blend}), parent=boss)
        for number, point in enumerate(points):
            builder.add(named(f"Camera Point {number + 1}", point), parent=shot)

    # A wave of the sword along the ground (his slam and the seal's barrage): a crimson streak flying low and fast
    builder.add(projectile("Boss Sword Wave", speed=17.0, damage=26.0, poise_damage=30.0, radius=1.1,
                           homing_degrees_per_second=0.0, lifetime_seconds=1.6, fuse_seconds=0.0,
                           trail_effect="Sword Wave Trail", trail_particles_per_second=260.0,
                           hit_effect="Sword Hit Burst", hit_burst=30, hit_sound="@game/Sounds/Combat/Hit",
                           hit_stop_seconds=0.08, camera_shake=0.35))
    builder.add(named("Sword Wave Trail", (0.0, -100.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 0.0, "lifetime_seconds": 0.35,
        "spawn_half_extents": [1.3, 0.25, 0.08], "velocity": [0.0, 0.6, 0.0], "velocity_randomness": 0.3,
        "gravity": [0.0, 0.0, 0.0], "start_size": 0.32, "end_size": 0.08, "start_color": [1.0, 0.35, 0.2],
        "end_color": [0.6, 0.02, 0.02], "intensity": 2.4, "max_particles": 1500}))
    builder.add(named("Sword Hit Burst", (0.0, -100.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 0.0, "lifetime_seconds": 0.5,
        "spawn_half_extents": [0.2, 0.2, 0.2], "velocity": [0.0, 2.0, 0.0], "velocity_randomness": 4.0,
        "gravity": [0.0, -6.0, 0.0], "start_size": 0.12, "end_size": 0.02, "start_color": [1.0, 0.4, 0.2],
        "end_color": [0.5, 0.02, 0.0], "intensity": 3.0, "max_particles": 300}))


def add_seal(builder, boss):
    """The hexagonal seal that burns behind his back from the summoning on (boss_fight.lua shows it and turns it), the
    way the wheel of the film's summoned monster turns behind it; a red aura of fire and smoke around him with it"""
    # (his back is his own +Z; the seal stands upright there, facing the camera behind him, above his shoulders)
    seal = named("Boss Seal", (0.0, SEAL_HEIGHT, SEAL_BEHIND), model={"is_visible": False, "model_name": "sih_hex_seal",
                                                                    "emissive_strength": 1.2})
    seal["transform"]["scale"] = [0.01] * 3
    builder.add(seal, parent=boss)
    # Dark red smoke curling off it, and embers
    builder.add(named("Boss Seal Smoke", (0.0, SEAL_HEIGHT, SEAL_BEHIND), particle_emitter={
        # (thin: a thick cloud of it covered half the screen behind him in the fight)
        "is_emitting": False, "particles_per_second": 9.0, "lifetime_seconds": 1.4,
        "spawn_half_extents": [0.9, 0.9, 0.15], "velocity": [0.0, 0.9, 0.3], "velocity_randomness": 0.5,
        "gravity": [0.0, 0.3, 0.0], "start_size": 0.45, "end_size": 0.9, "start_color": [0.35, 0.03, 0.02],
        "end_color": [0.08, 0.01, 0.01], "intensity": 0.8, "max_particles": 120, "is_dark": True, "style": 2,
        "flipbook_texture": "@engine/Textures/Flipbooks/smoke_puff_8x8.png", "flipbook_columns": 8, "flipbook_rows": 8,
        "flipbook_loops": 1.0, "flipbook_random_start": True}), parent=boss)
    builder.add(named("Boss Seal Embers", (0.0, SEAL_HEIGHT, SEAL_BEHIND), particle_emitter={
        "is_emitting": False, "particles_per_second": 40.0, "lifetime_seconds": 1.2,
        "spawn_half_extents": [1.0, 1.0, 0.1], "velocity": [0.0, 1.2, 0.0], "velocity_randomness": 0.9,
        "gravity": [0.0, 0.5, 0.0], "start_size": 0.06, "end_size": 0.01, "start_color": [1.0, 0.25, 0.08],
        "end_color": [0.6, 0.02, 0.0], "intensity": 2.5, "max_particles": 200, "style": 0, "size_randomness": 0.5}),
                parent=boss)
    # The aura: wisps of dark red smoke rising all around his body, lit from inside (the concept's red glow)
    builder.add(named("Boss Red Aura", (0.0, 0.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 14.0, "lifetime_seconds": 0.8,
        "spawn_half_extents": [0.38, 0.9, 0.38], "velocity": [0.0, 1.1, 0.0], "velocity_randomness": 0.35,
        "gravity": [0.0, 0.6, 0.0], "start_size": 0.25, "end_size": 0.5, "start_color": [0.55, 0.06, 0.04],
        "end_color": [0.12, 0.01, 0.01], "intensity": 0.7, "max_particles": 120, "style": 2, "size_randomness": 0.35,
        "spin_degrees_per_second": 40.0, "flipbook_texture": "@engine/Textures/Flipbooks/smoke_puff_8x8.png",
        "flipbook_columns": 8, "flipbook_rows": 8, "flipbook_loops": 1.0, "flipbook_random_start": True}),
                parent=boss, bone="Hips")
    # Embers rising off him: the last stand burns
    builder.add(named("Boss Ascension Aura", (0.0, 0.0, 0.0), particle_emitter={
        "is_emitting": False, "particles_per_second": 70.0, "lifetime_seconds": 1.4,
        "spawn_half_extents": [0.35, 0.8, 0.35], "velocity": [0.0, 1.6, 0.0], "velocity_randomness": 0.8,
        "gravity": [0.0, 0.6, 0.0], "start_size": 0.09, "end_size": 0.01,
        "start_color": [1.0, 0.25, 0.1], "end_color": [0.6, 0.02, 0.0], "intensity": 1.5, "max_particles": 400,
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
    # (the outer slabs: ground under the heaps of rubble and beyond them, no empty strip at their foot)
    z = -16.0
    while z <= STREET_FAR_Z:
        for x in ((-20.0, -12.0, -4.0, 4.0, 12.0, 20.0) if z >= 24.0 else (-20.0, -12.0, 12.0, 20.0)):
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
        border["transform"]["position"] = [side * 8.0, 0.0, 16.0 + (STREET_FAR_Z - 16.0) * 0.5]
        border["box_collider"]["half_extents"] = [0.5, 3.0, (STREET_FAR_Z - 16.0) * 0.5]
        border["box_collider"]["blocks_camera"] = False
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
            builder.add(rubble_heap((side * STREET_HEAP_X, -0.2, z), 90.0 + wobble, size, "Street Rubble"))
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
    # (their voices: a shout when they see the player, grunts of effort and pain, a last cry)
    entity["character_sounds"].update({"pain_voice": "@game/Sounds/Men/Pain", "attack_voice": "@game/Sounds/Men/Effort",
                                       "death_voice": "@game/Sounds/Men/Death", "alert_voice": "@game/Sounds/Men/Alert",
                                       "pain_voice_chance": 0.75, "attack_voice_chance": 0.4})
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
                          cast_sound="@game/Sounds/Spells/camera_flash.ogg"), parent=hand)


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
    entity["character_actions"].update({"knockback_animation": "Knockback", "knockback_seconds": 0.8,
                                        "knockback_dust": "Skid Dust"})
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
    # While his summon protects him (or a phase changes) the blows glance off him with guard sparks
    entity["health"].update({"max_health": 2400.0, "death_slow_motion_seconds": 3.0, "death_slow_motion_scale": 0.25,
                             "hit_effect": "Punch Impact", "deflect_effect": "Guard Impact"})
    # Staggered, he can't be staggered again for 2.5 s: a string of punches doesn't pin him (see also boss_fight.lua)
    entity["poise"] = {"max_poise": 70.0, "recover_after_seconds": 4.0, "stagger_immunity_seconds": 1.6}
    entity["enemy"].update({"sight_distance": 80.0, "hearing_distance": 3.0, "eye_height": 1.7})
    entity["enemy_health_bar"].update({"is_boss": True, "display_name": BOSS_DISPLAY_NAME})
    # Pressure: short breaths between his combos, longer combos (boss_fight.lua speeds him up in later phases)
    entity["melee_enemy"].update({"give_up_distance": 120.0, "run_distance": 7.0, "attack_cooldown_seconds": 0.6,
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
    add_seal(builder, boss)
    add_armor_transformation(builder, boss)

    # The spells leave from his right hand
    staff = builder.add(named("Spell Hand"), parent=boss, bone="Hand_R")
    # No glow gathering in his hand: what he throws is seen leaving it
    common = {"charge_effect": "", "spread_degrees": 0.0, "at_targets_feet": False}
    # What the boss chooses by itself between its blows
    builder.add(spell("Boss Orb Volley Spell", **{**common, "cast_animation": "Cast_Spell", "projectile": "Boss Orb",
                      "projectile_count": 3, "spread_degrees": 13.0, "release_at": 0.5, "min_distance": 4.5,
                      "max_distance": 30.0, "cooldown_seconds": 14.0, "charge_particles_per_second": 90.0,
                      "cast_sound": "@game/Sounds/Spells/cash_register.ogg"}), parent=staff)
    builder.add(spell("Boss Cleave Spell", **{**common, "cast_animation": "Cast_Cleave", "projectile": "Boss Slash",
                      "projectile_count": 5, "spread_degrees": 8.0, "release_at": CLEAVE_RELEASE_AT, "min_distance": 5.0,
                      "max_distance": 30.0, "cooldown_seconds": 18.0, "charge_particles_per_second": 160.0,
                      "cast_sound": "@game/Sounds/Spells/throw.ogg"}), parent=staff)
    builder.add(spell("Boss Fire Pillar Spell", **{**common, "cast_animation": "Cast_Pillars", "projectile": "Cultist Fire Pillar",
                      "projectile_count": 3, "release_at": 0.36, "at_targets_feet": True, "min_distance": 5.0,
                      "max_distance": 30.0, "cooldown_seconds": 22.0, "charge_particles_per_second": 140.0,
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
    entity["character_actions"].update({"shockwave_radius": 4.5, "shockwave_damage": 28.0, "shockwave_distance": 1.8,
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
    add_fists_and_foot(builder, summon, 22.0, ENEMY_HITBOX_LAYER, poise_damage=35.0, size=0.38, camera_shake=0.5)
    entity["transform"] = transform(SUMMON_WAITING_PLACE, (0.0, 180.0, 0.0))
    # Three meters tall: a body, senses and blows of that size
    entity["box_collider"].update({"half_extents": [1.05, 1.45, 1.05], "local_offset": [0.0, 1.45, 0.0]})
    entity["lock_on_target"].update({"aim_height": 1.9})
    entity["enemy_health_bar"]["height_above_feet"] = 3.3
    entity["character_sounds"].update({"footstep_meters": 2.4, "footstep_camera_shake": 0.16, "footstep_shake_radius": 12.0, "volume": 1.5})
    entity["health"].update({"max_health": 1350.0, "death_slow_motion_seconds": 1.5, "death_slow_motion_scale": 0.35,
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
    # The eight-spoked wheel over its head (the film's last frames): it turns an eighth each time the summon adapts
    # (shown and turned by the script)
    # (just over its stooping head, level: on the head bone it tipped over with every move of the head)
    halo = named("Summon Halo", (0.0, 2.75, 0.15), model={"is_visible": False, "model_name": "sih_summon_wheel"})
    builder.add(halo, parent=summon)
    # From half its health on a blade of light grows out of its right fist (the boss's sword, lit white by the script):
    # hidden and harmless until then; it strikes in the windows of its blows like its fists
    blade = named("Summon Blade", (0.0, 0.05, 0.0), model={"is_visible": False, "model_name": "sih_boss_sword",
                                                         "emissive_strength": 0.0})
    blade["hitbox"] = {"damage": 30.0, "poise_damage": 40.0, "local_offset": [0.0, 0.95, 0.0],
                       "half_extents": [0.2, 0.8, 0.14], "layer": ENEMY_HITBOX_LAYER, "show_gizmos": False,
                       "hit_stop_seconds": 0.08, "camera_shake": 0.5, "is_enabled": False}
    blade["weapon_trail"] = {"base_offset": [0.0, 0.4, 0.0], "tip_offset": [0.0, 1.7, 0.0], "color": [1.0, 0.92, 0.7],
                             "intensity": 1.4, "lifetime_seconds": 0.16, "only_while_striking": True}
    builder.add(blade, parent=summon, bone="Hand_R")
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
    # The gateway between the unseen borders is 6 m wide (the borders at x = +-5 reach in to +-3): MyGame's fog was
    # 3.5 m, so the player could walk out of the arena beside it. It fills the gateway now, and so does its mist
    builder.get(fog)["box_collider"]["half_extents"][0] = FOG_GATE_HALF_WIDTH
    # (a wall for the player, mist for the camera: closed behind a player standing at it, it pushed the camera into his head)
    builder.get(fog)["box_collider"]["blocks_camera"] = False
    mist = builder.get(builder.child_named(fog, "Fog Mist"))["particle_emitter"]
    mist["spawn_half_extents"][0] = FOG_GATE_HALF_WIDTH - 0.2
    mist["particles_per_second"] = mist["particles_per_second"] * FOG_GATE_HALF_WIDTH / 1.75

    builder.add(named("Film Intro", cutscene={
        "duration_seconds": 6.0, "look_at_entity": BOSS_NAME, "look_at_height": 1.6, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": BOSS_DISPLAY_NAME, "title_seconds": 3.0,
        "can_be_skipped": True, "video_file": "@game/Videos/intro.mpg",
        # (no sudden cut from the street: the game fades into black, the film comes out of it, and back after it;
        # the film's sound starts with its picture)
        "fade_seconds": 0.8, "cutscene_sound": "@game/Sounds/Film/intro.ogg"}))

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
        "duration_seconds": 6.2, "look_at_entity": "Domain Reveal Focus", "look_at_height": 0.0, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": "Domain Expansion: Epstein Island Temple",
        "title_seconds": 3.5, "can_be_skipped": False, "video_file": ""}))

    # Two moments taken from the film itself, as it can't be made in the engine (a face that smiles): his smile as his
    # eyes turn white and he turns black against the blast; and the wheel that turns before the summoning
    builder.add(named("Domain Smile", cutscene={
        "duration_seconds": 2.4, "look_at_entity": "", "look_at_height": 0.0, "animation_entity": "", "animation_name": "",
        "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0, "can_be_skipped": False,
        "video_file": "@game/Videos/domain_smile.mpg"}))
    # The film's own pictures of him floating on his back over the burning city as he speaks (2:31.4-2:36.6)
    builder.add(named("Summon Float", cutscene={
        "duration_seconds": 5.2, "look_at_entity": "", "look_at_height": 0.0, "animation_entity": "", "animation_name": "",
        "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0, "can_be_skipped": False,
        "video_file": "@game/Videos/summon_float.mpg", "fade_seconds": 0.5}))
    # Before it: low behind the player as Epstein leaps away from him into the air (he is seen going, then the film)
    leap = builder.add(named("Summon Leap Away", cutscene={
        "duration_seconds": 1.4, "look_at_entity": BOSS_NAME, "look_at_height": 1.6, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": "", "title_seconds": 0.0, "can_be_skipped": False,
        "video_file": "", "blend_in_seconds": 0.0}), parent=builder.find(PLAYER_NAME))
    for number, point in enumerate(((0.9, 1.0, 2.6), (1.1, 0.8, 3.2))):
        builder.add(named(f"Camera Point {number + 1}", point), parent=leap)
    # After it: the monster that came, low before it as it roars, its name (placed on it by the script)
    summon_reveal = builder.add(named("Summon Reveal", cutscene={
        "duration_seconds": 1.5, "look_at_entity": SUMMON_NAME, "look_at_height": 2.2, "animation_entity": "",
        "animation_name": "", "animation_at_seconds": 0.0, "title": SUMMON_DISPLAY_NAME, "title_seconds": 1.4,
        "can_be_skipped": False, "video_file": "", "blend_in_seconds": 0.0}), parent=builder.find(SUMMON_NAME))
    # (on the monster: low before it, looking up at it, coming closer)
    for number, point in enumerate(((1.6, 0.5, -5.5), (1.0, 0.7, -4.2))):
        builder.add(named(f"Camera Point {number + 1}", point), parent=summon_reveal)
    builder.add(named("Summon Wheel", cutscene={
        "duration_seconds": 4.4, "look_at_entity": "", "look_at_height": 0.0, "animation_entity": "", "animation_name": "",
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
    # (a cut, not a flight: he stands at the edge of the arena and the way there goes through the rubble)
    "Domain Sign":      {"blend_in_seconds": 0.0},
    "Domain Reveal":    {"blend_in_seconds": 1.4, "duration_seconds": 5.4},
    "Star Flight":      {"blend_in_seconds": 1.2, "duration_seconds": 6.05},
    "Meteor Fall":      {"duration_seconds": 10.0},
    "Down Wait":        {"blend_in_seconds": 0.0},
    # (lasts until the bottle's shot takes over: no game camera between his rising and the bottle coming down)
    # (a cut from the shot over him lying: a flight there crossed the whole arena)
    "Technique Rise":   {"blend_in_seconds": 0.0, "duration_seconds": 4.6},
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
