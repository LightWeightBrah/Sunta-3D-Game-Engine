"""
Generates the Knight player model, its animations and its sword for the Sunta engine.

Run (from the repository root):
    blender --background --factory-startup --python Tools/Blender/knight_generator.py -- --out Sunta/res/Models

Writes:
    <out>/Knight/knight.gltf (+ .bin, knight_diffuse.png, knight_specular.png)
    <out>/KnightSword/knight_sword.gltf (+ .bin, knight_sword_diffuse.png, knight_sword_specular.png)
Optional: --preview <folder> renders pictures of the model and of every animation, --blend <file> saves the .blend,
--icon <file> renders the editor icon (a picture of the helmet)

The armor is built from simple shapes, every color is a cell of a small palette. Cycles then bakes the final textures:
the palette color with fine metal grain, large stains, darker crevices (ambient occlusion) and brighter worn edges

Conventions (Blender axes): X = the knight's right, Y = forward, Z = up
The glTF exporter turns this into the engine's axes: Y up and forward along -Z

Every armor piece follows exactly one bone (rigid skinning), the joints are hidden under overlapping plates
The sword is a separate model. The bone 'Weapon_R' in the right hand is placed so that the sword attached to it
without any offset sits in the hand (in the engine: the sword is a child entity on that bone)
"""

import argparse
import math
import os
import sys

import bmesh
import bpy
from mathutils import Euler, Matrix, Quaternion, Vector

FRAMES_PER_SECOND = 30

# ========== Colors ==========

# Every color is one cell of a small texture, the UVs of a part point into the middle of its cell
PALETTE_GRID = 4
PALETTE_CELL_PIXELS = 64

# name: (diffuse RGB, specular intensity)
KNIGHT_COLORS = {
    "steel":        ((0.40, 0.41, 0.43), 0.75),
    "steel_dark":   ((0.20, 0.21, 0.22), 0.55),
    "steel_worn":   ((0.33, 0.33, 0.34), 0.55),
    "mail":         ((0.24, 0.24, 0.25), 0.45),
    "leather":      ((0.22, 0.14, 0.08), 0.12),
    "leather_dark": ((0.12, 0.08, 0.05), 0.08),
    "brass":        ((0.46, 0.35, 0.16), 0.70),
    "black":        ((0.01, 0.01, 0.01), 0.00),
}

SWORD_COLORS = {
    "blade":       ((0.62, 0.63, 0.65), 1.00),
    "blade_edge":  ((0.78, 0.79, 0.80), 1.00),
    "fuller":      ((0.34, 0.35, 0.37), 0.70),
    "guard":       ((0.26, 0.27, 0.28), 0.70),
    "brass":       ((0.46, 0.35, 0.16), 0.70),
    "grip":        ((0.13, 0.08, 0.05), 0.08),
    "wrap":        ((0.22, 0.14, 0.08), 0.12),
}

# A specular value above this marks metal: only metal gets brighter worn edges
METAL_SPECULAR = 0.5

# ========== Baked textures ==========

PALETTE_UV = "Palette"
BAKED_UV = "UVMap"
KNIGHT_TEXTURE_PIXELS = 2048
SWORD_TEXTURE_PIXELS = 1024
BAKE_SAMPLES = 16
BAKE_MARGIN_PIXELS = 8

# Smart UV Project: faces bent by more than this end up on separate islands
UV_ANGLE_LIMIT_DEGREES = 60.0
UV_ISLAND_MARGIN = 0.004

# Edges sharper than this stay sharp, the rest is shaded smooth
SHARP_EDGE_DEGREES = 35.0

# Size of the chamfer on the edges of box shaped plates (meters)
BOX_BEVEL = 0.005

# ========== Skeleton ==========

# name, parent, head, tail, the direction the bone's Z axis should point to (roll)
FORWARD = (0.0, 1.0, 0.0)
UP = (0.0, 0.0, 1.0)

SKELETON = [
    ("Hips",       None,         (0.00, 0.00, 0.98), (0.00, 0.00, 1.10), FORWARD),
    ("Spine",      "Hips",       (0.00, 0.00, 1.10), (0.00, 0.00, 1.28), FORWARD),
    ("Chest",      "Spine",      (0.00, 0.00, 1.28), (0.00, 0.00, 1.50), FORWARD),
    ("Neck",       "Chest",      (0.00, 0.00, 1.50), (0.00, 0.00, 1.60), FORWARD),
    ("Head",       "Neck",       (0.00, 0.00, 1.60), (0.00, 0.00, 1.82), FORWARD),

    ("Shoulder_R", "Chest",      (0.04, 0.00, 1.45), (0.19, 0.00, 1.45), UP),
    ("UpperArm_R", "Shoulder_R", (0.19, 0.00, 1.45), (0.23, 0.00, 1.17), FORWARD),
    ("LowerArm_R", "UpperArm_R", (0.23, 0.00, 1.17), (0.25, 0.03, 0.92), FORWARD),
    ("Hand_R",     "LowerArm_R", (0.25, 0.03, 0.92), (0.25, 0.04, 0.82), FORWARD),

    ("Shoulder_L", "Chest",      (-0.04, 0.00, 1.45), (-0.19, 0.00, 1.45), UP),
    ("UpperArm_L", "Shoulder_L", (-0.19, 0.00, 1.45), (-0.23, 0.00, 1.17), FORWARD),
    ("LowerArm_L", "UpperArm_L", (-0.23, 0.00, 1.17), (-0.25, 0.03, 0.92), FORWARD),
    ("Hand_L",     "LowerArm_L", (-0.25, 0.03, 0.92), (-0.25, 0.04, 0.82), FORWARD),

    ("Thigh_R",    "Hips",       (0.10, 0.00, 0.97), (0.11, 0.00, 0.54), FORWARD),
    ("Shin_R",     "Thigh_R",    (0.11, 0.00, 0.54), (0.11, -0.01, 0.10), FORWARD),
    ("Foot_R",     "Shin_R",     (0.11, -0.01, 0.10), (0.11, 0.14, 0.03), UP),

    ("Thigh_L",    "Hips",       (-0.10, 0.00, 0.97), (-0.11, 0.00, 0.54), FORWARD),
    ("Shin_L",     "Thigh_L",    (-0.11, 0.00, 0.54), (-0.11, -0.01, 0.10), FORWARD),
    ("Foot_L",     "Shin_L",     (-0.11, -0.01, 0.10), (-0.11, 0.14, 0.03), UP),
]

WEAPON_BONE = "Weapon_R"
WEAPON_PARENT_BONE = "Hand_R"
WEAPON_BONE_LENGTH = 0.15

# Where the sword grip sits in the rest pose (the middle of the right fist) and where its blade points:
# forward and 55 degrees down, the edges up and down
SWORD_GRIP_POSITION = Vector((0.25, 0.045, 0.855))
SWORD_BLADE_DOWN_DEGREES = 55.0

# ========== Sword size (meters) ==========

SWORD_BLADE_LENGTH = 0.86
SWORD_BLADE_HALF_WIDTH = 0.026
SWORD_BLADE_HALF_THICKNESS = 0.007
SWORD_GRIP_HALF_LENGTH = 0.10
SWORD_GUARD_HALF_WIDTH = 0.12

# ========== Small helpers ==========


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--preview", default="")
    parser.add_argument("--blend", default="")
    parser.add_argument("--icon", default="")
    return parser.parse_args(argv)


def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.fps = FRAMES_PER_SECOND


