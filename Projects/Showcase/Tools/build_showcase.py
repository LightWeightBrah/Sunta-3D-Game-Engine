"""Builds the small scenes of the Showcase project (pictures of the engine's features for the README and the docs):
    python Projects/Showcase/Tools/build_showcase.py
Scenes/Physics.scene: a pyramid of crates, boxes falling onto it turned every way, a heavy block thrown into it."""
import json
import math
from pathlib import Path

RES = Path(__file__).resolve().parents[1] / "res"


def quaternion(degrees):
    """Euler degrees (x, y, z, applied Y * X * Z as the engine does) to w, x, y, z"""
    x, y, z = (math.radians(value) * 0.5 for value in degrees)
    cx, sx, cy, sy, cz, sz = math.cos(x), math.sin(x), math.cos(y), math.sin(y), math.cos(z), math.sin(z)
    # q = qy * qx * qz
    w = cy * cx * cz + sy * sx * sz
    qx = cy * sx * cz + sy * cx * sz
    qy = sy * cx * cz - cy * sx * sz
    qz = cy * cx * sz - sy * sx * cz
    return [w, qx, qy, qz]


def entity(name, position, scale=(1.0, 1.0, 1.0), rotation=(0.0, 0.0, 0.0), **components):
    return {"tag": {"name": name},
            "transform": {"position": list(position), "rotation": list(rotation),
                          "rotation_quaternion": quaternion(rotation), "scale": list(scale)},
            **components}


def cube(material):
    return {"is_visible": True, "material_name": material, "mesh_name": "cube"}


def body(mass=1.0, velocity=(0.0, 0.0, 0.0), spin=(0.0, 0.0, 0.0), restitution=0.15, friction=0.6):
    return {"is_kinematic": False, "use_gravity": True, "velocity": list(velocity), "angular_velocity": list(spin),
            "mass": mass, "restitution": restitution, "friction": friction,
            "linear_damping": 0.02, "angular_damping": 0.05}


def physics_scene():
    entities = [
        entity("Sun", (0.0, 8.0, 0.0), rotation=(-55.0, -35.0, 0.0),
               directional_light={"ambient": [0.28, 0.27, 0.3], "diffuse": [0.85, 0.8, 0.72], "specular": [1.0, 1.0, 1.0]}),
        entity("Floor", (0.0, -0.5, 0.0), scale=(40.0, 1.0, 40.0), mesh=cube("default_material"),
               box_collider={"half_extents": [0.5, 0.5, 0.5]}),
        entity("Sky", (0.0, 0.0, 0.0), environment={"skybox_folder": "@game/Skyboxes/BurningDusk", "sky_brightness": 1.0,
                                                    "is_fog_enabled": False}),
        entity("Camera", (0.0, 4.2, 13.0), rotation=(-13.0, 0.0, 0.0),
               camera={"far_plane": 200.0, "field_of_view": 50.0, "is_active": True, "near_plane": 0.1}),
    ]
    # A pyramid of crates: 5 at the bottom
    number = 0
    for row in range(5):
        for column in range(5 - row):
            number += 1
            x = (column - (4 - row) * 0.5) * 1.02
            entities.append(entity(f"Crate {number}", (x, 0.5 + row * 1.0, 0.0), mesh=cube("textured_material"),
                                   physics_body=body(), box_collider={"half_extents": [0.5, 0.5, 0.5]}))
    # Boxes dropped from high, turned every way (they tumble when they land on a corner)
    for index, (x, z, height, turn) in enumerate(((-3.0, 2.5, 7.0, (35, 20, 50)), (2.5, 3.0, 9.0, (60, 45, 10)),
                                                  (-1.0, -3.0, 11.0, (15, 70, 40)), (3.5, -2.0, 8.0, (45, 0, 45)))):
        entities.append(entity(f"Falling Box {index + 1}", (x, height, z), rotation=turn, mesh=cube("default_material"),
                               physics_body=body(mass=1.5), box_collider={"half_extents": [0.5, 0.5, 0.5]}))
    # A heavy block thrown into the pyramid from the side
    entities.append(entity("Thrown Block", (-12.0, 2.5, 0.0), scale=(1.4, 1.4, 1.4), mesh=cube("unlit_material"),
                           physics_body=body(mass=8.0, velocity=(14.0, 2.5, 0.0), spin=(0.0, 0.0, -3.0), restitution=0.2),
                           box_collider={"half_extents": [0.5, 0.5, 0.5]}))
    return {"entities": entities, "scene_name": "Physics"}


def animation_scene():
    """The engine's own characters, each playing another animation of its model (bones, skinning, the clips of a
    glTF), and a camera circling around them"""
    entities = [
        entity("Sky", (0.0, 0.0, 0.0), environment={"skybox_folder": "@game/Skyboxes/BurningDusk", "sky_brightness": 1.0,
                                                    "is_fog_enabled": False}),
        entity("Sun", (0.0, 8.0, 0.0), rotation=(-50.0, -30.0, 0.0),
               directional_light={"ambient": [0.3, 0.28, 0.3], "diffuse": [0.9, 0.82, 0.72], "specular": [1.0, 1.0, 1.0]}),
        entity("Floor", (0.0, -0.5, 0.0), scale=(40.0, 1.0, 40.0), mesh=cube("default_material")),
        entity("Camera", (0.0, 2.6, 7.5), rotation=(-12.0, 0.0, 0.0),
               camera={"far_plane": 200.0, "field_of_view": 50.0, "is_active": True, "near_plane": 0.1},
               script=[{"script_path": "@game/Scripts/orbit_camera.lua"}]),
    ]
    actors = (("knight", "Idle", (-3.0, 0.0, 0.0)), ("knight", "Attack_Light_1", (-1.5, 0.0, -1.8)),
              ("rusted_knight", "Attack_Heavy", (1.5, 0.0, -1.8)), ("rusted_knight", "Roll", (3.0, 0.0, 0.0)),
              ("knight", "Block", (0.0, 0.0, 2.0)), ("boss", "Fury_Combo", (0.0, 0.0, -4.0)))
    for index, (model, clip, place) in enumerate(actors):
        entities.append(entity(f"{model} {clip}", place, rotation=(0.0, (index * 60.0) % 360.0, 0.0),
                               model={"is_visible": True, "model_name": model},
                               animator={"current_animation_name": clip}))
    return {"entities": entities, "scene_name": "Animation"}


def main():
    (RES / "Scenes").mkdir(parents=True, exist_ok=True)
    (RES / "Scenes" / "Physics.scene").write_text(json.dumps(physics_scene(), indent=1), encoding="utf-8")
    (RES / "Scenes" / "Animation.scene").write_text(json.dumps(animation_scene(), indent=1), encoding="utf-8")
    config = {"name": "Showcase", "start_scene": "@game/Scenes/Physics.scene", "window_width": 1280,
              "window_height": 720, "fullscreen": False, "vsync": True}
    (RES / "game.config").write_text(json.dumps(config, indent=1), encoding="utf-8")
    print("Showcase scenes written to", RES)


if __name__ == "__main__":
    main()
