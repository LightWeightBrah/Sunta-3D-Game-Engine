"""Turns Meshy models that are only scenery (they never move: the chained titan on the horizon) into the engine's glTF
with their color texture, standing on the ground, 'height' meters tall
    blender --background --factory-startup --python Projects/GraveCrown/Tools/scenery_models.py
"""
import os

import bpy
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
GENERATED = os.path.normpath(os.path.join(HERE, "..", "..", "..", "Tools", "Blender", "Generated", "GraveCrown"))
MODELS = os.path.normpath(os.path.join(HERE, "..", "res", "Models", "Scenery"))

# source glb: (engine name, height in meters)
SCENERY = {"titan.glb": ("chained_titan", 90.0)}


def keep_only_color(path):
    """Only the color texture stays (the engine lights with it and its own specular)"""
    import json
    with open(path, encoding="utf-8") as source:
        gltf = json.load(source)
    used = set()
    for material in gltf.get("materials", []):
        material.pop("normalTexture", None)
        material.get("pbrMetallicRoughness", {}).pop("metallicRoughnessTexture", None)
        if "baseColorTexture" in material.get("pbrMetallicRoughness", {}):
            used.add(gltf["textures"][material["pbrMetallicRoughness"]["baseColorTexture"]["index"]]["source"])
    for index, image in enumerate(gltf.get("images", [])):
        file = os.path.join(os.path.dirname(path), image["uri"])
        if index not in used and os.path.exists(file):
            os.remove(file)
    with open(path, "w", encoding="utf-8") as target:
        json.dump(gltf, target, indent=1)


def convert(source, name, height):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=os.path.join(GENERATED, source))
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    corners = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
    low = min(c.z for c in corners)
    scale = height / (max(c.z for c in corners) - low)
    for root in [o for o in bpy.context.scene.objects if o.parent is None]:
        root.scale = root.scale * scale
        root.location = (root.location - Vector((0.0, 0.0, low))) * scale
    # the size goes into the mesh itself (the engine doesn't take a scale from the glTF's nodes)
    bpy.ops.object.select_all(action="SELECT")
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.parent_clear(type="CLEAR_KEEP_TRANSFORM")
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    folder = os.path.join(MODELS, name)
    os.makedirs(folder, exist_ok=True)
    bpy.ops.export_scene.gltf(filepath=os.path.join(folder, name + ".gltf"), export_format="GLTF_SEPARATE",
                              export_yup=True, export_apply=True, export_animations=False, export_skins=False,
                              export_image_format="JPEG")
    keep_only_color(os.path.join(folder, name + ".gltf"))
    print("SCENERY", name, round(height, 1), "m")


for glb, (engine_name, meters) in SCENERY.items():
    convert(glb, engine_name, meters)
