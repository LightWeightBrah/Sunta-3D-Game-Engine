"""
Pins a hand of an animated character to where it should be, frame by frame, after the motion was moved onto our skeleton
(motion_retarget.py). The arm reaches there with a two bone IK (the upper arm and the forearm turn, the elbow keeps bending
the way the clip bent it), the hand takes the wanted rotation, and the result is keyed into the animation

What it fixes:
    - a two-handed weapon: clips made for a sword (or by Text to Motion) let the left hand wander off the haft;
      'on_the_haft' keeps it where it holds the weapon in a reference pose
    - a shield held up: Mixamo's block holds it sideways; 'shield_in_front' puts it before the chest, its face forward
    - a whole new motion of one arm (a parry): 'shield_path' moves the shield through key poses
'weight' (0..1 over the clip) fades between the clip's own arm and the pinned one, so a pin can come and go smoothly

Conventions (Blender axes): X = the character's right, Y = forward, Z = up, the character stands at the origin
"""

import math
from dataclasses import dataclass, field

import bpy
from mathutils import Matrix, Quaternion, Vector

from motion_retarget import local_rotation


# The left fist holds a two-handed haft this far above the right one (meters for a 1.8 m character: a big fist's width,
# the two fists touch without going into each other)
FIST_SPACING = 0.115

# How much of the hand's roll the solved forearm takes at once (0: spread_twist spreads every twist along the arm afterwards,
# frame by frame and without jumps; taking it here from the rest pose twisted forearms by up to 180 degrees)
FOREARM_ROLL_SHARE = 0.0

# The elbow turns around the line shoulder-wrist at most this far between two frames (degrees): it can't flip
MOST_BEND_TURN_PER_FRAME = 20.0


@dataclass
class HandPin:
    """'target(armature, progress)': the world matrix the hand bone should have now (the pose of the clip in this frame
    is set when it is called, the clip's arm included). 'weight(progress)': how much of the pin is used (0..1)"""
    side: str
    target: callable
    weight: callable = field(default=lambda progress: 1.0)
    # A hand around a haft may turn around it freely: (the haft's direction, a point on it), both in the hand's space
    # at the target. Each frame the turn with the straightest wrist is used (close to the turn of the frame before)
    free_turn: tuple = None


# ========== Reading the pose ==========


def world_matrix(armature, bone_name):
    return armature.matrix_world @ armature.pose.bones[bone_name].matrix


def rest_world_matrix(armature, bone_name):
    return armature.matrix_world @ armature.data.bones[bone_name].matrix_local


def arm_bones(side):
    return "UpperArm" + side, "LowerArm" + side, "Hand" + side


# ========== Two bone IK ==========


def bend_direction(shoulder, elbow, wrist, reach):
    """Which way the elbow points away from the line shoulder -> target: the way the clip bends it now"""
    along = reach.normalized()
    bend = elbow - shoulder
    bend = bend - along * bend.dot(along)
    if bend.length < 1e-4:
        # A straight arm: bend down and back, like a relaxed elbow
        bend = Vector((0.0, -0.3, -1.0))
        bend = bend - along * bend.dot(along)
    return bend.normalized()


def forearm_in_hand(armature, side):
    """Which way the forearm goes from the wrist (towards the elbow), in the space of the hand bone at rest"""
    hand = armature.data.bones["Hand" + side]
    lower = armature.data.bones["LowerArm" + side]
    to_elbow = (armature.matrix_world @ lower.head_local) - (armature.matrix_world @ hand.head_local)
    return rest_world_matrix(armature, hand.name).to_quaternion().inverted() @ to_elbow.normalized()


