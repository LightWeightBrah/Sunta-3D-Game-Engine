---
title: Scripting
---

# Scripting with Lua

The engine's components do most of a souls-like by themselves. A Lua script is for what makes *your* game: a boss's
own patterns and phases, staged moments, a door that opens when a lever is pulled.

## A script

A `.lua` file in the game's `Scripts/` folder, put on an entity with **Add Component > Script** (an entity can have
several). In it, `this` is the entity: each of its components is a field (`this.transform`, `this.health`,
`this.characterActions`, ...), `this.world` is the scene.

```lua
local Speed = 90   -- degrees a second

function OnStart()
    print("the wheel turns")            -- goes to the log (the editor's Console panel)
end

function OnUpdate(deltaTime)
    local rotation = this.transform.rotation
    this.transform.rotation = vec3(rotation.x, rotation.y + Speed * deltaTime, rotation.z)
end
```

## When the engine calls a script

| Function | When |
|---|---|
| `OnStart()` | once, when the game starts |
| `OnUpdate(deltaTime)` | every frame (`deltaTime`: seconds of game time, slower during slow motion and hit stops) |
| `OnDamaged(amount, kind, attacker)` | the entity lost health; `kind` is `"weapon"`, `"spell"` or `"other"` |
| `OnTriggerEnter(other)` / `OnTriggerStay` / `OnTriggerExit` | something entered a Box Collider of the entity that is a trigger |
| `OnCollisionEnter(other)` / `OnCollisionStay` / `OnCollisionExit` | a solid collision |

A function that is not written is simply not called. An error in a script is written to the log with the line, and
the game goes on. A *Development build* (and the editor) reloads a script when its file is saved.

## Finding and changing other entities

```lua
local boss = this.world:find("Boss")          -- by its name in the Hierarchy (nil when there is none)
if boss and boss.health.fraction < 0.5 then
    boss.model.glow = vec3(1.0, 0.2, 0.1)    -- an aura of light all over it
end
```

## The world

```lua
this.world:notify("DOMAIN EXPANSION")                         -- a big message in the middle of the screen
this.world:effect("Heavy Impact", position, this.transform.forward)   -- an effect of the scene, by name
this.world:launch("Boss Orb", this, from, direction)           -- a projectile without a cast
this.world:slowMotion(1.5, 0.3)                               -- 1.5 real seconds at 30% speed
this.world:shakeCamera(0.6)
```

## Enemies with your own behaviour

An enemy is a state machine (Idle, Chase, Attack, ...). A script can add states or replace any of them by name,
so a new kind of enemy needs no C++:

```lua
local Guard = {
    enter  = function(entity) entity.enemy:lookAtTarget(true) end,
    update = function(entity, deltaTime)
        local ai = entity.enemy
        if ai.distanceToTarget < 3 then return "Attack" end
        -- (returning nothing stays in this state)
    end
}

function OnStart()
    this.enemy:addState("Guard", Guard)
    this.enemy:changeState("Guard")
end
```

## Spells and attacks from a script

```lua
local spell = this.world:find("Boss Fire Spell")
spell.spellCaster:cast()        -- false when it is not ready or the caster is busy
spell.spellCaster:cast(true)    -- a staged moment: cast anyway (cuts what the caster does short)

this.characterActions:play("Roar")            -- any animation of the model, once
this.characterActions:setMove("Attack_Light_1", 0.25, 0.6, 1.0)   -- when an attack hurts, and how much
```

## Cutscenes

A Cutscene entity is played with `this.world:find("Intro").cutscene:play()`. With *Wait For Key* it holds until the
player presses a key (*Wait Presses* above 1: pressed that many times, quickly - getting up off the ground) and
the script reads `cutscene.wasAnswered`.

## Everything else

The [Scripting reference](scripting-reference.md) lists every field and function a script can reach, generated
from the engine's bindings. The two sample projects have large scripts to learn from:
`Projects/StrongestInHistoryVsToday/res/Scripts/boss_fight.lua` (a four-phase boss) and `player_techniques.lua`.

> A large script: Lua allows at most 200 `local` variables in one file's main part. Keep the rest in a table
> (`local Boss = {}` and `function Boss.phaseTwo() ... end`).
