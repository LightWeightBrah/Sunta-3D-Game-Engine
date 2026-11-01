"""
Turns a generated character mesh (Meshy, TRELLIS, ... in an A pose) into a character of the engine:
the mesh stood up at a given height, its textures turned into the engine's diffuse and specular maps,
a skeleton with the knight's bone names fitted into it, the mesh weighted to the bones, the Mixamo animations added

The skeleton is found in the mesh itself: in an A pose the arms and the legs stand apart from the body, so a horizontal
slice of the mesh at the height of the knees cuts two separate pieces (the legs), at the height of the hands three or more
(an arm, the body, an arm). The middles of those pieces are the joints. The spine, the neck and the head keep the
proportions of the knight's skeleton (knight_generator.SKELETON)

A character that comes already rigged (Meshy's Rigging) keeps its own bones and weights: load_rigged_character
only renames the bones to ours, which is better than Blender's automatic weights on a generated mesh

Used by characters_generator.py (the knights) and boss_generator.py
"""

import math

import bpy
import numpy
from mathutils import Matrix, Vector

import knight_generator
from sunta_blender import make_textured_material, select_only

FORWARD = knight_generator.FORWARD
UP = knight_generator.UP

# The knight's skeleton is 1.82 m tall (the top of the head): its proportions are the template
TEMPLATE_HEIGHT = 1.82

# Thin slices of the mesh, and how far apart two pieces of a slice must be to count as separate (parts of the height)
SLICE_THICKNESS = 0.01
PIECE_GAP = 0.02

# Heights as parts of the whole height (from the template)
HIPS_HEIGHT = 0.54
SPINE_HEIGHT = 0.605
CHEST_HEIGHT = 0.70
NECK_HEIGHT = 0.82
HEAD_HEIGHT = 0.88
SHOULDER_HEIGHT = 0.80
KNEE_HEIGHT = 0.29
ANKLE_HEIGHT = 0.055

# How far the knee stands in front of the leg's line, how long the foot is (parts of the height)
KNEE_FORWARD = 0.008
FOOT_LENGTH = 0.085


# ========== Standing the mesh up ==========


def load_mesh(path, height, faces=None):
    """The mesh of a .glb, on the ground at the origin, facing +Y (generators face -Y), 'height' tall"""
    objects_before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    imported = [obj for obj in bpy.data.objects if obj not in objects_before]

    mesh = next(obj for obj in imported if obj.type == "MESH")
    world_matrix = mesh.matrix_world.copy()
    mesh.parent = None
    for obj in imported:
        if obj is not mesh:
            bpy.data.objects.remove(obj)

    mesh.data.transform(world_matrix)
    mesh.matrix_world = Matrix.Identity(4)

    lowest = min(vertex.co.z for vertex in mesh.data.vertices)
    highest = max(vertex.co.z for vertex in mesh.data.vertices)
    scale = height / (highest - lowest)
    mesh.data.transform(Matrix.Rotation(math.pi, 4, "Z") @ Matrix.Scale(scale, 4) @ Matrix.Translation((0.0, 0.0, -lowest)))

    if faces and len(mesh.data.polygons) > faces:
        decimate = mesh.modifiers.new("Lighten", "DECIMATE")
        decimate.ratio = faces / len(mesh.data.polygons)
        select_only([mesh])
        bpy.ops.object.modifier_apply(modifier=decimate.name)

    return mesh


def load_rigged_character(path, height, bone_names):
    """A rigged character (.glb: an armature with the mesh bound to it) standing on the ground at the origin, facing +Y,
    'height' tall, its bones renamed with 'bone_names' (theirs -> ours; the weights follow the names). Returns (armature, mesh)"""
    objects_before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    imported = [obj for obj in bpy.data.objects if obj not in objects_before]
    armature = next(obj for obj in imported if obj.type == "ARMATURE")
    mesh = next(obj for obj in imported if obj.type == "MESH")

    for obj in (armature, mesh):
        world_matrix = obj.matrix_world.copy()
        obj.parent = None
        obj.matrix_world = world_matrix
    for obj in imported:
        if obj not in (armature, mesh):
            bpy.data.objects.remove(obj)
    for action in list(bpy.data.actions):
        bpy.data.actions.remove(action)

    # Stand it up: turned to face +Y, scaled to the height, on the ground. Applied, so the bones' rest pose changes too
    points = [mesh.matrix_world @ vertex.co for vertex in mesh.data.vertices]
    lowest, highest = min(p.z for p in points), max(p.z for p in points)
    fit = Matrix.Rotation(math.pi, 4, "Z") @ Matrix.Scale(height / (highest - lowest), 4) @ Matrix.Translation((0.0, 0.0, -lowest))
    for obj in (armature, mesh):
        obj.matrix_world = fit @ obj.matrix_world
    select_only([armature, mesh])
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    mesh.parent = armature
    modifier = next((m for m in mesh.modifiers if m.type == "ARMATURE"), None) or mesh.modifiers.new("Armature", "ARMATURE")
    modifier.object = armature

    point_bones_at_children(armature)

    # Through temporary names: theirs and ours share some names (their Spine01 becomes our Spine, their Spine our Chest)
    renamed = [(armature.data.bones[theirs], ours) for theirs, ours in bone_names.items() if theirs in armature.data.bones]
    for bone, ours in renamed:
        bone.name = "_renaming_" + ours
    for bone, ours in renamed:
        bone.name = ours
    return armature, mesh


