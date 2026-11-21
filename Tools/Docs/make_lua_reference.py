"""Writes docs/scripting-reference.md: what a Lua script of the engine can reach, read out of the engine's bindings
(Sunta/src/Sunta/Scripting/LuaBindings.cpp): each type (this.transform, this.health, this.world, ...), its fields and
functions, and the comments written next to them ("Usage in Lua: ..."), so it is as current as the engine.
Run after changing the bindings: python Tools/Docs/make_lua_reference.py"""
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
BINDINGS = REPO / "Sunta" / "src" / "Sunta" / "Scripting" / "LuaBindings.cpp"
OUT = REPO / "docs" / "scripting-reference.md"

USERTYPE = re.compile(r'lua\.new_usertype<[\w:]+>\("(\w+)"')
MEMBER = re.compile(r'^\s*"(\w+)"\s*,')


def comments_before(lines, index, keep_lines=False):
    """The // lines right above line 'index' (a blank line ends them)"""
    out = []
    i = index - 1
    while i >= 0 and lines[i].strip().startswith("//"):
        out.insert(0, lines[i].strip()[2:].strip())
        i -= 1
    return out if keep_lines else " ".join(out)


def about_markdown(comment_lines):
    """The explanation as a paragraph, and the examples after 'Usage in Lua:' as a block of Lua"""
    text, code = [], []
    is_code = False
    for line in comment_lines:
        if "Usage in Lua:" in line:
            before, after = line.split("Usage in Lua:", 1)
            is_code = True
            if before.strip():
                text.append(before.strip())
            if after.strip():
                code.append(after.strip())
        elif is_code:
            code.append(line)
        else:
            text.append(line)
    out = []
    if text:
        out += [" ".join(text), ""]
    if code:
        # (a note in brackets after the code, as the engine writes it, becomes a Lua comment)
        code = [re.sub(r"(\S)(\s{2,})\((.*)\)\s*$", r"\1\2-- \3", line) for line in code]
        out += ["```lua"] + code + ["```", ""]
    return out


def collect():
    lines = BINDINGS.read_text(encoding="utf-8").splitlines()
    types = []
    current = None
    depth = 0
    for index, line in enumerate(lines):
        match = USERTYPE.search(line)
        if match:
            current = {"name": match.group(1), "about": comments_before(lines, index, keep_lines=True), "members": []}
            types.append(current)
            depth = 0
        if current is None:
            continue
        member = MEMBER.match(line)
        if member and member.group(1) != current["name"]:
            current["members"].append((member.group(1), comments_before(lines, index)))
        depth += line.count("(") - line.count(")")
        if depth <= 0 and line.rstrip().endswith(");"):
            current = None
    return types


def write(types):
    out = ["---", "title: Scripting reference", "---", "", "# Scripting reference", "",
           "What a Lua script can reach. `this` is the entity the script is on; each of its components is a field of it",
           "(`this.transform`, `this.health`, ...), and `this.world` is the scene. Read out of the engine's bindings",
           "(`Sunta/src/Sunta/Scripting/LuaBindings.cpp`), with the notes written next to them.", "",
           "See [Scripting](scripting.md) for how scripts are written and when their functions are called.", ""]
    for kind in types:
        if not kind["members"]:
            continue
        out += [f"## {kind['name']}", ""]
        out += about_markdown(kind["about"])
        out += ["| Name | Note |", "|---|---|"]
        for name, note in kind["members"]:
            out.append(f"| `{name}` | {note.replace('|', '/')} |")
        out.append("")
    OUT.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"{len(types)} types -> {OUT}")


if __name__ == "__main__":
    write(collect())