def degrees_to_quaternion(degrees_xyz):
    return Euler([math.radians(angle) for angle in degrees_xyz], "XYZ").to_quaternion()


def make_matrix(location=(0, 0, 0), rotation_degrees=(0, 0, 0), scale=(1, 1, 1)):
    return Matrix.LocRotScale(Vector(location), degrees_to_quaternion(rotation_degrees), Vector(scale))


def matrix_aligning_z_to(head, tail):
    """Puts a shape built along +Z (from -depth/2 to +depth/2) between two points"""
    head, tail = Vector(head), Vector(tail)
    direction = tail - head
    rotation = Vector((0, 0, 1)).rotation_difference(direction.normalized())
    return Matrix.LocRotScale((head + tail) * 0.5, rotation, Vector((1, 1, 1)))


# ========== Palette texture ==========


class Palette:
    def __init__(self, colors):
        self.names = list(colors.keys())
        self.colors = colors
        assert len(self.names) <= PALETTE_GRID * PALETTE_GRID

    def uv(self, name):
        index = self.names.index(name)
        column, row = index % PALETTE_GRID, index // PALETTE_GRID
        return ((column + 0.5) / PALETTE_GRID, (row + 0.5) / PALETTE_GRID)

    def make_images(self, base_name):
        """The diffuse and the specular palette, only in memory (they are the input of the bake)"""
        size = PALETTE_GRID * PALETTE_CELL_PIXELS
        diffuse = bpy.data.images.new(base_name + "_palette_diffuse", size, size, alpha=False)
        specular = bpy.data.images.new(base_name + "_palette_specular", size, size, alpha=False)

        diffuse_pixels = [0.0] * (size * size * 4)
        specular_pixels = [0.0] * (size * size * 4)

        for index, name in enumerate(self.names):
            (red, green, blue), intensity = self.colors[name]
            column, row = index % PALETTE_GRID, index // PALETTE_GRID
            for y in range(row * PALETTE_CELL_PIXELS, (row + 1) * PALETTE_CELL_PIXELS):
                for x in range(column * PALETTE_CELL_PIXELS, (column + 1) * PALETTE_CELL_PIXELS):
                    pixel = (y * size + x) * 4
                    diffuse_pixels[pixel:pixel + 4] = [red, green, blue, 1.0]
                    specular_pixels[pixel:pixel + 4] = [intensity, intensity, intensity, 1.0]

        diffuse.pixels.foreach_set(diffuse_pixels)
        specular.colorspace_settings.name = "Non-Color"
        specular.pixels.foreach_set(specular_pixels)
        return diffuse, specular


def make_textured_material(name, diffuse_image):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    shader = nodes.get("Principled BSDF")
    shader.inputs["Metallic"].default_value = 0.0
    shader.inputs["Roughness"].default_value = 0.5

    texture = nodes.new("ShaderNodeTexImage")
    texture.image = diffuse_image
    material.node_tree.links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    return material


# ========== Building meshes out of simple shapes ==========


class PartBuilder:
    """Collects simple shapes into one mesh. Every shape has one color and follows one bone"""

    def __init__(self, palette):
        self.palette = palette
        self.mesh = bmesh.new()
        self.uv_layer = self.mesh.loops.layers.uv.verify()
        self.deform_layer = self.mesh.verts.layers.deform.verify()
        self.bone_indices = {}

    def bone_index(self, bone):
        if bone not in self.bone_indices:
            self.bone_indices[bone] = len(self.bone_indices)
        return self.bone_indices[bone]

    def _finish_shape(self, result_verts, matrix, color, bone, bevel=0.0):
        verts = [element for element in result_verts if isinstance(element, bmesh.types.BMVert)]
        bmesh.ops.transform(self.mesh, matrix=matrix, verts=verts)

        if bevel > 0.0:
            edges = list({edge for vert in verts for edge in vert.link_edges})
            bevelled = bmesh.ops.bevel(self.mesh, geom=edges, offset=bevel, segments=1, affect="EDGES", profile=0.5)
            verts = [vert for vert in verts if vert.is_valid] + list(bevelled["verts"])

        faces = {face for vert in verts for face in vert.link_faces}
        uv = self.palette.uv(color)
        for face in faces:
            for loop in face.loops:
                loop[self.uv_layer].uv = uv

        if bone is not None:
            index = self.bone_index(bone)
            for vert in verts:
                vert[self.deform_layer][index] = 1.0

    def box(self, bone, color, location, size, rotation=(0, 0, 0), bevel=BOX_BEVEL):
        result = bmesh.ops.create_cube(self.mesh, size=1.0)
        matrix = make_matrix(location, rotation, size)
        self._finish_shape(result["verts"], matrix, color, bone, bevel)

    def cylinder(self, bone, color, matrix, radius_bottom, radius_top, depth, segments=20, scale=(1, 1, 1)):
        result = bmesh.ops.create_cone(self.mesh, cap_ends=True, cap_tris=False, segments=segments,
                                       radius1=radius_bottom, radius2=radius_top, depth=depth)
        self._finish_shape(result["verts"], matrix @ Matrix.Diagonal(Vector((*scale, 1.0))), color, bone)

    def limb(self, bone, color, head, tail, radius_head, radius_tail, segments=16, scale=(1, 1, 1)):
        """A tapered tube between two points (radius_head at 'head')"""
        depth = (Vector(tail) - Vector(head)).length
        self.cylinder(bone, color, matrix_aligning_z_to(head, tail), radius_head, radius_tail, depth, segments, scale)

    def sphere(self, bone, color, location, radii, rotation=(0, 0, 0), segments=20, rings=12):
        result = bmesh.ops.create_uvsphere(self.mesh, u_segments=segments, v_segments=rings, radius=1.0)
        matrix = make_matrix(location, rotation, radii)
        self._finish_shape(result["verts"], matrix, color, bone)

    def half_sphere(self, bone, color, location, radii, rotation=(0, 0, 0), segments=20, rings=10):
        """The upper half (+Z) of a sphere, open at the bottom"""
        result = bmesh.ops.create_uvsphere(self.mesh, u_segments=segments, v_segments=rings, radius=1.0)
        lower_verts = [vert for vert in result["verts"] if vert.co.z < -1e-4]
        bmesh.ops.delete(self.mesh, geom=lower_verts, context="VERTS")
        remaining = [vert for vert in result["verts"] if vert.is_valid]
        matrix = make_matrix(location, rotation, radii)
        self._finish_shape(remaining, matrix, color, bone)

    def _shade_smooth_with_sharp_edges(self):
        sharp_angle = math.radians(SHARP_EDGE_DEGREES)
        for face in self.mesh.faces:
            face.smooth = True
        for edge in self.mesh.edges:
            edge.smooth = not (len(edge.link_faces) == 2 and edge.calc_face_angle(0.0) > sharp_angle)

    def to_object(self, name, armature=None):
        mesh = bpy.data.meshes.new(name)
        self._shade_smooth_with_sharp_edges()
        self.mesh.normal_update()
        self.mesh.to_mesh(mesh)
        self.mesh.free()
        mesh.uv_layers[0].name = PALETTE_UV

        mesh_object = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(mesh_object)

        for bone, _ in sorted(self.bone_indices.items(), key=lambda item: item[1]):
            mesh_object.vertex_groups.new(name=bone)

        if armature:
            mesh_object.parent = armature
            modifier = mesh_object.modifiers.new("Armature", "ARMATURE")
            modifier.object = armature

        return mesh_object


