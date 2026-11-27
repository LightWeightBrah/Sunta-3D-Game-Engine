"""
Grave Crown's plants from Poly Haven's scanned clumps (CC0): each clump is far too heavy to scatter by the thousand
(up to 6 000 triangles for 30 cm of grass), so a few of them are stood side by side and rendered from the side into a
picture with a transparent ground - the card picture the engine's Foliage draws on crossed quads
(foliage_generator.write_tuft). The scans keep their real blades and colors, toned to the plains' ash palette

    blender --background --factory-startup --python Projects/GraveCrown/Tools/foliage_from_scans.py -- <poly haven folder>
    python Projects/GraveCrown/Tools/foliage_from_scans.py --finish      (tones the pictures, writes the cards' glTF)

<poly haven folder> holds the downloaded glTF (1k) of each asset in <id>/<id>.gltf (see polyhaven_models.py)
"""

import math
import os
import sys

try:
    import bpy
except ImportError:
    bpy = None

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
RES = os.path.normpath(os.path.join(HERE, "..", "res"))
FOLIAGE = os.path.join(RES, "Models", "Foliage")

# name: (asset, clumps stood side by side (node name parts), card width and height in meters, scale of the clumps,
#        saturation kept, brightness)
CARDS = {
    "scan_grass_tall":  ("grass_medium_02", ["_e", "_d", "_c", "_e", "_b"], 1.0, 0.75, 1.9, 0.35, 0.85),
    "scan_grass_low":   ("grass_medium_01", ["large_a", "mid_a", "tall_a", "large_c", "tall_b", "mid_b"], 1.0, 0.5, 1.8, 0.3, 0.8),
    "scan_nettle":      ("nettle_plant", ["tall_a", "medium_a", "tall_b", "medium_b"], 0.7, 0.75, 2.6, 0.3, 0.75),
    "scan_fern":        ("fern_02", ["_b", "_c"], 1.2, 0.7, 1.2, 0.25, 0.7),
    "scan_celandine":   ("celandine_01", ["_c", "_a", "_d", "_b"], 0.7, 0.45, 2.2, 0.45, 0.85),
}
PIXELS_PER_METER = 420


def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def emission_material(source):
    """The scan's color texture as light (the engine shades the card itself), darker at the roots"""
    image = None
    for node in source.node_tree.nodes if source and source.node_tree else []:
        if node.type == "TEX_IMAGE" and node.image and "diff" in node.image.name.lower():
            image = node.image
    material = bpy.data.materials.new("card_" + (source.name if source else "none"))
    material.use_nodes = True
    nodes, links = material.node_tree.nodes, material.node_tree.links
    nodes.clear()
    out = nodes.new("ShaderNodeOutputMaterial")
    emission = nodes.new("ShaderNodeEmission")
    links.new(emission.outputs[0], out.inputs["Surface"])
    if image:
        texture = nodes.new("ShaderNodeTexImage")
        texture.image = image
        # height shade: 0.55 at the ground, 1.0 at 30 cm and above
        position = nodes.new("ShaderNodeNewGeometry")
        separate = nodes.new("ShaderNodeSeparateXYZ")
        ramp = nodes.new("ShaderNodeMapRange")
        ramp.inputs["From Min"].default_value = 0.0
        ramp.inputs["From Max"].default_value = 0.3
        ramp.inputs["To Min"].default_value = 0.55
        ramp.inputs["To Max"].default_value = 1.0
        multiply = nodes.new("ShaderNodeMix")
        multiply.data_type = "RGBA"
        multiply.blend_type = "MULTIPLY"
        multiply.inputs["Factor"].default_value = 1.0
        links.new(position.outputs["Position"], separate.inputs[0])
        links.new(separate.outputs["Z"], ramp.inputs["Value"])
        links.new(texture.outputs["Color"], multiply.inputs[6])
        links.new(ramp.outputs[0], multiply.inputs[7])
        links.new(multiply.outputs[2], emission.inputs["Color"])
        if True:
            # cut-out: the texture's alpha (if any) mixes with transparency
            mix = nodes.new("ShaderNodeMixShader")
            transparent = nodes.new("ShaderNodeBsdfTransparent")
            links.new(texture.outputs["Alpha"], mix.inputs[0])
            links.new(transparent.outputs[0], mix.inputs[1])
            links.new(emission.outputs[0], mix.inputs[2])
            links.new(mix.outputs[0], out.inputs["Surface"])
    return material


