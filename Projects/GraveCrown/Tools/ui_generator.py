"""
Draws Grave Crown's own HUD (res/UI/GraveCrown/): thin bars framed in blackened iron with violet light in their seams,
a crown medallion that fills with the king's wrath, quick slots, the vial of the Grave Draught, the summoned court's
helm, the ember of ash, and writes hud_style.json for the engine (Sunta/src/Sunta/Gameplay/HUDStyle.h)

    python Projects/GraveCrown/Tools/ui_generator.py

Everything is drawn here (Pillow + numpy), four times bigger and scaled down so its edges are smooth. The grain of
the iron is the CC0 photo rust_coarse_01 from Poly Haven (Tools/Blender/Textures)
"""

import json
import math
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.normpath(os.path.join(HERE, "..", "res", "UI", "GraveCrown"))
GRAIN = os.path.normpath(os.path.join(HERE, "..", "..", "..", "Tools", "Blender", "Textures", "rust_coarse_01_diff_1k.jpg"))
SS = 4  # supersampling

IRON = (34, 30, 36)
IRON_LIGHT = (92, 84, 98)
IRON_DARK = (10, 9, 12)
VIOLET = (150, 80, 235)
BONE = (214, 200, 176)


def grain(width, height, strength=0.25):
    """Light and dark spots of the iron photo, around 1"""
    photo = Image.open(GRAIN).convert("L").resize((max(width, 64), max(height, 64)))
    a = np.asarray(photo, dtype=np.float32) / 255.0
    a = (a - a.mean()) * strength + 1.0
    return a[:height, :width]


def finish(image, size):
    return image.resize(size, Image.LANCZOS)


def save(image, name):
    os.makedirs(OUT, exist_ok=True)
    image.save(os.path.join(OUT, name))
    print("UI", name, image.size)


def iron_shape(mask, highlight_mask=None):
    """Colors a mask as blackened iron: grain, a light upper edge, a dark lower one"""
    h, w = mask.shape
    g = grain(w, h, 0.35)
    ys = np.linspace(0.0, 1.0, h)[:, None]
    base = np.array(IRON, dtype=np.float32)[None, None, :] * g[..., None]
    shade = (1.15 - 0.5 * ys)[..., None]
    color = np.clip(base * shade, 0, 255)
    # bevel: the mask's upper edge lit, lower edge dark
    m = Image.fromarray((mask * 255).astype(np.uint8))
    up = np.asarray(m.transform(m.size, Image.AFFINE, (1, 0, 0, 0, 1, 3 * SS)), dtype=np.float32) / 255.0
    down = np.asarray(m.transform(m.size, Image.AFFINE, (1, 0, 0, 0, 1, -3 * SS)), dtype=np.float32) / 255.0
    lit = np.clip(mask - up, 0, 1)
    dark = np.clip(mask - down, 0, 1)
    color = color + lit[..., None] * (np.array(IRON_LIGHT) - color) * 0.9
    color = color + dark[..., None] * (np.array(IRON_DARK) - color) * 0.9
    rgba = np.dstack([color, mask * 255]).astype(np.uint8)
    return Image.fromarray(rgba, "RGBA")


