"""
The effects of Strongest in History Vs Today, made of the engine's general parts (see Sunta/src/Sunta/Effects):
Effect Shapes (rings of shock, arcs of slashes, pillars, blasts) and Particle Emitters (sparks, flames, dust) put
together under one entity, played by name: a fist that lands, a guard that takes a blow, every swing, the strikes
from the sky, the player's techniques

Used by build_scene.py: add_effect_library(builder) adds them, upgrade_effects(builder) gives the old one-emitter
effects of the scene (Oil Splash, Impact Smoke, ...) their shapes and sparks as children

The style: a fight of the film is fire and gold against a dusk sky. Hot white cores, orange to deep red edges,
very short lives (a blow is a blink: 0.1-0.3 s), a flash of light on everything around, sparks that shoot out fast
and hang in the air (drag), rings of shock standing across the blow
"""

HIDDEN = (0.0, -100.0, 0.0)

# Ring, Arc, Pillar, Sphere (EffectShapeKind)
RING, ARC, PILLAR, SPHERE, BOX = 0, 1, 2, 3, 4

# The player's colors (gold, the amber of baby oil) and the boss's (white-hot into crimson)
GOLD_CORE, GOLD_EDGE = [1.0, 0.95, 0.8], [1.0, 0.55, 0.12]
CRIMSON_CORE, CRIMSON_EDGE = [1.0, 0.9, 0.85], [0.95, 0.12, 0.05]
FIRE_CORE, FIRE_EDGE = [1.0, 0.85, 0.55], [1.0, 0.32, 0.04]
OIL_CORE, OIL_EDGE = [1.0, 0.97, 0.85], [1.0, 0.68, 0.2]


def entity(name, **components):
    return {"tag": {"name": name},
            "transform": {"position": list(HIDDEN), "rotation": [0.0, 0.0, 0.0],
                          "rotation_quaternion": [1.0, 0.0, 0.0, 0.0], "scale": [1.0, 1.0, 1.0]},
            **components}


def shape(kind, lifetime, start_radius, end_radius, core, edge, intensity, **settings):
    values = {"kind": kind, "lifetime_seconds": lifetime, "start_radius": start_radius, "end_radius": end_radius,
              "core_color": core, "edge_color": edge, "intensity": intensity}
    values.update(settings)
    return values


def specks(amount, lifetime, start_size, end_size, start_color, middle_color, end_color, intensity, **settings):
    """An emitter that only bursts when its effect is played ('amount' specks each time)"""
    values = {"is_emitting": False, "particles_per_second": 0.0, "lifetime_seconds": lifetime,
              "spawn_half_extents": [0.05, 0.05, 0.05], "velocity": [0.0, 0.0, 0.0], "velocity_randomness": 0.0,
              "gravity": [0.0, 0.0, 0.0], "start_size": start_size, "end_size": end_size,
              "start_color": start_color, "has_middle_color": middle_color is not None,
              "middle_color": middle_color or start_color, "end_color": end_color, "intensity": intensity,
              "max_particles": 3000, "burst_amount": amount}
    values.update(settings)
    return values


def sparks(amount, speed, color_core, color_edge, lifetime=0.35, size=0.05, gravity=-7.0, drag=4.5, stretch=0.045, intensity=5.0):
    """Streaks of light shot out in every direction, slowed by the air, falling"""
    return specks(amount, lifetime, size, size * 0.4, [1.0, 0.97, 0.9], color_core, color_edge, intensity,
                  style=3, velocity_randomness=0.0, spawn_shape=1, spawn_radius=0.08, outward_speed=speed,
                  size_randomness=0.4, gravity=[0.0, gravity, 0.0], drag=drag, stretch=stretch)


def flash(size, lifetime, color, intensity):
    """One big soft spot of light for a blink, where it happens"""
    return specks(1, lifetime, size, size * 0.3, color, None, [0.0, 0.0, 0.0], intensity)


# A puff of smoke baked in Blender (Tools/Blender/flipbook_baker.py): drawn on smoke specks instead of noise
SMOKE_FLIPBOOK = {"flipbook_texture": "@engine/Textures/Flipbooks/smoke_puff_8x8.png", "flipbook_columns": 8,
                  "flipbook_rows": 8, "flipbook_loops": 1.0, "flipbook_random_start": False}


