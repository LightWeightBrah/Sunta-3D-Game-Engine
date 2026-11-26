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
from dataclasses import dataclass

import bpy
import numpy
from mathutils import Euler, Matrix, Quaternion, Vector

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


def straighten_spine(armature, chain=("Hips", "Spine02", "Spine", "Chest", "Neck", "Head")):
    """An automatic rig sometimes puts a spine joint just above the hips and in front of them (Meshy did for a king in a
    heavy cape): the short bone between them lies nearly flat, and a retarget turns the hips by tens of degrees to stand
    it up like the actor's - the character stands bent double. The joints of the chain are put back on the straight
    line from the hips to the head, at their own heights (the lowest at least a fifth of the way up), and each bone
    points at the next (the mesh in its rest pose doesn't move: only the bones do)"""
    names = [name for name in chain if name in armature.data.bones]
    if len(names) < 3:
        return
    select_only([armature])
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode="EDIT")
    bones = armature.data.edit_bones
    bottom, top = bones[names[0]].head.copy(), bones[names[-1]].head.copy()
    span = top.z - bottom.z
    previous_z = bottom.z
    for index, name in enumerate(names[1:-1], start=1):
        bone = bones[name]
        least = bottom.z + span * index / (len(names) - 1) * 0.6
        z = max(bone.head.z, least, previous_z + span * 0.08)
        t = (z - bottom.z) / span
        bone.head = bottom.lerp(top, t)
        previous_z = z
    for name, following in zip(names[:-1], names[1:]):
        bones[name].tail = bones[following].head.copy()
    # The head points straight up to the top of the skull (the rig's head end lay a centimeter from the head joint, in
    # any direction: the retarget then tipped the head forward to point it like the actor's)
    head = bones[names[-1]]
    up = Vector((0.0, 0.0, max(span * 0.45, 0.12)))
    head.tail = head.head + up
    for child in head.children:
        if child.name.lower().startswith("head_end"):
            child.head = head.tail.copy()
            child.tail = child.head + up * 0.3
    bpy.ops.object.mode_set(mode="OBJECT")
    print("SPINE_STRAIGHTENED", [(name, round(armature.data.bones[name].head_local.z, 2)) for name in names])


def cape_follows_hips(armature, mesh, behind=0.1, legs=("Thigh_L", "Thigh_R", "Calf_L", "Calf_R", "Foot_L", "Foot_R")):
    """A cape or a long coat rigged to the legs stretches between them in every stride and blow. The part of the mesh
    behind the legs (more than 'behind' meters behind the thighs' line, below the hips) moves its leg weights to the
    hips: the cape swings with the body and the legs move inside it"""
    bones = armature.data.bones
    hips = bones["Hips"].head_local
    thigh_y = sum(bones[name].head_local.y for name in legs[:2] if name in bones) / 2.0
    leg_groups = {mesh.vertex_groups[name].index for name in legs if name in mesh.vertex_groups}
    hips_group = mesh.vertex_groups.get("Hips") or mesh.vertex_groups.new(name="Hips")
    moved = 0
    for vertex in mesh.data.vertices:
        # (the character faces +Y: behind is -Y)
        if vertex.co.z > hips.z or vertex.co.y > thigh_y - behind:
            continue
        weight = sum(slot.weight for slot in vertex.groups if slot.group in leg_groups)
        if weight <= 0.0:
            continue
        for slot in list(vertex.groups):
            if slot.group in leg_groups:
                mesh.vertex_groups[slot.group].remove([vertex.index])
        hips_group.add([vertex.index], weight, "ADD")
        moved += 1
    print("CAPE_ON_HIPS {} vertices".format(moved))


def load_rigged_character(path, height, bone_names):
    """A rigged character (.glb: an armature with the mesh bound to it) standing on the ground at the origin, facing +Y,
    'height' tall, its bones renamed with 'bone_names' (theirs -> ours; the weights follow the names). Returns (armature, mesh)"""
    objects_before = set(bpy.data.objects)
    if path.lower().endswith(".fbx"):
        # A character Mixamo rigged (downloaded with its skin): the scene keeps its own frame rate
        fps, fps_base = bpy.context.scene.render.fps, bpy.context.scene.render.fps_base
        bpy.ops.import_scene.fbx(filepath=path)
        bpy.context.scene.render.fps, bpy.context.scene.render.fps_base = fps, fps_base
    else:
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
    # In its rest pose (a file with a clip leaves the bones posed in its first frame)
    if armature.animation_data:
        armature.animation_data.action = None
    for pose_bone in armature.pose.bones:
        pose_bone.location = Vector()
        pose_bone.rotation_quaternion = Quaternion()
        pose_bone.rotation_euler = Euler()
        pose_bone.scale = Vector((1.0, 1.0, 1.0))

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


def weld_seams(mesh, distance=0.0002):
    """Generated meshes are cut along their texture seams: two vertices in one place, each with its own neighbours.
    Smoothing anything over such a mesh tears it open along the seams. Welded, the texture stays (it lives on the
    corners of the faces, not on the vertices) and the surface is one piece"""
    select_only([mesh])
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.remove_doubles(threshold=distance, use_unselected=False)
    bpy.ops.object.mode_set(mode="OBJECT")


