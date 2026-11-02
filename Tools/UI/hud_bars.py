"""Draws the bar frames of the HUD as blackened forged iron (no gems, no swords): bevelled rails, closed gothic end plates
with spikes and rivets, the grain of the iron from the CC0 photo rust_coarse_01 (Poly Haven), greyed and darkened.
Every piece of a bar has the same height, so the engine only stretches the middle horizontally (three-slice).
With a game frame and an output path it also builds a mockup the way the engine draws it.

Run after hud_icons_and_panel.py: python Tools/UI/hud_bars.py [frame.png mockup.png]   (writes into Tools/UI/Generated,
then Tools/UI/hud_export.py puts it in the game)"""
import os
import sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, 'Generated')
GRAIN_PHOTO = os.path.join(HERE, '..', 'Blender', 'Textures', 'rust_coarse_01_diff_1k.jpg')
os.makedirs(OUT, exist_ok=True)

SUPER = 4  # drawn this many times larger, then scaled down (smooth edges)

# The layout of every piece (in texture pixels): the rails of the frame and the channel between them
HEIGHT = 180
RAIL_TOP, RAIL_BOTTOM = 40, 140          # the outer edge of the rails
INNER_TOP, INNER_BOTTOM = 56, 124        # the channel the fill goes into

IRON = np.array([0.155, 0.150, 0.145])
IRON_LIGHT = np.array([0.62, 0.60, 0.56])
IRON_DARK = np.array([0.03, 0.03, 0.03])

grain_photo = Image.open(GRAIN_PHOTO).convert('L')


def grain(width, height, seed):
    """The iron's surface: a piece of the rust photo, without color, flattened around 1"""
    rng = np.random.default_rng(seed)
    x = int(rng.integers(0, grain_photo.width - 256))
    y = int(rng.integers(0, grain_photo.height - 256))
    tile = grain_photo.crop((x, y, x + 256, y + 256)).resize((width, height), Image.BICUBIC)
    g = np.asarray(tile).astype(np.float32) / 255.0
    return 0.75 + 0.5 * (g - g.mean())