def point_bones_at_children(armature):
    """glTF keeps only the joints, an importer guesses the bones' tails (and guesses badly): every bone is made to point
    at the joint of the child that goes on in its direction (the spine, not a shoulder), a bone at the end of a chain
    continues its parent's direction"""
    select_only([armature])
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode="EDIT")

    # The joints before any tail moves (moving a tail of a connected bone would move its child's head)
    heads = {bone.name: bone.head.copy() for bone in armature.data.edit_bones}
    for bone in armature.data.edit_bones:
        bone.use_connect = False

    for bone in armature.data.edit_bones:
        head = heads[bone.name]
        along = (head - heads[bone.parent.name]).normalized() if bone.parent else Vector((0.0, 0.0, 1.0))
        children = [child for child in bone.children if (heads[child.name] - head).length > 0.001]
        if children:
            bone.tail = max(children, key=lambda child: (heads[child.name] - head).normalized().dot(along)).head
        elif bone.parent:
            bone.tail = head + along * max((head - heads[bone.parent.name]).length * 0.5, 0.05)
        else:
            bone.tail = head + Vector((0.0, 0.0, 0.1))

    bpy.ops.object.mode_set(mode="OBJECT")


def add_held_item_bones(armature, held_items):
    """Bones that carry held models (see knight_generator.add_held_item_bone), added to an existing armature"""
    select_only([armature])
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode="EDIT")
    for bone_name, parent_name, item_rest_matrix in held_items:
        knight_generator.add_held_item_bone(armature.data.edit_bones, bone_name, parent_name, item_rest_matrix)
    bpy.ops.object.mode_set(mode="OBJECT")


def skeleton_of(armature):
    """The armature's bones as a skeleton table (name, parent, head, tail, roll axis) for the helpers below"""
    return [(bone.name, bone.parent.name if bone.parent else None, tuple(bone.head_local), tuple(bone.tail_local), FORWARD)
            for bone in armature.data.bones]


# ========== The engine's textures ==========


def find_texture(material, socket_name):
    """The image plugged (maybe through other nodes) into a socket of the material's shader"""
    shader = next(node for node in material.node_tree.nodes if node.type == "BSDF_PRINCIPLED")
    pending = [link.from_node for link in shader.inputs[socket_name].links]
    while pending:
        node = pending.pop()
        if node.type == "TEX_IMAGE":
            return node.image
        pending += [link.from_node for socket in node.inputs for link in socket.links]
    return None


def image_pixels(image):
    pixels = numpy.empty(len(image.pixels), dtype=numpy.float32)
    image.pixels.foreach_get(pixels)
    return pixels.reshape(image.size[1], image.size[0], 4)


def save_image(name, folder, rgba, is_color, has_alpha=False):
    height, width = rgba.shape[:2]
    image = bpy.data.images.new(name, width, height, alpha=has_alpha)
    image.colorspace_settings.name = "sRGB" if is_color else "Non-Color"
    image.pixels.foreach_set(rgba.ravel())
    image.filepath_raw = "{}/{}.png".format(folder, name)
    image.file_format = "PNG"
    image.save()
    return image


