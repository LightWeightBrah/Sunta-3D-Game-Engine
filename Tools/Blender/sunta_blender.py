"""
General tools for making the engine's models in Blender (used by knight_generator.py, bonfire_generator.py, ...)

Models are built from simple shapes (PartBuilder): every shape has one color of a small palette and follows one bone (or none).
Cycles then bakes the final textures (bake_textures): the palette color with fine grain, large stains, darker crevices
(ambient occlusion) and brighter worn edges on metal, plus a specular map. export_gltf writes what the engine loads
A palette color can also get the surface of a photo (SurfaceTexture, the photos are in Tools/Blender/Textures)

Blender axes: Z up. The glTF exporter turns them into the engine's axes (Y up)
"""

import math
import os
from dataclasses import dataclass

import bmesh
import bpy
import numpy
from mathutils import Euler, Matrix, Vector

FRAMES_PER_SECOND = 30

# ========== Colors ==========

# Every color is one cell of a small texture, the UVs of a part point into the middle of its cell
PALETTE_GRID = 4
PALETTE_CELL_PIXELS = 64

# A specular value above this marks metal: only metal gets brighter worn edges
METAL_SPECULAR = 0.5

# The photos for SurfaceTexture (Textures/CREDITS.md says where they come from)
TEXTURES_FOLDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Textures")

# How much the copies of a photo blend where the sides of a shape meet (box projection)
SURFACE_PROJECTION_BLEND = 0.25

# ========== Baked textures ==========

PALETTE_UV = "Palette"
BAKED_UV = "UVMap"
BAKE_SAMPLES = 16
BAKE_MARGIN_PIXELS = 8

# Smart UV Project: faces bent by more than this end up on separate islands
UV_ANGLE_LIMIT_DEGREES = 60.0
UV_ISLAND_MARGIN = 0.004

# Edges sharper than this stay sharp, the rest is shaded smooth
SHARP_EDGE_DEGREES = 35.0

# Size of the chamfer on the edges of box shaped plates (meters)
BOX_BEVEL = 0.005


def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.fps = FRAMES_PER_SECOND


def degrees_to_quaternion(degrees_xyz):
    return Euler([math.radians(angle) for angle in degrees_xyz], "XYZ").to_quaternion()


def make_matrix(location=(0, 0, 0), rotation_degrees=(0, 0, 0), scale=(1, 1, 1)):
    return Matrix.LocRotScale(Vector(location), degrees_to_quaternion(rotation_degrees), Vector(scale))


def matrix_aligning_z_to(head, tail):
    """Puts a shape built along +Z (from -depth/2 to +depth/2) between two points"""
    head, tail = Vector(head), Vector(tail)
    direction = tail - head
    rotation = Vector((0, 0, 1)).rotation_difference(direction.normalized())
    return Matrix.LocRotScale((head + tail) * 0.5, rotation, Vector((1, 1, 1)))


# ========== Palette texture ==========


@dataclass(frozen=True)
class SurfaceTexture:
    """A photo that gives a palette color its surface: only the light and dark spots of the photo are used,
    the color stays the palette's. The photo is projected onto every side of a shape (no UVs needed)"""
    file_name: str          # in TEXTURES_FOLDER
    meters: float = 1.0     # the size of one copy of the photo on the model
    strength: float = 1.0   # 0 = the plain palette color, 1 = the contrast of the photo, more = stronger than the photo


