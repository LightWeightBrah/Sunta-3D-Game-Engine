"""
Generates the fog of a boss arena's doorway (Dark Souls style): pale, slowly flowing veils of mist

Run (from the repository root):
    blender --background --factory-startup --python Tools/Blender/fog_gate_generator.py -- --out Sunta/res/Models

Writes <out>/FogGate/fog_gate.gltf (+ .bin, fog_gate_diffuse.png, fog_gate_specular.png)
Optional: --preview <file> renders a picture of the model

The engine has no blending for models, only alpha cutout: a pixel of the texture is drawn or not.
So the mist is drawn as scattered dots: every pixel is drawn with the probability of the fog's density there (dithering).
From a distance the dots blend into soft, see-through veils. Three veils stand one behind another for depth
The origin is on the ground in the middle of the doorway, the veils face the arena's doorway (along Y in Blender)
"""

import argparse
import os
import sys

import bpy
import numpy

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from sunta_blender import clear_scene, export_gltf, make_textured_material, place_preview_camera, setup_preview_render  # noqa: E402

# The doorway of castle_gate is 3.2 m wide and 4.2 m high: the veils reach a little into the stone around it
FOG_WIDTH = 3.5
FOG_HEIGHT = 4.6

# (offset along the doorway's depth in meters, sideways shift of the texture) of every veil
VEILS = ((-0.12, 0.0), (0.0, 0.37), (0.12, 0.71))

TEXTURE_WIDTH = 512
TEXTURE_HEIGHT = 1024

MIST_COLOR = numpy.array((0.80, 0.82, 0.86))
RANDOM_SEED = 3

# How much of the densest fog is drawn, on one veil (three veils add up)
MIST_OPACITY = 0.55


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--preview", default="")
    return parser.parse_args(argv)


# ========== The texture ==========


def smooth_noise(generator, columns, rows):
    """Random values on a coarse grid, stretched to the size of the texture (bilinear), wrapping around sideways"""
    grid = generator.random((rows + 1, columns))
    y = numpy.linspace(0, rows, TEXTURE_HEIGHT, endpoint=False)
    x = numpy.linspace(0, columns, TEXTURE_WIDTH, endpoint=False)
    y0, x0 = y.astype(int), x.astype(int)
    fy, fx = (y - y0)[:, None], (x - x0)[None, :]
    x1 = (x0 + 1) % columns

    top = grid[y0][:, x0] * (1 - fx) + grid[y0][:, x1] * fx
    bottom = grid[y0 + 1][:, x0] * (1 - fx) + grid[y0 + 1][:, x1] * fx
    return top * (1 - fy) + bottom * fy


def mist_density(generator):
    """Tall, thin streaks (many columns, few rows) over larger, softer clouds"""
    streaks = smooth_noise(generator, 48, 6) * 0.55 + smooth_noise(generator, 96, 10) * 0.25
    clouds = smooth_noise(generator, 6, 4) * 0.35
    density = streaks + clouds

    # Thinner near the top and the bottom: the fog fades into the arch and the floor
    height = numpy.linspace(0.0, 1.0, TEXTURE_HEIGHT)[:, None]
    fade = numpy.clip(numpy.minimum(height / 0.12, (1.0 - height) / 0.2), 0.0, 1.0)
    return density * (0.4 + 0.6 * fade)


def make_fog_textures(folder):
    generator = numpy.random.default_rng(RANDOM_SEED)
    density = mist_density(generator)
    density = (density - density.min()) / (density.max() - density.min())

    rgba = numpy.zeros((TEXTURE_HEIGHT, TEXTURE_WIDTH, 4), dtype=numpy.float32)
    rgba[..., :3] = MIST_COLOR * (0.8 + 0.2 * density[..., None])
    rgba[..., 3] = (generator.random(density.shape) < density ** 1.5 * MIST_OPACITY * 1.8).astype(numpy.float32)

    diffuse = save_image("fog_gate_diffuse", folder, rgba, has_alpha=True)

    # The mist doesn't shine
    black = numpy.zeros((8, 8, 4), dtype=numpy.float32)
    black[..., 3] = 1.0
    save_image("fog_gate_specular", folder, black, has_alpha=False)
    return diffuse


def save_image(name, folder, rgba, has_alpha):
    height, width = rgba.shape[:2]
    image = bpy.data.images.new(name, width, height, alpha=has_alpha)
    # Blender's pixels start at the bottom row
    image.pixels.foreach_set(rgba[::-1].ravel())
    image.filepath_raw = os.path.join(folder, name + ".png")
    image.file_format = "PNG"
    image.save()
    return image


# ========== The veils ==========


def build_veils(texture):
    """Three upright planes one behind another, each seen from both sides (two faces back to back)"""
    vertices, faces, uvs = [], [], []
    half_width = FOG_WIDTH * 0.5

    for depth, texture_shift in VEILS:
        corners = [(-half_width, depth, 0.0), (half_width, depth, 0.0), (half_width, depth, FOG_HEIGHT), (-half_width, depth, FOG_HEIGHT)]
        corner_uvs = [(texture_shift, 0.0), (texture_shift + 1.0, 0.0), (texture_shift + 1.0, 1.0), (texture_shift, 1.0)]

        for order in ((0, 1, 2, 3), (1, 0, 3, 2)):
            start = len(vertices)
            vertices += [corners[index] for index in order]
            uvs += [corner_uvs[index] for index in order]
            faces.append((start, start + 1, start + 2, start + 3))

    mesh = bpy.data.meshes.new("FogGate")
    mesh.from_pydata(vertices, [], faces)
    uv_layer = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        uv_layer.data[loop.index].uv = uvs[loop.vertex_index]

    fog = bpy.data.objects.new("FogGate", mesh)
    bpy.context.scene.collection.objects.link(fog)
    fog.data.materials.append(make_textured_material("fog_gate", texture))
    return fog


def main():
    arguments = parse_arguments()
    folder = os.path.join(os.path.abspath(arguments.out), "FogGate")
    os.makedirs(folder, exist_ok=True)

    clear_scene()
    fog = build_veils(make_fog_textures(folder))
    export_gltf(os.path.join(folder, "fog_gate.gltf"), [fog], with_animations=False)

    if arguments.preview:
        camera = setup_preview_render(500, 600)
        place_preview_camera(camera, 20, distance=7.0, height=2.3, target_height=2.2)
        bpy.context.scene.render.filepath = os.path.abspath(arguments.preview)
        bpy.ops.render.render(write_still=True)

    print("FOG_GATE_GENERATOR_DONE")


if __name__ == "__main__":
    main()
