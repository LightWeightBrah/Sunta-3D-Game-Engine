"""
Gives the hands of a rigged character fingers: generated rigs (Meshy) end at the hand bone, so their hands stay open
and stiff whatever the character does. Here every finger gets two bones and the thumb two, found from the shape of
the hand's mesh, and each animation keys a pose of the fingers: a fist for a punch, two raised fingers for a sign,
a relaxed hand for everything else

    rig = finger_rig.add_fingers(armature, mesh)          # once, before the animations are made
    finger_rig.key_poses(armature, rig, action, {"_R": "two", "_L": "relaxed"})
    target = finger_rig.hand_target(armature, rig, "_R", wrist, fingers_towards, palm_towards)   # for hand_pinning

How the hand is read (as character_from_mesh.make_fist does): its longest direction is along the fingers, its widest
across them is the line of the knuckles, the thumb sticks out on one side near the wrist. Past the knuckles the skin
falls into four groups across the hand: the fingers
"""

import math
from dataclasses import dataclass, field

import bpy
import numpy
from mathutils import Matrix, Quaternion, Vector

import character_from_mesh

FINGERS = ("Index", "Middle", "Ring", "Little")

# Where the first joint of a finger ends, as a part of its length
MIDDLE_JOINT = 0.45

# The skin turns from one bone to the next over this part of the finger's length around a joint
JOINT_BLEND = 0.16

# name: {finger: (degrees at the knuckle, degrees at the middle joint)}, "Thumb": (towards the palm, at its joint)
POSES = {
    "open":    {"Index": (0, 0), "Middle": (0, 0), "Ring": (0, 0), "Little": (0, 0), "Thumb": (0, 0)},
    "relaxed": {"Index": (14, 18), "Middle": (18, 22), "Ring": (22, 26), "Little": (26, 30), "Thumb": (10, 12)},
    "fist":    {"Index": (82, 98), "Middle": (84, 100), "Ring": (86, 100), "Little": (88, 100), "Thumb": (48, 55)},
    # Index and middle finger straight up together, the others closed under the thumb: the sign of a technique
    "two":     {"Index": (0, 2), "Middle": (0, 2), "Ring": (88, 100), "Little": (90, 100), "Thumb": (58, 50)},
    # A hand held out to carry something on its palm
    "cupped":  {"Index": (8, 22), "Middle": (10, 24), "Ring": (12, 26), "Little": (14, 28), "Thumb": (0, 8)},
}


@dataclass
class Hand:
    """One hand as it is at rest (the armature's space)"""
    side: str
    wrist: Vector
    fingers: Vector   # from the wrist to the fingertips
    across: Vector    # along the knuckles, towards the thumb
    palm: Vector      # out of the palm (the way the fingers close)
    bones: dict = field(default_factory=dict)   # finger -> (first bone, second bone)
    axes: dict = field(default_factory=dict)    # finger -> the direction its joints turn around (closing it)
    # bone -> how far it is already bent towards the palm at rest (degrees): a pose's angles are then from straight
    rest_bend: dict = field(default_factory=dict)


def bone_name(finger, joint, side):
    return "Finger_{}_{}{}".format(finger, joint, side)


# ========== Reading the hand ==========


