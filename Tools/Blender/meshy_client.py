"""
A small client of the Meshy API (https://docs.meshy.ai): pictures from text, textured 3D models from pictures
and motion clips from text

The API key is read from the environment variable MESHY_API_KEY (never written anywhere).
Every task is written down in Generated/meshy_log.json: what was asked for, with which settings, and what came back,
so it is always known where a model of the project comes from

Plain Python (no Blender needed), for example:
    python Tools/Blender/meshy_client.py image "a knight ..." --pose a-pose --out Concepts/knight.png
    python Tools/Blender/meshy_client.py model Concepts/knight.png --pose a-pose --faces 30000 --out Generated/knight.glb
"""

import argparse
import base64
import datetime
import json
import os
import time
import urllib.error
import urllib.request

try:
    # Antivirus programs often put their own certificate in front of HTTPS: Windows trusts it, plain Python doesn't
    import truststore
    truststore.inject_into_ssl()
except ImportError:
    pass

API = "https://api.meshy.ai/openapi/v1"
BLENDER_FOLDER = os.path.dirname(os.path.abspath(__file__))
LOG_PATH = os.path.join(BLENDER_FOLDER, "Generated", "meshy_log.json")

POLL_SECONDS = 10
GIVE_UP_SECONDS = 30 * 60

# Pictures: a capable model that keeps A/T poses
IMAGE_MODEL = "nano-banana-2"

# Models: the newest generator, the Ultra geometry pass, PBR maps (the engine turns metal and roughness into its specular map),
# remeshed to a face count a game can draw
MODEL_SETTINGS = {
    "ai_model": "meshy-7.1",
    "geometry_resolution": "2k",
    "should_texture": True,
    "enable_pbr": True,
    "texture_resolution": "2k",
    "should_remesh": True,
    "topology": "triangle",
    "target_formats": ["glb"],
}


# ========== Requests ==========


def api_key():
    key = os.environ.get("MESHY_API_KEY")
    if not key:
        raise RuntimeError("MESHY_API_KEY is not set (Windows: setx MESHY_API_KEY \"...\", then restart the program)")
    return key


def request(method, path, body=None):
    data = json.dumps(body).encode("utf-8") if body is not None else None
    req = urllib.request.Request(API + path, data=data, method=method,
                                 headers={"Authorization": "Bearer " + api_key(), "Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=60) as response:
            return json.loads(response.read())
    except urllib.error.HTTPError as error:
        raise RuntimeError("Meshy {} {}: {} {}".format(method, path, error.code, error.read().decode("utf-8", "ignore")))


def wait_for(kind, task_id):
    """Asks for the task until it is done, returns the finished task"""
    started = time.time()
    while True:
        task = request("GET", "/{}/{}".format(kind, task_id))
        status = task.get("status")
        if status == "SUCCEEDED":
            return task
        if status in ("FAILED", "CANCELED", "EXPIRED"):
            raise RuntimeError("Meshy task {} {}: {}".format(task_id, status, task.get("task_error")))
        if time.time() - started > GIVE_UP_SECONDS:
            raise RuntimeError("Meshy task {} took too long".format(task_id))
        print("  {} {}% ...".format(kind, task.get("progress", 0)), flush=True)
        time.sleep(POLL_SECONDS)


def download(url, path):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "Sunta"}), timeout=300) as response:
        open(path, "wb").write(response.read())


def as_data_uri(path):
    kind = "png" if path.lower().endswith(".png") else "jpeg"
    return "data:image/{};base64,{}".format(kind, base64.b64encode(open(path, "rb").read()).decode("ascii"))


def write_log(entry):
    os.makedirs(os.path.dirname(LOG_PATH), exist_ok=True)
    log = json.load(open(LOG_PATH, encoding="utf-8")) if os.path.isfile(LOG_PATH) else []
    entry["date"] = datetime.datetime.now().isoformat(timespec="seconds")
    log.append(entry)
    json.dump(log, open(LOG_PATH, "w", encoding="utf-8"), indent=2, ensure_ascii=False)


def relative(path):
    return os.path.relpath(os.path.abspath(path), BLENDER_FOLDER).replace("\\", "/")


# ========== Tasks ==========


def make_image(prompt, out_path, pose_mode=None, aspect_ratio="3:4"):
    """A picture from text (with a transparent background), saved as PNG. Returns the task ID (a model can start from it)"""
    body = {"ai_model": IMAGE_MODEL, "prompt": prompt, "remove_background": True}
    if pose_mode:
        body["pose_mode"] = pose_mode
    else:
        body["aspect_ratio"] = aspect_ratio

    task_id = request("POST", "/text-to-image", body)["result"]
    task = wait_for("text-to-image", task_id)
    download(task["image_urls"][0], out_path)

    write_log({"task": "text-to-image", "task_id": task_id, "settings": body, "output": relative(out_path)})
    return task_id


def make_model(image_path, out_path, pose_mode=None, faces=30000):
    """A textured 3D model (GLB) from a picture"""
    body = dict(MODEL_SETTINGS, image_url=as_data_uri(image_path), target_polycount=faces)
    if pose_mode:
        body["pose_mode"] = pose_mode

    task_id = request("POST", "/image-to-3d", body)["result"]
    task = wait_for("image-to-3d", task_id)
    download(task["model_urls"]["glb"], out_path)

    settings = {key: value for key, value in body.items() if key != "image_url"}
    write_log({"task": "image-to-3d", "task_id": task_id, "input": relative(image_path), "settings": settings, "output": relative(out_path)})
    return task_id


def make_motion(prompt, seconds, out_path, task_id=None):
    """A motion clip (FBX, not tied to any character) from a description. 'task_id' picks up a task started before"""
    body = {"prompt": prompt, "mode": "prime", "duration": seconds}
    if task_id is None:
        task_id = request("POST", "/text-to-motion", body)["result"]
    task = wait_for("text-to-motion", task_id)
    download(task["result"]["motion_url"], out_path)

    write_log({"task": "text-to-motion", "task_id": task_id, "settings": body, "output": relative(out_path)})
    return task_id


def balance():
    return request("GET", "/balance")


def main():
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)

    image = commands.add_parser("image")
    image.add_argument("prompt")
    image.add_argument("--out", required=True)
    image.add_argument("--pose", default=None)
    image.add_argument("--aspect", default="3:4")

    model = commands.add_parser("model")
    model.add_argument("image")
    model.add_argument("--out", required=True)
    model.add_argument("--pose", default=None)
    model.add_argument("--faces", type=int, default=30000)

    motion = commands.add_parser("motion")
    motion.add_argument("prompt")
    motion.add_argument("--seconds", type=float, default=3.0)
    motion.add_argument("--out", required=True)

    commands.add_parser("balance")

    arguments = parser.parse_args()
    if arguments.command == "image":
        print(make_image(arguments.prompt, arguments.out, arguments.pose, arguments.aspect))
    elif arguments.command == "model":
        print(make_model(arguments.image, arguments.out, arguments.pose, arguments.faces))
    elif arguments.command == "motion":
        print(make_motion(arguments.prompt, arguments.seconds, arguments.out))
    else:
        print(balance())


if __name__ == "__main__":
    main()
