"""
Generates the player's champagne bottle (his healing flask: he drinks from it, see the Healing Flask's
'shown_while_drinking'): dark green glass, a gold foil over the neck, a cream label.

Run (from the repository root):
    blender --background --factory-startup --python Projects/StrongestInHistoryVsToday/Tools/champagne_generator.py

Writes res/Models/Props/Champagne/sih_champagne.gltf (+ .bin, textures). 0.32 m tall, its bottom at the origin,
standing along +Y (the engine's up).
"""

import math
import os
import sys

import bpy

PROJECT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(PROJECT)), "Tools", "Blender"))

from sunta_blender import clear_scene, export_gltf  # noqa: E402

OUT_FOLDER = os.path.join(PROJECT, "res", "Models", "Props", "Champagne")
NAME = "sih_champagne"

# (radius, height) of the outline, bottom to top: the body, the shoulder, the neck, the lip
PROFILE = [(0.0, 0.0), (0.042, 0.0), (0.045, 0.01), (0.045, 0.19), (0.04, 0.215), (0.022, 0.245),
           (0.0145, 0.27), (0.0145, 0.31), (0.017, 0.312), (0.017, 0.32), (0.0, 0.32)]
COLORS = {"glass": (0.06, 0.2, 0.1, 1.0), "foil": (0.85, 0.66, 0.22, 1.0), "label": (0.93, 0.88, 0.74, 1.0)}


def material(name, color):
    image = bpy.data.images.new(f"{NAME}_{name}_diffuse", 4, 4)
    image.pixels = list(color) * 16
    image.filepath_raw = os.path.join(OUT_FOLDER, f"{NAME}_{name}_diffuse.png")
    image.file_format = "PNG"
    image.save()
    mat = bpy.data.materials.new(f"{NAME}_{name}")
    mat.use_nodes = True
    texture = mat.node_tree.nodes.new("ShaderNodeTexImage")
    texture.image = image
    mat.node_tree.links.new(texture.outputs["Color"], mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"])
    return mat


def lathe(name, profile, mat, segments=24):
    verts, faces = [], []
    for radius, height in profile:
        for i in range(segments):
            a = math.tau * i / segments
            verts.append((radius * math.cos(a), radius * math.sin(a), height))
    for r in range(len(profile) - 1):
        for i in range(segments):
            j = (i + 1) % segments
            faces.append((r * segments + i, r * segments + j, (r + 1) * segments + j, (r + 1) * segments + i))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    # (UVs: a model without them doesn't load in the engine)
    layer = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        x, y, z = mesh.vertices[loop.vertex_index].co
        layer.data[loop.index].uv = (math.atan2(y, x) / math.tau + 0.5, z / 0.32)
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    mesh.materials.append(mat)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def main():
    os.makedirs(OUT_FOLDER, exist_ok=True)
    clear_scene()
    glass = lathe(NAME, PROFILE, material("glass", COLORS["glass"]))
    # the foil over the neck and the label round the body: slightly bigger shells of the outline
    foil = lathe(NAME + "_foil", [(0.0, 0.25), (0.02, 0.25), (0.0165, 0.27), (0.0165, 0.322), (0.0, 0.322)],
                 material("foil", COLORS["foil"]))
    label = lathe(NAME + "_label", [(0.0462, 0.07), (0.0462, 0.15)], material("label", COLORS["label"]))
    bpy.ops.object.select_all(action="DESELECT")
    for obj in (glass, foil, label):
        obj.select_set(True)
    bpy.context.view_layer.objects.active = glass
    bpy.ops.object.join()
    export_gltf(os.path.join(OUT_FOLDER, NAME + ".gltf"), [glass], with_animations=False)
    print("CHAMPAGNE_DONE")


if __name__ == "__main__":
    main()