def glow(mask, color, radius, strength=1.0):
    blurred = Image.fromarray((mask * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(radius))
    a = np.asarray(blurred, dtype=np.float32) / 255.0 * strength
    rgba = np.dstack([np.full_like(a, color[0]), np.full_like(a, color[1]), np.full_like(a, color[2]), np.clip(a, 0, 1) * 255])
    return Image.fromarray(rgba.astype(np.uint8), "RGBA")


def layer(*images):
    out = images[0]
    for image in images[1:]:
        out = Image.alpha_composite(out, image)
    return out


def mask_from(draw_fn, width, height):
    image = Image.new("L", (width, height), 0)
    draw_fn(ImageDraw.Draw(image))
    return np.asarray(image, dtype=np.float32) / 255.0


# ========== Bars ==========

def bar_frame(name, width, height, channel_top, channel_bottom, cap_left, cap_right, left_spike=True, chains=False):
    """A thin bar: an iron band around the channel, a pointed left end with a crown spike and a violet gem, a tapered
    right end. Sizes in final pixels; returns the style entry"""
    W, H = width * SS, height * SS
    ct, cb = channel_top * SS, channel_bottom * SS
    band = 3 * SS

    def body(d):
        mid = H // 2
        # the band around the channel
        d.rectangle([cap_left * SS - 4 * SS, ct - band, W - cap_right * SS + 4 * SS, cb + band], fill=255)
        # the left end: a lozenge with a crown of three spikes
        lx = cap_left * SS
        d.polygon([(lx - 2 * SS, ct - band - 2 * SS), (lx * 0.55, ct - band - 2 * SS), (6 * SS, mid), (lx * 0.55, cb + band + 2 * SS),
                   (lx - 2 * SS, cb + band + 2 * SS)], fill=255)
        if left_spike:
            for k, (dx, hgt) in enumerate(((0.30, 0.80), (0.48, 1.0), (0.66, 0.8))):
                x = lx * dx
                d.polygon([(x - 4 * SS, ct - band), (x, ct - band - (ct - band) * hgt * 0.85), (x + 4 * SS, ct - band)], fill=255)
        # the right end: a tapered point
        rx = W - cap_right * SS
        d.polygon([(rx + 2 * SS, ct - band), (W - 4 * SS, mid), (rx + 2 * SS, cb + band)], fill=255)
        if chains:
            for k in range(3):
                x = rx + 8 * SS + k * 9 * SS
                d.ellipse([x, mid - 5 * SS, x + 10 * SS, mid + 5 * SS], outline=255, width=2 * SS)

    m = mask_from(body, W, H)
    hole = mask_from(lambda d: d.rectangle([cap_left * SS, ct, W - cap_right * SS, cb], fill=255), W, H)
    m = np.clip(m - hole, 0, 1)
    gem = mask_from(lambda d: d.ellipse([cap_left * SS * 0.38 - 5 * SS, H // 2 - 5 * SS, cap_left * SS * 0.38 + 5 * SS, H // 2 + 5 * SS], fill=255), W, H)
    seam = mask_from(lambda d: (d.line([(cap_left * SS, ct - 1 * SS), (W - cap_right * SS, ct - 1 * SS)], fill=255, width=SS),
                                d.line([(cap_left * SS, cb + 1 * SS), (W - cap_right * SS, cb + 1 * SS)], fill=255, width=SS)), W, H)
    image = layer(Image.new("RGBA", (W, H), (0, 0, 0, 0)), glow(m, (0, 0, 0), 4 * SS, 0.9), iron_shape(m),
                  glow(seam, VIOLET, 2 * SS, 0.55), glow(gem, VIOLET, 6 * SS, 1.2))
    gem_rgba = np.dstack([np.full_like(gem, 210), np.full_like(gem, 150), np.full_like(gem, 255), gem * 255]).astype(np.uint8)
    image = Image.alpha_composite(image, Image.fromarray(gem_rgba, "RGBA"))
    save(finish(image, (width, height)), name)
    return {"frame": "@game/UI/GraveCrown/" + name, "cap_left": cap_left, "cap_right": cap_right,
            "inner": [cap_left, channel_top, cap_right, height - channel_bottom]}


def bar_fill():
    W, H = 512, 32
    ys = np.linspace(0, 1, H)[:, None]
    shade = 0.78 + 0.45 * np.exp(-((ys - 0.3) / 0.18) ** 2) - 0.25 * ys
    g = grain(W, H, 0.18)
    v = np.clip(shade * g * 230, 0, 255)
    image = Image.fromarray(np.dstack([v, v, v, np.full_like(v, 255)]).astype(np.uint8), "RGBA")
    save(image, "bar_fill.png")


# ========== The crown medallion ==========

def crown(d, cx, cy, size, fill=255):
    """A crown of five broad points standing on a band, a pearl on each point"""
    w = size
    band_top = cy + size * 0.12
    band_bottom = cy + size * 0.36
    d.rectangle([cx - w * 0.5, band_top, cx + w * 0.5, band_bottom], fill=fill)
    heights = (0.42, 0.3, 0.6, 0.3, 0.42)
    step = w / 5
    for k, h in enumerate(heights):
        left = cx - w * 0.5 + step * k
        tip = (left + step * 0.5, cy - size * h)
        d.polygon([(left + step * 0.04, band_top + 1), tip, (left + step * 0.96, band_top + 1)], fill=fill)
        r = size * 0.06
        d.ellipse([tip[0] - r, tip[1] - r * 1.6, tip[0] + r, tip[1] + r * 0.4], fill=fill)


def medallion():
    S = 192
    W = S * SS
    c = W / 2

    def ring(d):
        d.ellipse([6 * SS, 6 * SS, W - 6 * SS, W - 6 * SS], fill=255)
        d.ellipse([22 * SS, 22 * SS, W - 22 * SS, W - 22 * SS], fill=0)
        # four studs and a pointed spike on top
        for a in (45, 135, 225, 315):
            x = c + math.cos(math.radians(a)) * (W / 2 - 14 * SS)
            y = c + math.sin(math.radians(a)) * (W / 2 - 14 * SS)
            d.ellipse([x - 6 * SS, y - 6 * SS, x + 6 * SS, y + 6 * SS], fill=255)

    ring_mask = mask_from(ring, W, W)
    face = mask_from(lambda d: d.ellipse([22 * SS, 22 * SS, W - 22 * SS, W - 22 * SS], fill=255), W, W)
    crown_mask = mask_from(lambda d: crown(d, c, c + 6 * SS, W * 0.46), W, W)
    # dark: an iron ring, a black face, the crown in dull iron
    face_rgba = np.dstack([np.full_like(face, 14), np.full_like(face, 11), np.full_like(face, 18), face * 235]).astype(np.uint8)
    dark = layer(Image.new("RGBA", (W, W), (0, 0, 0, 0)), glow(ring_mask, (0, 0, 0), 6 * SS, 0.8),
                 Image.fromarray(face_rgba, "RGBA"), iron_shape(ring_mask), iron_shape(crown_mask * 0.85))
    save(finish(dark, (S, S)), "emblem.png")
    # lit: the crown burning violet with a halo, the ring's inner edge glowing
    inner_edge = np.clip(mask_from(lambda d: d.ellipse([20 * SS, 20 * SS, W - 20 * SS, W - 20 * SS], outline=255, width=3 * SS), W, W), 0, 1)
    crown_rgba = np.dstack([np.full_like(crown_mask, 225), np.full_like(crown_mask, 175), np.full_like(crown_mask, 255), crown_mask * 255]).astype(np.uint8)
    lit = layer(Image.new("RGBA", (W, W), (0, 0, 0, 0)), Image.fromarray(face_rgba, "RGBA"), iron_shape(ring_mask),
                glow(face, VIOLET, 30 * SS, 0.35), glow(crown_mask, VIOLET, 14 * SS, 1.6), glow(inner_edge, VIOLET, 4 * SS, 0.9),
                Image.fromarray(crown_rgba, "RGBA"))
    save(finish(lit, (S, S)), "emblem_lit.png")


# ========== Slots and icons ==========

def slot_frame():
    S = 128
    W = S * SS

    def frame(d):
        d.rectangle([4 * SS, 4 * SS, W - 4 * SS, W - 4 * SS], fill=255)
        d.rectangle([12 * SS, 12 * SS, W - 12 * SS, W - 12 * SS], fill=0)
        for x, y in ((8, 8), (S - 8, 8), (8, S - 8), (S - 8, S - 8)):
            d.polygon([(x * SS, y * SS - 9 * SS), (x * SS + 9 * SS, y * SS), (x * SS, y * SS + 9 * SS), (x * SS - 9 * SS, y * SS)], fill=255)

    m = mask_from(frame, W, W)
    inside = mask_from(lambda d: d.rectangle([12 * SS, 12 * SS, W - 12 * SS, W - 12 * SS], fill=255), W, W)
    ys = np.linspace(0, 1, W)[:, None] * np.ones((1, W))
    inner = np.dstack([10 + 8 * ys, 8 + 6 * ys, 14 + 10 * ys, inside * 215]).astype(np.uint8)
    seam = mask_from(lambda d: d.rectangle([13 * SS, 13 * SS, W - 13 * SS, W - 13 * SS], outline=255, width=SS), W, W)
    image = layer(Image.new("RGBA", (W, W), (0, 0, 0, 0)), glow(m, (0, 0, 0), 4 * SS, 0.8), Image.fromarray(inner, "RGBA"),
                  glow(seam, VIOLET, 2 * SS, 0.45), iron_shape(m))
    save(finish(image, (S, S)), "slot_frame.png")


def vial_icon():
    S = 160
    W = S * SS
    c = W / 2

    def glass(d):
        d.ellipse([c - 46 * SS, 70 * SS, c + 46 * SS, 152 * SS], fill=255)
        d.rectangle([c - 13 * SS, 34 * SS, c + 13 * SS, 84 * SS], fill=255)

    g = mask_from(glass, W, W)
    liquid = mask_from(lambda d: d.ellipse([c - 40 * SS, 84 * SS, c + 40 * SS, 146 * SS], fill=255), W, W) * \
        mask_from(lambda d: d.rectangle([0, 98 * SS, W, W], fill=255), W, W)
    stopper = mask_from(lambda d: (d.rectangle([c - 17 * SS, 24 * SS, c + 17 * SS, 38 * SS], fill=255), crown(d, c, 14 * SS, 40 * SS)), W, W)
    glass_rgba = np.dstack([np.full_like(g, 120), np.full_like(g, 110), np.full_like(g, 140), g * 70]).astype(np.uint8)
    rim = np.clip(g - mask_from(lambda d: (d.ellipse([c - 42 * SS, 74 * SS, c + 42 * SS, 148 * SS], fill=255),
                                           d.rectangle([c - 9 * SS, 38 * SS, c + 9 * SS, 86 * SS], fill=255)), W, W), 0, 1)
    liquid_rgba = np.dstack([np.full_like(liquid, 170), np.full_like(liquid, 90), np.full_like(liquid, 245), liquid * 245]).astype(np.uint8)
    shine = mask_from(lambda d: d.ellipse([c - 30 * SS, 88 * SS, c - 18 * SS, 116 * SS], fill=255), W, W)
    image = layer(Image.new("RGBA", (W, W), (0, 0, 0, 0)), glow(liquid, VIOLET, 18 * SS, 1.1), Image.fromarray(glass_rgba, "RGBA"),
                  Image.fromarray(liquid_rgba, "RGBA"), glow(rim, (200, 190, 215), SS, 1.0), iron_shape(stopper),
                  glow(shine, (255, 255, 255), 2 * SS, 0.9))
    save(finish(image, (S, S)), "icon_vial.png")


def helm_icon():
    S = 160
    W = S * SS
    c = W / 2

    def helm(d):
        d.ellipse([c - 44 * SS, 22 * SS, c + 44 * SS, 110 * SS], fill=255)
        d.rectangle([c - 44 * SS, 66 * SS, c + 44 * SS, 136 * SS], fill=255)
        d.polygon([(c - 44 * SS, 136 * SS), (c, 152 * SS), (c + 44 * SS, 136 * SS)], fill=255)
        d.polygon([(c - 3 * SS, 8 * SS), (c + 3 * SS, 8 * SS), (c + 6 * SS, 40 * SS), (c - 6 * SS, 40 * SS)], fill=255)

    m = mask_from(helm, W, W)
    slits = mask_from(lambda d: (d.rectangle([c - 34 * SS, 78 * SS, c - 6 * SS, 86 * SS], fill=255),
                                 d.rectangle([c + 6 * SS, 78 * SS, c + 34 * SS, 86 * SS], fill=255),
                                 d.rectangle([c - 3 * SS, 86 * SS, c + 3 * SS, 128 * SS], fill=255)), W, W)
    body = np.clip(m - slits, 0, 1)
    flame = mask_from(lambda d: [d.ellipse([c - 30 * SS + k * 12 * SS, 60 * SS - (k % 2) * 10 * SS, c - 18 * SS + k * 12 * SS, 88 * SS], fill=255)
                                 for k in range(5)], W, W)
    slit_rgba = np.dstack([np.full_like(slits, 235), np.full_like(slits, 190), np.full_like(slits, 255), slits * 255]).astype(np.uint8)
    image = layer(Image.new("RGBA", (W, W), (0, 0, 0, 0)), glow(flame, VIOLET, 14 * SS, 0.9), iron_shape(body),
                  glow(slits, VIOLET, 6 * SS, 1.5), Image.fromarray(slit_rgba, "RGBA"))
    save(finish(image, (S, S)), "icon_court.png")


def ash_icon():
    S = 160
    W = S * SS
    c = W / 2
    rng = np.random.default_rng(5)

    def ember(d):
        # a lump of ash: overlapping rounded pieces
        for k in range(9):
            a = rng.uniform(0, 2 * math.pi)
            r = rng.uniform(0, 22) * SS
            x, y = c + math.cos(a) * r, c + 12 * SS + math.sin(a) * r * 0.7
            rad = rng.uniform(22, 34) * SS
            d.ellipse([x - rad, y - rad * 0.8, x + rad, y + rad * 0.8], fill=255)

    m = np.asarray(Image.fromarray((mask_from(ember, W, W) * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(2 * SS)), dtype=np.float32) / 255.0
    m = np.clip((m - 0.35) * 3.0, 0, 1)
    core = mask_from(lambda d: d.ellipse([c - 16 * SS, c, c + 16 * SS, c + 26 * SS], fill=255), W, W)
    cracks = mask_from(lambda d: [d.line([(c, c + 12 * SS), (c + math.cos(a) * 34 * SS, c + 12 * SS + math.sin(a) * 26 * SS)], fill=255, width=2 * SS)
                                  for a in (0.4, 1.9, 3.3, 4.6, 5.7)], W, W) * m
    h, w = m.shape
    g = grain(w, h, 0.7)
    ys = np.linspace(0, 1, h)[:, None]
    tone = (1.25 - 0.6 * ys) * g
    ash = np.dstack([92 * tone, 86 * tone, 94 * tone, m * 255])
    hot = np.clip(core * 0.9 + cracks, 0, 1)
    hot_rgba = np.dstack([np.full_like(hot, 225), np.full_like(hot, 165), np.full_like(hot, 255), hot * 220])
    image = layer(Image.new("RGBA", (W, W), (0, 0, 0, 0)), glow(m, (0, 0, 0), 5 * SS, 0.8), glow(core, VIOLET, 26 * SS, 0.8),
                  Image.fromarray(np.clip(ash, 0, 255).astype(np.uint8), "RGBA"), glow(hot, VIOLET, 6 * SS, 1.3),
                  Image.fromarray(hot_rgba.astype(np.uint8), "RGBA"))
    save(finish(image, (S, S)), "icon_ash.png")


def lock_on_mark():
    """A small bright pearl in a thin violet ring, softly glowing"""
    S = 64
    W = S * SS
    c = W / 2
    pearl = mask_from(lambda d: d.ellipse([c - 5 * SS, c - 5 * SS, c + 5 * SS, c + 5 * SS], fill=255), W, W)
    ring = mask_from(lambda d: d.ellipse([c - 16 * SS, c - 16 * SS, c + 16 * SS, c + 16 * SS], outline=255, width=2 * SS), W, W)
    pearl_rgba = np.dstack([np.full_like(pearl, 250), np.full_like(pearl, 240), np.full_like(pearl, 255), pearl * 255]).astype(np.uint8)
    ring_rgba = np.dstack([np.full_like(ring, 200), np.full_like(ring, 150), np.full_like(ring, 255), ring * 200]).astype(np.uint8)
    image = layer(Image.new("RGBA", (W, W), (0, 0, 0, 0)), glow(pearl, (0, 0, 0), 4 * SS, 0.7), glow(pearl, VIOLET, 8 * SS, 1.2),
                  Image.fromarray(ring_rgba, "RGBA"), Image.fromarray(pearl_rgba, "RGBA"))
    save(finish(image, (S, S)), "lock_on.png")


def main():
    health = bar_frame("health_frame.png", 420, 48, 18, 32, 70, 26)
    stamina = bar_frame("stamina_frame.png", 380, 36, 14, 22, 46, 22, left_spike=False)
    wrath = bar_frame("wrath_frame.png", 380, 32, 13, 19, 46, 22, left_spike=False)
    boss = bar_frame("boss_frame.png", 640, 44, 18, 30, 60, 60, left_spike=False, chains=True)
    bar_fill()
    medallion()
    slot_frame()
    vial_icon()
    helm_icon()
    ash_icon()
    lock_on_mark()

    # The style: the medallion at the top left, the bars beside it
    emblem = 96
    left = 24 + emblem - 18
    style = {
        "font": "@engine/Fonts/Crimson_Text/CrimsonText-Bold.ttf",
        "title_spacing": 0.14,
        "health": dict(health, height=40, offset=[left, 26]),
        "stamina": dict(stamina, height=30, offset=[left + 14, 64]),
        "wrath": dict(wrath, height=26, offset=[left + 14, 90]),
        "boss": dict(boss, height=38, offset=[0, 0]),
        "fill": "@game/UI/GraveCrown/bar_fill.png",
        "health_color": [0.58, 0.05, 0.09],
        "stamina_color": [0.62, 0.58, 0.42],
        "wrath_color": [0.6, 0.28, 0.95],
        "trail_color": [0.85, 0.72, 0.55],
        "channel_color": [0.02, 0.015, 0.025, 0.92],
        "emblem": "@game/UI/GraveCrown/emblem.png",
        "emblem_lit": "@game/UI/GraveCrown/emblem_lit.png",
        "emblem_size": [emblem, emblem],
        "emblem_offset": [22, 18],
        "slot_frame": "@game/UI/GraveCrown/slot_frame.png",
        "slot_size": [82, 82],
        "slot_gap": 12,
        "flask_icon": "@game/UI/GraveCrown/icon_vial.png",
        "summon_icon": "@game/UI/GraveCrown/icon_court.png",
        "currency_icon": "@game/UI/GraveCrown/icon_ash.png",
        "icon_height": 84,
        "currency_plain": True,
        "boss_name_centered": True,
        "posture_mark": True,
        "boss_width": 0.46,
        "lock_on_mark": "@game/UI/GraveCrown/lock_on.png",
        "lock_on_size": 30,
    }
    with open(os.path.join(OUT, "hud_style.json"), "w", encoding="utf-8") as out:
        json.dump(style, out, indent=2)
    print("UI hud_style.json")


if __name__ == "__main__":
    main()
