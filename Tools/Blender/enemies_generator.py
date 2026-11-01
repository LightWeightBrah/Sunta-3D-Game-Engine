"""
Generates the enemies of the example game (MyGame) from their generated, rigged models: the hollow ghoul, the ash cultist,
the executioner and the skeleton spearman, each with its weapons as separate models

Run (from the repository root, the Mixamo files of knight_generator.py must be downloaded):
    blender --background --factory-startup --python Tools/Blender/enemies_generator.py -- --out Projects/MyGame/res/Models/Enemies

Writes <out>/<Folder>/<name>.gltf for every enemy and weapon (+ .bin, _diffuse.png, _specular.png). They are the game's
models, not the engine's: the engine loads every model in the game's folder by its file name, so a scene uses them
with "model_name": "<name>". Prints the hitbox of every weapon (for its entity in the scene)
Optional: --preview <folder> renders pictures, --only <name> makes one enemy

Where they come from (see Generated/meshy_log.json): the concepts Concepts/Enemies/enemy_<name>.png (Meshy Text to Image,
approved by the user), the same character with empty hands and every weapon alone (Meshy Image to Image), textured models
of those (Meshy Image to 3D) and the bodies rigged by Meshy: Generated/Enemies/<name>_rigged.glb and <weapon>.glb.
The hands are closed into fists around the grips (character_from_mesh.make_fist), the weapons hang on bones in the fists
"""

import argparse
import os
import sys
from dataclasses import dataclass, field

import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import boss_generator  # noqa: E402
import character_from_mesh  # noqa: E402
import knight_generator  # noqa: E402
import motion_retarget  # noqa: E402
from sunta_blender import clear_scene, export_gltf, place_preview_camera, setup_preview_render, show_only  # noqa: E402

BLENDER_FOLDER = os.path.dirname(os.path.abspath(__file__))
ENEMIES_FOLDER = os.path.join(BLENDER_FOLDER, "Generated", "Enemies")

# Mixamo's sword-and-shield moves (the knights') for one-handed weapons, the Great Sword Pack's (the boss's) for two hands
ONE_HANDED = knight_generator.ANIMATIONS
TWO_HANDED = {name: clip for name, clip in boss_generator.BOSS_ANIMATIONS.items() if name not in ("Attack_Leap", "Phase_Two")}


@dataclass
class Weapon:
    """A weapon model held in a fist. Meshy made it standing upright, its edge (or face) towards -X"""
    source: str               # in Generated/Enemies
    name: str                 # the files and the model in the engine
    length: float             # meters, end to end
    grip: float               # where the fist holds it: this part of the length from the handle's end
    is_handle_on_top: bool = False
    is_shield: bool = False   # held on the back of the fist, its face outwards (instead of through the fist)


@dataclass
class Enemy:
    source: str               # in Generated/Enemies
    folder: str
    name: str
    height: float             # meters, to the top of the head
    animations: dict
    right_hand: Weapon
    left_hand: Weapon = None
    is_left_fist: bool = False  # two-handed weapons: the left hand closes around the haft too
    brightness: float = 1.3
    weapons: list = field(default_factory=list)


ENEMIES = [
    Enemy("hollow_ghoul_rigged.glb", "HollowGhoul", "hollow_ghoul", 1.8, ONE_HANDED,
          Weapon("ghoul_cleaver.glb", "ghoul_cleaver", 0.55, 0.18)),
    Enemy("ash_cultist_rigged.glb", "AshCultist", "ash_cultist", 1.9, TWO_HANDED,
          Weapon("cultist_staff.glb", "cultist_staff", 1.9, 0.42), is_left_fist=True, brightness=1.7),
    Enemy("executioner_rigged.glb", "Executioner", "executioner", 2.1, TWO_HANDED,
          Weapon("executioner_cleaver.glb", "executioner_cleaver", 1.6, 0.1, is_handle_on_top=True), is_left_fist=True),
    Enemy("skeleton_spearman_rigged.glb", "SkeletonSpearman", "skeleton_spearman", 1.8, ONE_HANDED,
          Weapon("skeleton_spear.glb", "skeleton_spear", 2.2, 0.4),
          left_hand=Weapon("skeleton_shield.glb", "skeleton_shield", 0.62, 0.5, is_shield=True)),
]

SHIELD_GAP = 0.008


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--preview", default="")
    parser.add_argument("--only", default="")
    return parser.parse_args(argv)


# ========== Weapons ==========