def read_hand(armature, mesh, side):
    """The hand's directions and the skin of each finger: (Hand, {finger: [(vertex index, along, world position)]})"""
    hand_bone = armature.data.bones["Hand" + side]
    wrist = armature.matrix_world @ hand_bone.head_local
    elbow = armature.matrix_world @ armature.data.bones["LowerArm" + side].head_local
    bone_direction = (wrist - elbow).normalized()
    bone_length = hand_bone.length * armature.matrix_world.to_scale().x
    reach = bone_length * character_from_mesh.HAND_REACH_IN_BONES
    radius = bone_length * character_from_mesh.HAND_RADIUS_IN_BONES

    group = mesh.vertex_groups[hand_bone.name].index
    to_world = mesh.matrix_world

    skin = []
    for vertex in mesh.data.vertices:
        if not any(slot.group == group and slot.weight >= character_from_mesh.FINGER_WEIGHT for slot in vertex.groups):
            continue
        position = to_world @ vertex.co
        offset = position - wrist
        along_bone = offset.dot(bone_direction)
        if -radius * 0.5 <= along_bone <= reach and (offset - bone_direction * along_bone).length <= radius:
            skin.append((vertex.index, position))

    points = numpy.array([tuple(position) for _, position in skin])
    _, _, directions = numpy.linalg.svd(points - points.mean(axis=0), full_matrices=False)
    fingers = Vector(directions[0]).normalized()
    if fingers.dot(Vector(points.mean(axis=0)) - wrist) < 0.0:
        fingers = -fingers
    if fingers.angle(bone_direction) > math.radians(character_from_mesh.FINGERS_OFF_BONE_DEGREES):
        fingers = bone_direction

    along = (points - numpy.array(wrist)) @ numpy.array(fingers)
    flat = points - numpy.outer(along, numpy.array(fingers))
    middle = flat.mean(axis=0)
    _, _, directions = numpy.linalg.svd(flat - middle, full_matrices=False)
    across = Vector(directions[0]).normalized()

    # The thumb's side: it sticks out between the wrist and the knuckles
    length = float(along.max())
    near_wrist = (along > length * 0.1) & (along < length * 0.45)
    sideways = (flat - middle) @ numpy.array(across)
    if near_wrist.sum() >= 4 and -sideways[near_wrist].min() > sideways[near_wrist].max():
        across = -across
        sideways = -sideways
    palm = across.cross(fingers) if side == "_R" else fingers.cross(across)
    palm.normalize()

    hand = Hand(side, wrist, fingers, across, palm)

    # Past the knuckles the skin falls into four groups across the hand
    knuckles = length * character_from_mesh.PALM_PART_OF_HAND
    # (far enough out that the thumb, which ends near the first joints of the fingers, is not among them)
    clear = along > knuckles + (length - knuckles) * 0.55
    centers = numpy.linspace(sideways[clear].min(), sideways[clear].max(), 9)[1::2]
    for _ in range(12):
        nearest = numpy.abs(sideways[clear][:, None] - centers[None, :]).argmin(axis=1)
        centers = numpy.array([sideways[clear][nearest == index].mean() if (nearest == index).any() else centers[index]
                               for index in range(4)])
    centers = numpy.sort(centers)[::-1]   # the index finger is next to the thumb
    spacing = float(numpy.abs(numpy.diff(centers)).mean())

    groups = {finger: [] for finger in FINGERS + ("Thumb",)}
    for (index, position), distance, side_way in zip(skin, along, sideways):
        is_thumb = side_way > centers[0] + spacing * 0.8 and distance > length * 0.05
        if is_thumb:
            groups["Thumb"].append((index, float(distance), position))
        elif distance > knuckles - (length - knuckles) * JOINT_BLEND:
            finger = FINGERS[int(numpy.abs(centers - side_way).argmin())]
            groups[finger].append((index, float(distance), position))

    return hand, groups, knuckles, centers, numpy.array(middle)


# ========== Bones and weights ==========


def add_bone(edit_bones, name, parent, head, tail):
    bone = edit_bones.new(name)
    bone.head, bone.tail = head, tail
    bone.parent = parent
    bone.use_deform = True
    return bone


