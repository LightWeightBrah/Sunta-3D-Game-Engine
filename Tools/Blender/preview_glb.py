"""Renders a front and a three-quarter view of a .glb side by side: blender -b --factory-startup -P preview_glb.py -- in.glb out.png"""
import math
import sys

import bpy
from mathutils import Vector

source, target = sys.argv[sys.argv.index("--") + 1:][:2]

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=source)

meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
corners = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
low = Vector((min(c.x for c in corners), min(c.y for c in corners), min(c.z for c in corners)))
high = Vector((max(c.x for c in corners), max(c.y for c in corners), max(c.z for c in corners)))
center, height = (low + high) / 2, (high - low).z
print("SIZE", tuple(round(v, 3) for v in (high - low)), "FACES", sum(len(o.data.polygons) for o in meshes))

scene = bpy.context.scene
scene.render.engine = "BLENDER_WORKBENCH"
scene.display.shading.light = "FLAT"
scene.view_settings.view_transform = "Standard"
scene.world = bpy.data.worlds.new("World")
scene.world.color = (0.35, 0.35, 0.37)
scene.display.shading.color_type = "TEXTURE"
scene.render.resolution_x, scene.render.resolution_y = 700, 1000
scene.render.film_transparent = False

camera = bpy.data.objects.new("Camera", bpy.data.cameras.new("Camera"))
scene.collection.objects.link(camera)
scene.camera = camera
camera.data.type = "ORTHO"
camera.data.ortho_scale = height * 1.12

for name, angle in (("front", 0.0), ("side", 55.0)):
    direction = Vector((math.sin(math.radians(angle)), -math.cos(math.radians(angle)), 0.0))
    camera.location = center + direction * height * 3
    camera.rotation_euler = (center - camera.location).to_track_quat("-Z", "Y").to_euler()
    scene.render.filepath = target.replace(".png", f"_{name}.png")
    bpy.ops.render.render(write_still=True)