# ========== The knight ==========


def build_skeleton():
    armature_data = bpy.data.armatures.new("KnightArmature")
    armature = bpy.data.objects.new("Armature", armature_data)
    bpy.context.scene.collection.objects.link(armature)
    bpy.context.view_layer.objects.active = armature

    bpy.ops.object.mode_set(mode="EDIT")
    edit_bones = armature_data.edit_bones

    for name, parent, head, tail, roll_axis in SKELETON:
        bone = edit_bones.new(name)
        bone.head, bone.tail = head, tail
        bone.align_roll(Vector(roll_axis))
        if parent:
            bone.parent = edit_bones[parent]
            bone.use_connect = False

    weapon = edit_bones.new(WEAPON_BONE)
    # A new bone has no length and no direction, it takes the matrix only after it got some
    weapon.head, weapon.tail = (0.0, 0.0, 0.0), (0.0, WEAPON_BONE_LENGTH, 0.0)
    weapon.matrix = get_weapon_bone_matrix()
    weapon.parent = edit_bones[WEAPON_PARENT_BONE]
    weapon.use_deform = False

    bpy.ops.object.mode_set(mode="OBJECT")
    return armature


def get_sword_rest_matrix():
    """Where the sword object stands in the rest pose (Blender axes, sword built with the blade along +Z)"""
    angle = math.radians(SWORD_BLADE_DOWN_DEGREES)
    blade = Vector((0.0, math.cos(angle), -math.sin(angle)))
    edge = Vector((0.0, math.sin(angle), math.cos(angle)))
    flat = blade.cross(edge)
    rotation = Matrix((edge, flat, blade)).transposed()
    return Matrix.Translation(SWORD_GRIP_POSITION) @ rotation.to_4x4()


# The glTF exporter turns Blender axes (Z up) into glTF axes (Y up): (x, y, z) -> (x, z, -y)
BLENDER_TO_GLTF = Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, -1, 0, 0), (0, 0, 0, 1)))


def get_weapon_bone_matrix():
    """The sword file is converted to glTF axes on its own, the bone must carry the same frame:
    bone frame in glTF = sword placement in glTF, so in Blender the bone frame is sword frame * (Blender->glTF)^-1"""
    return get_sword_rest_matrix() @ BLENDER_TO_GLTF.inverted()


def build_head(parts):
    """A close helm: rounded skull, a chin plate, a narrow eye slit and breathing holes"""
    parts.sphere("Head", "steel", (0.0, 0.0, 1.725), (0.108, 0.118, 0.128), segments=28, rings=16)
    parts.sphere("Head", "steel_worn", (0.0, 0.03, 1.662), (0.098, 0.1, 0.082), segments=28, rings=14)
    parts.cylinder("Head", "steel_dark", make_matrix((0.0, 0.005, 1.612)), 0.1, 0.098, 0.03, segments=28)

    # The eye slit follows the curve of the face in four short pieces
    for angle_degrees in (-36, -12, 12, 36):
        angle = math.radians(angle_degrees)
        location = (math.sin(angle) * 0.106, math.cos(angle) * 0.116, 1.738)
        parts.box("Head", "black", location, (0.048, 0.02, 0.011), rotation=(0, 0, -angle_degrees), bevel=0.0)

    # Breathing holes on the right cheek
    for row in range(3):
        for column in range(2):
            angle_degrees = 22 + column * 12
            angle = math.radians(angle_degrees)
            location = (math.sin(angle) * 0.098, math.cos(angle) * 0.1, 1.675 - row * 0.016)
            parts.box("Head", "black", location, (0.008, 0.016, 0.006), rotation=(0, 0, -angle_degrees), bevel=0.0)


def build_torso(parts):
    # Gorget around the neck
    parts.cylinder("Neck", "steel_worn", make_matrix((0, 0.0, 1.565)), 0.105, 0.088, 0.08)
    parts.cylinder("Chest", "steel", make_matrix((0, 0.0, 1.515)), 0.15, 0.11, 0.05)

    # Breastplate with a ridge down the middle and a dark rim at the bottom
    parts.sphere("Chest", "steel", (0.0, 0.02, 1.37), (0.2, 0.152, 0.19), segments=28, rings=16)
    parts.box("Chest", "steel_worn", (0.0, 0.158, 1.40), (0.024, 0.024, 0.2), rotation=(0, 0, 45))
    parts.cylinder("Chest", "steel_dark", make_matrix((0, 0.015, 1.255)), 0.178, 0.186, 0.028, scale=(1.0, 0.8, 1.0))

    # Belly under the plate, belt with a buckle
    parts.cylinder("Spine", "mail", make_matrix((0, 0.0, 1.18)), 0.16, 0.17, 0.18, scale=(1.0, 0.8, 1.0))
    parts.cylinder("Hips", "leather", make_matrix((0, 0.0, 1.05)), 0.175, 0.175, 0.05, scale=(1.0, 0.8, 1.0))
    parts.box("Hips", "brass", (0.0, 0.145, 1.05), (0.05, 0.016, 0.045))

    # Fauld (the plates over the hips) and a mail skirt that stays short, so the legs never cut through it
    parts.cylinder("Hips", "steel", make_matrix((0, 0.0, 0.975)), 0.212, 0.18, 0.10, scale=(1.0, 0.82, 1.0))
    parts.cylinder("Hips", "mail", make_matrix((0, 0.0, 0.90)), 0.20, 0.205, 0.07, scale=(1.0, 0.82, 1.0))


def build_arm(parts, side):
    suffix = "_R" if side > 0 else "_L"

    def mirrored(point):
        return (point[0] * side, point[1], point[2])

    shoulder, upper_arm, lower_arm, hand = ("Shoulder" + suffix, "UpperArm" + suffix, "LowerArm" + suffix, "Hand" + suffix)

    # Pauldron: three overlapping plates with a dark rim
    for index, (radius, z) in enumerate(((0.1, 1.47), (0.092, 1.425), (0.084, 1.39))):
        parts.half_sphere(shoulder, "steel" if index == 0 else "steel_worn", mirrored((0.20, 0.0, z)), (radius, radius * 0.95, radius * 0.7),
                          rotation=(0, side * 25, 0))
    parts.cylinder(shoulder, "steel_dark", make_matrix(mirrored((0.20, 0.0, 1.466)), (0, side * 25, 0)), 0.102, 0.102, 0.012)

    # Upper arm: mail with a plate on the outside
    parts.limb(upper_arm, "mail", mirrored((0.19, 0.0, 1.42)), mirrored((0.23, 0.0, 1.17)), 0.058, 0.05)
    parts.limb(upper_arm, "steel_worn", mirrored((0.20, 0.0, 1.36)), mirrored((0.23, 0.0, 1.20)), 0.062, 0.055)

    # Elbow cop with a small round wing
    parts.sphere(lower_arm, "steel", mirrored((0.23, 0.0, 1.17)), (0.055, 0.055, 0.055))
    parts.cylinder(lower_arm, "steel_worn", make_matrix(mirrored((0.265, -0.005, 1.17)), (0, 90, 0)), 0.045, 0.045, 0.012)

    # Vambrace and gauntlet
    parts.limb(lower_arm, "steel", mirrored((0.23, 0.0, 1.15)), mirrored((0.25, 0.03, 0.95)), 0.05, 0.042)
    parts.limb(hand, "steel_worn", mirrored((0.25, 0.03, 0.96)), mirrored((0.25, 0.034, 0.90)), 0.05, 0.062)
    parts.box(hand, "steel_dark", mirrored((0.25, 0.04, 0.86)), (0.05, 0.08, 0.09))
    parts.box(hand, "steel_dark", mirrored((0.25, 0.075, 0.835)), (0.048, 0.04, 0.06))