def load_weapon(weapon, folder, brightness):
    """The weapon with its grip at the origin: along +Z (the handle below), its edge towards -X.
    A shield: its middle at the origin, its face towards -Y and its top along +Z"""
    objects_before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(ENEMIES_FOLDER, weapon.source))
    imported = [obj for obj in bpy.data.objects if obj not in objects_before]
    model = next(obj for obj in imported if obj.type == "MESH")
    world_matrix = model.matrix_world.copy()
    model.parent = None
    for obj in imported:
        if obj is not model:
            bpy.data.objects.remove(obj)
    model.data.transform(world_matrix)
    model.matrix_world = Matrix.Identity(4)

    if weapon.is_handle_on_top:
        model.data.transform(Matrix.Rotation(3.14159265, 4, "X"))

    points = [vertex.co for vertex in model.data.vertices]
    lowest, highest = min(p.z for p in points), max(p.z for p in points)
    middle_x = (min(p.x for p in points) + max(p.x for p in points)) * 0.5
    middle_y = (min(p.y for p in points) + max(p.y for p in points)) * 0.5
    scale = weapon.length / (highest - lowest)
    grip = weapon.length * weapon.grip
    model.data.transform(Matrix.Translation((0.0, 0.0, -grip)) @ Matrix.Scale(scale, 4)
                         @ Matrix.Translation((-middle_x, -middle_y, -lowest)))

    model.name = weapon.name
    os.makedirs(folder, exist_ok=True)
    character_from_mesh.make_engine_textures(model, folder, weapon.name, brightness)
    export_gltf(os.path.join(folder, weapon.name + ".gltf"), [model], with_animations=False)

    # The hitbox in the engine (glTF axes: +Z here is +Y there): from the fist to the tip
    points = [vertex.co for vertex in model.data.vertices]
    reach = max(p.z for p in points)
    half_x = (max(p.x for p in points) - min(p.x for p in points)) * 0.5
    half_y = (max(p.y for p in points) - min(p.y for p in points)) * 0.5
    print("WEAPON_HITBOX {} local_offset (0, {:.3f}, 0) half_extents ({:.3f}, {:.3f}, {:.3f})".format(
        weapon.name, reach * 0.5, half_x, reach * 0.5, half_y))
    return model


def weapon_in_fist(weapon, grip):
    """Where the weapon stands in the rest pose: through the fist towards the thumb, its edge where the fingers pointed"""
    return grip.item_matrix(-grip.fingers, grip.fingers.cross(grip.along), grip.along)


def shield_on_fist(grip, thickness):
    """Its back just outside the back of the fist, its face outwards, its top towards the fingers"""
    face = -grip.palm
    top = grip.fingers
    side = (-face).cross(top)
    # The fist is behind the shield (towards +Y of the shield, whose face looks at -Y)
    middle = Vector((0.0, grip.back_of_fist + SHIELD_GAP + thickness * 0.5, 0.0))
    return grip.item_matrix(side, -face, top, middle)


# ========== Enemies ==========


def make_enemy(enemy, arguments):
    clear_scene()
    out_folder = os.path.join(os.path.abspath(arguments.out), enemy.folder)
    os.makedirs(out_folder, exist_ok=True)

    our_names = {theirs: ours for ours, theirs in motion_retarget.MESHY.bone_of.items()}
    armature, body = character_from_mesh.load_rigged_character(os.path.join(ENEMIES_FOLDER, enemy.source), enemy.height, our_names)
    body.name = enemy.name
    character_from_mesh.make_engine_textures(body, out_folder, enemy.name, enemy.brightness)

    right = character_from_mesh.make_fist(armature, body, "_R")
    held = [(knight_generator.WEAPON_BONE, knight_generator.WEAPON_PARENT_BONE, weapon_in_fist(enemy.right_hand, right))]
    weapons = [(enemy.right_hand, held[0])]

    if enemy.left_hand or enemy.is_left_fist:
        left = character_from_mesh.make_fist(armature, body, "_L")
        if enemy.left_hand:
            shield = load_weapon(enemy.left_hand, out_folder, enemy.brightness)
            thickness = max(v.co.y for v in shield.data.vertices) - min(v.co.y for v in shield.data.vertices)
            held.append((knight_generator.SHIELD_BONE, knight_generator.SHIELD_PARENT_BONE, shield_on_fist(left, thickness)))
            weapons.append((enemy.left_hand, held[1]))
            bpy.data.objects.remove(shield)

    character_from_mesh.add_held_item_bones(armature, held)
    knight_generator.create_animations(armature, enemy.animations)
    knight_generator.reset_to_rest_pose(armature)
    export_gltf(os.path.join(out_folder, enemy.name + ".gltf"), [armature, body], with_animations=True)

    weapon_object = load_weapon(enemy.right_hand, out_folder, enemy.brightness)
    if arguments.preview:
        shown = [body, weapon_object]
        knight_generator.hang_on_bone(weapon_object, armature, knight_generator.WEAPON_BONE, held[0][2])
        if enemy.left_hand:
            shield = load_weapon(enemy.left_hand, out_folder, enemy.brightness)
            knight_generator.hang_on_bone(shield, armature, knight_generator.SHIELD_BONE, held[1][2])
            shown.append(shield)
        render_previews(os.path.abspath(arguments.preview), armature, shown, enemy)


def render_previews(folder, armature, shown, enemy):
    os.makedirs(folder, exist_ok=True)
    show_only(shown)
    camera = setup_preview_render(360, 440)
    scene = bpy.context.scene
    size = enemy.height / 1.8
    for action_name in ("Idle", "Walk", "Attack_Light_1", "Attack_Heavy"):
        armature.animation_data.action = bpy.data.actions[action_name]
        start, end = (int(frame) for frame in armature.animation_data.action.frame_range)
        for step in range(3):
            scene.frame_set(start + round((end - start) * step / 2))
            place_preview_camera(camera, 35, distance=4.2 * size, height=1.2 * size, target_height=1.0 * size)
            scene.render.filepath = os.path.join(folder, "{}_{}_{}.png".format(enemy.name, action_name, step))
            bpy.ops.render.render(write_still=True)


def main():
    arguments = parse_arguments()
    for enemy in ENEMIES:
        if not arguments.only or arguments.only == enemy.name:
            make_enemy(enemy, arguments)
    print("ENEMIES_GENERATOR_DONE")


if __name__ == "__main__":
    main()
