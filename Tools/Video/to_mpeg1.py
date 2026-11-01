"""
Turns any video (.mp4, .mov, .webm, ... e.g. a cutscene from an AI video generator) into the MPEG-1 video the engine plays
(VideoPlayer, a Cutscene's 'videoFile'). Uses Blender, so nothing else has to be installed (no ffmpeg)

Run (from the repository root):
    blender --background --factory-startup --python Tools/Video/to_mpeg1.py -- <input video> <output .mpg> [--width 1280]

Blender's path on this machine: C:\\Program Files (x86)\\Steam\\steamapps\\common\\Blender\\blender.exe
The picture keeps its proportions ('--width' sets its width, the height follows), the sound is left out (the engine
has no sound yet). Put the result in the game's folder, e.g. Projects/MyGame/res/Videos/intro.mpg, and set the
Cutscene's 'videoFile' to @game/Videos/intro.mpg
"""

import argparse
import os
import sys

import bpy


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("input")
    parser.add_argument("output")
    parser.add_argument("--width", type=int, default=1280)
    return parser.parse_args(argv)


def main():
    arguments = parse_arguments()
    source = os.path.abspath(arguments.input)
    target = os.path.abspath(arguments.output)

    scene = bpy.context.scene
    editor = scene.sequence_editor_create()
    clip = editor.strips.new_movie("Video", source, channel=1, frame_start=1)

    # The scene plays exactly the clip, at its own speed and size
    fps = clip.fps if clip.fps > 0 else 30.0
    scene.render.fps = round(fps)
    scene.render.fps_base = scene.render.fps / fps
    scene.frame_start = 1
    scene.frame_end = clip.frame_final_duration

    source_width, source_height = clip.elements[0].orig_width, clip.elements[0].orig_height
    width = min(arguments.width, source_width)
    height = round(source_height * width / source_width / 2) * 2
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.resolution_percentage = 100

    scene.render.image_settings.media_type = "VIDEO"
    scene.render.image_settings.file_format = "FFMPEG"
    scene.render.ffmpeg.format = "MPEG1"
    scene.render.ffmpeg.codec = "MPEG1"
    scene.render.ffmpeg.constant_rate_factor = "HIGH"
    scene.render.ffmpeg.audio_codec = "NONE"
    scene.render.filepath = target
    scene.render.use_sequencer = True

    os.makedirs(os.path.dirname(target), exist_ok=True)
    bpy.ops.render.render(animation=True)
    print("TO_MPEG1_DONE {} ({}x{}, {} frames)".format(target, width, height, scene.frame_end))


if __name__ == "__main__":
    main()
