---
title: Scripting reference
---

# Scripting reference

What a Lua script can reach. `this` is the entity the script is on; each of its components is a field of it
(`this.transform`, `this.health`, ...), and `this.world` is the scene. Read out of the engine's bindings
(`Sunta/src/Sunta/Scripting/LuaBindings.cpp`), with the notes written next to them.

See [Scripting](scripting.md) for how scripts are written and when their functions are called.

## vec3

```lua
vec3(1, 2, 3), a + b, a * 2, v:length(), v:normalized()
```

| Name | Note |
|---|---|
| `x` | Plain data members: in Lua they behave like fields (v.x, v.x = 5) |
| `y` |  |
| `z` |  |
| `length` |  |
| `normalized` | A zero vector can't be normalized (it would give NaN), so we return it unchanged |
| `dot` |  |
| `cross` |  |
| `distance` |  |

## Transform

| Name | Note |
|---|---|
| `position` |  |
| `rotation` | Euler angles in degrees (same as in the Inspector) |
| `scale` |  |
| `forward` | Direction the entity is facing (same convention lights use) |
| `translate` | Usage in Lua: this.transform:translate(vec3(0, 1, 0)) This is a TELEPORT by the offset (it changes the position directly), so it pushes nothing To move a kinematic body so that it pushes other bodies, use physicsBody:movePosition |

## PhysicsBody

| Name | Note |
|---|---|
| `velocity` |  |
| `angularVelocity` |  |
| `mass` |  |
| `useGravity` |  |
| `isKinematic` |  |
| `freezeRotationX` | Body can't spin around these of its own axes (e.g. freeze X and Z for a player that must not fall over) |
| `freezeRotationY` |  |
| `freezeRotationZ` |  |
| `addImpulse` | Instant change of velocity (heavier bodies change less) |
| `movePosition` | Moves a KINEMATIC body to 'targetPosition' as real movement: it pushes other bodies on the way Use it to move a player or a platform. Usage in Lua: this.physicsBody:movePosition(newPosition) (Setting this.transform.position directly is a teleport: it pushes nothing) |
| `addVelocityChange` | Instant change of velocity, the same for every mass |

## CharacterController

```lua
this.characterController.walkSpeed = 3
To drive a character from a script (e.g. an enemy): this.characterController.isPlayerControlled = false,
then every frame set moveDirection (a vec3 in the world, its length 0..1 is how much of the speed is used) and wantsToRun
```

| Name | Note |
|---|---|
| `walkSpeed` |  |
| `runSpeed` |  |
| `acceleration` |  |
| `turnSpeed` |  |
| `isPlayerControlled` |  |
| `moveDirection` |  |
| `wantsToRun` |  |

## LockOn

```lua
if this.lockOn.hasTarget then print(this.lockOn.target.name) end
To decide from a script what is locked on to (e.g. for an enemy): this.lockOn.isPlayerControlled = false,
then this.lockOn:setTarget(otherEntity) and this.lockOn:clear()
```

| Name | Note |
|---|---|
| `maxDistance` |  |
| `isPlayerControlled` |  |
| `hasTarget` |  |
| `target` | nil when nothing is locked on to |
| `setTarget` | Locks on to an entity that has a Lock On Target component (a warning in the log when it has none) |
| `clear` |  |

## LockOnTarget

```lua
this.lockOnTarget.isLockable = false (e.g. when the enemy dies, the lock on it is dropped)
```

| Name | Note |
|---|---|
| `isLockable` |  |
| `aimHeight` |  |

## CharacterActions

```lua
this.characterActions:request("block") (every frame while it should last), :request("parry"), :request("light"),
if this.characterActions.isGuarding then ... end, this.characterActions:play("Roar") (any animation of the model, once)
```