def build_leg(parts, side):
    suffix = "_R" if side > 0 else "_L"

    def mirrored(point):
        return (point[0] * side, point[1], point[2])

    thigh, shin, foot = "Thigh" + suffix, "Shin" + suffix, "Foot" + suffix

    # Cuisse (thigh plate) with a tasset hanging over it
    parts.limb(thigh, "mail", mirrored((0.10, 0.0, 0.97)), mirrored((0.11, 0.0, 0.56)), 0.085, 0.065)
    parts.limb(thigh, "steel", mirrored((0.10, 0.012, 0.88)), mirrored((0.11, 0.012, 0.58)), 0.08, 0.066)
    parts.box(thigh, "steel_worn", mirrored((0.105, 0.075, 0.86)), (0.14, 0.025, 0.14), rotation=(8, 0, 0))

    # Knee cop with a pointed front
    parts.sphere(shin, "steel", mirrored((0.11, 0.015, 0.54)), (0.065, 0.06, 0.065))
    parts.box(shin, "steel_worn", mirrored((0.11, 0.07, 0.545)), (0.045, 0.04, 0.045), rotation=(0, 45, 0))

    # Greave
    parts.limb(shin, "steel", mirrored((0.11, 0.0, 0.52)), mirrored((0.11, -0.01, 0.13)), 0.062, 0.048)

    # Sabaton
    parts.cylinder(foot, "steel_worn", make_matrix(mirrored((0.11, -0.005, 0.11))), 0.055, 0.05, 0.07)
    parts.box(foot, "steel_dark", mirrored((0.11, 0.04, 0.05)), (0.10, 0.20, 0.08))
    parts.box(foot, "steel_worn", mirrored((0.11, 0.155, 0.04)), (0.085, 0.08, 0.06), rotation=(-12, 0, 0))
    parts.box(foot, "leather_dark", mirrored((0.11, 0.03, 0.006)), (0.10, 0.22, 0.012), bevel=0.002)


def build_knight_mesh(armature, palette):
    parts = PartBuilder(palette)
    build_head(parts)
    build_torso(parts)
    for side in (1, -1):
        build_arm(parts, side)
        build_leg(parts, side)
    return parts.to_object("Knight", armature)


# ========== The sword ==========


def build_sword_mesh(palette):
    """Built with the grip in the middle of the origin and the blade along +Z"""
    parts = PartBuilder(palette)

    guard_z = SWORD_GRIP_HALF_LENGTH
    blade_start = guard_z + 0.012
    blade_end = blade_start + SWORD_BLADE_LENGTH

    add_blade(parts, blade_start, blade_end)

    # Crossguard, slightly bent towards the blade, with rounded tips
    for side in (-1, 1):
        parts.box(None, "guard", (side * SWORD_GUARD_HALF_WIDTH * 0.5, 0.0, guard_z + 0.004),
                  (SWORD_GUARD_HALF_WIDTH, 0.022, 0.022), rotation=(0, side * -8, 0))
        parts.sphere(None, "brass", (side * SWORD_GUARD_HALF_WIDTH, 0.0, guard_z + 0.012), (0.017, 0.017, 0.017), segments=12, rings=8)
    parts.box(None, "brass", (0.0, 0.0, guard_z + 0.004), (0.05, 0.03, 0.034))

    # Grip wrapped in leather, pommel
    parts.cylinder(None, "grip", make_matrix((0, 0, 0)), 0.016, 0.016, SWORD_GRIP_HALF_LENGTH * 2, segments=12)
    for index in range(5):
        z = -SWORD_GRIP_HALF_LENGTH + 0.02 + index * 0.04
        parts.cylinder(None, "wrap", make_matrix((0, 0, z), (12, 0, 0)), 0.018, 0.018, 0.012, segments=12)
    parts.sphere(None, "brass", (0, 0, -SWORD_GRIP_HALF_LENGTH - 0.022), (0.03, 0.03, 0.026), segments=16, rings=8)
    parts.cylinder(None, "guard", make_matrix((0, 0, -SWORD_GRIP_HALF_LENGTH - 0.002)), 0.02, 0.022, 0.012, segments=12)

    return parts.to_object("KnightSword")


def add_blade(parts, blade_start, blade_end):
    """A diamond cross section (edges along X, ridge along Y) that narrows into a point"""
    blade_mesh = parts.mesh
    uv_blade = parts.palette.uv("blade")
    uv_edge = parts.palette.uv("blade_edge")
    uv_fuller = parts.palette.uv("fuller")

    point_start = blade_end - 0.14
    sections = [
        (blade_start, 1.0, 1.0),
        (blade_start + 0.25, 0.96, 1.0),
        (point_start, 0.82, 0.95),
        (blade_end - 0.05, 0.38, 0.7),
    ]

    rings = []
    for z, width_factor, thickness_factor in sections:
        width = SWORD_BLADE_HALF_WIDTH * width_factor
        thickness = SWORD_BLADE_HALF_THICKNESS * thickness_factor
        rings.append([blade_mesh.verts.new(point) for point in
                      ((width, 0, z), (0, thickness, z), (-width, 0, z), (0, -thickness, z))])
    tip = blade_mesh.verts.new((0, 0, blade_end))

    faces = []
    for lower, upper in zip(rings, rings[1:]):
        for corner in range(4):
            next_corner = (corner + 1) % 4
            faces.append(blade_mesh.faces.new((lower[corner], lower[next_corner], upper[next_corner], upper[corner])))
    for corner in range(4):
        faces.append(blade_mesh.faces.new((rings[-1][corner], rings[-1][(corner + 1) % 4], tip)))
    faces.append(blade_mesh.faces.new(list(reversed(rings[0]))))

    for face in faces:
        center = face.calc_center_median()
        uv = uv_edge if center.z > point_start else uv_blade
        for loop in face.loops:
            loop[parts.uv_layer].uv = uv

    # A darker groove (fuller) along both flats of the blade
    for side in (-1, 1):
        parts.box(None, "fuller", (0.0, side * SWORD_BLADE_HALF_THICKNESS * 0.62, (blade_start + point_start) * 0.5 - 0.03),
                  (SWORD_BLADE_HALF_WIDTH * 0.45, 0.002, point_start - blade_start - 0.12))
    del uv_fuller


# ========== Baked textures ==========


