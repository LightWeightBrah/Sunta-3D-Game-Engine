"""
Generates the props of the boss arena of the example game (MyGame): a ruined throne, an iron brazier, a wall module with
a gothic arch, a headless kneeling knight statue, swords stuck in rubble and a ruined corner tower

Run (from the repository root):
    blender --background --factory-startup --python Tools/Blender/arena_props_generator.py -- --out Projects/MyGame/res/Models/Arena

Writes <out>/<Folder>/<name>.gltf (+ .bin, _diffuse.png, _specular.png). They are the game's models: a scene uses them
with "model_name": "<name>". Prints each prop's box collider (half extents and offset, in the engine's axes)

Where they come from (see Generated/meshy_log.json): the concepts Concepts/Arena/<name>.png (Meshy Text to Image,
approved by the user) turned into textured models by Meshy (Image to 3D): Generated/Arena/<name>.glb.
Each stands on the ground with its footprint centered at the origin, its front towards +Z in the engine (towards -Y here,
as Meshy made it from the front view). The big pieces are modules: the wall with the arch is 4 m wide, like the walls
of level_kit_generator.py, so it takes the place of any wall
"""

import argparse
import os
import sys
from dataclasses import dataclass

import bpy
from mathutils import Matrix

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import character_from_mesh  # noqa: E402
from sunta_blender import clear_scene, export_gltf  # noqa: E402

BLENDER_FOLDER = os.path.dirname(os.path.abspath(__file__))
SOURCE_FOLDER = os.path.join(BLENDER_FOLDER, "Generated", "Arena")

# A little brighter than generated: the engine has no reflections, dark stone would look flat
BRIGHTNESS = 1.4


@dataclass
class Prop:
    source: str       # in Generated/Arena
    folder: str
    name: str         # the files and the model in the engine
    size: float       # meters
    is_width: bool = False  # 'size' is the width (left to right), otherwise the height


PROPS = [
    Prop("throne.glb", "Throne", "arena_throne", 3.4),
    Prop("brazier.glb", "Brazier", "arena_brazier", 1.7),
    Prop("wall_arch.glb", "WallArch", "arena_wall_arch", 4.0, is_width=True),
    Prop("knight_statue.glb", "KnightStatue", "arena_knight_statue", 3.8),
    Prop("sword_graves.glb", "SwordGraves", "arena_sword_graves", 1.5),
    Prop("corner_tower.glb", "CornerTower", "arena_corner_tower", 9.0),
]


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--only", default="")
    return parser.parse_args(argv)


def load_prop(prop):
    """The model standing on the ground, its footprint centered at the origin, 'size' tall (or wide)"""
    objects_before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(SOURCE_FOLDER, prop.source))
    imported = [obj for obj in bpy.data.objects if obj not in objects_before]
    model = next(obj for obj in imported if obj.type == "MESH")
    world_matrix = model.matrix_world.copy()
    model.parent = None
    for obj in imported:
        if obj is not model:
            bpy.data.objects.remove(obj)
    model.data.transform(world_matrix)
    model.matrix_world = Matrix.Identity(4)

    points = [vertex.co for vertex in model.data.vertices]
    lowest = [min(p[axis] for p in points) for axis in range(3)]
    highest = [max(p[axis] for p in points) for axis in range(3)]
    measured = (highest[0] - lowest[0]) if prop.is_width else (highest[2] - lowest[2])
    scale = prop.size / measured
    middle_x = (lowest[0] + highest[0]) * 0.5
    middle_y = (lowest[1] + highest[1]) * 0.5
    model.data.transform(Matrix.Scale(scale, 4) @ Matrix.Translation((-middle_x, -middle_y, -lowest[2])))
    model.name = prop.name
    return model


def print_collider(prop, model):
    """A box around the whole prop, in the engine's axes (Y up, Z towards the front)"""
    points = [vertex.co for vertex in model.data.vertices]
    width = max(p.x for p in points) - min(p.x for p in points)
    depth = max(p.y for p in points) - min(p.y for p in points)
    height = max(p.z for p in points)
    print("PROP_COLLIDER {} half_extents ({:.2f}, {:.2f}, {:.2f}) local_offset (0, {:.2f}, 0)".format(
        prop.name, width * 0.5, height * 0.5, depth * 0.5, height * 0.5))


def main():
    arguments = parse_arguments()
    for prop in PROPS:
        if arguments.only and arguments.only != prop.name:
            continue
        clear_scene()
        folder = os.path.join(os.path.abspath(arguments.out), prop.folder)
        os.makedirs(folder, exist_ok=True)

        model = load_prop(prop)
        character_from_mesh.make_engine_textures(model, folder, prop.name, BRIGHTNESS)
        export_gltf(os.path.join(folder, prop.name + ".gltf"), [model], with_animations=False)
        print_collider(prop, model)
    print("ARENA_PROPS_DONE")


if __name__ == "__main__":
    main()
