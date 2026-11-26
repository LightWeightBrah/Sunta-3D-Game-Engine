"""
A-pose concept variants of every Grave Crown character, made with Meshy from crops of the approved concept art
(Image to Image keeps the look of the drawing) or from text where there is no drawing yet. The user picks one variant
of each character before any 3D model is made (models cost far more credits than pictures)

    python Projects/GraveCrown/Tools/concept_variants.py            # everything not made yet
    python Projects/GraveCrown/Tools/concept_variants.py player     # only names starting with "player"

Pictures go to Tools/Blender/Concepts/ThirdGame/Variants/<name>.png, crops to .../Variants/crops/
"""

import os
import sys
import concurrent.futures

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "Tools", "Blender"))
import meshy_client  # noqa: E402

CONCEPTS = os.path.join(ROOT, "Tools", "Blender", "Concepts", "ThirdGame")
OUT = os.path.join(CONCEPTS, "Variants")

STYLE = ("dark fantasy, painterly realistic concept art, NOT anime, muted colors, game character model reference sheet, "
         "single character, full body, front view, plain flat light grey background, even soft lighting, no text")
APOSE = ("standing in a neutral A-pose with arms held straight down and out at 30 degrees from the body, legs slightly apart, "
         "empty open hands holding nothing")
BEAST = "standing on all four legs in a neutral side view, whole body visible from nose to tail, legs straight"

# name: (source picture, crop box or None, prompt). A crop uses Image to Image, None uses Text to Image
VARIANTS = {
    # The player: the dead king in black armor with violet fire (sovereign_hero.png - the user's favourite)
    "player_a": ("sovereign_hero.png", (90, 20, 720, 768),
                 "Exactly the same undead king, keep every detail of the design: black cracked plate armor with violet fire glowing in the cracks, "
                 "horned spiked crown fused to the helmet with glowing violet eyes, round pauldrons, chain across the chest, crimson and gold tabard, "
                 "tattered dark crimson cape hanging straight down behind, no sword, no chain in the hand. " + APOSE),
    "player_b": ("sovereign_hero.png", (90, 20, 720, 768),
                 "The same undead king, but a leaner gothic silhouette: long tattered black coat over the cracked black plate, "
                 "tall thin crown of iron spikes on a closed helmet, violet fire in the cracks, iron manacle on the left wrist, no weapon. " + APOSE),
    # The black horse
    "horse_a": ("dark_hero_horse.png", (780, 50, 1376, 768),
                "The same black war horse with a scratched iron head plate and chest plate, dark leather saddle and red saddle cloth, no rider. " + BEAST),
    "horse_b": ("dark_hero_horse.png", (780, 50, 1376, 768),
                "The same black war horse but undead: gaunt, dark ragged mane, faint violet fire in the eyes and between the plates of the barding, "
                "black iron barding, dark saddle, no rider. " + BEAST),
    # Enemies from the approved lineup
    "enemy_knight_a": ("dark_enemies.png", (10, 200, 350, 768),
                       "The same hollow knight of the gods in dented dark plate armor with a closed bucket helm and a torn red cloth, no sword. " + APOSE),
    "enemy_knight_b": ("dark_enemies.png", (10, 200, 350, 768),
                       "The same hollow knight but a gothic church guard: dark plate with a white tabard bearing a chain sigil, tall pointed helm, no sword. " + APOSE),
    "enemy_cultist_a": ("dark_enemies.png", (330, 220, 600, 768),
                        "The same hooded cultist with a golden mask and a long dark robe with glowing orange runes, no flail. " + APOSE),
    "enemy_cultist_b": ("dark_enemies.png", (330, 220, 600, 768),
                        "The same cultist as a gothic street zealot: wide-brimmed hat over the hood, ragged long coat, beaked iron mask, lantern on the belt, no weapon. " + APOSE),
    "enemy_wolf_a": ("dark_enemies.png", (520, 350, 880, 720),
                     "The same corrupted wolf with dark brown fur and glowing orange cracks in its skin. " + BEAST),
    "enemy_wolf_b": ("dark_enemies.png", (520, 350, 880, 720),
                     "The same wolf as a gaunt gothic beast: huge, long-limbed, matted black fur, exposed ribs, ember-orange eyes. " + BEAST),
    "enemy_golem_a": ("dark_enemies.png", (760, 140, 1110, 700),
                      "The same obsidian golem of black stone plates with glowing lava cracks and one burning eye. " + APOSE),
    "enemy_golem_b": ("dark_enemies.png", (760, 140, 1110, 700),
                      "The same golem but a chained cathedral idol: grey-black stone with iron chains wrapped around its arms, cracks glowing orange. " + APOSE),
    "enemy_rider_a": ("dark_enemies.png", (1050, 100, 1376, 700),
                      "Only the horned armored rider, standing on the ground without the horse: spiked black armor, horned helm, no spear. " + APOSE),
    "enemy_skeleton_horse_a": ("dark_enemies.png", (1050, 100, 1376, 700),
                               "Only the skeletal horse without its rider: bone horse with a fire glowing orange inside the rib cage, horse skull with burning eyes. " + BEAST),
    # The court in the crypt
    "court_butler_a": ("ashen_sovereign_v6.png", (0, 430, 768, 860),
                       "Only the skeleton butler from the picture: a dignified skeleton in a black tailcoat and white shirt with a cravat, white gloves. " + APOSE),
    "court_butler_b": (None, None,
                       "A skeleton butler of a dead king's crypt: a tall skeleton in an old black tailcoat with tarnished silver buttons, a waistcoat, "
                       "a monocle in an eye socket, a ring of keys on the belt. " + APOSE),
    # (the user: bare leg bones under a tailcoat look odd - fully dressed, only the skull and the hands are bone)
    "court_butler_c": (None, None,
                       "A skeleton butler of a dead king's crypt, FULLY DRESSED: only the skull and the bony hands are visible, everything else "
                       "covered - a long black tailcoat, a dark waistcoat, a high white collar and cravat, long black trousers down to "
                       "polished black shoes, white gloves held in one hand, upright dignified posture. " + APOSE),
    "court_butler_d": (None, None,
                       "An old skeleton majordomo, FULLY DRESSED in heavy dark velvet livery of a fallen royal house: a long coat with tarnished "
                       "gold trim down to the knees, a waistcoat, black breeches and long boots, a chain of keys at the belt, only the skull "
                       "and bony hands visible, a faint violet light in the eye sockets. " + APOSE),
    "court_lich_a": ("ashen_sovereign_v6.png", (0, 430, 768, 860),
                     "Only the lich advisor from the picture: a skeletal sorcerer in a dark hooded robe with blue ghostly light, an open book hanging at the belt. " + APOSE),
    "court_lich_b": (None, None,
                     "A lich advisor and court scholar: a gaunt skeletal sage in layered black and deep blue robes embroidered with silver runes, "
                     "a tall collar, a small tarnished circlet, pale blue light in the eye sockets, scroll cases at the belt. " + APOSE),
    "court_knight_a": ("ashen_sovereign_v6.png", (0, 430, 768, 860),
                       "Only one of the undead court knights from the picture: dark plate armor with violet fire burning from the helmet, no weapon. " + APOSE),
    "court_knight_b": (None, None,
                       "An undead royal guard knight: blackened plate armor with a violet flame burning in the eye slit of a tall helm, "
                       "a faded royal crest on the breastplate, a short torn cape. " + APOSE),
    # The companion (freed slave warrior)
    "companion_a": (None, None,
                    "A freed slave warrior woman: lean and scarred, short dark hair, a broken iron collar still on her neck, mismatched leather armor "
                    "and a chainmail shirt, wrapped forearms, a ragged hooded cloak hanging behind. " + APOSE),
    "companion_b": (None, None,
                    "A former gladiator slave woman, now a knight of a dead king: dark violet-black leather and plate pieces, broken shackles on the wrists, "
                    "long braid, a pale scar across the face, determined look. " + APOSE),
    "companion_c": (None, None,
                    "A young beast-hunter woman freed from chains, gothic style: long dark coat with a short cape, tricorn hat, leather gloves, "
                    "an iron collar broken open around her neck, belts and vials. " + APOSE),
    # The titan
    "titan_a": ("ashen_sovereign_v6.png", (768, 0, 1536, 430),
                "Only the colossal stone titan, a ruined temple city on its back, mossy rock skin with golden runes, chains hanging from its arms, "
                "no background landscape. " + APOSE),
    "titan_b": ("E_skuty_tytan.png", (300, 60, 860, 680),
                "Only the colossal chained titan, a dark armored giant wrapped in heavy iron chains, spikes on the shoulders, red eyes, "
                "whole body visible, no background landscape. " + APOSE),
}

