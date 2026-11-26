"""
The 3D models of Grave Crown from the concept variants the user picked (AIContext picker, 2026-11-26): Meshy Image to 3D,
then Meshy Rigging for humanoids (beasts get the CC0 Mesh2Motion quadruped skeletons in Blender instead)

    python Projects/GraveCrown/Tools/models_meshy.py               # every model not made yet
    python Projects/GraveCrown/Tools/models_meshy.py enemy_knight  # only names starting with it

Everything lands in Tools/Blender/Generated/GraveCrown/<name>.glb (+ <name>_rigged.glb) and in meshy_log.json
"""

import concurrent.futures
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "Tools", "Blender"))
import meshy_client  # noqa: E402

VARIANTS = os.path.join(ROOT, "Tools", "Blender", "Concepts", "ThirdGame", "Variants")
OUT = os.path.join(ROOT, "Tools", "Blender", "Generated", "GraveCrown")

# name: (concept picture, A-pose, rig height in meters or None for no rig, faces)
MODELS = {
    "player":         ("player_a.png", True, 2.0, 30000),
    "horse":          ("horse_b.png", False, None, 30000),
    "enemy_cultist":  ("enemy_cultist_a.png", True, 1.8, 30000),   # made before the final pick (b), kept as a spare
    "enemy_cultist_b": ("enemy_cultist_b.png", True, 1.8, 30000),
    "enemy_knight":   ("enemy_knight_a.png", True, 1.95, 30000),
    "enemy_wolf":     ("enemy_wolf_b.png", False, None, 25000),
    "enemy_rider":    ("enemy_rider_a.png", True, 1.95, 30000),
    "court_knight":   ("court_knight_b.png", True, 1.95, 30000),
    "weapon_greatsword": ("weapon_greatsword.png", False, None, 8000),
    # Round 2 of the picker
    "enemy_golem":    ("enemy_golem_a.png", True, 2.6, 30000),
    "titan":          ("titan_a.png", True, 2.0, 30000),    # rigged at a man's height, scaled up in the scene
    "enemy_skeleton_horse": ("enemy_skeleton_horse_g1.png", False, None, 25000),
    "court_lich":     ("court_lich_g1b.png", True, 1.95, 30000),
    "court_butler":   ("court_butler_f.png", True, 1.85, 30000),
}


def task_id_of(output):
    """The Image to 3D task that made this model, from the log (rigging needs it)"""
    log = json.load(open(meshy_client.LOG_PATH, encoding="utf-8"))
    wanted = os.path.relpath(output, meshy_client.BLENDER_FOLDER).replace("\\", "/")
    for entry in reversed(log):
        if entry.get("task") == "image-to-3d" and entry.get("output") == wanted:
            return entry["task_id"]
    return None


def make(name, picture, is_apose, rig_height, faces):
    model = os.path.join(OUT, name + ".glb")
    rigged = os.path.join(OUT, name + "_rigged.glb")
    try:
        if not os.path.isfile(model):
            meshy_client.make_model(os.path.join(VARIANTS, picture), model, "a-pose" if is_apose else None, faces)
        if rig_height and not os.path.isfile(rigged):
            meshy_client.rig_model(task_id_of(model), rig_height, rigged)
        return name, "ok"
    except Exception as error:
        return name, "FAILED " + str(error)[:300]


def main():
    prefix = sys.argv[1] if len(sys.argv) > 1 else ""
    jobs = {name: job for name, job in MODELS.items() if name.startswith(prefix)}
    with concurrent.futures.ThreadPoolExecutor(max_workers=5) as pool:
        for name, result in pool.map(lambda item: make(item[0], *item[1]), jobs.items()):
            print(name, result, flush=True)
    print(meshy_client.balance())


if __name__ == "__main__":
    main()
