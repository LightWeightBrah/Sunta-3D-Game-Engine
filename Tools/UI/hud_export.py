"""Puts the HUD pictures into the game: one frame picture per bar (left end | middle | right end), the fill, the panel,
the icons, and the style file that tells the engine how to cut and place them.

Run after hud_icons_and_panel.py and hud_bars.py: python Tools/UI/hud_export.py"""
import json
import os
import shutil
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SOURCE = os.path.join(HERE, 'Generated')
GAME = os.path.join(HERE, '..', '..', 'Projects', 'MyGame', 'res', 'UI', 'HUD')
os.makedirs(GAME, exist_ok=True)

# The layout of every bar piece (see hud_bars.py): 180 rows, the channel between rows 56 and 124
HEIGHT, INNER_TOP, INNER_BOTTOM = 180, 56, 124
# Where the channel begins in the pieces: after the plate of an end (heavy end 150 wide, light end 120 wide)
HEAVY_INNER, LIGHT_INNER = 116, 86


def load(name):
    return Image.open(os.path.join(SOURCE, name)).convert('RGBA')


def join(left, middle, right, name):
    frame = Image.new('RGBA', (left.width + middle.width + right.width, HEIGHT), (0, 0, 0, 0))
    frame.paste(left, (0, 0))
    frame.paste(middle, (left.width, 0))
    frame.paste(right, (left.width + middle.width, 0))
    frame.save(os.path.join(GAME, name))
    return left.width, right.width


light_left = load('bar_left.png')
light_right = light_left.transpose(Image.FLIP_LEFT_RIGHT)
heavy_left = load('bar_health_left.png')
middle = load('bar_middle.png')
boss_left = load('boss_bar_left.png')

health_caps = join(heavy_left, middle, light_right, 'health_frame.png')
stamina_caps = join(light_left, middle, light_right, 'stamina_frame.png')
boss_caps = join(boss_left, load('boss_bar_middle.png'), boss_left.transpose(Image.FLIP_LEFT_RIGHT), 'boss_frame.png')

for name, target in (('boss_bar_crest.png', 'boss_crest.png'), ('bar_fill.png', 'bar_fill.png'), ('panel.png', 'panel.png'),
                     ('icon_flask.png', 'icon_flask.png'), ('icon_soul.png', 'icon_soul.png')):
    shutil.copy(os.path.join(SOURCE, name), os.path.join(GAME, target))


def bar(frame, caps, inner_left, inner_right, height, offset, ornament=None):
    data = {
        'frame': f'@game/UI/HUD/{frame}',
        'cap_left': caps[0],
        'cap_right': caps[1],
        'inner': [inner_left, INNER_TOP, inner_right, HEIGHT - INNER_BOTTOM],
        'height': height,
        'offset': offset,
    }
    if ornament:
        data['ornament'] = f'@game/UI/HUD/{ornament}'
    return data


style = {
    'health': bar('health_frame.png', health_caps, HEAVY_INNER, LIGHT_INNER, 72, [22, 12]),
    'stamina': bar('stamina_frame.png', stamina_caps, LIGHT_INNER, LIGHT_INNER, 54, [46, 64]),
    'boss': bar('boss_frame.png', boss_caps, HEAVY_INNER, HEAVY_INNER, 66, [0, 0], 'boss_crest.png'),
    'fill': '@game/UI/HUD/bar_fill.png',
    'health_color': [0.62, 0.07, 0.05],
    'stamina_color': [0.24, 0.48, 0.17],
    'trail_color': [0.85, 0.70, 0.40],
    'channel_color': [0.025, 0.02, 0.02, 0.94],
    'panel': '@game/UI/HUD/panel.png',
    'panel_border': 24,
    'corner_size': [210, 78],
    'icon_height': 64,
    'flask_icon': '@game/UI/HUD/icon_flask.png',
    'currency_icon': '@game/UI/HUD/icon_soul.png',
}

with open(os.path.join(GAME, 'hud_style.json'), 'w', encoding='utf-8', newline='\n') as file:
    json.dump(style, file, indent=4)
    file.write('\n')

print(sorted(os.listdir(GAME)))
