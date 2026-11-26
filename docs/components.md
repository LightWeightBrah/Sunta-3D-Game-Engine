---
title: Components
---

# Components

Every part an entity can have, as **Add Component** in the Inspector lists it. The text comes from the engine's
own code (each component explains itself above its definition), so it is as current as the engine.
Field names are the ones of the engine's code (the Inspector shows them as words).

## Scene and rendering

### Transform

| Field | Type |
|---|---|
| `position` | Float3 |
| `rotation` | Float3 |
| `scale` | Float3 |

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

### Tag

| Field | Type |
|---|---|
| `name` | String |

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

### Parent

Makes this entity a child of another one: it moves, turns and scales together with its parent Its Transform is then relative to the parent (position 0 = where the parent is)

The child can also follow a bone of the parent's Model ('boneName', e.g. 'Weapon_R' of the Knight): a sword that is a child of the player, on the bone of the hand, swings with every animation

In the editor: drag an entity onto another one in the Hierarchy to make it its child, drag it onto the empty space below the list (or use Unparent) to make it free again Children of a moving physics body follow it, but physics never moves a child on its own

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

### Mesh

| Field | Type |
|---|---|
| `isVisible` | Bool |

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

### Model

| Field | Type |
|---|---|
| `isVisible` | Bool |
| `emissiveStrength` | Float |
| `fogAmount` | Float |

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

### Animator

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

### Camera

The point of view the game is drawn from The Transform of the entity decides where the camera stands and where it looks: the camera looks along the -Z axis of the entity, +Y is up While the game plays, the first active camera replaces the free camera of the editor

| Field | Type |
|---|---|
| `fieldOfView` | Float |
| `nearPlane` | Float |
| `farPlane` | Float |
| `isActive` | Bool |

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

### Directional Light