def smooth_shape_of(mesh, bone_name, iterations=4, factor=0.5, least_weight=0.5):
    """Evens out the surface the bone carries (a beard and hair generated as a heap of spiky shards): each vertex moves
    towards its neighbours, the rest of the body stays as it is"""
    import bmesh

    group = mesh.vertex_groups.get(bone_name)
    if group is None:
        return
    chosen = {vertex.index for vertex in mesh.data.vertices
              if any(slot.group == group.index and slot.weight >= least_weight for slot in vertex.groups)}

    shape = bmesh.new()
    shape.from_mesh(mesh.data)
    shape.verts.ensure_lookup_table()
    verts = [shape.verts[index] for index in chosen]
    for _ in range(iterations):
        bmesh.ops.smooth_vert(shape, verts=verts, factor=factor, use_axis_x=True, use_axis_y=True, use_axis_z=True)
    shape.to_mesh(mesh.data)
    shape.free()
    mesh.data.update()


def smooth_skin_weights(mesh, iterations=6, factor=0.5, most_bones=4):
    """Generated rigs (Meshy's automatic rigging) weight the skin in hard patches: in a pose the cloth tears into
    stretched spikes where one patch meets the next (a jaw, the collar, the armpits). The weights are blurred over the
    surface a few times (each vertex towards its neighbours), then each vertex keeps its strongest bones, adding up to 1"""
    select_only([mesh])
    bpy.ops.object.mode_set(mode="WEIGHT_PAINT")
    mesh.data.use_paint_mask_vertex = False
    bpy.ops.object.vertex_group_smooth(group_select_mode="ALL", factor=factor, repeat=iterations, expand=0.0)
    bpy.ops.object.vertex_group_limit_total(group_select_mode="ALL", limit=most_bones)
    bpy.ops.object.vertex_group_normalize_all(group_select_mode="ALL", lock_active=False)
    bpy.ops.object.mode_set(mode="OBJECT")


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


# ========== Hands closed around a grip ==========

# The palm is this part of the hand's length (from the wrist), the fingers the rest
PALM_PART_OF_HAND = 0.42

# The hand is measured in a cylinder going on from the forearm: this many lengths of its bone from the wrist, this
# many lengths of it around the axis. Its fingers go on along the forearm when the mesh says otherwise by more than
# FINGERS_OFF_BONE_DEGREES (something else weighted to the hand)
HAND_REACH_IN_BONES = 2.0
HAND_RADIUS_IN_BONES = 1.2
FINGERS_OFF_BONE_DEGREES = 35.0

# A vertex is part of a finger to bend when the hand bone moves it at least this much (some models weight their
# fingers partly to the hand only)
FINGER_WEIGHT = 0.05

# The fingers wrap around the grip this far (degrees): a little less than a full circle, the fingertips end at the palm
FINGER_WRAP_DEGREES = 250.0


@dataclass
class Grip:
    """A closed fist in the rest pose: the middle of the hole the fingers make, the hole's direction (towards the thumb),
    the straight fingers' direction (from the wrist), the palm's side (out of the palm) and how far the back of the fist
    reaches from the middle (towards -palm)"""
    center: Vector
    along: Vector
    fingers: Vector
    palm: Vector
    back_of_fist: float

    def item_matrix(self, x, y, z, grip_in_item=Vector()):
        """Where a held model stands so that its grip (at 'grip_in_item', its own coordinates) is in this fist,
        with its local axes along the given world directions"""
        rotation = Matrix((x, y, z)).transposed().to_4x4()
        return Matrix.Translation(self.center - rotation.to_3x3() @ grip_in_item) @ rotation


def hand_frames(armature, grips):
    """Per side ("_L", "_R" -> Grip): the knuckle line (the hole of the fist) and the straight fingers of the hand,
    in the space of its bone, for motion_retarget.match_hands"""
    frames = {}
    for side, grip in grips.items():
        if grip is None:
            continue
        rest = (armature.matrix_world @ armature.data.bones["Hand" + side].matrix_local).to_quaternion().inverted()
        frames[side] = (rest @ grip.along, rest @ grip.fingers)
    return frames


