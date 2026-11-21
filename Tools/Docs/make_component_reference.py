"""Writes docs/components.md: every component of the engine as the Inspector shows it (Add Component > ...), with the
explanation written above it in the engine's code and the fields it has (the same names the Inspector shows).
Run after changing a component: python Tools/Docs/make_component_reference.py"""
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SOURCE = REPO / "Sunta" / "src" / "Sunta"
OUT = REPO / "docs" / "components.md"

# The groups of the reference, in order (the rest goes under "Other")
GROUPS = [
    ("Scene and rendering", ["Transform", "Tag", "Parent", "Mesh", "Model", "Animator", "Camera", "Directional Light",
                             "Point Light", "Spotlight", "Environment", "Glow Sprite", "Liquid Orb"]),
    ("Physics", ["Physics Body", "Box Collider"]),
    ("The player and characters", ["Character Controller", "Character Actions", "Character Animation",
                                   "Character Sounds", "Strike Aim", "Health", "Stamina", "Poise", "Shield", "Hitbox",
                                   "Lock On", "Lock On Target", "Healing Flask", "Player HUD", "Respawn", "Level Up",
                                   "Currency", "Weapon Trail"]),
    ("Enemies and bosses", ["Enemy", "Melee Enemy", "Enemy Health Bar", "Currency Reward", "Boss", "Fog Gate",
                            "Spell Caster", "Projectile"]),
    ("The world", ["Bonfire", "Cutscene", "Audio Source", "Script"]),
    ("Effects", ["Particle Emitter", "Effect Shape", "Swarm"]),
]

REGISTER = re.compile(r'RegisterComponent<(\w+)>\("([^"]+)"')
PROPERTY = re.compile(r'ADD_PROPERTY(?:_RANGED)?\(\s*\w+\s*,\s*(\w+)\s*,\s*PropertyDataType::(\w+)')


def comment_above(text, struct_name):
    """The block of // lines right above 'struct <name>'"""
    match = re.search(r"((?:^//[^\n]*\n)+)\s*struct\s+" + struct_name + r"\b", text, re.MULTILINE)
    if not match:
        return ""
    # (the indentation is kept: an indented line is an item of a list, or the rest of the item above it)
    lines = [line[3:] if line.startswith("// ") else line[2:] for line in match.group(1).strip().splitlines()]
    return "\n".join(line.rstrip() for line in lines)


LIST_ITEM = re.compile(r"^\s{1,3}(\S[^\s]*(?: \S+)?)\s{2,}(.*)$")   # "  term     what it is"
DASH_ITEM = re.compile(r"^\s*-\s+(.*)$")


def to_paragraphs(comment):
    """Comment lines to Markdown: paragraphs are joined, indented 'term   text' lines and '-' lines become a list,
    deeper indented lines go on the item above them"""
    blocks = []   # [kind, text] with kind "p" (a paragraph) or "li" (an item)
    for line in comment.splitlines():
        if not line.strip():
            blocks.append(["gap", ""])
            continue
        item = LIST_ITEM.match(line)
        dash = DASH_ITEM.match(line)
        if item:
            blocks.append(["li", f"`{item.group(1)}` {item.group(2)}"])
        elif dash:
            blocks.append(["li", dash.group(1)])
        elif line.startswith("    ") and blocks and blocks[-1][0] == "li":
            blocks[-1][1] += " " + line.strip()
        elif blocks and blocks[-1][0] == "p":
            blocks[-1][1] += " " + line.strip()
        else:
            blocks.append(["p", line.strip()])

    out = []
    previous = None
    for kind, text in blocks:
        if kind == "gap":
            previous = None
            continue
        if out and (kind != previous or kind == "p"):
            out.append("")
        out.append(("- " + text) if kind == "li" else text)
        previous = kind
    return "\n".join(out)


def snake(name):
    return re.sub(r"(?<!^)(?=[A-Z])", "_", name).lower()


def collect():
    components = {}
    for header in SOURCE.rglob("*.h"):
        text = header.read_text(encoding="utf-8", errors="ignore")
        for struct_name, display in REGISTER.findall(text):
            # The properties registered for this component (the block after its RegisterComponent)
            start = text.find(f"RegisterComponent<{struct_name}>")
            end = text.find("});", start)
            fields = PROPERTY.findall(text[start:end])
            components[display] = {
                "struct": struct_name,
                "file": header.relative_to(REPO).as_posix(),
                "comment": comment_above(text, struct_name),
                "fields": fields,
            }
    return components


def write(components):
    lines = ["---", "title: Components", "---", "", "# Components", "",
             "Every part an entity can have, as **Add Component** in the Inspector lists it. The text comes from the engine's",
             "own code (each component explains itself above its definition), so it is as current as the engine.",
             "Field names are the ones of the engine's code (the Inspector shows them as words).", ""]
    used = set()
    for group, names in GROUPS:
        lines += [f"## {group}", ""]
        for name in names:
            if name in components:
                lines += section(name, components[name])
                used.add(name)
    rest = sorted(set(components) - used)
    if rest:
        lines += ["## Other", ""]
        for name in rest:
            lines += section(name, components[name])
    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"{len(components)} components -> {OUT}")


def section(name, info):
    out = [f"### {name}", ""]
    if info["comment"]:
        out += [to_paragraphs(info["comment"]), ""]
    if info["fields"]:
        out += ["| Field | Type |", "|---|---|"]
        for field, kind in info["fields"]:
            out.append(f"| `{field}` | {kind} |")
        out.append("")
    out += [f"<sub>Defined in `{info['file']}`</sub>", ""]
    return out


if __name__ == "__main__":
    write(collect())