def solve_arm(armature, side, hand_target, previous_bend=None, bend_out=None):
    """The world rotations of the upper arm, the forearm and the hand that put the wrist at the target's place
    (as close as the arm reaches) with the target's rotation. 'previous_bend': which way the elbow pointed in the frame
    before (it turns at most MOST_BEND_TURN_PER_FRAME from there, so it never flips to the other side in one frame);
    the way it points now is appended to 'bend_out'"""
    upper, lower, hand = arm_bones(side)
    shoulder = world_matrix(armature, upper).translation
    elbow = world_matrix(armature, lower).translation
    wrist = world_matrix(armature, hand).translation
    upper_length = (elbow - shoulder).length
    lower_length = (wrist - elbow).length

    goal = hand_target.translation
    reach = goal - shoulder
    distance = min(reach.length, (upper_length + lower_length) * 0.999)
    distance = max(distance, abs(upper_length - lower_length) * 1.001)
    along = reach.normalized()

    # The elbow on the circle where both bones meet: where the wrist bends the least (the forearm points from
    # the hand the way it does at rest), so a held shield stays along the forearm and the wrists never twist
    to_middle = (upper_length ** 2 - lower_length ** 2 + distance ** 2) / (2.0 * distance)
    height = math.sqrt(max(upper_length ** 2 - to_middle ** 2, 0.0))
    circle_middle = shoulder + along * to_middle
    forearm_from_hand = hand_target.to_quaternion() @ forearm_in_hand(armature, side)
    straight_wrist_elbow = goal + forearm_from_hand * lower_length
    towards = straight_wrist_elbow - circle_middle
    towards = towards - along * towards.dot(along)
    bend = towards.normalized() if towards.length > 1e-3 else bend_direction(shoulder, elbow, wrist, reach)
    if previous_bend is not None:
        kept = previous_bend - along * previous_bend.dot(along)
        if kept.length > 1e-3:
            kept.normalize()
            angle = kept.angle(bend, 0.0)
            most = math.radians(MOST_BEND_TURN_PER_FRAME)
            if angle > most:
                bend = kept.slerp(bend, most / angle, bend).normalized()
    if bend_out is not None:
        bend_out.append(bend.copy())
    new_elbow = circle_middle + bend * height
    new_wrist = shoulder + along * distance

    # Each bone turns by the smallest rotation onto its new direction (its twist stays as the clip had it)
    upper_turn = (elbow - shoulder).rotation_difference(new_elbow - shoulder)
    upper_rotation = upper_turn @ world_matrix(armature, upper).to_quaternion()
    # The elbow is a hinge: the forearm keeps its rest turn against the upper arm and only bends (any roll of the hand
    # is the wrist's, spread_twist shares it with the forearm afterwards)
    rest_upper_to_lower = rest_world_matrix(armature, upper).to_quaternion().inverted() @ rest_world_matrix(armature, lower).to_quaternion()
    hinged = upper_rotation @ rest_upper_to_lower
    lower_rotation = (hinged @ Vector((0.0, 1.0, 0.0))).rotation_difference(new_wrist - new_elbow) @ hinged

    # The forearm also rolls with the hand (a wrist can't twist, the forearm turns the hand): its roll as the hand
    # has it at rest, shared with the elbow, so the skin twists over the whole forearm instead of at the wrist
    hand_rotation = hand_target.to_quaternion()
    rest_hand_to_lower = rest_world_matrix(armature, hand).to_quaternion().inverted() @ rest_world_matrix(armature, lower).to_quaternion()
    rolled = hand_rotation @ rest_hand_to_lower
    rolled_direction = rolled @ Vector((0.0, 1.0, 0.0))
    rolled = rolled_direction.rotation_difference(new_wrist - new_elbow) @ rolled
    lower_rotation = lower_rotation.slerp(rolled, FOREARM_ROLL_SHARE)

    return {upper: upper_rotation, lower: lower_rotation, hand: hand_rotation}


# ========== Keying ==========