def dust(amount, speed, lifetime=0.9, size=0.35, end_size=0.9, color=(0.42, 0.36, 0.3)):
    """A puff of dark dust thrown out and hanging in the air (it covers what is behind it)"""
    # (a baked puff fills only the middle of its square: drawn bigger than the noise puff it replaces)
    return specks(amount, lifetime, size * 1.5, end_size * 1.5, list(color), None, [c * 1.3 for c in color], 0.7,
                  style=2, spawn_shape=1, spawn_radius=0.15, outward_speed=speed, drag=3.5, gravity=[0.0, 0.6, 0.0],
                  size_randomness=0.4, spin_degrees_per_second=60.0, **SMOKE_FLIPBOOK)


def flames(amount, speed, lifetime=0.55, size=0.7, intensity=1.2):
    return specks(amount, lifetime, size, size * 1.8, FIRE_CORE, [1.0, 0.4, 0.06], [0.25, 0.04, 0.0], intensity,
                  style=1, spawn_shape=1, spawn_radius=0.3, outward_speed=speed, drag=3.0, gravity=[0.0, 2.5, 0.0],
                  size_randomness=0.35, spin_degrees_per_second=90.0)


class Effect:
    """One effect: a root entity (with a shape or specks) and its parts as children"""

    def __init__(self, builder, name, root_shape=None, root_specks=None):
        components = {}
        if root_shape:
            components["effect_shape"] = root_shape
        if root_specks:
            components["particle_emitter"] = root_specks
        self.builder = builder
        self.name = name
        self.index = builder.add(entity(name, **components))

    def shape(self, part, values):
        self.builder.add(entity(f"{self.name} {part}", effect_shape=values), parent=self.index)
        return self

    def specks(self, part, values):
        self.builder.add(entity(f"{self.name} {part}", particle_emitter=values), parent=self.index)
        return self


def index_of(builder, name):
    return next(index for index, entity_ in enumerate(builder.entities) if entity_["tag"]["name"] == name)


def add_to(builder, name, part, **components):
    """A part added under an effect that already is in the scene (an old one-emitter effect)"""
    builder.add(entity(f"{name} {part}", **components), parent=index_of(builder, name))


# ========== Fists ==========

