"""
Generates the boss of the example game: a giant horned knight in blackened plate armor, and its great axe

Run (from the repository root, the Mixamo files of knight_generator.py must be downloaded):
    blender --background --factory-startup --python Tools/Blender/boss_generator.py -- --out Sunta/res/Models

Writes:
    <out>/Boss/boss.gltf (+ .bin, boss_diffuse.png, boss_specular.png)              the body with the knight's animations
    <out>/BossAxe/boss_axe.gltf (+ .bin, boss_axe_diffuse.png, boss_axe_specular.png)
Optional: --preview <folder> renders pictures of the boss with its axe

Where the body comes from: the concept picture Concepts/boss_concept.png was made by Z-Image-Turbo (Apache 2.0)
and turned into a textured mesh by TRELLIS.2 (Microsoft, MIT): Generated/boss_trellis.glb. This script stands that mesh
up (2 m with the horns, facing +Y), lightens it for a game, fits a skeleton into it (the knight's bone names, so the knight's
Mixamo animations play on it), lets Blender weight the mesh to the bones and exports it with the animations

The axe is made from the picture Concepts/boss_axe_concept.png (Z-Image-Turbo): an axe is nearly flat, so its blades are
the outline cut out of the picture, made thick and painted with the picture itself, and its haft is turned like on a lathe:
at every height as wide as the picture shows (the bands and the pommel stand out). The haft is longer than in the picture
and its metal bands are recolored to brass

In the engine the boss entity is scaled up (1.5: a 3 m giant), its weapon and collider grow with it
"""

import argparse
import math
import os
import sys

import bpy
import numpy
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import knight_generator  # noqa: E402
from sunta_blender import (  # noqa: E402
    clear_scene,
    export_gltf,
    make_textured_material,
    place_preview_camera,
    select_only,
    setup_preview_render,
    show_only,
)

BLENDER_FOLDER = os.path.dirname(os.path.abspath(__file__))
BODY_SOURCE = os.path.join(BLENDER_FOLDER, "Generated", "boss_trellis.glb")

# The mesh from TRELLIS stands in a 1 m box looking along -Y
BODY_HEIGHT = 2.0
BODY_FACES = 30000

AXE_TEXTURE_PIXELS = 1024

# ========== The skeleton, fitted into the mesh ==========

# The knight's bones (see knight_generator.SKELETON), placed where the joints of this armor are:
# measured from the mesh (its width at every height), the arms and the legs stand further apart than the knight's
FORWARD = knight_generator.FORWARD
UP = knight_generator.UP

RIGHT_SIDE = [
    ("Shoulder",  "Chest",     (0.05, 0.00, 1.47), (0.26, 0.00, 1.47), UP),
    ("UpperArm",  "Shoulder",  (0.26, 0.00, 1.47), (0.35, 0.00, 1.18), FORWARD),
    ("LowerArm",  "UpperArm",  (0.35, 0.00, 1.18), (0.39, 0.03, 0.96), FORWARD),
    ("Hand",      "LowerArm",  (0.39, 0.03, 0.96), (0.40, 0.04, 0.84), FORWARD),
    ("Thigh",     "Hips",      (0.12, 0.00, 0.98), (0.18, 0.00, 0.52), FORWARD),
    ("Shin",      "Thigh",     (0.18, 0.00, 0.52), (0.23, -0.01, 0.12), FORWARD),
    ("Foot",      "Shin",      (0.23, -0.01, 0.12), (0.25, 0.16, 0.03), UP),
]


def mirrored_side(side_suffix, sign):
    bones = []
    for name, parent, head, tail, roll_axis in RIGHT_SIDE:
        parent_name = parent if parent in ("Chest", "Hips") else parent + side_suffix
        bones.append((name + side_suffix, parent_name, (head[0] * sign, head[1], head[2]), (tail[0] * sign, tail[1], tail[2]), roll_axis))
    return bones


