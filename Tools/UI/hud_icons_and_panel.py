"""The HUD pictures that are not bars: the flask and soul icons (cut out of a generated sheet on a black background),
the grey fill of the bars (tinted by the engine) and the smoky panel of the corners (stretched by nine-slice).

Run: python Tools/UI/hud_icons_and_panel.py   (writes into Tools/UI/Generated, then Tools/UI/hud_export.py puts it in the game)"""
import os
import numpy as np
from PIL import Image, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, 'Generated')
os.makedirs(OUT, exist_ok=True)

ICON_SHEET = os.path.join(HERE, 'Concepts', 'hud_icons_sheet.webp')  # Z-Image-Turbo: a flask and a soul flame on black


def to_rgba(rgb, black_level, gain):
    """Black background -> transparent, the dark fringe brightened back so edges don't look grey over the game"""
    alpha = np.clip((rgb.max(axis=2) - black_level) * gain, 0.0, 1.0)
    color = np.clip(rgb / np.maximum(alpha[..., None], 0.35), 0.0, 1.0)
    return np.dstack([color, alpha])


def save(rgba, name):
    Image.fromarray((np.clip(rgba, 0, 1) * 255).astype(np.uint8), 'RGBA').save(os.path.join(OUT, name))


def noise(rng, height, width, blur):
    values = rng.normal(0.0, 1.0, (height, width))
    picture = Image.fromarray(((values - values.min()) / np.ptp(values) * 255).astype(np.uint8))
    return picture.filter(ImageFilter.GaussianBlur(blur))


# ========== Icons ==========

sheet = np.asarray(Image.open(ICON_SHEET).convert('RGB')).astype(np.float32) / 255.0
save(to_rgba(sheet[100:770, 190:610], 0.04, 4.0), 'icon_flask.png')
save(to_rgba(sheet[70:770, 880:1350], 0.04, 4.0), 'icon_soul.png')

rng = np.random.default_rng(7)

# ========== The fill of the bars: grey (the engine tints it), a bright top edge, a darker bottom, faint streaks ==========

h, w = 64, 512
y = np.linspace(0.0, 1.0, h)[:, None]
shade = 0.62 + 0.38 * np.exp(-((y - 0.18) / 0.16) ** 2) - 0.28 * y
streaks = np.asarray(Image.fromarray(np.repeat(np.asarray(noise(rng, h, 1, 0)), w, axis=1)).filter(ImageFilter.GaussianBlur(1.2))) / 255.0
grain = np.asarray(noise(rng, h, w, 2)) / 255.0
gray = np.clip(shade * (0.88 + 0.12 * streaks) * (0.9 + 0.2 * grain), 0, 1)
gray[:2] *= 0.6
gray[-3:] *= 0.5
save(np.dstack([gray, gray, gray, np.ones_like(gray)]), 'bar_fill.png')

# ========== The panel of the corners: dark smoke with soft edges, a thin gold line along the top and the bottom ==========

h, w = 128, 384
yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
edge = np.minimum.reduce([xx / 40.0, (w - 1 - xx) / 40.0, yy / 22.0, (h - 1 - yy) / 22.0])
alpha = np.clip(edge, 0, 1) ** 1.5 * 0.78
smoke = np.asarray(noise(rng, h // 8, w // 8, 0).resize((w, h), Image.BICUBIC)) / 255.0
panel = np.dstack([0.05 + 0.03 * smoke, 0.045 + 0.025 * smoke, 0.04 + 0.02 * smoke, alpha])
line = np.clip(1.0 - np.abs(xx - w / 2) / (w / 2), 0, 1) ** 0.6
for row in (6, h - 7):
    panel[row, :, :3] = np.array([0.72, 0.58, 0.32])
    panel[row, :, 3] = 0.9 * line[row]
save(panel, 'panel.png')

print('icons, fill and panel in', OUT)
