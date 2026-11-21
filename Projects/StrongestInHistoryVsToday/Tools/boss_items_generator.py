"""
The things of the boss's last form in 'Strongest in History Vs Today': his great sword and the hexagonal seal that
burns behind his back from his last stand on (the way the wheel of the film's summoned monster turns behind it)

Run (from the repository root):
    blender --background --factory-startup --python Projects/StrongestInHistoryVsToday/Tools/boss_items_generator.py
    (optional: -- --only sword|seal|diddy_sword|wheel --preview <folder>)

Writes res/Models/Props/BossSword/sih_boss_sword.gltf and res/Models/Props/HexSeal/sih_hex_seal.gltf (+ .bin and
textures: _diffuse, _specular, _emissive), and the player's answer to the boss's sword: his own sword of baby oil
(res/Models/Props/DiddySword/sih_diddy_sword.gltf, Meshy's model of the user's concept B, see
AIContext/scripts/meshy_diddy_sword.py)

Where they come from: the sword is Meshy's model (Multi-Image to 3D) of the user's concept
Tools/Blender/Concepts/StrongestInHistory/boss_sword_concept.webp (its front and side views); the seal is built here out
of simple shapes: a hexagonal frame of dark red stone with a thinner hexagon turned inside it and a spike at each corner,
cracked, the cracks glowing
"""

import argparse
import math
import os
import sys

import bmesh
import bpy
import numpy
from mathutils import Matrix, Vector

PROJECT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLENDER_FOLDER = os.path.join(os.path.dirname(os.path.dirname(PROJECT)), "Tools", "Blender")
sys.path.insert(0, BLENDER_FOLDER)

import boss_generator  # noqa: E402  (its nodes of glowing cracks)
import character_from_mesh  # noqa: E402
from sunta_blender import clear_scene, export_gltf  # noqa: E402

GENERATED = os.path.join(BLENDER_FOLDER, "Generated", "StrongestInHistory")
PROPS = os.path.join(PROJECT, "res", "Models", "Props")

# ========== The sword ==========

SWORD_SOURCE = os.path.join(GENERATED, "boss_sword.glb")
SWORD_LENGTH = 1.7           # meters from the pommel to the tip
# The grip is this part of the way from the crossguard to the pommel (the right fist; the left one goes above it)
GRIP_FROM_GUARD = 0.42
# Its blade glows with cracks of fire from this far above the crossguard (the hilt stays cold)
SWORD_EMBER = (1.0, 0.16, 0.03)