def key_rotations(armature, rotations, frame):
    """Keys the bones' wanted world rotations as their local rotations (their parents first)"""
    world_rotations = {}
    for bone in armature.data.bones:
        world_rotations[bone.name] = world_matrix(armature, bone.name).to_quaternion()
    world_rotations.update(rotations)

    for name in rotations:
        bone = armature.data.bones[name]
        pose_bone = armature.pose.bones[name]
        rotation = local_rotation(armature, bone, world_rotations)
        rotation.make_compatible(pose_bone.rotation_quaternion)
        pose_bone.rotation_quaternion = rotation
        pose_bone.keyframe_insert("rotation_quaternion", frame=frame)


def pin_hands(armature, action, pins):
    """Keys the pins into the action: every frame, each pinned arm (one after another) reaches its target"""
    scene = bpy.context.scene
    armature.animation_data.action = action
    first, last = (int(frame) for frame in action.frame_range)
    frames = max(last - first, 1)
    previous_turns = {}
    previous_bends = {}

    for frame in range(first, last + 1):
        progress = (frame - first) / frames
        for pin in pins:
            scene.frame_set(frame)
            weight = max(0.0, min(1.0, pin.weight(progress)))
            if weight <= 0.0:
                continue
            target = pin.target(armature, progress)
            if pin.free_turn:
                target, previous_turn = turn_for_straight_wrist(armature, pin, target, previous_turns.get(pin.side))
                previous_turns[pin.side] = previous_turn
            bends = []
            pinned = solve_arm(armature, pin.side, target, previous_bends.get(pin.side), bends)
            previous_bends[pin.side] = bends[0]
            blended = {name: world_matrix(armature, name).to_quaternion().slerp(rotation, weight)
                       for name, rotation in pinned.items()}
            key_rotations(armature, blended, frame)
            # The next pin (the other hand) sees this arm already moved
            bpy.context.view_layer.update()


# Turns tried around a haft (degrees) and how much a turn away from the previous frame's counts against it
# (in degrees of wrist bend per degree of turn), so the hand doesn't jump around the haft
TURN_STEP_DEGREES = 15
TURN_CHANGE_COST = 0.3
MOST_TURN_CHANGE_PER_FRAME = 30


def wrist_bend(armature, side, rotations):
    """How far the solved hand bends from its solved forearm (radians)"""
    hand, lower = rotations["Hand" + side], rotations["LowerArm" + side]
    straight = hand @ forearm_in_hand(armature, side)
    actual = -(lower @ Vector((0.0, 1.0, 0.0)))
    return straight.angle(actual)


def turn_for_straight_wrist(armature, pin, target, previous_turn):
    """The target turned around the haft (pin.free_turn) to where the arm reaching it bends its wrist the least"""
    axis_in_hand, point_in_hand = pin.free_turn
    axis = (target.to_quaternion() @ axis_in_hand).normalized()
    point = target @ point_in_hand
    best = None
    for degrees in range(-180, 180, TURN_STEP_DEGREES):
        if previous_turn is not None and abs((degrees - previous_turn + 180) % 360 - 180) > MOST_TURN_CHANGE_PER_FRAME:
            continue
        turn = Matrix.Translation(point) @ Matrix.Rotation(math.radians(degrees), 4, axis) @ Matrix.Translation(-point)
        turned = turn @ target
        cost = math.degrees(wrist_bend(armature, pin.side, solve_arm(armature, pin.side, turned)))
        if previous_turn is not None:
            change = abs((degrees - previous_turn + 180) % 360 - 180)
            cost += change * TURN_CHANGE_COST
        if best is None or cost < best[0]:
            best = (cost, turned, degrees)
    return best[1], best[2]


# ========== Targets ==========


def held_in_reference(armature, item_bone, side, reference_action, reference_frame=0):
    """The hand's place relative to a held item (the weapon in the other hand) in a reference pose,
    e.g. the left hand on the haft in the two-handed idle"""
    scene = bpy.context.scene
    previous = armature.animation_data.action
    armature.animation_data.action = reference_action
    scene.frame_set(reference_frame)
    relative = world_matrix(armature, item_bone).inverted() @ world_matrix(armature, "Hand" + side)
    armature.animation_data.action = previous
    return relative


