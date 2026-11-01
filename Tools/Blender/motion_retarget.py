"""
Moves motion clips onto the skeleton of the engine's characters (the knight's bone names, see knight_generator.SKELETON)

Clips come from Mixamo (downloaded as "FBX Binary, Without Skin": the Mixamo skeleton and its motion), from Meshy's
animation library (an FBX with the skeleton of a character Meshy rigged, several clips in one file) or from Meshy's
Text to Motion (an FBX with an SMPL skeleton). Each kind of skeleton is a Rig below: which of its bones drives
which bone of ours, and which joint each of those bones points at

Every bone of ours copies the world rotation of its bone in the clip. The two skeletons stand differently at rest
(the clips: a T pose, ours: an A pose or arms down), so each bone gets a fixed correction made from the rest poses:
when the clip's limb points somewhere, our limb points the same way
Only the hips move: up and down with the motion, never along the ground (the engine moves the character itself)

The raw Mixamo files are not committed (their license forbids sharing them), knight_generator.py lists which ones it needs
"""

import math
import os
from dataclasses import dataclass, field

import bpy
from mathutils import Matrix, Quaternion, Vector

BLENDER_FOLDER = os.path.dirname(os.path.abspath(__file__))

ROOT_BONE = "Hips"


@dataclass(frozen=True)
class Rig:
    """A kind of skeleton in clip files. 'bone_of': our bone -> its bone. 'aims_at': its bone -> the bone whose head
    shows where it points (its own tail when missing: Mixamo's bones lie along the limbs, SMPL's don't)"""
    bone_of: dict
    aims_at: dict = field(default_factory=dict)
    # Characters in both kinds of file look along -Y in Blender, ours along +Y
    turn_degrees: float = 180.0


MIXAMO = Rig(bone_of={
    "Hips":       "mixamorig:Hips",
    "Spine":      "mixamorig:Spine1",
    "Chest":      "mixamorig:Spine2",
    "Neck":       "mixamorig:Neck",
    "Head":       "mixamorig:Head",
    "Shoulder_R": "mixamorig:RightShoulder",
    "UpperArm_R": "mixamorig:RightArm",
    "LowerArm_R": "mixamorig:RightForeArm",
    "Hand_R":     "mixamorig:RightHand",
    "Shoulder_L": "mixamorig:LeftShoulder",
    "UpperArm_L": "mixamorig:LeftArm",
    "LowerArm_L": "mixamorig:LeftForeArm",
    "Hand_L":     "mixamorig:LeftHand",
    "Thigh_R":    "mixamorig:RightUpLeg",
    "Shin_R":     "mixamorig:RightLeg",
    "Foot_R":     "mixamorig:RightFoot",
    "Thigh_L":    "mixamorig:LeftUpLeg",
    "Shin_L":     "mixamorig:LeftLeg",
    "Foot_L":     "mixamorig:LeftFoot",
})


# Characters rigged by Meshy (its library animations come on them): Mixamo's names without the prefix, a longer spine
MESHY = Rig(bone_of={
    "Hips":       "Hips",
    "Spine":      "Spine01",
    "Chest":      "Spine",
    "Neck":       "neck",
    "Head":       "Head",
    "Shoulder_R": "RightShoulder",
    "UpperArm_R": "RightArm",
    "LowerArm_R": "RightForeArm",
    "Hand_R":     "RightHand",
    "Shoulder_L": "LeftShoulder",
    "UpperArm_L": "LeftArm",
    "LowerArm_L": "LeftForeArm",
    "Hand_L":     "LeftHand",
    "Thigh_R":    "RightUpLeg",
    "Shin_R":     "RightLeg",
    "Foot_R":     "RightFoot",
    "Thigh_L":    "LeftUpLeg",
    "Shin_L":     "LeftLeg",
    "Foot_L":     "LeftFoot",
})