The light of the sun or the moon: parallel rays from one direction (the entity's rotation) The first one with 'castsShadows' casts shadows (see ShadowRenderer): 'shadowDistance' meters around the camera, 'shadowStrength' how dark they are (1 = only the ambient light is left), 'shadowSoftness' how soft their edge is (0 = hard, 1..3 = more samples), 'shadowResolution' the pixels of the shadow map (sharper for more memory and time)

| Field | Type |
|---|---|
| `castsShadows` | Bool |
| `shadowDistance` | Float |
| `shadowStrength` | Float |
| `shadowSoftness` | Int |
| `shadowResolution` | Int |

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

### Point Light

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

### Spotlight

| Field | Type |
|---|---|
| `innerCutOffAngle` | Float |
| `outerCutOffAngle` | Float |

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

### Environment

The look of the whole scene: the sky around it, the fog in it, and how the picture is put on the screen (HDR, bloom)

How to use it: Add Component > Environment on one entity of the scene (e.g. the Moonlight); a second one is ignored 'skyboxFolder' is a folder with six pictures of the sky: right, left, top, bottom, front and back (.jpg or .png), e.g. @game/Skyboxes/AshenNight (Tools/Skybox/panorama_to_cubemap.py makes them from a panorama). Empty = no sky 'skyBrightness' darkens or brightens it. With 'isFogEnabled' far things fade into 'fogColor': 'fogDensity' 0.03 is a light haze (half the color at about 28 m), 0.08 a thick fog (half at about 10 m) A script can change it during the game: this.environment.fogDensity = 0.1 (the fog thickens when the boss wakes up)

| Field | Type |
|---|---|
| `skyboxFolder` | String |
| `skyBrightness` | Float |
| `isFogEnabled` | Bool |
| `fogColor` | Color |
| `fogDensity` | Float |
| `isPostProcessingEnabled` | Bool |
| `exposure` | Float |
| `isBloomEnabled` | Bool |
| `bloomThreshold` | Float |
| `bloomIntensity` | Float |
| `bloomBlurPasses` | Int |
| `hitImpactFrame` | Float |
| `pixelSize` | Int |
| `paletteLevels` | Int |
| `ditherStrength` | Float |
| `outlineStrength` | Float |
| `outlineColor` | Color |
| `paintRadius` | Int |
| `saturation` | Float |
| `contrast` | Float |
| `colorTint` | Color |
| `vignette` | Float |

<sub>Defined in `Sunta/src/Sunta/Effects/EnvironmentComponent.h`</sub>

### Glow Sprite

A soft spot of light that always faces the camera: glowing eyes, a lantern's halo, the heart of a spell

How to use it: Add Component > Glow Sprite on an entity (make it a child of a character's bone for eyes that glow: one entity per eye, a few centimeters in front of the head bone). Its light is added to the picture, so with Bloom on (Environment) a bright one shines. A script turns it on and off: entity.glowSprite.isVisible = true, or fades it with entity.glowSprite.intensity

| Field | Type |
|---|---|
| `isVisible` | Bool |
| `color` | Color |
| `size` | Float |
| `intensity` | Float |

<sub>Defined in `Sunta/src/Sunta/Effects/GlowSpriteComponent.h`</sub>

### Liquid Orb

A ball of clear liquid (water, oil, a crystal ball): it bends the sky seen through it, mirrors it on its skin, shines along its rim and wobbles slowly like a drop held in the air

How to use it: Add Component > Liquid Orb on an entity. Its Transform's scale is its size (1 = one meter across). It reads the sky of the scene's Environment, so give the scene a skybox. As a child of a Projectile template it is what the spell throws (see ProjectileComponent)

| Field | Type |
|---|---|
| `isVisible` | Bool |
| `tint` | Color |
| `brightness` | Float |
| `wobble` | Float |
| `wobbleSpeed` | Float |
| `clearness` | Float |

<sub>Defined in `Sunta/src/Sunta/Effects/LiquidOrbComponent.h`</sub>

## Physics

### Physics Body

| Field | Type |
|---|---|
| `isKinematic` | Bool |
| `useGravity` | Bool |
| `velocity` | Float3 |
| `angularVelocity` | Float3 |
| `mass` | Float |
| `restitution` | Float |
| `friction` | Float |
| `freezeRotationX` | Bool |
| `freezeRotationY` | Bool |
| `freezeRotationZ` | Bool |

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

### Box Collider

| Field | Type |
|---|---|
| `showGizmos` | Bool |
| `localOffset` | Float3 |
| `halfExtents` | Float3 |
| `isTrigger` | Bool |
| `blocksCamera` | Bool |
| `layer` | CollisionLayer |

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

## The player and characters

### Character Controller

Turns an entity into a character that walks and runs, no script needed

How to use it: in the editor choose Add Component > Character Controller It also adds the Physics Body and the Box Collider a character needs After pressing Play the character moves with W A S D (relative to where the camera looks) and runs while Left Shift is held. A quick tap of Left Shift is a dodge (see Character Actions), as in Dark Souls

Every number below shows up in the Inspector, so the feel of the character is tuned there

A script (or an enemy) can drive the character instead of the keyboard: turn 'isPlayerControlled' off, then write 'moveDirection' and 'wantsToRun' every frame

| Field | Type |
|---|---|
| `walkSpeed` | Float |
| `runSpeed` | Float |
| `acceleration` | Float |
| `turnSpeed` | Float |
| `facingOffsetDegrees` | Float |
| `isPlayerControlled` | Bool |

<sub>Defined in `Sunta/src/Sunta/Gameplay/CharacterControllerComponent.h`</sub>

### Character Actions

Rolls and attacks of a character, no script needed (the Dark Souls style of combat)

How to use it: in the editor choose Add Component > Character Actions on a character that has a Character Controller, a Model with animations and an Animator. Animations whose names contain roll, backstep, light_1/2/3 and heavy are chosen for you, the others are picked from the dropdowns in the Inspector After pressing Play: a tap of Left Shift rolls (a backstep when no direction is held; holding it runs), left mouse button = light attack (press again during the attack to chain the combo), Left Ctrl + left mouse button = heavy attack, holding the right mouse button blocks and a tap of F parries (both need a Shield on the character, see ShieldComponent) The block plays on the upper body only (from 'upperBodyBone' up): the legs keep walking, so the character can move behind its shield (walking, not running)

The action plays its animation once. While it plays, the character moves on its own (the roll carries it forward, an attack makes a small step) and turns towards the locked-on enemy or the held direction when the action starts

A script (or an enemy) can ask for an action instead of the keyboard: turn 'isPlayerControlled' off on the Character Controller and set 'requestedAction'

Attacks hurt through the Hitbox of the weapon during their striking part ('lightHitFrom'..'lightHitTo' and the heavy one). With a Stamina component every action costs stamina, and with none left it doesn't start

<sub>Defined in `Sunta/src/Sunta/Gameplay/CharacterActionsComponent.h`</sub>

### Character Animation

Plays the right animation for what the character is doing, no script needed: standing, walking or running, and while locked on to an enemy (facing it): stepping sideways and backwards

How to use it: in the editor choose Add Component > Character Animation on the character (the entity needs a Model with animations, it gets the Animator automatically) The animations are chosen from the dropdowns in the Inspector, they list the animations of the Model Animations called Idle, Walk, Run, Walk_Back, Strafe_Left and Strafe_Right are chosen for you when the component is added

Which one plays depends on how fast the character really moves (the Physics Body), so a character that walks into a wall stands still. Running is chosen above the middle between the walk speed and the run speed of the Character Controller. An animation that is not set is replaced by the closest one (running by walking, walking by standing, stepping sideways or backwards by walking)

<sub>Defined in `Sunta/src/Sunta/Gameplay/CharacterAnimationComponent.h`</sub>

### Character Sounds

The sounds a character makes in a fight, played by the engine at the right moment, heard from where the character is

How to use it: Add Component > Character Sounds on a character (the player, an enemy, a boss). Each field is a sound file or a FOLDER of them (one at random each time, e.g. @game/Sounds/Footsteps/Stone); an empty field is silent

- `attackSound` the weapon cuts the air: starts 'attackSoundLeadSeconds' before the blow lands (the start of the attack's hit window), so the loudest part of the swoosh meets the blade
- `hurtSound` a weapon or a shockwave hits the body
- `blockSound` the shield takes a blow (also when the guard breaks)
- `parrySound` the shield turns an attack away
- `deathSound` the character falls
- `rollSound` a roll or a backstep
- `footstepSound` each time a foot of the animation touches the ground: the bones 'leftFootBone' and 'rightFootBone' (a model without them: every 'footstepMeters' walked)
- `shockwaveSound` a heavy blow shakes the ground (Character Actions with a shockwave)

Its voice (a man grunting, a monster roaring): never two of them at once, and only now and then

- `painVoice` hurt but still alive ('painVoiceChance' of the hits)
- `attackVoice` the effort of a blow ('attackVoiceChance' of the attacks)
- `deathVoice` the last cry as it falls
- `alertVoice` an enemy that notices the player (a shout, a curse)

'volume' is for all of them (a giant is louder than a ghoul)

| Field | Type |
|---|---|
| `attackSound` | String |
| `hurtSound` | String |
| `blockSound` | String |
| `parrySound` | String |
| `deathSound` | String |
| `rollSound` | String |
| `footstepSound` | String |
| `shockwaveSound` | String |
| `painVoice` | String |
| `attackVoice` | String |
| `deathVoice` | String |
| `alertVoice` | String |
| `painVoiceChance` | Float |
| `attackVoiceChance` | Float |
| `attackSoundLeadSeconds` | Float |
| `leftFootBone` | String |
| `rightFootBone` | String |
| `footstepMeters` | Float |
| `footstepCameraShake` | Float |
| `footstepShakeRadius` | Float |
| `volume` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/CharacterSoundsComponent.h`</sub>

### Strike Aim

Aims a character's attacks at the enemy it fights: while it attacks, the spine bends up or down and turns left or right a little, so a punch made for an opponent of another height (or one standing a bit aside) meets the enemy's head or chest instead of its belly or the air. Common in action games ("aim offset")

How to use it: Add Component > Strike Aim on the character (with Character Actions and an Animator). 'spineBones' are the bones that bend, from the hips up (e.g. "mixamorig:Spine,mixamorig:Spine1,mixamorig:Spine2"); the angle is shared between them. 'clipStrikeHeight' is where the character's attack animations strike by themselves (meters above its feet); the aim raises or lowers that to 'targetHeight' of the enemy's body (0 = feet, 1 = top of its body box: 0.85 is about the face). The enemy is the one locked on to, or the nearest character in front

| Field | Type |
|---|---|
| `spineBones` | String |
| `clipStrikeHeight` | Float |
| `targetHeight` | Float |
| `maxPitchDegrees` | Float |
| `maxYawDegrees` | Float |
| `fadeSeconds` | Float |
| `searchDistance` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/StrikeAimComponent.h`</sub>

### Health

How much damage a character can take before it dies

How to use it: Add Component > Health on the player or an enemy. Weapons (Hitbox) hurt it through its Box Collider A hit plays the hit animation of the Character Actions (the character staggers), the last one the death animation While a roll is invulnerable (Character Actions), hits do nothing

| Field | Type |
|---|---|
| `maxHealth` | Float |
| `currentHealth` | Float |
| `hitEffect` | String |
| `deflectEffect` | String |
| `deathSlowMotionSeconds` | Float |
| `deathSlowMotionScale` | Float |
| `weaponDamageTaken` | Float |
| `spellDamageTaken` | Float |
| `spellImmunitySeconds` | Float |
| `rallyWindowSeconds` | Float |
| `rallyPerDamage` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/HealthComponent.h`</sub>

### Stamina

What rolling, attacking and running cost (the Dark Souls stamina bar)

How to use it: Add Component > Stamina on a character with Character Actions An action can start while any stamina is left (it may take the bar down to zero), running stops at zero Stamina comes back on its own a moment after it was last used, but not during an action

| Field | Type |
|---|---|
| `maxStamina` | Float |
| `regenerationPerSecond` | Float |
| `regenerationDelaySeconds` | Float |
| `rollCost` | Float |
| `backstepCost` | Float |
| `lightAttackCost` | Float |
| `heavyAttackCost` | Float |
| `parryCost` | Float |
| `runCostPerSecond` | Float |
| `currentStamina` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/StaminaComponent.h`</sub>

### Poise

How much a character can take before a hit makes it stagger (poise, as in Dark Souls)

How to use it: Add Component > Poise on a character with Health. Every weapon hit takes the weapon's 'poiseDamage' (Hitbox) away from it. While some poise is left the character keeps doing what it does, when it runs out the character staggers and its poise is full again 'recoverAfterSeconds' after the last hit the poise is full again too A character without Poise staggers after every hit

| Field | Type |
|---|---|
| `maxPoise` | Float |
| `recoverAfterSeconds` | Float |
| `staggerImmunitySeconds` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/PoiseComponent.h`</sub>

### Shield

A shield the character blocks with (the Dark Souls style of guarding)

How to use it: put it on the shield, a child entity of the character (the Souls Player and the enemies get it on their left hand). The owner is the closest parent with Character Actions, which blocks while the block key is held (the right mouse button) and parries with F

A hit that comes from the front (inside 'guardAngleDegrees') while the owner blocks hurts only by 'damageThrough' and costs stamina instead: 'staminaPerDamage' for every point of the blocked damage. When there is not enough stamina the guard breaks: the owner staggers and takes the whole hit

| Field | Type |
|---|---|
| `guardAngleDegrees` | Float |
| `damageThrough` | Float |
| `staminaPerDamage` | Float |
| `blockEffect` | String |

<sub>Defined in `Sunta/src/Sunta/Gameplay/ShieldComponent.h`</sub>

### Hitbox

The part of a weapon that hurts: a box that deals damage while its owner swings it

How to use it: put it on the weapon, a child entity of the character (the Souls Player and the enemies get it on their sword) The owner is the closest parent with Character Actions. The box hurts only during the striking part of an attack and hits every target once per swing. A heavy attack multiplies the damage (Character Actions) Targets are entities with Health and a Box Collider, and the collision rules decide who can be hit: the 'layer' of the hitbox against the layer of the target's collider (by default PlayerHitbox hits Enemy, EnemyHitbox hits Player)

| Field | Type |
|---|---|
| `damage` | Float |
| `poiseDamage` | Float |
| `hitStopSeconds` | Float |
| `cameraShake` | Float |
| `knockback` | Float |
| `localOffset` | Float3 |
| `halfExtents` | Float3 |
| `layer` | CollisionLayer |
| `showGizmos` | Bool |

<sub>Defined in `Sunta/src/Sunta/Gameplay/HitboxComponent.h`</sub>

### Lock On

Lets a character lock on to an enemy (an entity with a Lock On Target component)

How to use it: in the editor choose Add Component > Lock On on the player (next to the Character Controller) After pressing Play, Q locks on to the enemy the camera looks at, and Q again lets go While locked on, the mouse switches to another enemy (flick it left or right) instead of turning the camera, the camera frames the enemy (needs a Follow Camera) and the character faces it while walking (strafing) Running (Left Shift) turns the character towards the way it runs, as it does when it is not locked on

A script (or an enemy) can use it too: turn 'isPlayerControlled' off, then call setTarget / clear from Lua

| Field | Type |
|---|---|
| `maxDistance` | Float |
| `searchAngleDegrees` | Float |
| `eyeHeight` | Float |
| `switchMouseDistance` | Float |
| `requireLineOfSight` | Bool |
| `isPlayerControlled` | Bool |
| `showSightRays` | Bool |

<sub>Defined in `Sunta/src/Sunta/Gameplay/LockOnComponent.h`</sub>

### Lock On Target

Marks an entity (an enemy, a boss) as something the player can lock on to

How to use it: in the editor choose Add Component > Lock On Target on the enemy The player needs the Lock On component too (Add Component > Lock On), then Q locks on to the enemy in front of the camera

A script turns 'isLockable' off when the enemy dies (or should not be targeted for another reason), the lock on it is dropped at once

| Field | Type |
|---|---|
| `isLockable` | Bool |
| `aimHeight` | Float |
| `aimBone` | String |

<sub>Defined in `Sunta/src/Sunta/Gameplay/LockOnTargetComponent.h`</sub>

### Healing Flask

A flask that heals with a few drinks, filled again at every rest (the Estus Flask of Dark Souls, the Flask of Crimson Tears of Elden Ring: the game gives it its own name)

How to use it: Add Component > Healing Flask on the player (or an enemy that drinks). 'displayName' is what the HUD shows, 'maxCharges' how many drinks it holds, each heals 'healAmount' health (at most up to the maximum) The player drinks with R: 'drinkAnimation' plays once on the upper body, the character walks slowly meanwhile and the health comes at 'healAtProgress' of the animation. A hit before that stops the drink and the charge is not used. Without an animation the drink heals at once 'hiddenWhileDrinking' is the name of a child entity whose Model disappears for the drink (the shield in the drinking hand), 'shownWhileDrinking' one that appears only then (a model of the flask held in that hand); empty = none 'healEffect' is the name of a child entity with a Particle Emitter that bursts with the heal (a warm glow), 'drinkSound' and 'healSound' are sound files or folders (see Audio.h). Resting at a bonfire and coming back after death fill the flask A script can use it: this.healingFlask:drink(), this.healingFlask.charges, this.healingFlask.maxCharges

| Field | Type |
|---|---|
| `displayName` | String |
| `maxCharges` | Int |
| `healAmount` | Float |
| `drinkAnimation` | String |
| `healAtProgress` | Float |
| `hiddenWhileDrinking` | String |
| `shownWhileDrinking` | String |
| `healEffect` | String |
| `drinkSound` | String |
| `healSound` | String |

<sub>Defined in `Sunta/src/Sunta/Gameplay/HealingFlaskComponent.h`</sub>

### Player HUD

What the player sees on the screen about itself: the health and stamina bars in the top left corner, a dot on the locked-on enemy, and the death message when it dies (the Dark Souls style HUD)

How to use it: Add Component > Player HUD on the player (the Souls Player has it). It reads the Health, Stamina and Lock On of the same entity. The bars grow with the maximum health and stamina, as in Dark Souls

| Field | Type |
|---|---|
| `pixelsPerHealthPoint` | Float |
| `pixelsPerStaminaPoint` | Float |
| `showLockOnMarker` | Bool |
| `deathMessage` | String |
| `style` | String |

<sub>Defined in `Sunta/src/Sunta/Gameplay/PlayerHUDComponent.h`</sub>

### Respawn

Brings the player back after death: a few seconds after "YOU DIED" the player stands up at the last bonfire it rested at (at the start of the level when it rested nowhere yet), healed, and every enemy is back at its place

How to use it: Add Component > Respawn on the player (the Souls Player has it), and put Bonfires in the level

| Field | Type |
|---|---|
| `respawnDelaySeconds` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/RespawnComponent.h`</sub>

### Level Up

Raising the character's level with the Currency while resting at a bonfire (leveling up in Dark Souls, Elden Ring...)

How to use it: Add Component > Level Up on the player (it needs Currency to pay with). While the player rests at a bonfire a menu shows the level and the attributes as in Dark Souls, each with its value now and after the level up ("10 -> 11"): W/S (or up/down) chooses an attribute, D/A (or right/left) adds or takes back a point, Enter pays and keeps them all Leaving the bonfire forgets the points not confirmed. Every level costs more: the price starts at 'firstLevelCost' and grows 'costGrowth' times with every level There are three attributes, each with its own effect in the engine and a name the game gives it ('displayName'):

- `- health:` 'healthPerPoint' more maximum health per point (Vigor)
- stamina: 'staminaPerPoint' more maximum stamina per point (Endurance)
- `- damage:` 'damagePerPoint' more weapon damage per point, 0.05 = +5% (Strength)

An attribute with an empty name is left out of the menu. The menu shows each one as 'startingAttributeValue' + the points The points are saved with the scene: the Health and Stamina in the scene are the values without them 'levelUpSound' plays and 'levelUpEffect' (a child entity with a Particle Emitter) bursts with every raised level A script can use it: this.levelUp.level, this.levelUp.cost, this.levelUp:raise("health"|"stamina"|"damage")

| Field | Type |
|---|---|
| `displayName` | String |
| `level` | Int |
| `firstLevelCost` | Int |
| `costGrowth` | Float |
| `startingAttributeValue` | Int |
| `healthAttribute` | String |
| `healthPerPoint` | Float |
| `healthPoints` | Int |
| `staminaAttribute` | String |
| `staminaPerPoint` | Float |
| `staminaPoints` | Int |
| `damageAttribute` | String |
| `damagePerPoint` | Float |
| `damagePoints` | Int |
| `levelUpSound` | String |
| `levelUpEffect` | String |

<sub>Defined in `Sunta/src/Sunta/Gameplay/LevelUpComponent.h`</sub>

### Currency

What the player collects from defeated enemies (the souls of Dark Souls, the runes of Elden Ring: the game names it)

How to use it: Add Component > Currency on the player, Add Component > Currency Reward on the enemies (how much each gives) 'displayName' is shown next to the amount in the HUD. With 'isDroppedOnDeath' (as in Dark Souls) dying leaves all of it where the player fell: walking back there gets it back, dying again on the way loses it for good 'dropMarker' is the name of an entity of the scene that shows the spot (e.g. one with a glowing Particle Emitter): it is moved there and its emitters run while something lies there. Within 'retrieveDistance' meters the player picks it up: with E and 'retrievePrompt' shown when 'isRetrievedWithKey', just by walking over it otherwise 'gainSound' plays when the amount grows (a kill, picking it up), 'retrieveSound' when the dropped amount is picked up A script can read and change it: this.currency.amount, this.currency:add(100)

| Field | Type |
|---|---|
| `displayName` | String |
| `amount` | Int |
| `isDroppedOnDeath` | Bool |
| `dropMarker` | String |
| `retrieveDistance` | Float |
| `isRetrievedWithKey` | Bool |
| `retrievePrompt` | String |
| `gainSound` | String |
| `retrieveSound` | String |

<sub>Defined in `Sunta/src/Sunta/Gameplay/CurrencyComponent.h`</sub>

### Weapon Trail

A glowing streak behind a swung weapon: the blade leaves a fading ribbon of light along the path of its swing

How to use it: Add Component > Weapon Trail on the weapon (a child entity of the character, like the Hitbox). The owner is the closest parent with Character Actions: the trail is drawn during the swing of its attacks (from just before the striking part until just after it), or always with 'onlyWhileStriking' off. 'baseOffset' and 'tipOffset' are where the blade starts and ends, in the space of the weapon (the Knight's sword: along +Y from the grip)

| Field | Type |
|---|---|
| `baseOffset` | Float3 |
| `tipOffset` | Float3 |
| `color` | Color |
| `intensity` | Float |
| `lifetimeSeconds` | Float |
| `onlyWhileStriking` | Bool |

<sub>Defined in `Sunta/src/Sunta/Effects/WeaponTrailComponent.h`</sub>

## Enemies and bosses

### Enemy

What every enemy has, whatever kind it is: its senses and its state machine

The kind of the enemy is a second component next to this one (Melee Enemy: a sword fighter, more kinds later), it brings the states that make the behaviour. Without a kind the enemy only has the basic states: Idle (stands still) and Dead (see BaseEnemyStates.h)

A Lua script can add its own states or replace any state by name, so a new behaviour doesn't need C++: this.enemy:addState("Idle", { update = function(enemy, deltaTime) ... return "Chase" end }) (see the Enemy bindings in LuaBindings.cpp for everything a state can ask and do)

With 'startsDormant' the enemy is not there until it is called: hidden, it doesn't think, nothing hurts it and it can't be locked on to. A script wakes it (a boss summoning help): move it where it should appear, then ally.enemy:wake(). Its body still stands where the scene has it, so place it out of the way (outside the arena)

Needs a Character Controller that is not player controlled (the enemy walks with it), Character Actions (to attack) and Lock On (to face the player). Add Component > Enemy adds the missing ones

| Field | Type |
|---|---|
| `sightDistance` | Float |
| `sightAngleDegrees` | Float |
| `hearingDistance` | Float |
| `eyeHeight` | Float |
| `startsDormant` | Bool |
| `isAlly` | Bool |

<sub>Defined in `Sunta/src/Sunta/Gameplay/EnemyComponent.h`</sub>

### Melee Enemy

A kind of enemy: a sword fighter that chases the player, attacks up close, circles and gives up when the player gets away With 'leapAttackDistance' set (and a Leap Attack animation in its Character Actions) it also jumps at a player who keeps a distance: anywhere between 'attackDistance' and 'leapAttackDistance' (a boss closing the gap)

How to use it: Add Component > Melee Enemy (it adds the Enemy it is built on). Its states are Idle, Chase, Attack, Recover, ReturnHome (see MeleeEnemy.h), the numbers below tune them in the Inspector

| Field | Type |
|---|---|
| `attackDistance` | Float |
| `runDistance` | Float |
| `giveUpDistance` | Float |
| `leapAttackDistance` | Float |
| `attackCooldownSeconds` | Float |
| `heavyAttackChance` | Float |
| `maxLightCombo` | Int |
| `finisherChance` | Float |
| `recoverSeconds` | Float |
| `dodgeChance` | Float |
| `dodgeDistance` | Float |
| `dodgeCooldownSeconds` | Float |
| `guardChance` | Float |
| `guardSeconds` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/MeleeEnemyComponent.h`</sub>

### Enemy Health Bar

The health of an enemy on the screen

How to use it: Add Component > Enemy Health Bar on an enemy with Health (the Rusted Knight has it) A normal enemy shows a small bar over its head for a while after it was hit, and while the player is locked on to it A boss ('isBoss' on) shows a long bar with its name at the bottom of the screen, from the moment it notices the player

| Field | Type |
|---|---|
| `isBoss` | Bool |
| `displayName` | String |
| `showSecondsAfterHit` | Float |
| `heightAboveFeet` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/EnemyHealthBarComponent.h`</sub>

### Currency Reward

How much Currency a character (or anything with Health) gives to the player who defeats it

How to use it: Add Component > Currency Reward on an enemy. It is given each time the enemy falls by the player's hand, so enemies brought back at a bonfire give it again (a defeated boss stays dead and gives it once)

| Field | Type |
|---|---|
| `amount` | Int |

<sub>Defined in `Sunta/src/Sunta/Gameplay/CurrencyComponent.h`</sub>

### Boss

A boss: an enemy that waits in an arena behind a fog gate (see FogGateComponent) and fights harder in its second phase

How to use it: on an enemy (e.g. a Rusted Knight) together with Enemy Health Bar with 'isBoss' on (its name at the bottom of the screen). The boss wakes up the moment the fog closes behind the player, wherever the player is in the arena Below 'phaseTwoHealthFraction' of its health it attacks more often ('phaseTwoAttackSpeedup') and its weapon hurts more How the second phase starts can be shown too (all optional):

- 'phaseTwoAnimation': an animation of its Model played once (a roar), hits don't stagger it meanwhile
- 'phaseTwoGlow': the emissive strength of its Model and of its children's (the weapon) in phase two,

pulsing a little (models with an *_emissive texture start to glow: cracks with embers, red-hot metal). 0 = no change

- 'phaseTwoGrowth': it grows to this size (1.2 = 20% bigger) during the first seconds of phase two
- Particle Emitters on it or on its children start emitting (with a burst) in phase two
- 'phaseTwoSound': heard from the boss when phase two starts (a roar); 'phaseTwoMusic': the music of the fight changes

to it (a sound file or a folder, see Audio.h; empty = the fog gate's battle music goes on)

- a different set of attacks (Character Actions): the 'phaseTwo...Animation' names (empty = keep the old one), their hit

windows, a shockwave of heavy attacks and leaps ('phaseTwoShockwaveRadius', 0 = keep) and longer combos A defeated boss stays dead: resting at a bonfire or dying brings back every other enemy, not this one

The component works with any kind of enemy: how the boss fights comes from its kind (Melee Enemy) or from Lua states (this.enemy:addState). A script can follow the fight and add its own phases: this.boss.phase (1 or 2), this.boss.isAwake, this.boss.isDefeated, e.g. if this.boss.phase == 2 then this.enemy:attack("heavy") end

| Field | Type |
|---|---|
| `phaseTwoHealthFraction` | Float |
| `phaseTwoDamageMultiplier` | Float |
| `phaseTwoAttackSpeedup` | Float |
| `phaseTwoAnimation` | String |
| `phaseTwoSound` | String |
| `phaseTwoMusic` | String |
| `phaseTwoGlow` | Float |
| `phaseTwoGrowth` | Float |
| `phaseTwoLight1Animation` | String |
| `phaseTwoLight2Animation` | String |
| `phaseTwoLight3Animation` | String |
| `phaseTwoHeavyAnimation` | String |
| `phaseTwoLightHitFrom` | Float |
| `phaseTwoLightHitTo` | Float |
| `phaseTwoHeavyHitFrom` | Float |
| `phaseTwoHeavyHitTo` | Float |
| `phaseTwoShockwaveRadius` | Float |
| `phaseTwoMaxLightCombo` | Int |
| `defeatCutscene` | String |
| `defeatCutsceneDelay` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/BossComponent.h`</sub>

### Fog Gate

The fog in the doorway of a boss arena (as in Dark Souls)

How to use it: on an entity standing in the doorway, with a Model (the fog) and a Box Collider that fills the doorway The arena is the box 'arenaCenter' +- 'arenaHalfExtents' (in the world). With 'isCrossedWithKey' (as in Dark Souls) the fog is a wall: standing close to it the player presses E and walks through it into the arena. Without it the fog can simply be walked through (its collider is a trigger). Once the player is inside the arena while a boss (BossComponent) is alive in it, the fog becomes a wall and the boss wakes up. When every boss of the arena is dead the fog disappears for good When the player dies, it comes back outside: the fog is passable again and the boss waits for the next try Particle Emitters on the fog or its children (swirling mist) stop when the fog disappears 'battleMusic' plays while the fog is closed (it fades out when the player dies or wins), 'victorySound' once when the last boss falls. Both are sound files or folders (see Audio.h), empty = silence A Lua script can read the fight: this.fogGate.isClosed, this.fogGate.isCleared

| Field | Type |
|---|---|
| `arenaCenter` | Float3 |
| `arenaHalfExtents` | Float3 |
| `victoryMessage` | String |
| `isCrossedWithKey` | Bool |
| `cutscene` | String |
| `battleMusic` | String |
| `battleMusicVolume` | Float |
| `victorySound` | String |

<sub>Defined in `Sunta/src/Sunta/Gameplay/FogGateComponent.h`</sub>

### Spell Caster

A spell a character can cast: an animation, and at its moment the Projectile it throws (see ProjectileComponent)

How to use it: Add Component > Spell Caster on a child entity of the character, placed where the spell leaves from (on a bone: the hand, the tip of a staff). The caster is the closest parent with Character Actions. A character with several spells has several such children (a fireball, a pillar of fire). A Melee Enemy casts a spell that is ready when the player is between 'minDistance' and 'maxDistance' and in sight, instead of walking up to it

The cast is an action of the character: a hit staggers it and the spell is lost. Until 'releaseAt' the caster turns after its target (Character Actions 'attackTurnDegreesPerSecond'), then the projectiles leave at the target: 'projectileCount' of them fanned out by 'spreadDegrees' (a volley that is hard to side step), or with 'atTargetsFeet' they appear under the target (the first right under it, the others around it)

The player casts a spell with its 'castKey' (a letter or a digit, e.g. R), at the locked-on enemy or straight ahead. With 'isCastOnRequestOnly' an enemy never chooses the spell itself: only a script casts it (spell.spellCaster:cast(), e.g. a boss's great spell at the start of its phase)

| Field | Type |
|---|---|
| `castAnimation` | String |
| `projectile` | String |
| `projectileCount` | Int |
| `spreadDegrees` | Float |
| `releaseAt` | Float |
| `atTargetsFeet` | Bool |
| `dropHeight` | Float |
| `minDistance` | Float |
| `maxDistance` | Float |
| `cooldownSeconds` | Float |
| `chargeEffect` | String |
| `chargeParticlesPerSecond` | Float |
| `castSound` | String |
| `castKey` | String |
| `displayName` | String |
| `isCastOnRequestOnly` | Bool |
| `isUninterruptible` | Bool |

<sub>Defined in `Sunta/src/Sunta/Gameplay/SpellCasterComponent.h`</sub>

### Projectile

What a spell throws: a ball of fire, a bolt, or a pillar of flame bursting from the ground (see SpellCasterComponent)

How to use it: Add Component > Projectile on an entity of the scene that stands for every projectile of its kind (a template, like the hit effects: it is never moved itself). A Spell Caster names it. A projectile is a moving point that leaves the specks of the emitter named 'trailEffect' behind it: give the scene an entity with a Particle Emitter (not emitting) for its look. It hurts the first character of the other side it touches (a roll goes through it, a shield blocks it, a parry can't turn it away) and bursts on walls

A projectile can have a body: give the template child entities with a Model (a thrown bottle, an arrow, a skull), a Liquid Orb (a ball of water) or a Glow Sprite (a spot of light). Each projectile in the air borrows one of them, carries it along its way (the front of the model first, tumbling with 'spinDegreesPerSecond') and gives it back when it is gone, so as many bodies can be seen at once as the template has children; a projectile that finds none free flies with its trail only. Don't rotate or scale the template itself (scale the children)

With 'fuseSeconds' set it doesn't burst on contact: it waits (its trail marks the place, the player has time to run), then bursts once and hurts everyone of the other side within 'radius' (a pillar of fire under the player)

| Field | Type |
|---|---|
| `speed` | Float |
| `damage` | Float |
| `poiseDamage` | Float |
| `radius` | Float |
| `homingDegreesPerSecond` | Float |
| `lifetimeSeconds` | Float |
| `fuseSeconds` | Float |
| `trailEffect` | String |
| `trailParticlesPerSecond` | Float |
| `spinDegreesPerSecond` | Float |
| `launchDelaySeconds` | Float |
| `formSeconds` | Float |
| `aimsAfterForming` | Bool |
| `bodyFacesFlight` | Bool |
| `spinsAroundFlight` | Bool |
| `hitEffect` | String |
| `hitBurst` | Int |
| `hitSound` | String |
| `hitStopSeconds` | Float |
| `cameraShake` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/ProjectileComponent.h`</sub>

## The world

### Bonfire

A place to rest (the Dark Souls bonfire)

How to use it: Entity > Create Bonfire (or the toolbar) makes one with the model and a warm light. By hand: Add Component > Bonfire Near it the player sees "Rest at the bonfire [E]". Resting heals the player, refills the stamina, brings every enemy back to life at its starting place, and makes this bonfire the place the player comes back to after dying The player needs a Respawn component for that (the Souls Player has one)

| Field | Type |
|---|---|
| `interactDistance` | Float |
| `respawnOffset` | Float3 |
| `restPrompt` | String |
| `leavePrompt` | String |
| `litMessage` | String |

<sub>Defined in `Sunta/src/Sunta/Gameplay/BonfireComponent.h`</sub>

### Cutscene

A short scene played by the engine itself (the camera flies, characters move), e.g. a boss rising before the fight

How to use it: Add Component > Cutscene on an empty entity, then give it children: the points the camera flies through, in order (their positions; the camera passes them smoothly in 'durationSeconds'). The camera looks at the entity named 'lookAtEntity' ('lookAtHeight' above its feet), or along its path when it is empty At 'animationAtSeconds' the entity named 'animationEntity' plays its animation 'animationName' once (a roar) During the last 'titleSeconds' the 'title' fades in over the picture (the boss's name). Black bars frame the picture While it plays nobody fights: the enemies wait and the player's keys do nothing; E skips it ('canBeSkipped') It starts from a Fog Gate (its 'cutscene': played the first time the fog closes behind the player) or from a script: this.cutscene:play() With 'videoFile' (e.g. @game/Videos/intro.mpg) a video plays instead of the engine's camera: a cutscene made elsewhere, e.g. by an AI video generator. It must be MPEG-1 (see VideoPlayer.h for the ffmpeg command), it plays without sound and ends with the video (or E). The camera points, the animation and the title are not used then. Its 'sound' (the video's own sound track as a separate file, e.g. @game/Sounds/Film/intro.ogg) starts with it and fades out when it ends With 'waitForKey' (a letter or a digit, e.g. "3") it is a moment the player answers: the camera reaches the end of its path and stays there, 'waitPrompt' is shown, and the cutscene ends only when that key is pressed (a script then goes on: this.cutscene.wasAnswered). With 'waitPresses' above 1 the key must be pressed that many times, quickly enough: what was pressed drains away at one press per 'pressWindowSeconds' (a slower tempo never gets there), and a bar under the prompt shows how far the player got (getting up off the ground, breaking free) 'blendInSeconds' above 0: the camera doesn't cut to the first point, it flies there from wherever it was (the shot before it, or the game's camera) over that long - shots of one sequence flow into each other 'fadeSeconds' above 0: no sudden cut - the game fades to black, the cutscene comes out of the black, and when it ends the game comes back out of the black (each fade that long)

| Field | Type |
|---|---|
| `durationSeconds` | Float |
| `lookAtEntity` | String |
| `lookAtHeight` | Float |
| `lookAlsoAtEntity` | String |
| `animationEntity` | String |
| `animationName` | String |
| `animationAtSeconds` | Float |
| `title` | String |
| `titleSeconds` | Float |
| `canBeSkipped` | Bool |
| `videoFile` | String |
| `sound` | String |
| `waitForKey` | String |
| `waitPrompt` | String |
| `pressSound` | String |
| `waitPresses` | Int |
| `pressWindowSeconds` | Float |
| `blendInSeconds` | Float |
| `fadeSeconds` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/CutsceneComponent.h`</sub>

### Audio Source

A sound that belongs to an entity: a bonfire crackling, wind over the ruins, a chant in a chapel

How to use it: Add Component > Audio Source, 'sound' = a file or a folder of files (@game/Sounds/Ambience/fire.ogg; a folder plays one of them at random). With 'playOnStart' it starts with the game, with 'isLooping' it never ends A spatial source ('isSpatial') is heard from the entity and follows it: full 'volume' up to 'minDistance' meters, quieter further away, no quieter beyond 'maxDistance'. Without it the sound is the same everywhere (wind, music) A script can play it: this.audioSource:play(), this.audioSource:stop(), this.audioSource.volume = 0.5

| Field | Type |
|---|---|
| `sound` | String |
| `volume` | Float |
| `isLooping` | Bool |
| `playOnStart` | Bool |
| `isSpatial` | Bool |
| `minDistance` | Float |
| `maxDistance` | Float |

<sub>Defined in `Sunta/src/Sunta/Audio/AudioSourceComponent.h`</sub>

### Script

<sub>Defined in `Sunta/src/Sunta/ECS/Component.h`</sub>

## Effects

### Particle Emitter

Throws out small glowing specks: embers, sparks, ash, magic. They shine (their color is added to the picture, so many of them together get brighter) and face the camera

How to use it: Add Component > Particle Emitter on any entity (a child of a character on a bone, e.g. 'Chest', makes the specks rise from it). Each speck is born somewhere in the box 'spawnHalfExtents' around the entity, flies with 'velocity' (plus a random part up to 'velocityRandomness' in every direction), is pulled by 'gravity' (e.g. -9.8 down for sparks, a little up for embers), and over its 'lifetimeSeconds' turns from 'startColor' and 'startSize' to 'endColor' and 'endSize' (a black end color fades it out) 'style' gives them the look of fire or smoke instead of a spot of light; 'spawnShape' a ball or a dome of 'spawnRadius' instead of the box, from which they fly out at 'outwardSpeed'. Tens of thousands at once are fine (drawn together) 'drag' makes a fast burst hang in the air; 'hasMiddleColor' a three-color fade (hot, burning, ash); 'style' Spark streaks along the flight ('stretch'), Ring a swelling circle of shock 'isEmitting' off stops new specks (the ones in the air live on). A script: this.particles.isEmitting = true, this.particles:burst(50) throws 50 at once (a hit, an explosion)

| Field | Type |
|---|---|
| `isEmitting` | Bool |
| `particlesPerSecond` | Float |
| `lifetimeSeconds` | Float |
| `spawnHalfExtents` | Float3 |
| `velocity` | Float3 |
| `velocityRandomness` | Float |
| `gravity` | Float3 |
| `startSize` | Float |
| `endSize` | Float |
| `startColor` | Color |
| `endColor` | Color |
| `intensity` | Float |
| `maxParticles` | Int |
| `isDark` | Bool |
| `style` | Int |
| `spawnShape` | Int |
| `spawnRadius` | Float |
| `outwardSpeed` | Float |
| `sizeRandomness` | Float |
| `spinDegreesPerSecond` | Float |
| `drag` | Float |
| `stretch` | Float |
| `hasMiddleColor` | Bool |
| `middleColor` | Color |
| `burstAmount` | Int |
| `flipbookTexture` | String |
| `flipbookColumns` | Int |
| `flipbookRows` | Int |
| `flipbookLoops` | Float |
| `flipbookRandomStart` | Bool |

<sub>Defined in `Sunta/src/Sunta/Effects/ParticleEmitterComponent.h`</sub>

### Effect Shape

One shape of light played where something happens: a slash, a shockwave, a column of fire. The way the effects of fighting games and spells are made (meshes with scrolling noise, a white-hot core and a colored edge, eaten away as they die), together with Particle Emitters as children of one entity

How to use it: an entity (anywhere, e.g. under the arena) with an Effect Shape, and maybe Particle Emitters and more Effect Shapes as its children: the whole thing is one effect. It is played by name where it should happen: a script calls this.world:effect("Heavy Impact", position, direction), a weapon's or a spell's 'hit effect' names it. Each play adds one shape at that place, facing 'direction' (level, along the ground), and bursts the children The shape grows from 'startRadius' to 'endRadius' over 'lifetimeSeconds' (fast at first, like a blast); its look: 'coreColor' (white-hot) inside, 'edgeColor' outside, 'intensity' how strongly it shines (above 1 it blooms), 'noiseScale' and 'scrollSpeed' the ragged pattern flowing through it, 'dissolve' how much of it noise eats away at its end. 'lightIntensity' above 0 also lights the world around it for its short life

| Field | Type |
|---|---|
| `kind` | Int |
| `lifetimeSeconds` | Float |
| `startRadius` | Float |
| `endRadius` | Float |
| `thickness` | Float |
| `height` | Float |
| `arcDegrees` | Float |
| `rotationDegrees` | Float3 |
| `rotationRandomness` | Float3 |
| `offset` | Float3 |
| `spinDegreesPerSecond` | Float |
| `twistDegrees` | Float |
| `coreColor` | Color |
| `edgeColor` | Color |
| `intensity` | Float |
| `noiseScale` | Float |
| `scrollSpeed` | Float |
| `dissolve` | Float |
| `lightIntensity` | Float |
| `lightRadius` | Float |
| `lightColor` | Color |

<sub>Defined in `Sunta/src/Sunta/Effects/EffectShapeComponent.h`</sub>

### Swarm

Thousands of copies of one model moving together: a cloud of missiles that bursts out of a point into a turning sphere, spreads over the whole sky, hangs there, then rains down glowing and stays stuck in the ground burning (a great spell, a storm of blades, a flock seen from afar). Every copy has its own size, turn and timing. All of them are drawn at once (instancing), so a few thousand cost about as much as one model

How to use it: Add Component > Swarm on an entity where the cloud is born (its center), choose 'modelName' (a model without bones) and 'count'. A script moves it from stage to stage by changing the numbers below: this.swarm.isVisible = true this.swarm.gathered = 1     -- 0 = all in the center, 1 = all on the sphere of 'radius' (each leaves at its own moment) this.swarm.radius = 30      -- a growing sphere this.swarm.spread = 1       -- 0 = on the sphere, 1 = spread over the sky box ('skyCenter', 'skyHalfExtents')

- `this.swarm:fall()` -- they come down, one after another during 'fallSpreadSeconds'
- `this.swarm:reset()` -- back to the start, hidden

| Field | Type |
|---|---|
| `isVisible` | Bool |
| `count` | Int |
| `seed` | Int |
| `pieceSize` | Float |
| `sizeVariation` | Float |
| `radius` | Float |
| `shellThickness` | Float |
| `spinDegreesPerSecond` | Float |
| `tumbleDegreesPerSecond` | Float |
| `skyCenter` | Float3 |
| `skyHalfExtents` | Float3 |
| `fallSpeed` | Float |
| `fallSpreadSeconds` | Float |
| `groundHeight` | Float |
| `stuckSeconds` | Float |
| `glowColor` | Color |
| `trailLength` | Float |
| `hitEffect` | String |
| `hitBurst` | Int |

<sub>Defined in `Sunta/src/Sunta/Effects/SwarmComponent.h`</sub>

## Other

### Follow Camera

A camera that circles around the player and follows it, no script needed

How to use it: in the editor choose Add Component > Follow Camera on the entity that has the Camera component (it adds the Camera component when it is missing). The player is the entity with a Character Controller that is player controlled After pressing Play press TAB to give the mouse to the game: the mouse turns the camera around the player Walls between the player and the camera push the camera closer, so the player is never hidden behind a wall

With a Lock On on the player the camera turns on its own to the locked enemy, so the player and the enemy are both in view The Transform of the camera entity is the starting view: the camera begins looking the way the entity looks

Every number below shows up in the Inspector, so the feel of the camera is tuned there

| Field | Type |
|---|---|
| `distance` | Float |
| `pivotHeight` | Float |
| `shoulderOffset` | Float |
| `viewYawOffsetDegrees` | Float |
| `viewPitchOffsetDegrees` | Float |
| `mouseSensitivity` | Float |
| `invertY` | Bool |
| `minPitchDegrees` | Float |
| `maxPitchDegrees` | Float |
| `followSharpness` | Float |
| `lockOnPitchDegrees` | Float |
| `lockOnSharpness` | Float |
| `lockOnSideDegrees` | Float |
| `shakeDegrees` | Float |
| `shakeFrequency` | Float |
| `shakeFadePerSecond` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/FollowCameraComponent.h`</sub>

### Inventory

What the player carries (see PickupComponent): item ids, one entry per item picked up. Scripts ask it (this.inventory:has("crypt_key"), :count("ember")) and change it (:add, :remove)

<sub>Defined in `Sunta/src/Sunta/Gameplay/PickupComponent.h`</sub>

### Loot Drop

An enemy that leaves something behind when it dies: a copy of the Pickup entity named 'pickupTemplate' appears where it fell (with 'chance', 0..1). 'onlyOnce': a book or a key drops the first time only, not again after the enemy came back at a bonfire

| Field | Type |
|---|---|
| `pickupTemplate` | String |
| `chance` | Float |
| `onlyOnce` | Bool |

<sub>Defined in `Sunta/src/Sunta/Gameplay/PickupComponent.h`</sub>

### Mount

A mount the player can ride (a horse, a beast of burden, a war wolf): the open-world way to travel

How to use it: Add Component > Mount on a character that has a Character Controller (turn its 'isPlayerControlled' off: the mount moves only when ridden), a Model with its gaits and a Character Animation (idle / walk / run are the mount's stand, walk and gallop). Next to it the player sees 'mountPrompt' and mounts with E; while riding W A S D steer it (relative to the camera), Left Shift gallops, E again dismounts beside it The rider sits 'seatOffset' meters from the mount's feet (in the mount's own space: +Y up, -Z its front), turned with it, and plays 'riderIdleAnimation' / 'riderWalkAnimation' / 'riderRunAnimation' by the mount's speed. Its own keys and attacks wait until it dismounts. The camera pivot rises by 'cameraRise' and the camera stands 'cameraPullBack' meters farther while riding

| Field | Type |
|---|---|
| `seatOffset` | Float3 |
| `mountDistance` | Float |
| `mountPrompt` | String |
| `dismountPrompt` | String |
| `riderIdleAnimation` | String |
| `riderWalkAnimation` | String |
| `riderRunAnimation` | String |
| `runAnimationFromSpeed` | Float |
| `cameraRise` | Float |
| `cameraPullBack` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/MountComponent.h`</sub>

### Pickup

Something lying in the world the player can pick up: an item, a key, a rune, a page or a book whose text is read aloud (the lore of souls-likes, the books of Diablo)

How to use it: Add Component > Pickup on an entity with a Model (and a Particle Emitter child for its glint). Near it the player sees 'prompt' and takes it with E: it disappears, 'itemId' goes into the player's Inventory and "Obtained: <displayName>" shows. A book ('readText' set) is read aloud: 'voiceSound' plays over the quieted music while the text stays on the screen ('readSeconds', 0 = as long as the voice or the length of the text) An enemy drops a copy of a pickup when it dies (see LootDropComponent): the pickup then serves as a template

| Field | Type |
|---|---|
| `itemId` | String |
| `displayName` | String |
| `prompt` | String |
| `interactDistance` | Float |
| `readText` | String |
| `voiceSound` | String |
| `readSeconds` | Float |
| `pickupSound` | String |

<sub>Defined in `Sunta/src/Sunta/Gameplay/PickupComponent.h`</sub>

### Posture

The posture of a character, as in Sekiro: how long it can keep its guard against pressure before it breaks

How to use it: Add Component > Posture on a character with Health (an enemy, a boss, the player). Its posture fills:

- from every weapon blow that hurts it ('fromHits' x the blow's poise damage),
- from blows it blocks ('fromBlocks' x the blocked damage, on top of the stamina the shield takes),
- when its own blow is parried ('fromBeingParried', the deflect: the parrying one's whole point).

It drains by itself 'recoverDelaySeconds' after the last fill, faster while the character is healthy (as in Sekiro: a hurt enemy keeps its posture broken longer). Full posture breaks: the character staggers for 'brokenSeconds' (the parried animation), and the first weapon blow from the front in that time is a finisher - 'finisherDamage' of its full health (1 kills it) - with slow motion, the finisher's effect and an event scripts see ("FINISHER") A character with Posture and no hits taken shows a posture bar over its health bar (the HUD) while it fills

| Field | Type |
|---|---|
| `maxPosture` | Float |
| `fromHits` | Float |
| `fromBlocks` | Float |
| `fromBeingParried` | Float |
| `recoverPerSecond` | Float |
| `recoverDelaySeconds` | Float |
| `lowHealthRecoverFactor` | Float |
| `brokenSeconds` | Float |
| `finisherDamage` | Float |
| `finisherSlowMotion` | Float |
| `finisherSlowSeconds` | Float |
| `breakEffect` | String |
| `finisherEffect` | String |

<sub>Defined in `Sunta/src/Sunta/Gameplay/PostureComponent.h`</sub>

### Summon

Calling allies into a fight (as Elden Ring's spirit ashes, Overlord's minions): G sets the entities named in 'allies' (comma separated, e.g. "Court Knight Ally, Court Knight Ally") down at the player's side; they fight the enemies until they fall or 'lifetimeSeconds' pass, then fade away. 'maxCharges' calls between two rests at a bonfire (a rest also sends the allies away)

How to use it: Add Component > Summon on the player. Each ally is an enemy of the scene used as a template: an Enemy with 'isAlly' and 'startsDormant' (it waits hidden, out of the way), its body on the player's collision layer and its weapon's Hitbox on the player's hitbox layer. 'summonEffect' bursts where each one appears (a Particle Emitter)

| Field | Type |
|---|---|
| `allies` | String |
| `maxCharges` | Int |
| `lifetimeSeconds` | Float |
| `summonEffect` | String |
| `calledText` | String |

<sub>Defined in `Sunta/src/Sunta/Gameplay/SummonComponent.h`</sub>

### Terrain

The ground of an open world: hills, valleys and roads from a heightmap (a grey picture: black is the lowest, white the highest). Bodies (the player, enemies, a horse) stand on it and walk up and down its slopes

How to use it: Add Component > Terrain on an entity whose Model is the terrain's mesh made from the same picture (e.g. Tools/Blender: a grid displaced by the heightmap). 'heightmap' is a .png (16-bit for smooth slopes), 'sizeX' / 'sizeZ' its width and depth in meters, centered on the entity's position; 'heightScale' the height of white above the entity. Outside the picture there is no ground from it (box colliders still are)

| Field | Type |
|---|---|
| `heightmap` | String |
| `sizeX` | Float |
| `sizeZ` | Float |
| `heightScale` | Float |

<sub>Defined in `Sunta/src/Sunta/Gameplay/TerrainComponent.h`</sub>

### Wrath

Wrath, a rage that grows in a fight (as the Spartan Rage of God of War): it fills from the damage the character deals and takes and from the blows it deflects; once full, the player sets it loose (V) - for 'durationSeconds' its blows hurt 'damageMultiplier' times more and fill the enemies' posture 'postureMultiplier' times faster, the figure burns in 'glowColor' (its outline and an aura). It drains away slowly when the fighting stops

How to use it: Add Component > Wrath on the player (with Health and Character Actions). The HUD shows a thin bar under the stamina while it fills, pulsing when full. A script reads it: this.wrath.current, this.wrath.isActive

| Field | Type |
|---|---|
| `maxWrath` | Float |
| `fromDamageDealt` | Float |
| `fromDamageTaken` | Float |
| `fromDeflects` | Float |
| `drainPerSecond` | Float |
| `drainDelaySeconds` | Float |
| `durationSeconds` | Float |
| `damageMultiplier` | Float |
| `postureMultiplier` | Float |
| `glowColor` | Color |
| `releaseEffect` | String |

<sub>Defined in `Sunta/src/Sunta/Gameplay/WrathComponent.h`</sub>

