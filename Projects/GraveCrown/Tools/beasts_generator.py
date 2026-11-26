"""
Generates the four-legged beasts of Grave Crown: the dead king's black horse (and later the undead horse of the rider and
the wolf). Meshy made their meshes; Mesh2Motion (CC0, app.mesh2motion.org) gives a quadruped skeleton with motion
capture-like clips (horse: idle, walk, trot, gallop, rear, kick, death...; fox: for the wolf). The skeleton is fitted to the
mesh (the length of the body and the height), the mesh is skinned to it by Blender's automatic weights and the clips
play as they are.

    blender --background --factory-startup --python Projects/GraveCrown/Tools/beasts_generator.py -- --out Projects/GraveCrown/res/Models/Characters
    (--only black_horse, --preview <folder>)

Writes <out>/<Folder>/<name>.gltf (+ textures). The beast faces +Y in Blender (the engine's forward, -Z)
"""

import argparse
import math
import os
import sys

import bpy
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
BLENDER = os.path.normpath(os.path.join(HERE, "..", "..", "..", "Tools", "Blender"))
sys.path.insert(0, BLENDER)

import character_from_mesh  # noqa: E402
from sunta_blender import clear_scene, export_gltf, select_only, place_preview_camera, setup_preview_render, show_only  # noqa: E402

GENERATED = os.path.join(BLENDER, "Generated", "GraveCrown")
MESH2MOTION = os.path.join(BLENDER, "Mesh2Motion")


class Beast:
    def __init__(self, source, folder, name, height, skeleton, clips, renames, brightness=1.4):
        self.source, self.folder, self.name, self.height = source, folder, name, height
        self.skeleton, self.clips, self.renames, self.brightness = skeleton, clips, renames, brightness


# The clip names the engine sees: idle, walk, the trot as the run of a led horse, the gallop as the run of a ridden one
HORSE_CLIPS = os.path.join(MESH2MOTION, "horse", "horse-animations.glb")
HORSE_NAMES = {"Idle": "Idle", "Walk": "Walk", "Trot": "Trot", "Run": "Gallop", "Rear": "Rear", "Kick": "Kick",
               "Death": "Death", "Head_But": "Head_Butt", "Eating": "Eat", "Turn_Left": "Turn_Left",
               "Turn_Right": "Turn_Right"}

# The wolf: the fox's skeleton and clips (one file per clip), named as a melee enemy's moves
FOX = os.path.join(MESH2MOTION, "fox")
WOLF_CLIPS = {"idle": "Idle", "walk": "Walk", "run": "Run", "bite": "Attack_Light_1", "jump": "Attack_Heavy",
              "death": "Death", "shake-off": "Hit", "howl": "Howl", "idle-alert": "Alert"}

BEASTS = [
    Beast("horse.glb", "BlackHorse", "black_horse", 2.25, HORSE_CLIPS, HORSE_NAMES, HORSE_NAMES),
    Beast("enemy_wolf.glb", "AshWolf", "ash_wolf", 1.25, os.path.join(FOX, "skeleton.glb"),
          {os.path.join(FOX, "animations", name + ".glb"): ours for name, ours in WOLF_CLIPS.items()}, None),
]


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--only", default="")
    parser.add_argument("--preview", default="")
    return parser.parse_args(argv)


def bounds(points):
    return (Vector([min(p[i] for p in points) for i in range(3)]), Vector([max(p[i] for p in points) for i in range(3)]))


def load_beast_mesh(beast):
    """The mesh standing on the ground, its length along Y with the head towards +Y, 'height' tall"""
    mesh = character_from_mesh.load_mesh(os.path.join(GENERATED, beast.source), beast.height)
    points = [v.co for v in mesh.data.vertices]
    low, high = bounds(points)
    # The body's length lies along X or Y: turn it onto Y
    if (high.x - low.x) > (high.y - low.y):
        mesh.data.transform(Matrix.Rotation(math.radians(90.0), 4, "Z"))
        low, high = bounds([v.co for v in mesh.data.vertices])
    # The head is the higher end (the neck rises, the tail hangs)
    middle = (low.y + high.y) * 0.5
    front = [v.co.z for v in mesh.data.vertices if v.co.y > high.y - (high.y - low.y) * 0.15]
    back = [v.co.z for v in mesh.data.vertices if v.co.y < low.y + (high.y - low.y) * 0.15]
    if max(back) > max(front):
        mesh.data.transform(Matrix.Rotation(math.pi, 4, "Z"))
    low, high = bounds([v.co for v in mesh.data.vertices])
    mesh.data.transform(Matrix.Translation((-(low.x + high.x) * 0.5, -(low.y + high.y) * 0.5, 0.0)))
    print("BEAST_MESH", beast.name, "length", round(high.y - low.y, 2), "height", round(high.z - low.z, 2), "middle", round(middle, 2))
    return mesh


