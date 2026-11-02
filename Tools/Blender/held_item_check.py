"""
Checks every animation of a character for the way it holds its things, frame by frame, and prints a report:
    - how deep the hand and the forearm (the skinned body, as the animation bends it) go through a held shield
    - how far each wrist bends away from its rest (a wrist bent past about 60 degrees looks broken)
Used by the generators after the animations are made: a clip that fails is fixed (hand_pinning) or replaced

The engine draws exactly these poses: the held models hang on their bones the same way
"""

import math

import bpy
import numpy
from mathutils import Matrix, Quaternion, Vector

from hand_pinning import forearm_in_hand, rest_world_matrix, world_matrix

# Above these the report marks the action
DEEPEST_ALLOWED = 0.01        # meters through the shield
MOST_WRIST_BEND_DEGREES = 60.0
BODY_POINTS_ALLOWED = 10      # vertices of the body inside the shield's planks


def item_world_matrix(armature, item_bone, item_rest_matrix):
    """Where the held model is now: it moves with its bone from where it stood at rest"""
    return world_matrix(armature, item_bone) @ rest_world_matrix(armature, item_bone).inverted() @ item_rest_matrix


def skinned_points(mesh, bone_names):
    """The posed positions (world) of the vertices that follow these bones"""
    groups = {mesh.vertex_groups[name].index for name in bone_names if name in mesh.vertex_groups}
    indices = [vertex.index for vertex in mesh.data.vertices
               if any(slot.group in groups and slot.weight >= 0.2 for slot in vertex.groups)]
    evaluated = mesh.evaluated_get(bpy.context.evaluated_depsgraph_get())
    coordinates = numpy.empty(len(evaluated.data.vertices) * 3)
    evaluated.data.vertices.foreach_get("co", coordinates)
    points = coordinates.reshape(-1, 3)[indices]
    matrix = numpy.array(mesh.matrix_world)
    return points @ matrix[:3, :3].T + matrix[:3, 3]


def shield_face(shield_matrix, face_axis):
    return numpy.array((shield_matrix.to_3x3() @ face_axis).normalized())


def depth_through_shield(points, shield_matrix, radius, back_offset, face_axis):
    """How deep the points go past the back of the shield's planks (inside its disc), below 0 = clear"""
    center = numpy.array(shield_matrix.translation)
    face = shield_face(shield_matrix, face_axis)
    offsets = points - center
    along_face = offsets @ face
    in_plane = numpy.linalg.norm(offsets - numpy.outer(along_face, face), axis=1)
    inside = in_plane <= radius
    if not inside.any():
        return -1.0
    return float((along_face[inside] + back_offset).max())


def shield_through_body(points, shield_matrix, radius, half_thickness, face_axis):
    """How many points of the body (not the shield's arm) lie in the planks: the shield cuts through the body there"""
    center = numpy.array(shield_matrix.translation)
    face = shield_face(shield_matrix, face_axis)
    offsets = points - center
    along_face = offsets @ face
    in_plane = numpy.linalg.norm(offsets - numpy.outer(along_face, face), axis=1)
    return int(((in_plane <= radius * 0.95) & (numpy.abs(along_face) <= half_thickness + 0.005)).sum())


def wrist_bend_degrees(armature, side):
    """The angle between the forearm and where it would point from the hand with a straight wrist"""
    hand = world_matrix(armature, "Hand" + side)
    straight = hand.to_quaternion() @ forearm_in_hand(armature, side)
    actual = (world_matrix(armature, "LowerArm" + side).translation - hand.translation).normalized()
    return math.degrees(straight.angle(actual))