def add_fist_effects(builder):
    # A fist landing: a ring of shock standing across the blow, a hot core inside it, sparks of sweat and dust
    # thrown out along the blow, a flash that lights the two fighters
    (Effect(builder, "Punch Impact", root_shape=shape(
        RING, 0.22, 0.1, 0.8, GOLD_CORE, GOLD_EDGE, 2.0, thickness=0.3, rotation_degrees=[90.0, 0.0, 0.0],
        rotation_randomness=[0.0, 0.0, 180.0], noise_scale=5.0, scroll_speed=2.0, dissolve=0.8,
        light_intensity=2.0, light_radius=4.0, light_color=[1.0, 0.6, 0.3]))
     .shape("Core", shape(RING, 0.12, 0.03, 0.35, GOLD_CORE, GOLD_EDGE, 2.5, thickness=0.7,
                          rotation_degrees=[90.0, 0.0, 0.0], noise_scale=4.0, dissolve=0.6))
     .shape("Echo", shape(RING, 0.3, 0.25, 1.4, GOLD_CORE, [1.0, 0.35, 0.05], 1.0, thickness=0.1,
                          rotation_degrees=[90.0, 0.0, 0.0], offset=[0.0, 0.0, 0.35], noise_scale=6.0, dissolve=0.9))
     .specks("Sparks", sparks(22, 9.0, [1.0, 0.6, 0.15], [0.6, 0.08, 0.0]))
     .specks("Flash", flash(0.8, 0.08, [1.0, 0.75, 0.45], 1.5))
     .specks("Dust", dust(6, 2.5, lifetime=0.6, size=0.18, end_size=0.5)))

    # A blow taken on the forearms: a spray of bright sparks and a hard white ring, no dust
    (Effect(builder, "Guard Impact", root_shape=shape(
        RING, 0.18, 0.1, 0.7, [1.0, 0.95, 0.85], [0.95, 0.7, 0.35], 2.4, thickness=0.22, rotation_degrees=[90.0, 0.0, 0.0],
        noise_scale=5.0, dissolve=0.7, light_intensity=2.0, light_radius=4.0, light_color=[1.0, 0.85, 0.6]))
     .specks("Sparks", sparks(34, 11.0, [1.0, 0.85, 0.4], [0.8, 0.25, 0.0], lifetime=0.45, gravity=-11.0, drag=3.0))
     .specks("Flash", flash(0.7, 0.07, [1.0, 0.9, 0.7], 1.5)))

    # Every swing: an arc of light cutting the air in front of the fighter, tilted each time another way
    (Effect(builder, "Player Swing", root_shape=shape(
        ARC, 0.2, 0.75, 1.05, GOLD_CORE, GOLD_EDGE, 2.2, thickness=0.32, arc_degrees=120.0,
        rotation_degrees=[0.0, 0.0, 0.0], rotation_randomness=[8.0, 10.0, 40.0], noise_scale=5.0, scroll_speed=4.0,
        dissolve=0.5))
     .specks("Streaks", sparks(6, 4.0, [1.0, 0.75, 0.3], [0.5, 0.15, 0.0], lifetime=0.2, size=0.03, gravity=0.0,
                              drag=6.0, intensity=3.0)))
    (Effect(builder, "Player Heavy Swing", root_shape=shape(
        ARC, 0.3, 0.9, 1.6, [1.0, 1.0, 0.9], GOLD_EDGE, 3.0, thickness=0.42, arc_degrees=210.0,
        rotation_degrees=[0.0, 0.0, -25.0], rotation_randomness=[0.0, 10.0, 25.0], noise_scale=5.0, scroll_speed=5.0,
        dissolve=0.6, light_intensity=2.0, light_radius=5.0, light_color=[1.0, 0.65, 0.3]))
     .shape("Wind", shape(ARC, 0.36, 1.1, 2.3, GOLD_CORE, [1.0, 0.4, 0.05], 1.1, thickness=0.15, arc_degrees=230.0,
                          rotation_degrees=[0.0, 0.0, -25.0], noise_scale=6.0, scroll_speed=6.0, dissolve=0.85))
     .specks("Streaks", sparks(16, 6.0, [1.0, 0.75, 0.3], [0.5, 0.15, 0.0], lifetime=0.3, size=0.04, gravity=0.0,
                              drag=5.0, intensity=3.5)))

    (Effect(builder, "Boss Swing", root_shape=shape(
        ARC, 0.22, 0.75, 1.1, CRIMSON_CORE, CRIMSON_EDGE, 2.4, thickness=0.3, arc_degrees=130.0,
        rotation_randomness=[8.0, 10.0, 45.0], noise_scale=5.0, scroll_speed=4.0, dissolve=0.55))
     .specks("Streaks", sparks(6, 4.0, [1.0, 0.3, 0.15], [0.4, 0.0, 0.0], lifetime=0.22, size=0.03, gravity=0.0,
                              drag=6.0, intensity=3.0)))
    (Effect(builder, "Boss Heavy Swing", root_shape=shape(
        ARC, 0.32, 0.9, 1.7, CRIMSON_CORE, CRIMSON_EDGE, 3.2, thickness=0.42, arc_degrees=220.0,
        rotation_degrees=[0.0, 0.0, 25.0], rotation_randomness=[0.0, 10.0, 25.0], noise_scale=5.0, scroll_speed=5.0,
        dissolve=0.6, light_intensity=2.5, light_radius=5.0, light_color=[1.0, 0.25, 0.1]))
     .shape("Wind", shape(ARC, 0.38, 1.1, 2.4, CRIMSON_CORE, [0.6, 0.02, 0.0], 1.2, thickness=0.15, arc_degrees=240.0,
                          rotation_degrees=[0.0, 0.0, 25.0], noise_scale=6.0, scroll_speed=6.0, dissolve=0.85))
     .specks("Streaks", sparks(16, 6.0, [1.0, 0.3, 0.15], [0.4, 0.0, 0.0], lifetime=0.3, size=0.04, gravity=0.0,
                              drag=5.0, intensity=3.5)))

    # The monster's fists into the ground: a wave along the ground, a column of dust and rock, a shower of sparks
    (Effect(builder, "Ground Slam", root_shape=shape(
        RING, 0.6, 0.6, 7.0, FIRE_CORE, FIRE_EDGE, 3.5, thickness=0.22, noise_scale=3.0, scroll_speed=1.5,
        dissolve=0.85, light_intensity=6.0, light_radius=10.0, light_color=[1.0, 0.5, 0.2]))
     .shape("Crack", shape(RING, 0.9, 0.3, 3.0, [1.0, 0.7, 0.4], [0.7, 0.12, 0.0], 2.0, thickness=0.9,
                           noise_scale=6.0, scroll_speed=0.3, dissolve=1.0))
     .shape("Column", shape(PILLAR, 0.5, 0.8, 1.8, FIRE_CORE, [0.8, 0.2, 0.0], 0.8, height=4.0, twist_degrees=200.0,
                            spin_degrees_per_second=200.0, noise_scale=5.0, scroll_speed=3.0, dissolve=0.85))
     .specks("Sparks", sparks(60, 12.0, [1.0, 0.6, 0.2], [0.5, 0.06, 0.0], lifetime=0.7, size=0.07, gravity=-14.0, drag=2.0))
     .specks("Dust", dust(30, 7.0, lifetime=1.6, size=0.9, end_size=2.6))
     .specks("Flash", flash(3.0, 0.1, [1.0, 0.8, 0.5], 1.4)))