class Palette:
    """name: (diffuse RGB, specular intensity) or (diffuse RGB, specular intensity, True) for a color that glows
    (fire, embers): it keeps its plain color, without the grain, the stains and the dark crevices of the bake
    'surfaces' gives some colors a photo surface: {name: SurfaceTexture}"""

    def __init__(self, colors, surfaces=None):
        self.names = list(colors.keys())
        self.colors = colors
        self.surfaces = surfaces or {}
        assert len(self.names) <= PALETTE_GRID * PALETTE_GRID
        assert all(name in colors for name in self.surfaces)

    def uv(self, name):
        index = self.names.index(name)
        column, row = index % PALETTE_GRID, index // PALETTE_GRID
        return ((column + 0.5) / PALETTE_GRID, (row + 0.5) / PALETTE_GRID)

    def make_images(self, base_name):
        """The diffuse, the specular and the glow palette, only in memory (they are the input of the bake)"""
        size = PALETTE_GRID * PALETTE_CELL_PIXELS
        layers = {"diffuse": [], "specular": [], "glow": []}
        pixels = {layer: [0.0] * (size * size * 4) for layer in layers}

        for index, name in enumerate(self.names):
            (red, green, blue), intensity, *glow = self.colors[name]
            glow_amount = 1.0 if glow and glow[0] else 0.0
            cell = {
                "diffuse": [red, green, blue, 1.0],
                "specular": [intensity, intensity, intensity, 1.0],
                "glow": [glow_amount, glow_amount, glow_amount, 1.0],
            }

            column, row = index % PALETTE_GRID, index // PALETTE_GRID
            for y in range(row * PALETTE_CELL_PIXELS, (row + 1) * PALETTE_CELL_PIXELS):
                for x in range(column * PALETTE_CELL_PIXELS, (column + 1) * PALETTE_CELL_PIXELS):
                    pixel = (y * size + x) * 4
                    for layer in layers:
                        pixels[layer][pixel:pixel + 4] = cell[layer]

        return [self._make_image(base_name + "_palette_" + layer, pixels[layer], is_color=(layer == "diffuse"))
                for layer in ("diffuse", "specular", "glow")]

    def make_surface_masks(self, base_name):
        """[(SurfaceTexture, mask)]: the mask is white in the cells of the colors that use that photo"""
        size = PALETTE_GRID * PALETTE_CELL_PIXELS
        masks = []

        for surface in dict.fromkeys(self.surfaces.values()):
            pixels = [0.0] * (size * size * 4)
            for index, name in enumerate(self.names):
                value = 1.0 if self.surfaces.get(name) == surface else 0.0
                column, row = index % PALETTE_GRID, index // PALETTE_GRID
                for y in range(row * PALETTE_CELL_PIXELS, (row + 1) * PALETTE_CELL_PIXELS):
                    for x in range(column * PALETTE_CELL_PIXELS, (column + 1) * PALETTE_CELL_PIXELS):
                        pixel = (y * size + x) * 4
                        pixels[pixel:pixel + 4] = [value, value, value, 1.0]

            name = base_name + "_mask_" + os.path.splitext(surface.file_name)[0]
            masks.append((surface, self._make_image(name, pixels, is_color=False)))

        return masks

    @staticmethod
    def _make_image(name, pixels, is_color):
        size = PALETTE_GRID * PALETTE_CELL_PIXELS
        image = bpy.data.images.new(name, size, size, alpha=False)
        if not is_color:
            image.colorspace_settings.name = "Non-Color"
        image.pixels.foreach_set(pixels)
        return image


def load_surface_photo(surface):
    """The photo and its average color (linear, the way the shader sees it): dividing by it leaves only the light and dark spots"""
    image = bpy.data.images.load(os.path.join(TEXTURES_FOLDER, surface.file_name), check_existing=True)

    pixels = numpy.empty(len(image.pixels), dtype=numpy.float32)
    image.pixels.foreach_get(pixels)
    srgb = pixels.reshape(-1, 4)[:, :3]
    linear = numpy.where(srgb <= 0.04045, srgb / 12.92, ((srgb + 0.055) / 1.055) ** 2.4)

    return image, tuple(float(channel) for channel in linear.mean(axis=0))


def make_textured_material(name, diffuse_image):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    shader = nodes.get("Principled BSDF")
    shader.inputs["Metallic"].default_value = 0.0
    shader.inputs["Roughness"].default_value = 0.5

    texture = nodes.new("ShaderNodeTexImage")
    texture.image = diffuse_image
    material.node_tree.links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    return material


# ========== Building meshes out of simple shapes ==========