BOSS_SKELETON = [
    ("Hips",  None,    (0.00, 0.00, 1.00), (0.00, 0.00, 1.12), FORWARD),
    ("Spine", "Hips",  (0.00, 0.00, 1.12), (0.00, 0.00, 1.30), FORWARD),
    ("Chest", "Spine", (0.00, 0.00, 1.30), (0.00, 0.00, 1.55), FORWARD),
    ("Neck",  "Chest", (0.00, 0.00, 1.55), (0.00, 0.00, 1.68), FORWARD),
    ("Head",  "Neck",  (0.00, 0.00, 1.68), (0.00, 0.00, 1.92), FORWARD),
] + mirrored_side("_R", 1.0) + mirrored_side("_L", -1.0)

# The middle of the skirt between the legs (see pin_skirt_to_hips)
SKIRT_HALF_WIDTH = 0.09
SKIRT_BOTTOM = 0.35
SKIRT_TOP = 1.0

# Where the axe's grip sits: in the middle of the right fist. The knight's sword points forward and 55 degrees down from
# the fist, the long axe forward and up, so its head doesn't sink into the floor when the Mixamo pose lowers the hand
AXE_GRIP_POSITION = Vector((0.40, 0.045, 0.88))
AXE_DOWN_DEGREES = -20.0

# ========== The axe ==========

AXE_CONCEPT = os.path.join(BLENDER_FOLDER, "Concepts", "boss_axe_concept.png")

# The blades are this wide from tip to tip (meters, before the boss is scaled up in the engine)
AXE_HEAD_WIDTH = 0.8
# The haft is stretched to this length below the head
AXE_HAFT_LENGTH = 1.9
# Where the fist holds the haft: this part of the haft from its bottom end
AXE_GRIP_FROM_BOTTOM = 0.12
AXE_BLADE_THICKNESS = 0.04

# A pixel belongs to the axe when its color differs this much from the background
AXE_OUTLINE_THRESHOLD = 0.08
# The head is the part of the picture wider than this many times the haft
AXE_HEAD_WIDTH_FACTOR = 2.5
AXE_GRID_PIXELS = 4
AXE_HAFT_RING_PIXELS = 6
AXE_HAFT_SEGMENTS = 16

BRASS = numpy.array((0.78, 0.56, 0.24))

def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--preview", default="")
    return parser.parse_args(argv)


# ========== The body ==========


def load_body():
    """The TRELLIS mesh standing on the ground at the origin, facing +Y, BODY_HEIGHT tall, lightened to BODY_FACES"""
    objects_before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=BODY_SOURCE)
    imported = [obj for obj in bpy.data.objects if obj not in objects_before]

    body = next(obj for obj in imported if obj.type == "MESH")
    world_matrix = body.matrix_world.copy()
    body.parent = None
    for obj in imported:
        if obj is not body:
            bpy.data.objects.remove(obj)

    body.data.transform(world_matrix)
    body.matrix_world = Matrix.Identity(4)

    lowest = min(vertex.co.z for vertex in body.data.vertices)
    highest = max(vertex.co.z for vertex in body.data.vertices)
    scale = BODY_HEIGHT / (highest - lowest)
    body.data.transform(Matrix.Rotation(math.pi, 4, "Z") @ Matrix.Scale(scale, 4) @ Matrix.Translation((0.0, 0.0, -lowest)))

    decimate = body.modifiers.new("Lighten", "DECIMATE")
    decimate.ratio = min(1.0, BODY_FACES / len(body.data.polygons))
    select_only([body])
    bpy.ops.object.modifier_apply(modifier=decimate.name)

    body.name = "Boss"
    return body


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
    image.filepath_raw = os.path.join(folder, name + ".png")
    image.file_format = "PNG"
    image.save()
    return image


def make_body_textures(body, folder):
    """The engine reads a diffuse and a specular texture: the color comes from TRELLIS's base color,
    the shine from its metal and roughness maps (smooth metal shines, rough cloth and leather don't)"""
    material = body.data.materials[0]
    color = image_pixels(find_texture(material, "Base Color"))
    diffuse = save_image("boss_diffuse", folder, color, is_color=True)

    metal_roughness = find_texture(material, "Metallic") or find_texture(material, "Roughness")
    if metal_roughness:
        channels = image_pixels(metal_roughness)
        roughness, metallic = channels[..., 1], channels[..., 2]
        shine = numpy.clip(0.08 + 0.55 * metallic * (1.0 - roughness) + 0.25 * (1.0 - roughness), 0.0, 1.0)
    else:
        shine = numpy.full(color.shape[:2], 0.3, dtype=numpy.float32)

    specular = numpy.ones_like(color)
    specular[..., 0] = specular[..., 1] = specular[..., 2] = shine
    save_image("boss_specular", folder, specular, is_color=False)

    body.data.materials.clear()
    body.data.materials.append(make_textured_material("boss", diffuse))