def make_fist(armature, mesh, side):
    """Bends the hand's fingers (the mesh in the rest pose, the hand has no finger bones) into a fist around a grip.
    Rigged models come with open hands: a weapon only looks held when the fingers close around it. Returns the Grip"""
    hand = armature.data.bones["Hand" + side]
    wrist = armature.matrix_world @ hand.head_local

    group = mesh.vertex_groups[hand.name].index
    to_world = mesh.matrix_world
    to_mesh = to_world.inverted()
    # The hand lies in a cylinder going on from the forearm (the end of a hand bone from a glTF file points anywhere):
    # outside it are a chain or a strap hanging from the wrist, weighted to the hand too (they were taken for fingers)
    elbow = armature.matrix_world @ armature.data.bones["LowerArm" + side].head_local
    bone_direction = (wrist - elbow).normalized()
    bone_length = hand.length * armature.matrix_world.to_scale().x
    reach, radius_of_hand = bone_length * HAND_REACH_IN_BONES, bone_length * HAND_RADIUS_IN_BONES

    def in_hand(position):
        offset = position - wrist
        along_bone = offset.dot(bone_direction)
        return -radius_of_hand * 0.5 <= along_bone <= reach and (offset - bone_direction * along_bone).length <= radius_of_hand

    skin = {vertex.index for vertex in mesh.data.vertices
            if any(slot.group == group and slot.weight >= FINGER_WEIGHT for slot in vertex.groups)
            and in_hand(to_world @ vertex.co)}
    vertices = [vertex for vertex in mesh.data.vertices if vertex.index in skin
                and any(slot.group == group and slot.weight >= 0.3 for slot in vertex.groups)]
    points = numpy.array([tuple(to_world @ vertex.co) for vertex in vertices])

    # The hand is longest along the fingers (the bone has no finger bones to point at)
    _, _, directions = numpy.linalg.svd(points - points.mean(axis=0), full_matrices=False)
    fingers = Vector(directions[0]).normalized()
    if fingers.dot(Vector(points.mean(axis=0)) - wrist) < 0.0:
        fingers = -fingers
    if fingers.angle(bone_direction) > math.radians(FINGERS_OFF_BONE_DEGREES):
        fingers = bone_direction

    # The hand is flat: across the fingers its widest direction is the knuckles' line, its thinnest the palm's side
    along_fingers = (points - numpy.array(wrist)) @ numpy.array(fingers)
    across = points - numpy.outer(along_fingers, numpy.array(fingers))
    middle = across.mean(axis=0)
    _, _, directions = numpy.linalg.svd(across - middle, full_matrices=False)
    along = Vector(directions[0]).normalized()

    # The thumb's side: the thumb sticks out of the hand near the wrist (between the wrist and the knuckles), further
    # than the little finger's edge. (Thumbs pointing forward in an A or T pose is not true of every model: some hold
    # their palms forward, the thumbs up.) The palm is on the side the fingers bend to
    length = along_fingers.max()
    near_wrist = (along_fingers > length * 0.1) & (along_fingers < length * 0.45)
    if near_wrist.sum() >= 4:
        sideways = (across[near_wrist] - middle) @ numpy.array(along)
        if -sideways.min() > sideways.max():
            along = -along
    elif along.y < 0.0:
        along = -along
    palm = along.cross(fingers) if side == "_R" else fingers.cross(along)

    knuckle_distance = along_fingers.max() * PALM_PART_OF_HAND
    finger_length = along_fingers.max() - knuckle_distance
    radius = finger_length / math.radians(FINGER_WRAP_DEGREES)
    knuckles = Vector(middle) + fingers * knuckle_distance
    center = knuckles + palm * radius

    # Bent: every vertex the hand moves at all (the executioner's fingers are weighted only partly
    # to the hand: with only the strongly weighted vertices they stayed straight under the grip), past the knuckles, within the hand
    hand_reach = numpy.linalg.norm(across - middle, axis=1).max()
    bent_vertices, bent_distances = [], []
    for vertex in mesh.data.vertices:
        if vertex.index not in skin:
            continue
        offset = numpy.array(to_world @ vertex.co) - numpy.array(wrist)
        distance = float(offset @ numpy.array(fingers))
        if distance <= knuckle_distance:
            continue
        if numpy.linalg.norm((offset - numpy.array(fingers) * distance) - (middle - numpy.array(wrist))) > hand_reach * 1.2:
            continue
        bent_vertices.append(vertex)
        bent_distances.append(distance)

    for vertex, distance in zip(bent_vertices, bent_distances):
        out_of_palm = distance - knuckle_distance
        if out_of_palm <= 0.0:
            continue
        position = to_world @ vertex.co
        offset = position - knuckles
        to_palm = offset.dot(palm)
        sideways = offset.dot(along)
        # Rolled onto a circle around the grip: the farther along the finger, the more it turns towards the palm
        angle = out_of_palm / radius
        distance_from_center = radius - to_palm
        bent = center + along * sideways + (-palm * math.cos(angle) + fingers * math.sin(angle)) * distance_from_center
        vertex.co = to_mesh @ bent

    mesh.data.update()

    # The hole the bent fingers really close around: a circle fitted to them (across the hole). A hand modelled
    # half closed, or with a thick palm, closes its fingers elsewhere than the ideal circle above: the grip went
    # past the curled fingers, along the base of the thumb
    curled = [to_world @ vertex.co for vertex, distance in zip(bent_vertices, bent_distances) if distance > knuckle_distance + finger_length * 0.3]
    if len(curled) >= 8:
        center = fitted_hole(curled, center, palm, fingers)

    back_of_fist = max((to_world @ vertex.co - center).dot(-palm) for vertex in vertices)
    return Grip(center, along, fingers, palm, back_of_fist)


def fitted_hole(points, center, palm, fingers):
    """The middle of the circle the points lie around, seen along the hole (a least squares circle in the plane of the
    palm and the fingers); its depth along the hole stays the old center's"""
    xs = numpy.array([(point - center).dot(palm) for point in points])
    ys = numpy.array([(point - center).dot(fingers) for point in points])
    # x² + y² = 2ax + 2by + c: linear in a, b, c
    matrix = numpy.column_stack((2.0 * xs, 2.0 * ys, numpy.ones(len(xs))))
    (a, b, c), *_ = numpy.linalg.lstsq(matrix, xs * xs + ys * ys, rcond=None)
    return center + palm * a + fingers * b


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
    # (a file just written may be held for a moment by another program, e.g. a virus scanner: try again)
    for attempt in range(6):
        try:
            image.save()
            break
        except RuntimeError:
            if attempt == 5:
                raise
            import time
            time.sleep(1.0)
    return image