def on_the_haft(item_bone, relative):
    """Target: the hand stays where it held the item in the reference pose, wherever the item goes now"""
    return lambda armature, progress: world_matrix(armature, item_bone) @ relative


def straighten_wrist(armature, actions, side, most_degrees):
    """Every frame the hand bends no further than 'most_degrees' from its forearm: the hand that holds a shield
    (the shield lies along the forearm, a bent wrist would push the forearm through it)"""
    scene = bpy.context.scene
    forearm = forearm_in_hand(armature, side)
    hand_name = "Hand" + side
    for action in actions:
        armature.animation_data.action = action
        first, last = (int(frame) for frame in action.frame_range)
        for frame in range(first, last + 1):
            scene.frame_set(frame)
            hand = world_matrix(armature, hand_name)
            straight = hand.to_quaternion() @ forearm
            actual = (world_matrix(armature, "LowerArm" + side).translation - hand.translation).normalized()
            bend = straight.angle(actual)
            if math.degrees(bend) <= most_degrees:
                continue
            share = 1.0 - math.radians(most_degrees) / bend
            turn = Quaternion().slerp(straight.rotation_difference(actual), share)
            key_rotations(armature, {hand_name: turn @ hand.to_quaternion()}, frame)


def left_fist_on_haft(armature, item_bone, left_grip, haft_at_rest, right_fist_at_rest, spacing, reference_action, reference_frame=0):
    """Where the left hand holds a two-handed weapon, relative to the weapon's bone: the middle of the left fist on the
    haft's line 'spacing' from the right fist, on the side where the reference pose has the left hand (motion capture of
    a great sword holds it below the right one, at the pommel: put above, the arm reached up the haft into the body),
    its hole along the haft (the thumb towards the head, like the right one), turned around the haft as the reference
    pose turns it"""
    scene = bpy.context.scene
    previous = armature.animation_data.action
    armature.animation_data.action = reference_action
    scene.frame_set(reference_frame)

    hand_rest = rest_world_matrix(armature, "Hand_L")
    along_in_hand = hand_rest.to_quaternion().inverted() @ left_grip.along
    center_in_hand = hand_rest.inverted() @ left_grip.center
    item_rest = rest_world_matrix(armature, item_bone)
    haft_in_item = item_rest.to_quaternion().inverted() @ haft_at_rest.normalized()
    right_in_item = item_rest.inverted() @ right_fist_at_rest

    item = world_matrix(armature, item_bone)
    reference_left = world_matrix(armature, "Hand_L") @ center_in_hand
    side = 1.0 if (reference_left - item @ right_in_item).dot(item.to_quaternion() @ haft_in_item) >= 0.0 else -1.0
    point_in_item = right_in_item + haft_in_item * spacing * side
    reference_rotation = world_matrix(armature, "Hand_L").to_quaternion()
    haft = item.to_quaternion() @ haft_in_item
    rotation = (reference_rotation @ along_in_hand).rotation_difference(haft) @ reference_rotation
    center = item @ point_in_item
    hand = Matrix.Translation(center - rotation @ center_in_hand) @ rotation.to_matrix().to_4x4()

    armature.animation_data.action = previous
    return item.inverted() @ hand