# ========== Fitting the body to the skeleton ==========


def distance_to_bone(point, head, tail):
    along = tail - head
    t = max(0.0, min(1.0, (point - head).dot(along) / along.length_squared))
    return (point - (head + along * t)).length


def weight_unreached_vertices(body, armature):
    """Blender's automatic weights miss parts it can't reach through the mesh (loose plates, horns): they follow the nearest bone"""
    bones = [bone for bone in armature.data.bones if bone.use_deform]
    groups = {bone.name: body.vertex_groups.get(bone.name) or body.vertex_groups.new(name=bone.name) for bone in bones}

    for vertex in body.data.vertices:
        if any(group.weight > 0.0 for group in vertex.groups):
            continue
        nearest = min(bones, key=lambda bone: distance_to_bone(vertex.co, bone.head_local, bone.tail_local))
        groups[nearest.name].add([vertex.index], 1.0, "REPLACE")


def pin_skirt_to_hips(body):
    """The long plates hanging between the legs (the middle of the skirt) follow the hips: weighted to both thighs
    they would stretch into a triangle with every step"""
    hips = body.vertex_groups.get("Hips") or body.vertex_groups.new(name="Hips")
    others = [group for group in body.vertex_groups if group.name != "Hips"]

    for vertex in body.data.vertices:
        if abs(vertex.co.x) < SKIRT_HALF_WIDTH and SKIRT_BOTTOM < vertex.co.z < SKIRT_TOP:
            for group in others:
                group.remove([vertex.index])
            hips.add([vertex.index], 1.0, "REPLACE")


def skin_to_skeleton(body, armature):
    select_only([body, armature])
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.parent_set(type="ARMATURE_AUTO")
    weight_unreached_vertices(body, armature)
    pin_skirt_to_hips(body)


# ========== The axe ==========


def get_axe_rest_matrix():
    """Where the axe object stands in the rest pose (built with the haft along +Z, the blades along X)"""
    angle = math.radians(AXE_DOWN_DEGREES)
    haft = Vector((0.0, math.cos(angle), -math.sin(angle)))
    # The blade looks down at rest: raised and swung forward, it leads the cut
    toward_back_of_blade = Vector((0.0, math.sin(angle), math.cos(angle)))
    side = haft.cross(toward_back_of_blade)
    rotation = Matrix((toward_back_of_blade, side, haft)).transposed()
    return Matrix.Translation(AXE_GRIP_POSITION) @ rotation.to_4x4()


# ========== The axe from its picture ==========


class AxePicture:
    """The concept picture read into numbers: its colors, the outline of the axe, where the head ends and the haft is"""

    def __init__(self, path):
        self.image = bpy.data.images.load(path, check_existing=True)
        self.width, self.height = self.image.size
        # Rows from the top, like the picture is seen
        self.colors = image_pixels(self.image)[::-1, :, :3].copy()

        border = numpy.concatenate([self.colors[0], self.colors[-1], self.colors[:, 0], self.colors[:, -1]])
        background = numpy.median(border, axis=0)
        self.outline = numpy.linalg.norm(self.colors - background, axis=2) > AXE_OUTLINE_THRESHOLD

        rows = numpy.where(self.outline.any(axis=1))[0]
        columns = numpy.where(self.outline.any(axis=0))[0]
        self.top_row, self.bottom_row = int(rows.min()), int(rows.max())
        self.center_column = (columns.min() + columns.max()) * 0.5

        widths = self.outline.sum(axis=1)
        haft_width = numpy.median(widths[rows])
        head_rows = rows[widths[rows] > haft_width * AXE_HEAD_WIDTH_FACTOR]
        self.head_bottom_row = int(head_rows.max())
        self.head_width_pixels = columns.max() - columns.min()

    def row_width(self, row):
        return int(self.outline[row].sum())


