# Pictures of the HUD of the sample game

The look of the HUD is described by `hud_style.json` (read by the engine, see `Sunta/src/Sunta/Gameplay/HUDStyle.h`):
which picture is the frame of each bar, how wide its ornate ends are, where the channel for the fill is inside it,
how tall the bar is on the screen and where it stands, the colors of the fill, and the pictures of the corners.

| File | Made with | License |
|---|---|---|
| `health_frame.png`, `stamina_frame.png`, `boss_frame.png`, `boss_crest.png` | Drawn by a Python script (`Tools/UI/hud_bars.py`): blackened forged iron with bevelled edges, rivets and gothic end plates; the grain of the iron is the photo [rust_coarse_01](https://polyhaven.com/a/rust_coarse_01) from Poly Haven, greyed and darkened | Script: the project's own. Photo: CC0 |
| `bar_fill.png`, `panel.png` | Drawn by the same script (light and shade of the fill, a smoky panel with a thin gold line) | the project's own |
| `icon_flask.png`, `icon_soul.png` | Generated with Z-Image-Turbo (Hugging Face Space, Apache 2.0 model), cut out of the black background by the script | Generated, free to use |