class PartBuilder:
    """Collects simple shapes into one mesh. Every shape has one color and follows one bone"""

    def __init__(self, palette):
        self.palette = palette
        self.mesh = bmesh.new()
        self.uv_layer = self.mesh.loops.layers.uv.verify()
        self.deform_layer = self.mesh.verts.layers.deform.verify()
        self.bone_indices = {}

    def bone_index(self, bone):
        if bone not in self.bone_indices:
            self.bone_indices[bone] = len(self.bone_indices)
        return self.bone_indices[bone]

    def _finish_shape(self, result_verts, matrix, color, bone, bevel=0.0):
        verts = [element for element in result_verts if isinstance(element, bmesh.types.BMVert)]
        bmesh.ops.transform(self.mesh, matrix=matrix, verts=verts)

        if bevel > 0.0:
            edges = list({edge for vert in verts for edge in vert.link_edges})
            bevelled = bmesh.ops.bevel(self.mesh, geom=edges, offset=bevel, segments=1, affect="EDGES", profile=0.5)
            verts = [vert for vert in verts if vert.is_valid] + list(bevelled["verts"])

        faces = {face for vert in verts for face in vert.link_faces}
        uv = self.palette.uv(color)
        for face in faces:
            for loop in face.loops:
                loop[self.uv_layer].uv = uv

        if bone is not None:
            index = self.bone_index(bone)
            for vert in verts:
                vert[self.deform_layer][index] = 1.0

    def box(self, bone, color, location, size, rotation=(0, 0, 0), bevel=BOX_BEVEL):
        result = bmesh.ops.create_cube(self.mesh, size=1.0)
        matrix = make_matrix(location, rotation, size)
        self._finish_shape(result["verts"], matrix, color, bone, bevel)

    def cylinder(self, bone, color, matrix, radius_bottom, radius_top, depth, segments=20, scale=(1, 1, 1)):
        result = bmesh.ops.create_cone(self.mesh, cap_ends=True, cap_tris=False, segments=segments,
                                       radius1=radius_bottom, radius2=radius_top, depth=depth)
        self._finish_shape(result["verts"], matrix @ Matrix.Diagonal(Vector((*scale, 1.0))), color, bone)

    def limb(self, bone, color, head, tail, radius_head, radius_tail, segments=16, scale=(1, 1, 1)):
        """A tapered tube between two points (radius_head at 'head')"""
        depth = (Vector(tail) - Vector(head)).length
        self.cylinder(bone, color, matrix_aligning_z_to(head, tail), radius_head, radius_tail, depth, segments, scale)

    def sphere(self, bone, color, location, radii, rotation=(0, 0, 0), segments=20, rings=12):
        result = bmesh.ops.create_uvsphere(self.mesh, u_segments=segments, v_segments=rings, radius=1.0)
        matrix = make_matrix(location, rotation, radii)
        self._finish_shape(result["verts"], matrix, color, bone)

    def half_sphere(self, bone, color, location, radii, rotation=(0, 0, 0), segments=20, rings=10):
        """The upper half (+Z) of a sphere, open at the bottom"""
        result = bmesh.ops.create_uvsphere(self.mesh, u_segments=segments, v_segments=rings, radius=1.0)
        lower_verts = [vert for vert in result["verts"] if vert.co.z < -1e-4]
        bmesh.ops.delete(self.mesh, geom=lower_verts, context="VERTS")
        remaining = [vert for vert in result["verts"] if vert.is_valid]
        matrix = make_matrix(location, rotation, radii)
        self._finish_shape(remaining, matrix, color, bone)

    def _shade_smooth_with_sharp_edges(self):
        sharp_angle = math.radians(SHARP_EDGE_DEGREES)
        for face in self.mesh.faces:
            face.smooth = True
        for edge in self.mesh.edges:
            edge.smooth = not (len(edge.link_faces) == 2 and edge.calc_face_angle(0.0) > sharp_angle)

    def to_object(self, name, armature=None):
        mesh = bpy.data.meshes.new(name)
        self._shade_smooth_with_sharp_edges()
        self.mesh.normal_update()
        self.mesh.to_mesh(mesh)
        self.mesh.free()
        mesh.uv_layers[0].name = PALETTE_UV

        mesh_object = bpy.data.objects.new(name, mesh)
        bpy.context.scene.collection.objects.link(mesh_object)

        for bone, _ in sorted(self.bone_indices.items(), key=lambda item: item[1]):
            mesh_object.vertex_groups.new(name=bone)

        if armature:
            mesh_object.parent = armature
            modifier = mesh_object.modifiers.new("Armature", "ARMATURE")
            modifier.object = armature

        return mesh_object


