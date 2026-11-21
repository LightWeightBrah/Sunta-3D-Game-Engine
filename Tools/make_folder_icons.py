"""Makes the editor's folder (and file) icons for folders that had none, in the style of the hand painted ones:
a leather folder tinted per kind with an emblem (one of the engine's own icons) on its front, a soft shadow under it.
    python Tools/make_folder_icons.py      (writes Sunta/res/Textures/Icons/<name>_icon.png)"""
import colorsys
from pathlib import Path

from PIL import Image, ImageFilter

ICONS = Path(__file__).resolve().parents[1] / "Sunta" / "res" / "Textures" / "Icons"

# name: (base icon, emblem icon, hue shift of the leather (0..1) or None, saturation, brightness, emblem size)
FOLDERS = {
    "skybox_folder":    ("default_folder", "skybox",          0.55, 1.1, 1.0, 0.42),
    "video_folder":     ("default_folder", "camera",          None, 1.0, 0.8, 0.46),
    "ui_folder":        ("default_folder", "console",         0.08, 1.0, 1.05, 0.46),
    "app_folder":       ("default_folder", "souls_bonfire",   0.02, 1.2, 1.0, 0.44),
    "animation_folder": ("default_folder", "animation_clip",  0.75, 0.9, 1.0, 0.46),
    "effects_folder":   ("default_folder", "particle",        0.88, 1.0, 1.0, 0.40),
    "character_folder": ("default_folder", "souls_player",    0.33, 0.8, 0.95, 0.42),
    "props_folder":     ("default_folder", "cube",            0.12, 0.9, 1.0, 0.36),
}
FILES = {
    "video_file": ("default_file", "camera", None, 1.0, 1.0, 0.62),
}


def tinted(image, hue, saturation, brightness):
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            r, g, b, a = pixels[x, y]
            if a == 0:
                continue
            h, s, v = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
            if hue is not None:
                h = hue
            s = min(s * saturation, 1.0)
            v = min(v * brightness, 1.0)
            r2, g2, b2 = colorsys.hsv_to_rgb(h, s, v)
            pixels[x, y] = (int(r2 * 255), int(g2 * 255), int(b2 * 255), a)
    return image


def make(name, base, emblem, hue, saturation, brightness, size, center_y):
    image = tinted(Image.open(ICONS / f"{base}_icon.png").convert("RGBA"), hue, saturation, brightness)
    mark = Image.open(ICONS / f"{emblem}_icon.png").convert("RGBA")
    mark = mark.crop(mark.getbbox())
    scale = size * image.width / max(mark.size)
    mark = mark.resize((max(1, int(mark.width * scale)), max(1, int(mark.height * scale))), Image.LANCZOS)
    left = (image.width - mark.width) // 2
    top = int(image.height * center_y - mark.height / 2)

    # A soft dark shadow under the emblem, as the painted ones have it pressed into the leather
    shadow = Image.new("RGBA", image.size, (0, 0, 0, 0))
    alpha = mark.split()[3].point(lambda value: int(value * 0.55))
    shadow.paste((0, 0, 0, 255), (left + 3, top + 4), alpha)
    image.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(4)))
    image.alpha_composite(mark, (left, top))
    image.save(ICONS / f"{name}_icon.png")
    print("wrote", name)


for name, values in FOLDERS.items():
    make(name, *values, center_y=0.6)
for name, values in FILES.items():
    make(name, *values, center_y=0.58)