def hold_with_both_hands(armature, item_bone, actions, left_grip, haft_at_rest, right_fist_at_rest, spacing, reference_action="Idle",
                         aimed_actions=()):
    """Every action keeps the left fist on the haft of the two-handed weapon in the right hand (see left_fist_on_haft;
    a motion made for a sword, or by Text to Motion, would let it wander off the haft). In 'aimed_actions' (names:
    motions made by Text to Motion, whose right hand turns as if it held nothing) the haft first points at the clip's
    left hand. Motion capture keeps the weapon as its right hand holds it: aimed there too, idle, walk and run would
    each hold it differently and the weapon would jump whenever one changes into another"""
    relative = left_fist_on_haft(armature, item_bone, left_grip, haft_at_rest, right_fist_at_rest, spacing, bpy.data.actions[reference_action])
    item_rest = rest_world_matrix(armature, item_bone)
    haft_in_item = item_rest.to_quaternion().inverted() @ haft_at_rest.normalized()
    center_in_hand = rest_world_matrix(armature, "Hand_L").inverted() @ left_grip.center
    along_in_hand = rest_world_matrix(armature, "Hand_L").to_quaternion().inverted() @ left_grip.along
    for action in actions:
        if action.name in aimed_actions:
            aim_haft_between_hands(armature, action, item_bone, haft_in_item, center_in_hand)
        pin_hands(armature, action, [HandPin("_L", on_the_haft(item_bone, relative), free_turn=(along_in_hand, center_in_hand))])


# A haft points along the line between the clip's two hands only when they are this close (meters): further apart
# the clip holds the weapon in one hand
HANDS_ON_ONE_HAFT = 0.6
MOST_HAFT_TURN_DEGREES = 75.0
RIGHT_WRIST_MOST_BEND = 65.0


def aim_haft_between_hands(armature, action, item_bone, haft_in_item, left_center_in_hand):
    """The right wrist turns the weapon so its haft points through where the clip has the left fist: the clip's two hands
    (both on an imagined haft) decide how the weapon is held, not the right hand's turn alone (a motion made by
    Text to Motion turns that hand as if it held nothing)"""
    scene = bpy.context.scene
    armature.animation_data.action = action
    first, last = (int(frame) for frame in action.frame_range)
    for frame in range(first, last + 1):
        scene.frame_set(frame)
        item = world_matrix(armature, item_bone)
        haft = item.to_quaternion() @ haft_in_item
        between = (world_matrix(armature, "Hand_L") @ left_center_in_hand) - item.translation
        if not 0.03 < between.length < HANDS_ON_ONE_HAFT:
            continue
        wanted = between.normalized()
        if wanted.dot(haft) < 0.0:
            wanted = -wanted
        turn = haft.rotation_difference(wanted)
        if turn.angle > math.radians(MOST_HAFT_TURN_DEGREES):
            turn = Quaternion().slerp(turn, math.radians(MOST_HAFT_TURN_DEGREES) / turn.angle)

        # Only as far as the right wrist stays as straight as the clip had it (or within RIGHT_WRIST_MOST_BEND)
        hand = world_matrix(armature, "Hand_R").to_quaternion()
        lower = world_matrix(armature, "LowerArm_R").to_quaternion()
        bend_before = wrist_bend(armature, "_R", {"Hand_R": hand, "LowerArm_R": lower})
        allowed = max(bend_before, math.radians(RIGHT_WRIST_MOST_BEND))
        for share in (1.0, 0.75, 0.5, 0.25, 0.0):
            turned = Quaternion().slerp(turn, share) @ hand
            if share == 0.0 or wrist_bend(armature, "_R", {"Hand_R": turned, "LowerArm_R": lower}) <= allowed:
                break
        if share > 0.0:
            key_rotations(armature, {"Hand_R": turned}, frame)


def item_face_in_bone(armature, item_bone, face_at_rest):
    """The direction the face of a held item (a shield) looks at, in the space of its bone"""
    return rest_world_matrix(armature, item_bone).to_quaternion().inverted() @ face_at_rest


def hand_for_item(armature, side, item_bone, item_center, face_in_bone, wanted_face, wanted_center):
    """The hand matrix that turns the held item (by the smallest turn) to face 'wanted_face' with its middle at
    'wanted_center', from where the item is now"""
    hand = world_matrix(armature, "Hand" + side)
    item = world_matrix(armature, item_bone)
    face_now = item.to_quaternion() @ face_in_bone
    turn = face_now.rotation_difference(wanted_face)

    center_now = item @ item_center
    hand_to_center = turn @ (center_now - hand.translation)
    rotation = (turn @ hand.to_quaternion()).to_matrix().to_4x4()
    return Matrix.Translation(wanted_center - hand_to_center) @ rotation