def smpl_side(side):
    return {
        "Shoulder_" + side: side + "_Collar",
        "UpperArm_" + side: side + "_Shoulder",
        "LowerArm_" + side: side + "_Elbow",
        "Hand_" + side:     side + "_Wrist",
        "Thigh_" + side:    side + "_Hip",
        "Shin_" + side:     side + "_Knee",
        "Foot_" + side:     side + "_Ankle",
    }


def smpl_aims(side):
    return {
        side + "_Collar":   side + "_Shoulder",
        side + "_Shoulder": side + "_Elbow",
        side + "_Elbow":    side + "_Wrist",
        side + "_Wrist":    side + "_Middle1",
        side + "_Hip":      side + "_Knee",
        side + "_Knee":     side + "_Ankle",
        side + "_Ankle":    side + "_Foot",
    }


# Meshy's Text to Motion clips (the SMPL body model's joints)
SMPL = Rig(
    bone_of=dict({"Hips": "Pelvis", "Spine": "Spine2", "Chest": "Spine3", "Neck": "Neck", "Head": "Head"},
                 **smpl_side("R"), **smpl_side("L")),
    aims_at=dict({"Pelvis": "Spine1", "Spine2": "Spine3", "Spine3": "Neck", "Neck": "Head"}, **smpl_aims("R"), **smpl_aims("L")),
)


@dataclass
class MotionClip:
    """One animation taken from a clip file (a path from Tools/Blender): the whole file or a part of it
    (the file's frames, inclusive), played at some speed. 'action' picks one clip of a file that has several"""
    file_name: str
    first_frame: int = None
    last_frame: int = None
    speed: float = 1.0
    rig: Rig = MIXAMO
    action: str = None
    # The character keeps facing where it faced in the first frame (a motion that turns the whole body around,
    # e.g. a parry made by Text to Motion, would otherwise face away from the enemy in the middle of it)
    keeps_facing: bool = False


# ========== Rotations and directions of the bones (world space, the armature objects included) ==========


def world_rest_rotation(armature, bone_name):
    return (armature.matrix_world @ armature.data.bones[bone_name].matrix_local).to_quaternion()


def world_pose_rotation(armature, bone_name):
    return (armature.matrix_world @ armature.pose.bones[bone_name].matrix).to_quaternion()


def world_rest_head(armature, bone_name):
    return armature.matrix_world @ armature.data.bones[bone_name].head_local


def world_rest_direction(armature, bone_name, aims_at=None):
    """Where the bone points at rest: towards the joint it aims at, or along itself"""
    bone = armature.data.bones[bone_name]
    target = aims_at.get(bone_name) if aims_at else None
    end = world_rest_head(armature, target) if target else armature.matrix_world @ bone.tail_local
    return (end - world_rest_head(armature, bone_name)).normalized()


def make_rest_corrections(source, rig, character):
    """Per bone of ours: the rotation added after its clip bone's rotation, so that the clip's rest pose turned onto
    our bone directions gives our rest pose"""
    corrections = {}
    for our_bone, their_bone in rig.bone_of.items():
        to_our_direction = world_rest_direction(source, their_bone, rig.aims_at).rotation_difference(world_rest_direction(character, our_bone))
        corrections[our_bone] = (world_rest_rotation(source, their_bone).inverted()
                                 @ to_our_direction.inverted()
                                 @ world_rest_rotation(character, our_bone))
    return corrections


# ========== Loading a clip file ==========


def load_source_armature(path, rig, action_name=None):
    """The clip's armature turned to look along +Y (playing the chosen clip), and the frame rate of the file"""
    if not os.path.isfile(path):
        raise FileNotFoundError("Motion clip not found: {} (knight_generator.py lists the Mixamo files to download)".format(path))

    # The FBX importer sets the scene to the file's frame rate: it is read, then the scene gets its own back
    scene = bpy.context.scene
    fps, fps_base = scene.render.fps, scene.render.fps_base

    objects_before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    source_fps = scene.render.fps / scene.render.fps_base
    scene.render.fps, scene.render.fps_base = fps, fps_base

    imported = [obj for obj in bpy.data.objects if obj not in objects_before]
    armature = next(obj for obj in imported if obj.type == "ARMATURE")
    for obj in imported:
        if obj is not armature:
            bpy.data.objects.remove(obj)

    if action_name:
        armature.animation_data.action = next(action for action in bpy.data.actions if action.name.endswith(action_name))

    armature.matrix_world = Matrix.Rotation(math.radians(rig.turn_degrees), 4, "Z") @ armature.matrix_world
    bpy.context.view_layer.update()
    return armature, source_fps


