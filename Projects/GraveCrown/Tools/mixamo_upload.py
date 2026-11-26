"""
Exports a Meshy model of Grave Crown (unrigged, textured) as an FBX for Mixamo's Auto-Rigger: one mesh, the texture
embedded, standing on the ground and facing the front. Mixamo's rig skins bulky armour far better than Meshy's (the
king's shoulders collapsed whenever the arms rose), the same fix as Strongest in History Vs Today's player

    blender --background --factory-startup --python Projects/GraveCrown/Tools/mixamo_upload.py -- player.glb Tools/Blender/Mixamo/GraveKing/grave_king_upload.fbx
(the FBX is then dragged onto mixamo.com by hand: its page blocks scripted uploads)
"""
import os
import sys

import bpy

HERE = os.path.dirname(os.path.abspath(__file__))
GENERATED = os.path.normpath(os.path.join(HERE, "..", "..", "..", "Tools", "Blender", "Generated", "GraveCrown"))

source, target = sys.argv[sys.argv.index("--") + 1:][:2]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=os.path.join(GENERATED, source))
meshes = [o for o in bpy.data.objects if o.type == "MESH"]
for o in bpy.data.objects:
    o.select_set(o in meshes)
bpy.context.view_layer.objects.active = meshes[0]
if len(meshes) > 1:
    bpy.ops.object.join()
mesh = bpy.context.view_layer.objects.active
bpy.ops.object.parent_clear(type="CLEAR_KEEP_TRANSFORM")
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
# Feet on the ground, centred
low = min((mesh.matrix_world @ v.co).z for v in mesh.data.vertices)
xs = [(mesh.matrix_world @ v.co).x for v in mesh.data.vertices]
ys = [(mesh.matrix_world @ v.co).y for v in mesh.data.vertices]
mesh.location = (-(min(xs) + max(xs)) / 2, -(min(ys) + max(ys)) / 2, -low)
bpy.ops.object.transform_apply(location=True)
for o in list(bpy.data.objects):
    if o != mesh:
        bpy.data.objects.remove(o)
os.makedirs(os.path.dirname(os.path.abspath(target)), exist_ok=True)
bpy.ops.export_scene.fbx(filepath=os.path.abspath(target), use_selection=False, path_mode="COPY", embed_textures=True,
                         axis_forward="-Z", axis_up="Y")
print("MIXAMO_UPLOAD", target, len(mesh.data.vertices), "vertices")
