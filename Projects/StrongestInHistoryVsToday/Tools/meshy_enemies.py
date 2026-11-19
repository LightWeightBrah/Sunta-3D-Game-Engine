"""The ordinary enemies of the street before the arena (made with Meshy: a picture in A-pose, a textured model of it,
Meshy's rigging). Steps can be run one at a time: python meshy_enemies.py concept|model|all [name]
Outputs: Tools/Blender/Concepts/StrongestInHistory/enemy_<name>.png, Tools/Blender/Generated/StrongestInHistory/
enemy_<name>.glb and enemy_<name>_rigged.glb (used by characters_generator.py)"""
import json
import os
import sys

BLENDER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "Tools", "Blender")
sys.path.insert(0, BLENDER)
import meshy_client  # noqa: E402

CONCEPTS = os.path.join(BLENDER, "Concepts", "StrongestInHistory")
GENERATED = os.path.join(BLENDER, "Generated", "StrongestInHistory")

# Made up people (no one real): the boss's men who keep the street to his arena
ENEMIES = {
    "guard": ("Full body photo of a big muscular bodyguard, a bald man in his forties in a black suit, white shirt, "
              "black tie, black sunglasses, an earpiece with a coiled wire, black leather gloves, black shoes, "
              "realistic, standing in A-pose, arms away from the body, empty hands, plain background", 1.9),
    "paparazzo": ("Full body photo of a scruffy paparazzi photographer, a thin man in his fifties with messy grey hair "
                  "and stubble, a beige photographer's vest with many pockets over a dark t-shirt, jeans, sneakers, "
                  "a professional camera on a strap over his shoulder hanging at his right hip, at his side (not in "
                  "front of him), realistic, standing straight in A-pose, arms away from the body, plain background",
                  1.76),
    # (the first try had the camera hanging in front of his belly: Meshy's rig then put his hips 12 cm behind
    # the body and every clip bent his head down)
}


def concept(name):
    os.makedirs(CONCEPTS, exist_ok=True)
    out = os.path.join(CONCEPTS, f"enemy_{name}.png")
    task = meshy_client.make_image(ENEMIES[name][0], out, pose_mode="a-pose")
    print("CONCEPT", name, task, out, flush=True)
    return out


def model(name):
    image = os.path.join(CONCEPTS, f"enemy_{name}.png")
    out = os.path.join(GENERATED, f"enemy_{name}.glb")
    task = meshy_client.make_model(image, out, pose_mode="a-pose", faces=20000)
    print("MODEL", name, task, flush=True)
    rigged = os.path.join(GENERATED, f"enemy_{name}_rigged.glb")
    meshy_client.rig_model(task, ENEMIES[name][1], rigged)
    print("RIGGED", name, rigged, json.dumps(meshy_client.balance()), flush=True)


if __name__ == "__main__":
    step = sys.argv[1]
    names = sys.argv[2:] or list(ENEMIES)
    for enemy in names:
        if step in ("concept", "all"):
            concept(enemy)
        if step in ("model", "all"):
            model(enemy)
