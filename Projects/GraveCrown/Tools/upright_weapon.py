"""Stands a weapon model made from a picture of it lying across (Meshy) upright the way the generators expect it:
along +Z with the handle at the bottom, the flat of the blade in the X-Z plane (its edge towards -X)
    blender --background --factory-startup --python Projects/GraveCrown/Tools/upright_weapon.py -- <in.glb> <out.glb>
The handle end is the one nearer the widest cross-section (the guard of a sword, the head of an axe is not: pass
--head-is-wide for weapons whose wide part is at the far end)"""
import sys
import bpy
from mathutils import Matrix

argv = sys.argv[sys.argv.index("--") + 1:]
source, target = argv[0], argv[1]
head_is_wide = "--head-is-wide" in argv
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=source)
mesh = next(o for o in bpy.context.scene.objects if o.type == "MESH")
mesh.data.transform(mesh.matrix_world)
mesh.parent = None
mesh.matrix_world = Matrix.Identity(4)

points = [v.co.copy() for v in mesh.data.vertices]
extent = [max(p[i] for p in points) - min(p[i] for p in points) for i in range(3)]
length_axis = max(range(3), key=lambda i: extent[i])
thin_axis = min(range(3), key=lambda i: extent[i])
width_axis = 3 - length_axis - thin_axis

# New axes: length -> Z, width -> X, thickness -> Y (a proper rotation: flip the width if the basis would mirror)
rows = [[0.0] * 3 for _ in range(3)]
rows[0][width_axis] = 1.0
rows[1][thin_axis] = 1.0
rows[2][length_axis] = 1.0
basis = Matrix(rows)
if basis.determinant() < 0:
    rows[0][width_axis] = -1.0
    basis = Matrix(rows)
mesh.data.transform(basis.to_4x4())

# The widest slice along the length: the guard - the handle is on its nearer side
points = [v.co for v in mesh.data.vertices]
low, high = min(p.z for p in points), max(p.z for p in points)
slices = 40
widths = [0.0] * slices
for p in points:
    k = min(int((p.z - low) / (high - low) * slices), slices - 1)
    widths[k] = max(widths[k], abs(p.x))
widest = max(range(slices), key=lambda k: widths[k])
handle_at_top = (widest > slices / 2) != head_is_wide
if handle_at_top:
    mesh.data.transform(Matrix.Rotation(3.14159265, 4, "X"))
print("UPRIGHT", extent, "widest slice", widest, "flipped", handle_at_top)
bpy.ops.object.select_all(action="DESELECT")
mesh.select_set(True)
bpy.ops.export_scene.gltf(filepath=target, use_selection=True, export_format="GLB")