def load_skeleton(beast):
    """The Mesh2Motion armature with all its clips (actions), facing +Y"""
    before_objects, before_actions = set(bpy.data.objects), set(bpy.data.actions)
    bpy.ops.import_scene.gltf(filepath=beast.skeleton)
    imported = [o for o in bpy.data.objects if o not in before_objects]
    armature = next(o for o in imported if o.type == "ARMATURE")
    for obj in imported:
        if obj is not armature:
            bpy.data.objects.remove(obj)
    actions = [a for a in bpy.data.actions if a not in before_actions]
    if isinstance(beast.clips, dict):
        # One file per clip: each brings its own copy of the skeleton, only its action is kept (renamed to ours)
        actions = []
        for path, name in beast.clips.items():
            objects, known = set(bpy.data.objects), set(bpy.data.actions)
            bpy.ops.import_scene.gltf(filepath=path)
            for obj in [o for o in bpy.data.objects if o not in objects]:
                bpy.data.objects.remove(obj)
            new = [a for a in bpy.data.actions if a not in known]
            if new:
                new[0].name = name
                actions.append(new[0])
                for extra in new[1:]:
                    bpy.data.actions.remove(extra)
    head = armature.data.bones["head"].head_local
    tail = armature.data.bones["tail_1"].head_local if "tail_1" in armature.data.bones else Vector((0, 0, 0))
    turn = 0.0 if (armature.matrix_world @ head).y > (armature.matrix_world @ tail).y else math.pi
    return armature, actions, turn


def hooves(points, height):
    """The middle (y) of the front and of the hind feet: the lowest vertices, split at their middle along the body"""
    low = sorted(p.y for p in points if p.z < height * 0.06)
    middle = (low[0] + low[-1]) * 0.5
    back = [y for y in low if y < middle]
    front = [y for y in low if y >= middle]
    return sum(front) / len(front), sum(back) / len(back)


def fit_skeleton(armature, turn, mesh):
    """Scales the armature's rest pose to the mesh: the distance between its front and hind hooves along the body, its
    height to the top of the head; then lays the tail bones along the mesh's own tail (which hangs, theirs points back).
    The clips' moves of the root and the hips are scaled the same way (scale_root_moves)"""
    points = [v.co for v in mesh.data.vertices]
    mesh_low, mesh_high = bounds(points)
    height = mesh_high.z - mesh_low.z
    mesh_front, mesh_back = hooves(points, height)

    armature.matrix_world = Matrix.Rotation(turn, 4, "Z") @ armature.matrix_world
    select_only([armature])
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bones = armature.data.bones
    feet_front = (bones["front_leg_foot_l"].head_local.y + bones["front_leg_foot_r"].head_local.y) * 0.5
    feet_back = (bones["back_leg_foot_l"].head_local.y + bones["back_leg_foot_r"].head_local.y) * 0.5
    top = max(max(b.head_local.z, b.tail_local.z) for b in bones)
    along = (mesh_front - mesh_back) / max(feet_front - feet_back, 1e-3)
    up = height / max(top, 1e-3)
    across = (along + up) * 0.5
    armature.matrix_world = Matrix.Diagonal((across, along, up, 1.0))
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    feet_back = (bones["back_leg_foot_l"].head_local.y + bones["back_leg_foot_r"].head_local.y) * 0.5
    armature.location = Vector((0.0, mesh_back - feet_back, 0.0))
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)

    # The tail: the mesh behind the hind legs and off the ground, from its highest point down to its end
    tail = [p for p in points if p.y < mesh_back - 0.12 * (mesh_front - mesh_back) and p.z > height * 0.12]
    chain = [b for b in ("tail_1", "tail_2", "tail_3", "tail_4", "tail_leaf", "tail_tip") if b in bones]
    if tail and chain:
        root = max(tail, key=lambda p: p.z)
        end = min(tail, key=lambda p: p.z - (root.y - p.y) * 0.3)
        bpy.context.view_layer.objects.active = armature
        bpy.ops.object.mode_set(mode="EDIT")
        edit = armature.data.edit_bones
        for i, name in enumerate(chain):
            bone = edit[name]
            bone.head = root.lerp(end, i / len(chain))
            bone.tail = root.lerp(end, (i + 1) / len(chain))
        bpy.ops.object.mode_set(mode="OBJECT")
    print("SKELETON_FIT along {:.2f} up {:.2f}, hooves {:.2f}..{:.2f}, tail {} vertices".format(
        along, up, mesh_back, mesh_front, len(tail)))
    return Vector((across, along, up))


def scale_root_moves(actions, factors):
    """The location keys of the clips move bones in their own (scaled) space: scale them like the rest pose"""
    for action in actions:
        for curve in getattr(action, "fcurves", []):
            if curve.data_path.endswith(".location"):
                factor = factors[curve.array_index]
                for point in curve.keyframe_points:
                    point.co.y *= factor
                    point.handle_left.y *= factor
                    point.handle_right.y *= factor