class SurfaceDetailMaterial:
    """The material the textures are baked from: the palette color made to look like used armor
    (fine grain, large stains, dark crevices, bright worn edges on metal)"""

    def __init__(self, name, palette_diffuse, palette_specular):
        self.material = bpy.data.materials.new(name)
        self.material.use_nodes = True
        self.nodes = self.material.node_tree.nodes
        self.links = self.material.node_tree.links
        self.nodes.clear()

        palette_uv = self._node("ShaderNodeUVMap", uv_map=PALETTE_UV)
        diffuse = self._image(palette_diffuse, palette_uv)
        specular = self._image(palette_specular, palette_uv)

        object_coordinates = self._node("ShaderNodeTexCoord").outputs["Object"]
        fine_grain = self._noise(object_coordinates, scale=90.0, detail=8.0, low=0.72, high=1.18)
        stains = self._noise(object_coordinates, scale=7.0, detail=4.0, low=0.55, high=1.0, from_low=0.35, from_high=0.75)
        scratches = self._noise(object_coordinates, scale=160.0, detail=10.0, low=0.6, high=1.15)

        occlusion = self._node("ShaderNodeAmbientOcclusion", samples=16)
        occlusion.inputs["Distance"].default_value = 0.06
        darker_crevices = self._map_range(occlusion.outputs["AO"], 0.0, 1.0, 0.35, 1.0)

        pointiness = self._node("ShaderNodeNewGeometry").outputs["Pointiness"]
        edges = self._map_range(pointiness, 0.5, 0.57, 0.0, 0.22)
        metal = self._map_range(specular.outputs["Color"], METAL_SPECULAR - 0.1, METAL_SPECULAR + 0.1, 0.0, 1.0)
        worn_edges = self._math("MULTIPLY", edges, metal)

        color = self._multiply(diffuse.outputs["Color"], fine_grain)
        color = self._multiply(color, stains)
        color = self._multiply(color, darker_crevices)
        self.color = self._vector_math("ADD", color, worn_edges)

        shine = self._multiply(specular.outputs["Color"], scratches)
        self.specular = self._multiply(shine, darker_crevices)

        self.emission = self._node("ShaderNodeEmission")
        output = self._node("ShaderNodeOutputMaterial")
        self.links.new(self.emission.outputs["Emission"], output.inputs["Surface"])

        baked_uv = self._node("ShaderNodeUVMap", uv_map=BAKED_UV)
        self.target = self._node("ShaderNodeTexImage")
        self.links.new(baked_uv.outputs["UV"], self.target.inputs["Vector"])

    def bake(self, mesh_object, source, image):
        """Bakes one output (self.color or self.specular) into the image, through the unwrapped UVs"""
        self.links.new(source, self.emission.inputs["Color"])
        self.target.image = image
        self.nodes.active = self.target

        select_only([mesh_object])
        bpy.ops.object.bake(type="EMIT", margin=BAKE_MARGIN_PIXELS, use_clear=True)
        image.save()

    def _node(self, node_type, **properties):
        node = self.nodes.new(node_type)
        for name, value in properties.items():
            setattr(node, name, value)
        return node

    def _image(self, image, uv_node):
        node = self._node("ShaderNodeTexImage", image=image, interpolation="Closest")
        self.links.new(uv_node.outputs["UV"], node.inputs["Vector"])
        return node

    def _map_range(self, value, from_low, from_high, to_low, to_high):
        node = self._node("ShaderNodeMapRange")
        self.links.new(value, node.inputs["Value"])
        for name, number in (("From Min", from_low), ("From Max", from_high), ("To Min", to_low), ("To Max", to_high)):
            node.inputs[name].default_value = number
        return node.outputs["Result"]

    def _noise(self, coordinates, scale, detail, low, high, from_low=0.3, from_high=0.7):
        noise = self._node("ShaderNodeTexNoise")
        self.links.new(coordinates, noise.inputs["Vector"])
        noise.inputs["Scale"].default_value = scale
        noise.inputs["Detail"].default_value = detail
        return self._map_range(noise.outputs["Fac"], from_low, from_high, low, high)

    def _math(self, operation, first, second):
        node = self._node("ShaderNodeMath", operation=operation)
        self.links.new(first, node.inputs[0])
        self.links.new(second, node.inputs[1])
        return node.outputs["Value"]

    def _vector_math(self, operation, first, second):
        node = self._node("ShaderNodeVectorMath", operation=operation)
        self.links.new(first, node.inputs[0])
        self.links.new(second, node.inputs[1])
        return node.outputs["Vector"]

    def _multiply(self, first, second):
        return self._vector_math("MULTIPLY", first, second)


def unwrap(mesh_object):
    """Second UV map with every face on its own place of the texture (the palette UVs share a few pixels)"""
    baked_uv = mesh_object.data.uv_layers.new(name=BAKED_UV)
    mesh_object.data.uv_layers.active = baked_uv

    select_only([mesh_object])
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(UV_ANGLE_LIMIT_DEGREES), island_margin=UV_ISLAND_MARGIN)
    bpy.ops.object.mode_set(mode="OBJECT")


def new_texture(name, folder, pixels, is_color):
    image = bpy.data.images.new(name, pixels, pixels, alpha=False)
    image.colorspace_settings.name = "sRGB" if is_color else "Non-Color"
    image.filepath_raw = os.path.join(folder, name + ".png")
    image.file_format = "PNG"
    return image


def setup_bake_render():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = BAKE_SAMPLES


def bake_textures(mesh_object, palette, base_name, folder, pixels):
    """Writes <base_name>_diffuse.png and <base_name>_specular.png and gives the object the material the engine reads"""
    palette_diffuse, palette_specular = palette.make_images(base_name)
    detail = SurfaceDetailMaterial(base_name + "_bake", palette_diffuse, palette_specular)

    mesh_object.data.materials.clear()
    mesh_object.data.materials.append(detail.material)
    unwrap(mesh_object)

    diffuse = new_texture(base_name + "_diffuse", folder, pixels, is_color=True)
    specular = new_texture(base_name + "_specular", folder, pixels, is_color=False)
    detail.bake(mesh_object, detail.color, diffuse)
    detail.bake(mesh_object, detail.specular, specular)

    # The engine reads one UV map and one texture (the specular one is found by its name next to it)
    mesh_object.data.uv_layers.remove(mesh_object.data.uv_layers[PALETTE_UV])
    mesh_object.data.materials.clear()
    mesh_object.data.materials.append(make_textured_material(base_name, diffuse))


# ========== Animations ==========

# A pose is {bone: (x, y, z) degrees}, a rotation relative to the rest pose expressed in the axes of the knight
# (X = right, Y = forward, Z = up), so +X swings a hanging limb forward and bends the spine backward,
# +Z turns to the left and +Y tips the top of a bone to the right. 'Hips' may also have "location" (meters)
# Every bone inherits the turn of its parent, like in any skeleton

MIRRORED_BONE_NAMES = {}
for _name, *_ in SKELETON:
    if _name.endswith("_R"):
        MIRRORED_BONE_NAMES[_name] = _name[:-2] + "_L"
        MIRRORED_BONE_NAMES[_name[:-2] + "_L"] = _name


def mirror_pose(pose):
    """The same pose for the other side of the body (left becomes right)"""
    mirrored = {}
    for bone, value in pose.items():
        if bone == "location":
            mirrored[bone] = (-value[0], value[1], value[2])
            continue
        x, y, z = value
        mirrored[MIRRORED_BONE_NAMES.get(bone, bone)] = (x, -y, -z)
    return mirrored


def combine(*poses):
    """Adds poses together (angles and locations are summed per axis)"""
    result = {}
    for pose in poses:
        for bone, value in pose.items():
            previous = result.get(bone, (0.0, 0.0, 0.0))
            result[bone] = tuple(a + b for a, b in zip(previous, value))
    return result