def make_engine_textures(mesh, folder, base_name, brightness=1.0):
    """The engine reads a diffuse and a specular texture: the color comes from the base color (a little brighter if asked:
    the engine has no reflections, dark metal would look flat), the shine from the metal and roughness maps"""
    material = mesh.data.materials[0]
    color = image_pixels(find_texture(material, "Base Color"))
    color[..., :3] = numpy.clip(color[..., :3] * brightness, 0.0, 1.0)
    diffuse = save_image(base_name + "_diffuse", folder, color, is_color=True)

    metal_roughness = find_texture(material, "Metallic") or find_texture(material, "Roughness")
    if metal_roughness:
        channels = image_pixels(metal_roughness)
        roughness, metallic = channels[..., 1], channels[..., 2]
        shine = numpy.clip(0.08 + 0.55 * metallic * (1.0 - roughness) + 0.25 * (1.0 - roughness), 0.0, 1.0)
    else:
        shine = numpy.full(color.shape[:2], 0.3, dtype=numpy.float32)

    specular = numpy.ones_like(color)
    specular[..., 0] = specular[..., 1] = specular[..., 2] = shine
    save_image(base_name + "_specular", folder, specular, is_color=False)

    mesh.data.materials.clear()
    mesh.data.materials.append(make_textured_material(base_name, diffuse))


# ========== Finding the joints ==========


class BodyMeasure:
    """Horizontal slices of the mesh: which separate pieces each slice cuts"""

    def __init__(self, mesh):
        self.points = numpy.array([vertex.co[:] for vertex in mesh.data.vertices])
        self.height = self.points[:, 2].max()

    def pieces(self, part_of_height):
        """The separate pieces of the slice at this height, left to right: [(min x, max x, middle x, middle y)]"""
        z = part_of_height * self.height
        band = self.points[numpy.abs(self.points[:, 2] - z) < SLICE_THICKNESS * self.height]
        if len(band) == 0:
            return []

        band = band[numpy.argsort(band[:, 0])]
        cuts = numpy.where(numpy.diff(band[:, 0]) > PIECE_GAP * self.height)[0] + 1
        return [(piece[:, 0].min(), piece[:, 0].max(), piece[:, 0].mean(), piece[:, 1].mean())
                for piece in numpy.split(band, cuts)]

    def leg(self, part_of_height):
        """The middle (x, y) of the right leg (positive x) at this height"""
        pieces = [piece for piece in self.pieces(part_of_height) if piece[2] > 0.0]
        piece = min(pieces, key=lambda p: p[2])
        return piece[2], piece[3]

    def lowest_hand_height(self):
        """The lowest slice where the right arm is a piece of its own, apart from the body (the fingertips)"""
        for step in range(30, 80):
            part = step / 100.0
            pieces = self.pieces(part)
            if len(pieces) >= 3 and pieces[-1][0] > 0.12 * self.height:
                return part
        raise RuntimeError("No arm apart from the body found: is the character in an A pose?")

    def arm(self, part_of_height):
        """The middle (x, y) of the right arm (the outermost piece) at this height"""
        piece = self.pieces(part_of_height)[-1]
        return piece[2], piece[3]


def point(x, y, part_of_height, height):
    return (x, y, part_of_height * height)


