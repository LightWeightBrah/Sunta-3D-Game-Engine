"""
Grave Crown's props from Poly Haven's scanned models (polyhaven.com, CC0): barrels, a table and a stool, a chest, a
fire pit, a gothic statue, clay urns, mossy rocks, a dead trunk, roots, a shield, a mace, a spade and a hatchet left on
the plains. Each scan is reduced to a triangle budget (some are 100 000 triangles), its color photo toned to the plains'
palette (charred wood, ash-grey stone and iron, dull clay) - the same look as the Nature/ models - and written to
res/Models/Props/<name>/<name>.gltf

    python Projects/GraveCrown/Tools/props_from_scans.py

Downloads go to Tools/Blender/PolyHaven/ (not in the repository); Blender (its path in BLENDER) reduces and exports
"""

import json
import os
import subprocess
import sys
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
PROPS = os.path.normpath(os.path.join(HERE, "..", "res", "Models", "Props"))
CACHE = os.path.join(REPO, "Tools", "Blender", "PolyHaven")
BLENDER = r"C:\Program Files (x86)\Steam\steamapps\common\Blender\blender.exe"
AGENT = {"User-Agent": "SuntaEngineTools/1.0"}

# (tone RGB, brightness, contrast) - see polyhaven_models.py
WOOD = ((0.44, 0.34, 0.27), 0.72, 1.25)
IRON = ((0.46, 0.45, 0.50), 0.62, 1.3)
CLAY = ((0.50, 0.40, 0.35), 0.66, 1.2)
STONE = ((0.50, 0.48, 0.52), 0.6, 1.25)
CHARRED = ((0.36, 0.31, 0.30), 0.7, 1.35)

# name: (Poly Haven id, most triangles, tone)
PROPS_LIST = {
    "ph_barrel_a":     ("Barrel_01", 2700, WOOD),
    "ph_barrel_b":     ("Barrel_02", 2700, WOOD),
    "ph_barrel_c":     ("barrel_03", 1500, WOOD),
    "ph_table":        ("WoodenTable_01", 1000, WOOD),
    "ph_stool":        ("folding_wooden_stool", 1500, WOOD),
    "ph_chest":        ("treasure_chest", 5000, WOOD),
    "ph_fire_pit":     ("stone_fire_pit", 3900, STONE),
    "ph_statue":       ("gothic_statue", 7000, STONE),
    "ph_urn":          ("ceramic_pot", 2500, CLAY),
    "ph_vase":         ("ceramic_vase_01", 2500, CLAY),
    "ph_jug":          ("jug_01", 2000, CLAY),
    "ph_rocks_moss":   ("rock_moss_set_02", 6000, STONE),
    "ph_rock_a":       ("rock_07", 3000, STONE),
    "ph_rock_b":       ("rock_09", 3000, STONE),
    "ph_boulder":      ("boulder_01", 5000, STONE),
    "ph_dead_trunk":   ("dead_tree_trunk", 6000, CHARRED),
    "ph_branches":     ("dry_branches_medium_01", 3000, CHARRED),
    "ph_root":         ("single_root", 3000, CHARRED),
    "ph_shield":       ("kite_shield", 2500, IRON),
    "ph_mace":         ("ornate_medieval_mace", 2000, IRON),
    "ph_spade":        ("rusted_spade_01", 2000, IRON),
    "ph_hatchet":      ("hatchet", 1500, IRON),
}


def fetch(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=AGENT), timeout=180) as response:
        return response.read()


def download(asset):
    folder = os.path.join(CACHE, asset)
    gltf = os.path.join(folder, asset + ".gltf")
    if os.path.exists(gltf):
        return gltf
    files = json.loads(fetch("https://api.polyhaven.com/files/" + asset))["gltf"]["1k"]["gltf"]
    for relative, entry in files["include"].items():
        path = os.path.join(folder, relative)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as out:
            out.write(fetch(entry["url"]))
    with open(gltf, "wb") as out:
        out.write(fetch(files["url"]))
    print("GOT", asset)
    return gltf


BLENDER_PASS = r'''
import bpy, sys, os
source, target, budget = sys.argv[sys.argv.index("--") + 1:][:3]
budget = int(budget)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=source)
meshes = [o for o in bpy.data.objects if o.type == "MESH"]
for o in bpy.data.objects:
    o.select_set(o in meshes)
bpy.context.view_layer.objects.active = meshes[0]
if len(meshes) > 1:
    bpy.ops.object.join()
mesh = bpy.context.view_layer.objects.active
bpy.ops.object.parent_clear(type="CLEAR_KEEP_TRANSFORM")
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
for o in list(bpy.data.objects):
    if o != mesh:
        bpy.data.objects.remove(o)
triangles = sum(len(p.vertices) - 2 for p in mesh.data.polygons)
if triangles > budget:
    modifier = mesh.modifiers.new("reduce", "DECIMATE")
    modifier.ratio = budget / triangles
    bpy.ops.object.modifier_apply(modifier="reduce")
# Feet on the ground, centred
low = min(v.co.z for v in mesh.data.vertices)
xs = [v.co.x for v in mesh.data.vertices]
ys = [v.co.y for v in mesh.data.vertices]
for v in mesh.data.vertices:
    v.co.x -= (min(xs) + max(xs)) / 2
    v.co.y -= (min(ys) + max(ys)) / 2
    v.co.z -= low
mesh.name = os.path.splitext(os.path.basename(target))[0]
os.makedirs(os.path.dirname(target), exist_ok=True)
bpy.ops.export_scene.gltf(filepath=target, export_format="GLTF_SEPARATE", export_animations=False)
after = sum(len(p.vertices) - 2 for p in mesh.data.polygons)
dims = mesh.dimensions
print("PROP {} {} -> {} triangles, size {:.2f} x {:.2f} x {:.2f}".format(mesh.name, triangles, after, dims.x, dims.z, dims.y))
'''


def main():
    sys.path.insert(0, HERE)
    import polyhaven_models
    only = sys.argv[1:]
    script = os.path.join(CACHE, "_reduce.py")
    os.makedirs(CACHE, exist_ok=True)
    with open(script, "w", encoding="utf-8") as out:
        out.write(BLENDER_PASS)
    for name, (asset, budget, tone) in PROPS_LIST.items():
        if only and name not in only:
            continue
        source = download(asset)
        target = os.path.join(PROPS, name, name + ".gltf")
        result = subprocess.run([BLENDER, "--background", "--factory-startup", "--python", script, "--", source, target, str(budget)],
                                capture_output=True, text=True)
        lines = [line for line in result.stdout.splitlines() if line.startswith("PROP")]
        if not lines:
            print("FAILED", name, result.stdout[-1500:], result.stderr[-800:])
            continue
        print(lines[0])
        polyhaven_models.keep_only_color(target)
        for file_name in os.listdir(os.path.dirname(target)):
            if file_name.lower().endswith((".jpg", ".png")):
                polyhaven_models.recolor(os.path.join(os.path.dirname(target), file_name), tone)


if __name__ == "__main__":
    main()