# The ready stance the knight returns to: knees soft, sword held low in front
STANCE = {
    "location": (0.0, 0.0, -0.03),
    "Spine": (-4, 0, 0), "Chest": (-3, 0, 4), "Neck": (3, 0, -2), "Head": (4, 0, -2),
    "Thigh_R": (18, 0, -4), "Shin_R": (-30, 0, 0), "Foot_R": (12, 0, 4),
    "Thigh_L": (14, 0, 6), "Shin_L": (-26, 0, 0), "Foot_L": (12, 0, -6),
    "Shoulder_R": (0, 0, 0), "UpperArm_R": (14, -6, 0), "LowerArm_R": (38, 0, 0), "Hand_R": (-8, 0, 0),
    "Shoulder_L": (0, 0, 0), "UpperArm_L": (6, 8, 0), "LowerArm_L": (22, 0, 0), "Hand_L": (0, 0, 0),
}

UPPER_BODY_STANCE = {bone: value for bone, value in STANCE.items()
                     if bone not in ("location",) and not bone.startswith(("Thigh", "Shin", "Foot"))}


def idle_animation():
    breathe_in = combine(STANCE, {"location": (0, 0, -0.008), "Chest": (2, 0, 0), "Head": (-2, 0, 0),
                                  "UpperArm_L": (0, 2, 0), "UpperArm_R": (2, -1, 0)})
    return [(0, STANCE), (30, breathe_in), (60, STANCE)]


def walk_step(right_leg_forward):
    """Right foot touches the ground in front, the left one pushes off behind"""
    contact = combine(UPPER_BODY_STANCE, {
        "location": (0.0, 0.0, -0.045),
        "Hips": (0, 0, 6), "Chest": (0, 0, -9), "Spine": (-2, 0, 0),
        "Thigh_R": (28, 0, 0), "Shin_R": (-6, 0, 0), "Foot_R": (-12, 0, 0),
        "Thigh_L": (-20, 0, 0), "Shin_L": (-24, 0, 0), "Foot_L": (24, 0, 0),
        "UpperArm_L": (22, 0, 0), "LowerArm_L": (12, 0, 0),
        "UpperArm_R": (-12, 0, 0),
    })
    passing = combine(UPPER_BODY_STANCE, {
        "location": (0.0, 0.0, -0.005),
        "Hips": (0, 0, 1), "Chest": (0, 0, -2),
        "Thigh_R": (2, 0, 0), "Shin_R": (-8, 0, 0), "Foot_R": (6, 0, 0),
        "Thigh_L": (24, 0, 0), "Shin_L": (-58, 0, 0), "Foot_L": (20, 0, 0),
        "UpperArm_L": (4, 0, 0), "UpperArm_R": (-2, 0, 0),
    })
    if right_leg_forward:
        return contact, passing
    return mirror_pose(contact), mirror_pose(passing)


def walk_animation():
    contact_right, passing_right = walk_step(True)
    contact_left, passing_left = walk_step(False)
    return [(0, contact_right), (8, passing_right), (15, contact_left), (23, passing_left), (30, contact_right)]


def run_step(right_leg_forward):
    lean = {"Spine": (-8, 0, 0), "Chest": (-6, 0, 0), "Neck": (6, 0, 0), "Head": (6, 0, 0)}
    arms = {"LowerArm_R": (55, 0, 0), "LowerArm_L": (70, 0, 0), "Hand_R": (-45, 0, 0)}
    contact = combine(lean, arms, {
        "location": (0.0, 0.0, -0.07),
        "Hips": (0, 0, 9), "Chest": (0, 0, -16),
        "Thigh_R": (40, 0, 0), "Shin_R": (-22, 0, 0), "Foot_R": (-6, 0, 0),
        "Thigh_L": (-28, 0, 0), "Shin_L": (-40, 0, 0), "Foot_L": (28, 0, 0),
        "UpperArm_L": (40, 6, 0), "UpperArm_R": (-30, -6, 0),
    })
    flight = combine(lean, arms, {
        "location": (0.0, 0.0, 0.02),
        "Hips": (0, 0, 3), "Chest": (0, 0, -5),
        "Thigh_R": (12, 0, 0), "Shin_R": (-30, 0, 0), "Foot_R": (10, 0, 0),
        "Thigh_L": (30, 0, 0), "Shin_L": (-100, 0, 0), "Foot_L": (24, 0, 0),
        "UpperArm_L": (8, 6, 0), "UpperArm_R": (0, -6, 0),
    })
    if right_leg_forward:
        return contact, flight
    return mirror_pose(contact), mirror_pose(flight)


def run_animation():
    contact_right, flight_right = run_step(True)
    contact_left, flight_left = run_step(False)
    return [(0, contact_right), (5, flight_right), (10, contact_left), (15, flight_left), (20, contact_right)]


def tucked_body(hips_pitch, hips_drop, tuck=1.0):
    """The knight curled into a ball and turned forward by 'hips_pitch' degrees"""
    return {
        "location": (0.0, 0.0, -hips_drop),
        "Hips": (-hips_pitch, 0, 0),
        "Spine": (-35 * tuck, 0, 0), "Chest": (-30 * tuck, 0, 0), "Neck": (-25 * tuck, 0, 0), "Head": (-10 * tuck, 0, 0),
        "Thigh_R": (105 * tuck, 0, 0), "Shin_R": (-130 * tuck, 0, 0), "Foot_R": (30 * tuck, 0, 0),
        "Thigh_L": (100 * tuck, 0, 0), "Shin_L": (-125 * tuck, 0, 0), "Foot_L": (30 * tuck, 0, 0),
        "UpperArm_R": (60 * tuck, -10, 0), "LowerArm_R": (90 * tuck, 0, 0),
        "UpperArm_L": (70 * tuck, 15, 0), "LowerArm_L": (100 * tuck, 0, 0),
    }


def roll_animation():
    crouch = combine(STANCE, {"location": (0, 0, -0.16), "Spine": (-25, 0, 0), "Chest": (-10, 0, 0),
                              "Thigh_R": (45, 0, 0), "Shin_R": (-70, 0, 0), "Foot_R": (20, 0, 0),
                              "Thigh_L": (40, 0, 0), "Shin_L": (-65, 0, 0), "Foot_L": (20, 0, 0)})
    landing = combine(STANCE, {"location": (0, 0, -0.22), "Hips": (-20, 0, 0), "Spine": (-10, 0, 0),
                               "Thigh_R": (70, 0, 0), "Shin_R": (-95, 0, 0), "Foot_R": (10, 0, 0),
                               "Thigh_L": (55, 0, 0), "Shin_L": (-90, 0, 0), "Foot_L": (25, 0, 0),
                               "UpperArm_L": (20, 25, 0)})
    return [
        (0, STANCE),
        (3, crouch),
        (6, tucked_body(80, 0.36, 0.8)),
        (9, tucked_body(165, 0.50)),
        (12, tucked_body(250, 0.50)),
        (15, tucked_body(320, 0.34, 0.9)),
        (18, landing),
        (26, STANCE),
    ]


def backstep_animation():
    push = combine(STANCE, {"location": (0, 0, -0.08), "Spine": (-12, 0, 0), "Chest": (-6, 0, 0),
                            "Thigh_R": (30, 0, 0), "Shin_R": (-50, 0, 0), "Foot_R": (18, 0, 0),
                            "Thigh_L": (24, 0, 0), "Shin_L": (-48, 0, 0), "Foot_L": (22, 0, 0)})
    airborne = combine(STANCE, {"location": (0, 0, 0.02), "Spine": (-14, 0, 0), "Chest": (-4, 0, 0),
                                "Thigh_R": (30, 0, 0), "Shin_R": (-40, 0, 0), "Foot_R": (-10, 0, 0),
                                "Thigh_L": (10, 0, 0), "Shin_L": (-30, 0, 0), "Foot_L": (-10, 0, 0),
                                "UpperArm_L": (20, 20, 0), "UpperArm_R": (10, -15, 0)})
    landing = combine(STANCE, {"location": (0, 0, -0.10), "Spine": (-10, 0, 0),
                               "Thigh_R": (40, 0, 0), "Shin_R": (-60, 0, 0), "Foot_R": (20, 0, 0),
                               "Thigh_L": (30, 0, 0), "Shin_L": (-58, 0, 0), "Foot_L": (28, 0, 0)})
    return [(0, STANCE), (3, push), (7, airborne), (11, landing), (18, STANCE)]