def in_character_space(armature, point):
    return armature.matrix_world @ Vector(point)


def shield_in_front(item_bone, face_in_bone, center, face, item_center=Vector()):
    """Target: the shield held still before the body, its middle at 'center' and its face looking along 'face'
    (both in the character's space: X right, Y forward, Z up)"""
    def target(armature, progress):
        wanted_face = (armature.matrix_world.to_3x3() @ Vector(face)).normalized()
        return hand_for_item(armature, "_L", item_bone, item_center, face_in_bone, wanted_face, in_character_space(armature, center))
    return target


@dataclass
class ShieldPose:
    """A key pose of the shield: at 'progress' of the clip its middle is at 'center' and its face looks along 'face'"""
    progress: float
    center: tuple
    face: tuple


def smooth(t):
    return t * t * (3.0 - 2.0 * t)


def shield_path(item_bone, face_in_bone, poses, item_center=Vector()):
    """Target: the shield goes through the key poses (eased from one to the next)"""
    def target(armature, progress):
        if progress <= poses[0].progress:
            before, after, t = poses[0], poses[0], 0.0
        elif progress >= poses[-1].progress:
            before, after, t = poses[-1], poses[-1], 0.0
        else:
            index = next(i for i in range(len(poses) - 1) if poses[i].progress <= progress <= poses[i + 1].progress)
            before, after = poses[index], poses[index + 1]
            t = smooth((progress - before.progress) / (after.progress - before.progress))
        center = Vector(before.center).lerp(Vector(after.center), t)
        face = Vector(before.face).normalized().slerp(Vector(after.face).normalized(), t) \
            if Vector(before.face).normalized().dot(Vector(after.face).normalized()) > -0.99 else Vector(after.face)
        wanted_face = (armature.matrix_world.to_3x3() @ face).normalized()
        return hand_for_item(armature, "_L", item_bone, item_center, face_in_bone, wanted_face, in_character_space(armature, center))
    return target


def fade(fade_in_until=0.0, fade_out_from=1.0):
    """Weight: grows from 0 to 1 until 'fade_in_until', falls back to 0 after 'fade_out_from'"""
    def weight(progress):
        if fade_in_until > 0.0 and progress < fade_in_until:
            return smooth(progress / fade_in_until)
        if fade_out_from < 1.0 and progress > fade_out_from:
            return smooth(1.0 - (progress - fade_out_from) / (1.0 - fade_out_from))
        return 1.0
    return weight


# ========== A staff held upright ==========


def frame_matrix(first, second):
    """A rotation from two directions: the first exactly, the second as close as it can be"""
    first = first.normalized()
    third = first.cross(second).normalized()
    second = third.cross(first)
    return Matrix((first, second, third)).transposed()


def staff_upright(grip, side, rest_point, follow_clip=0.0):
    """Target: the hand holds a staff upright (its hole along the world's up), before the body at 'rest_point'
    (the character's space at rest, moving with the spine), or 'follow_clip' of the way to where the clip has the fist
    (a cast or a thrust pushes the staff along). Use with free_turn: the hand turns around the staff for a straight wrist"""
    hand = "Hand" + side

    def target(armature, progress):
        hand_rest = rest_world_matrix(armature, hand)
        along_in_hand = hand_rest.to_quaternion().inverted() @ grip.along
        fingers_in_hand = hand_rest.to_quaternion().inverted() @ grip.fingers
        center_in_hand = hand_rest.inverted() @ grip.center

        spine_change = world_matrix(armature, "Spine") @ rest_world_matrix(armature, "Spine").inverted()
        center = (spine_change @ (armature.matrix_world @ Vector(rest_point))).lerp(world_matrix(armature, hand) @ center_in_hand, follow_clip)
        forward = spine_change.to_3x3() @ Vector((0.0, 1.0, 0.0))
        forward.z = 0.0

        wanted = frame_matrix(Vector((0.0, 0.0, 1.0)), forward)
        current = frame_matrix(along_in_hand, fingers_in_hand)
        rotation = (wanted @ current.transposed()).to_4x4()
        return Matrix.Translation(center - rotation.to_3x3() @ center_in_hand) @ rotation
    return target