| Name | Note |
|---|---|
| `play` |  |
| `request` |  |
| `isBusy` |  |
| `knockBack` | Thrown back along the ground: this.characterActions:knockBack(direction, meters) (see knockbackAnimation) |
| `isGuarding` |  |
| `isParrying` | True during the part of a parry that turns blows away (a script can show it: a flash, a shield of light) |
| `isOpenToRiposte` |  |
| `isAttacking` |  |
| `action` | What the character does now: "light", "heavy", "leap", "roll", "hit", "cast", ... ("" = nothing), and the animation it plays for it (a summon that adapts to each kind of blow tells a jab from a kick) |
| `animation` |  |
| `setMove` | The timing of one attack animation (see AttackMove): this.characterActions:setMove("Attack_Light_1", 0.3, 0.5, 1.2) - a boss who took up a sword strikes at other moments of the same names' new clips |
| `attackSpeed` | How fast the attacks play (1 = as the animations were made), and how much more a heavy one hurts |
| `heavyDamageMultiplier` |  |
| `attackDelayChance` | A delayed blow now and then (0..1), see CharacterActionsComponent |
| `swingEffect` | What a fighter who takes up another weapon changes: the effects of its swings, its hyper armor (0 none, 1 heavy blows, 2 every blow), how close its lunge ends |
| `heavySwingEffect` |  |
| `swingEffectHeight` |  |
| `hyperArmor` |  |
| `attackReach` |  |
| `finisherKnockback` | this.characterActions.finisherKnockback = 3: the last blow of the light combo throws the target 3 m back; .heavyKnockback the same for heavy attacks |
| `heavyKnockback` |  |
| `knockdownChance` | .knockdownChance = 0.3: now and then a heavy throw knocks it off its feet (0 = never: a model without the clips) |

## CharacterAnimation

```lua
this.characterAnimation.idleAnimation = "Float" (hanging in the air: standing is floating now),
this.characterAnimation.lockedOnIdleAnimation = "" (the idle while locked on too); set them back afterwards
```

| Name | Note |
|---|---|
| `idleAnimation` |  |
| `walkAnimation` |  |
| `runAnimation` |  |
| `lockedOnIdleAnimation` |  |
| `blendSeconds` |  |

## Health

```lua
if this.health.current < 30 then ... end, other.health:damage(10) (a trap), this.health.isDead
```