def fit_skeleton(mesh):
    """The knight's bones (same names, same parents, same rolls) placed at the joints of this mesh"""
    body = BodyMeasure(mesh)
    height = body.height
    scale = height / TEMPLATE_HEIGHT

    ankle_x, ankle_y = body.leg(ANKLE_HEIGHT + 0.01)
    knee_x, knee_y = body.leg(KNEE_HEIGHT)
    hip_x = knee_x * 0.85

    fingertips = body.lowest_hand_height()
    hand_end = fingertips + 0.025
    wrist = fingertips + 0.075
    hand_end_x, hand_end_y = body.arm(hand_end)
    wrist_x, wrist_y = body.arm(wrist)
    elbow_part = wrist + (SHOULDER_HEIGHT - wrist) * 0.45
    elbow_x, elbow_y = body.arm(elbow_part)

    # The shoulder joint lies on the line from the wrist through the elbow, at the shoulder's height
    t = (SHOULDER_HEIGHT - wrist) / (elbow_part - wrist)
    shoulder_x = wrist_x + (elbow_x - wrist_x) * t
    clavicle_x = 0.04 * scale

    def right(name, parent, head, tail, roll):
        return (name, parent, head, tail, roll)

    right_side = [
        right("Shoulder", "Chest", point(clavicle_x, 0.0, SHOULDER_HEIGHT, height), point(shoulder_x, 0.0, SHOULDER_HEIGHT, height), UP),
        right("UpperArm", "Shoulder", point(shoulder_x, 0.0, SHOULDER_HEIGHT, height), point(elbow_x, elbow_y, elbow_part, height), FORWARD),
        right("LowerArm", "UpperArm", point(elbow_x, elbow_y, elbow_part, height), point(wrist_x, wrist_y, wrist, height), FORWARD),
        right("Hand", "LowerArm", point(wrist_x, wrist_y, wrist, height), point(hand_end_x, hand_end_y, hand_end, height), FORWARD),
        right("Thigh", "Hips", point(hip_x, 0.0, HIPS_HEIGHT - 0.01, height), point(knee_x, knee_y + KNEE_FORWARD * height, KNEE_HEIGHT, height), FORWARD),
        right("Shin", "Thigh", point(knee_x, knee_y + KNEE_FORWARD * height, KNEE_HEIGHT, height), point(ankle_x, ankle_y, ANKLE_HEIGHT, height), FORWARD),
        right("Foot", "Shin", point(ankle_x, ankle_y, ANKLE_HEIGHT, height), point(ankle_x, ankle_y + FOOT_LENGTH * height, 0.015 * height, height), UP),
    ]

    skeleton = [
        ("Hips",  None,    point(0.0, 0.0, HIPS_HEIGHT, height),  point(0.0, 0.0, SPINE_HEIGHT, height), FORWARD),
        ("Spine", "Hips",  point(0.0, 0.0, SPINE_HEIGHT, height), point(0.0, 0.0, CHEST_HEIGHT, height), FORWARD),
        ("Chest", "Spine", point(0.0, 0.0, CHEST_HEIGHT, height), point(0.0, 0.0, NECK_HEIGHT, height),  FORWARD),
        ("Neck",  "Chest", point(0.0, 0.0, NECK_HEIGHT, height),  point(0.0, 0.0, HEAD_HEIGHT, height),  FORWARD),
        ("Head",  "Neck",  point(0.0, 0.0, HEAD_HEIGHT, height),  point(0.0, 0.0, 1.0, height),          FORWARD),
    ]
    for suffix, sign in (("_R", 1.0), ("_L", -1.0)):
        for name, parent, head, tail, roll in right_side:
            parent_name = parent if parent in ("Chest", "Hips") else parent + suffix
            skeleton.append((name + suffix, parent_name, (head[0] * sign, head[1], head[2]), (tail[0] * sign, tail[1], tail[2]), roll))
    return skeleton


def bone_table(skeleton):
    return {name: (Vector(head), Vector(tail)) for name, parent, head, tail, roll in skeleton}


def fist_center(skeleton, side="_R"):
    """Where a weapon's grip sits: two thirds down the hand"""
    head, tail = bone_table(skeleton)["Hand" + side]
    return head.lerp(tail, 0.65)


def forearm_middle(skeleton, side="_L"):
    head, tail = bone_table(skeleton)["LowerArm" + side]
    return (head + tail) * 0.5


# ========== Weighting the mesh to the bones ==========


def distance_to_bone(position, head, tail):
    along = tail - head
    t = max(0.0, min(1.0, (position - head).dot(along) / along.length_squared))
    return (position - (head + along * t)).length


def weight_unreached_vertices(mesh, armature):
    """Blender's automatic weights miss parts it can't reach through the mesh (loose plates, horns): they follow the nearest bone"""
    bones = [bone for bone in armature.data.bones if bone.use_deform]
    groups = {bone.name: mesh.vertex_groups.get(bone.name) or mesh.vertex_groups.new(name=bone.name) for bone in bones}

    for vertex in mesh.data.vertices:
        if any(group.weight > 0.0 for group in vertex.groups):
            continue
        nearest = min(bones, key=lambda bone: distance_to_bone(vertex.co, bone.head_local, bone.tail_local))
        groups[nearest.name].add([vertex.index], 1.0, "REPLACE")


def skin_to_skeleton(mesh, armature):
    select_only([mesh, armature])
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.parent_set(type="ARMATURE_AUTO")
    weight_unreached_vertices(mesh, armature)


def build_character(mesh, held_items, animations=None):
    """The armature fitted into the mesh, the mesh weighted to it, the animations added. Returns (armature, skeleton)
    'held_items' is a function of the fitted skeleton giving [(bone, parent bone, rest matrix of the held model)]"""
    skeleton = fit_skeleton(mesh)
    armature = knight_generator.build_skeleton(skeleton, held_items(skeleton))
    skin_to_skeleton(mesh, armature)
    knight_generator.create_animations(armature, animations)
    knight_generator.reset_to_rest_pose(armature)
    return armature, skeleton