def add_hand_fingers(armature, mesh, side):
    hand, groups, knuckles, centers, middle = read_hand(armature, mesh, side)
    to_armature = armature.matrix_world.inverted()

    # Where each finger starts, bends and ends
    joints = {}
    for number, finger in enumerate(FINGERS):
        skin = groups[finger]
        if len(skin) < 6:
            continue
        tip_distance = max(distance for _, distance, _ in skin)
        positions = numpy.array([tuple(position) for _, distance, position in skin if distance > knuckles])
        center = Vector(positions.mean(axis=0))
        # A line along the hand's fingers through the middle of this finger's skin
        base = center + hand.fingers * (knuckles - (center - hand.wrist).dot(hand.fingers))
        tip = base + hand.fingers * (tip_distance - knuckles)
        joints[finger] = (base, base.lerp(tip, MIDDLE_JOINT), tip)
        hand.axes[finger] = hand.across.copy()

    thumb = groups["Thumb"]
    if len(thumb) >= 6:
        positions = numpy.array([tuple(position) for _, _, position in thumb])
        _, _, directions = numpy.linalg.svd(positions - positions.mean(axis=0), full_matrices=False)
        direction = Vector(directions[0]).normalized()
        if direction.dot(hand.across) < 0.0:
            direction = -direction
        reach = (positions - positions.mean(axis=0)) @ numpy.array(direction)
        center = Vector(positions.mean(axis=0))
        base, tip = center + direction * float(reach.min()), center + direction * float(reach.max())
        joints["Thumb"] = (base, base.lerp(tip, 0.5), tip)
        # The thumb folds over the palm: around the direction of the fingers
        hand.axes["Thumb"] = hand.fingers.copy()

    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode="EDIT")
    edit_bones = armature.data.edit_bones
    parent = edit_bones["Hand" + side]
    for finger, (base, bend, tip) in joints.items():
        first = add_bone(edit_bones, bone_name(finger, 1, side), parent, to_armature @ base, to_armature @ bend)
        add_bone(edit_bones, bone_name(finger, 2, side), first, to_armature @ bend, to_armature @ tip)
        hand.bones[finger] = (bone_name(finger, 1, side), bone_name(finger, 2, side))
    bpy.ops.object.mode_set(mode="OBJECT")

    # The skin follows the joints: what the hand bone moved is handed over to the finger's bones, smoothly around each joint
    hand_group = mesh.vertex_groups["Hand" + side]
    for finger, (base, bend, tip) in joints.items():
        first = mesh.vertex_groups.new(name=bone_name(finger, 1, side))
        second = mesh.vertex_groups.new(name=bone_name(finger, 2, side))
        direction = (tip - base).normalized()
        length = (tip - base).length
        blend = length * JOINT_BLEND
        bend_at = (bend - base).length

        for index, _, position in groups[finger]:
            vertex = mesh.data.vertices[index]
            hand_weight = next((slot.weight for slot in vertex.groups if slot.group == hand_group.index), 0.0)
            if hand_weight <= 0.0:
                continue
            distance = (position - base).dot(direction)
            on_finger = smooth_step((distance + blend) / (2.0 * blend))
            on_second = smooth_step((distance - bend_at + blend) / (2.0 * blend))
            if on_finger <= 0.0:
                continue
            hand_group.add([index], hand_weight * (1.0 - on_finger), "REPLACE")
            first.add([index], hand_weight * on_finger * (1.0 - on_second), "REPLACE")
            second.add([index], hand_weight * on_finger * on_second, "REPLACE")

    return hand


def smooth_step(value):
    value = min(max(value, 0.0), 1.0)
    return value * value * (3.0 - 2.0 * value)


def hands_from_skeleton(armature):
    """The hands of a skeleton that has its fingers already (Mixamo's auto-rigger: three bones a finger, named by
    motion_retarget.mixamo_native_names). Returns {"_R": Hand, "_L": Hand} for key_poses and hand_target"""
    rig = {}
    for side in ("_R", "_L"):
        def head(name):
            return armature.matrix_world @ armature.data.bones[name].head_local

        wrist = head("Hand" + side)
        fingers = (head(bone_name("Middle", 1, side)) - wrist).normalized()
        across = head(bone_name("Index", 1, side)) - head(bone_name("Little", 1, side))
        across = (across - fingers * across.dot(fingers)).normalized()
        # Out of the palm: the side of the hand's plane where the thumb's root lies
        palm = fingers.cross(across)
        if (head(bone_name("Thumb", 2, side)) - wrist).dot(palm) < 0.0:
            palm = -palm
        hand = Hand(side, wrist, fingers, across, palm)
        for finger in FINGERS + ("Thumb",):
            hand.bones[finger] = tuple(bone_name(finger, joint, side) for joint in (1, 2, 3))
            hand.axes[finger] = fingers.copy() if finger == "Thumb" else across.copy()
        # The fingers of the uploaded mesh rest a little curled: measured, so 'open' is straight and 'two' points up
        for finger in FINGERS:
            sign = closing_sign(hand, finger)
            before = fingers
            for name in hand.bones[finger]:
                bone = armature.data.bones[name]
                direction = (armature.matrix_world.to_3x3() @ (bone.tail_local - bone.head_local)).normalized()
                bend = math.degrees(math.atan2(before.cross(direction).dot(hand.axes[finger]) * sign, before.dot(direction)))
                hand.rest_bend[name] = bend
                before = direction
        rig[side] = hand
    return rig


