"""The corner icons of Strongest in History Vs Today (the flask is champagne, the currency is Clout), cut out of their
generated pictures on black (Z-Image-Turbo, Tools/UI/Concepts/sih_icon_*.webp) the same way as the sample game's ones.
Run: python Tools/UI/sih_hud_icons.py   (writes into Projects/StrongestInHistoryVsToday/res/UI/HUD)"""
import os

import numpy as np
from PIL import Image

from hud_icons_and_panel import to_rgba

HERE = os.path.dirname(os.path.abspath(__file__))
HUD = os.path.join(HERE, '..', '..', 'Projects', 'StrongestInHistoryVsToday', 'res', 'UI', 'HUD')

# source picture, file in the game, black level, gain, height of the icon (pixels)
ICONS = [('sih_icon_champagne.webp', 'icon_champagne.png', 0.10, 3.0, 670),
         ('sih_icon_clout.webp', 'icon_clout.png', 0.08, 3.0, 560)]

for source, target, black_level, gain, height in ICONS:
    rgb = np.asarray(Image.open(os.path.join(HERE, 'Concepts', source)).convert('RGB')).astype(np.float32) / 255.0
    image = Image.fromarray((to_rgba(rgb, black_level, gain) * 255).astype(np.uint8), 'RGBA')
    image = image.crop(image.getchannel('A').point(lambda a: 255 if a > 8 else 0).getbbox())
    image = image.resize((max(1, round(image.width * height / image.height)), height), Image.LANCZOS)
    image.save(os.path.join(HUD, target))
    print('wrote', target, image.size)
