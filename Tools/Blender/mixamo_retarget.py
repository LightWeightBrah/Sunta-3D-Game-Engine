"""
Moves Mixamo animations onto the Sunta knight skeleton (see knight_generator.py)

A Mixamo file is downloaded as "FBX Binary, Without Skin" (only the Mixamo skeleton and its motion)
Every knight bone copies the world rotation of one Mixamo bone. The two skeletons stand differently at rest
(Mixamo: T-pose, the knight: arms down), so each bone gets a fixed correction made from the rest poses:
when the Mixamo bone points somewhere, the knight bone points the same way
Only the hips move: up and down with the motion, never along the ground (the engine moves the character itself)

The raw Mixamo files are not committed (their license forbids sharing them), knight_generator.py lists which ones it needs
"""

import math
import os
from dataclasses import dataclass

import bpy
from mathutils import Matrix, Vector

# The knight bone and the Mixamo bone whose rotation it copies (Spine and Spine2 are skipped on purpose:
# copying world rotations, the knight's two spine bones take the middle and the top of the Mixamo spine)
MIXAMO_BONE_OF = {
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
}

ROOT_BONE = "Hips"

# Mixamo characters look along -Y in Blender, the knight along +Y
TURN_TO_KNIGHT_FORWARD = Matrix.Rotation(math.pi, 4, "Z")

MIXAMO_FPS = 30


@dataclass
class MixamoClip:
    """One animation taken from a Mixamo file: the whole file or a part of it (Mixamo frames, inclusive)"""
    file_name: str
    first_frame: int = None
    last_frame: int = None
    speed: float = 1.0


# ========== Rotations of the bones (world space, the armature objects included) ==========


def world_rest_rotation(armature, bone_name):
    return (armature.matrix_world @ armature.data.bones[bone_name].matrix_local).to_quaternion()


def world_pose_rotation(armature, bone_name):
    return (armature.matrix_world @ armature.pose.bones[bone_name].matrix).to_quaternion()


def world_rest_direction(armature, bone_name):
    bone = armature.data.bones[bone_name]
    return (armature.matrix_world @ bone.tail_local - armature.matrix_world @ bone.head_local).normalized()


def make_rest_corrections(mixamo, knight):
    """Per knight bone: the rotation added after the Mixamo bone's rotation, so that the Mixamo rest pose
    turned onto the knight's bone directions gives the knight's rest pose"""
    corrections = {}
    for knight_bone, mixamo_bone in MIXAMO_BONE_OF.items():
        to_knight_direction = world_rest_direction(mixamo, mixamo_bone).rotation_difference(world_rest_direction(knight, knight_bone))
        corrections[knight_bone] = (world_rest_rotation(mixamo, mixamo_bone).inverted()
                                    @ to_knight_direction.inverted()
                                    @ world_rest_rotation(knight, knight_bone))
    return corrections


# ========== Loading a Mixamo file ==========


def load_mixamo_armature(path):
    if not os.path.isfile(path):
        raise FileNotFoundError("Mixamo animation not found: {} (knight_generator.py lists the files to download)".format(path))

    # The FBX importer sets the scene to the file's frame rate, the knight's animations keep the scene's own
    scene = bpy.context.scene
    fps, fps_base = scene.render.fps, scene.render.fps_base

    objects_before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    scene.render.fps, scene.render.fps_base = fps, fps_base
    imported = [obj for obj in bpy.data.objects if obj not in objects_before]

    armature = next(obj for obj in imported if obj.type == "ARMATURE")
    for obj in imported:
        if obj is not armature:
            bpy.data.objects.remove(obj)

    armature.matrix_world = TURN_TO_KNIGHT_FORWARD @ armature.matrix_world
    bpy.context.view_layer.update()
    return armature


def remove_mixamo_armature(armature):
    action = armature.animation_data.action if armature.animation_data else None
    data = armature.data
    bpy.data.objects.remove(armature)
    bpy.data.armatures.remove(data)
    if action:
        bpy.data.actions.remove(action)


# ========== Turning world rotations into the knight's bone keys ==========


