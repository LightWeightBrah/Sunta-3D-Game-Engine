"""
The ground of the ash plains: one function of (x, z) used three ways - the 16-bit heightmap the engine's Terrain
component reads, the terrain's mesh (terrain_generator.py, Blender) and the height of everything build_scene.py places
on it. Gentle hills, a flat old road south from the crypt's gate, flat ground around the camps, the plains bonfire and
the Chain Warden's arena, mountains rising at the edges (the world's natural walls)

    python Projects/GraveCrown/Tools/terrain_map.py      # writes res/Terrain/plains_height.png
"""

import math
import os

# (reaching 40 m south of the Warden's court, so its mountains rise behind the court's walls, not inside it)
CENTER_X, CENTER_Z = 0.0, -105.0
SIZE_X, SIZE_Z = 200.0, 220.0
HEIGHT_SCALE = 26.0
PIXELS_X, PIXELS_Z = 401, 441

# Flat places: (x, z, radius of the flat middle, radius where the hills are back)
FLAT = [(0.0, -146.0, 30.0, 40.0),     # the arena
        (-10.0, -48.0, 6.0, 14.0),     # the plains bonfire
        (14.0, -28.0, 8.0, 16.0), (-18.0, -78.0, 8.0, 16.0), (16.0, -96.0, 8.0, 16.0)]  # camps
ROAD_HALF_WIDTH, ROAD_EDGE = 5.0, 12.0
CRYPT_APRON_Z = -6.0                   # flat in front of the crypt's walls (north of this)


def smooth(edge0, edge1, value):
    t = min(max((value - edge0) / (edge1 - edge0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


def noise(x, z):
    """Rolling hills: a few waves of different sizes and directions (no randomness: the same map every time)"""
    h = 0.0
    for size, amount, angle in ((47.0, 1.0, 0.3), (23.0, 0.55, 1.4), (11.0, 0.25, 2.6), (71.0, 0.8, 4.1)):
        dx, dz = math.cos(angle), math.sin(angle)
        h += amount * math.sin((x * dx + z * dz) / size * 2.0 * math.pi + angle) * math.cos((x * dz - z * dx) / size * 1.7)
    return h


def height(x, z):
    """Meters above the plains' floor at (x, z)"""
    hills = 1.6 + 1.4 * noise(x, z)                  # 0.. ~4 m
    # Mountains at the edges of the map
    side_edge = smooth(58.0, 95.0, abs(x - CENTER_X))
    far_edge = smooth(SIZE_Z * 0.5 - 30.0, SIZE_Z * 0.5 + 5.0, abs(z - CENTER_Z))
    if z > CENTER_Z:
        # The north edge is the crypt's side: rock beside its walls, an open way out of its gate
        far_edge *= smooth(16.0, 30.0, abs(x))
    edge = max(side_edge, far_edge)
    mountains = edge * (16.0 + 6.0 * noise(x * 1.7 + 30.0, z * 1.3))
    flatten = 1.0
    flatten = min(flatten, smooth(ROAD_HALF_WIDTH, ROAD_EDGE, abs(x)))
    if z > CRYPT_APRON_Z - 10.0:
        flatten = min(flatten, smooth(0.0, 10.0, CRYPT_APRON_Z - z))
    for fx, fz, inner, outer in FLAT:
        flatten = min(flatten, smooth(inner, outer, math.hypot(x - fx, z - fz)))
    # Mountains stay even where the ground is flattened near the edges (the arena's back wall of rock)
    return max(0.0, hills * flatten + mountains * max(flatten, edge))


def write_heightmap(path):
    import numpy
    from PIL import Image
    pixels = numpy.zeros((PIXELS_Z, PIXELS_X), dtype=numpy.uint16)
    for row in range(PIXELS_Z):
        z = CENTER_Z - SIZE_Z * 0.5 + SIZE_Z * row / (PIXELS_Z - 1)
        for column in range(PIXELS_X):
            x = CENTER_X - SIZE_X * 0.5 + SIZE_X * column / (PIXELS_X - 1)
            pixels[row, column] = int(round(min(max(height(x, z) / HEIGHT_SCALE, 0.0), 1.0) * 65535))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    Image.fromarray(pixels).save(path)
    print("HEIGHTMAP {} ({}x{}, highest {:.1f} m)".format(path, PIXELS_X, PIXELS_Z, pixels.max() / 65535 * HEIGHT_SCALE))


if __name__ == "__main__":
    write_heightmap(os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "res", "Terrain",
                                                  "plains_height.png")))
