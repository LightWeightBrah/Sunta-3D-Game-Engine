"""A build of the game for testing the boss fight: the player starts right before the fog of the arena (not at the far
end of the street). Writes Projects/StrongestInHistoryVsToday/Build/Strongest in History Vs Today_ArenaTest/.
    python Projects/StrongestInHistoryVsToday/Tools/build_arena_test.py"""
import os
import shutil
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
PROJECT = REPO / "Projects" / "StrongestInHistoryVsToday"
OUT = PROJECT / "Build" / "Strongest in History Vs Today_ArenaTest"

result = subprocess.run([sys.executable, str(REPO / "Tools" / "Testing" / "build_test_game.py"), "StrongestInHistoryVsToday",
                         "SihArenaTest", "Scripts/player_techniques.lua::local StartAtArena = true::local StartAtArena = true"],
                        capture_output=True, text=True, env=dict(os.environ, SUNTA_BACKGROUND="1"))
if result.returncode != 0:
    print(result.stdout, result.stderr)
    raise SystemExit("build failed")
built = Path(result.stdout.strip().splitlines()[-1]).parent
if OUT.exists():
    shutil.rmtree(OUT)
shutil.copytree(built, OUT)
print(OUT / (built.name.split("_")[0] + ".exe"))
