"""
Generates the modular castle kit of the example game (MyGame): walls, a wall with a doorway, a ruined wall, a ruined corner,
a pillar, a floor slab and stairs, every piece made to snap together on a 4 m grid

Run (from the repository root):
    blender --background --factory-startup --python Tools/Blender/kit_generator.py -- --out Projects/MyGame/res/Models/Kit

Writes <out>/<Folder>/<name>.gltf (+ .bin, _diffuse.png, _specular.png) and prints each piece's box collider

How the pieces were made (the method of a kit, not one big model; see Generated/meshy_log.json):
 - every piece is drawn by itself, its front and its side view side by side on a dark background, with its real size in
   the description (Concepts/Kit/<name>.png, approved by the user), so the generator knows how thick it is
 - Meshy makes the model from both views (Multi Image to 3D); where a side view came out wrong, from the front view only
 - here every piece gets its EXACT size (a little stretched if needed): walls are 4 m wide, so two walls always meet,
   floors 8 m, and the corner of the footprint is where the grid expects it
Pieces stand on the ground (y = 0) with their footprint centered, their front towards +Z in the engine;
the floor slab's top is at y = 0
"""

import argparse
import math
import os
import sys
from dataclasses import dataclass

import bpy
from mathutils import Matrix

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import character_from_mesh  # noqa: E402
from sunta_blender import clear_scene, export_gltf, select_only  # noqa: E402

BLENDER_FOLDER = os.path.dirname(os.path.abspath(__file__))
SOURCE_FOLDER = os.path.join(BLENDER_FOLDER, "Generated", "Kit")

BRIGHTNESS = 1.4


@dataclass
class Piece:
    source: str            # in Generated/Kit
    folder: str
    name: str              # the files and the model in the engine
    width: float           # meters, left to right (X)
    height: float          # meters, up
    depth: float           # meters, front to back
    faces: int             # the mesh is simplified to about this many triangles (many pieces stand in a level)
    lay_flat: bool = False # made standing up (a floor drawn from above): turned to lie on the ground
    top_at_ground: bool = False


PIECES = [
    Piece("wall.glb", "Wall", "kit_wall", 4.0, 6.0, 1.0, 6000),
    Piece("wall_door.glb", "WallDoor", "kit_wall_door", 4.0, 6.0, 0.8, 7000),
    Piece("wall_ruined.glb", "WallRuined", "kit_wall_ruined", 4.0, 2.6, 1.0, 6000),
    Piece("ruined_corner.glb", "RuinedCorner", "kit_ruined_corner", 4.0, 5.5, 4.0, 7000),
    Piece("pillar.glb", "Pillar", "kit_pillar", 1.2, 7.0, 1.2, 5000),
    Piece("floor.glb", "Floor", "kit_floor", 8.0, 0.3, 8.0, 100000, lay_flat=True, top_at_ground=True),
    Piece("stairs.glb", "Stairs", "kit_stairs", 4.0, 2.0, 4.0, 6000),
]


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--only", default="")
    return parser.parse_args(argv)


def load_piece(piece):
    objects_before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(SOURCE_FOLDER, piece.source))
    imported = [obj for obj in bpy.data.objects if obj not in objects_before]
    model = next(obj for obj in imported if obj.type == "MESH")
    world_matrix = model.matrix_world.copy()
    model.parent = None
    for obj in imported:
        if obj is not model:
            bpy.data.objects.remove(obj)
    model.data.transform(world_matrix)
    model.matrix_world = Matrix.Identity(4)

    if piece.lay_flat:
        # Its face looked at -Y (the front view): now it looks up
        model.data.transform(Matrix.Rotation(math.radians(-90.0), 4, "X"))
    return model


def snap_to_size(model, piece):
    """Exactly 'width' x 'height' x 'depth' (Blender: X, Z, Y), footprint centered, standing on the ground"""
    points = [vertex.co for vertex in model.data.vertices]
    low = [min(p[axis] for p in points) for axis in range(3)]
    high = [max(p[axis] for p in points) for axis in range(3)]
    size = [high[axis] - low[axis] for axis in range(3)]
    center = [(low[axis] + high[axis]) * 0.5 for axis in range(3)]

    scale = Matrix.Diagonal((piece.width / size[0], piece.depth / size[1], piece.height / size[2], 1.0))
    ground = -piece.height if piece.top_at_ground else 0.0
    model.data.transform(Matrix.Translation((0.0, 0.0, ground)) @ scale
                         @ Matrix.Translation((-center[0], -center[1], -low[2])))


def simplify(model, faces):
    if len(model.data.polygons) <= faces:
        return
    select_only([model])
    bpy.context.view_layer.objects.active = model
    modifier = model.modifiers.new("Simplify", "DECIMATE")
    modifier.ratio = faces / len(model.data.polygons)
    bpy.ops.object.modifier_apply(modifier=modifier.name)


def print_collider(piece):
    offset = -piece.height * 0.5 if piece.top_at_ground else piece.height * 0.5
    print("KIT_COLLIDER {} half_extents ({:.2f}, {:.2f}, {:.2f}) local_offset (0, {:.2f}, 0)".format(
        piece.name, piece.width * 0.5, piece.height * 0.5, piece.depth * 0.5, offset))


def main():
    arguments = parse_arguments()
    for piece in PIECES:
        if arguments.only and arguments.only != piece.name:
            continue
        clear_scene()
        folder = os.path.join(os.path.abspath(arguments.out), piece.folder)
        os.makedirs(folder, exist_ok=True)

        model = load_piece(piece)
        simplify(model, piece.faces)
        snap_to_size(model, piece)
        model.name = piece.name
        character_from_mesh.make_engine_textures(model, folder, piece.name, BRIGHTNESS)
        export_gltf(os.path.join(folder, piece.name + ".gltf"), [model], with_animations=False)
        print_collider(piece)
    print("KIT_DONE")


if __name__ == "__main__":
    main()
