"""Counts the lines of source code of the engine and of each game project and writes the table into README.md
(between the markers <!-- LINES-OF-CODE:START --> and <!-- LINES-OF-CODE:END -->).

    python Tools/count_lines.py            write the table into README.md
    python Tools/count_lines.py --check    only print it; exit 1 if README.md is out of date

Only files tracked by git count, and only lines with something on them (blank lines don't). Third-party code
(Dependencies/) and generated files (scenes, models, the generated reference in docs/) are not source code.
Run by .github/workflows/lines-of-code.yml on every push to main, so the README stays current by itself.
"""
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
README = REPO / "README.md"
START, END = "<!-- LINES-OF-CODE:START -->", "<!-- LINES-OF-CODE:END -->"

LANGUAGES = {
    ".cpp": "C++", ".h": "C++", ".hpp": "C++", ".inl": "C++",
    ".shader": "GLSL", ".glsl": "GLSL",
    ".lua": "Lua",
    ".py": "Python",
    ".js": "JavaScript",
    ".cmake": "CMake", "CMakeLists.txt": "CMake",
    ".ps1": "Shell", ".sh": "Shell",
}

# (title, the folders whose files count; the first part that matches a file takes it)
ENGINE_PARTS = [
    ("Engine core (`Sunta/src`, without the editor)", ["Sunta/src/"], ["Sunta/src/Sunta/Editor/"]),
    ("Editor (`Sunta/src/Sunta/Editor`, `Apps/Editor`)", ["Sunta/src/Sunta/Editor/", "Apps/Editor/"], []),
    ("Game runtime (`Apps/Runtime`)", ["Apps/Runtime/"], []),
    ("Shaders and engine scripts (`Sunta/res`)", ["Sunta/res/"], []),
    ("Build (CMake, `cmake/`)", ["CMakeLists.txt", "cmake/", "Sunta/CMakeLists.txt", "Apps/CMakeLists.txt",
                                 "Apps/Editor/CMakeLists.txt", "Apps/Runtime/CMakeLists.txt"], []),
    ("Tools (`Tools/`: asset generators, tests, docs)", ["Tools/"], []),
]


def tracked_files():
    output = subprocess.run(["git", "ls-files"], cwd=REPO, capture_output=True, text=True, check=True).stdout
    return [line for line in output.splitlines() if line]


def language_of(path):
    name = Path(path).name
    return LANGUAGES.get(name) or LANGUAGES.get(Path(path).suffix.lower())


def count_lines(path):
    try:
        text = (REPO / path).read_text(encoding="utf-8", errors="replace")
    except OSError:
        return 0
    return sum(1 for line in text.splitlines() if line.strip())


def is_under(path, prefix):
    return path == prefix or path.startswith(prefix)


def measure():
    files = [path for path in tracked_files() if language_of(path) and not path.startswith("Dependencies/")]
    lines = {path: count_lines(path) for path in files}

    engine = []
    claimed = set()
    # (CMake files belong to the build, wherever they are)
    cmake_files = {path for path in files if language_of(path) == "CMake"}
    for title, prefixes, excluded in ENGINE_PARTS:
        if title.startswith("Build"):
            chosen = [path for path in cmake_files if not path.startswith("Projects/")]
        else:
            chosen = [path for path in files if path not in cmake_files and path not in claimed
                      and any(is_under(path, prefix) for prefix in prefixes)
                      and not any(is_under(path, prefix) for prefix in excluded)]
        claimed.update(chosen)
        engine.append((title, chosen))

    projects = {}
    for path in files:
        parts = path.split("/")
        if parts[0] == "Projects" and len(parts) > 2:
            projects.setdefault(parts[1], []).append(path)
    games = sorted(((name, chosen) for name, chosen in projects.items() if sum(lines[p] for p in chosen) > 0),
                   key=lambda item: -sum(lines[p] for p in item[1]))
    return lines, engine, games


def languages_of(paths, lines):
    totals = {}
    for path in paths:
        totals[language_of(path)] = totals.get(language_of(path), 0) + lines[path]
    return ", ".join(f"{name} {count:,}" for name, count in sorted(totals.items(), key=lambda item: -item[1]) if count)


def table():
    lines, engine, games = measure()
    rows = ["| Part | Lines of code | Files | Languages |", "|---|---:|---:|---|"]
    engine_total = engine_files = 0
    for title, paths in engine:
        total = sum(lines[p] for p in paths)
        engine_total += total
        engine_files += len(paths)
        rows.append(f"| {title} | {total:,} | {len(paths)} | {languages_of(paths, lines)} |")
    rows.append(f"| **Engine in all** | **{engine_total:,}** | **{engine_files}** | |")
    games_total = games_files = 0
    for name, paths in games:
        total = sum(lines[p] for p in paths)
        games_total += total
        games_files += len(paths)
        rows.append(f"| Game: [`{name}`](Projects/{name}) | {total:,} | {len(paths)} | {languages_of(paths, lines)} |")
    rows.append(f"| **Games in all** | **{games_total:,}** | **{games_files}** | |")
    rows.append(f"| **Everything** | **{engine_total + games_total:,}** | **{engine_files + games_files}** | |")
    note = ("Non-empty lines of the files tracked by git, without third-party code (`Dependencies/`) and generated "
            "files (scenes, models). Counted by [`Tools/count_lines.py`](Tools/count_lines.py) on every push to main.")
    return "\n".join([START, "", *rows, "", note, "", END])


def main():
    text = README.read_text(encoding="utf-8")
    if START not in text or END not in text:
        raise SystemExit(f"README.md has no {START} ... {END} markers")
    before, rest = text.split(START, 1)
    _, after = rest.split(END, 1)
    updated = before + table() + after
    if "--check" in sys.argv:
        print(table())
        sys.exit(0 if updated == text else 1)
    if updated != text:
        README.write_text(updated, encoding="utf-8", newline="\n")
        print("README.md: lines of code updated")
    else:
        print("README.md: lines of code already current")


if __name__ == "__main__":
    main()