def make_engine_textures(mesh, folder, base_name, brightness=1.0, shine_scale=1.0):
    """The engine reads a diffuse and a specular texture: the color comes from the base color (a little brighter if asked:
    the engine has no reflections, dark metal would look flat), the shine from the metal and roughness maps
    ('shine_scale' below 1: cloth that the generator made too glossy looks like cloth, not plastic)"""
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

    shine = numpy.clip(shine * shine_scale, 0.0, 1.0)
    specular = numpy.ones_like(color)
    specular[..., 0] = specular[..., 1] = specular[..., 2] = shine
    save_image(base_name + "_specular", folder, specular, is_color=False)

    mesh.data.materials.clear()
    mesh.data.materials.append(make_textured_material(base_name, diffuse))


def bake_emission(mesh, folder, name, build_color, size=1024):
    """Bakes a glow made of shader nodes into <folder>/<name>.png (the engine's *_emissive texture).
    'build_color(node_tree, diffuse_image)' adds the nodes and returns the color socket: the light of every point
    of the surface. Nodes can use the object's own coordinates (a pattern in 3D, without seams between UV islands)"""
    scene = bpy.context.scene
    previous_engine = scene.render.engine
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 1

    original_materials = list(mesh.data.materials)
    diffuse_image = next((node.image for node in original_materials[0].node_tree.nodes if node.type == "TEX_IMAGE"), None)

    material = bpy.data.materials.new(name + "_bake")
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = tree.nodes.new("ShaderNodeOutputMaterial")
    emission = tree.nodes.new("ShaderNodeEmission")
    tree.links.new(build_color(tree, diffuse_image), emission.inputs["Color"])
    tree.links.new(emission.outputs["Emission"], output.inputs["Surface"])

    target = bpy.data.images.new(name, size, size, alpha=False)
    target_node = tree.nodes.new("ShaderNodeTexImage")
    target_node.image = target
    tree.nodes.active = target_node

    mesh.data.materials.clear()
    mesh.data.materials.append(material)
    select_only([mesh])
    bpy.context.view_layer.objects.active = mesh
    bpy.ops.object.bake(type="EMIT", margin=4)

    target.filepath_raw = "{}/{}.png".format(folder, name)
    target.file_format = "PNG"
    target.save()

    mesh.data.materials.clear()
    for original in original_materials:
        mesh.data.materials.append(original)
    scene.render.engine = previous_engine
    return target


# ========== A face from a photo ==========


@dataclass
class FacePhoto:
    """A photo of the face seen from the front, and where in it (pixels) the character's right eye, left eye and the
    middle of the mouth are. The character's right eye is on the photo's left"""
    path: str
    right_eye: tuple
    left_eye: tuple
    mouth: tuple


def find_eyes(mesh, height):
    """Where the eyes of a model standing at the origin and looking along +Y are (the world, rest pose): (right, left).
    Read from the shape of the face: the front of the head a little below the top of the head"""
    points = [mesh.matrix_world @ vertex.co for vertex in mesh.data.vertices]
    top = max(point.z for point in points)
    size = height / 1.83
    eye_height = top - 0.118 * size
    eyes = []
    for side in (1.0, -1.0):
        across = side * 0.033 * size
        near = [point for point in points if abs(point.z - eye_height) < 0.012 * size and abs(point.x - across) < 0.012 * size]
        front = max(point.y for point in near) if near else 0.1
        eyes.append(Vector((across, front, eye_height)))
    return eyes[0], eyes[1]


def affine_from_points(model_points, photo_points):
    """u = a x + b z + c, v = d x + e z + f through three pairs: model (x, z) -> photo pixels (u, v)"""
    matrix = numpy.array([[x, z, 1.0] for x, z in model_points])
    us = numpy.linalg.solve(matrix, numpy.array([u for u, _ in photo_points]))
    vs = numpy.linalg.solve(matrix, numpy.array([v for _, v in photo_points]))
    return us, vs


def face_photo_mapping(photo, right_eye, left_eye):
    """Where the photo lies on the model (seen from the front): the affine map model (x, z) -> photo pixels, the middle
    of the face, the size of its oval and the mouth"""
    eye_distance = (right_eye - left_eye).length
    photo_eye_distance = math.dist(photo.right_eye, photo.left_eye)
    eyes_middle_photo = ((photo.right_eye[0] + photo.left_eye[0]) * 0.5, (photo.right_eye[1] + photo.left_eye[1]) * 0.5)
    mouth_below = math.dist(eyes_middle_photo, photo.mouth) / photo_eye_distance * eye_distance
    eyes_middle = (right_eye + left_eye) * 0.5
    mouth = eyes_middle - Vector((0.0, 0.0, mouth_below))
    us, vs = affine_from_points([(right_eye.x, right_eye.z), (left_eye.x, left_eye.z), (mouth.x, mouth.z)],
                                [photo.right_eye, photo.left_eye, photo.mouth])
    center = (eyes_middle + mouth) * 0.5 + Vector((0.0, 0.0, mouth_below * 0.15))
    return us, vs, center, (eye_distance * 1.25, mouth_below * 1.75), mouth