def remove_source_armature(armature, actions_before):
    """The clip's armature and every action its file brought"""
    data = armature.data
    bpy.data.objects.remove(armature)
    bpy.data.armatures.remove(data)
    for action in [action for action in bpy.data.actions if action not in actions_before]:
        bpy.data.actions.remove(action)


# ========== Turning world rotations into our bone keys ==========


def parent_first(character):
    """Our bones in an order where every parent comes before its children"""
    ordered = []

    def visit(bone):
        ordered.append(bone)
        for child in bone.children:
            visit(child)

    for bone in character.data.bones:
        if bone.parent is None:
            visit(bone)
    return ordered


def local_rotation(character, bone, world_rotations):
    """The bone's own key rotation that gives it the wanted world rotation under its (already posed) parent"""
    rest = world_rest_rotation(character, bone.name)
    wanted = world_rotations[bone.name]
    if bone.parent is None:
        return rest.inverted() @ wanted

    parent_rest = world_rest_rotation(character, bone.parent.name)
    parent_pose = world_rotations[bone.parent.name]
    return rest.inverted() @ parent_rest @ parent_pose.inverted() @ wanted


def character_world_rotations(source, rig, character, corrections):
    """The world rotation of each of our bones in the current frame. A bone without a clip bone (the weapon bone)
    keeps its place under its parent"""
    rotations = {}
    for bone in parent_first(character):
        if bone.name in corrections:
            rotations[bone.name] = world_pose_rotation(source, rig.bone_of[bone.name]) @ corrections[bone.name]
        else:
            parent_change = rotations[bone.parent.name] @ world_rest_rotation(character, bone.parent.name).inverted()
            rotations[bone.name] = parent_change @ world_rest_rotation(character, bone.name)
    return rotations


def heading(source, rig):
    """How far the clip's hips are turned around the up axis (radians). Measured with the hip bone's side axis:
    a bone points along its Y axis, which for the hips is mostly up"""
    hips = source.pose.bones[rig.bone_of[ROOT_BONE]]
    side = (source.matrix_world.to_3x3() @ hips.matrix.to_3x3()) @ Vector((1.0, 0.0, 0.0))
    return math.atan2(side.y, side.x)


def turned_back(rotations, angle):
    """Every world rotation turned by 'angle' around the up axis"""
    turn = Quaternion((0.0, 0.0, 1.0), angle)
    return {name: turn @ rotation for name, rotation in rotations.items()}


def lowest_rest_point(armature):
    return min((armature.matrix_world @ bone.head_local).z for bone in armature.data.bones)


def hips_offset(source, rig, character):
    """How far the hips are above or below their rest height, scaled to our character's size (never along the ground)"""
    their_hips = rig.bone_of[ROOT_BONE]
    rest = world_rest_head(source, their_hips)
    now = source.matrix_world @ source.pose.bones[their_hips].head
    their_hips_height = rest.z - lowest_rest_point(source)
    our_hips_height = world_rest_head(character, ROOT_BONE).z - lowest_rest_point(character)

    return Vector((0.0, 0.0, (now.z - rest.z) * our_hips_height / their_hips_height))


def leg_bones(character):
    """The bones below the thighs (shins, feet, toes): what touches the ground"""
    return [bone.name for bone in character.data.bones
            if any(parent.name.startswith("Thigh") for parent in bone.parent_recursive)]