class SurfaceDetailMaterial:
    """The material the textures are baked from: the palette color made to look like used armor
    (fine grain, large stains, dark crevices, bright worn edges on metal)"""

    def __init__(self, name, palette_diffuse, palette_specular, palette_glow, surface_masks=()):
        self.material = bpy.data.materials.new(name)
        self.material.use_nodes = True
        self.nodes = self.material.node_tree.nodes
        self.links = self.material.node_tree.links
        self.nodes.clear()

        palette_uv = self._node("ShaderNodeUVMap", uv_map=PALETTE_UV)
        diffuse = self._image(palette_diffuse, palette_uv)
        specular = self._image(palette_specular, palette_uv)
        glow = self._image(palette_glow, palette_uv)

        object_coordinates = self._node("ShaderNodeTexCoord").outputs["Object"]
        fine_grain = self._noise(object_coordinates, scale=90.0, detail=8.0, low=0.72, high=1.18)
        stains = self._noise(object_coordinates, scale=7.0, detail=4.0, low=0.55, high=1.0, from_low=0.35, from_high=0.75)
        scratches = self._noise(object_coordinates, scale=160.0, detail=10.0, low=0.6, high=1.15)

        occlusion = self._node("ShaderNodeAmbientOcclusion", samples=16)
        occlusion.inputs["Distance"].default_value = 0.06
        darker_crevices = self._map_range(occlusion.outputs["AO"], 0.0, 1.0, 0.35, 1.0)

        pointiness = self._node("ShaderNodeNewGeometry").outputs["Pointiness"]
        edges = self._map_range(pointiness, 0.5, 0.57, 0.0, 0.22)
        metal = self._map_range(specular.outputs["Color"], METAL_SPECULAR - 0.1, METAL_SPECULAR + 0.1, 0.0, 1.0)
        worn_edges = self._math("MULTIPLY", edges, metal)

        color = self._multiply(diffuse.outputs["Color"], fine_grain)
        for surface, mask in surface_masks:
            color = self._multiply(color, self._surface_spots(surface, self._image(mask, palette_uv), object_coordinates))
        color = self._multiply(color, stains)
        color = self._multiply(color, darker_crevices)
        color = self._vector_math("ADD", color, worn_edges)

        # A glowing color keeps its plain palette color: detailed + (plain - detailed) * glow
        towards_plain = self._vector_math("SUBTRACT", diffuse.outputs["Color"], color)
        self.color = self._vector_math("ADD", color, self._multiply(towards_plain, glow.outputs["Color"]))

        shine = self._multiply(specular.outputs["Color"], scratches)
        self.specular = self._multiply(shine, darker_crevices)

        self.emission = self._node("ShaderNodeEmission")
        output = self._node("ShaderNodeOutputMaterial")
        self.links.new(self.emission.outputs["Emission"], output.inputs["Surface"])

        baked_uv = self._node("ShaderNodeUVMap", uv_map=BAKED_UV)
        self.target = self._node("ShaderNodeTexImage")
        self.links.new(baked_uv.outputs["UV"], self.target.inputs["Vector"])

    def bake(self, mesh_object, source, image):
        """Bakes one output (self.color or self.specular) into the image, through the unwrapped UVs"""
        self.links.new(source, self.emission.inputs["Color"])
        self.target.image = image
        self.nodes.active = self.target

        select_only([mesh_object])
        bpy.ops.object.bake(type="EMIT", margin=BAKE_MARGIN_PIXELS, use_clear=True)
        image.save()

    def _surface_spots(self, surface, mask, object_coordinates):
        """1 + (photo / its average - 1) * strength, only where the mask is white: the photo's light and dark spots around 1"""
        photo, average = load_surface_photo(surface)

        mapping = self._node("ShaderNodeMapping")
        mapping.inputs["Scale"].default_value = [1.0 / surface.meters] * 3
        self.links.new(object_coordinates, mapping.inputs["Vector"])

        texture = self._node("ShaderNodeTexImage", image=photo, projection="BOX", projection_blend=SURFACE_PROJECTION_BLEND)
        self.links.new(mapping.outputs["Vector"], texture.inputs["Vector"])

        inverse_average = self._node("ShaderNodeCombineXYZ")
        for axis, channel in zip("XYZ", average):
            inverse_average.inputs[axis].default_value = 1.0 / max(channel, 0.001)

        spots = self._multiply(texture.outputs["Color"], inverse_average.outputs["Vector"])
        spots = self._vector_math("SUBTRACT", spots, self._constant(1.0))
        spots = self._multiply(spots, mask.outputs["Color"])
        spots = self._multiply(spots, self._constant(surface.strength))
        return self._vector_math("ADD", spots, self._constant(1.0))

    def _constant(self, value):
        node = self._node("ShaderNodeCombineXYZ")
        for axis in "XYZ":
            node.inputs[axis].default_value = value
        return node.outputs["Vector"]

    def _node(self, node_type, **properties):
        node = self.nodes.new(node_type)
        for name, value in properties.items():
            setattr(node, name, value)
        return node

    def _image(self, image, uv_node):
        node = self._node("ShaderNodeTexImage", image=image, interpolation="Closest")
        self.links.new(uv_node.outputs["UV"], node.inputs["Vector"])
        return node

    def _map_range(self, value, from_low, from_high, to_low, to_high):
        node = self._node("ShaderNodeMapRange")
        self.links.new(value, node.inputs["Value"])
        for name, number in (("From Min", from_low), ("From Max", from_high), ("To Min", to_low), ("To Max", to_high)):
            node.inputs[name].default_value = number
        return node.outputs["Result"]

    def _noise(self, coordinates, scale, detail, low, high, from_low=0.3, from_high=0.7):
        noise = self._node("ShaderNodeTexNoise")
        self.links.new(coordinates, noise.inputs["Vector"])
        noise.inputs["Scale"].default_value = scale
        noise.inputs["Detail"].default_value = detail
        return self._map_range(noise.outputs["Fac"], from_low, from_high, low, high)

    def _math(self, operation, first, second):
        node = self._node("ShaderNodeMath", operation=operation)
        self.links.new(first, node.inputs[0])
        self.links.new(second, node.inputs[1])
        return node.outputs["Value"]

    def _vector_math(self, operation, first, second):
        node = self._node("ShaderNodeVectorMath", operation=operation)
        self.links.new(first, node.inputs[0])
        self.links.new(second, node.inputs[1])
        return node.outputs["Vector"]

    def _multiply(self, first, second):
        return self._vector_math("MULTIPLY", first, second)