def skin_color_builder(base_image, base_uv="", photo=None, mapping=None):
    """A 'build_color' for bake_emission: the model's own texture (read through the UV map 'base_uv', the active one
    when empty), with the photo of the face over its front when there is one"""

    def build(tree, _):
        nodes, links = tree.nodes, tree.links

        uv_map = nodes.new("ShaderNodeUVMap")
        uv_map.uv_map = base_uv
        base_node = nodes.new("ShaderNodeTexImage")
        base_node.image = base_image
        links.new(uv_map.outputs["UV"], base_node.inputs["Vector"])
        if photo is None:
            return base_node.outputs["Color"]

        us, vs, center, (radius_across, radius_up), _ = mapping
        photo_image = bpy.data.images.load(photo.path, check_existing=True)
        width, picture_height = photo_image.size

        def math_node(operation, first, second=None, value=None):
            node = nodes.new("ShaderNodeMath")
            node.operation = operation
            links.new(first, node.inputs[0])
            if second is not None:
                links.new(second, node.inputs[1])
            elif value is not None:
                node.inputs[1].default_value = value
            return node.outputs[0]

        def linear(x, z, a, b, c):
            return math_node("ADD", math_node("ADD", math_node("MULTIPLY", x, value=a), math_node("MULTIPLY", z, value=b)), value=c)

        coordinates = nodes.new("ShaderNodeTexCoord")
        parts = nodes.new("ShaderNodeSeparateXYZ")
        links.new(coordinates.outputs["Object"], parts.inputs[0])
        x, z = parts.outputs["X"], parts.outputs["Z"]

        u = linear(x, z, us[0] / width, us[1] / width, us[2] / width)
        v = linear(x, z, -vs[0] / picture_height, -vs[1] / picture_height, 1.0 - vs[2] / picture_height)
        photo_uv = nodes.new("ShaderNodeCombineXYZ")
        links.new(u, photo_uv.inputs[0])
        links.new(v, photo_uv.inputs[1])
        photo_node = nodes.new("ShaderNodeTexImage")
        photo_node.image = photo_image
        photo_node.extension = "EXTEND"
        photo_node.interpolation = "Cubic"
        links.new(photo_uv.outputs[0], photo_node.inputs["Vector"])

        # Facing the camera of the photo: the front of the face, not its sides or the back of the head
        geometry = nodes.new("ShaderNodeNewGeometry")
        normal = nodes.new("ShaderNodeSeparateXYZ")
        links.new(geometry.outputs["Normal"], normal.inputs[0])
        facing = nodes.new("ShaderNodeMapRange")
        links.new(normal.outputs["Y"], facing.inputs["Value"])
        # Only the faces turned away are left out: the generated beard is a heap of shards turned every way, each
        # must still show the photo (else the old texture shows through in flecks)
        facing.inputs["From Min"].default_value = -0.35
        facing.inputs["From Max"].default_value = 0.05

        across = math_node("DIVIDE", math_node("SUBTRACT", x, value=center.x), value=radius_across)
        up = math_node("DIVIDE", math_node("SUBTRACT", z, value=center.z), value=radius_up)
        oval = math_node("SQRT", math_node("ADD", math_node("MULTIPLY", across, across), math_node("MULTIPLY", up, up)))
        inside = nodes.new("ShaderNodeMapRange")
        links.new(oval, inside.inputs["Value"])
        inside.inputs["From Min"].default_value = 0.7
        inside.inputs["From Max"].default_value = 1.0
        inside.inputs["To Min"].default_value = 1.0
        inside.inputs["To Max"].default_value = 0.0

        mask = math_node("MULTIPLY", facing.outputs["Result"], inside.outputs["Result"])
        mix = nodes.new("ShaderNodeMix")
        mix.data_type = "RGBA"
        links.new(mask, mix.inputs["Factor"])
        links.new(base_node.outputs["Color"], mix.inputs[6])
        links.new(photo_node.outputs["Color"], mix.inputs[7])
        return mix.outputs[2]

    return build


def project_face_photo(mesh, armature, photo, height, folder, name, eye_offset=(0.0, 0.0)):
    """Paints the photo of the face onto the model's own texture, seen straight from the front, the eyes and the mouth
    on the model's eyes and mouth; it fades out towards the sides of the head and its edges (the texture made by the
    generator stays there). A generated face is a guess at the person, the photo is the person.
    'eye_offset' (meters, across and up) moves where the model's eyes are taken to be, when the shape misleads.
    On a generated atlas the face is cut into many small islands: separate_head gives the head a texture of its own"""
    right_eye, left_eye = find_eyes(mesh, height)
    shift = Vector((eye_offset[0], 0.0, eye_offset[1]))
    mapping = face_photo_mapping(photo, right_eye + shift, left_eye + shift)

    material = mesh.data.materials[0]
    base_image = find_texture(material, "Base Color")

    # The pose of the bake is the rest pose (the face where its coordinates say)
    previous_pose = armature.data.pose_position
    armature.data.pose_position = "REST"
    baked = bake_emission(mesh, folder, name + "_face_baked", skin_color_builder(base_image, "", photo, mapping), size=base_image.size[0])
    armature.data.pose_position = previous_pose

    # The model's texture is now the one with the face
    for node in material.node_tree.nodes:
        if node.type == "TEX_IMAGE" and node.image == base_image:
            node.image = baked
    return baked


