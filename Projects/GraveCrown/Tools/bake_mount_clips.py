"""The riding clips of Mesh2Motion (CC0: rigs/human/mounts/Mount_Idle|Walk|Gallop.blend) are driven by a control rig;
this bakes them onto the deforming bones (Unreal-style names: pelvis, spine_01, thigh_l...) and saves one GLB per clip,
which motion_retarget reads with its QUATERNIUS rig
    blender --background --factory-startup --python Projects/GraveCrown/Tools/bake_mount_clips.py"""
import os
import bpy

FOLDER = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "Tools", "Blender",
                                       "Mesh2Motion", "human"))

for clip in ("Mount_Idle", "Mount_Walk", "Mount_Gallop"):
    bpy.ops.wm.open_mainfile(filepath=os.path.join(FOLDER, clip + ".blend"))
    armature = next(o for o in bpy.data.objects if o.type == "ARMATURE")
    action = armature.animation_data.action
    start, end = (int(v) for v in action.frame_range)
    bpy.context.view_layer.objects.active = armature
    for obj in bpy.data.objects:
        obj.select_set(obj is armature)
    bpy.ops.object.mode_set(mode="POSE")
    bpy.ops.nla.bake(frame_start=start, frame_end=end, only_selected=False, visual_keying=True, clear_constraints=True,
                     use_current_action=False, bake_types={"POSE"})
    bpy.ops.object.mode_set(mode="OBJECT")
    baked = armature.animation_data.action
    # Only the deforming bones go out (export_def_bones): the control rig would confuse the retarget
    for action in list(bpy.data.actions):
        if action is not baked:
            bpy.data.actions.remove(action)
    baked.name = clip
    # The file's own mannequin comes from a library that is not there: a tiny triangle skinned to every deforming bone
    # gives the exporter a skin, so the skeleton is written (glTF keeps bones only as joints of a skin)
    for obj in list(bpy.data.objects):
        if obj.library or obj.type == "EMPTY" or (obj.type == "MESH" and not obj.data.polygons):
            bpy.data.objects.remove(obj)
    mesh = bpy.data.meshes.new("Carrier")
    mesh.from_pydata([(0, 0, 0), (0.01, 0, 0), (0, 0.01, 0)], [], [(0, 1, 2)])
    carrier = bpy.data.objects.new("Carrier", mesh)
    bpy.context.scene.collection.objects.link(carrier)
    carrier.parent = armature
    carrier.modifiers.new("Armature", "ARMATURE").object = armature
    for bone in armature.data.bones:
        if bone.use_deform:
            carrier.vertex_groups.new(name=bone.name).add([0, 1, 2], 0.01, "REPLACE")
    out = os.path.join(FOLDER, clip + ".glb")
    bpy.ops.export_scene.gltf(filepath=out, use_selection=False, export_format="GLB", export_animations=True,
                              export_def_bones=True)
    print("BAKED", clip, start, end, "->", out)
