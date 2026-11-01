"""
Turns a panorama of the sky (equirectangular: 360 degrees across, 180 degrees down, e.g. a Poly Haven HDRI's
tonemapped JPG) into the six pictures of an engine skybox (an OpenGL cubemap, see learnopengl.com: Cubemaps)

Run (plain Python with numpy and Pillow):
    python Tools/Skybox/panorama_to_cubemap.py <panorama.jpg> <out folder> --size 1024 --brightness 0.35 --saturation 0.6

Writes right.jpg, left.jpg, top.jpg, bottom.jpg, front.jpg, back.jpg (+X, -X, +Y, -Y, +Z, -Z), the names the engine's
Environment component looks for in its skybox folder. The look can be changed on the way: darker, less colorful, tinted
(a night sky over a burned castle), the horizon and everything below it faded into the fog color
"""

import argparse
import os

import numpy
from PIL import Image

Image.MAX_IMAGE_PIXELS = None

# The faces in OpenGL's order and the direction through each pixel: u goes right, v goes down the picture (-1..1)
FACES = {
    "right":  lambda u, v, one: (one, -v, -u),
    "left":   lambda u, v, one: (-one, -v, u),
    "top":    lambda u, v, one: (u, one, v),
    "bottom": lambda u, v, one: (u, -one, -v),
    "front":  lambda u, v, one: (u, -v, one),
    "back":   lambda u, v, one: (-u, -v, -one),
}


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("panorama")
    parser.add_argument("out")
    parser.add_argument("--size", type=int, default=1024)
    parser.add_argument("--brightness", type=float, default=1.0)
    parser.add_argument("--saturation", type=float, default=1.0)
    parser.add_argument("--tint", type=float, nargs=3, default=(1.0, 1.0, 1.0))
    parser.add_argument("--horizon-color", type=float, nargs=3, default=None,
                        help="0..1 RGB: the horizon and the ground below it fade into this (the fog's color)")
    return parser.parse_args()


def sample(panorama, directions):
    """The panorama's color seen in every direction (nearest pixel of an equirectangular picture, -Z in its middle)"""
    height, width = panorama.shape[:2]
    x, y, z = directions
    length = numpy.sqrt(x * x + y * y + z * z)
    longitude = numpy.arctan2(x, -z)
    latitude = numpy.arcsin(numpy.clip(y / length, -1.0, 1.0))
    column = ((longitude / (2.0 * numpy.pi) + 0.5) * width).astype(int) % width
    row = numpy.clip(((0.5 - latitude / numpy.pi) * height).astype(int), 0, height - 1)
    return panorama[row, column], y / length


def grade(colors, height_above_horizon, arguments):
    gray = colors.mean(axis=-1, keepdims=True)
    colors = gray + (colors - gray) * arguments.saturation
    colors = colors * arguments.brightness * numpy.array(arguments.tint)

    if arguments.horizon_color is not None:
        # Fully the fog's color at the horizon and below, the sky shows from about 15 degrees up
        sky = numpy.clip(height_above_horizon / 0.25, 0.0, 1.0)[..., None]
        sky = sky * sky * (3.0 - 2.0 * sky)
        colors = numpy.array(arguments.horizon_color) * (1.0 - sky) + colors * sky

    return numpy.clip(colors, 0.0, 1.0)


def main():
    arguments = parse_arguments()
    panorama = numpy.asarray(Image.open(arguments.panorama).convert("RGB"), dtype=numpy.float32) / 255.0
    os.makedirs(arguments.out, exist_ok=True)

    steps = (numpy.arange(arguments.size) + 0.5) / arguments.size * 2.0 - 1.0
    u, v = numpy.meshgrid(steps, steps)
    one = numpy.ones_like(u)

    for name, direction in FACES.items():
        colors, height_above_horizon = sample(panorama, direction(u, v, one))
        face = grade(colors, height_above_horizon, arguments)
        Image.fromarray((face * 255.0 + 0.5).astype(numpy.uint8)).save(os.path.join(arguments.out, name + ".jpg"), quality=92)
        print("wrote", name)


if __name__ == "__main__":
    main()