def staff_free_turn(armature, grip, side):
    """free_turn of a HandPin: around the staff (the fist's hole) through the middle of the fist"""
    hand_rest = rest_world_matrix(armature, "Hand" + side)
    return hand_rest.to_quaternion().inverted() @ grip.along, hand_rest.inverted() @ grip.center


# ========== The body joining an arm's motion ==========


def curve(keys):
    """A value over the clip from (progress, value) keys, eased from one to the next"""
    def value(progress):
        if progress <= keys[0][0]:
            return keys[0][1]
        for (start, first), (end, second) in zip(keys, keys[1:]):
            if progress <= end:
                return first + (second - first) * smooth((progress - start) / (end - start))
        return keys[-1][1]
    return value


def add_turn(armature, action, bone_name, axis, degrees):
    """Turns a bone (and everything it carries) around a direction of the character's space by 'degrees(progress)'
    on top of the clip: the upper body twisting into a parry. Pin the hands afterwards, so they still reach their targets"""
    scene = bpy.context.scene
    armature.animation_data.action = action
    first, last = (int(frame) for frame in action.frame_range)
    world_axis = (armature.matrix_world.to_3x3() @ Vector(axis)).normalized()
    for frame in range(first, last + 1):
        scene.frame_set(frame)
        angle = math.radians(degrees((frame - first) / max(last - first, 1)))
        turned = Quaternion(world_axis, angle) @ world_matrix(armature, bone_name).to_quaternion()
        key_rotations(armature, {bone_name: turned}, frame)


def add_hips_drop(armature, action, meters):
    """Lowers the hips by 'meters(progress)' on top of the clip (bending the knees into a parry or a blow)"""
    data_path = 'pose.bones["Hips"].location'
    rest = rest_world_matrix(armature, "Hips").to_quaternion().inverted()
    scene = bpy.context.scene
    armature.animation_data.action = action
    first, last = (int(frame) for frame in action.frame_range)
    hips = armature.pose.bones["Hips"]
    for frame in range(first, last + 1):
        scene.frame_set(frame)
        hips.location = hips.location + rest @ Vector((0.0, 0.0, -meters((frame - first) / max(last - first, 1))))
        hips.keyframe_insert("location", frame=frame)


# ========== Twist spread along the arm ==========

# A real arm turns the hand with the forearm (both bones of the forearm roll) and the forearm with the upper arm (the
# shoulder): the clips (motion capture) put the whole turn into the hand, and our skeleton has no twist bones, so the
# skin of the wrist screwed up like a candy wrapper. These parts of the twist move up the arm
HAND_TWIST_TO_FOREARM = 0.7
FOREARM_TWIST_TO_UPPER_ARM = 0.65


def signed_twist(armature, bone_name):
    """How far the bone is turned around its own length against its parent, compared to the rest pose (radians, signed)"""
    bone = armature.data.bones[bone_name]
    rest_local = bone.parent.matrix_local.inverted() @ bone.matrix_local
    pose_local = armature.pose.bones[bone.parent.name].matrix.inverted() @ armature.pose.bones[bone_name].matrix
    change = (rest_local.to_quaternion().inverted() @ pose_local.to_quaternion()).normalized()
    if change.w < 0.0:
        change.negate()
    return 2.0 * math.atan2(change.y, change.w)


