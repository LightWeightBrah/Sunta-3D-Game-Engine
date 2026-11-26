"""The violet fire in the cracks of the dead king's armor (and other glowing parts) shines by itself: the parts of a
model's color texture of one hue, bright and saturated, become its *_emissive texture (the engine adds it on top of the
lit color, scaled by the Model's emissiveStrength). Plain Python with Pillow and numpy, after characters_generator.py
    python Projects/GraveCrown/Tools/glow_maps.py"""
import os
import numpy
from PIL import Image

MODELS = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "res", "Models", "Characters"))

# folder/name: hues (degrees) that glow
GLOWS = {"GraveKing/grave_king": (250.0, 310.0)}


def make_glow(path, hue_from, hue_to):
    picture = Image.open(path + "_diffuse.png").convert("RGB")
    color = numpy.asarray(picture, dtype=numpy.float32) / 255.0
    hsv = numpy.asarray(picture.convert("HSV"), dtype=numpy.float32) / 255.0
    hue, saturation, value = hsv[..., 0] * 360.0, hsv[..., 1], hsv[..., 2]
    mask = (hue >= hue_from) & (hue <= hue_to) & (saturation > 0.3) & (value > 0.22)
    strength = numpy.clip((value - 0.22) / 0.3, 0.0, 1.0) * mask
    glow = numpy.clip(color * (2.4 * strength[..., None]), 0.0, 1.0)
    Image.fromarray((glow * 255).astype(numpy.uint8)).save(path + "_emissive.png")
    print("GLOW {}: {:.2%} of the texture shines".format(os.path.basename(path), float(mask.mean())))


for name, hues in GLOWS.items():
    make_glow(os.path.join(MODELS, name), *hues)