def keep_shading_across_cut(mesh):
    """Fixes the smooth shading of every corner as it is now: when the mesh is later cut in two, the faces along the
    cut keep the light of the whole surface (no visible line where the pieces meet)"""
    mesh.data.update()
    normals = [tuple(normal.vector) for normal in mesh.data.corner_normals]
    mesh.data.normals_split_custom_set(normals)


def bone_weights(mesh, bone_names):
    """How much the bones (together) carry each vertex"""
    groups = {mesh.vertex_groups[name].index for name in bone_names if name in mesh.vertex_groups}
    weight = numpy.zeros(len(mesh.data.vertices))
    for vertex in mesh.data.vertices:
        for slot in vertex.groups:
            if slot.group in groups:
                weight[vertex.index] += slot.weight
    return weight


def head_faces(mesh, height):
    """The faces of the head: those the head bone carries, and below them the chin and the jaw down to a little under
    the mouth (generated rigs give the beard to the neck)"""
    head = bone_weights(mesh, ["Head"])
    head_and_neck = bone_weights(mesh, ["Head", "Neck"])
    right_eye, left_eye = find_eyes(mesh, height)
    size = height / 1.83
    eyes_z = (right_eye.z + left_eye.z) * 0.5
    jaw = eyes_z - 0.115 * size
    # The goatee hangs lower, in the middle of the face, in front
    chin = eyes_z - 0.165 * size
    front = (right_eye.y + left_eye.y) * 0.5 - 0.06 * size
    chosen = set()
    for polygon in mesh.data.polygons:
        indices = list(polygon.vertices)
        center = polygon.center
        if numpy.mean(head[indices]) >= 0.5:
            chosen.add(polygon.index)
        elif numpy.mean(head_and_neck[indices]) >= 0.5 and center.z >= jaw:
            chosen.add(polygon.index)
        elif numpy.mean(head_and_neck[indices]) >= 0.2 and center.z >= chin and abs(center.x) < 0.06 * size and center.y >= front:
            chosen.add(polygon.index)
    return chosen


def delete_faces(mesh, indices):
    """Deletes the faces and the vertices left without any face"""
    import bmesh
    shape = bmesh.new()
    shape.from_mesh(mesh.data)
    shape.faces.ensure_lookup_table()
    bmesh.ops.delete(shape, geom=[shape.faces[index] for index in indices], context="FACES_ONLY")
    loose = [vertex for vertex in shape.verts if not vertex.link_faces]
    bmesh.ops.delete(shape, geom=loose, context="VERTS")
    shape.to_mesh(mesh.data)
    shape.free()
    mesh.data.update()


def unwrap_around(mesh, uv_layer):
    """One unwrap around the vertical axis of the piece (a cylinder seen from inside): the face is a single unbroken
    region in front, the seam runs down the back of the head. (An automatic unwrap of a generated head breaks the face
    into dozens of small islands along its shards)"""
    points = numpy.array([vertex.co[:] for vertex in mesh.data.vertices])
    middle_x, middle_y = points[:, 0].mean(), points[:, 1].mean()
    bottom, top = points[:, 2].min(), points[:, 2].max()
    # The angle from the front (+Y) towards the sides: 0.5 in front, 0 and 1 at the back
    angle = numpy.arctan2(points[:, 0] - middle_x, points[:, 1] - middle_y) / (2.0 * math.pi) + 0.5
    up = (points[:, 2] - bottom) / max(top - bottom, 1e-6)
    margin = 0.01
    for polygon in mesh.data.polygons:
        indices = list(polygon.vertices)
        us = [angle[index] for index in indices]
        # A face across the seam at the back: all its corners on one side of it
        if max(us) - min(us) > 0.5:
            us = [u + 1.0 if u < 0.5 else u for u in us]
            if min(us) > 1.0:
                us = [u - 1.0 for u in us]
        for loop_index, index, u in zip(polygon.loop_indices, indices, us):
            uv_layer.data[loop_index].uv = (margin + u * (1.0 - 2.0 * margin), margin + up[index] * (1.0 - 2.0 * margin))