# ========== Techniques and strikes ==========

def add_technique_effects(builder):
    # The bottle landing on him turns into a block of baby oil around him: a cube of amber light growing out of
    # the impact and turning slowly, its edges burning, drops of oil flying (it bursts a moment later: boss_fight.lua
    # plays 'Oil Detonation' after it)
    (Effect(builder, "Oil Cube", root_shape=shape(
        BOX, 0.95, 0.4, 2.3, OIL_CORE, OIL_EDGE, 2.4, spin_degrees_per_second=60.0, noise_scale=4.0, scroll_speed=0.8,
        dissolve=0.25, light_intensity=4.0, light_radius=9.0, light_color=[1.0, 0.75, 0.35]))
     .shape("Inner", shape(BOX, 0.95, 0.2, 1.6, [1.0, 0.95, 0.75], [0.9, 0.5, 0.1], 0.9, spin_degrees_per_second=-90.0,
                           noise_scale=5.0, scroll_speed=1.4, dissolve=0.3))
     .specks("Drops", sparks(50, 7.0, OIL_EDGE, [0.6, 0.2, 0.0], lifetime=0.8, size=0.07, gravity=-10.0, drag=2.0,
                            intensity=3.0)))


    # The great bottle bursting: a blast of oil and fire (a ball swelling out), a wave over the whole ground,
    # a column of fire, burning drops, smoke
    (Effect(builder, "Oil Detonation", root_shape=shape(
        SPHERE, 0.75, 0.6, 4.2, [1.0, 1.0, 0.9], OIL_EDGE, 1.3, noise_scale=4.0, scroll_speed=1.2, dissolve=0.9,
        light_intensity=6.0, light_radius=18.0, light_color=[1.0, 0.7, 0.35]))
     .shape("Wave", shape(RING, 0.9, 1.0, 12.0, OIL_CORE, FIRE_EDGE, 2.2, thickness=0.18, noise_scale=3.0, dissolve=0.9))
     .shape("Column", shape(PILLAR, 0.9, 1.2, 2.4, FIRE_CORE, FIRE_EDGE, 0.8, height=10.0, twist_degrees=260.0,
                            spin_degrees_per_second=160.0, noise_scale=5.0, scroll_speed=2.5, dissolve=0.9))
     .specks("Flames", flames(160, 9.0, lifetime=0.9, size=1.2, intensity=1.3))
     .specks("Drops", sparks(140, 16.0, OIL_EDGE, [0.6, 0.15, 0.0], lifetime=1.1, size=0.09, gravity=-12.0, drag=1.6))
     .specks("Smoke", dust(40, 6.0, lifetime=2.4, size=1.4, end_size=3.6, color=(0.18, 0.14, 0.12)))
     .specks("Flash", flash(4.0, 0.12, [1.0, 0.85, 0.6], 1.2)))

    # The domain shattering (the player's [3] inside it): a white sphere and a wave across the arena
    # (a wave along the ground and shards: no sphere - it took in the camera and the whole screen went white)
    (Effect(builder, "Domain Shatter", root_specks=flash(3.0, 0.15, [1.0, 0.8, 0.5], 1.2))
     .shape("Wave", shape(RING, 1.4, 2.0, 40.0, OIL_CORE, [1.0, 0.5, 0.15], 1.8, thickness=0.1, dissolve=0.9))
     .specks("Shards", sparks(220, 22.0, [1.0, 0.85, 0.6], [0.6, 0.15, 0.0], lifetime=1.4, size=0.12, gravity=-6.0, drag=1.2)))

    # The boss's knockback through the ruins: a burst of rock and fire where he smashes through
    (Effect(builder, "Crash Impact", root_shape=shape(
        RING, 0.45, 0.5, 4.0, FIRE_CORE, FIRE_EDGE, 2.5, thickness=0.25, rotation_degrees=[90.0, 0.0, 0.0],
        dissolve=0.9, light_intensity=6.0, light_radius=12.0))
     .specks("Sparks", sparks(70, 13.0, [1.0, 0.6, 0.2], [0.5, 0.06, 0.0], lifetime=0.8, size=0.07, gravity=-12.0, drag=2.0))
     .specks("Dust", dust(24, 6.0, lifetime=1.8, size=1.0, end_size=2.8)))


