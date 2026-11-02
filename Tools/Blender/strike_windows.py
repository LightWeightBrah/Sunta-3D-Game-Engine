"""
Measures the striking part of each attack: the swing, when the tip of the weapon moves fastest. The engine hurts only
in the hit window of an attack (light_hit_from/to, heavy_hit_from/to in the scene), so a new clip needs its own window;
the generators print one line per attack:
    STRIKE Attack_Light_1 0.31..0.55
The window is the run of frames around the fastest one where the tip moves at least SWING_SPEED_SHARE of that speed
(a long weapon held forward already touches someone standing close, so where it is says less than how it moves).
The weapon is a line from its grip along its own +Z (as the generators build weapons), its tip 'reach' meters away
"""

import bpy
from mathutils import Vector

from hand_pinning import rest_world_matrix, world_matrix

SWING_SPEED_SHARE = 0.45


def tip_position(armature, item_bone, item_rest_matrix, reach):
    item = world_matrix(armature, item_bone) @ rest_world_matrix(armature, item_bone).inverted() @ item_rest_matrix
    return item @ Vector((0.0, 0.0, reach))


def swing_window(speeds):
    """The run of samples around the fastest one that stay above the share of its speed: (first, last) indices"""
    fastest = max(range(len(speeds)), key=lambda index: speeds[index])
    limit = speeds[fastest] * SWING_SPEED_SHARE
    first = last = fastest
    while first > 0 and speeds[first - 1] >= limit:
        first -= 1
    while last < len(speeds) - 1 and speeds[last + 1] >= limit:
        last += 1
    return first, last


def print_strike_windows(armature, item_bone, item_rest_matrix, reach, actions):
    scene = bpy.context.scene
    for action in actions:
        if not action.name.startswith(("Attack", "Fury")):
            continue
        armature.animation_data.action = action
        first, last = (int(frame) for frame in action.frame_range)
        tips = []
        for frame in range(first, last + 1):
            scene.frame_set(frame)
            tips.append(tip_position(armature, item_bone, item_rest_matrix, reach))
        speeds = [(tips[i + 1] - tips[i]).length for i in range(len(tips) - 1)]
        if not speeds:
            continue
        # Averaged over three frames: one frame where the tip jumps is not a swing
        speeds = [sum(speeds[max(i - 1, 0):i + 2]) / len(speeds[max(i - 1, 0):i + 2]) for i in range(len(speeds))]
        start, end = swing_window(speeds)
        frames = max(last - first, 1)
        print("STRIKE {:<16} {:.2f}..{:.2f}".format(action.name, start / frames, (end + 1) / frames))


# ========== How fast the walking clips move ==========

LOCOMOTION = ("Walk", "Run", "Walk_Back", "Strafe_Left", "Strafe_Right")


def print_stride_speeds(armature, actions):
    """The speed each walking clip moves the character at (meters per second, at the model's size): how fast the foot
    standing on the ground slides back under the body. The scene's walk_animation_speed / run_animation_speed
    (Character Animation) take these, multiplied by the entity's scale:
        STRIDE Walk 1.42"""
    scene = bpy.context.scene
    fps = scene.render.fps / scene.render.fps_base
    for action in actions:
        if action.name not in LOCOMOTION:
            continue
        armature.animation_data.action = action
        first, last = (int(frame) for frame in action.frame_range)
        feet = []
        for frame in range(first, last + 1):
            scene.frame_set(frame)
            feet.append([world_matrix(armature, "Foot" + side).translation.copy() for side in ("_L", "_R")])
        speeds = []
        travel = Vector((0.0, 0.0, 0.0))
        for before, after in zip(feet, feet[1:]):
            planted = 0 if before[0].z <= before[1].z else 1
            step = after[planted] - before[planted]
            speeds.append((step.x ** 2 + step.y ** 2) ** 0.5 * fps)
            travel -= step
        # The body goes the opposite way to the foot standing on the ground
        heading = "forward" if abs(travel.y) >= abs(travel.x) and travel.y > 0 else                   "back" if abs(travel.y) >= abs(travel.x) else ("right" if travel.x > 0 else "left")
        speeds.sort()
        # The middle half: a foot just landing or lifting off is not standing yet
        middle = speeds[len(speeds) // 4: max(len(speeds) * 3 // 4, len(speeds) // 4 + 1)]
        print("STRIDE {:<14} {:.2f} {}".format(action.name, sum(middle) / len(middle), heading))