def skin(armature, mesh, nearest=2, sharpness=4.0):
    """Every vertex follows its 'nearest' bones, weighted by 1/distance^sharpness to each bone's segment. Blender's
    automatic weights (bone heat) find nothing on generated meshes full of loose, overlapping pieces"""
    import numpy
    bones = [b for b in armature.data.bones if b.use_deform and not b.name.endswith(("_leaf", "_tip")) and b.name != "root"]
    points = numpy.array([v.co[:] for v in mesh.data.vertices])
    distances = numpy.empty((len(bones), len(points)))
    for i, bone in enumerate(bones):
        head, tail = numpy.array(bone.head_local[:]), numpy.array(bone.tail_local[:])
        segment = tail - head
        t = numpy.clip(((points - head) @ segment) / max(segment @ segment, 1e-9), 0.0, 1.0)
        distances[i] = numpy.linalg.norm(points - (head + t[:, None] * segment), axis=1)
    # The tail follows only the tail bones and the body never does (a hanging tail passes close to the hind legs)
    tail_rows = numpy.array([b.name.startswith("tail") for b in bones])
    if tail_rows.any():
        tail_distance = distances[tail_rows].min(axis=0)
        body_distance = distances[~tail_rows].min(axis=0)
        is_tail = tail_distance < body_distance * 0.8
        distances[numpy.ix_(tail_rows, ~is_tail)] = 1e6
        distances[numpy.ix_(~tail_rows, is_tail)] = 1e6
    order = numpy.argsort(distances, axis=0)[:nearest]
    chosen = numpy.take_along_axis(distances, order, axis=0)
    weights = 1.0 / numpy.maximum(chosen, 1e-3) ** sharpness
    weights /= weights.sum(axis=0)
    groups = [mesh.vertex_groups.get(b.name) or mesh.vertex_groups.new(name=b.name) for b in bones]
    for vertex in range(len(points)):
        for k in range(nearest):
            groups[order[k, vertex]].add([vertex], float(weights[k, vertex]), "REPLACE")
    mesh.parent = armature
    modifier = mesh.modifiers.new("Armature", "ARMATURE")
    modifier.object = armature
    print("SKIN {} vertices to {} bones".format(len(points), len(bones)))


def make_beast(beast, arguments):
    clear_scene()
    out_folder = os.path.join(os.path.abspath(arguments.out), beast.folder)
    os.makedirs(out_folder, exist_ok=True)
    mesh = load_beast_mesh(beast)
    armature, actions, turn = load_skeleton(beast)
    factors = fit_skeleton(armature, turn, mesh)
    scale_root_moves(actions, factors)
    skin(armature, mesh)
    mesh.name = beast.name
    character_from_mesh.make_engine_textures(mesh, out_folder, beast.name, beast.brightness)
    for action in actions:
        base = action.name.split(".")[0]
        if beast.renames is None:
            continue
        if base in beast.renames:
            action.name = beast.renames[base]
        else:
            bpy.data.actions.remove(action)
    kept = [a for a in bpy.data.actions]
    print("BEAST_CLIPS", beast.name, [(a.name, int(a.frame_range[1])) for a in kept])
    if not armature.animation_data:
        armature.animation_data_create()
    armature.animation_data.action = bpy.data.actions.get("Idle")
    export_gltf(os.path.join(out_folder, beast.name + ".gltf"), [armature, mesh], with_animations=True)
    if arguments.preview:
        render_previews(os.path.abspath(arguments.preview), armature, mesh, beast)


def render_previews(folder, armature, mesh, beast):
    os.makedirs(folder, exist_ok=True)
    show_only([mesh])
    camera = setup_preview_render(520, 400)
    scene = bpy.context.scene
    for name in ("Idle", "Walk", "Gallop", "Rear", "Run", "Attack_Light_1", "Attack_Heavy"):
        action = bpy.data.actions.get(name)
        if not action:
            continue
        armature.animation_data.action = action
        if hasattr(armature.animation_data, "action_slot") and action.slots:
            armature.animation_data.action_slot = action.slots[0]
        start, end = (int(f) for f in action.frame_range)
        for step in range(3):
            scene.frame_set(start + round((end - start) * step / 2))
            place_preview_camera(camera, 90, distance=6.5, height=1.4, target_height=1.1)
            scene.render.filepath = os.path.join(folder, "{}_{}_{}.png".format(beast.name, name, step))
            bpy.ops.render.render(write_still=True)


def main():
    arguments = parse_arguments()
    for beast in BEASTS:
        if not arguments.only or arguments.only == beast.name:
            make_beast(beast, arguments)
    print("BEASTS_DONE")


if __name__ == "__main__":
    main()
