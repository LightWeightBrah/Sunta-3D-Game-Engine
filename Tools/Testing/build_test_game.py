"""Builds a windowed test copy of a sample project into out/test-games (or the folder named by SUNTA_TEST_FOLDER).
    python Tools/Testing/build_test_game.py <Project folder name> <test name> ["file::old::new" ...]
Each substitution is applied to a file of the copied 'res' before the game is built (shortening a fight for a test).
Prints the path of the built exe"""
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

REPO = Path(r"C:\Dev\Sunta-3D-Game-Engine")
SCRATCH = Path(os.environ.get("SUNTA_TEST_FOLDER", Path(__file__).resolve().parents[2] / "out" / "test-games"))

project, name = sys.argv[1], sys.argv[2]
work = Path(SCRATCH) / name
if work.exists():
    shutil.rmtree(work)
shutil.copytree(REPO / "Projects" / project / "res", work / "res")

config_path = work / "res" / "game.config"
config = json.loads(config_path.read_text(encoding="utf-8"))
config.update({"name": name, "fullscreen": False, "window_width": 1280, "window_height": 720})
config_path.write_text(json.dumps(config, indent=1), encoding="utf-8")

for substitution in sys.argv[3:]:
    file_name, old, new = substitution.split("::")
    path = work / "res" / file_name
    text = path.read_text(encoding="utf-8")
    if old not in text:
        raise SystemExit(f"NOT FOUND in {file_name}: {old}")
    path.write_text(text.replace(old, new), encoding="utf-8")

result = subprocess.run([str(REPO / "bin" / "Release-Windows-AMD64" / "SuntaEditor.exe"), "--build-game", str(work), "--development"],
                        capture_output=True, text=True)
if result.returncode != 0:
    print(result.stdout[-2000:])
    raise SystemExit("build failed")
print(work / "Build" / f"{name}_Development" / f"{name}.exe")