def add_fingers(armature, mesh):
    """Finger bones for both hands. Returns {"_R": Hand, "_L": Hand}"""
    rig = {side: add_hand_fingers(armature, mesh, side) for side in ("_R", "_L")}
    for side, hand in rig.items():
        print("FINGERS {} {}".format(side, " ".join(sorted(hand.bones))))
    return rig


# ========== Posing ==========


def closing_sign(hand, finger):
    """+1 or -1: the way around the finger's axis that closes it towards the palm"""
    direction = hand.across if finger == "Thumb" else hand.fingers
    turned = Quaternion(hand.axes[finger], math.radians(30.0)) @ direction
    towards = hand.palm if finger != "Thumb" else hand.palm
    return 1.0 if turned.dot(towards) > 0.0 else -1.0


def pose_rotations(armature, hand, pose):
    """{bone name: its rotation_quaternion} for a pose of POSES (or a dict like its entries)"""
    angles = POSES[pose] if isinstance(pose, str) else pose
    rotations = {}
    for finger, names in hand.bones.items():
        sign = closing_sign(hand, finger)
        first, second = angles[finger]
        if len(names) == 3:
            # Three joints (a skeleton rigged by Mixamo): the last one bends less, the thumb's base turns a little
            joints = (first * 0.4, first * 0.6, second) if finger == "Thumb" else (first, second, second * 0.75)
        else:
            joints = (first, second)
        for name, degrees in zip(names, joints):
            degrees -= hand.rest_bend.get(name, 0.0)
            # A turn around a direction of the armature, said in the bone's own space
            rest = (armature.matrix_world @ armature.data.bones[name].matrix_local).to_quaternion()
            turn = Quaternion(hand.axes[finger], math.radians(degrees) * sign)
            rotations[name] = rest.inverted() @ turn @ rest
    return rotations


def blend_poses(first, second, share):
    first = POSES[first] if isinstance(first, str) else first
    second = POSES[second] if isinstance(second, str) else second
    return {finger: tuple(a + (b - a) * share for a, b in zip(first[finger], second[finger])) for finger in first}


def key_poses(armature, rig, action, poses):
    """Keys the fingers through the whole action. 'poses': {side: a pose name, or pose(progress) -> a pose name or dict}"""
    armature.animation_data.action = action
    first, last = (int(frame) for frame in action.frame_range)
    frames = max(last - first, 1)

    for side, pose in poses.items():
        hand = rig[side]
        for frame in range(first, last + 1):
            now = pose((frame - first) / frames) if callable(pose) else pose
            for name, rotation in pose_rotations(armature, hand, now).items():
                pose_bone = armature.pose.bones[name]
                pose_bone.rotation_mode = "QUATERNION"
                pose_bone.rotation_quaternion = rotation
                pose_bone.keyframe_insert("rotation_quaternion", frame=frame)


def hand_target(armature, rig, side, wrist, fingers_towards, palm_towards):
    """The world matrix of the hand bone with its wrist at 'wrist', its fingers pointing along 'fingers_towards' and
    its palm looking along 'palm_towards' (as close as it can): a target for hand_pinning.HandPin"""
    hand = rig[side]

    def frame(fingers, palm):
        fingers = fingers.normalized()
        palm = (palm - fingers * palm.dot(fingers)).normalized()
        return Matrix((fingers.cross(palm), fingers, palm)).transposed()

    turn = frame(Vector(fingers_towards), Vector(palm_towards)) @ frame(hand.fingers, hand.palm).transposed()
    rest = (armature.matrix_world @ armature.data.bones["Hand" + side].matrix_local).to_3x3()
    return Matrix.Translation(Vector(wrist)) @ (turn @ rest).to_4x4()
