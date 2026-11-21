# Strongest in History Vs Today

The second sample project of the Sunta engine: one boss fight in four phases, staged after the short film
"Epstein vs Diddy" (Artificial Productions). It tests whether the engine's general components (spells, summons,
cutscenes, swarms, Lua) are enough for a game unlike the first one.

## After cloning: the film's files

The film's opening, two moments of it, its soundtrack (which runs on into "Lady Gaga - Judas") and the voices of the
techniques are somebody else's work, so they are not in the repository. One script downloads them from YouTube and
cuts them exactly as the game uses them (the same videos, the same soundtrack to the millisecond):

```bash
python -m pip install -r Projects/StrongestInHistoryVsToday/Tools/requirements_film.txt
python Projects/StrongestInHistoryVsToday/Tools/fetch_film_assets.py
```

It writes `res/Videos/*.mpg`, `res/Sounds/Film/*.ogg` and `res/Sounds/Voice/*.ogg`. The voices are separated from
the film's music with Demucs on the CPU (a minute or two); `--no-voices` skips them.

Without these files the game still runs, but the opening and two moments of the fight are black pauses and there is
no music.

## The fight

One boss, staged after the film, in phases (`res/Scripts/boss_fight.lua`):

1. **Strongest in History** - a fist fight; he blocks with his forearms, blinks behind a player who keeps away and
   throws volleys of orbs.
2. **Domain Expansion** - the sky turns, the temple rises, fire under the player's feet and the domain heals him; the
   player's own domain (key 3) breaks it for a while.
3. **The Summon** - he can't be hurt while his summon fights (`res/Scripts/summon_adaptation.lua`). It **adapts**: four
   hits of one kind in a row (fists, kicks, techniques) and that kind barely hurts it - switch to another. Each time,
   the eight-spoked wheel over its head turns an eighth and its wounds close a little. It **rushes** through a player
   who keeps away (roll through it), and from half its health on a **blade of light** grows from its fist: it fights
   faster and brings both fists down three times, the last one breaking the ground wide around it. The great
   technique (key 2) always hurts in full and makes it forget what it learnt.
4. **Last Stand** - the domain is back for good and everything comes faster.
5. **Armor and sword** - he puts on his armor and takes up a burning sword; the player answers with
   *Divine Baby Oil: Crystal Vessel*, a sword of frozen baby oil (rendered by the engine's crystal material).

The boss has 2400 health, the summon 1500. The player's techniques light him up gold and then white, as in the film.
