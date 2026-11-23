"""
Generates the wall of baby oil of the player's domain (Divine Baby Oil Evolve, film 2:24-2:26): a wave whose crest
curls over towards the enemy, its lip hanging over, tapering away at both ends. Two of them rise on both sides of the
enemy and crash together (player_techniques.lua moves and grows them). A sphere of liquid stretched into a wall read
as a lump or a lens; this is the shape of a breaking wave.

Run (from the repository root):
    blender --background --factory-startup --python Projects/StrongestInHistoryVsToday/Tools/oil_wave_generator.py

Writes res/Models/Props/OilWave/sih_oil_wave.gltf (+ .bin, _diffuse.png). Its size is 1 m every way (across, up,
along), the script scales it. Its material's name holds "crystal": the engine draws it as clear crystal (milky oil,
see Material::Data::crystal). Engine axes: +X is where it curls to, +Y up, Z along it; its foot on the ground (y 0)
"""

import math
import os
import random
import sys

import bpy
import bmesh

PROJECT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(PROJECT)), "Tools", "Blender"))

from sunta_blender import clear_scene, export_gltf  # noqa: E402

OUT_FOLDER = os.path.join(PROJECT, "res", "Models", "Props", "OilWave")
NAME = "sih_oil_wave"

# The cross-section of the wave (across, up), going round it: up its back, over the crest, out to the lip hanging
# over, under the lip and down its curling face to the foot (1 x 1, the foot from -0.5 to +0.3 across)
PROFILE = [
    (-0.50, 0.00), (-0.42, 0.22), (-0.30, 0.48), (-0.15, 0.72), (0.00, 0.88), (0.15, 0.98), (0.30, 1.00),
    (0.43, 0.93), (0.52, 0.80), (0.55, 0.68),   # the lip
    (0.47, 0.64), (0.36, 0.69), (0.25, 0.66), (0.17, 0.55), (0.13, 0.40), (0.16, 0.22), (0.24, 0.08), (0.30, 0.00),
]
SEGMENTS = 48        # along it
RANDOM_SEED = 7
MILK = (0.97, 0.95, 0.9, 1.0)


def taper(t):
    """How tall the wave stands at 't' along it (0..1): full in the middle, falling away at both ends"""
    edge = abs(t * 2.0 - 1.0)
    return max(0.0, 1.0 - edge ** 3)


def build_wave():
    generator = random.Random(RANDOM_SEED)
    mesh = bpy.data.meshes.new(NAME)
    obj = bpy.data.objects.new(NAME, mesh)
    bpy.context.scene.collection.objects.link(obj)

    bm = bmesh.new()
    uv = bm.loops.layers.uv.new()
    rings = []
    # (a slow ripple along the crest, so it isn't a perfect extrusion)
    phase = generator.uniform(0, math.tau)
    for segment in range(SEGMENTS + 1):
        t = segment / SEGMENTS
        along = t - 0.5
        height = max(taper(t), 0.04)
        ripple = 1.0 + 0.06 * math.sin(t * 9.0 + phase)
        ring = []
        for across, up in PROFILE:
            # (Blender: X across, Y along, Z up)
            ring.append(bm.verts.new((across * (0.6 + 0.4 * height), along, up * height * ripple)))
        rings.append(ring)

    count = len(PROFILE)
    for segment in range(SEGMENTS):
        for i in range(count - 1):
            a, b = rings[segment][i], rings[segment][i + 1]
            c, d = rings[segment + 1][i + 1], rings[segment + 1][i]
            face = bm.faces.new((a, b, c, d))
            for loop, (u, v) in zip(face.loops, ((segment, i), (segment, i + 1), (segment + 1, i + 1), (segment + 1, i))):
                loop[uv].uv = (u / SEGMENTS, v / (count - 1))

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    return obj


def give_material(obj, folder):
    """A milky material whose name holds 'crystal' (the engine's clear liquid look), with a plain milky texture"""
    image = bpy.data.images.new(NAME + "_diffuse", 8, 8)
    image.pixels = list(MILK) * 64
    image.filepath_raw = os.path.join(folder, NAME + "_diffuse.png")
    image.file_format = "PNG"
    image.save()

    material = bpy.data.materials.new(NAME + "_crystal")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    shader = nodes.get("Principled BSDF")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    material.node_tree.links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    obj.data.materials.append(material)


def main():
    os.makedirs(OUT_FOLDER, exist_ok=True)
    clear_scene()
    wave = build_wave()
    give_material(wave, OUT_FOLDER)
    export_gltf(os.path.join(OUT_FOLDER, NAME + ".gltf"), [wave], with_animations=False)
    print("OIL_WAVE_DONE", len(wave.data.vertices), "vertices")


if __name__ == "__main__":
    main()