def check_actions(armature, mesh, actions, shield=None, steps=16):
    """Prints a line per action. 'shield': (bone, rest matrix of the model, radius, half thickness, the axis of the model
    its face looks along) or None. Returns the names of the actions that fail"""
    scene = bpy.context.scene
    failing = []
    for action in actions:
        armature.animation_data.action = action
        first, last = (int(frame) for frame in action.frame_range)
        deepest, deepest_at = -1.0, 0.0
        cutting, cutting_at = 0, 0.0
        body_bones = [bone.name for bone in armature.data.bones if bone.use_deform and bone.name not in ("Hand_L", "LowerArm_L", "UpperArm_L")]
        bends = {"_L": (0.0, 0.0), "_R": (0.0, 0.0)}
        for step in range(steps):
            progress = step / (steps - 1)
            scene.frame_set(first + round((last - first) * progress))
            if shield:
                bone, rest_matrix, radius, half_thickness, face_axis = shield
                points = skinned_points(mesh, ("Hand_L", "LowerArm_L"))
                depth = depth_through_shield(points, item_world_matrix(armature, bone, rest_matrix), radius, half_thickness, face_axis)
                if depth > deepest:
                    deepest, deepest_at = depth, progress
                cuts = shield_through_body(skinned_points(mesh, body_bones), item_world_matrix(armature, bone, rest_matrix), radius, half_thickness, face_axis)
                if cuts > cutting:
                    cutting, cutting_at = cuts, progress
            for side in bends:
                bend = wrist_bend_degrees(armature, side)
                if bend > bends[side][0]:
                    bends[side] = (bend, progress)

        problems = []
        if shield and deepest > DEEPEST_ALLOWED:
            problems.append("arm {:.3f} m into the shield at {:.2f}".format(deepest, deepest_at))
        if shield and cutting > BODY_POINTS_ALLOWED:
            problems.append("shield through the body ({} points) at {:.2f}".format(cutting, cutting_at))
        for side, (bend, at) in bends.items():
            if bend > MOST_WRIST_BEND_DEGREES:
                problems.append("wrist{} bent {:.0f} deg at {:.2f}".format(side, bend, at))
        print("HELD_CHECK {:<16} {}".format(action.name, "; ".join(problems) if problems else "ok"))
        if problems:
            failing.append(action.name)
    return failing


# ========== Fixing it ==========

# The shield arm swings away from the body by at most this much (degrees), in these steps
MOST_PUSH_DEGREES = 60
PUSH_STEP_DEGREES = 4


def push_shield_out_of_body(armature, mesh, actions, shield):
    """Frames where the shield cuts through the body: the whole shield arm swings away from the body at the shoulder,
    just as far as needed (the shield moves with it, the arm keeps its bend). 'shield' as in check_actions"""
    from hand_pinning import key_rotations

    bone, rest_matrix, radius, half_thickness, face_axis = shield
    scene = bpy.context.scene
    body_bones = [b.name for b in armature.data.bones if b.use_deform and b.name not in ("Hand_L", "LowerArm_L", "UpperArm_L")]
    for action in actions:
        armature.animation_data.action = action
        first, last = (int(frame) for frame in action.frame_range)
        pushed = 0
        for frame in range(first, last + 1):
            scene.frame_set(frame)
            points = skinned_points(mesh, body_bones)
            item = item_world_matrix(armature, bone, rest_matrix)
            if shield_through_body(points, item, radius, half_thickness, face_axis) <= BODY_POINTS_ALLOWED:
                continue

            # Away from the part of the body the shield is in, around the shoulder
            shoulder = world_matrix(armature, "UpperArm_L").translation
            center = numpy.array(item.translation)
            near = points[numpy.linalg.norm(points - center, axis=1) <= radius]
            away = item.translation - Vector(near.mean(axis=0)) if len(near) else Vector((-1.0, 0.0, 0.0))
            axis = (item.translation - shoulder).cross(away)
            if axis.length < 1e-6:
                continue
            axis.normalize()

            upper = world_matrix(armature, "UpperArm_L").to_quaternion()
            for degrees in range(PUSH_STEP_DEGREES, MOST_PUSH_DEGREES + 1, PUSH_STEP_DEGREES):
                turn = Quaternion(axis, math.radians(degrees))
                turned = Matrix.Translation(shoulder) @ turn.to_matrix().to_4x4() @ Matrix.Translation(-shoulder) @ item
                if shield_through_body(points, turned, radius, half_thickness, face_axis) <= BODY_POINTS_ALLOWED:
                    break
            key_rotations(armature, {"UpperArm_L": turn @ upper}, frame)
            pushed += 1
        if pushed:
            print("HELD_FIX {:<16} the shield arm swung away from the body in {} frames".format(action.name, pushed))