def build_row(asset_folder, asset, parts, width, scale):
    bpy.ops.import_scene.gltf(filepath=os.path.join(asset_folder, asset, asset + ".gltf"))
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    chosen = []
    for part in parts:
        match = [o for o in meshes if part in o.name]
        if not match:
            print("NO PART", part, "in", asset)
            continue
        obj = match[0]
        if obj in chosen:
            obj = obj.copy()
            obj.data = match[0].data
            bpy.context.scene.collection.objects.link(obj)
        chosen.append(obj)
    for obj in meshes:
        if obj not in chosen and obj.name in bpy.context.scene.objects:
            bpy.data.objects.remove(obj)
    count = len(chosen)
    for index, obj in enumerate(chosen):
        obj.parent = None
        obj.rotation_mode = "XYZ"
        obj.rotation_euler = (0.0, 0.0, index * 2.1)
        obj.scale = (scale, scale, scale)
        obj.location = ((index + 0.5) / count * width - width * 0.5 + ((index * 37) % 7 - 3) * 0.01, ((index * 53) % 5 - 2) * 0.04, 0.0)
        for slot in obj.material_slots:
            slot.material = emission_material(slot.material)
    return chosen


def render_card(name, width, height, path):
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 24
    scene.cycles.use_denoising = False
    scene.render.film_transparent = True
    scene.render.resolution_x = int(width * PIXELS_PER_METER)
    scene.render.resolution_y = int(height * PIXELS_PER_METER)
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.view_settings.view_transform = "Standard"
    if not scene.world:
        scene.world = bpy.data.worlds.new("w")
    scene.world.color = (0, 0, 0)
    camera_data = bpy.data.cameras.new(name)
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = max(width, height)
    camera = bpy.data.objects.new(name, camera_data)
    scene.collection.objects.link(camera)
    camera.location = (0.0, -5.0, height * 0.5)
    camera.rotation_euler = (math.pi / 2, 0.0, 0.0)
    scene.camera = camera
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def tone(raw, path, saturation, brightness):
    """The scan's colors pulled toward grey and the plains' ash, the edges' colors bled into the transparent pixels"""
    import numpy as np
    from PIL import Image, ImageFilter
    picture = np.asarray(Image.open(raw).convert("RGBA"), dtype=np.float32) / 255.0
    rgb, alpha = picture[..., :3], picture[..., 3:4]
    # Cycles writes straight colors on a transparent film; a pixel half covered is dimmed by nothing
    grey = (rgb @ np.array([0.299, 0.587, 0.114], dtype=np.float32))[..., None]
    ash = np.array([1.0, 0.94, 0.86], dtype=np.float32)
    rgb = (grey * ash + (rgb - grey) * saturation) * brightness
    # Parts the scan's texture leaves near black (seed heads, the backs of leaves mapped to a dark corner) take the
    # card's own average color, a little darker
    solid = picture[..., 3] > 0.5
    average = rgb[solid].mean(axis=0) if solid.any() else np.array([0.4, 0.38, 0.34], dtype=np.float32)
    darkness = np.clip((0.12 - grey) / 0.08, 0.0, 1.0)
    rgb = rgb * (1.0 - darkness) + average * 0.75 * darkness
    alpha = np.clip((alpha - 0.15) / 0.7, 0.0, 1.0)
    out = Image.fromarray((np.clip(np.concatenate([rgb, alpha], axis=2), 0, 1) * 255).astype(np.uint8), "RGBA")
    color = out.convert("RGB").filter(ImageFilter.MaxFilter(5))
    color_a = np.asarray(color, dtype=np.uint8).copy()
    covered = np.asarray(out)[..., 3] > 8
    color_a[covered] = np.asarray(out)[..., :3][covered]
    Image.fromarray(np.dstack([color_a, np.asarray(out)[..., 3]]), "RGBA").save(path)


def render_all():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    asset_folder = argv[0]
    for name in argv[1:] or list(CARDS):
        asset, parts, width, height, scale, _, _ = CARDS[name]
        clear()
        build_row(asset_folder, asset, parts, width, scale)
        folder = os.path.join(FOLIAGE, name)
        os.makedirs(folder, exist_ok=True)
        render_card(name, width, height, os.path.join(folder, name + "_raw.png"))
        print("RENDERED", name)


def finish():
    import foliage_generator
    for name, (_, _, width, height, _, saturation, brightness) in CARDS.items():
        folder = os.path.join(FOLIAGE, name)
        raw = os.path.join(folder, name + "_raw.png")
        if not os.path.exists(raw):
            continue
        path = os.path.join(folder, name + "_diffuse.png")
        tone(raw, path, saturation, brightness)
        foliage_generator.write_tuft(name, name + "_diffuse.png", width, height)
        os.remove(raw)
        print("CARD", name)


if bpy:
    render_all()
elif "--finish" in sys.argv:
    finish()