def parent_first(knight):
    """The knight bones in an order where every parent comes before its children"""
    ordered = []

    def visit(bone):
        ordered.append(bone)
        for child in bone.children:
            visit(child)

    for bone in knight.data.bones:
        if bone.parent is None:
            visit(bone)
    return ordered


def local_rotation(knight, bone, world_rotations):
    """The bone's own key rotation that gives it the wanted world rotation under its (already posed) parent"""
    rest = world_rest_rotation(knight, bone.name)
    wanted = world_rotations[bone.name]
    if bone.parent is None:
        return rest.inverted() @ wanted

    parent_rest = world_rest_rotation(knight, bone.parent.name)
    parent_pose = world_rotations[bone.parent.name]
    return rest.inverted() @ parent_rest @ parent_pose.inverted() @ wanted


def knight_world_rotations(mixamo, knight, corrections):
    """The world rotation of every knight bone in the current Mixamo frame. A bone without a Mixamo bone
    (the sword bone) keeps its place under its parent"""
    rotations = {}
    for bone in parent_first(knight):
        if bone.name in corrections:
            rotations[bone.name] = world_pose_rotation(mixamo, MIXAMO_BONE_OF[bone.name]) @ corrections[bone.name]
        else:
            parent_change = rotations[bone.parent.name] @ world_rest_rotation(knight, bone.parent.name).inverted()
            rotations[bone.name] = parent_change @ world_rest_rotation(knight, bone.name)
    return rotations


def hips_offset(mixamo, knight):
    """How far the hips are above or below their rest height, scaled to the knight's size (never along the ground)"""
    mixamo_rest = mixamo.matrix_world @ mixamo.data.bones[MIXAMO_BONE_OF[ROOT_BONE]].head_local
    mixamo_now = mixamo.matrix_world @ mixamo.pose.bones[MIXAMO_BONE_OF[ROOT_BONE]].head
    knight_rest_height = (knight.matrix_world @ knight.data.bones[ROOT_BONE].head_local).z

    return Vector((0.0, 0.0, (mixamo_now.z - mixamo_rest.z) * knight_rest_height / mixamo_rest.z))


def key_frame(knight, rotations, offset, frame, previous_rotations):
    for bone in knight.data.bones:
        pose_bone = knight.pose.bones[bone.name]
        rotation = local_rotation(knight, bone, rotations)

        # Neighbouring keys on the same side of the quaternion sphere: a fast turn goes the short way
        if bone.name in previous_rotations:
            rotation.make_compatible(previous_rotations[bone.name])
        previous_rotations[bone.name] = rotation.copy()

        pose_bone.rotation_mode = "QUATERNION"
        pose_bone.rotation_quaternion = rotation
        pose_bone.keyframe_insert("rotation_quaternion", frame=frame)

    hips = knight.pose.bones[ROOT_BONE]
    hips.location = world_rest_rotation(knight, ROOT_BONE).inverted() @ offset
    hips.keyframe_insert("location", frame=frame)


def mixamo_frame_range(mixamo, clip):
    first, last = mixamo.animation_data.action.frame_range
    return (clip.first_frame if clip.first_frame is not None else int(first),
            clip.last_frame if clip.last_frame is not None else int(last))


def retarget(knight, clip, folder, action):
    """Keys 'action' on the knight with the Mixamo clip, at the scene's frame rate. Returns the last frame"""
    mixamo = load_mixamo_armature(os.path.join(folder, clip.file_name))
    corrections = make_rest_corrections(mixamo, knight)

    scene = bpy.context.scene
    first, last = mixamo_frame_range(mixamo, clip)
    mixamo_frames_per_knight_frame = MIXAMO_FPS * clip.speed / scene.render.fps
    knight_frames = int((last - first) / mixamo_frames_per_knight_frame)

    knight.animation_data.action = action
    previous_rotations = {}

    for frame in range(knight_frames + 1):
        mixamo_frame = first + frame * mixamo_frames_per_knight_frame
        scene.frame_set(int(mixamo_frame), subframe=mixamo_frame - int(mixamo_frame))

        rotations = knight_world_rotations(mixamo, knight, corrections)
        key_frame(knight, rotations, hips_offset(mixamo, knight), frame, previous_rotations)

    remove_mixamo_armature(mixamo)
    return knight_frames