| Name | Note |
|---|---|
| `max` |  |
| `minimum` | For bosses with phases: this.health.minimum = 1 (no hit takes it below: it can't fall yet), this.health.isInvulnerable = true (nothing hurts it while it changes its phase) |
| `isInvulnerable` |  |
| `weaponDamageTaken` | How much of a weapon's or a spell's damage gets through (1 = all of it): a boss that adapts to what hits it |
| `spellDamageTaken` |  |
| `fraction` | 0..1 of the full health |
| `heal` | Gives health back (never above the full health), nothing for the dead |
| `current` |  |
| `isDead` |  |
| `damage` | Hurts like a weapon does: plays the hit (or death) animation, does nothing while rolling |

## Enemy

The enemy's state machine from Lua. A script on the enemy adds its own states or replaces the ready ones by name (Idle, Chase, Attack, Recover, ReturnHome, Dead):  function OnStart() this.enemy:addState("Chase", { enter  = function(entity) entity.enemy:lookAtTarget(true) end, update = function(entity, deltaTime) local ai = entity.enemy if ai.distanceToTarget < 2 then return "Attack" end ai:moveTo(ai.target.transform.position, true) end }) end

| Name | Note |
|---|---|
| `sightDistance` | The senses from the Inspector |
| `hearingDistance` |  |
| `isDormant` | An enemy that waits to be called (Enemy 'startsDormant'): move it where it should appear, then ally.enemy:wake(). ally.enemy:sleep() puts it away again |
| `wake` |  |
| `sleep` |  |
| `state` |  |
| `secondsInState` |  |
| `addState` |  |
| `changeState` |  |
| `hasTarget` |  |
| `target` |  |
| `distanceToTarget` |  |
| `distanceToHome` |  |
| `isBusy` |  |
| `canSeeTarget` |  |
| `moveTo` |  |
| `goHome` |  |
| `circleTarget` |  |
| `stop` |  |
| `lookAtTarget` |  |
| `dodge` | A roll in a direction of the world (a backstep for vec3(0, 0, 0)), e.g. out of the way of something a script knows of |
| `startAttackCooldown` | No attack of its own choosing for that long (a script that wants the enemy to hold back for a moment) |
| `attack` | "light", "heavy", "leap", "roll", "backstep", "parry" or "block" (a block lasts only while it is asked for, every frame) |

## Boss

Usage in Lua (on the boss): if this.boss.phase == 2 then ... end, this.boss.phaseTwoHealthFraction = 0.3

| Name | Note |
|---|---|
| `phaseTwoHealthFraction` |  |
| `phaseTwoDamageMultiplier` |  |
| `phaseTwoAttackSpeedup` |  |
| `phase` |  |
| `isAwake` |  |
| `isDefeated` |  |

## FogGate

Usage in Lua (on the fog): if this.fogGate.isClosed then ... end (e.g. start the boss music)

| Name | Note |
|---|---|
| `victoryMessage` |  |
| `isClosed` |  |
| `isCleared` |  |

## MeleeEnemy

The tuning of a Melee Enemy, e.g. a script makes it more aggressive when hurt: this.meleeEnemy.attackCooldownSeconds = 0.5

| Name | Note |
|---|---|
| `attackDistance` |  |
| `runDistance` |  |
| `giveUpDistance` |  |
| `attackCooldownSeconds` |  |
| `heavyAttackChance` |  |
| `recoverSeconds` |  |
| `maxLightCombo` |  |
| `finisherChance` |  |
| `dodgeChance` |  |
| `dodgeCooldownSeconds` |  |
| `leapAttackDistance` |  |

## Entity

| Name | Note |
|---|---|
| `id` |  |
| `isValid` |  |
| `findChild` | A child (or a child's child...) with this name: this:findChild("Right Fist") - not another character's |
| `name` |  |
| `transform` | nil when the entity doesn't have the component, so scripts can check: if body then ... end |
| `physicsBody` |  |
| `characterController` |  |
| `lockOn` |  |
| `lockOnTarget` |  |
| `health` |  |
| `characterActions` |  |
| `characterAnimation` |  |
| `enemy` |  |
| `meleeEnemy` |  |
| `boss` |  |
| `fogGate` |  |
| `model` |  |
| `hitbox` |  |
| `enemyHealthBar` |  |
| `projectile` |  |
| `particles` |  |
| `environment` |  |
| `cutscene` |  |
| `audioSource` |  |
| `healingFlask` |  |
| `currency` |  |
| `currencyReward` |  |
| `levelUp` |  |
| `spellCaster` |  |
| `glowSprite` |  |
| `followCamera` |  |
| `liquidOrb` |  |
| `swarm` |  |
| `light` |  |
| `world` | The scene this entity lives in: this.world:find("Boss"), this.world:notify("..."), see RegisterWorld |

## SpellCaster

Usage in Lua (on the entity with the Spell Caster, a child of the character): local spell = this.world:find("Fire Volley Spell") if spell.spellCaster.isReady then spell.spellCaster:cast() end 'timesReleased' grows each time its projectiles leave: a script notices a finished cast by its change

| Name | Note |
|---|---|
| `cooldownSeconds` |  |
| `cooldownLeft` |  |
| `projectileCount` |  |
| `spreadDegrees` |  |
| `isCastOnRequestOnly` |  |
| `isUninterruptible` |  |
| `isReady` |  |
| `isCasting` |  |
| `timesReleased` |  |
| `cast` | False when the spell is not ready or its caster is busy (attacking, staggering). spell.spellCaster:cast(true): forced (a staged moment): cooling down or busy, it is cast anyway |

## Light

The sun or the moon of the scene (a Directional Light).

```lua
sun.light.color = vec3(1.0, 0.2, 0.1)
(its light, red as blood), sun.light.ambient = vec3(0.1, 0.0, 0.0) (what the shadows keep)
```

| Name | Note |
|---|---|
| `color` |  |
| `ambient` |  |

## World

What a script does to the game as a whole.

```lua
local boss = this.world:find("Boss")                 -- nil when there is no entity with that name
this.world:notify("THE GREAT RITE")                   -- a big message in the middle of the screen, fading away
this.world:prompt("R: answer with your own spell")   -- at the bottom, only in the frames it is asked for
this.world:shakeCamera(0.6)                          -- 0..1
this.world:burst("Fireball Burst", position, 80)     -- specks of the Particle Emitter with that name
this.world:slowMotion(1.5, 0.3)                      -- real seconds, the speed of the game meanwhile
if this.world.isCutscenePlaying then return end
```

| Name | Note |
|---|---|
| `find` |  |
| `spawn` | A copy of an entity of the scene used as a template (with its children) where it is wanted, turned 'yaw' degrees: local guard = this.world:spawn("Bodyguard Template", position, 180) (nil without the template). Keep the template itself out of the fight (far away, an enemy asleep). Its scripts start next frame |
| `destroy` | Removes an entity and everything under it at the end of the frame: this.world:destroy(guard) |
| `notify` |  |
| `prompt` |  |
| `shakeCamera` |  |
| `burst` |  |
| `effect` | Plays an effect of the scene (an entity with Effect Shapes and Particle Emitters, its children too) at a point, facing a direction along the ground (optional), its emitters throwing 'count' specks (optional): this.world:effect("Heavy Impact", target.transform.position, this.transform.forward) |
| `impactFrame` | An impact frame: the picture blurred out from its middle, colors split, contrast punched for a blink this.world:impactFrame(0.8)   (strength 0..1, optional seconds of real time, 0.3 by default) |
| `holdLetterbox` | Keeps the black bars of the cutscenes on through a sequence of shots (true), lets them go (false) |
| `slowMotion` |  |
| `flash` | The whole screen turns to a color and fades back: this.world:flash(vec3(1, 1, 1), 1.5) (a third number keeps it full for that long first; a fourth is how much it covers at its fullest: this.world:flash(vec3(1, 0.9, 0.7), 0.3, 0, 0.35) is a blink of light, not a white screen) |
| `launch` | Throws one projectile without a cast: the Projectile template with that name, on the side of 'caster', from a point of the world in a direction (a script's own patterns: rings, rains, walls of fire). this.world:launch("Boss Orb", this, this.transform.position + vec3(0, 1.4, 0), this.transform.forward) |
| `isSpellIncoming` | True when a flying spell of the other side reaches the entity within that many seconds |
| `isCutscenePlaying` |  |

## Model

```lua
this.model.isVisible = false, this.model.emissiveStrength = 2 (its glowing parts shine brighter)
```

| Name | Note |
|---|---|
| `isVisible` |  |
| `emissiveStrength` |  |
| `darkness` | this.model.darkness = 1 (a black silhouette), this.model.glow = vec3(1.2, 0.6, 0.1) (an aura of light all over) |
| `glow` |  |
| `rimGlow` | this.model.rimGlow = vec3(1.0, 0.7, 0.2): light along its outline; this.model.expression = 1: its smile on |
| `aura` | this.model.aura = vec3(1.4, 0.8, 0.2): a soft glow around the figure, beyond its outline; .auraSize = 0.12 (m) |
| `auraSize` |  |
| `expression` |  |
| `modelName` | this.model.modelName = "sih_boss_armored": another model put on in its place (a boss changing its body). Its animations go on by name, so the two models should share their skeleton and the names of their clips |

## Projectile

```lua
sword.hitbox.isEnabled = true (a weapon taken up), boss's fist: fist.hitbox.isEnabled = false;
sword.hitbox.damage = 40
Usage in Lua (on a projectile template): if bottle.projectile.timesBurst > seen then ... bottle.projectile.lastBurstPosition
```

| Name | Note |
|---|---|
| `timesBurst` |  |
| `lastBurstPosition` |  |

## EnemyHealthBar

```lua
this.enemyHealthBar.displayName = "The Hollow King, Unbound" (a boss renamed by its phase)
```

| Name | Note |
|---|---|
| `displayName` |  |

## Hitbox

| Name | Note |
|---|---|
| `isEnabled` |  |
| `damage` |  |
| `knockback` |  |

## FollowCamera

Usage in Lua (on the camera's entity): camera.followCamera.distance = 4 (closer in a fist fight, farther for a giant), .shoulderOffset = 0.7 (over the right shoulder: a tall enemy isn't hidden behind the player), .lockOnPitchDegrees = 10 (how far down it looks while locked on), .lockOnSideDegrees = 35 (swung off the line to the enemy: a fist fight seen side by side)

| Name | Note |
|---|---|
| `distance` |  |
| `pivotHeight` |  |
| `shoulderOffset` |  |
| `lockOnPitchDegrees` |  |
| `lockOnSideDegrees` |  |

## GlowSprite

```lua
eye.glowSprite.isVisible = true, eye.glowSprite.intensity = 8 (glowing eyes turned on by a script)
```

| Name | Note |
|---|---|
| `isVisible` |  |
| `color` |  |
| `size` |  |
| `intensity` |  |

## LiquidOrb

```lua
orb.liquidOrb.isVisible = true, orb.liquidOrb.brightness = 2
```

| Name | Note |
|---|---|
| `isVisible` |  |
| `tint` |  |
| `brightness` |  |
| `clearness` |  |
| `wobble` |  |

## Swarm

Usage in Lua (see SwarmComponent): s = find("Ember Swarm").swarm; s.isVisible = true; s.gathered = 1; s.radius = 40; s.spread = 1; s:fall(); s:reset(); if s.isFalling then ... end

| Name | Note |
|---|---|
| `isVisible` |  |
| `gathered` |  |
| `spread` |  |
| `radius` |  |
| `pieceSize` |  |
| `spinDegreesPerSecond` |  |
| `skyCenter` |  |
| `skyHalfExtents` |  |
| `groundHeight` |  |
| `glowColor` |  |
| `isFalling` |  |
| `fall` |  |
| `reset` |  |
| `breakWithin` | for _, at in ipairs(s:breakWithin(center, 12)) do ... end: the stuck copies that close break (a list of where they were, for an effect on each) |

## Cutscene

Usage in Lua (on the cutscene's entity): this.cutscene:play(), this.cutscene:replay() (again, even if it was played), if this.cutscene.isPlaying then ... end

| Name | Note |
|---|---|
| `play` |  |
| `replay` | Plays it even if it was already played (a shot cut back to again and again: angles taken in turn) |
| `wasAnswered` | The player pressed its 'waitForKey' (a moment the player answers) |
| `isPlaying` |  |

## Environment

Usage in Lua (on the entity with the Environment): this.environment.fogDensity = 0.1, this.environment.skyBrightness = 0.5, this.environment.exposure = 1.2, this.environment.bloomIntensity = 1.5 (the boss fight glows more)

| Name | Note |
|---|---|
| `skyBrightness` |  |
| `isFogEnabled` |  |
| `fogDensity` |  |
| `fogColor` |  |
| `skyboxFolder` | Another sky (a folder with its six pictures), e.g. when a boss changes the world around the fight |
| `exposure` |  |
| `bloomIntensity` |  |
| `bloomThreshold` |  |

## ParticleEmitter

```lua
this.particles.isEmitting = true, this.particles:burst(40) (40 specks at once), this.particles.particlesPerSecond = 80
```

| Name | Note |
|---|---|
| `isEmitting` |  |
| `particlesPerSecond` |  |
| `intensity` |  |
| `spawnRadius` |  |
| `outwardSpeed` |  |
| `startSize` |  |
| `burst` |  |

## AudioSource

Usage in Lua (on an entity with an Audio Source): this.audioSource:play(), this.audioSource:stop(2) (fading out for 2 seconds), if this.audioSource.isPlaying then ... end, this.audioSource.volume = 0.5

| Name | Note |
|---|---|
| `volume` |  |
| `play` |  |
| `stop` |  |
| `isPlaying` |  |

## HealingFlask

```lua
this.healingFlask:drink() (false when empty or busy), this.healingFlask.charges, this.healingFlask.maxCharges = 7
```

| Name | Note |
|---|---|
| `charges` |  |
| `maxCharges` |  |
| `healAmount` |  |
| `drink` |  |

## Currency

```lua
this.currency.amount, this.currency:add(500) (shown as a gain), this.currency:add(-100) (spent)
```

| Name | Note |
|---|---|
| `amount` |  |
| `droppedAmount` |  |
| `add` |  |

## CurrencyReward

Usage in Lua (on an enemy): this.currencyReward.amount = 1000

| Name | Note |
|---|---|
| `amount` |  |

## LevelUp

```lua
this.levelUp.level, this.levelUp.cost (of the next level), this.levelUp:raise("health") (false when the
Currency is too low), this.levelUp.healthPoints / staminaPoints / damagePoints
```

| Name | Note |
|---|---|
| `level` |  |
| `healthPoints` |  |
| `staminaPoints` |  |
| `damagePoints` |  |
| `cost` |  |
| `raise` |  |