def brass_bands(picture):
    """The colors of the axe with the grey metal of the haft (bands, pommel) turned into brass"""
    colors = picture.colors.copy()
    brightness = colors.mean(axis=2)
    saturation = colors.max(axis=2) - colors.min(axis=2)

    is_haft = numpy.zeros_like(picture.outline)
    is_haft[picture.head_bottom_row + 1:] = True
    is_metal = picture.outline & is_haft & (saturation < 0.08) & (brightness > 0.3)

    colors[is_metal] = BRASS * (0.55 + brightness[is_metal, None])
    return colors, is_metal


def make_axe_textures(picture, folder):
    colors, is_brass = brass_bands(picture)
    rgba = numpy.ones((picture.height, picture.width, 4), dtype=numpy.float32)
    rgba[..., :3] = colors
    rgba[..., 3] = picture.outline

    # Shine: the iron and the brass shine, the wood and the leather don't
    saturation = colors.max(axis=2) - colors.min(axis=2)
    is_metal = picture.outline & ((saturation < 0.1) | is_brass)
    shine = numpy.where(is_metal, 0.35 + 0.5 * colors.mean(axis=2), 0.08)
    specular = numpy.ones_like(rgba)
    specular[..., 0] = specular[..., 1] = specular[..., 2] = shine

    # The images are saved bottom row first (Blender's order)
    diffuse = save_image("boss_axe_diffuse", folder, rgba[::-1], is_color=True, has_alpha=True)
    save_image("boss_axe_specular", folder, specular[::-1], is_color=False)
    return diffuse


class AxeMeasure:
    """Pixels of the picture to meters of the axe: the grip at the origin, the haft along +Z, the head at the top"""

    def __init__(self, picture):
        self.picture = picture
        self.meters_per_pixel = AXE_HEAD_WIDTH / picture.head_width_pixels
        haft_pixels = picture.bottom_row - picture.head_bottom_row
        self.haft_stretch = AXE_HAFT_LENGTH / (haft_pixels * self.meters_per_pixel)
        self.grip_z = (AXE_HAFT_LENGTH * AXE_GRIP_FROM_BOTTOM)

    def x(self, column):
        return (column - self.picture.center_column) * self.meters_per_pixel

    def z(self, row):
        """The haft is stretched, the head keeps its proportions"""
        picture = self.picture
        if row >= picture.head_bottom_row:
            return (picture.bottom_row - row) * self.meters_per_pixel * self.haft_stretch - self.grip_z
        head_bottom = (picture.bottom_row - picture.head_bottom_row) * self.meters_per_pixel * self.haft_stretch
        return head_bottom + (picture.head_bottom_row - row) * self.meters_per_pixel - self.grip_z

    def uv(self, column, row):
        return (column / self.picture.width, 1.0 - row / self.picture.height)


