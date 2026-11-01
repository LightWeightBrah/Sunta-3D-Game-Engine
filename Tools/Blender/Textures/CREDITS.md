# Photo surfaces used by the Blender generators

All of them are from [Poly Haven](https://polyhaven.com) and are **CC0** (public domain): free to use, change and share, no credit required.
They are listed anyway, so it is clear where every picture comes from.

The generators (`sunta_blender.SurfaceTexture`) take only the light and dark spots of a photo; the color of a model stays the color of its palette.

| File | Poly Haven asset | Used for |
|---|---|---|
| `rock_boulder_dry_diff_1k.jpg` | [rock_boulder_dry](https://polyhaven.com/a/rock_boulder_dry) | light stone: castle blocks, floor slabs, pillars, bonfire stones |
| `rock_08_diff_1k.jpg` | [rock_08](https://polyhaven.com/a/rock_08) | dark stone |
| `gravel_concrete_03_diff_1k.jpg` | [gravel_concrete_03](https://polyhaven.com/a/gravel_concrete_03) | mortar between the blocks |
| `forest_ground_04_diff_1k.jpg` | [forest_ground_04](https://polyhaven.com/a/forest_ground_04) | moss |
| `rust_coarse_01_diff_1k.jpg` | [rust_coarse_01](https://polyhaven.com/a/rust_coarse_01) | rusted iron, the sword in the bonfire, the Rusted Knight's armor |
| `bark_brown_02_diff_1k.jpg` | [bark_brown_02](https://polyhaven.com/a/bark_brown_02) | burnt logs of the bonfire |
| `brown_leather_diff_1k.jpg` | [brown_leather](https://polyhaven.com/a/brown_leather) | leather straps and belts of the knights |
| `old_wood_floor_diff_1k.jpg` | [old_wood_floor](https://polyhaven.com/a/old_wood_floor) | planks of the knights' shield |
| `burned_ground_01_diff_1k.jpg` | [burned_ground_01](https://polyhaven.com/a/burned_ground_01) | ash of the bonfire |

Downloaded through the public Poly Haven API (`https://api.polyhaven.com/files/<asset>`, the 1k JPG "Diffuse" map).

## Pictures and models made by AI

| File | Made with | License | Used for |
|---|---|---|---|
| `../Concepts/boss_concept.png` | Z-Image-Turbo (Tongyi-MAI) | Apache 2.0 | concept of the boss, input of TRELLIS.2 and later of Meshy |
| `../Generated/boss_trellis.glb` | TRELLIS.2 (Microsoft) from the concept | MIT | the first boss's body (replaced by Meshy's) |
| `../Concepts/boss_axe_concept.png` | Z-Image-Turbo (Tongyi-MAI) | Apache 2.0 | the first boss's axe (replaced by Meshy's) |

Made with Meshy (meshy.ai) on the user's paid plan: the user owns these, commercial use included. Every task (its id,
prompt and settings) is in `../Generated/meshy_log.json`.

| File | Made with | Used for |
|---|---|---|
| `../Concepts/knight_concept.png`, `../Concepts/rusted_knight_concept.png` | Meshy Text to Image | concepts of the knights |
| `../Generated/knight_meshy.glb`, `../Generated/rusted_knight_meshy.glb` | Meshy Image to 3D | the knights' bodies (`characters_generator.py`) |
| `../Generated/knight_meshy_rigged.glb` | Meshy Image to 3D of `knight_concept.png`, rigged by Meshy | the player's body (`characters_generator.py`) |
| `../Generated/boss_meshy_rigged.glb` | Meshy Image to 3D of `boss_concept.png`, rigged by Meshy | the boss's body (`boss_generator.py`) |
| `../Concepts/boss_great_axe_concept.png` | Meshy Text to Image | concept of the boss's long brass-bound axe |
| `../Generated/boss_great_axe_meshy.glb` | Meshy Image to 3D | the boss's axe (`boss_generator.py`) |
| `../Generated/Motions/leap_slam.fbx` | Meshy Text to Motion | the boss's jump attack |
| `../Concepts/Enemies/enemy_*.png` | Meshy Text to Image | concepts of the enemies (approved by the user) |
| `../Concepts/Enemies/enemy_*_body.png`, `../Concepts/Enemies/weapon_*.png` | Meshy Image to Image of the concepts | each enemy with empty hands and its weapons alone |
| `../Generated/Enemies/*_rigged.glb` | Meshy Image to 3D, rigged by Meshy | the enemies' bodies |
| `../Generated/Enemies/<weapon>.glb` | Meshy Image to 3D | the enemies' weapons |
| `../Concepts/boss_phase_two_concept.png` | Meshy Text to Image | the look of the boss's second phase |

The glow of the boss's second phase (`boss_emissive.png`, `boss_axe_emissive.png` in Sunta/res/Models) is ours: baked in Blender
from procedural nodes (`boss_generator.py`), no outside source.

The sky of the example game (`Projects/MyGame/res/Skyboxes/AshenNight`): Poly Haven HDRI "Kloppenheim 07 (Pure Sky)"
(https://polyhaven.com/a/kloppenheim_07_puresky, CC0), its tonemapped JPG cut into cubemap faces, darkened and tinted by
`Tools/Skybox/panorama_to_cubemap.py`.

Arena props (Meshy, the user's paid plan, the user owns them; every task in `../Generated/meshy_log.json`):

| File | Made with | Used for |
|---|---|---|
| `../Concepts/Arena/*.png` | Meshy Text to Image (approved by the user) | concepts of the throne, brazier, wall with an arch, knight statue, sword graves, corner tower |
| `../Generated/Arena/*.glb` | Meshy Image to 3D of the concepts | the props (`arena_props_generator.py`) |

Level kit (Meshy, the user's paid plan): `../Concepts/Kit/*.png` (Meshy Text to Image, front + side views, approved by the user;
`*_front.png` / `*_side.png` are their halves) → `../Generated/Kit/*.glb` (Meshy Multi Image to 3D or Image to 3D) →
`kit_generator.py`.

Motions (Meshy Text to Motion): `../Generated/Motions/boss_*.fbx` (the boss's phase two) and `executioner_*.fbx` (the executioner).
Also `../Generated/Motions/knight_parry.fbx` (the player's parry) and `boss_intro.fbx` (the boss's intro cutscene).