def attack_pose(torso_turn, lean, arm, elbow, wrist, legs=None, drop=0.03, left_arm=(6, 14, 0)):
    """'torso_turn' > 0 turns the chest to the left, 'arm' is the rotation of the right upper arm (x, y, z)"""
    pose = {
        "location": (0.0, 0.0, -drop),
        "Hips": (0, 0, torso_turn * 0.3),
        "Spine": (lean * 0.5, 0, torso_turn * 0.35), "Chest": (lean * 0.5, 0, torso_turn * 0.35),
        "Neck": (-lean * 0.4, 0, -torso_turn * 0.4), "Head": (-lean * 0.3, 0, -torso_turn * 0.3),
        "UpperArm_R": arm, "LowerArm_R": (elbow, 0, 0), "Hand_R": wrist,
        "UpperArm_L": left_arm, "LowerArm_L": (40, 0, 0),
    }
    pose.update(legs if legs is not None else {
        "Thigh_R": (14, 0, -4), "Shin_R": (-28, 0, 0), "Foot_R": (14, 0, 4),
        "Thigh_L": (14, 0, 6), "Shin_L": (-28, 0, 0), "Foot_L": (14, 0, -6),
    })
    return pose


LUNGE_RIGHT = {"Thigh_R": (42, 0, 0), "Shin_R": (-50, 0, 0), "Foot_R": (8, 0, 0),
               "Thigh_L": (-14, 0, 6), "Shin_L": (-24, 0, 0), "Foot_L": (30, 0, -6)}

LUNGE_LEFT = {"Thigh_L": (40, 0, 0), "Shin_L": (-48, 0, 0), "Foot_L": (8, 0, 0),
              "Thigh_R": (-12, 0, -6), "Shin_R": (-24, 0, 0), "Foot_R": (28, 0, 6)}


def light_attack_1():
    """Diagonal slash from the right shoulder down to the left"""
    wind_up = attack_pose(-40, 6, (115, -45, -10), 70, (-15, 0, 0), drop=0.04)
    strike = attack_pose(38, -14, (45, 0, 55), 8, (-30, 0, 0), LUNGE_RIGHT, drop=0.09)
    follow = attack_pose(48, -12, (30, 10, 70), 12, (-25, 0, 0), LUNGE_RIGHT, drop=0.08)
    return [(0, STANCE), (8, wind_up), (12, strike), (16, follow), (28, STANCE)]


def light_attack_2():
    """Backhand slash from the left to the right"""
    wind_up = attack_pose(40, 4, (70, 10, 75), 75, (5, 0, 0), drop=0.04, left_arm=(10, 25, 0))
    strike = attack_pose(-42, -12, (80, -55, -25), 6, (-20, 0, -20), LUNGE_LEFT, drop=0.09)
    follow = attack_pose(-52, -10, (70, -70, -35), 10, (-15, 0, -20), LUNGE_LEFT, drop=0.08)
    return [(0, STANCE), (7, wind_up), (11, strike), (15, follow), (27, STANCE)]


def light_attack_3():
    """Finisher: the sword raised over the head and brought straight down"""
    wind_up = attack_pose(-10, 12, (165, -10, 0), 45, (10, 0, 0), drop=0.02, left_arm=(30, 30, 0))
    strike = attack_pose(5, -28, (60, -4, 8), 0, (-15, 0, 0), LUNGE_RIGHT, drop=0.14)
    follow = attack_pose(5, -30, (45, -4, 8), 4, (-20, 0, 0), LUNGE_RIGHT, drop=0.15)
    return [(0, STANCE), (11, wind_up), (15, strike), (20, follow), (34, STANCE)]


def heavy_attack():
    """Slow, wide swing: the knight twists far back, then cuts down through everything in front"""
    wind_up = attack_pose(-60, 15, (165, -45, -20), 95, (10, 0, 0), drop=0.06, left_arm=(40, 40, 0))
    hold = attack_pose(-66, 18, (170, -48, -22), 100, (12, 0, 0), drop=0.07, left_arm=(42, 42, 0))
    strike = attack_pose(45, -30, (50, -5, 60), 5, (-30, 0, 0), LUNGE_RIGHT, drop=0.16)
    follow = attack_pose(55, -32, (30, 5, 75), 10, (-30, 0, 0), LUNGE_RIGHT, drop=0.17)
    return [(0, STANCE), (14, wind_up), (20, hold), (25, strike), (30, follow), (46, STANCE)]


def hit_animation():
    recoil = combine(STANCE, {"location": (0, 0, -0.06), "Spine": (14, 0, 6), "Chest": (10, 0, 4), "Head": (16, 0, -6),
                              "UpperArm_L": (-10, 30, 0), "UpperArm_R": (-6, -25, 0), "LowerArm_R": (-10, 0, 0),
                              "Thigh_R": (8, 0, 0), "Thigh_L": (-8, 0, 0)})
    return [(0, STANCE), (3, recoil), (8, combine(recoil, {"Spine": (-6, 0, 0)})), (18, STANCE)]


def death_animation():
    knees = combine(STANCE, {"location": (0, 0, -0.42), "Spine": (-20, 0, 0), "Chest": (-15, 0, 0), "Head": (-20, 0, 0),
                             "Thigh_R": (85, 0, 0), "Shin_R": (-150, 0, 0), "Foot_R": (65, 0, 0),
                             "Thigh_L": (80, 0, 0), "Shin_L": (-145, 0, 0), "Foot_L": (65, 0, 0),
                             "UpperArm_R": (10, -10, 0), "LowerArm_R": (10, 0, 0),
                             "UpperArm_L": (5, 10, 0), "LowerArm_L": (10, 0, 0)})
    sway = combine(knees, {"Spine": (8, 0, 0), "Head": (14, 0, 0)})
    falling = combine(knees, {"location": (0, 0.05, -0.55), "Hips": (-50, 0, 0), "Thigh_R": (-40, 0, 0), "Thigh_L": (-40, 0, 0),
                              "UpperArm_R": (30, -20, 0), "UpperArm_L": (30, 20, 0)})
    lying = {"location": (0, 0.15, -0.86), "Hips": (-88, 0, 0), "Spine": (-2, 0, 0), "Chest": (0, 0, 0), "Head": (10, 0, 25),
             "Thigh_R": (6, 0, 0), "Shin_R": (-4, 0, 0), "Foot_R": (-50, 0, 0),
             "Thigh_L": (8, 0, 8), "Shin_L": (-12, 0, 0), "Foot_L": (-40, 0, 0),
             "UpperArm_R": (110, -40, 0), "LowerArm_R": (20, 0, 0),
             "UpperArm_L": (160, 40, 0), "LowerArm_L": (30, 0, 0)}
    return [(0, STANCE), (10, knees), (22, sway), (32, falling), (40, lying), (60, lying)]


