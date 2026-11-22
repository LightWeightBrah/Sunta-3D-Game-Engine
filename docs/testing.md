---
title: Testing a game automatically
---

# Testing a game automatically

A built game (or the editor) can play itself from a script of inputs and save pictures: a fight checked after every
change without anyone holding the mouse. All of it is switched on by environment variables, a normal run of the game
is not affected.

| Variable | What it does |
|---|---|
| `SUNTA_INPUT_SCRIPT` | the input script to play (see below) |
| `SUNTA_TEST_OUT` | the folder for the screenshots and `combat_log.csv` |
| `SUNTA_MUTE=1` | the game plays silent (the person at the computer may be listening to something else) |
| `SUNTA_BACKGROUND=1` | the window is never shown, never fullscreen, no console - nothing pops up over other programs; the pictures are still drawn and saved |
| `SUNTA_FIXED_DT` | every frame lasts exactly this long (seconds): smooth recordings |
| `SUNTA_COMBAT_TRACE=1` | `combat_log.csv` also traces every weapon box in every frame of an attack |

```powershell
$env:SUNTA_MUTE = "1"; $env:SUNTA_BACKGROUND = "1"
$env:SUNTA_INPUT_SCRIPT = "tests\fight.txt"; $env:SUNTA_TEST_OUT = "test_out"
.\Build\MyGame_Development\MyGame.exe
```

## The input script

One command a line: the second it happens (since the first frame), the command, its arguments. `#` starts a comment.

```text
# walk to the fog, go through, lock on and let the bot fight, a picture every second
1   tap F1                  # immortal (a Development build)
15  down W
21  up W
23  tap E
26  lockon
27  fight 40 1.0
28  every 1 60 shot fight   # fight_000.png, fight_001.png, ...
61  quit
```

| Command | Arguments |
|---|---|
| `down` / `up` | a key (`W`, `Space`, `Shift`, `Ctrl`, `Esc`, `F1`, `Mouse1`, `Mouse2`, digits, letters) |
| `tap` | a key pressed for one frame |
| `hold` | a key and how many seconds |
| `look` | mouse movement: x, y and over how many seconds |
| `cursor` | the pointer of the editor's GUI: x, y and over how many seconds (clicks with `down`/`up Mouse1`) |
| `shot` | a picture of the screen with that name (numbered when repeated) |
| `fps` | `fps [label]`: the frames since the last `fps` - their average FPS and the slowest frame, written to the log |
| `every` | `every <seconds> <until> <command...>`: a command repeated |
| `log` | a line written to the log |
| `quit` | ends the game |

The loading screen records itself once (`loading_screen.png`) when a script runs.

## The test bot

Commands that play the player for you:

| Command | What the bot does |
|---|---|
| `lockon` / `unlock` | keeps the lock on an enemy / lets go |
| `approach <distance> <seconds>` | walks up to the locked enemy |
| `fight <seconds> <distance> [free]` | fights it: approaches, attacks, keeps the distance (`free`: without the lock) |
| `freeze` / `unfreeze` | every enemy stands still (to film an attack from aside) |
| `teleport <x> <y> <z>` | puts the player there |
| `where` | writes where the player, its target and the camera are to `combat_log.csv` |

## The combat log

`combat_log.csv` gets a line for every swing: who, at whom, with what, whether it hit, at what height of the target's
body it landed and from how far - a test can tell whether the player's blows land where they look like they land.

## The game's log

Every run writes its log next to the program (`<Game>.log`, and `SuntaEditor.log` for the editor): loading, every
hit (`Combat: 'Player' hit 'Boss' for 38`), every cutscene, every landing of a jump, Lua errors with their lines.