def lowest_leg_point(character, is_rest=False):
    points = []
    for name in leg_bones(character):
        if is_rest:
            bone = character.data.bones[name]
            points += [bone.head_local, bone.tail_local]
        else:
            bone = character.pose.bones[name]
            points += [bone.head, bone.tail]
    return min((character.matrix_world @ point).z for point in points)


def action_curves(action):
    """Every animation curve of an action (Blender 4.4+ keeps them in layers, strips and channel bags)"""
    if not hasattr(action, "layers"):
        return list(action.fcurves)
    return [curve for layer in action.layers for strip in layer.strips for bag in strip.channelbags for curve in bag.fcurves]


def put_on_ground(character, action, frames):
    """Moves the whole clip up or down so that its lowest step is on the ground, like the feet at rest.
    Clips whose skeleton stands at another height than its rest pose (Meshy's Text to Motion) would float or sink"""
    scene = bpy.context.scene
    lowest = math.inf
    for frame in range(frames + 1):
        scene.frame_set(frame)
        lowest = min(lowest, lowest_leg_point(character))
    rise = lowest_leg_point(character, is_rest=True) - lowest
    if abs(rise) < 0.005:
        return

    local_rise = world_rest_rotation(character, ROOT_BONE).inverted() @ Vector((0.0, 0.0, rise))
    data_path = 'pose.bones["{}"].location'.format(ROOT_BONE)
    for curve in [curve for curve in action_curves(action) if curve.data_path == data_path]:
        for point in curve.keyframe_points:
            for coordinates in (point.co, point.handle_left, point.handle_right):
                coordinates[1] += local_rise[curve.array_index]
        curve.update()


def key_frame(character, rotations, offset, frame, previous_rotations):
    for bone in character.data.bones:
        pose_bone = character.pose.bones[bone.name]
        rotation = local_rotation(character, bone, rotations)

        # Neighbouring keys on the same side of the quaternion sphere: a fast turn goes the short way
        if bone.name in previous_rotations:
            rotation.make_compatible(previous_rotations[bone.name])
        previous_rotations[bone.name] = rotation.copy()

        pose_bone.rotation_mode = "QUATERNION"
        pose_bone.rotation_quaternion = rotation
        pose_bone.keyframe_insert("rotation_quaternion", frame=frame)

    hips = character.pose.bones[ROOT_BONE]
    hips.location = world_rest_rotation(character, ROOT_BONE).inverted() @ offset
    hips.keyframe_insert("location", frame=frame)


def clip_frame_range(source, clip):
    first, last = source.animation_data.action.frame_range
    return (clip.first_frame if clip.first_frame is not None else int(first),
            clip.last_frame if clip.last_frame is not None else int(last))


def retarget(character, clip, action):
    """Keys 'action' on our character with the clip, at the scene's frame rate. Returns the last frame"""
    actions_before = set(bpy.data.actions)
    source, source_fps = load_source_armature(os.path.join(BLENDER_FOLDER, clip.file_name), clip.rig, clip.action)
    corrections = make_rest_corrections(source, clip.rig, character)

    scene = bpy.context.scene
    first, last = clip_frame_range(source, clip)
    source_frames_per_frame = source_fps * clip.speed / (scene.render.fps / scene.render.fps_base)
    frames = int((last - first) / source_frames_per_frame)

    character.animation_data.action = action
    previous_rotations = {}
    first_heading = None

    for frame in range(frames + 1):
        source_frame = first + frame * source_frames_per_frame
        scene.frame_set(int(source_frame), subframe=source_frame - int(source_frame))

        rotations = character_world_rotations(source, clip.rig, character, corrections)
        if clip.keeps_facing:
            now = heading(source, clip.rig)
            first_heading = now if first_heading is None else first_heading
            rotations = turned_back(rotations, first_heading - now)
        key_frame(character, rotations, hips_offset(source, clip.rig, character), frame, previous_rotations)

    remove_source_armature(source, actions_before | {action})
    put_on_ground(character, action, frames)
    return frames
