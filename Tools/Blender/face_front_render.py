"""
Renders the head of a rigged character straight from the front (orthographic, with its own texture) and a grid of
world coordinates over it, so the eyes and the mouth of the model can be read off the picture: those are the points a
photo of the face is fitted to (character_from_mesh.FacePhoto, 'model_points').

    blender --background --factory-startup --python Tools/Blender/face_front_render.py -- <rigged.glb> <height> <out.png>

Prints FACE_VIEW: the world x and z of the picture's left, right, top and bottom edges
"""

import os
import sys

import bpy
from mathutils import Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import character_from_mesh  # noqa: E402
import motion_retarget  # noqa: E402
from sunta_blender import clear_scene, show_only  # noqa: E402

PIXELS = 900
VIEW_METERS = 0.3


def main():
    source, height, out = sys.argv[sys.argv.index("--") + 1:][:3]
    clear_scene()
    our_names = {theirs: ours for ours, theirs in motion_retarget.MESHY.bone_of.items()}
    armature, body = character_from_mesh.load_rigged_character(source, float(height), our_names)
    show_only([body])

    points = [body.matrix_world @ vertex.co for vertex in body.data.vertices]
    top = max(point.z for point in points)
    center = Vector((0.0, 1.0, top - VIEW_METERS * 0.5))

    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "FLAT"
    scene.display.shading.color_type = "TEXTURE"
    scene.view_settings.view_transform = "Standard"
    scene.render.resolution_x = scene.render.resolution_y = PIXELS
    camera = bpy.data.objects.new("Camera", bpy.data.cameras.new("Camera"))
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = VIEW_METERS
    camera.location = center
    camera.rotation_euler = (Vector((0.0, -1.0, 0.0))).to_track_quat("-Z", "Y").to_euler()
    scene.render.filepath = os.path.abspath(out)
    bpy.ops.render.render(write_still=True)

    # Seen from +Y the world's +X is on the picture's left
    half = VIEW_METERS * 0.5
    print("FACE_VIEW left_x {:.4f} right_x {:.4f} top_z {:.4f} bottom_z {:.4f} pixels {}".format(
        center.x + half, center.x - half, center.z + half, center.z - half, PIXELS))


main()
