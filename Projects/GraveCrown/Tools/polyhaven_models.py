"""Downloads the CC0 models of Grave Crown's plains from Poly Haven (polyhaven.com, public domain): dead trees, boulders,
cliffs, roots. Each goes into res/Models/Nature/<id>/ as the glTF Poly Haven made (1k textures); only its color
texture is kept (the engine lights with the diffuse texture and its own specular, the normal and roughness maps would
only take space)
    python Projects/GraveCrown/Tools/polyhaven_models.py
Listed in Tools/Blender/Textures/CREDITS.md"""
import json
import os
import urllib.request

import numpy
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
NATURE = os.path.normpath(os.path.join(HERE, "..", "res", "Models", "Nature"))
API = "https://api.polyhaven.com/files/"
AGENT = {"User-Agent": "SuntaEngineTools/1.0"}

# The world's palette: every photo becomes its own light and dark spots in one tone (ash-grey stone, charred wood), so
# a golden sandstone or a pale bleached trunk doesn't break the look. (tone RGB, brightness, contrast)
STONE_TONE = ((0.50, 0.48, 0.52), 0.85, 1.25)
CHARRED_TONE = ((0.36, 0.31, 0.30), 0.7, 1.35)
TONES = {"dead_tree_trunk_02": CHARRED_TONE, "dead_quiver_trunk": CHARRED_TONE, "tree_stump_01": CHARRED_TONE,
         "root_cluster_01": CHARRED_TONE}

MODELS = ["dead_tree_trunk_02", "dead_quiver_trunk", "namaqualand_boulder_02", "namaqualand_boulder_04",
          "rock_face_01", "root_cluster_01", "tree_stump_01", "namaqualand_cliff_01"]


def fetch(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=AGENT), timeout=120) as response:
        return response.read()


def keep_only_color(gltf_path):
    """Drops the normal, roughness and occlusion maps from the materials and their files"""
    folder = os.path.dirname(gltf_path)
    with open(gltf_path, encoding="utf-8") as source:
        gltf = json.load(source)
    used = set()
    for material in gltf.get("materials", []):
        pbr = material.get("pbrMetallicRoughness", {})
        pbr.pop("metallicRoughnessTexture", None)
        material.pop("normalTexture", None)
        material.pop("occlusionTexture", None)
        if "baseColorTexture" in pbr:
            used.add(gltf["textures"][pbr["baseColorTexture"]["index"]]["source"])
    for index, image in enumerate(gltf.get("images", [])):
        path = os.path.join(folder, image["uri"])
        if index not in used and os.path.exists(path):
            os.remove(path)
    # The engine looks for a model's textures next to it: out of the textures/ folder
    for image in gltf.get("images", []):
        name = os.path.basename(image["uri"])
        if os.path.exists(os.path.join(folder, image["uri"])) and os.path.dirname(image["uri"]):
            os.replace(os.path.join(folder, image["uri"]), os.path.join(folder, name))
        image["uri"] = name
    with open(gltf_path, "w", encoding="utf-8") as target:
        json.dump(gltf, target, indent=1)


def recolor(path, tone):
    """The photo's brightness only, around its own average, painted in 'tone'"""
    (red, green, blue), brightness, contrast = tone
    picture = numpy.asarray(Image.open(path).convert("RGB"), dtype=numpy.float32) / 255.0
    light = picture @ numpy.array([0.299, 0.587, 0.114], dtype=numpy.float32)
    light = numpy.clip((light - light.mean()) * contrast + 0.5, 0.0, 1.0) * brightness
    color = numpy.stack([light * red, light * green, light * blue], axis=-1) * 2.0
    Image.fromarray((numpy.clip(color, 0.0, 1.0) * 255).astype(numpy.uint8)).save(path, quality=92)


def download(asset):
    folder = os.path.join(NATURE, asset)
    gltf_path = os.path.join(folder, asset + ".gltf")
    if os.path.exists(gltf_path):
        print("HAVE", asset)
        return
    files = json.loads(fetch(API + asset))["gltf"]["1k"]["gltf"]
    os.makedirs(os.path.join(folder, "textures"), exist_ok=True)
    for relative, entry in files["include"].items():
        with open(os.path.join(folder, relative), "wb") as out:
            out.write(fetch(entry["url"]))
    with open(gltf_path, "wb") as out:
        out.write(fetch(files["url"]))
    keep_only_color(gltf_path)
    for name in os.listdir(folder):
        if name.endswith(".jpg"):
            recolor(os.path.join(folder, name), TONES.get(asset, STONE_TONE))
    print("GOT", asset)


for model in MODELS:
    download(model)
