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