def separate_head(mesh, armature, folder, name, height, photo=None, eye_offset=(0.0, 0.0), size=2048):
    """Cuts the head off into a mesh of its own with its own texture (<name>_head_diffuse.png, 'size' pixels square):
    on a generated atlas the face is spread over many small islands, a few dozen pixels each - blocky up close.
    The head gets an unwrap of its own, baked from the model's texture (and the photo of the face, when given).
    Both pieces keep the skin weights and the armature, so they move as one. Returns the head"""
    mapping = None
    if photo is not None:
        right_eye, left_eye = find_eyes(mesh, height)
        shift = Vector((eye_offset[0], 0.0, eye_offset[1]))
        mapping = face_photo_mapping(photo, right_eye + shift, left_eye + shift)

    keep_shading_across_cut(mesh)
    chosen = head_faces(mesh, height)

    head = mesh.copy()
    head.data = mesh.data.copy()
    head.name = mesh.name + "_head"
    for collection in mesh.users_collection:
        collection.objects.link(head)
    delete_faces(head, set(range(len(head.data.polygons))) - chosen)
    delete_faces(mesh, chosen)

    base_uv = head.data.uv_layers.active.name
    head_uv = head.data.uv_layers.new(name="HeadUV")
    head.data.uv_layers.active = head_uv
    head_uv.active_render = True
    unwrap_around(head, head_uv)

    base_image = find_texture(mesh.data.materials[0], "Base Color")
    previous_pose = armature.data.pose_position
    armature.data.pose_position = "REST"
    baked = bake_emission(head, folder, name + "_head_diffuse", skin_color_builder(base_image, base_uv, photo, mapping), size=size)
    armature.data.pose_position = previous_pose

    head.data.uv_layers.remove(head.data.uv_layers[base_uv])
    head.data.uv_layers.active = head.data.uv_layers["HeadUV"]
    specular = numpy.ones((64, 64, 4), dtype=numpy.float32)
    specular[..., :3] = 0.1
    save_image(name + "_head_specular", folder, specular, is_color=False)
    head.data.materials.clear()
    head.data.materials.append(make_textured_material(name + "_head", baked))
    print("HEAD {} faces {} texture {}".format(name, len(head.data.polygons), size))
    return head


def reskin_with_heat(mesh, armature, least_covered=0.97):
    """Replaces generated skin weights (hard patches that crumple the shoulders when the arms go up) with Blender's
    automatic weights from bone heat, which spread over a shoulder the way skin stretches. Keeps the old weights
    when the heat solver leaves part of the surface unweighted. Returns whether the new weights were kept"""
    names = {group.index: group.name for group in mesh.vertex_groups}
    old = {name: [] for name in names.values()}
    for vertex in mesh.data.vertices:
        for slot in vertex.groups:
            old[names[slot.group]].append((vertex.index, slot.weight))

    for modifier in [m for m in mesh.modifiers if m.type == "ARMATURE"]:
        mesh.modifiers.remove(modifier)
    mesh.vertex_groups.clear()
    world = mesh.matrix_world.copy()
    mesh.parent = None
    mesh.matrix_world = world

    select_only([mesh, armature])
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.parent_set(type="ARMATURE_AUTO")

    covered = sum(1 for vertex in mesh.data.vertices if any(slot.weight > 0.0 for slot in vertex.groups))
    share = covered / max(len(mesh.data.vertices), 1)
    print("HEAT weights cover {:.1%} of the vertices".format(share))
    if share >= least_covered:
        return True

    mesh.vertex_groups.clear()
    for group_name, slots in old.items():
        group = mesh.vertex_groups.new(name=group_name)
        for index, weight in slots:
            group.add([index], weight, "REPLACE")
    print("HEAT failed, the generated weights stay")
    return False


