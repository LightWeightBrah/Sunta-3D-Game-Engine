---
title: "Tutorial: a small souls-like"
---

# Tutorial: a small souls-like, step by step

By the end you have a game you can send to someone: a courtyard with an enemy, a bonfire, a fog gate and a boss
behind it, a player who rolls, blocks, parries, drinks from a flask and loses his souls when he dies. No C++ - the
engine's components do the work; one short Lua script at the end gives the boss a phase of its own.

Every component named here is described in [Components](components.md).

## 1. A project

1. Start `SuntaEditor`. In the launcher choose **New Project...**, give it a name (e.g. `MyFirstSouls`) and a folder.
2. The editor opens with an empty scene. **File > Save Scene As** - save it as `Scenes/Main.scene`.
3. **Project > Project Settings...**: set the game's name and the *Start scene* (**Choose scene...**) to `@game/Scenes/Main.scene`.

## 2. Ground and light

1. From the toolbar add a **cube**. In the Inspector set its Transform: position `0, -0.5, 0`, scale `40, 1, 40`.
   It already has a **Box Collider**: the player will stand on it.
2. Add a **Directional Light** (the sun icon) and turn it to look down at an angle (rotation e.g. `-50, 30, 0`).
3. Optional: an **Environment** component on any entity gives the scene a sky, fog and the light of the sky.

Press **Play** to check: you see the floor from the free camera. **Stop**.

## 3. The player

1. **Entity > Create Souls Player**. It is a knight with a sword and a shield and everything a souls-like player
   needs: Character Controller, Character Actions, Character Animation, Health, Stamina, Poise, Lock On, Healing
   Flask, Player HUD, Respawn, Currency, Character Sounds.
2. Put it at `0, 0, 10`.
3. The camera: select the **Camera** entity (or *Entity > Create Camera From View*), **Add Component > Follow Camera**.
   Place it behind the player, e.g. `0, 2.2, 14.5`.

Press **Play**, then **Tab** to give the mouse and keys to the game: walk (W A S D), run (Shift), roll (tap Shift),
attack (left mouse, Ctrl + left mouse for a heavy one), block (right mouse), parry (F), jump (Space), drink (R).
The bars in the top left corner are the Player HUD.

> Tune the feel in the Inspector while you test: walk and run speed (Character Controller), roll distance and the
> windows of every attack (Character Actions), stamina costs (Stamina). Each field explains itself in
> [Components](components.md).

## 4. An enemy

1. **Entity > Create Rusted Knight (enemy)**, put it at `0, 0, -2`.
2. It is a **Melee Enemy**: it waits, chases the player it sees, attacks, circles, dodges, gives up when the player
   runs far away. Its numbers (attack distance, combo length, dodge chance) are in the Inspector.
3. **Currency Reward** on it says how many souls it gives (**Add Component** if it has none).

Play: lock on with **Q**, fight it. When the player dies: "YOU DIED", the souls stay on the ground where he fell,
the player comes back at the start.

## 5. A bonfire

1. **Entity > Create Bonfire**, put it near the start, e.g. `4, 0, 12`.
2. Play: standing next to it, **E** rests - health, stamina and the flask are refilled, the enemies come back, and
   this is where the player wakes up after dying.

## 6. A boss behind a fog gate

1. Walls: a few cubes around an area at the far end (e.g. centre `0, 0, -25`, 20 x 20 m), leaving a doorway 4 m wide.
2. The fog: an entity in the doorway with a **Model** (any semi-transparent model, the engine has `fog_gate`),
   a **Box Collider** filling the doorway and **Add Component > Fog Gate**. Set *Arena Center* `0, 0, -25` and
   *Arena Half Extents* `10, 5, 10`; turn on *Is Crossed With Key* (E walks through it, as in Dark Souls).
3. The boss: another Rusted Knight inside the arena, bigger (scale `1.4`). **Add Component > Boss** and
   **Enemy Health Bar** with *Is Boss* on and a *Display Name* - its name and bar appear at the bottom of the screen
   when the fog closes behind the player.
4. Its second phase: in **Boss** set *Phase Two Health Fraction* `0.5`, *Phase Two Glow* `2` and
   *Phase Two Growth* `1.15` - at half health it roars, glows and grows, and attacks faster.
5. Optional: a **Cutscene** entity with a few child points the camera flies through, named in the Fog Gate's
   *Cutscene*: it plays the first time the player enters. *Fade Seconds* `0.8` fades in and out of it.

## 7. A boss that thinks: a Lua script

Create `Scripts/boss.lua` in the File Browser and put it on the boss (**Add Component > Script**):

```lua
-- Below a third of its health the boss throws fire at a player who keeps away
local FireEverySeconds = 4
local fireIn = FireEverySeconds

function OnUpdate(deltaTime)
    if this.health.fraction > 0.33 then return end
    fireIn = fireIn - deltaTime
    if fireIn > 0 or this.enemy.distanceToTarget < 5 then return end

    fireIn = FireEverySeconds
    this.world:notify("THE KNIGHT BURNS")
    this.world:launch("Cultist Fireball", this, this.transform.position + vec3(0, 1.5, 0), this.transform.forward)
end

function OnDamaged(amount, kind, attacker)
    if kind == "weapon" then this.world:shakeCamera(0.2) end
end
```

`"Cultist Fireball"` is the name of an entity with a **Projectile** component - copy one from the sample project
`Scrap Souls` or make your own (Add Component > Projectile: speed, damage, homing, hit effect). Everything a script can
use is in the [Scripting reference](scripting-reference.md).

## 8. Sounds and effects

- **Character Sounds** on the player and the enemies: a swing, a hit, steps; and voices (pain, effort, death, a
  shout when an enemy sees the player). A field can be a folder: one of its sounds at random each time.
- **Particle Emitter** and **Effect Shape** entities are effects played by name: put their name in a Hitbox's
  *Hit Effect* or a Projectile's *Hit Effect*, or play them from a script (`this.world:effect("Heavy Impact", ...)`).
- **Weapon Trail** on a sword draws its trail while it strikes.

## 9. Build the game

**Project > Project Settings... > Save and Build Game.** The folder `Build/<Name>_Release` is the whole game: zip it
and send it. Players get a loading screen, the game, and **Escape** for the pause menu.

---

Next: open `Projects/ScrapSouls` and `Projects/StrongestInHistoryVsToday` - every system above, used at full size.