def load_single_mesh(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    imported = [obj for obj in bpy.data.objects if obj not in before]
    meshes = [obj for obj in imported if obj.type == "MESH"]
    for obj in meshes:
        obj.data.transform(obj.matrix_world)
        obj.parent = None
        obj.matrix_world = Matrix.Identity(4)
    for obj in imported:
        if obj not in meshes:
            bpy.data.objects.remove(obj)
    if len(meshes) > 1:
        bpy.ops.object.select_all(action="DESELECT")
        for obj in meshes:
            obj.select_set(True)
        bpy.context.view_layer.objects.active = meshes[0]
        bpy.ops.object.join()
    return meshes[0]


def make_sword(folder):
    """The sword with its grip at the origin and its blade going up along +Z (the engine's +Y), its edges along X"""
    sword = load_single_mesh(SWORD_SOURCE)
    points = numpy.array([tuple(vertex.co) for vertex in sword.data.vertices])
    # Meshy made it standing with its long side along Z, the pommel up: the widest slice of the upper half is the guard
    low, high = points[:, 2].min(), points[:, 2].max()
    slices = numpy.linspace(low, high, 61)
    widths = []
    for a, b in zip(slices[:-1], slices[1:]):
        inside = points[(points[:, 2] >= a) & (points[:, 2] < b)]
        widths.append(numpy.ptp(inside[:, 0]) if len(inside) else 0.0)
    widths = numpy.array(widths)
    upper = len(widths) // 2
    guard_index = upper + int(widths[upper:].argmax())
    guard = (slices[guard_index] + slices[guard_index + 1]) * 0.5
    grip = guard + (high - guard) * GRIP_FROM_GUARD
    # The blade was below: turned over (pommel down, tip up), the grip to the origin, scaled to its length
    scale = SWORD_LENGTH / (high - low)
    sword.data.transform(Matrix.Scale(scale, 4) @ Matrix.Rotation(math.pi, 4, "X") @ Matrix.Translation((0.0, 0.0, -grip)))
    sword.name = "BossSword"
    blade_from = (guard - grip) * -scale + 0.02     # (after turning over: the guard is above the grip)
    tip = (grip - low) * scale
    print("SWORD guard at {:.3f}, tip at {:.3f} above the grip".format(blade_from, tip))
    print("SWORD_HITBOX local_offset (0, {:.3f}, 0) half_extents (0.14, {:.3f}, 0.07)".format(
        (blade_from + tip) * 0.5, (tip - blade_from) * 0.5))

    character_from_mesh.make_engine_textures(sword, folder, "sih_boss_sword", 1.25, 1.2)

    def glowing_blade(tree, diffuse_image):
        cracks = boss_generator.ember_cracks(tree, diffuse_image)
        coordinates = tree.nodes.new("ShaderNodeTexCoord").outputs["Object"]
        along = tree.nodes.new("ShaderNodeSeparateXYZ")
        tree.links.new(coordinates, along.inputs["Vector"])
        blade = boss_generator.map_range(tree, along.outputs["Z"], blade_from + 0.05, blade_from + 0.25, 0.0, 1.0)
        # (the cracks as a strength: their color is the ember's; the blade's own crimson comes on top)
        strength = tree.nodes.new("ShaderNodeRGBToBW")
        tree.links.new(cracks, strength.inputs["Color"])
        glow = boss_generator.combine(tree, strength.outputs["Val"], blade)
        return boss_generator.tinted(tree, glow, SWORD_EMBER)

    character_from_mesh.bake_emission(sword, folder, "sih_boss_sword_emissive", glowing_blade, size=1024)
    return sword


# ========== The player's sword of baby oil ==========

DIDDY_SWORD_SOURCE = os.path.join(GENERATED, "diddy_sword.glb")
DIDDY_SWORD_LENGTH = 1.35    # meters: a long one-handed sword (at 1.0 it barely reached the boss)
# The fist holds the grip this part of the way from the guard to the end of the pommel
DIDDY_GRIP_FROM_GUARD = 0.45


def make_diddy_sword(folder):
    """The same as the boss's sword (grip at the origin, blade up along +Z, edges along X), whichever way Meshy stood
    it: the guard is the widest slice, the blade is the longer side of it"""
    sword = load_single_mesh(DIDDY_SWORD_SOURCE)
    points = numpy.array([tuple(vertex.co) for vertex in sword.data.vertices])
    # (the long side of the model is its Z or its Y: turned so it is Z)
    if numpy.ptp(points[:, 1]) > numpy.ptp(points[:, 2]):
        sword.data.transform(Matrix.Rotation(math.pi / 2, 4, "X"))
        points = numpy.array([tuple(vertex.co) for vertex in sword.data.vertices])
    low, high = points[:, 2].min(), points[:, 2].max()
    slices = numpy.linspace(low, high, 81)
    widths = []
    for a, b in zip(slices[:-1], slices[1:]):
        inside = points[(points[:, 2] >= a) & (points[:, 2] < b)]
        widths.append(numpy.ptp(inside[:, 0]) if len(inside) else 0.0)
    widths = numpy.array(widths)
    # (not at the very ends: a pommel ball or a tip can't be the guard)
    margin = len(widths) // 10
    guard_index = margin + int(widths[margin:-margin].argmax())
    guard = (slices[guard_index] + slices[guard_index + 1]) * 0.5
    tip_is_up = (high - guard) > (guard - low)
    pommel_end = low if tip_is_up else high
    grip = guard + (pommel_end - guard) * DIDDY_GRIP_FROM_GUARD
    scale = DIDDY_SWORD_LENGTH / (high - low)
    turn = Matrix.Identity(4) if tip_is_up else Matrix.Rotation(math.pi, 4, "X")
    sword.data.transform(Matrix.Scale(scale, 4) @ turn @ Matrix.Translation((0.0, 0.0, -grip)))
    sword.name = "DiddySword"
    tip = abs((high if tip_is_up else low) - grip) * scale
    blade_from = abs(guard - grip) * scale + 0.02
    print("DIDDY_SWORD guard at {:.3f}, tip at {:.3f} above the grip".format(blade_from, tip))
    print("DIDDY_SWORD_HITBOX local_offset (0, {:.3f}, 0) half_extents (0.08, {:.3f}, 0.05)".format(
        (blade_from + tip) * 0.5, (tip - blade_from) * 0.5))
    # Glossy like the oil it is made of
    character_from_mesh.make_engine_textures(sword, folder, "sih_diddy_sword", 1.1, 1.15)
    # Frozen baby oil, clear as in the film (2:29, the bottle turns to crystal): the engine renders a material whose
    # name says crystal as clear crystal (Material::Data::crystal)
    sword.data.materials[0].name = "sih_diddy_sword_crystal"
    return sword


# ========== The wheel of the summon ==========

# The eight-spoked wheel that floats over the summon's head (the film's last frames, 2:39): it turns an eighth each
# time the summon adapts (summon_adaptation.lua). Lying flat, turning about its own up
WHEEL_RADIUS = 0.8           # meters from the hub to the middle of the rim
WHEEL_COLOR = (0.78, 0.6, 0.22)


def make_wheel(folder):
    parts = []

    def keep():
        parts.append(bpy.context.active_object)

    bpy.ops.mesh.primitive_torus_add(major_radius=WHEEL_RADIUS, minor_radius=0.05, major_segments=64, minor_segments=10)
    keep()
    bpy.ops.mesh.primitive_torus_add(major_radius=WHEEL_RADIUS * 0.28, minor_radius=0.035, major_segments=32, minor_segments=8)
    keep()
    bpy.ops.mesh.primitive_cylinder_add(radius=0.11, depth=0.1, vertices=24)
    keep()
    for spoke in range(8):
        angle = math.radians(45 * spoke)
        out = Vector((math.cos(angle), math.sin(angle), 0.0))
        length = WHEEL_RADIUS * 1.22
        bpy.ops.mesh.primitive_cylinder_add(radius=0.028, depth=length, vertices=10,
                                            location=out * length * 0.5,
                                            rotation=(0.0, math.pi / 2, angle))
        keep()
        # (a handle out past the rim, like a ship's wheel)
        bpy.ops.mesh.primitive_uv_sphere_add(radius=0.065, segments=14, ring_count=8, location=out * (length + 0.03))
        keep()

    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    wheel = bpy.context.active_object
    wheel.name = "SummonWheel"
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.shade_smooth()

    size = 64
    pixels = numpy.ones((size, size, 4), dtype=numpy.float32)
    pixels[..., :3] = WHEEL_COLOR
    image = bpy.data.images.new("wheel_gold", size, size)
    image.pixels = pixels.ravel()
    material = bpy.data.materials.new("SummonWheelGold")
    material.use_nodes = True
    texture = material.node_tree.nodes.new("ShaderNodeTexImage")
    texture.image = image
    shader = material.node_tree.nodes["Principled BSDF"]
    material.node_tree.links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    shader.inputs["Roughness"].default_value = 0.25
    wheel.data.materials.append(material)
    character_from_mesh.make_engine_textures(wheel, folder, "sih_summon_wheel", 1.0, 1.6)
    return wheel


# ========== The seal ==========

SEAL_RADIUS = 1.0            # meters from the middle to a corner of the outer frame
SEAL_COLORS = ((0.07, 0.012, 0.01), (0.2, 0.03, 0.02))   # the darkest and the lightest of its stone
SEAL_EMBER = (1.0, 0.1, 0.02)


def hexagon_frame(mesh, outer, inner, depth, turn=0.0):
    """A flat hexagonal frame in the XZ plane (facing -Y, the way it is seen from behind the boss), 'depth' thick"""
    rings = []
    for radius in (outer, inner):
        for y in (-depth * 0.5, depth * 0.5):
            ring = []
            for corner in range(6):
                angle = math.radians(60 * corner + 30 + turn)
                ring.append(mesh.verts.new((radius * math.cos(angle), y, radius * math.sin(angle))))
            rings.append(ring)
    outer_front, outer_back, inner_front, inner_back = rings
    for corner in range(6):
        nxt = (corner + 1) % 6
        mesh.faces.new((outer_front[corner], outer_front[nxt], inner_front[nxt], inner_front[corner]))
        mesh.faces.new((outer_back[nxt], outer_back[corner], inner_back[corner], inner_back[nxt]))
        mesh.faces.new((outer_back[corner], outer_back[nxt], outer_front[nxt], outer_front[corner]))
        mesh.faces.new((inner_front[corner], inner_front[nxt], inner_back[nxt], inner_back[corner]))


def spike(mesh, base, direction, length, width, depth):
    """A four-sided spike from 'base' pointing along 'direction' (in the XZ plane)"""
    side = Vector((-direction.z, 0.0, direction.x)) * width
    thick = Vector((0.0, depth * 0.5, 0.0))
    corners = [mesh.verts.new(base + side + thick), mesh.verts.new(base - side + thick),
               mesh.verts.new(base - side - thick), mesh.verts.new(base + side - thick)]
    point = mesh.verts.new(base + direction * length)
    for index in range(4):
        mesh.faces.new((corners[index], corners[(index + 1) % 4], point))
    mesh.faces.new(list(reversed(corners)))


def seal_texture(size=1024):
    """Dark red stone: smooth noise of two colors, darker veins"""
    rng = numpy.random.default_rng(7)
    noise = numpy.zeros((size, size), dtype=numpy.float32)
    for octave, weight in ((8, 0.5), (32, 0.3), (128, 0.2)):
        small = rng.random((octave, octave)).astype(numpy.float32)
        noise += weight * numpy.kron(small, numpy.ones((size // octave, size // octave), dtype=numpy.float32))
    noise = (noise - noise.min()) / (noise.max() - noise.min())
    dark, light = numpy.array(SEAL_COLORS[0]), numpy.array(SEAL_COLORS[1])
    pixels = numpy.ones((size, size, 4), dtype=numpy.float32)
    pixels[..., :3] = dark + (light - dark) * noise[..., None]
    image = bpy.data.images.new("seal_stone", size, size)
    image.pixels = pixels.ravel()
    return image


def make_seal(folder):
    data = bpy.data.meshes.new("HexSeal")
    seal = bpy.data.objects.new("HexSeal", data)
    bpy.context.scene.collection.objects.link(seal)
    mesh = bmesh.new()
    hexagon_frame(mesh, SEAL_RADIUS, SEAL_RADIUS * 0.8, 0.14)
    hexagon_frame(mesh, SEAL_RADIUS * 0.66, SEAL_RADIUS * 0.58, 0.08, turn=30.0)
    for corner in range(6):
        angle = math.radians(60 * corner + 30)
        out = Vector((math.cos(angle), 0.0, math.sin(angle)))
        spike(mesh, out * SEAL_RADIUS * 0.96, out, SEAL_RADIUS * 0.42, 0.09, 0.1)
        # and a small one inward from each corner of the inner hexagon
        inner_angle = math.radians(60 * corner)
        inward = Vector((math.cos(inner_angle), 0.0, math.sin(inner_angle)))
        spike(mesh, inward * SEAL_RADIUS * 0.6, -inward, SEAL_RADIUS * 0.22, 0.05, 0.06)
    mesh.to_mesh(data)
    mesh.free()

    bpy.context.view_layer.objects.active = seal
    seal.select_set(True)
    bevel = seal.modifiers.new("Bevel", "BEVEL")
    bevel.width = 0.015
    bevel.segments = 2
    bpy.ops.object.modifier_apply(modifier="Bevel")
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")

    material = bpy.data.materials.new("HexSealStone")
    material.use_nodes = True
    texture = material.node_tree.nodes.new("ShaderNodeTexImage")
    texture.image = seal_texture()
    shader = material.node_tree.nodes["Principled BSDF"]
    material.node_tree.links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    shader.inputs["Roughness"].default_value = 0.55
    data.materials.append(material)

    character_from_mesh.make_engine_textures(seal, folder, "sih_hex_seal", 1.0, 1.0)

    def burning_cracks(tree, diffuse_image):
        coordinates = tree.nodes.new("ShaderNodeTexCoord").outputs["Object"]
        cells = boss_generator.add_node(tree, "ShaderNodeTexVoronoi", feature="DISTANCE_TO_EDGE", Scale=9.0)
        tree.links.new(coordinates, cells.inputs["Vector"])
        cracks = boss_generator.map_range(tree, cells.outputs["Distance"], 0.0, 0.05, 1.0, 0.0)
        # the inner edge of the frame burns all along (the light comes from the middle of the seal)
        distance = boss_generator.add_node(tree, "ShaderNodeVectorMath", operation="LENGTH")
        tree.links.new(coordinates, distance.inputs[0])
        rim = boss_generator.map_range(tree, distance.outputs["Value"], SEAL_RADIUS * 0.8, SEAL_RADIUS * 0.86, 1.0, 0.0)
        return boss_generator.tinted(tree, boss_generator.combine(tree, cracks, rim, operation="ADD"), SEAL_EMBER)

    character_from_mesh.bake_emission(seal, folder, "sih_hex_seal_emissive", burning_cracks, size=1024)
    return seal


def render_preview(folder, obj, name):
    from sunta_blender import place_preview_camera, setup_preview_render, show_only
    os.makedirs(folder, exist_ok=True)
    show_only([obj])
    camera = setup_preview_render(400, 500)
    for angle in (0, 60):
        place_preview_camera(camera, angle, distance=3.2, height=0.4, target_height=0.4)
        bpy.context.scene.render.filepath = os.path.join(folder, "{}_{}.png".format(name, angle))
        bpy.ops.render.render(write_still=True)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--only", default="")
    parser.add_argument("--preview", default="")
    arguments = parser.parse_args(argv)

    for name, folder_name, model_name, make in (("sword", "BossSword", "sih_boss_sword", make_sword),
                                                ("seal", "HexSeal", "sih_hex_seal", make_seal),
                                                ("diddy_sword", "DiddySword", "sih_diddy_sword", make_diddy_sword),
                                                ("wheel", "SummonWheel", "sih_summon_wheel", make_wheel)):
        if arguments.only and arguments.only != name:
            continue
        clear_scene()
        folder = os.path.join(PROPS, folder_name)
        os.makedirs(folder, exist_ok=True)
        obj = make(folder)
        export_gltf(os.path.join(folder, model_name + ".gltf"), [obj], with_animations=False)
        if arguments.preview:
            render_preview(os.path.abspath(arguments.preview), obj, name)
    print("BOSS_ITEMS_DONE")


if __name__ == "__main__":
    main()