ANIMATIONS = {
    "Idle": idle_animation,
    "Walk": walk_animation,
    "Run": run_animation,
    "Roll": roll_animation,
    "Backstep": backstep_animation,
    "Attack_Light_1": light_attack_1,
    "Attack_Light_2": light_attack_2,
    "Attack_Light_3": light_attack_3,
    "Attack_Heavy": heavy_attack,
    "Hit": hit_animation,
    "Death": death_animation,
}


def get_rest_rotation(armature, bone_name):
    return armature.data.bones[bone_name].matrix_local.to_quaternion()


def keyframe_pose(armature, pose, frame, previous_rotations):
    """Puts every bone into the pose and stores a key. Bones missing from the pose go back to the rest pose"""
    for pose_bone in armature.pose.bones:
        name = pose_bone.name
        rest = get_rest_rotation(armature, name)

        angles = pose.get(name, (0.0, 0.0, 0.0))
        rotation_in_knight_axes = degrees_to_quaternion(angles)
        local_rotation = rest.inverted() @ rotation_in_knight_axes @ rest

        # Keeps neighbouring keys on the same side of the quaternion sphere, so a full roll turns the short way each step
        if name in previous_rotations:
            local_rotation.make_compatible(previous_rotations[name])
        previous_rotations[name] = local_rotation.copy()

        pose_bone.rotation_mode = "QUATERNION"
        pose_bone.rotation_quaternion = local_rotation
        pose_bone.keyframe_insert("rotation_quaternion", frame=frame)

        if name == "Hips":
            location = Vector(pose.get("location", (0.0, 0.0, 0.0)))
            pose_bone.location = rest.inverted() @ location
            pose_bone.keyframe_insert("location", frame=frame)


def create_animations(armature):
    armature.animation_data_create()
    actions = []

    for name, make_keys in ANIMATIONS.items():
        action = bpy.data.actions.new(name)
        action.use_fake_user = True
        armature.animation_data.action = action

        previous_rotations = {}
        keys = make_keys()
        for frame, pose in keys:
            keyframe_pose(armature, pose, frame, previous_rotations)

        action.frame_range = (keys[0][0], keys[-1][0])
        actions.append(action)

    armature.animation_data.action = actions[0]
    return actions


# ========== Export ==========


def select_only(objects):
    bpy.ops.object.select_all(action="DESELECT")
    for scene_object in objects:
        scene_object.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]


def export_gltf(path, objects, with_animations):
    select_only(objects)
    bpy.ops.export_scene.gltf(
        filepath=path,
        export_format="GLTF_SEPARATE",
        use_selection=True,
        export_yup=True,
        export_apply=False,
        export_texcoords=True,
        export_normals=True,
        export_materials="EXPORT",
        export_image_format="AUTO",
        export_skins=with_animations,
        export_animations=with_animations,
        export_animation_mode="ACTIONS",
        export_force_sampling=True,
        export_frame_step=1,
        export_def_bones=False,
        export_current_frame=False,
    )


# ========== Preview pictures ==========

ICON_PIXELS = 256
ICON_CAMERA_ANGLE = 30
ICON_CAMERA_DISTANCE = 1.0


def setup_preview_render(width=480, height=600, is_transparent=False):
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "TEXTURE"
    scene.display.shading.show_shadows = False
    scene.display.shading.show_object_outline = is_transparent
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.film_transparent = is_transparent
    world = bpy.data.worlds.new("PreviewWorld")
    world.color = (0.35, 0.37, 0.40)
    scene.world = world

    camera_data = bpy.data.cameras.new("PreviewCamera")
    camera_data.lens = 50
    camera = bpy.data.objects.new("PreviewCamera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    return camera


def place_preview_camera(camera, angle_degrees, distance=3.4, height=1.1, target_height=0.85):
    angle = math.radians(angle_degrees)
    camera.location = (math.sin(angle) * distance, math.cos(angle) * distance, height)
    direction = Vector((0, 0, target_height)) - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def reset_to_rest_pose(armature):
    armature.animation_data.action = None
    for pose_bone in armature.pose.bones:
        pose_bone.rotation_quaternion = Quaternion()
        pose_bone.location = Vector()
    bpy.context.view_layer.update()


def render_icon(path, armature):
    """The toolbar icon of the editor: the helmet and the shoulders, on a transparent background"""
    reset_to_rest_pose(armature)
    camera = setup_preview_render(ICON_PIXELS, ICON_PIXELS, is_transparent=True)
    place_preview_camera(camera, ICON_CAMERA_ANGLE, distance=ICON_CAMERA_DISTANCE, height=1.62, target_height=1.56)

    scene = bpy.context.scene
    scene.render.filepath = os.path.abspath(path)
    bpy.ops.render.render(write_still=True)


def render_previews(folder, armature, actions):
    os.makedirs(folder, exist_ok=True)
    camera = setup_preview_render()
    scene = bpy.context.scene

    reset_to_rest_pose(armature)

    for label, angle in (("front", 0), ("side", 90), ("back", 180), ("three_quarter", 35)):
        place_preview_camera(camera, angle)
        scene.render.filepath = os.path.join(folder, "rest_" + label + ".png")
        bpy.ops.render.render(write_still=True)

    place_preview_camera(camera, 60)
    for action in actions:
        armature.animation_data.action = action
        start, end = (int(frame) for frame in action.frame_range)
        for step in range(4):
            frame = start + round((end - start) * step / 3)
            scene.frame_set(frame)
            scene.render.filepath = os.path.join(folder, "{}_{:02d}.png".format(action.name, frame))
            bpy.ops.render.render(write_still=True)


# ========== Main ==========


def main():
    arguments = parse_arguments()
    out_folder = os.path.abspath(arguments.out)
    knight_folder = os.path.join(out_folder, "Knight")
    sword_folder = os.path.join(out_folder, "KnightSword")
    os.makedirs(knight_folder, exist_ok=True)
    os.makedirs(sword_folder, exist_ok=True)

    clear_scene()

    knight_palette = Palette(KNIGHT_COLORS)
    sword_palette = Palette(SWORD_COLORS)

    armature = build_skeleton()
    knight = build_knight_mesh(armature, knight_palette)
    actions = create_animations(armature)
    sword = build_sword_mesh(sword_palette)

    # Baked in the rest pose: the armor must not be bent by an animation while its texture is made
    reset_to_rest_pose(armature)
    setup_bake_render()
    bake_textures(knight, knight_palette, "knight", knight_folder, KNIGHT_TEXTURE_PIXELS)
    bake_textures(sword, sword_palette, "knight_sword", sword_folder, SWORD_TEXTURE_PIXELS)

    export_gltf(os.path.join(knight_folder, "knight.gltf"), [armature, knight], with_animations=True)
    export_gltf(os.path.join(sword_folder, "knight_sword.gltf"), [sword], with_animations=False)

    # For the pictures (and the .blend) the sword hangs on the weapon bone, exactly as the engine attaches it
    reset_to_rest_pose(armature)
    sword.parent = armature
    sword.parent_type = "BONE"
    sword.parent_bone = WEAPON_BONE
    sword.matrix_world = get_sword_rest_matrix()

    if arguments.icon:
        render_icon(arguments.icon, armature)

    if arguments.blend:
        bpy.ops.wm.save_as_mainfile(filepath=os.path.abspath(arguments.blend))

    if arguments.preview:
        render_previews(os.path.abspath(arguments.preview), armature, actions)

    print("KNIGHT_GENERATOR_DONE")


if __name__ == "__main__":
    main()