def unwrapped(angle, previous):
    """The angle (radians) moved by whole turns to be closest to 'previous': a twist past 180 degrees stays on its side"""
    if previous is None:
        return angle
    while angle - previous > math.pi:
        angle -= 2.0 * math.pi
    while angle - previous < -math.pi:
        angle += 2.0 * math.pi
    return angle


def spread_twist(armature, actions):
    """Every frame: part of the hand's twist goes into the forearm, part of the forearm's into the upper arm (each turned
    around its own length), while the hand and the held weapon stay exactly where they were"""
    scene = bpy.context.scene
    for action in actions:
        armature.animation_data.action = action
        first, last = (int(frame) for frame in action.frame_range)
        previous = {}
        for frame in range(first, last + 1):
            for side in ("_L", "_R"):
                upper, lower, hand = arm_bones(side)
                scene.frame_set(frame)
                upper_rotation = world_matrix(armature, upper).to_quaternion()
                lower_rotation = world_matrix(armature, lower).to_quaternion()
                hand_rotation = world_matrix(armature, hand).to_quaternion()

                forearm_axis = (lower_rotation @ Vector((0.0, 1.0, 0.0))).normalized()
                hand_twist = unwrapped(signed_twist(armature, hand), previous.get(hand))
                previous[hand] = hand_twist
                lower_rotation = Quaternion(forearm_axis, hand_twist * HAND_TWIST_TO_FOREARM) @ lower_rotation
                key_rotations(armature, {lower: lower_rotation, hand: hand_rotation}, frame)

                scene.frame_set(frame)
                upper_axis = (upper_rotation @ Vector((0.0, 1.0, 0.0))).normalized()
                forearm_twist = unwrapped(signed_twist(armature, lower), previous.get(lower))
                previous[lower] = forearm_twist
                upper_rotation = Quaternion(upper_axis, forearm_twist * FOREARM_TWIST_TO_UPPER_ARM) @ upper_rotation
                key_rotations(armature, {upper: upper_rotation, lower: lower_rotation, hand: hand_rotation}, frame)


def straight_hands(armature, actions):
    """Both hands go on straight from their forearms, as at rest: for motions made by Text to Motion, whose wrists
    turn at random (single frames flip the hand half around) because the body model holds nothing"""
    scene = bpy.context.scene
    for action in actions:
        armature.animation_data.action = action
        first, last = (int(frame) for frame in action.frame_range)
        for frame in range(first, last + 1):
            scene.frame_set(frame)
            rotations = {}
            for side in ("_L", "_R"):
                _, lower, hand = arm_bones(side)
                rest_lower_to_hand = rest_world_matrix(armature, lower).to_quaternion().inverted() @ rest_world_matrix(armature, hand).to_quaternion()
                rotations[hand] = world_matrix(armature, lower).to_quaternion() @ rest_lower_to_hand
            key_rotations(armature, rotations, frame)


# An arm bone of a motion made by Text to Motion turns at most this far between two frames (degrees): a jerk of the
# generated data (a whole arm turning in one frame) is spread over the next frames instead
MOST_ARM_TURN_PER_FRAME = 45.0
ARM_CHAIN = ("UpperArm_L", "LowerArm_L", "Hand_L", "UpperArm_R", "LowerArm_R", "Hand_R")


def smooth_jerks(armature, actions, most_degrees=MOST_ARM_TURN_PER_FRAME):
    scene = bpy.context.scene
    most = math.radians(most_degrees)
    for action in actions:
        armature.animation_data.action = action
        first, last = (int(frame) for frame in action.frame_range)
        previous = None
        for frame in range(first, last + 1):
            scene.frame_set(frame)
            wanted = {name: world_matrix(armature, name).to_quaternion() for name in ARM_CHAIN}
            if previous:
                for name, rotation in wanted.items():
                    change = previous[name].rotation_difference(rotation)
                    if change.angle > most and change.angle < 2.0 * math.pi - most:
                        wanted[name] = previous[name].slerp(rotation, most / change.angle)
                key_rotations(armature, wanted, frame)
            previous = wanted
