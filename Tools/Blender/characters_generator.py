"""
Generates the knights of the example game from their generated meshes: the player (Knight) and the enemy (Rusted Knight)

Run (from the repository root, the Mixamo files of knight_generator.py must be downloaded):
    blender --background --factory-startup --python Tools/Blender/characters_generator.py -- --out Sunta/res/Models

Writes <out>/Knight/knight.gltf and <out>/RustedKnight/rusted_knight.gltf (+ .bin, _diffuse.png, _specular.png)
Optional: --icons <folder> renders the editor icons, --preview <folder> renders pictures of some animations

Where the bodies come from: the concepts Concepts/knight_concept.png and Concepts/rusted_knight_concept.png
were made by Meshy (Text to Image, A pose) and turned into textured meshes by Meshy (Image to 3D, A pose),
see Generated/meshy_log.json for every setting. The player is rigged by Meshy too (its bones and weights bend the armor
much better than automatic weights); for the others character_from_mesh.py fits the skeleton. Then come the animations,
and the sword and the shield (knight_generator.py) hang on bones placed at the hands
"""

import argparse
import os
import sys
from dataclasses import dataclass

import bpy
from mathutils import Vector

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import character_from_mesh  # noqa: E402
import knight_generator  # noqa: E402
import motion_retarget  # noqa: E402
from sunta_blender import clear_scene, export_gltf, place_preview_camera, setup_preview_render, show_only  # noqa: E402

BLENDER_FOLDER = os.path.dirname(os.path.abspath(__file__))

# A little brighter than the generated base color: the engine has no reflections, so dark steel would look flat
KNIGHT_BRIGHTNESS = 1.5

# A round shield is held by a grip behind its boss: its middle is at the left fist, its face this far outside the fist
# (parts of the character's height), so the hand stays behind the face
SHIELD_OUT_OF_FIST = 0.04


@dataclass
class Character:
    source: str        # in Generated
    folder: str        # in the output folder
    name: str          # the files and the model in the engine
    height: float      # meters, to the top of the head
    icon: str          # the editor icon's file name
    brightness: float = KNIGHT_BRIGHTNESS
    is_rigged: bool = False  # rigged by Meshy (its bones and weights are kept), otherwise the skeleton is fitted here


CHARACTERS = [
    Character("knight_meshy_rigged.glb", "Knight", "knight", 1.85, "souls_player_icon.png", is_rigged=True),
    Character("rusted_knight_meshy.glb", "RustedKnight", "rusted_knight", 1.85, "rusted_knight_icon.png"),
]


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--icons", default="")
    parser.add_argument("--preview", default="")
    parser.add_argument("--only", default="", help="the name of one character to make")
    return parser.parse_args(argv)


def sword_and_shield(skeleton):
    """The sword in the right fist, the shield held by the left fist, for a skeleton (fitted or rigged)"""
    height = max(head[2] for name, parent, head, tail, roll in skeleton if name == "Head") / character_from_mesh.HEAD_HEIGHT
    shield_center = character_from_mesh.fist_center(skeleton, "_L") + Vector((-SHIELD_OUT_OF_FIST * height, 0.0, 0.0))

    return [(knight_generator.WEAPON_BONE, knight_generator.WEAPON_PARENT_BONE,
             knight_generator.get_sword_rest_matrix(character_from_mesh.fist_center(skeleton, "_R"))),
            (knight_generator.SHIELD_BONE, knight_generator.SHIELD_PARENT_BONE,
             knight_generator.get_shield_rest_matrix(shield_center))]


def render_icon(path, armature, mesh, height):
    """The editor icon: the helmet and the shoulders on a transparent background"""
    show_only([mesh])
    camera = setup_preview_render(knight_generator.ICON_PIXELS, knight_generator.ICON_PIXELS, is_transparent=True)
    place_preview_camera(camera, knight_generator.ICON_CAMERA_ANGLE, distance=1.0 * height / 1.82,
                         height=0.89 * height, target_height=0.86 * height)
    bpy.context.scene.render.filepath = os.path.abspath(path)
    bpy.ops.render.render(write_still=True)


def render_previews(folder, armature, mesh, name):
    os.makedirs(folder, exist_ok=True)
    show_only([mesh])
    camera = setup_preview_render(300, 380)
    scene = bpy.context.scene

    for action_name in ("Idle", "Walk", "Roll", "Attack_Light_1", "Attack_Heavy", "Block"):
        armature.animation_data.action = bpy.data.actions[action_name]
        start, end = (int(frame) for frame in armature.animation_data.action.frame_range)
        for step in range(3):
            scene.frame_set(start + round((end - start) * step / 2))
            place_preview_camera(camera, 35, distance=3.6, height=1.1, target_height=0.95)
            scene.render.filepath = os.path.join(folder, "{}_{}_{}.png".format(name, action_name, step))
            bpy.ops.render.render(write_still=True)


def make_character(character, arguments):
    clear_scene()
    folder = os.path.join(os.path.abspath(arguments.out), character.folder)
    os.makedirs(folder, exist_ok=True)

    source = os.path.join(BLENDER_FOLDER, "Generated", character.source)
    if character.is_rigged:
        our_names = {theirs: ours for ours, theirs in motion_retarget.MESHY.bone_of.items()}
        armature, mesh = character_from_mesh.load_rigged_character(source, character.height, our_names)
    else:
        mesh = character_from_mesh.load_mesh(source, character.height)
    mesh.name = character.name
    character_from_mesh.make_engine_textures(mesh, folder, character.name, character.brightness)

    if character.is_rigged:
        character_from_mesh.add_held_item_bones(armature, sword_and_shield(character_from_mesh.skeleton_of(armature)))
        knight_generator.create_animations(armature)
        knight_generator.reset_to_rest_pose(armature)
    else:
        armature, skeleton = character_from_mesh.build_character(mesh, sword_and_shield)
    export_gltf(os.path.join(folder, character.name + ".gltf"), [armature, mesh], with_animations=True)

    if arguments.icons:
        render_icon(os.path.join(os.path.abspath(arguments.icons), character.icon), armature, mesh, character.height)
    if arguments.preview:
        render_previews(os.path.abspath(arguments.preview), armature, mesh, character.name)


def main():
    arguments = parse_arguments()
    for character in CHARACTERS:
        if not arguments.only or arguments.only == character.name:
            make_character(character, arguments)
    print("CHARACTERS_GENERATOR_DONE")


if __name__ == "__main__":
    main()