def build_axe_head(picture, measure):
    """The outline of the head (and the spike on top) as a flat grid of squares, made thick with Solidify"""
    step = AXE_GRID_PIXELS
    vertices, faces, uvs, index_of = [], [], [], {}

    def vertex(column, row):
        key = (column, row)
        if key not in index_of:
            index_of[key] = len(vertices)
            vertices.append((measure.x(column), 0.0, measure.z(row)))
            uvs.append(measure.uv(column, row))
        return index_of[key]

    last_row = picture.head_bottom_row + 4 * step
    for row in range(picture.top_row, last_row, step):
        for column in range(0, picture.width - step, step):
            if picture.outline[min(row + step // 2, picture.height - 1), column + step // 2]:
                faces.append((vertex(column, row + step), vertex(column + step, row + step), vertex(column + step, row), vertex(column, row)))

    head = mesh_object("BossAxeHead", vertices, faces, uvs)
    solidify = head.modifiers.new("Thickness", "SOLIDIFY")
    solidify.thickness = AXE_BLADE_THICKNESS
    solidify.offset = 0.0
    select_only([head])
    bpy.ops.object.modifier_apply(modifier=solidify.name)
    return head


def build_axe_haft(picture, measure):
    """Rings around the haft, each as wide as the picture shows at its height, painted with the picture wrapped around"""
    vertices, faces, uvs = [], [], []
    rows = list(range(picture.bottom_row, picture.head_bottom_row - 1, -AXE_HAFT_RING_PIXELS))
    segments = AXE_HAFT_SEGMENTS

    for ring, row in enumerate(rows):
        half_width_pixels = max(picture.row_width(row), 4) * 0.5
        for segment in range(segments + 1):
            angle = 2.0 * math.pi * segment / segments
            column = picture.center_column + math.sin(angle) * half_width_pixels
            radius = half_width_pixels * measure.meters_per_pixel
            vertices.append((math.sin(angle) * radius, math.cos(angle) * radius, measure.z(row)))
            uvs.append(measure.uv(column, row))
            if ring > 0 and segment < segments:
                current, previous = ring * (segments + 1) + segment, (ring - 1) * (segments + 1) + segment
                faces.append((previous, previous + 1, current + 1, current))

    return mesh_object("BossAxeHaft", vertices, faces, uvs)


def mesh_object(name, vertices, faces, uvs):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    uv_layer = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        uv_layer.data[loop.index].uv = uvs[loop.vertex_index]
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def build_axe(folder):
    picture = AxePicture(AXE_CONCEPT)
    measure = AxeMeasure(picture)
    texture = make_axe_textures(picture, folder)

    head = build_axe_head(picture, measure)
    haft = build_axe_haft(picture, measure)
    select_only([head, haft])
    bpy.context.view_layer.objects.active = head
    bpy.ops.object.join()

    head.name = "BossAxe"
    head.data.materials.append(make_textured_material("boss_axe", texture))

    # The head's box for the weapon's Hitbox (glTF axes: the haft along +Y)
    head_middle = (measure.z(picture.top_row) + measure.z(picture.head_bottom_row)) * 0.5
    head_half_height = (measure.z(picture.top_row) - measure.z(picture.head_bottom_row)) * 0.5
    print("AXE_HITBOX local_offset (0, {:.3f}, 0) half_extents ({:.3f}, {:.3f}, {:.3f})".format(
        head_middle, AXE_HEAD_WIDTH * 0.5, head_half_height, AXE_BLADE_THICKNESS * 2))
    return head


# ========== Pictures ==========


def render_previews(folder, armature, body, axe):
    os.makedirs(folder, exist_ok=True)
    camera = setup_preview_render(400, 500)
    show_only([body, axe])
    scene = bpy.context.scene

    for action_name in ("Idle", "Attack_Heavy", "Attack_Light_1", "Walk"):
        armature.animation_data.action = bpy.data.actions[action_name]
        start, end = (int(frame) for frame in armature.animation_data.action.frame_range)
        for step in range(4):
            frame = start + round((end - start) * step / 3)
            scene.frame_set(frame)
            place_preview_camera(camera, 35, distance=4.6, height=1.4, target_height=1.1)
            scene.render.filepath = os.path.join(folder, "{}_{:02d}.png".format(action_name, frame))
            bpy.ops.render.render(write_still=True)


# ========== Main ==========


def main():
    arguments = parse_arguments()
    out_folder = os.path.abspath(arguments.out)
    boss_folder = os.path.join(out_folder, "Boss")
    axe_folder = os.path.join(out_folder, "BossAxe")
    for folder in (boss_folder, axe_folder):
        os.makedirs(folder, exist_ok=True)

    clear_scene()

    body = load_body()
    make_body_textures(body, boss_folder)

    held_axe = [(knight_generator.WEAPON_BONE, knight_generator.WEAPON_PARENT_BONE, get_axe_rest_matrix())]
    armature = knight_generator.build_skeleton(BOSS_SKELETON, held_axe)
    skin_to_skeleton(body, armature)
    knight_generator.create_animations(armature)
    knight_generator.reset_to_rest_pose(armature)

    axe = build_axe(axe_folder)

    export_gltf(os.path.join(boss_folder, "boss.gltf"), [armature, body], with_animations=True)
    export_gltf(os.path.join(axe_folder, "boss_axe.gltf"), [axe], with_animations=False)

    if arguments.preview:
        knight_generator.hang_on_bone(axe, armature, knight_generator.WEAPON_BONE, get_axe_rest_matrix())
        render_previews(os.path.abspath(arguments.preview), armature, body, axe)

    print("BOSS_GENERATOR_DONE")


if __name__ == "__main__":
    main()
