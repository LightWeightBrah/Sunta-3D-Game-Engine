"""
Checks where the sword of the Knight goes during each attack: in which frames the blade passes through an enemy
standing straight in front (at a few distances), and whether that falls inside the hit window of the engine

Run on a .blend saved by the generator:
    blender --background --factory-startup --python Tools/Blender/knight_generator.py -- --out <folder> --blend knight.blend
    blender --background knight.blend --python Tools/Blender/check_attack_reach.py

Prints one line per attack and distance: the frames that hit, as fractions of the animation (0 = start, 1 = end)
"""

import bpy
from mathutils import Vector

# The enemy's body in the engine: a box 0.6 x 0.6 x 1.8 m standing on the ground (Box Collider of a character)
ENEMY_HALF_WIDTH = 0.3
ENEMY_HEIGHT = 1.8

# Half the thickness of the sword's Hitbox, added around the enemy instead of around the blade
HITBOX_HALF_THICKNESS = 0.06

# The blade of the sword object (built along +Z from the grip)
BLADE_START = 0.12
BLADE_END = 0.98
BLADE_SAMPLES = 12

ENEMY_DISTANCES = (1.2, 1.6, 2.0)
SUBFRAMES = 4

# Must match CharacterActionsSystem.cpp (AttackStepMove) and the Character Actions defaults
ATTACK_STEP_SPEED = 1.5
ATTACK_STEP_WINDOW = (0.20, 0.35, 0.50)
HIT_WINDOWS = {"Attack_Light": (0.28, 0.56), "Attack_Heavy": (0.45, 0.66)}


def step_factor(progress):
    start, full_until, end = ATTACK_STEP_WINDOW
    if progress < start or progress >= end:
        return 0.0
    if progress <= full_until:
        return 1.0
    return 1.0 - (progress - full_until) / (end - full_until)


def blade_points(sword):
    matrix = sword.matrix_world
    for index in range(BLADE_SAMPLES + 1):
        z = BLADE_START + (BLADE_END - BLADE_START) * index / BLADE_SAMPLES
        yield matrix @ Vector((0.0, 0.0, z))


def touches_enemy(point, distance, stepped_forward):
    margin = ENEMY_HALF_WIDTH + HITBOX_HALF_THICKNESS
    y = point.y + stepped_forward
    return (abs(point.x) <= margin and abs(y - distance) <= margin
            and -HITBOX_HALF_THICKNESS <= point.z <= ENEMY_HEIGHT + HITBOX_HALF_THICKNESS)


def hit_window_for(action_name):
    for prefix, window in HIT_WINDOWS.items():
        if action_name.startswith(prefix):
            return window
    return None


def check_action(scene, armature, sword, action):
    armature.animation_data.action = action
    start, end = (int(frame) for frame in action.frame_range)
    duration_seconds = (end - start) / scene.render.fps
    steps = (end - start) * SUBFRAMES

    hits = {distance: [] for distance in ENEMY_DISTANCES}
    stepped_forward = 0.0

    for step in range(steps + 1):
        progress = step / steps
        stepped_forward += ATTACK_STEP_SPEED * step_factor(progress) * duration_seconds / steps
        scene.frame_set(start + int(step // SUBFRAMES), subframe=(step % SUBFRAMES) / SUBFRAMES)

        for distance in ENEMY_DISTANCES:
            if any(touches_enemy(point, distance, stepped_forward) for point in blade_points(sword)):
                hits[distance].append(progress)

    window = hit_window_for(action.name)
    for distance, progresses in hits.items():
        inside = [p for p in progresses if window and window[0] <= p <= window[1]]
        print("REACH {:<16} enemy at {:.1f} m: hits {:<30} in the hit window {}: {}".format(
            action.name, distance, describe_ranges(progresses, 1.0 / steps), window, "yes" if inside else "NO"))


def describe_ranges(progresses, step):
    """0.10..0.25, 0.60..0.70 instead of a long list of fractions"""
    if not progresses:
        return "never"

    ranges = [[progresses[0], progresses[0]]]
    for progress in progresses[1:]:
        if progress - ranges[-1][1] > step * 1.5:
            ranges.append([progress, progress])
        else:
            ranges[-1][1] = progress

    return ", ".join("{:.2f}..{:.2f}".format(first, last) for first, last in ranges)


def main():
    scene = bpy.context.scene
    armature = bpy.data.objects["Armature"]
    sword = bpy.data.objects["KnightSword"]

    for action in bpy.data.actions:
        if action.name.startswith("Attack"):
            check_action(scene, armature, sword, action)


main()