def unwrap(mesh_object):
    """Second UV map with every face on its own place of the texture (the palette UVs share a few pixels)"""
    baked_uv = mesh_object.data.uv_layers.new(name=BAKED_UV)
    mesh_object.data.uv_layers.active = baked_uv

    select_only([mesh_object])
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(UV_ANGLE_LIMIT_DEGREES), island_margin=UV_ISLAND_MARGIN)
    bpy.ops.object.mode_set(mode="OBJECT")


def new_texture(name, folder, pixels, is_color):
    image = bpy.data.images.new(name, pixels, pixels, alpha=False)
    image.colorspace_settings.name = "sRGB" if is_color else "Non-Color"
    image.filepath_raw = os.path.join(folder, name + ".png")
    image.file_format = "PNG"
    return image


def setup_bake_render():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = BAKE_SAMPLES


def show_only(visible_objects):
    """Hides every other mesh from rendering and baking (the models overlap: the knight and the rusted knight stand in one place)"""
    for scene_object in bpy.context.scene.objects:
        if scene_object.type == "MESH":
            scene_object.hide_render = scene_object not in visible_objects


def bake_textures(mesh_object, palette, base_name, folder, pixels):
    """Writes <base_name>_diffuse.png and <base_name>_specular.png and gives the object the material the engine reads"""
    show_only([mesh_object])
    palette_diffuse, palette_specular, palette_glow = palette.make_images(base_name)
    surface_masks = palette.make_surface_masks(base_name)
    detail = SurfaceDetailMaterial(base_name + "_bake", palette_diffuse, palette_specular, palette_glow, surface_masks)

    mesh_object.data.materials.clear()
    mesh_object.data.materials.append(detail.material)
    unwrap(mesh_object)

    diffuse = new_texture(base_name + "_diffuse", folder, pixels, is_color=True)
    specular = new_texture(base_name + "_specular", folder, pixels, is_color=False)
    detail.bake(mesh_object, detail.color, diffuse)
    detail.bake(mesh_object, detail.specular, specular)

    # The engine reads one UV map and one texture (the specular one is found by its name next to it)
    mesh_object.data.uv_layers.remove(mesh_object.data.uv_layers[PALETTE_UV])
    mesh_object.data.materials.clear()
    mesh_object.data.materials.append(make_textured_material(base_name, diffuse))


def select_only(objects):
    bpy.ops.object.select_all(action="DESELECT")
    for scene_object in objects:
        scene_object.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]


def export_gltf(path, objects, with_animations):
    select_only(objects)
    bpy.ops.export_scene.gltf(
        filepath=path,
        export_format="GLTF_SEPARATE",
        use_selection=True,
        export_yup=True,
        export_apply=False,
        export_texcoords=True,
        export_normals=True,
        export_materials="EXPORT",
        export_image_format="AUTO",
        export_skins=with_animations,
        export_animations=with_animations,
        export_animation_mode="ACTIONS",
        export_force_sampling=True,
        export_frame_step=1,
        export_def_bones=False,
        export_current_frame=False,
    )


def setup_preview_render(width=480, height=600, is_transparent=False):
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "TEXTURE"
    scene.display.shading.show_shadows = False
    scene.display.shading.show_object_outline = is_transparent
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.film_transparent = is_transparent
    world = bpy.data.worlds.new("PreviewWorld")
    world.color = (0.35, 0.37, 0.40)
    scene.world = world

    camera_data = bpy.data.cameras.new("PreviewCamera")
    camera_data.lens = 50
    camera = bpy.data.objects.new("PreviewCamera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    return camera


def place_preview_camera(camera, angle_degrees, distance=3.4, height=1.1, target_height=0.85):
    angle = math.radians(angle_degrees)
    camera.location = (math.sin(angle) * distance, math.cos(angle) * distance, height)
    direction = Vector((0, 0, target_height)) - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