def turn_shield_off_the_body(armature, mesh, actions, shield, most_degrees=120, step=6):
    """Frames where the shield cuts through the body while the hand must stay where it is (at the mouth, drinking):
    the hand turns around its forearm (the forearm rolls it) as little as needed for the shield to swing clear"""
    from hand_pinning import key_rotations

    bone, rest_matrix, radius, half_thickness, face_axis = shield
    scene = bpy.context.scene
    body_bones = [b.name for b in armature.data.bones if b.use_deform and b.name not in ("Hand_L", "LowerArm_L", "UpperArm_L")]
    for action in actions:
        armature.animation_data.action = action
        first, last = (int(frame) for frame in action.frame_range)
        turned_frames = 0
        previous = 0
        for frame in range(first, last + 1):
            scene.frame_set(frame)
            points = skinned_points(mesh, body_bones)
            item = item_world_matrix(armature, bone, rest_matrix)
            if shield_through_body(points, item, radius, half_thickness, face_axis) <= BODY_POINTS_ALLOWED:
                previous = 0
                continue
            wrist = world_matrix(armature, "Hand_L").translation
            axis = (wrist - world_matrix(armature, "LowerArm_L").translation).normalized()
            # Both ways, the smallest turn first (the one of the frame before is tried first: no flicking)
            tries = sorted((degrees for degrees in range(-most_degrees, most_degrees + 1, step) if degrees),
                           key=lambda degrees: (abs(degrees - previous), abs(degrees)))
            best = None
            for degrees in tries:
                turn = Quaternion(axis, math.radians(degrees))
                turned = Matrix.Translation(wrist) @ turn.to_matrix().to_4x4() @ Matrix.Translation(-wrist) @ item
                cuts = shield_through_body(points, turned, radius, half_thickness, face_axis)
                if best is None or cuts < best[0]:
                    best = (cuts, degrees, turn)
                if cuts <= BODY_POINTS_ALLOWED:
                    break
            previous = best[1]
            key_rotations(armature, {"Hand_L": best[2] @ world_matrix(armature, "Hand_L").to_quaternion()}, frame)
            turned_frames += 1
        if turned_frames:
            print("HELD_FIX {:<16} the shield hand turned off the body in {} frames".format(action.name, turned_frames))


# ========== Twisted and popping arms ==========

# Above these the report marks the action: a forearm or wrist twisted further than a real one turns (the skin, without
# twist bones, screws up like a candy wrapper), and a bone that turns further than this between two frames (a pop)
MOST_TWIST_DEGREES = 80.0
MOST_TURN_PER_FRAME_DEGREES = 25.0

ARM_BONES = ("UpperArm_L", "LowerArm_L", "Hand_L", "UpperArm_R", "LowerArm_R", "Hand_R")


def twist_degrees(armature, bone_name):
    """How far the bone is turned around its own length, against its parent, compared to the rest pose"""
    bone = armature.data.bones[bone_name]
    rest_local = bone.parent.matrix_local.inverted() @ bone.matrix_local
    pose_local = armature.pose.bones[bone.parent.name].matrix.inverted() @ armature.pose.bones[bone_name].matrix
    change = (rest_local.to_quaternion().inverted() @ pose_local.to_quaternion()).normalized()
    along = change.y  # the bone lies along its own Y axis
    return abs(math.degrees(2.0 * math.atan2(along, change.w)))


def check_arms(armature, actions):
    """Prints a line per action with its most twisted forearm/wrist and its biggest jump of an arm bone between frames.
    Returns the names of the actions that fail"""
    scene = bpy.context.scene
    failing = []
    for action in actions:
        armature.animation_data.action = action
        first, last = (int(frame) for frame in action.frame_range)
        twist, twist_where = 0.0, ""
        jump, jump_where = 0.0, ""
        previous = None
        for frame in range(first, last + 1):
            scene.frame_set(frame)
            progress = (frame - first) / max(last - first, 1)
            for name in ("LowerArm_L", "Hand_L", "LowerArm_R", "Hand_R"):
                degrees = twist_degrees(armature, name)
                degrees = min(degrees, 360.0 - degrees)
                if degrees > twist:
                    twist, twist_where = degrees, "{} at {:.2f}".format(name, progress)
            rotations = {name: world_matrix(armature, name).to_quaternion() for name in ARM_BONES}
            if previous:
                for name, rotation in rotations.items():
                    degrees = math.degrees(previous[name].rotation_difference(rotation).angle)
                    degrees = min(degrees, 360.0 - degrees)
                    if degrees > jump:
                        jump, jump_where = degrees, "{} at {:.2f}".format(name, progress)
            previous = rotations
        problems = []
        if twist > MOST_TWIST_DEGREES:
            problems.append("twisted {:.0f} deg ({})".format(twist, twist_where))
        if jump > MOST_TURN_PER_FRAME_DEGREES:
            problems.append("pops {:.0f} deg in a frame ({})".format(jump, jump_where))
        print("ARMS_CHECK {:<16} {}".format(action.name, "; ".join(problems) if problems else "ok"))
        if problems:
            failing.append(action.name)
    return failing