WEAPONS = {
    "weapon_greatsword": ("sovereign_hero.png", (20, 380, 380, 720),
                          "Only the huge dark iron greatsword with runes on the blade, isolated, lying horizontally across the picture, whole sword visible"),
    "weapon_flail": ("dark_enemies.png", (330, 180, 620, 400),
                     "Only the flail: a wooden handle, an iron chain and a spiked iron ball, isolated, lying horizontally, whole weapon visible"),
    "weapon_knight_sword": ("dark_enemies.png", (10, 290, 350, 500),
                            "Only the rusty longsword, isolated, lying horizontally across the picture, whole sword visible"),
    "weapon_spear": ("dark_enemies.png", (1040, 40, 1140, 560),
                     "Only the long barbed iron spear, isolated, lying horizontally across the picture, whole spear visible"),
}

WEAPON_STYLE = "dark fantasy painterly realistic, plain flat light grey background, no text"


def make(name, source, box, prompt):
    out = os.path.join(OUT, name + ".png")
    if os.path.isfile(out):
        return name, "exists"
    full_prompt = prompt + ". " + (WEAPON_STYLE if name.startswith("weapon") else STYLE)
    try:
        if source is None:
            meshy_client.make_image(full_prompt, out, aspect_ratio="3:4")
        else:
            from PIL import Image
            crop = os.path.join(OUT, "crops", name + ".png")
            os.makedirs(os.path.dirname(crop), exist_ok=True)
            Image.open(os.path.join(CONCEPTS, source)).convert("RGB").crop(box).save(crop)
            meshy_client.edit_image(crop, full_prompt, out)
        return name, "ok"
    except Exception as error:  # one failed picture must not stop the others
        return name, "FAILED " + str(error)[:300]


def main():
    prefix = sys.argv[1] if len(sys.argv) > 1 else ""
    jobs = {name: job for name, job in {**VARIANTS, **WEAPONS}.items() if name.startswith(prefix)}
    os.makedirs(OUT, exist_ok=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
        for name, result in pool.map(lambda item: make(item[0], *item[1]), jobs.items()):
            print(name, result, flush=True)


if __name__ == "__main__":
    main()