def shade(mask, seed, light=(-0.6, -0.8)):
    """Forged iron of this shape: raised (bevelled) edges lit from the top left, darker in the hollows, a dark outline"""
    m = mask.astype(np.float32)
    height = np.asarray(Image.fromarray((m * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(3 * SUPER))).astype(np.float32) / 255.0
    gy, gx = np.gradient(height)
    lit = -(gx * light[0] + gy * light[1]) * 6.0 * SUPER
    h, w = m.shape
    surface = grain(w, h, seed)

    color = IRON[None, None, :] * surface[..., None]
    color = color + np.clip(lit, 0, 1)[..., None] * (IRON_LIGHT - IRON)[None, None, :]
    color = color + np.clip(lit, -1, 0)[..., None] * (IRON - IRON_DARK)[None, None, :]

    # A thin dark outline around the shape
    inner = np.asarray(Image.fromarray((m * 255).astype(np.uint8)).filter(ImageFilter.MinFilter(1 + 2 * SUPER))).astype(np.float32) / 255.0
    color = np.where((m > 0.5)[..., None] & (inner < 0.5)[..., None], IRON_DARK[None, None, :] * 1.5, color)
    return np.dstack([np.clip(color, 0, 1), m])


def canvas(width):
    return Image.new('L', (width * SUPER, HEIGHT * SUPER), 0)


def S(points):
    return [(x * SUPER, y * SUPER) for x, y in points]


def rails(draw, x0, x1):
    draw.rectangle(S([(x0, RAIL_TOP), (x1, INNER_TOP)]), fill=255)
    draw.rectangle(S([(x0, INNER_BOTTOM), (x1, RAIL_BOTTOM)]), fill=255)


def rivet(draw, x, y, r=4.5):
    draw.ellipse(S([(x - r, y - r), (x + r, y + r)]), fill=255)


def finish(image, seed, rivets=()):
    mask = np.asarray(image).astype(np.float32) / 255.0
    rgba = shade(mask, seed)
    # Rivets: a separate raised bump each, so they read as heads on the plate
    for (x, y, r) in rivets:
        bump = Image.new('L', image.size, 0)
        ImageDraw.Draw(bump).ellipse(S([(x - r, y - r), (x + r, y + r)]), fill=255)
        b = np.asarray(bump).astype(np.float32) / 255.0
        head = shade(b, seed + 99)
        head[..., :3] *= 1.25
        rgba = np.where(b[..., None] > 0.5, head, rgba)
    small = Image.fromarray((np.clip(rgba, 0, 1) * 255).astype(np.uint8), 'RGBA').resize((image.width // SUPER, HEIGHT), Image.LANCZOS)
    return small


def middle(width=256, seed=1):
    image = canvas(width)
    rails(ImageDraw.Draw(image), 0, width)
    return finish(image, seed)


def end_cap(width, is_heavy, seed):
    """The left end of a bar (the right end is this mirrored): the rails end in a closed plate with spikes up and down
    and a point to the side. The heavy one (health, boss) is taller with a notched gothic outline"""
    image = canvas(width)
    draw = ImageDraw.Draw(image)
    plate_x0, plate_x1 = width - 62, width - 34
    rails(draw, plate_x1, width)

    top = 6 if is_heavy else 22
    bottom = HEIGHT - top
    mid = HEIGHT / 2
    # The plate with a spike up and down
    draw.polygon(S([(plate_x0, RAIL_TOP - 8), ((plate_x0 + plate_x1) / 2, top), (plate_x1, RAIL_TOP - 8),
                    (plate_x1, RAIL_BOTTOM + 8), ((plate_x0 + plate_x1) / 2, bottom), (plate_x0, RAIL_BOTTOM + 8)]), fill=255)
    # The point to the side, notched like a gothic finial
    tip = 4 if is_heavy else 16
    draw.polygon(S([(plate_x0 + 2, mid - 30), (plate_x0 - 22, mid - 30), (plate_x0 - 14, mid - 16), (tip, mid),
                    (plate_x0 - 14, mid + 16), (plate_x0 - 22, mid + 30), (plate_x0 + 2, mid + 30)]), fill=255)
    if is_heavy:
        # Side wings on the plate
        draw.polygon(S([(plate_x0, RAIL_TOP - 2), (plate_x0 - 18, RAIL_TOP - 20), (plate_x0 - 6, RAIL_TOP + 14)]), fill=255)
        draw.polygon(S([(plate_x0, RAIL_BOTTOM + 2), (plate_x0 - 18, RAIL_BOTTOM + 20), (plate_x0 - 6, RAIL_BOTTOM - 14)]), fill=255)

    cx = (plate_x0 + plate_x1) / 2
    rivets = [(cx, RAIL_TOP + 2, 5.5), (cx, RAIL_BOTTOM - 2, 5.5), (cx, mid, 6.5)]
    return finish(image, seed, rivets)


def crest(width=160, seed=7):
    """The ornament on top of the boss bar: a pointed gothic arch of iron sitting on the upper rail"""
    image = canvas(width)
    draw = ImageDraw.Draw(image)
    c = width / 2
    draw.polygon(S([(c - 52, RAIL_TOP + 6), (c - 30, RAIL_TOP - 10), (c - 16, 6), (c, RAIL_TOP - 22), (c + 16, 6),
                    (c + 30, RAIL_TOP - 10), (c + 52, RAIL_TOP + 6)]), fill=255)
    draw.polygon(S([(c - 22, RAIL_BOTTOM - 4), (c, HEIGHT - 10), (c + 22, RAIL_BOTTOM - 4)]), fill=255)
    rails(draw, 0, width)
    return finish(image, seed, [(c, RAIL_TOP - 6, 6.5)])


def save(image, name):
    image.save(os.path.join(OUT, name))
    return image


heavy_left = save(end_cap(150, True, 3), 'bar_health_left.png')
light_left = end_cap(120, False, 4)
save(light_left, 'bar_left.png')
save(light_left.transpose(Image.FLIP_LEFT_RIGHT), 'bar_right.png')
bar_middle = save(middle(256, 1), 'bar_middle.png')

boss_left = save(end_cap(150, True, 5), 'boss_bar_left.png')
save(boss_left.transpose(Image.FLIP_LEFT_RIGHT), 'boss_bar_right.png')
boss_middle = save(middle(256, 6), 'boss_bar_middle.png')
boss_crest = save(crest(), 'boss_bar_crest.png')
print('bars in', OUT)


# ========== The mockup ==========

def three_slice(base, left, mid, right, x, y, width, height):
    scale = height / HEIGHT
    lw, rw = round(left.width * scale), round(right.width * scale)
    base.alpha_composite(left.resize((lw, height), Image.LANCZOS), (x, y))
    base.alpha_composite(mid.resize((max(width - lw - rw, 1), height), Image.LANCZOS), (x + lw, y))
    base.alpha_composite(right.resize((rw, height), Image.LANCZOS), (x + width - rw, y))
    return lw, rw


fill_texture = Image.open(os.path.join(OUT, 'bar_fill.png')).convert('RGBA')


def bar(base, left, mid, right, x, y, width, height, fraction, color, crest_image=None):
    """Like the engine: the dark channel, the fill, then the frame over them"""
    scale = height / HEIGHT
    lw, rw = round(left.width * scale), round(right.width * scale)
    x0, x1 = x + lw - round(30 * scale), x + width - rw + round(30 * scale)
    y0, y1 = y + round(INNER_TOP * scale), y + round(INNER_BOTTOM * scale)
    ImageDraw.Draw(base).rectangle((x0, y0, x1, y1), fill=(6, 5, 5, 240))
    w = max(int((x1 - x0) * fraction), 1)
    arr = np.asarray(fill_texture.resize((w, y1 - y0), Image.LANCZOS)).astype(np.float32) / 255.0
    arr[..., :3] *= np.array(color)
    base.alpha_composite(Image.fromarray((arr * 255).astype(np.uint8), 'RGBA'), (x0, y0))
    three_slice(base, left, mid, right, x, y, width, height)
    if crest_image:
        cw = round(crest_image.width * scale)
        base.alpha_composite(crest_image.resize((cw, height), Image.LANCZOS), (x + width // 2 - cw // 2, y))


def mockup(frame_path, out_path):
    base = Image.open(frame_path).convert('RGBA')
    draw = ImageDraw.Draw(base)
    font = ImageFont.truetype('georgia.ttf', 24)
    small = ImageFont.truetype('georgia.ttf', 17)
    big = ImageFont.truetype('georgia.ttf', 26)
    RED, GREEN, BOSS = (0.62, 0.07, 0.05), (0.24, 0.48, 0.17), (0.62, 0.07, 0.05)

    light_right = light_left.transpose(Image.FLIP_LEFT_RIGHT)
    bar(base, light_left, bar_middle, light_right, 46, 64, 330, 54, 1.0, GREEN)
    bar(base, heavy_left, bar_middle, light_right, 22, 12, 400, 72, 0.8, RED)

    bar(base, boss_left, boss_middle, boss_left.transpose(Image.FLIP_LEFT_RIGHT), 300, 772, 1000, 66, 0.65, BOSS, boss_crest)
    draw.text((340, 750), 'Warden of the Ashen Gate', font=big, fill=(232, 222, 202, 255))

    panel = Image.open(os.path.join(OUT, 'panel.png'))
    for corner, icon, label, value in (('left', 'icon_flask.png', 'Estus Flask', '5'), ('right', 'icon_soul.png', 'Souls', '1000')):
        w, h = 210, 78
        x = 30 if corner == 'left' else 1600 - 30 - w
        y = 900 - 26 - h
        base.alpha_composite(panel.resize((w, h), Image.LANCZOS), (x, y))
        ic = Image.open(os.path.join(OUT, icon))
        ih = 64
        iw = round(ic.width / ic.height * ih)
        base.alpha_composite(ic.resize((iw, ih), Image.LANCZOS), (x + 16, y + (h - ih) // 2))
        draw.text((x + 26 + iw, y + 14), label, font=small, fill=(190, 176, 150, 255))
        draw.text((x + 26 + iw, y + 36), value, font=font, fill=(240, 230, 210, 255))

    base.convert('RGB').save(out_path)
    base.crop((0, 0, 480, 140)).resize((1440, 420), Image.LANCZOS).convert('RGB').save(out_path.replace('.png', '_zoom.png'))


if len(sys.argv) > 2:
    mockup(sys.argv[1], sys.argv[2])