def reskin_by_proxy(mesh, armature, voxel_size=0.025, most_bones=4, only_bones=None):
    """Skin weights from bone heat computed on a watertight copy of the body, then carried over to the real mesh.
    Bone heat (Blender's automatic weights) spreads a shoulder's weights the way skin stretches, but fails on
    generated meshes full of holes and loose shells; a voxel remesh of the body is closed, the heat solves on it, and
    each vertex of the real mesh takes the weights of the nearest point of that copy. Returns whether it worked.
    Not for figures in loose cloth: tried on Grave Crown's king, the heat tied his cape to his hands and arms, and the
    cloth stretched into long sheets whenever the arms moved"""
    proxy = mesh.copy()
    proxy.data = mesh.data.copy()
    bpy.context.scene.collection.objects.link(proxy)
    for modifier in list(proxy.modifiers):
        proxy.modifiers.remove(modifier)
    proxy.vertex_groups.clear()
    proxy.parent = None
    proxy.matrix_world = mesh.matrix_world.copy()

    select_only([proxy])
    remesh = proxy.modifiers.new("Closed", "REMESH")
    remesh.mode = "VOXEL"
    remesh.voxel_size = voxel_size
    bpy.ops.object.modifier_apply(modifier=remesh.name)
    # The heat solver fails at a human's size in meters (a known limit of Blender's bone heat): it solves on copies
    # ten times bigger, and the copy of the body is scaled back before the weights are carried over
    upscale = 10.0
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    rig = armature.copy()
    rig.data = armature.data.copy()
    bpy.context.scene.collection.objects.link(rig)
    rig.animation_data_clear()
    for target in (proxy, rig):
        target.scale = target.scale * upscale
        target.location = target.location * upscale
        select_only([target])
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    print("PROXY {} vertices".format(len(proxy.data.vertices)))

    select_only([proxy, rig])
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.parent_set(type="ARMATURE_AUTO")
    for modifier in list(proxy.modifiers):
        proxy.modifiers.remove(modifier)
    proxy.parent = None
    bpy.data.objects.remove(rig)
    proxy.scale = (1.0 / upscale,) * 3
    select_only([proxy])
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    covered = sum(1 for vertex in proxy.data.vertices if any(slot.weight > 0.0 for slot in vertex.groups))
    share = covered / max(len(proxy.data.vertices), 1)
    print("PROXY heat weights cover {:.1%}".format(share))
    if share < 0.97:
        bpy.data.objects.remove(proxy)
        return False

    # The generated weights are kept aside: a vertex the copy leaves unweighted keeps them (it would stay behind in
    # its rest pose while the body moves, a long spike)
    names = {group.index: group.name for group in mesh.vertex_groups}
    previous = [[(names[slot.group], slot.weight) for slot in vertex.groups] for vertex in mesh.data.vertices]

    # The real mesh takes the copy's weights (its own groups emptied first, the same names made again)
    mesh.vertex_groups.clear()
    for group in proxy.vertex_groups:
        mesh.vertex_groups.new(name=group.name)
    select_only([mesh])
    transfer = mesh.modifiers.new("Weights", "DATA_TRANSFER")
    transfer.object = proxy
    transfer.use_vert_data = True
    transfer.data_types_verts = {"VGROUP_WEIGHTS"}
    transfer.vert_mapping = "POLYINTERP_NEAREST"
    transfer.layers_vgroup_select_src = "ALL"
    transfer.layers_vgroup_select_dst = "NAME"
    # (the transfer must come before the armature)
    while mesh.modifiers.find(transfer.name) > 0:
        bpy.ops.object.modifier_move_up(modifier=transfer.name)
    bpy.ops.object.modifier_apply(modifier=transfer.name)
    bpy.data.objects.remove(proxy)

    def strongest(slots):
        return max(slots, key=lambda slot: slot[1])[0] if slots else None

    kept = 0
    for vertex in mesh.data.vertices:
        new_slots = [(mesh.vertex_groups[slot.group].name, slot.weight) for slot in vertex.groups]
        is_weighted = sum(weight for _, weight in new_slots) > 0.05
        # 'only_bones': the new weights only where both the old and the new ones give the vertex to those bones (the
        # shoulders and arms of a body in a cape: the cloth keeps its own weights)
        is_inside = only_bones is None or (strongest(previous[vertex.index]) in only_bones and strongest(new_slots) in only_bones)
        if is_weighted and is_inside:
            continue
        kept += 1
        for slot in list(vertex.groups):
            mesh.vertex_groups[slot.group].remove([vertex.index])
        for name, weight in previous[vertex.index]:
            group = mesh.vertex_groups.get(name) or mesh.vertex_groups.new(name=name)
            group.add([vertex.index], weight, "REPLACE")
    print("PROXY {} vertices kept their generated weights".format(kept))

    select_only([mesh])
    bpy.ops.object.mode_set(mode="WEIGHT_PAINT")
    bpy.ops.object.vertex_group_limit_total(group_select_mode="ALL", limit=most_bones)
    bpy.ops.object.vertex_group_normalize_all(group_select_mode="ALL", lock_active=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    return True


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


# ========== A held shield clear of the arm ==========


def depth_through_disc(mesh, bone_names, center, face, radius, back_offset):
    """How deep the body of these bones (their vertices, in the rest pose) goes through a disc: the plane 'back_offset'
    behind 'center' along -'face' (the back of a shield's planks), within 'radius' of its middle. Below 0 = clear of it"""
    groups = {mesh.vertex_groups[name].index for name in bone_names if name in mesh.vertex_groups}
    back = center - face * back_offset
    deepest = -1.0
    for vertex in mesh.data.vertices:
        if not any(slot.group in groups and slot.weight >= 0.2 for slot in vertex.groups):
            continue
        point = mesh.matrix_world @ vertex.co
        offset = point - center
        if (offset - face * offset.dot(face)).length <= radius:
            deepest = max(deepest, (point - back).dot(face))
    return deepest


# How far a shield may be turned around its grip to keep the forearm behind it (degrees, both ways)
SHIELD_MOST_TILT = 45


def shield_along_forearm(armature, mesh, grip, side, make_matrix, face_in_model, radius, half_thickness):
    """Where a shield held by this fist stands at rest: its face on the side of the back of the fist, lying along the
    forearm and turned around the grip until the forearm (and the hand) goes away from it instead of through it.
    'make_matrix(face)': the shield's rest matrix for a face direction, 'face_in_model': the model's axis its face
    looks along. Prints how deep the arm still goes into it"""
    forearm = (armature.data.bones["Hand" + side].head_local - armature.data.bones["LowerArm" + side].head_local).normalized()
    face = grip.along.cross(forearm).normalized()
    if face.dot(-grip.palm) < 0.0:
        face = -face

    def turned(degrees):
        return make_matrix(Quaternion(grip.along, math.radians(degrees)) @ face)

    def depth(matrix):
        return depth_through_disc(mesh, ("Hand" + side, "LowerArm" + side), matrix.translation,
                                  (matrix.to_3x3() @ face_in_model).normalized(), radius, half_thickness)

    tilt = min(range(-SHIELD_MOST_TILT, SHIELD_MOST_TILT + 1, 3), key=lambda degrees: (round(max(depth(turned(degrees)), 0.0), 3), abs(degrees)))
    matrix = turned(tilt)
    print("Shield turned {} degrees around the grip, the arm goes {:.3f} m into it".format(tilt, max(depth(matrix), 0.0)))
    return matrix