def upgrade_effects(builder):
    """The scene's older effects (one emitter each) get shapes, sparks and light as children"""
    # The player's bottle hitting: a splash ring of amber, sparks of oil, a flash
    add_to(builder, "Oil Splash", "Ring", effect_shape=shape(
        RING, 0.26, 0.15, 1.3, OIL_CORE, OIL_EDGE, 2.0, thickness=0.25, rotation_degrees=[90.0, 0.0, 0.0],
        rotation_randomness=[0.0, 0.0, 180.0], noise_scale=5.0, dissolve=0.85, light_intensity=2.0, light_radius=6.0,
        light_color=[1.0, 0.75, 0.4]))
    add_to(builder, "Oil Splash", "Burst", effect_shape=shape(
        SPHERE, 0.2, 0.1, 0.9, [1.0, 1.0, 0.95], OIL_EDGE, 1.2, noise_scale=5.0, dissolve=0.9))
    add_to(builder, "Oil Splash", "Sparks", particle_emitter=sparks(26, 10.0, OIL_EDGE, [0.5, 0.15, 0.0]))

    # Where the strikes from the sky and the lines of fire land: fire and a wave along the ground
    add_to(builder, "Impact Smoke", "Wave", effect_shape=shape(
        RING, 0.55, 0.4, 4.5, FIRE_CORE, FIRE_EDGE, 1.4, thickness=0.22, noise_scale=4.0, dissolve=0.85,
        light_intensity=1.5, light_radius=7.0, light_color=[1.0, 0.5, 0.2]))
    # (no column of fire: several land at once around the player, and walls of light hid the fight)
    add_to(builder, "Impact Smoke", "Flames", particle_emitter=flames(24, 5.0, intensity=0.8))
    add_to(builder, "Impact Smoke", "Sparks", particle_emitter=sparks(30, 11.0, [1.0, 0.6, 0.2], [0.5, 0.06, 0.0],
                                                                     lifetime=0.6, gravity=-12.0, drag=2.2))

    # The boss's pages, tapes and money hitting: a small hard ring of their color
    add_to(builder, "Page Burst", "Ring", effect_shape=shape(
        RING, 0.18, 0.1, 0.9, [1.0, 1.0, 1.0], [0.9, 0.85, 0.7], 3.0, thickness=0.25, rotation_degrees=[90.0, 0.0, 0.0],
        dissolve=0.8, light_intensity=2.0, light_radius=5.0, light_color=[1.0, 0.95, 0.85]))
    add_to(builder, "Flash Burst", "Ring", effect_shape=shape(
        SPHERE, 0.16, 0.1, 1.3, [1.0, 1.0, 1.0], [0.6, 0.75, 1.0], 2.5, dissolve=0.9,
        light_intensity=3.0, light_radius=6.0, light_color=[0.85, 0.9, 1.0]))
    add_to(builder, "Money Burst", "Ring", effect_shape=shape(
        RING, 0.2, 0.1, 1.0, [0.9, 1.0, 0.8], [0.3, 0.9, 0.25], 2.5, thickness=0.25, rotation_degrees=[90.0, 0.0, 0.0],
        dissolve=0.8, light_intensity=2.0, light_radius=5.0, light_color=[0.6, 1.0, 0.5]))
    add_to(builder, "Money Burst", "Sparks", particle_emitter=sparks(12, 7.0, [0.6, 1.0, 0.4], [0.1, 0.3, 0.0]))

    # (no ring for the stars of the domain: hundreds land at once, and their light added up to a white screen)


def add_effect_library(builder):
    add_fist_effects(builder)
    add_technique_effects(builder)
