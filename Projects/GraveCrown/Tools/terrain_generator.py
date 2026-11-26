"""
The mesh of the ash plains' ground (terrain_map.py's heights, one vertex a meter) with a tiling ground texture
(ambientCG Ground037, CC0: Tools/Textures/ambientCG/Ground037), darkened and turned toward ash grey. The engine's Terrain
component reads the heightmap made from the same function, so what is seen is what is walked on

    blender --background --factory-startup --python Projects/GraveCrown/Tools/terrain_generator.py
Writes res/Models/Terrain/ash_plains.gltf (+ .bin, _diffuse.png, _specular.png)
"""

import os
import sys

import bpy
import numpy

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "Tools", "Blender"))

import terrain_map  # noqa: E402
from character_from_mesh import save_image  # noqa: E402
from sunta_blender import clear_scene, export_gltf, make_textured_material  # noqa: E402

OUT = os.path.join(HERE, "..", "res", "Models", "Terrain")
NAME = "ash_plains"
TEXTURE = os.path.join(ROOT, "Tools", "Textures", "ambientCG", "Ground037", "Ground037_1K-JPG_Color.jpg")
# Meters one repeat of the texture covers
TILE_METERS = 7.0
STEP = 1.0
# The ground is ash: the soil's colors pulled toward a dark violet grey
ASH_TINT = numpy.array([0.5, 0.48, 0.5])
ASH_MIX = 0.75
# and darker: ash and cinders, not sand
DARKEN = 0.62


def ash_texture(folder):
    image = bpy.data.images.load(TEXTURE)
    width, height = image.size
    pixels = numpy.array(image.pixels[:], dtype=numpy.float32).reshape(height, width, 4)
    grey = pixels[..., :3].mean(axis=2, keepdims=True)
    color = (pixels[..., :3] * (1.0 - ASH_MIX) + grey * ASH_TINT * ASH_MIX * 1.25) * DARKEN
    pixels[..., :3] = numpy.clip(color, 0.0, 1.0)
    diffuse = save_image(NAME + "_diffuse", folder, pixels, is_color=True)
    specular = numpy.ones_like(pixels)
    specular[..., :3] = 0.08
    save_image(NAME + "_specular", folder, specular, is_color=False)
    return diffuse


def build_mesh():
    xs = numpy.arange(terrain_map.CENTER_X - terrain_map.SIZE_X * 0.5, terrain_map.CENTER_X + terrain_map.SIZE_X * 0.5 + 0.01, STEP)
    zs = numpy.arange(terrain_map.CENTER_Z - terrain_map.SIZE_Z * 0.5, terrain_map.CENTER_Z + terrain_map.SIZE_Z * 0.5 + 0.01, STEP)
    vertices, uvs = [], []
    # Blender: X right, -Y is the engine's +Z, Z up (glTF export turns Y up)
    for z in zs:
        for x in xs:
            # (around the middle of the map: the entity of the terrain stands there, the engine samples from it)
            vertices.append((x - terrain_map.CENTER_X, -(z - terrain_map.CENTER_Z), terrain_map.height(x, z)))
    columns = len(xs)
    faces = []
    for r in range(len(zs) - 1):
        for c in range(columns - 1):
            a = r * columns + c
            faces.append((a, a + 1, a + columns + 1, a + columns))
    mesh = bpy.data.meshes.new(NAME)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    uv_layer = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        x, y, _ = vertices[loop.vertex_index]
        uv_layer.data[loop.index].uv = (x / TILE_METERS, y / TILE_METERS)  # (relative: the same tiling, shifted)
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    obj = bpy.data.objects.new(NAME, mesh)
    bpy.context.scene.collection.objects.link(obj)
    # Faces must look up (the winding above may face down)
    if mesh.polygons[0].normal.z < 0:
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.mesh.flip_normals()
        bpy.ops.object.mode_set(mode="OBJECT")
    print("TERRAIN {} vertices, {} faces".format(len(vertices), len(faces)))
    return obj


def main():
    clear_scene()
    folder = os.path.normpath(OUT)
    os.makedirs(folder, exist_ok=True)
    obj = build_mesh()
    obj.data.materials.append(make_textured_material(NAME, ash_texture(folder)))
    export_gltf(os.path.join(folder, NAME + ".gltf"), [obj], with_animations=False)
    print("TERRAIN_DONE")


if __name__ == "__main__":
    main()
