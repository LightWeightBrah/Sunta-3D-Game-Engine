"""
Bakes a flipbook for the engine's particles (ParticleEmitterComponent.flipbookTexture): a smoke or fire simulation
(Blender's Mantaflow, the same kind of thing Houdini or EmberGen do for game effects) rendered frame by frame with a
transparent background; Tools/Blender/flipbook_atlas.py then puts the frames into one picture of columns x rows.

    blender --background --factory-startup --python Tools/Blender/flipbook_baker.py -- <smoke|fire|burst> <out folder>
        [--frames 64] [--size 160] [--resolution 72] [--samples 24]

The smoke is white (the emitter's colors tint it), the fire keeps its own colors (an emitter with white colors
shows it as rendered). One puff from birth to fading away: a speck plays it once over its life
"""

import argparse
import math
import os
import sys

import bpy

argv = sys.argv[sys.argv.index("--") + 1:]
parser = argparse.ArgumentParser()
parser.add_argument("kind", choices=("smoke", "fire", "burst"))
parser.add_argument("out")
parser.add_argument("--frames", type=int, default=64)
parser.add_argument("--size", type=int, default=160)
parser.add_argument("--resolution", type=int, default=72)
parser.add_argument("--samples", type=int, default=24)
args = parser.parse_args(argv)

os.makedirs(args.out, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene

# How long the simulation runs; the flipbook takes 'frames' pictures spread over it
SIM_FRAMES = {"smoke": 90, "fire": 60, "burst": 70}[args.kind]
scene.frame_start, scene.frame_end = 1, SIM_FRAMES

# ----- the source: a small ball that puffs for a moment -----
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.34 if args.kind != "burst" else 0.42, location=(0, 0, -0.42))
source = bpy.context.active_object
bpy.ops.object.quick_smoke()
domain = [o for o in scene.objects if o.name.startswith("Smoke Domain")][0]

flow = source.modifiers["Fluid"].flow_settings
flow.flow_type = "SMOKE" if args.kind == "smoke" else "BOTH"
flow.flow_behavior = "INFLOW"
flow.use_initial_velocity = True
flow.velocity_coord = (0.0, 0.0, 1.2 if args.kind == "smoke" else 0.6)
flow.smoke_color = (1.0, 1.0, 1.0)
flow.fuel_amount = 1.4 if args.kind == "burst" else 1.0
flow.temperature = 2.0 if args.kind == "fire" else 1.0
# It puffs only at first: frame 1-12 (a burst 1-4), then the cloud lives on and thins out
flow.surface_distance = 0.6 if args.kind == "burst" else 0.3
source.modifiers["Fluid"].flow_settings.use_plane_init = False
scene.frame_set(1)
flow.density = 1.0
flow.keyframe_insert("density", frame=1)
last_puff = 4 if args.kind == "burst" else 14
flow.keyframe_insert("density", frame=last_puff)
flow.density = 0.0
flow.keyframe_insert("density", frame=last_puff + 2)
if args.kind != "smoke":
    flow.fuel_amount = flow.fuel_amount
    flow.keyframe_insert("fuel_amount", frame=last_puff)
    flow.fuel_amount = 0.0
    flow.keyframe_insert("fuel_amount", frame=last_puff + 2)

# ----- the domain: a cube around it, the cloud stays inside -----
domain.location = (0, 0, 0)
domain.scale = (1.0, 1.0, 1.0)
settings = domain.modifiers["Fluid"].domain_settings
settings.resolution_max = args.resolution
settings.use_adaptive_domain = False
settings.vorticity = 0.5 if args.kind == "smoke" else 0.45
settings.use_noise = True              # finer wisps over the coarse simulation
settings.noise_scale = 2
settings.noise_strength = 1.2
settings.alpha = -0.2 if args.kind == "smoke" else 0.4      # smoke barely rises by itself (the speck carries it)
settings.beta = 0.6
settings.dissolve_speed = 60 if args.kind == "smoke" else 26
settings.use_dissolve_smoke = True
settings.burning_rate = 0.9
settings.flame_vorticity = 0.6
settings.cache_type = "ALL"
settings.cache_directory = os.path.join(args.out, "cache")

bpy.context.view_layer.objects.active = domain
domain.select_set(True)
with bpy.context.temp_override(object=domain, active_object=domain, selected_objects=[domain]):
    bpy.ops.fluid.bake_all()
print("BAKED", flush=True)

source.hide_render = True

# ----- the look: white smoke, fire glowing in its own colors -----
material = domain.active_material
nodes = material.node_tree.nodes
volume = [n for n in nodes if n.type == "PRINCIPLED_VOLUME"][0]
volume.inputs["Color"].default_value = (1.0, 1.0, 1.0, 1.0)
volume.inputs["Density"].default_value = 16.0 if args.kind == "smoke" else 6.0
if args.kind != "smoke":
    volume.inputs["Blackbody Intensity"].default_value = 1.6
    volume.inputs["Temperature"].default_value = 1400
else:
    volume.inputs["Blackbody Intensity"].default_value = 0.0

# ----- render: orthographic from the side, transparent, soft light from above and the front -----
scene.render.engine = "CYCLES"
scene.cycles.device = "GPU"
preferences = bpy.context.preferences.addons["cycles"].preferences
for kind in ("OPTIX", "CUDA"):
    try:
        preferences.compute_device_type = kind
        preferences.get_devices()
        for device in preferences.devices:
            device.use = True
        break
    except Exception:
        continue
scene.cycles.samples = args.samples
scene.cycles.use_denoising = True
scene.cycles.volume_step_rate = 1.0
scene.render.film_transparent = True
scene.render.resolution_x = scene.render.resolution_y = args.size
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGBA"
scene.view_settings.view_transform = "Standard"

camera = bpy.data.objects.new("Camera", bpy.data.cameras.new("Camera"))
scene.collection.objects.link(camera)
camera.data.type = "ORTHO"
camera.data.ortho_scale = 2.05
camera.location = (0, -6, 0)
camera.rotation_euler = (math.radians(90), 0, 0)
scene.camera = camera

world = bpy.data.worlds.new("World")
world.use_nodes = True
world.node_tree.nodes["Background"].inputs[1].default_value = 0.35 if args.kind == "smoke" else 0.0
scene.world = world
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
sun.data.energy = 3.0 if args.kind == "smoke" else 0.0
sun.rotation_euler = (math.radians(50), math.radians(10), math.radians(30))
scene.collection.objects.link(sun)

first, last = 3, SIM_FRAMES
for index in range(args.frames):
    frame = round(first + (last - first) * index / max(args.frames - 1, 1))
    scene.frame_set(frame)
    scene.render.filepath = os.path.join(args.out, f"frame_{index:03d}.png")
    bpy.ops.render.render(write_still=True)
print("FLIPBOOK_FRAMES_DONE", args.out, flush=True)
