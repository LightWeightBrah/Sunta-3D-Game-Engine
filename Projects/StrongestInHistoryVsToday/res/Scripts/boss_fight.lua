-- The fight of this project: one boss in four phases, loosely following the film's soundtrack
--
-- Put on the boss (an enemy with Boss, Melee Enemy and Spell Caster children). Everything it uses is general:
-- the Health floor ('minimum') keeps the boss alive until its last phase, 'isInvulnerable' covers the changes of phase,
-- a dormant enemy is woken as the summon, the sky belongs to the Environment and the temple is moved by its Transform
--
--   Phase 1  Strongest in History : blows and volleys of orbs
--   Phase 2  Domain Expansion     : the sky turns, the temple rises, fire under the player's feet, the domain heals him.
--                                   The player's own domain (key 2) shatters it for a while
--   Phase 3  The Summon           : the boss can't be hurt and channels spells from afar while his summon fights
--   Phase 4  Last Stand           : the summon is gone, the domain is back for good and everything comes faster
--
-- Through all of it he fights like the film's sorcerers do: he raises his forearms against punches (the state "Guard"),
-- blinks behind a player who keeps away, throws rings of orbs around himself and draws lines of fire across his domain
-- (projectiles launched by this script, not by a cast)

-- ========== The names of the scene ==========

local PlayerName        = "Player"
local SummonName        = "Summon"
local SkyName           = "Sky"
local TempleName        = "Domain Temple"
local AshName           = "Domain Ash"
local PlayerDomainSpell = "Player Domain Spell"
local DomainPillars     = "Boss Domain Pillars Spell"
local OrbStorm          = "Boss Orb Storm Spell"

local PlayerGreatSpell  = "Player Great Orb Spell"

-- What the screen calls each technique while it is being cast (the spells of the scene by their entities' names)
local TechniqueNames = {
    ["Boss Orb Volley Spell"]     = "Hush Money",
    ["Boss Cleave Spell"]         = "Black Book: Torn Pages",
    ["Boss Fire Pillar Spell"]    = "Private Jet: Air Strike",
    ["Boss Domain Pillars Spell"] = "Private Jet: Carpet Bombing",
    ["Boss Orb Storm Spell"]      = "Hush Money: Bailout",
    ["Player Orb Spell"]          = "Solid Baby Oil",
    ["Player Great Orb Spell"]    = "Imaginary Technique: Baby Oil Detonation",
    ["Player Domain Spell"]       = "Domain Expansion: Divine Baby Oil Evolve",
}
local RingTechnique = "Hidden Cameras: Flash"
local LineTechnique = "Private Jet: Strafing Run"

local RainTechnique = "Island Temple: Hush Money Rain"
local JetSpells = { "Boss Fire Pillar Spell", "Boss Domain Pillars Spell" }

local Jet               = "Private Jet"
local RainBundle        = "Domain Rain"
local RingOrb           = "Boss Ring Orb"
local LineFire          = "Domain Line Fire"
local BlinkEffect       = "Blink Burst"

-- The film's music alone (its song, Lady Gaga's Judas, from the second the fight starts: the film's music is that
-- song 34.28 s ahead), so the soundtrack's clock stays the film's; what the characters say in the film is played
-- by itself at the moment it belongs to (O.Voice*, cut from the film without its song: AIContext/scripts)
local FightMusic  = "@game/Sounds/Film/fight_music.ogg"
local SummonMusic = "@game/Sounds/Music/boss_phase_two.ogg"
local RoarSound   = "@game/Sounds/Boss/roar.ogg"
local BlinkSound  = "@game/Sounds/Spells/blink.ogg"
local RingSound   = "@game/Sounds/Spells/camera_shutter.ogg"
local JetSound    = "@game/Sounds/Spells/jet_pass.ogg"
local RumbleSound = "@game/Sounds/Spells/rumble.ogg"
local BoomSound   = "@game/Sounds/Spells/deep_boom.ogg"

-- ========== The rhythm of the fight ==========

-- Seconds of the fight (of the film's soundtrack) at which a phase starts at the latest, and how much earlier
-- the player's blows may start it. In between the boss's health can't fall below the floor of the phase
-- (the domain: the sign of his fingers at 1:20.8 of the film, see domainOpeningSteps)
local DomainAtSeconds = 44.6
local SummonAtSeconds = 121
local EarlyWindowSeconds = 15

local DomainHealthFraction = 0.66
local SummonHealthFraction = 0.34
local FloorMargin = 0.04

-- The orb that ends the film's opening has just hit the boss
local OpeningHitDamage = 70

local PhaseChangeInvulnerableSeconds = 3.0
local DomainHealPerSecond   = 6
local DomainSpellEverySeconds = 6.5
local DomainBrokenSeconds   = 18
local ChannelSpellEverySeconds = 10
local SummonAppearsAhead    = 4.5

-- The guard: how often he answers a punch thrown close to him by raising his forearms, and for how long
local GuardChance   = 0.45
local GuardDistance = 2.6
local GuardSeconds  = 0.9

-- The blink: after the player kept further than BlinkFromDistance for BlinkAfterSeconds he appears behind them.
-- He needs BlinkStrikeDelay before his first blow there (time to turn around or roll)
local BlinkFromDistance = 11
local BlinkAfterSeconds = 2.2
local BlinkEverySeconds = 9
local BlinkBehindPlayer = 1.7
local BlinkStrikeDelay  = 0.5

-- A ring of orbs flying out from him in all directions (roll through it, or stand in a gap)
local RingOrbs         = 14
local RingEverySeconds = 13
local RingHeight       = 1.1

-- Lines of fire across the domain: three rows, one after another like a wave, each this long
local LineEverySeconds = 10
local LineFires        = 9
local LineSpacing      = 2.3
local LineGap          = 3.6
local LineWaveSeconds  = 0.4

-- The jet that flies over the arena before fire comes down: how high, and how far before its target it appears
local JetHeight      = 15
local JetRunUp       = 52

-- The rain of the domain: bundles thrown from above the far wall (where the temple stands) at where the player is,
-- in a few waves when the domain opens and again every RainEverySeconds while it stands
local RainFrom         = vec3(0, 17, -80)
local RainWidth        = 44
local RainWaves        = 3
local RainPerWave      = 9
local RainWaveSeconds  = 0.7
local RainScatter      = 5.5
local RainEverySeconds = 16

-- His great patterns (the rain, the lines of fire, the ring, the bombing of the domain) take turns: after one starts
-- the next waits this long, so the player always has a moment to answer with blows of their own
local PatternGapSeconds = 5.5
local LastStandGapSeconds = 3.5

-- The last gamble: below this part of his health, once, three rings from the middle of the arena
local GambleHealthFraction = 0.14

-- Where he may appear (inside the walls of the arena)
local ArenaMin = vec3(-26, 0, -76)
local ArenaMax = vec3(26, 0, -24)
local ArenaCenter = vec3(0, 0, -50)

local TempleRisenY  = -1.8
local TempleSunkenY = -48
local SignCutscene   = "Domain Sign"
local RevealCutscene = "Domain Reveal"
local SmileCutscene  = "Domain Smile"
local WheelCutscene  = "Summon Wheel"
local SwarmEffect    = "Domain Swarm"
local BossEyes       = { "Boss Eye R", "Boss Eye L" }

-- Where he stands on the dome of his temple while it rises (from the temple's own place)
-- His feet on the golden dome (measured on the model: the dome's top is 18.0 m of the model, 21.6 m at the temple's
-- scale 1.2, with a spike on it; he stands 1 m before the spike, towards the door, where the gold is 21.5 m high)
local OnTheDome = vec3(0.32, 21.5, -2.2)

-- Where he is back on the ground after the blast, and how long he stays a black shape with white eyes
local BackOnTheGround = vec3(0, 0, -62)
-- Before the summoning he rises this high on his back, over this long
local FloatHeight = 4.5
local FloatSeconds = 2.4

-- The camera: close for the fist fight (the film's camera is right in it), farther while the three-meter summon fights
local CameraName = "Camera"
local FistFightCameraDistance = 4.3
local GiantCameraDistance = 5.8
local CameraZoomPerSecond = 1.5

local SilhouetteSeconds = 4.0
local SilhouetteFadeSeconds = 1.6
local TempleRiseSeconds = 5

-- ========== The looks of the world ==========

local Dusk = {
    light = vec3(0.95, 0.7, 0.52), ambient = vec3(0.24, 0.19, 0.17), fog = vec3(0.2, 0.12, 0.09),
    fogDensity = 0.012, sky = 0.85, exposure = 0.85, bloom = 1.0
}
local Domain = {
    -- (the film: the sky turns to fire and the ground glows when he raises his fingers)
    light = vec3(1.0, 0.52, 0.28), ambient = vec3(0.3, 0.14, 0.08), fog = vec3(0.34, 0.12, 0.04),
    fogDensity = 0.006, sky = 1.5, exposure = 0.85, bloom = 1.3
}
local Shattered = {
    light = vec3(0.6, 0.75, 1.0), ambient = vec3(0.16, 0.18, 0.26), fog = vec3(0.1, 0.13, 0.22),
    fogDensity = 0.02, sky = 1.2, exposure = 0.9, bloom = 1.5
}
local Dawn = {
    light = vec3(1.0, 0.9, 0.75), ambient = vec3(0.3, 0.28, 0.26), fog = vec3(0.3, 0.26, 0.22),
    fogDensity = 0.012, sky = 2.2, exposure = 0.9, bloom = 1.0
}

-- ========== What the fight remembers ==========

local phase = 0              -- 0 = not started
local seconds = 0            -- since the fight started
local invulnerableLeft = 0
local domainIsUp = false
local domainBrokenLeft = 0
local domainSpellIn = 0
local channelSpellIn = 0
local isChanneling = false
local playerDomainCasts = 0
local hasAnnouncedSummon = false
local hasSeenDefeat = false
local flash = 0              -- a white flash of the picture fading out (0..1)

local farSeconds = 0         -- how long the player has kept away
local blinkIn = 0            -- seconds until he may blink again
local ringIn = 0
local lineIn = 0
local linesAlongX = true
local wasPlayerAttacking = false
local hasGambled = false
local timers = {}            -- things to do a moment later: { seconds left, function }
local caption, captionLeft = nil, 0   -- the name of a technique thrown by the script itself
local playerGreatCasts = 0
local jetCasts = {}          -- how many times each of the jet's spells was seen released
local rainIn = 0
local domainHintLeft = 0
local patternLeft = 0        -- seconds until the next of his great patterns may start (they take turns)
local show = nil             -- the steps of a staged moment still to come (the opening of the domain, the summoning)
local isOnTheDome = false
local swarmIn = 0
local silhouetteLeft = 0
local awaitingDetonation = false   -- after the domain's opening: the player's answer is asked for
local shatterDomain                -- (defined with the domain, called earlier by the watch on the player's spells)

local look = nil             -- the look the world has now (a table like Dusk)
local lookFrom, lookTo = nil, nil
local lookSeconds, lookDuration = 0, 0

local templeFromY, templeToY = TempleSunkenY, TempleSunkenY
local templeSeconds, templeDuration = 0, 0

-- ========== Small helpers ==========

local function find(name) return this.world:find(name) end

local function mix(a, b, t) return a + (b - a) * t end

local function mixLook(a, b, t)
    return {
        light = mix(a.light, b.light, t), ambient = mix(a.ambient, b.ambient, t), fog = mix(a.fog, b.fog, t),
        fogDensity = mix(a.fogDensity, b.fogDensity, t), sky = mix(a.sky, b.sky, t),
        exposure = mix(a.exposure, b.exposure, t), bloom = mix(a.bloom, b.bloom, t)
    }
end

local function applyLook(l)
    local sky = find(SkyName)
    if not sky then return end

    if sky.light then
        sky.light.color = l.light
        sky.light.ambient = l.ambient
    end

    local environment = sky.environment
    if environment then
        environment.fogColor = l.fog
        environment.fogDensity = l.fogDensity
        environment.skyBrightness = l.sky
        environment.exposure = l.exposure + flash * 2.5
        environment.bloomIntensity = l.bloom + flash
    end
end

local function changeLook(target, duration)
    lookFrom, lookTo = look or Dusk, target
    lookSeconds, lookDuration = 0, duration
end

local function updateLook(deltaTime)
    flash = math.max(flash - deltaTime * 0.9, 0)

    if lookTo then
        lookSeconds = lookSeconds + deltaTime
        local t = math.min(lookSeconds / math.max(lookDuration, 0.01), 1)
        look = mixLook(lookFrom, lookTo, t * t * (3 - 2 * t))
        if t >= 1 then lookTo = nil end
    end

    if look then applyLook(look) end
end

local function moveTemple(toY, duration)
    local temple = find(TempleName)
    if not temple then return end

    templeFromY, templeToY = temple.transform.position.y, toY
    templeSeconds, templeDuration = 0, duration
end

local function updateTemple(deltaTime)
    local temple = find(TempleName)
    if not temple or templeSeconds >= templeDuration then return end

    templeSeconds = math.min(templeSeconds + deltaTime, templeDuration)
    local t = templeSeconds / templeDuration
    local position = temple.transform.position
    position.y = mix(templeFromY, templeToY, t * t * (3 - 2 * t))
    temple.transform.position = position

    -- The camera of the reveal follows the boss rising on the dome, aimed between him and the middle of the
    -- temple (its points are far enough to hold all of it), never at the empty sky where the top will end up
    local focus = find("Domain Reveal Focus")
    if focus then
        local look = focus.transform.position
        local middle = position.y + 12
        look.y = math.max(3, math.min(17, (middle + this.transform.position.y + 1.5) * 0.5))
        focus.transform.position = look
    end

    -- The ground trembles while it moves
    this.world:shakeCamera(0.05)
end

local function setAsh(isFalling)
    local ash = find(AshName)
    if ash and ash.particles then ash.particles.isEmitting = isFalling end
end

local function castSpell(name)
    local spell = find(name)
    return spell ~= nil and spell.spellCaster ~= nil and spell.spellCaster:cast()
end

local function setHealthFloor(fraction)
    this.health.minimum = this.health.max * fraction
end

local function becomeInvulnerable(forSeconds)
    this.health.isInvulnerable = true
    invulnerableLeft = forSeconds
end

local function after(delaySeconds, action)
    timers[#timers + 1] = { left = delaySeconds, action = action }
end

local function updateTimers(deltaTime)
    local i = 1
    while i <= #timers do
        local timer = timers[i]
        timer.left = timer.left - deltaTime
        if timer.left <= 0 then
            table.remove(timers, i)
            timer.action()
        else
            i = i + 1
        end
    end
end

-- The name of the technique being cast, at the bottom of the screen (the player's own first: it is their button)
local function showCaption(text, forSeconds)
    caption, captionLeft = text, forSeconds
end

local function announceTechniques(deltaTime)
    for _, name in ipairs({ "Player Great Orb Spell", "Player Domain Spell", "Player Orb Spell", "Boss Domain Pillars Spell",
                            "Boss Orb Storm Spell", "Boss Cleave Spell", "Boss Fire Pillar Spell", "Boss Orb Volley Spell" }) do
        local spell = find(name)
        if spell and spell.spellCaster and spell.spellCaster.isCasting then
            this.world:prompt(TechniqueNames[name])
            return
        end
    end

    if captionLeft > 0 then
        captionLeft = captionLeft - deltaTime
        this.world:prompt(caption)
    end
end

-- The player's great technique has just left his hands: the world holds its breath
local function watchPlayerGreatSpell()
    local spell = find(PlayerGreatSpell)
    if not spell or not spell.spellCaster then return end

    local casts = spell.spellCaster.timesReleased
    if casts > playerGreatCasts then
        -- The answer to his domain: when the bottle comes down, the domain breaks with it
        if domainIsUp then
            awaitingDetonation = false
            after(2.4, function() if domainIsUp then shatterDomain() end end)
        end
        this.world:notify("BABY OIL DETONATION")
        this.world:slowMotion(0.5, 0.45)
    end
    playerGreatCasts = casts
end

-- True when a great pattern may start now; it then takes its turn (see PatternGapSeconds). While the summon fights
-- for him the domain holds its fire: the monster and his storms of money are enough
local function takePatternTurn()
    if patternLeft > 0 or phase == 3 then return false end

    patternLeft = (phase >= 4) and LastStandGapSeconds or PatternGapSeconds
    return true
end

local function clampToArena(point)
    return vec3(math.max(ArenaMin.x, math.min(ArenaMax.x, point.x)), point.y, math.max(ArenaMin.z, math.min(ArenaMax.z, point.z)))
end

local function isFreeToAct()
    local state = this.enemy.state
    return not isChanneling and not this.characterActions.isBusy and (state == "Chase" or state == "Recover")
end

-- ========== His own moves ==========

-- Gone in a puff of red, back in another one
local function blinkTo(place)
    local from = this.transform.position
    this.world:burst(BlinkEffect, from + vec3(0, 1.0, 0), 90)

    this.transform.position = clampToArena(place)
    this.world:burst(BlinkEffect, this.transform.position + vec3(0, 1.0, 0), 90)
    Audio.playAt(BlinkSound, this.transform.position, 1.0)
end

local function blinkBehindPlayer()
    local player = find(PlayerName)
    if not player then return end

    local behind = player.transform.position - player.transform.forward * BlinkBehindPlayer
    behind.y = this.transform.position.y
    blinkTo(behind)

    this.enemy:startAttackCooldown(BlinkStrikeDelay)
    blinkIn = (phase == 4) and BlinkEverySeconds * 0.6 or BlinkEverySeconds
    farSeconds = 0
end

local function updateBlink(deltaTime)
    blinkIn = math.max(blinkIn - deltaTime, 0)

    local isFar = this.enemy.distanceToTarget > BlinkFromDistance
    farSeconds = isFar and farSeconds + deltaTime or 0

    -- In his last stand he doesn't wait for the player to run: he keeps appearing behind them
    local wantsTo = farSeconds >= BlinkAfterSeconds or (phase == 4 and this.enemy.distanceToTarget > 4)
        or (phase == 6 and this.enemy.distanceToTarget > 9)
    if wantsTo and blinkIn <= 0 and isFreeToAct() then blinkBehindPlayer() end
end

local function throwRing(turnedByDegrees)
    local from = this.transform.position + vec3(0, RingHeight, 0)
    for i = 1, RingOrbs do
        local angle = math.rad(turnedByDegrees + 360 * i / RingOrbs)
        this.world:launch(RingOrb, this, from, vec3(math.cos(angle), 0, math.sin(angle)))
    end
    this.world:burst("Flash Burst", from, 24)
    this.world:shakeCamera(0.35)
    showCaption(RingTechnique, 1.6)
    Audio.playAt(RingSound, from, 1.0)
end

local function updateRing(deltaTime)
    ringIn = ringIn - deltaTime
    -- (not in his armor: the white flashes of the orbs blinded more than they threatened next to his sword)
    if ringIn > 0 or phase < 2 or phase >= 5 or not isFreeToAct() or this.enemy.distanceToTarget > 9 or not takePatternTurn() then return end

    ringIn = (phase == 4) and RingEverySeconds * 0.65 or RingEverySeconds
    if this.characterActions:play("Cast_Pillars") then
        after(0.75, function() throwRing(0) end)
        if phase == 4 then after(1.25, function() throwRing(360 / RingOrbs * 0.5) end) end
    end
end

-- The jet crosses the arena over 'target', flying in 'direction' (it only announces the fire: it is far above everyone)
local function flyJetOver(target, direction)
    direction.y = 0
    direction = direction:normalized()
    local from = vec3(target.x, JetHeight, target.z) - direction * JetRunUp
    this.world:launch(Jet, this, from, direction)
    Audio.playAt(JetSound, vec3(target.x, JetHeight, target.z), 1.3)
end

-- His air strikes (the casts of the scene): the jet comes from behind him the moment the fire is called
local function watchJetSpells()
    local player = find(PlayerName)
    for _, name in ipairs(JetSpells) do
        local spell = find(name)
        if spell and spell.spellCaster then
            local casts = spell.spellCaster.timesReleased
            if casts > (jetCasts[name] or 0) and player then
                flyJetOver(player.transform.position, player.transform.position - this.transform.position)
            end
            jetCasts[name] = casts
        end
    end
end

-- One wave of bundles from above the far wall, each at a spot near the player
local function throwRainWave()
    local player = find(PlayerName)
    if not player then return end

    for i = 1, RainPerWave do
        local from = RainFrom + vec3((i - (RainPerWave + 1) * 0.5) * RainWidth / RainPerWave, math.random() * 3, 0)
        local at = player.transform.position
            + vec3((math.random() - 0.5) * 2 * RainScatter, 1.0, (math.random() - 0.5) * 2 * RainScatter)
        this.world:launch(RainBundle, this, from, at - from)
    end
end

local function startRain()
    showCaption(RainTechnique, 2.5)
    for wave = 0, RainWaves - 1 do
        after(wave * RainWaveSeconds, throwRainWave)
    end
    rainIn = RainEverySeconds
end

local function updateRain(deltaTime)
    if not domainIsUp then return end

    rainIn = rainIn - deltaTime
    if rainIn <= 0 and takePatternTurn() then startRain() end
end

-- One row of fires through 'center', along X or along Z
local function drawLine(center, alongX)
    for i = 1, LineFires do
        local offset = (i - (LineFires + 1) * 0.5) * LineSpacing
        local place = alongX and vec3(center.x + offset, 0.1, center.z) or vec3(center.x, 0.1, center.z + offset)
        this.world:launch(LineFire, this, clampToArena(place), vec3(0, 1, 0))
    end
end

-- Three rows through where the player stands, lit one after another (the first under their feet: move!)
local function drawLines()
    local player = find(PlayerName)
    if not player then return end

    showCaption(LineTechnique, 2.0)

    local center = player.transform.position
    local alongX = linesAlongX
    linesAlongX = not linesAlongX

    flyJetOver(center, alongX and vec3(1, 0, 0) or vec3(0, 0, 1))

    for row = -1, 1 do
        local rowCenter = alongX and vec3(center.x, 0, center.z + row * LineGap) or vec3(center.x + row * LineGap, 0, center.z)
        after((row + 1) * LineWaveSeconds, function() drawLine(rowCenter, alongX) end)
    end
end

local function updateLines(deltaTime)
    if not domainIsUp then return end

    lineIn = lineIn - deltaTime
    if lineIn <= 0 and takePatternTurn() then
        lineIn = (phase == 4) and LineEverySeconds * 0.7 or LineEverySeconds
        drawLines()
    end
end

-- The player throws a punch close to him: now and then he takes it on his forearms instead
local function updateGuard()
    local player = find(PlayerName)
    local isAttacking = player ~= nil and player.characterActions ~= nil and player.characterActions.isAttacking
    local hasJustStarted = isAttacking and not wasPlayerAttacking
    wasPlayerAttacking = isAttacking

    if hasJustStarted and isFreeToAct() and this.enemy.distanceToTarget <= GuardDistance and math.random() < GuardChance then
        this.enemy:changeState("Guard")
    end
end

-- Nothing hurts him while he roars, then three rings of orbs, each turned a little further
local function lastGamble()
    becomeInvulnerable(2.6)
    this.characterActions:play("Cast_Domain")
    this.world:notify("I AM THE STRONGEST IN HISTORY")
    this.world:slowMotion(0.8, 0.4)
    this.world:shakeCamera(0.8)
    Audio.playAt(RoarSound, this.transform.position, 1.4)

    local step = 360 / RingOrbs / 3
    after(0.9, function() throwRing(0) end)
    after(1.5, function() throwRing(step) end)
    after(2.1, function() throwRing(step * 2) end)
end

-- Once, close to the end: he steps into the middle of the arena and lets everything go
local function updateGamble()
    if hasGambled or phase ~= 4 or this.health.fraction > GambleHealthFraction or not isFreeToAct() then return end

    hasGambled = true
    blinkTo(ArenaCenter)
    lastGamble()
end

-- ========== Staged moments ==========

local function playCutscene(name)
    local entity = find(name)
    if entity and entity.cutscene then entity.cutscene:play() end
end

-- The steps run one after another; a step that starts a cutscene holds the next one until it is over
local function startShow(steps)
    show = { steps = steps, at = 1 }
end

local function updateShow(deltaTime)
    if not show then return end

    -- A step may ask for time (it returns seconds): the next one waits, and 'show.during' runs every frame meanwhile
    if show.waitLeft and show.waitLeft > 0 then
        show.waitLeft = show.waitLeft - deltaTime
        if show.during then show.during(deltaTime) end
        return
    end
    show.during = nil

    local step = show.steps[show.at]
    if not step then
        show = nil
        return
    end

    show.at = show.at + 1
    show.waitLeft = step()
end

local function setEyes(intensity)
    for _, name in ipairs(BossEyes) do
        local eye = find(name)
        if eye and eye.glowSprite then
            eye.glowSprite.isVisible = intensity > 0.01
            eye.glowSprite.intensity = intensity
        end
    end
end

local function updateCameraDistance(deltaTime)
    local camera = find(CameraName)
    if not camera or not camera.followCamera then return end

    local wanted = (phase == 3) and GiantCameraDistance or FistFightCameraDistance
    local now = camera.followCamera.distance
    local step = CameraZoomPerSecond * deltaTime
    camera.followCamera.distance = now + math.max(-step, math.min(step, wanted - now))
end

-- A black shape with white eyes, fading back into a man
local function updateSilhouette(deltaTime)
    if silhouetteLeft <= 0 then return end

    silhouetteLeft = math.max(silhouetteLeft - deltaTime, 0)
    local darkness = math.min(silhouetteLeft / SilhouetteFadeSeconds, 1)
    if this.model then this.model.darkness = darkness end
    setEyes(10 * darkness)
end

-- ========== The opening of the domain, on the film's beat ==========
--
-- Every step at the second of the soundtrack the film shows it (game music time = film time - 36.2):
--   1:20.8 the sign of two fingers, close  ·  1:22.5 the sky catches fire  ·  1:24.2 the temple comes up out of the
--   ground under his feet and lifts him, he holds the sign up there  ·  1:30.6 the sky fills with what is about to
--   fall  ·  1:34.65 his smile, the white eyes, the black shape (the film's own pictures)  ·  1:37.05 white, then the
--   rain comes straight down, glowing  ·  the player is thrown to the ground, lies there, gets up  ·  then it is the
--   player's turn: Imaginary Technique: Baby Oil Detonation

-- (one table: Lua allows a chunk 200 local names, the fight has nearly that many)
local O = {
    SkyCutscene    = "Domain Sky",
    MeteorCutscene = "Meteor Fall",
    -- The film's six-pointed stars (one Swarm of thousands, Tools/build_scene.py), the hill of fire the temple comes
    -- out of, and what the boss's flight throws up
    StarSwarm      = "Domain Star Swarm",
    Sun            = "Domain Sun",
    SunSmoke       = "Domain Sun Smoke",
    SkidDust       = "Skid Dust",
    CrashRubble    = "Crash Rubble",
    StarFlight     = "Star Flight",
    StarFlightFocus = "Star Flight Focus",
    -- How big the fire grows (meters) and how many tongues of it a second
    SunRadius      = 7,
    SunRate        = 22000,
    -- The player's last blow before the domain (film 1:14.8 = this second of the soundtrack): he is hurled back
    -- this far through the air, skids on, gets up with the sign
    KnockCutscene  = "Knockback Flight",
    KnockMusicAt   = 38.6,
    KnockMeters    = 20,
    KnockSkidMeters = 5,
    KnockFlySeconds = 1.4,
    KnockSkidSeconds = 1.0,
    KnockLieSeconds = 0.6,
    KnockGetUpSeconds = 2.1,
    -- If the player never hurts him enough, the domain still opens (seconds of the soundtrack)
    DomainLatestAt = 150,
    DetonationPrompt = "2: Imaginary Technique: Baby Oil Detonation",
    DetonationHintSeconds = 10,
    KnockdownDamage = 25,
    -- Where the temple stands as a backdrop once the opening is over (behind the far wall)
    TempleHome = vec3(0, 0, -122),
    TempleBackdropY = -1.8,
    -- How far from the player the temple comes up (it must not rise around them; nor far beyond where he landed)
    TempleNearest = 15,
    TempleFarthest = 28,
    opening = nil, -- the steps still to come: { list, next, offset }
    tweens = {},   -- numbers sliding from one value to another (the swarm's stages, the fire's size)
    knock = nil,   -- the boss's flight while it lasts
    wantsKnockback = false,
    domeOffset = OnTheDome,
    holdSignIn = 0,
    isHoldingSign = false,
    detonationHintLeft = 0,
}

-- The soundtrack's own clock: hit stops and cutscenes don't make it drift from the film
local function clock()
    local music = Audio.musicSeconds()
    if music and music >= 0 then return music end
    return seconds
end

function O.yawToward(from, to)
    local d = to - from
    return math.deg(math.atan(d.x, d.z))
end

-- A point given around something turned by 'yaw' degrees (its +Z looking that way)
function O.turned(v, yaw)
    local r = math.rad(yaw)
    local c, s = math.cos(r), math.sin(r)
    return vec3(v.x * c + v.z * s, v.y, -v.x * s + v.z * c)
end

function O.playerEntity() return find(PlayerName) end

function O.playPlayer(animation)
    local player = O.playerEntity()
    if player and player.characterActions then
        player.characterActions:play(animation)
    end
end

-- The steps run when the soundtrack reaches them (the times are the nominal ones; 'offset' moves them all when the
-- O.opening started a moment late)
function O.startOpening(steps)
    O.opening = { list = steps, next = 1, offset = clock() - steps[1][1] }
end

local function updateOpening()
    if not O.opening then return end

    local now = clock() - O.opening.offset
    while O.opening.list[O.opening.next] and now >= O.opening.list[O.opening.next][1] do
        local step = O.opening.list[O.opening.next]
        O.opening.next = O.opening.next + 1
        step[2]()
    end
    if not O.opening.list[O.opening.next] then O.opening = nil end
end

-- He keeps the sign up while the temple carries him (the held clip again and again: it starts with the hand up)
function O.updateSign(deltaTime)
    if not O.isHoldingSign then return end

    O.holdSignIn = O.holdSignIn - deltaTime
    if O.holdSignIn <= 0 then
        this.characterActions:play("Sign_Hold")
        O.holdSignIn = 2.5
    end
end

-- A number slides from 'from' to 'to' in 'seconds' (smoothly), handed to 'set' every frame
function O.tween(set, from, to, seconds)
    set(from)
    O.tweens[#O.tweens + 1] = { set = set, from = from, to = to, seconds = math.max(seconds, 0.001), t = 0 }
end

local function updateTweens(deltaTime)
    for i = #O.tweens, 1, -1 do
        local w = O.tweens[i]
        w.t = math.min(w.t + deltaTime, w.seconds)
        local k = w.t / w.seconds
        w.set(w.from + (w.to - w.from) * (k * k * (3 - 2 * k)))
        if w.t >= w.seconds then table.remove(O.tweens, i) end
    end
end

function O.stars()
    local entity = find(O.StarSwarm)
    return entity and entity.swarm, entity
end

-- The hill of fire swells out of the ground where the temple is about to rise (film 1:22.5-1:24.5), hides it
-- coming up, then sinks into it
function O.startSun(place)
    local sun, smoke = find(O.Sun), find(O.SunSmoke)
    if not sun or not sun.particles then return end

    sun.transform.position = vec3(place.x, -1, place.z)
    sun.particles.particlesPerSecond = O.SunRate
    sun.particles.isEmitting = true
    -- As many tongues of fire as its surface holds (a small dome with all of them would burn white)
    O.tween(function(v)
        sun.particles.spawnRadius = v
        sun.particles.particlesPerSecond = O.SunRate * (v / O.SunRadius) ^ 2
    end, 3, O.SunRadius, 1.3)
    if smoke and smoke.particles then
        smoke.transform.position = vec3(place.x, 2, place.z)
        smoke.particles.isEmitting = true
    end

    -- (it burns out early: the temple is what the shot is about)
    after(1.4, function()
        O.tween(function(v) sun.particles.particlesPerSecond = v end, O.SunRate, 0, 1.0)
        after(1.7, function() sun.particles.isEmitting = false end)
    end)
    after(4.5, function() if smoke and smoke.particles then smoke.particles.isEmitting = false end end)
end

function O.stopSun()
    for _, name in ipairs({ O.Sun, O.SunSmoke }) do
        local e = find(name)
        if e and e.particles then e.particles.isEmitting = false end
    end
end

-- The stars burst out of the dome under his feet into a turning sphere (film 1:28.5-1:30.5)
function O.burstStars()
    local stars, entity = O.stars()
    if not stars then return end

    entity.transform.position = this.transform.position + vec3(0, 1.5, 0)
    stars:reset()
    stars.isVisible = true
    O.tween(function(v) stars.gathered = v end, 0, 1, 2.2)
    O.tween(function(v) stars.radius = v end, 3, 36, 4.2)
end

-- The camera flies through them, while they spread over the whole sky above the player (film 1:30.75-1:32.75)
function O.flyThroughStars()
    local stars, entity = O.stars()
    local player = O.playerEntity()
    if not stars or not player then return end

    local center = entity.transform.position
    local flight = find(O.StarFlight)
    if flight then
        flight.transform.position = vec3(center.x, 0, center.z)
        flight.transform.rotation = vec3(0, O.yawToward(center, player.transform.position), 0)
    end
    local focus = find(O.StarFlightFocus)
    if focus then focus.transform.position = center end
    playCutscene(O.StarFlight)

    local over = player.transform.position
    stars.skyCenter = vec3((over.x + center.x) * 0.5, 36, (over.z + center.z) * 0.5)
    after(1.0, function() O.tween(function(v) stars.spread = v end, 0, 1, 3.4) end)
end

-- ========== The player's last blow before the domain: he is hurled back (film 1:15-1:21) ==========

function O.startKnockback()
    local player = O.playerEntity()
    if not player then return end

    local from = this.transform.position
    local away = from - player.transform.position
    away.y = 0
    away = away:length() > 0.1 and away:normalized() or vec3(0, 0, -1)

    -- He faces the player and flies backwards
    this.transform.rotation = vec3(0, O.yawToward(from, player.transform.position) + 180, 0)
    local land = clampToArena(from + away * O.KnockMeters)
    land.y = 0
    local skid = clampToArena(land + away * O.KnockSkidMeters)
    skid.y = 0

    this.characterActions:play("Knocked_Down")
    this.world:flash(vec3(1, 0.95, 0.85), 0.25, 0.05)
    this.world:effect("Punch Impact", from + vec3(0, 1.3, 0), -away)
    this.world:effect("Player Heavy Swing", from + vec3(0, 1.3, 0), -away)
    this.world:impactFrame(1.0, 0.5)
    this.world:shakeCamera(1.0)
    this.world:slowMotion(0.4, 0.3)
    Audio.play(BoomSound, 1.0)
    playCutscene(O.KnockCutscene)

    O.knock = { t = 0, from = from, land = land, skid = skid, hasCrashed = false, hasLanded = false,
                hasRisen = false, dustIn = 0 }
end

-- What he smashes through on his way: a burst of rubble and dust (the ruins of the film)
function O.crash(place)
    local rubble = find(O.CrashRubble)
    if rubble and rubble.swarm then
        rubble.transform.position = place
        rubble.swarm:reset()
        rubble.swarm.isVisible = true
        O.tween(function(v) rubble.swarm.gathered = v end, 0, 1, 0.35)
        after(0.4, function() rubble.swarm:fall() end)
    end
    this.world:burst(O.SkidDust, place, 18)
    this.world:effect("Crash Impact", place, this.transform.forward)
    this.world:impactFrame(0.7, 0.35)
    this.world:shakeCamera(0.8)
    Audio.play(BoomSound, 0.8)
end

function O.updateKnock(deltaTime)
    local k = O.knock
    if not k then return end

    k.t = k.t + deltaTime
    local fly, skid = O.KnockFlySeconds, O.KnockSkidSeconds

    if k.t < fly then
        local f = k.t / fly
        local place = mix(k.from, k.land, 1 - (1 - f) * (1 - f))
        -- Low and flat, as if thrown by a car
        place.y = math.sin(math.pi * f) * 1.4
        this.transform.position = place
        if not k.hasCrashed and f >= 0.55 then
            k.hasCrashed = true
            O.crash(place + vec3(0, 1.2, 0))
        end
        return
    end

    if k.t < fly + skid then
        if not k.hasLanded then
            k.hasLanded = true
            this.characterActions:play("Lying")
            this.world:shakeCamera(0.7)
            this.world:burst(O.SkidDust, k.land, 20)
        end
        local f = (k.t - fly) / skid
        this.transform.position = mix(k.land, k.skid, 1 - (1 - f) * (1 - f))
        k.dustIn = k.dustIn - deltaTime
        if k.dustIn <= 0 then
            k.dustIn = 0.1
            this.world:burst(O.SkidDust, this.transform.position, 2)
        end
        return
    end

    this.transform.position = k.skid
    if not k.hasRisen and k.t >= fly + skid + O.KnockLieSeconds then
        k.hasRisen = true
        this.characterActions:play("Get_Up")
        this.world:burst(O.SkidDust, k.skid, 12)
    end
    -- Rising, the hand already goes up with the sign
    if k.hasRisen and k.t >= fly + skid + O.KnockLieSeconds + O.KnockGetUpSeconds then
        O.isHoldingSign, O.holdSignIn = true, 0
        O.knock = nil
    end
end

function O.placeTempleUnderBoss()
    local temple = find(TempleName)
    local player = O.playerEntity()
    if not temple or not player then return end

    local away = this.transform.position - player.transform.position
    away.y = 0
    if away:length() < 0.1 then away = vec3(0, 0, -1) end
    local distance = math.max(O.TempleNearest, math.min(away:length(), O.TempleFarthest))
    local feet = clampToArena(player.transform.position + away:normalized() * distance)
    feet.y = 0
    local yaw = O.yawToward(feet, player.transform.position)
    temple.transform.position = vec3(feet.x, TempleSunkenY, feet.z)
    temple.transform.rotation = vec3(0, yaw, 0)
    O.domeOffset = O.turned(OnTheDome, yaw)
    -- He stands where the dome will come up (his front is his own -Z: turned to the player)
    this.transform.position = vec3(feet.x + O.domeOffset.x, feet.y, feet.z + O.domeOffset.z)
    this.transform.rotation = vec3(0, yaw + 180, 0)

    -- The camera of the reveal goes around the temple, wherever it comes up
    local reveal = find(RevealCutscene)
    if reveal then
        reveal.transform.position = vec3(feet.x, 0, feet.z)
        reveal.transform.rotation = vec3(0, yaw, 0)
    end
    local focus = find("Domain Reveal Focus")
    if focus then focus.transform.position = vec3(feet.x, 13, feet.z) end
end

function O.domainOpeningSteps()
    return {
        -- The last blow: he is hurled back, skids, gets up (the soundtrack was moved here: see startDomainPhase)
        { O.KnockMusicAt, function()
            becomeInvulnerable(45)
            O.startKnockback()
        end },
        { 44.6, function()
            O.knock = nil
            O.isHoldingSign = false
            playCutscene(SignCutscene)
        end },
        -- "Domain expansion: Epstein Island Temple."
        { 45.05, function() Audio.play(O.VoiceDomain, O.VoiceVolume) end },
        { 46.3, function()
            changeLook(Domain, 1.8)
            setAsh(true)
            this.world:shakeCamera(0.5)
            Audio.playAt(RoarSound, this.transform.position, 1.0)
        end },
        { 48.0, function()
            O.placeTempleUnderBoss()
            isOnTheDome = true
            O.isHoldingSign, O.holdSignIn = true, 0
            O.startSun(find(TempleName).transform.position)
            moveTemple(TempleRisenY, 5.4)
            Audio.play(RumbleSound, 1.0)
            playCutscene(RevealCutscene)
        end },
        -- Out of the dome under his feet the stars burst into a turning sphere
        { 52.3, function() O.burstStars() end },
        { 54.4, function() O.flyThroughStars() end },
        { 58.45, function() playCutscene(SmileCutscene) end },
        { 60.85, function()
            local stars = O.stars()
            if stars then stars:fall() end
            -- a blink of light as they fall, not a white screen
            this.world:flash(vec3(1, 0.85, 0.6), 0.5, 0.05)
            this.world:impactFrame(0.8, 0.5)
            Audio.play(BoomSound, 1.0)
            if this.model then this.model.darkness = 1 end
            setEyes(10)
            playCutscene(O.MeteorCutscene)
        end },
        -- The first of them come down around him: he is thrown off his feet
        { 62.5, function()
            local player = O.playerEntity()
            O.playPlayer("Knocked_Down")
            if player and player.health and player.health.current > O.KnockdownDamage + 1 then
                player.health:damage(O.KnockdownDamage)
            end
            this.world:shakeCamera(1.0)
            Audio.play(BoomSound, 1.0)
        end },
        { 63.7, function() O.playPlayer("Lying") end },
        { 65.2, function() this.world:shakeCamera(0.6); Audio.play(BoomSound, 0.8) end },
        { 66.6, function() O.playPlayer("Lying") end },
        -- (he stays down: the player gets him up with [2], see O.updateRise)
        -- The temple sinks again, he is back on the ground before the player, still black with white eyes
        { 68.4, function()
            isOnTheDome = false
            O.isHoldingSign = false
            local temple = find(TempleName)
            local player = O.playerEntity()
            if temple and player then
                local base = temple.transform.position
                local toPlayer = player.transform.position - base
                toPlayer.y = 0
                local ahead = toPlayer:normalized() * math.min(toPlayer:length() * 0.5, 9)
                this.transform.position = clampToArena(vec3(base.x + ahead.x, 0, base.z + ahead.z))
            end
            silhouetteLeft = SilhouetteSeconds
            moveTemple(TempleSunkenY, 4.5)
        end },
        -- The player's turn (film 1:45.6: up in time for the sign at 1:47.5 when [2] comes at once)
        { 69.4, function()
            awaitingDetonation = true
            O.detonationHintLeft = O.DetonationHintSeconds
            O.lyingUntilPress, O.lyingReplayIn = true, 0
            playCutscene(O.DownCutscene)
            invulnerableLeft = 0.5
            this.enemy:startAttackCooldown(2.5)
            rainIn = RainEverySeconds
        end },
        -- Sunk out of sight: the temple goes back behind the far wall and stands there for the rest of the domain
        { 74.0, function()
            local stars = O.stars()
            if stars then stars:reset(); stars.isVisible = false end
            local temple = find(TempleName)
            if temple then
                temple.transform.position = vec3(O.TempleHome.x, TempleSunkenY, O.TempleHome.z)
                temple.transform.rotation = vec3(0, 0, 0)
                O.domeOffset = OnTheDome
            end
            moveTemple(O.TempleBackdropY, 7)
        end },
    }
end

-- ========== Down on his back until the player answers (film 1:44-1:49) ==========

-- A moment of the cutscene the player answers: the camera over him lying where the stars threw him, the prompt on
-- the screen ('Down Wait' waits for [2]); the answer: he rises with his hand up ('Technique Rise') and the great
-- technique leaves his hand as he stands (its voice, eyes and the bottle from the sky: player_techniques.lua)
O.DownCutscene, O.RiseCutscene, O.RiseSeconds = "Down Wait", "Technique Rise", 1.35

function O.updateRise(deltaTime)
    if not O.lyingUntilPress then return end

    -- Nobody strikes a man who is down
    this.enemy:startAttackCooldown(1)
    rainIn, lineIn = math.max(rainIn, 2), math.max(lineIn, 2)

    O.lyingReplayIn = O.lyingReplayIn - deltaTime
    if O.lyingReplayIn <= 0 then
        O.lyingReplayIn = 1.5
        O.playPlayer("Lying")
    end

    local down = find(O.DownCutscene)
    if not down or not down.cutscene or down.cutscene.isPlaying or not down.cutscene.wasAnswered then return end

    O.lyingUntilPress = false
    O.playPlayer("Get_Up")
    playCutscene(O.RiseCutscene)
    this.enemy:startAttackCooldown(O.RiseSeconds + 2)
    after(O.RiseSeconds, function()
        local spell = find(PlayerGreatSpell)
        if spell and spell.spellCaster then spell.spellCaster:cast() end
    end)
end

-- The prompt for the player's answer (the great technique's slot of the HUD shows when it is ready)
local function promptDetonation(deltaTime)
    if not awaitingDetonation then return false end

    O.detonationHintLeft = math.max(O.detonationHintLeft - deltaTime, 0)
    if not domainIsUp then awaitingDetonation = false end
    if O.detonationHintLeft > 0 and captionLeft <= 0 then
        this.world:prompt(O.DetonationPrompt)
        return true
    end
    return false
end

-- While the temple rises he stands on its dome (on the ground until the dome reaches his feet), and the swarm pours
-- out from under his feet
local function rideTheDome(deltaTime)
    if not isOnTheDome then return end

    local temple = find(TempleName)
    if not temple then return end

    local place = temple.transform.position + O.domeOffset
    place.y = math.max(place.y, 0)
    this.transform.position = place
end

-- ========== The domain ==========

local function raiseDomain(isFirstTime)
    domainIsUp = true
    domainBrokenLeft = 0
    domainSpellIn = 2.5
    domainHintLeft = 9
    rainIn = isFirstTime and RainEverySeconds or 2.0
    lineIn = isFirstTime and 9 or 5

    changeLook(Domain, 2.5)
    setAsh(true)
    Audio.playAt(RoarSound, this.transform.position, 1.2)
    -- A rumble, not a quake: the fight goes on right away and the player must see it
    this.world:shakeCamera(0.2)

    if isFirstTime then
        O.startOpening(O.domainOpeningSteps())
    else
        this.world:notify("THE DOMAIN RETURNS")
    end
end

shatterDomain = function()
    domainIsUp = false
    domainBrokenLeft = DomainBrokenSeconds
    flash = 0.25

    changeLook(Shattered, 1.5)
    setAsh(false)
    Audio.play(BoomSound, 1.0)
    this.world:notify("DOMAIN SHATTERED")
    this.world:slowMotion(1.0, 0.3)
    -- A white sphere and a wave across the whole arena, the picture kicked (scene_effects.py)
    this.world:effect("Domain Shatter", this.transform.position + vec3(0, 1, 0))
    this.world:impactFrame(1.0, 0.6)
end

-- The player's own domain: its spell has just left
local function watchPlayerDomain()
    local spell = find(PlayerDomainSpell)
    if not spell or not spell.spellCaster then return end

    local casts = spell.spellCaster.timesReleased
    local hasJustCast = casts > playerDomainCasts
    playerDomainCasts = casts

    if not hasJustCast then return end

    -- (his domain breaks when the player's waves of oil meet: player_techniques.lua, PLAYER_DOMAIN_CRASH_AT)
    if domainIsUp then
        after(O.PlayerDomainCrashAt, function() if domainIsUp then shatterDomain() end end)
    end
    this.world:notify("DOMAIN EXPANSION: DIVINE BABY OIL EVOLVE")
end

-- A reminder for a while after his domain opened (the spell's own slot of the HUD shows when it is ready)
local function promptPlayerDomain(deltaTime)
    domainHintLeft = math.max(domainHintLeft - deltaTime, 0)

    local spell = find(PlayerDomainSpell)
    if domainIsUp and domainHintLeft > 0 and captionLeft <= 0 and spell and spell.spellCaster and spell.spellCaster.isReady then
        this.world:prompt("3: shatter his domain with your own")
    end
end

local function updateDomain(deltaTime)
    if not domainIsUp then
        if domainBrokenLeft > 0 then
            domainBrokenLeft = domainBrokenLeft - deltaTime
            if domainBrokenLeft <= 0 and phase >= 2 and not this.health.isDead then raiseDomain(false) end
        end
        return
    end

    -- Until the last phase the domain feeds him
    if phase == 2 then this.health:heal(DomainHealPerSecond * deltaTime) end

    domainSpellIn = domainSpellIn - deltaTime
    if domainSpellIn <= 0 and not isChanneling and patternLeft <= 0 and phase ~= 3 and castSpell(DomainPillars) then
        patternLeft = (phase == 4) and LastStandGapSeconds or PatternGapSeconds
        domainSpellIn = (phase == 4) and DomainSpellEverySeconds * 0.7 or DomainSpellEverySeconds
    end
end

-- ========== The seal behind his back, and his ascension (the start of the last phase) ==========

-- From the summoning on a hexagonal seal burns behind his back (the way the wheel of the film's summoned monster turns
-- behind it): it comes out of a red flash, turns slowly, smokes, and a red aura burns around him. In his last stand it
-- flares up: he rises on its power, hangs over the player and drops onto them (roll out of the place the flashes
-- mark), and up close he pounds the ground with both fists. Everything here is data of the scene (the hidden seal, its
-- smoke, the aura) and general engine calls
O.Seal, O.SealParts, O.AscensionAura = "Boss Seal", { "Boss Seal Smoke", "Boss Seal Embers", "Boss Red Aura" }, "Boss Ascension Aura"
O.AscensionTechnique = "Island Seal: Ascension"
O.DiveTechnique, O.PoundTechnique = "Island Seal: Descent", "Black Book: Final Chapter"
O.AscensionScale = 1.12            -- he grows a little with it
O.DiveEverySeconds, O.DiveHeight = 11, 5.0   -- (higher, the locked-on camera loses him)
O.DiveRiseSeconds, O.DiveHoverSeconds, O.DiveFallSeconds = 0.9, 1.0, 0.32
O.SlamRadius, O.SlamDamage = 3.6, 46
O.PoundEverySeconds, O.PoundDistance, O.PoundRadius, O.PoundDamage, O.PoundStrikesAt = 8, 3.2, 3.0, 34, 1.05
-- The seal: its size now and the size it grows to, how fast it turns (degrees a second) and how fast that settles
O.sealSize, O.sealTarget, O.sealSpin, O.sealSpinSettled, O.sealTurn = 0, 0, 0, 35, 0
O.SealScale = 0.62   -- (the model is 2.8 m across with its spikes: a wheel a little wider than his shoulders)

function O.setSealParts(isOn)
    for _, name in ipairs(O.SealParts) do
        local part = find(name)
        if part and part.particles then part.particles.isEmitting = isOn end
    end
end

-- It comes out of a red flash behind him, spinning fast, and settles to a slow turn
function O.showSeal(size)
    local seal = find(O.Seal)
    if not seal or not seal.model then return end
    seal.model.isVisible = true
    O.sealTarget, O.sealSpin = size or 1, 900
    O.setSealParts(true)
    local behind = seal.transform.position
    this.world:flash(vec3(0.7, 0.3, 0.25), 0.12, 0.03)
    this.world:burst("Blink Burst", this.transform.position + vec3(0, 1.7, 0), 140)
    this.world:shakeCamera(0.5)
    Audio.playAt(RumbleSound, this.transform.position, 1.0)
end

function O.hideSeal()
    local seal = find(O.Seal)
    if seal and seal.model then seal.model.isVisible = false; seal.transform.scale = vec3(0.01, 0.01, 0.01) end
    O.sealSize, O.sealTarget, O.sealSpin = 0, 0, 0
    O.setSealParts(false)
end

function O.updateSeal(deltaTime)
    local seal = find(O.Seal)
    if not seal or not seal.model or not seal.model.isVisible then return end
    O.sealSize = O.sealSize + (O.sealTarget - O.sealSize) * math.min(deltaTime * 3.0, 1)
    O.sealSpin = O.sealSpinSettled + (O.sealSpin - O.sealSpinSettled) * math.max(1 - deltaTime * 1.5, 0)
    O.sealTurn = (O.sealTurn + O.sealSpin * deltaTime) % 360
    local s = math.max(O.sealSize * O.SealScale, 0.01)
    seal.transform.scale = vec3(s, s, s)
    seal.transform.rotation = vec3(0, 0, O.sealTurn)
    -- (it breathes: its cracks burn brighter and dimmer)
    seal.model.emissiveStrength = 1.1 + math.sin(seconds * 2.3) * 0.35 + (O.sealSpin - O.sealSpinSettled) / 900
end

function O.setRedEyes(intensity)
    for _, name in ipairs(BossEyes) do
        local eye = find(name)
        if eye and eye.glowSprite then eye.glowSprite.color = vec3(1.0, 0.12, 0.05) end
    end
    setEyes(intensity)
end

-- A ring of hurt around a point: the player inside it (and not rolling) takes the blow
function O.blastAround(center, radius, damage)
    local player = find(PlayerName)
    if not player or radius <= 0 then return end
    local between = player.transform.position - center
    between.y = 0
    if between:length() <= radius then player.health:damage(damage) end
end

function O.groundBlast(place, radius, damage, strength)
    this.world:effect("Crash Impact", place, this.transform.forward)
    this.world:burst("Star Impact", place + vec3(0, 0.3, 0), 70)
    this.world:burst("Skid Dust", place + vec3(0, 0.2, 0), 28)
    this.world:shakeCamera(strength)
    this.world:impactFrame(0.6 * strength, 0.25)
    Audio.playAt(BoomSound, place, 1.0)
    O.blastAround(place, radius, damage)
end

function O.startAscension()
    O.hasAscended = false
    isChanneling = true
    this.enemy:changeState("Channel")
    becomeInvulnerable(8)
    local ground = this.transform.position
    -- The player's camera turns to him (locked on): the moment the jet strikes is not missed
    local player = find(PlayerName)
    if player and player.lockOn then player.lockOn:setTarget(this) end

    startShow({
        -- He powers up, the seal behind him flares and spins
        function()
            this.characterActions:play("Power_Up")
            this.world:notify("THE TREASURE IS SPENT")
            this.world:slowMotion(1.6, 0.5)
            this.world:shakeCamera(0.5)
            O.setRedEyes(3.0)
            O.showSeal(1.35)
            Audio.playAt(RoarSound, ground, 1.3)
            return 1.3
        end,
        -- A blast of red fire out of him, and he rises on the seal's power
        function()
            this.world:flash(vec3(1.0, 0.2, 0.1), 0.8, 0.08)
            O.groundBlast(ground, 0, 0, 1.0)
            local aura = find(O.AscensionAura)
            if aura and aura.particles then aura.particles.isEmitting = true end
            this.transform.scale = vec3(O.AscensionScale, O.AscensionScale, O.AscensionScale)
            this.characterActions:play("Hover")
            local risen = 0
            show.during = function(deltaTime)
                risen = math.min(risen + deltaTime / 1.2, 1)
                this.transform.position = ground + vec3(0, math.sin(risen * math.pi) * 2.2, 0)
            end
            return 1.2
        end,
        function()
            this.transform.position = ground
            this.characterActions:play("Land_Slam")
            O.groundBlast(ground, 0, 0, 0.7)
            showCaption(O.AscensionTechnique, 2.5)
            this.world:notify("PRIVATE JET: ASCENSION")
            O.hasAscended = true
            O.diveIn, O.poundIn = 4.0, 2.5
            isChanneling = false
            return 0.8
        end,
    })
end

-- The nosedive: up, a moment over the player (flashes mark the place, following them), then down onto it
function O.startDive()
    O.dive = { stage = "rise", t = 0, ground = this.transform.position, from = this.transform.position }
    isChanneling = true
    this.enemy:changeState("Channel")
    this.characterActions:play("Hover")
    showCaption(O.DiveTechnique, 2.6)
    O.sealSpin = 700
    Audio.playAt(RumbleSound, this.transform.position, 1.2)
end

function O.updateDive(deltaTime)
    local dive = O.dive
    if not dive then return end
    local player = find(PlayerName)
    dive.t = dive.t + deltaTime

    if dive.stage == "rise" then
        local k = math.min(dive.t / O.DiveRiseSeconds, 1)
        local eased = k * k * (3 - 2 * k)
        this.transform.position = dive.ground + vec3(0, O.DiveHeight * eased, 0)
        if k >= 1 then dive.stage, dive.t = "hover", 0 end
    elseif dive.stage == "hover" then
        this.transform.position = dive.ground + vec3(0, O.DiveHeight + math.sin(dive.t * 6) * 0.15, 0)
        if player then
            dive.target = clampToArena(vec3(player.transform.position.x, dive.ground.y, player.transform.position.z))
            dive.markIn = (dive.markIn or 0) - deltaTime
            if dive.markIn <= 0 then
                this.world:burst("Flash Burst", dive.target + vec3(0, 0.15, 0), 14)
                dive.markIn = 0.12
            end
        end
        if dive.t >= O.DiveHoverSeconds then
            dive.stage, dive.t = "fall", 0
            dive.from = this.transform.position
            this.characterActions:play("Dive")
        end
    elseif dive.stage == "fall" then
        local k = math.min(dive.t / O.DiveFallSeconds, 1)
        local target = dive.target or dive.ground
        this.transform.position = dive.from + (target - dive.from) * (k * k)
        if k >= 1 then
            this.transform.position = target
            this.characterActions:play("Land_Slam")
            O.groundBlast(target, O.SlamRadius, O.SlamDamage, 1.1)
            dive.stage, dive.t = "land", 0
        end
    elseif dive.stage == "land" then
        if dive.t >= 0.9 then
            O.dive = nil
            isChanneling = false
            O.diveIn = O.DiveEverySeconds
        end
    end
end

-- Both fists into the ground when the player stands close: get out of the ring (or roll through it)
function O.updatePound(deltaTime)
    O.poundIn = (O.poundIn or 0) - deltaTime
    if O.poundIn > 0 or this.enemy.distanceToTarget > O.PoundDistance or not isFreeToAct() then return end
    if not this.characterActions:play("Ground_Pound") then return end

    O.poundIn = O.PoundEverySeconds
    showCaption(O.PoundTechnique, 1.6)
    after(O.PoundStrikesAt, function()
        if this.health.isDead then return end
        O.groundBlast(this.transform.position + this.transform.forward * 0.8, O.PoundRadius, O.PoundDamage, 0.8)
    end)
end

function O.updateAscension(deltaTime)
    if phase ~= 4 or not O.hasAscended then return end
    if O.dive then O.updateDive(deltaTime); return end

    O.diveIn = (O.diveIn or 0) - deltaTime
    if O.diveIn <= 0 and isFreeToAct() and takePatternTurn() then
        O.startDive()
        return
    end
    O.updatePound(deltaTime)
end

function O.resetAscension()
    O.hasAscended, O.dive, O.diveIn, O.poundIn = false, nil, 0, 0
    O.hideSeal()
    local aura = find(O.AscensionAura)
    if aura and aura.particles then aura.particles.isEmitting = false end
    this.transform.scale = vec3(1, 1, 1)
    for _, name in ipairs(BossEyes) do
        local eye = find(name)
        if eye and eye.glowSprite then eye.glowSprite.color = vec3(1, 1, 1) end
    end
end

-- ========== His armor: the transformation at 0 health of his last stand, and the two phases with the sword ==========
--
-- The user's script (10.5 s): the last blow throws him back, he stays on his feet, a beat of silence, a smile, his
-- health fills up with a boom; a rift opens over the arena and drops the plates of his armor around him and his sword
-- point-first into the stone; the plates snap onto him in two beats (the model of his armored self is put on in the
-- second), he rips the sword out of the ground, his eyes burn red, a stamp - and he fights with the sword: phase 5,
-- and at half his health phase 6 (the blade catches fire, everything comes faster, the seal behind him joins in)
O.ArmoredModel, O.PlainModel = "sih_boss_armored", "sih_boss"
O.Pieces = { "torso", "greave_l", "greave_r", "bracer_l", "bracer_r" }
-- Where each piece sits on him (PIECE lines of Tools/characters_generator.py, meters from his feet, his front -Z)
O.PieceOffsets = {
    torso = vec3(-0.001, 1.426, 0.005), bracer_l = vec3(-0.551, 1.211, -0.027), bracer_r = vec3(0.554, 1.208, -0.028),
    greave_l = vec3(-0.202, 0.588, 0.017), greave_r = vec3(0.189, 0.584, 0.025),
}
-- Where each lands on the floor (angle from his front, meters from him) and when it falls (seconds of the scene)
O.PieceFloor = { torso = { 0, 2.6 }, greave_l = { -55, 2.3 }, greave_r = { 55, 2.3 }, bracer_l = { -95, 2.0 }, bracer_r = { 95, 2.0 } }
O.PieceFallAt = { torso = 2.45, greave_l = 2.65, greave_r = 2.8, bracer_l = 2.95, bracer_r = 3.1 }
O.FallSeconds, O.FallFrom = 0.45, 13
O.SwordFallAt, O.SwordInGround = 3.5, 1.45
-- The two beats of the snap: the torso and the greaves, then the bracers (and his armored model put on)
O.FirstSnapAt, O.SecondSnapAt = 4.8, 5.7
O.LiftFrom, O.BracersLiftFrom = 4.2, 5.0
O.DrawAt, O.IgniteAt, O.TransformEnds = 7.4, 8.5, 10.5
O.Shots = { { 0.0, "Armor Fakeout" }, { 2.2, "Armor Rift Shot" }, { 4.2, "Armor Snap" }, { 6.4, "Armor Draw" }, { 8.4, "Armor Ignite" } }
O.ArmorTitle = "Jeffrey Epstein, Armored in Sin"
O.Blood = {
    light = vec3(1.0, 0.58, 0.5), ambient = vec3(0.22, 0.12, 0.12), fog = vec3(0.17, 0.05, 0.05),
    fogDensity = 0.011, sky = 0.9, exposure = 0.9, bloom = 0.9
}
O.MetalSound = "@game/Sounds/Combat/Block"
O.SlashSound = "@game/Sounds/Combat/Swing"

-- His sword techniques: how often, from how far, how hard
O.SlamTechnique, O.DashTechnique = "Black Blade: Earthsplitter", "Island Seal: Phantom Rush"
O.WhirlTechnique, O.BarrageTechnique = "Black Blade: Crimson Whirl", "Island Seal: Thousand Blades"
O.ExecutionTechnique = "Final Chapter: Execution"
O.SlamEvery, O.SlamRadius, O.SlamDamage, O.SlamWaves = 9, 3.8, 40, 3
O.DashEvery, O.DashDamage, O.DashMeters, O.DashSeconds = 8, 30, 3.5, 0.22
O.WhirlEvery, O.WhirlRadius, O.WhirlDamage = 7, 3.6, 32
O.BarrageEvery, O.BarrageWaves = 10, 5
O.SwordWave = "Boss Sword Wave"

function O.child(name)
    local entity = this:findChild(name)
    return entity
end

function O.setFists(isOn)
    for _, name in ipairs({ "Right Fist", "Left Fist", "Right Foot" }) do
        local limb = O.child(name)
        if limb and limb.hitbox then limb.hitbox.isEnabled = isOn end
    end
end

function O.setHeldSword(isOn)
    local sword = O.child("Boss Sword")
    if sword then
        if sword.model then sword.model.isVisible = isOn end
        if sword.hitbox then sword.hitbox.isEnabled = isOn end
    end
end

function O.setSwordFire(rate)
    local flames = O.child("Boss Sword Flames")
    if flames and flames.particles then
        flames.particles.isEmitting = rate > 0
        if rate > 0 then flames.particles.particlesPerSecond = rate end
    end
end

function O.pieceEntity(piece) return find("Armor Piece " .. piece) end

-- A point around him: 'local' in his own space (front -Z), turned with him
function O.onHim(offset)
    return this.transform.position + O.turned(offset, this.transform.rotation.y)
end

function O.floorSpot(piece)
    local spot = O.PieceFloor[piece]
    local angle = math.rad(spot[1])
    return O.onHim(vec3(math.sin(angle) * spot[2], 0.25, -math.cos(angle) * spot[2]))
end

function O.hideArmorProps()
    for _, piece in ipairs(O.Pieces) do
        local entity = O.pieceEntity(piece)
        if entity and entity.model then entity.model.isVisible = false end
    end
    local dropped = find("Armor Drop Sword")
    if dropped and dropped.model then dropped.model.isVisible = false end
end

-- Back to the man of the first phases (the player died: the fight starts over)
function O.resetArmor()
    O.transform = nil
    O.hideArmorProps()
    if this.model then this.model.modelName = O.PlainModel end
    O.setHeldSword(false)
    O.setSwordFire(0)
    O.setFists(true)
    local actions = this.characterActions
    actions.swingEffect, actions.heavySwingEffect, actions.swingEffectHeight = "Boss Swing", "Boss Heavy Swing", 1.3
    actions.hyperArmor, actions.attackReach = 1, 1.3
    for _, name in ipairs({ "Attack_Light_1", "Attack_Light_2", "Attack_Light_3" }) do actions:setMove(name, 0.25, 0.72, 1.0) end
    actions:setMove("Attack_Heavy", 0.28, 0.75, 1.8)
    O.armored = nil
end

function O.startTransformation()
    O.transform = { t = 0, shot = 1, fallen = {}, landed = {}, snapped = {} }
    O.hasAscended, O.dive = false, nil
    becomeInvulnerable(30)
    isChanneling = true
    this.enemy:changeState("Channel")
    timers = {}
    domainIsUp = false
    setAsh(false)
    local aura = find(O.AscensionAura)
    if aura and aura.particles then aura.particles.isEmitting = false end

    -- Thrown back along the floor by the last blow, away from the player
    local player = find(PlayerName)
    local away = player and (this.transform.position - player.transform.position) or vec3(0, 0, -1)
    away.y = 0
    away = away:length() > 0.1 and away:normalized() or vec3(0, 0, -1)
    -- (facing the player: his front is his own -Z)
    this.transform.rotation = vec3(0, O.yawToward(this.transform.position, this.transform.position - away) + 180, 0)
    O.transform.slideFrom = this.transform.position
    O.transform.slideTo = clampToArena(this.transform.position + away * 3.5)
    -- (the player is thrown back too, out of the shots: the camera is on him alone now)
    if player then
        local back = clampToArena(this.transform.position - away * 6.0)
        player.transform.position = vec3(back.x, player.transform.position.y, back.z)
    end
    this.characterActions:play("Hit")
    this.world:impactFrame(1.0, 0.45)
    this.world:slowMotion(0.5, 0.25)
    this.world:shakeCamera(0.9)
    Audio.play(BoomSound, 1.0)
    this.world:effect("Punch Impact", this.transform.position + vec3(0, 1.3, 0), away)
end

function O.updatePieces(tr)
    for _, piece in ipairs(O.Pieces) do
        local entity = O.pieceEntity(piece)
        if entity and entity.model then
            local spot = O.floorSpot(piece)
            local yaw = this.transform.rotation.y
            local fallAt = O.PieceFallAt[piece]
            local isBracer = piece == "bracer_l" or piece == "bracer_r"
            local liftFrom = isBracer and O.BracersLiftFrom or O.LiftFrom
            local snapAt = isBracer and O.SecondSnapAt or O.FirstSnapAt
            local onBody = O.onHim(O.PieceOffsets[piece])

            if tr.t >= fallAt and tr.t < liftFrom then
                entity.model.isVisible = true
                -- out of the rift, tumbling, then lying where it fell
                local k = math.min((tr.t - fallAt) / O.FallSeconds, 1)
                local place = spot + vec3(0, O.FallFrom * (1 - k * k), 0)
                entity.transform.position = place
                entity.transform.rotation = vec3(80 * (1 - k) + 12, yaw + 140 * (1 - k) + O.PieceFloor[piece][1], 25 * (1 - k))
                if k >= 1 and not tr.landed[piece] then
                    tr.landed[piece] = true
                    this.world:effect("Armor Land", spot, this.transform.forward)
                    this.world:shakeCamera(0.35)
                    Audio.playAt(O.MetalSound, spot, 1.2)
                end
            elseif tr.t >= liftFrom and tr.t < snapAt then
                -- torn out of the floor, flying onto him on a short sharp curve
                local k = (tr.t - liftFrom) / (snapAt - liftFrom)
                local e = k * k * k
                local place = spot + (onBody - spot) * e + vec3(0, math.sin(k * math.pi) * 0.9, 0)
                entity.transform.position = place
                entity.transform.rotation = vec3(12 * (1 - e), yaw + O.PieceFloor[piece][1] * (1 - e), 0)
            elseif tr.t >= snapAt then
                entity.transform.position = onBody
                entity.transform.rotation = vec3(0, yaw, 0)
                if not tr.snapped[piece] then
                    tr.snapped[piece] = true
                    this.world:effect("Armor Clank", onBody, this.transform.forward)
                end
            end
        end
    end
end

function O.updateTransform(deltaTime)
    local tr = O.transform
    if not tr then return end
    local before = tr.t
    tr.t = tr.t + deltaTime
    local function at(second) return before < second and tr.t >= second end

    -- the shots of the cutscene, one after another
    local shot = O.Shots[tr.shot]
    if shot and tr.t >= shot[1] and not this.world.isCutscenePlaying then
        playCutscene(shot[2])
        tr.shot = tr.shot + 1
    end

    -- 0-2.2: thrown back, skidding on his boots; a beat of silence; the smile; his health fills with a boom
    if tr.t < 0.9 then
        local k = tr.t / 0.9
        this.transform.position = mix(tr.slideFrom, tr.slideTo, 1 - (1 - k) * (1 - k))
        tr.dustIn = (tr.dustIn or 0) - deltaTime
        if tr.dustIn <= 0 then
            tr.dustIn = 0.08
            this.world:burst(O.SkidDust, this.transform.position, 3)
        end
    end
    if at(0.9) then this.characterActions:play("Idle") end
    if at(1.45) then
        this.world:flash(vec3(0.55, 0.12, 0.08), 0.12, 0.02)
        this.world:shakeCamera(0.6)
        Audio.play(BoomSound, 1.1)
        this.world:notify(string.upper(O.ArmorTitle))
        O.tween(function(v)
            local wanted = this.health.max * v
            if this.health.current < wanted then this.health:heal(wanted - this.health.current) end
        end, 0.05, 1, 0.9)
    end

    -- 2.2-4.2: the rift over the arena, the pieces and the sword falling out of it
    if at(2.2) then
        this.characterActions:play("Cast_Pillars")
        this.world:effect("Armor Rift", this.transform.position + vec3(0, 11, 0), this.transform.forward)
        Audio.play(RumbleSound, 1.2)
        Audio.playAt(RoarSound, this.transform.position, 1.0)
        changeLook(O.Blood, 2.0)
    end
    O.updatePieces(tr)

    local dropped = find("Armor Drop Sword")
    if dropped and dropped.model then
        local ground = O.onHim(vec3(0, 0, -O.SwordInGround))
        local stuck = vec3(ground.x, 1.13, ground.z)
        if tr.t >= O.SwordFallAt and tr.t < O.DrawAt then
            dropped.model.isVisible = true
            local k = math.min((tr.t - O.SwordFallAt) / 0.35, 1)
            dropped.transform.position = stuck + vec3(0, 14 * (1 - k * k), 0)
            dropped.transform.rotation = vec3(180, this.transform.rotation.y + 90, 0)
        end
        if at(O.SwordFallAt + 0.35) then
            this.world:effect("Armor Land", vec3(ground.x, 0.1, ground.z), this.transform.forward)
            this.world:burst(O.SkidDust, vec3(ground.x, 0.2, ground.z), 20)
            this.world:shakeCamera(0.7)
            Audio.play(BoomSound, 0.9)
        end
    end

    -- 4.2-6.5: arms spread, the plates snap onto him in two beats
    if at(O.LiftFrom) then this.characterActions:play("Power_Up") end
    if at(O.FirstSnapAt) then
        this.world:shakeCamera(0.55)
        Audio.playAt(O.MetalSound, this.transform.position, 1.4)
    end
    if at(O.SecondSnapAt) then
        -- the bracers click onto his forearms: he is in his armor now
        if this.model then this.model.modelName = O.ArmoredModel end
        O.hideArmorProps()
        if dropped and dropped.model then dropped.model.isVisible = true end
        this.characterActions:play("Power_Up")
        this.world:shakeCamera(0.7)
        this.world:effect("Armor Clank", O.onHim(O.PieceOffsets.bracer_l), this.transform.forward)
        this.world:effect("Armor Clank", O.onHim(O.PieceOffsets.bracer_r), this.transform.forward)
        Audio.playAt(O.MetalSound, this.transform.position, 1.5)
    end

    -- 6.5-8.5: a heavy step to the sword, it is ripped out of the stone
    if at(6.5) then this.characterActions:play("Walk") end
    if tr.t >= 6.5 and tr.t < 7.1 then
        this.transform.position = this.transform.position + this.transform.forward * (deltaTime * 0.9)
    end
    if at(7.1) then this.characterActions:play("Attack_Light_3") end
    if at(O.DrawAt) then
        if dropped and dropped.model then dropped.model.isVisible = false end
        O.setHeldSword(true)
        if O.child("Boss Sword") and O.child("Boss Sword").hitbox then O.child("Boss Sword").hitbox.isEnabled = false end
        local ground = O.onHim(vec3(0, 0.2, -O.SwordInGround * 0.6))
        this.world:burst(O.SkidDust, ground, 24)
        this.world:effect("Boss Sword Swing", this.transform.position + vec3(0, 1.2, 0), this.transform.forward)
        this.world:slowMotion(0.25, 0.08)
        this.world:shakeCamera(0.8)
        Audio.play(O.SlashSound, 1.2)
    end

    -- 8.5-10.5: his eyes burn red, a stamp, a wave of dust - and he fights
    if at(O.IgniteAt) then
        O.setRedEyes(8.0)
        O.sealSpin = 900
        O.sealTarget = 1.2
        O.setSwordFire(28)
        this.world:effect("Armor Stomp", this.transform.position + vec3(0, 0.1, 0), this.transform.forward)
        this.world:shakeCamera(0.8)
        Audio.play(BoomSound, 1.2)
        Audio.playAt(RoarSound, this.transform.position, 1.3)
    end
    if tr.t >= O.TransformEnds then
        O.transform = nil
        O.startArmoredPhase()
    end
end

function O.startArmoredPhase()
    phase = 5
    isChanneling = false
    invulnerableLeft = 0.5
    setHealthFloor(0.5 - FloorMargin)
    O.setFists(false)
    O.setHeldSword(true)
    changeLook(O.Blood, 1.5)

    local actions = this.characterActions
    actions.swingEffect, actions.heavySwingEffect, actions.swingEffectHeight = "Boss Sword Swing", "Boss Sword Heavy Swing", 1.1
    actions.hyperArmor, actions.attackReach = 1, 1.9
    actions.attackSpeed = 1.1
    actions.attackDelayChance = 0.2
    actions:setMove("Attack_Light_1", 0.25, 0.62, 1.0)
    actions:setMove("Attack_Light_2", 0.2, 0.6, 1.0)
    actions:setMove("Attack_Light_3", 0.35, 0.64, 1.1)
    actions:setMove("Attack_Heavy", 0.42, 0.72, 1.6)
    this.meleeEnemy.attackDistance = 2.6
    this.meleeEnemy.attackCooldownSeconds = 1.0
    this.meleeEnemy.heavyAttackChance = 0.35
    this.meleeEnemy.dodgeChance = 0.15
    O.armored = { slamIn = 3, dashIn = 6, whirlIn = 4, barrageIn = 99, hasExecuted = false }
    showCaption(O.ArmorTitle, 3)
end

function O.startPhaseSix()
    phase = 6
    setHealthFloor(0)
    becomeInvulnerable(2.2)
    this.characterActions:play("Sword_Power_Up")
    this.world:notify("THE BLADE OF THE ISLAND BURNS")
    this.world:slowMotion(0.8, 0.4)
    this.world:shakeCamera(0.9)
    this.world:flash(vec3(1.0, 0.45, 0.35), 0.2, 0.05)
    Audio.playAt(RoarSound, this.transform.position, 1.5)
    O.setSwordFire(60)
    O.sealSpin, O.sealTarget = 1200, 1.45
    local aura = find(O.AscensionAura)
    if aura and aura.particles then aura.particles.isEmitting = true end
    this.characterActions.attackSpeed = 1.3
    this.characterActions.attackDelayChance = 0.35
    this.meleeEnemy.attackCooldownSeconds = 0.7
    this.meleeEnemy.heavyAttackChance = 0.45
    O.armored.barrageIn = 4
    O.armored.slamIn = math.min(O.armored.slamIn, 3)
end

-- A wave of the sword thrown along the floor from 'from' in 'direction' (level)
function O.throwWave(from, direction)
    direction.y = 0
    if direction:length() < 0.01 then return end
    this.world:launch(O.SwordWave, this, vec3(from.x, 0.5, from.z), direction:normalized())
end

function O.wavesAtPlayer(from, count, spreadDegrees)
    local player = find(PlayerName)
    if not player then return end
    local aim = player.transform.position - from
    aim.y = 0
    local yaw = math.deg(math.atan(aim.x, aim.z))
    for i = 1, count do
        local angle = math.rad(yaw + (i - (count + 1) * 0.5) * spreadDegrees)
        O.throwWave(from, vec3(math.sin(angle), 0, math.cos(angle)))
    end
end

-- Black Blade: Earthsplitter - a leap onto the player and the sword into the floor: the floor caves in, waves run out
function O.startSlam()
    local player = find(PlayerName)
    if not player then return end
    O.slam = { t = 0, from = this.transform.position, hasHit = false }
    local toPlayer = player.transform.position - this.transform.position
    toPlayer.y = 0
    O.slam.to = clampToArena(player.transform.position - toPlayer:normalized() * 1.6)
    this.transform.rotation = vec3(0, O.yawToward(this.transform.position, player.transform.position) + 180, 0)
    isChanneling = true
    this.enemy:changeState("Channel")
    this.characterActions:play("Sword_Slam")
    showCaption(O.SlamTechnique, 2.2)
    Audio.playAt(O.SlashSound, this.transform.position, 1.2)
end

function O.updateSlam(deltaTime)
    local s = O.slam
    if not s then return false end
    s.t = s.t + deltaTime
    local leap = 0.55
    if s.t < leap then
        local k = s.t / leap
        local place = mix(s.from, s.to, k * k * (3 - 2 * k))
        place.y = math.sin(k * math.pi) * 2.2
        this.transform.position = place
        s.markIn = (s.markIn or 0) - deltaTime
        if s.markIn <= 0 then
            s.markIn = 0.1
            this.world:burst("Sword Hit Burst", s.to + vec3(0, 0.15, 0), 8)
        end
    elseif not s.hasHit then
        s.hasHit = true
        this.transform.position = s.to
        local point = this.transform.position + this.transform.forward * 1.4
        this.world:effect("Sword Ground Burst", vec3(point.x, 0.1, point.z), this.transform.forward)
        this.world:impactFrame(1.0, 0.4)
        this.world:shakeCamera(1.0)
        Audio.playAt(BoomSound, point, 1.3)
        O.blastAround(point, O.SlamRadius, O.SlamDamage)
        O.wavesAtPlayer(point, (phase == 6) and O.SlamWaves + 2 or O.SlamWaves, 22)
    elseif s.t >= leap + 0.9 then
        O.slam = nil
        isChanneling = false
        return false
    end
    return true
end

-- Island Seal: Phantom Rush - a moment's wind-up, then gone through the player in a streak of red, cutting
function O.startDash()
    local player = find(PlayerName)
    if not player then return end
    local toPlayer = player.transform.position - this.transform.position
    toPlayer.y = 0
    local direction = toPlayer:normalized()
    O.dash = { t = 0, from = this.transform.position, to = clampToArena(player.transform.position + direction * O.DashMeters),
               direction = direction, hasHit = false }
    this.transform.rotation = vec3(0, O.yawToward(this.transform.position, player.transform.position) + 180, 0)
    isChanneling = true
    this.enemy:changeState("Channel")
    this.characterActions:play("Sword_Slide")
    showCaption(O.DashTechnique, 1.8)
    O.sealSpin = 1000
    this.world:burst("Blink Burst", this.transform.position + vec3(0, 1.0, 0), 60)
end

function O.updateDash(deltaTime)
    local d = O.dash
    if not d then return false end
    d.t = d.t + deltaTime
    local windup = 0.4
    if d.t >= windup and d.t < windup + O.DashSeconds then
        local k = (d.t - windup) / O.DashSeconds
        this.transform.position = mix(d.from, d.to, k)
        d.streakIn = (d.streakIn or 0) - deltaTime
        if d.streakIn <= 0 then
            d.streakIn = 0.03
            this.world:effect("Dash Streak", this.transform.position + vec3(0, 0.1, 0), d.direction)
        end
        local player = find(PlayerName)
        if player and not d.hasHit then
            local between = player.transform.position - this.transform.position
            between.y = 0
            if between:length() < 1.4 then
                d.hasHit = true
                player.health:damage(O.DashDamage)
                this.world:effect("Boss Sword Heavy Swing", this.transform.position + vec3(0, 1.1, 0), d.direction)
                this.world:impactFrame(0.7, 0.25)
            end
        end
    elseif d.t >= windup + O.DashSeconds and not d.hasEnded then
        d.hasEnded = true
        this.transform.position = d.to
        this.world:effect("Boss Sword Heavy Swing", this.transform.position + vec3(0, 1.1, 0), -d.direction)
        this.world:shakeCamera(0.5)
        Audio.play(O.SlashSound, 1.1)
        if phase == 6 then O.wavesAtPlayer(this.transform.position, 1, 0) end
    elseif d.t >= windup + O.DashSeconds + 0.6 then
        O.dash = nil
        isChanneling = false
        return false
    end
    return true
end

-- Black Blade: Crimson Whirl - the sword all around him, twice in phase 6
function O.startWhirl()
    this.characterActions:play("Sword_Spin")
    showCaption(O.WhirlTechnique, 1.6)
    local count = (phase == 6) and 2 or 1
    for i = 1, count do
        after(0.42 + (i - 1) * 0.5, function()
            if this.health.isDead then return end
            this.world:effect("Sword Whirl", this.transform.position + vec3(0, 1.0, 0), this.transform.forward)
            this.world:shakeCamera(0.5)
            Audio.playAt(O.SlashSound, this.transform.position, 1.2)
            O.blastAround(this.transform.position, O.WhirlRadius, O.WhirlDamage)
        end)
    end
end

-- Island Seal: Thousand Blades - the seal behind him flares and throws a fan of waves at the player
function O.startBarrage()
    showCaption(O.BarrageTechnique, 2.0)
    O.sealSpin = 1400
    local seal = find(O.Seal)
    for volley = 0, 1 do
        after(0.6 + volley * 0.7, function()
            if this.health.isDead then return end
            local from = seal and seal.transform.position or this.transform.position
            O.wavesAtPlayer(from, O.BarrageWaves, 14)
            this.world:burst("Blink Burst", from, 40)
            Audio.playAt(O.SlashSound, from, 1.0)
        end)
    end
end

-- Final Chapter: Execution - once, low on health: the middle of the arena, three leaps and the seal's blades
function O.startExecution()
    O.armored.hasExecuted = true
    blinkTo(ArenaCenter)
    becomeInvulnerable(1.5)
    this.characterActions:play("Sword_Power_Up")
    this.world:notify(string.upper(O.ExecutionTechnique))
    this.world:slowMotion(0.8, 0.4)
    Audio.playAt(RoarSound, this.transform.position, 1.5)
    after(1.6, function() O.startBarrage() end)
    after(3.2, function() if not this.health.isDead and not O.slam then O.startSlam() end end)
end

function O.updateArmored(deltaTime)
    local a = O.armored
    if not a or phase < 5 then return end
    if O.updateSlam(deltaTime) or O.updateDash(deltaTime) then return end

    for key, value in pairs(a) do
        if type(value) == "number" then a[key] = value - deltaTime end
    end
    if not isFreeToAct() then return end

    local distance = this.enemy.distanceToTarget
    if phase == 6 and not a.hasExecuted and this.health.fraction < 0.2 then
        O.startExecution()
    elseif a.slamIn <= 0 and distance > 3.5 and distance < 13 and takePatternTurn() then
        a.slamIn = (phase == 6) and O.SlamEvery * 0.7 or O.SlamEvery
        O.startSlam()
    elseif a.dashIn <= 0 and distance > 5 and distance < 15 and takePatternTurn() then
        a.dashIn = (phase == 6) and O.DashEvery * 0.6 or O.DashEvery
        O.startDash()
    elseif a.whirlIn <= 0 and distance < 3.2 and takePatternTurn() then
        a.whirlIn = (phase == 6) and O.WhirlEvery * 0.7 or O.WhirlEvery
        O.startWhirl()
    elseif phase == 6 and a.barrageIn <= 0 and takePatternTurn() then
        a.barrageIn = O.BarrageEvery
        O.startBarrage()
    end
end

-- ========== The phases ==========

local function startFight()
    phase = 1
    seconds = 0
    hasSeenDefeat = false
    setHealthFloor(DomainHealthFraction - FloorMargin)
    Audio.playMusic(FightMusic, 0.9, 0.2)

    -- The film's O.opening ends with the player's orb in the boss's face
    this.world:burst("Oil Splash", this.transform.position + vec3(0, 1.4, 0), 160)
    this.world:shakeCamera(0.7)
    this.health:damage(OpeningHitDamage)
end

local function startDomainPhase()
    phase = 2
    setHealthFloor(SummonHealthFraction - FloorMargin)
    becomeInvulnerable(PhaseChangeInvulnerableSeconds)
    raiseDomain(true)
end

local callTheSummon

local function startSummonPhase()
    phase = 3
    isChanneling = true
    channelSpellIn = 4
    this.health.isInvulnerable = true
    invulnerableLeft = 0
    this.enemy:changeState("Channel")

    -- As in the film: he hangs in the air on his back for a moment, the wheel turns (the film's own pictures),
    -- then what he called stands there
    local ground = this.transform.position
    local floated = 0

    startShow({
        -- (film 2:31) "He is doing better than I expected."
        function()
            Audio.play(O.VoiceBetter, O.VoiceVolume)
            this.enemy:lookAtTarget(true)
            showCaption("He is doing better than I expected.", 2.6)
            return 2.9
        end,
        -- (film 2:34) "Well, with this treasure I summon..." - and he rises on his back
        function()
            Audio.play(O.VoiceTreasure, O.VoiceVolume)
            showCaption("Well, with this treasure I summon...", 2.5)
            this.characterActions:play("Death")
            this.world:slowMotion(FloatSeconds, 0.6)
            show.during = function(deltaTime)
                floated = math.min(floated + deltaTime / FloatSeconds, 1)
                local eased = floated * floated * (3 - 2 * floated)
                this.transform.position = ground + vec3(0, FloatHeight * eased, 0)
            end
            return FloatSeconds
        end,
        function()
            this.world:flash(vec3(1, 1, 1), 0.4, 0.1)
            playCutscene(WheelCutscene)
            -- (film 2:37) "47th President of the United States, Donald J. Trump." over the turning wheel
            after(0.5, function() Audio.play(O.VoicePresident, O.VoiceVolume) end)
        end,
        function()
            this.transform.position = ground
            this.world:flash(vec3(0.85, 0.85, 0.85), 1.0, 0.1)
            callTheSummon()
        end,
    })
end

callTheSummon = function()
    this.characterActions:play("Summon")

    -- (the music goes on: the film's soundtrack runs straight into the rest of its song, see FightMusic)

    local summon = find(SummonName)
    local player = find(PlayerName)
    if not summon then return end

    local toPlayer = player and (player.transform.position - this.transform.position) or this.transform.forward
    toPlayer.y = 0
    local place = this.transform.position + toPlayer:normalized() * SummonAppearsAhead

    summon.transform.position = place
    summon.enemy:wake()
    summon.enemy:changeState("Chase")
    -- It roars as it arrives (hits don't stagger it meanwhile)
    summon.characterActions:play("Taunt")
    Audio.playAt(RoarSound, place, 1.6)

    this.world:burst("Summon Burst", place + vec3(0, 1.0, 0), 500)
    this.world:shakeCamera(0.9)
    this.world:slowMotion(1.5, 0.3)
    flash = 0.6

    this.world:notify("WITH THIS TREASURE, I SUMMON...")
    -- The seal burns behind his back from now on
    O.showSeal(1)
end

local function startLastPhase()
    phase = 4
    isChanneling = false
    this.health.isInvulnerable = false
    -- (he doesn't fall at the end of it: at the last point of health his armor comes, see O.startTransformation)
    this.health.minimum = 1

    -- Everything comes faster now
    this.meleeEnemy.attackCooldownSeconds = 0.85
    this.meleeEnemy.heavyAttackChance = 0.35
    this.meleeEnemy.dodgeChance = 0.4
    this.characterActions.attackSpeed = 1.25
    blinkIn = 2.5
    ringIn = 5

    if not domainIsUp then raiseDomain(false) end
    O.startAscension()
end

local function hasReached(healthFraction, atSeconds)
    local now = clock()
    local isHurtEnough = this.health.fraction <= healthFraction and now >= atSeconds - EarlyWindowSeconds
    return isHurtEnough or now >= atSeconds
end

local function updatePhases()
    -- The domain opens when the player has hurt him enough (or, at the latest, late in the soundtrack): the
    -- soundtrack is moved to the film's moment of that blow, so the picture and the sound stay together
    if phase == 1 and (O.wantsKnockback or clock() >= O.DomainLatestAt) then
        O.wantsKnockback = false
        Audio.seekMusic(O.KnockMusicAt)
        startDomainPhase()
    elseif phase == 2 then
        if not hasAnnouncedSummon and seconds >= SummonAtSeconds - 4 then
            hasAnnouncedSummon = true
        end
        if invulnerableLeft <= 0 and hasReached(SummonHealthFraction, SummonAtSeconds) then
            startSummonPhase()
        end
    elseif phase == 3 then
        local summon = find(SummonName)
        if not summon or summon.health.isDead then startLastPhase() end
    elseif phase == 4 and this.health.current <= this.health.minimum + 0.5 and not O.dive and isFreeToAct() then
        O.startTransformation()
    elseif phase == 5 and this.health.fraction <= 0.5 + 0.01 and invulnerableLeft <= 0 and isFreeToAct() then
        O.startPhaseSix()
    end
end

local function updateInvulnerability(deltaTime)
    if invulnerableLeft <= 0 then return end

    invulnerableLeft = invulnerableLeft - deltaTime
    if invulnerableLeft <= 0 and phase ~= 3 then this.health.isInvulnerable = false end
end

-- Everything back to how the scene starts (the player died, the boss rests again)
local function reset()
    phase = 0
    seconds = 0
    invulnerableLeft = 0
    domainIsUp = false
    domainBrokenLeft = 0
    isChanneling = false
    hasAnnouncedSummon = false
    flash = 0
    playerDomainCasts = 0

    this.meleeEnemy.attackCooldownSeconds = 1.25
    this.meleeEnemy.heavyAttackChance = 0.25
    this.meleeEnemy.dodgeChance = 0.3
    this.characterActions.attackSpeed = 1.1

    farSeconds = 0
    blinkIn = 6
    ringIn = 8
    lineIn = 5
    hasGambled = false
    show = nil
    O.opening = nil
    O.tweens = {}
    O.knock = nil
    O.wantsKnockback = false
    O.stopSun()
    local stars = O.stars()
    if stars then stars:reset(); stars.isVisible = false end
    O.isHoldingSign = false
    awaitingDetonation = false
    O.lyingUntilPress = false
    isOnTheDome = false
    silhouetteLeft = 0
    if this.model then this.model.darkness = 0 end
    O.resetAscension()
    O.resetArmor()
    O.slam, O.dash = nil, nil
    setEyes(0)
    captionLeft = 0
    playerGreatCasts = 0
    jetCasts = {}
    rainIn = 0
    patternLeft = 0
    wasPlayerAttacking = false
    timers = {}

    setAsh(false)
    look = Dusk
    lookTo = nil
    applyLook(Dusk)

    local temple = find(TempleName)
    if temple then
        temple.transform.position = vec3(O.TempleHome.x, TempleSunkenY, O.TempleHome.z)
        temple.transform.rotation = vec3(0, 0, 0)
    end
    O.domeOffset = OnTheDome
    templeSeconds, templeDuration = 0, 0
end

local function onDefeat()
    hasSeenDefeat = true
    show = nil
    O.opening = nil
    if this.model then this.model.darkness = 0 end
    setEyes(0)
    timers = {}
    domainIsUp = false
    setAsh(false)
    changeLook(Dawn, 6)
    moveTemple(TempleSunkenY, 9)

    local summon = find(SummonName)
    if summon and not summon.health.isDead and not summon.enemy.isDormant then summon.health:damage(100000) end
    O.setSwordFire(0)
    O.hideSeal()
    local aura = find(O.AscensionAura)
    if aura and aura.particles then aura.particles.isEmitting = false end
end

-- ========== The boss's own state for the summon phase ==========

-- Stands back, can't be hurt, throws a storm of orbs now and then while the summon fights for him
local ChannelState = {
    enter = function(entity)
        entity.enemy:stop()
        entity.enemy:lookAtTarget(true)
    end,

    update = function(entity, deltaTime)
        if not isChanneling then return "Chase" end

        entity.enemy:stop()
        channelSpellIn = channelSpellIn - deltaTime
        if channelSpellIn > 0 then return end

        if phase ~= 3 then return end
        local summon, player = find(SummonName), find(PlayerName)
        local isNearSummon = false
        if summon and player and summon.health and not summon.health.isDead then
            local between = player.transform.position - summon.transform.position
            between.y = 0
            isNearSummon = between:length() < O.ChannelHoldsWithinMeters
        end
        if isNearSummon then
            channelSpellIn = 1.0
        elseif castSpell(OrbStorm) then
            channelSpellIn = ChannelSpellEverySeconds
        end
    end
}

-- The forearms up for a moment, then straight back at the player
local GuardState = {
    enter = function(entity)
        entity.enemy:stop()
        entity.enemy:lookAtTarget(true)
    end,

    update = function(entity, deltaTime)
        if isChanneling then return "Channel" end

        entity.enemy:stop()
        if entity.enemy.secondsInState >= GuardSeconds then
            entity.enemy:startAttackCooldown(0)
            return "Chase"
        end

        -- A block lasts only while it is asked for
        entity.enemy:attack("block")
    end
}

-- ========== Pressure: a string of punches doesn't pin him down ==========

-- Four blows within 2 s (a whole combo landed): he hops back out of them or answers with a heavy blow through them
-- (then not again for 3 s). A combo gets through first: the player's reward, his answer the price of greed
O.PressureHits, O.PressureSeconds, O.PressureCooldown, O.PressureDodgeChance = 4, 2.0, 3.0, 0.5

function O.notePressure()
    local now = clock()
    O.hits = O.hits or {}
    table.insert(O.hits, now)
    while #O.hits > 0 and now - O.hits[1] > O.PressureSeconds do table.remove(O.hits, 1) end
    if #O.hits < O.PressureHits or now < (O.pressureFreeAt or 0) or isChanneling or O.opening or O.knock then return end

    O.hits = {}
    O.pressureFreeAt = now + O.PressureCooldown
    if math.random() < O.PressureDodgeChance then
        this.enemy:dodge(vec3(0, 0, 0))
    else
        this.enemy:attack("heavy")
    end
end

O.CubeBurstsAfter = 0.8
O.PlayerDomainCrashAt = 4.0

-- His lines from the film (the music no longer carries them)
O.VoiceDomain    = "@game/Sounds/Voice/epstein_domain_expansion.ogg"
O.VoiceBetter    = "@game/Sounds/Voice/epstein_better_than_expected.ogg"
O.VoiceTreasure  = "@game/Sounds/Voice/epstein_treasure_summon.ogg"
O.VoicePresident = "@game/Sounds/Voice/epstein_president_trump.ogg"
O.VoiceVolume    = 1.5

-- While the player fights his summon up close he only watches: his storms of money are for a player who runs from it
-- (further than this from the summon), so the summon is never fought with a rain of money in the back
O.ChannelHoldsWithinMeters = 7

-- ========== Called by the engine ==========

-- A hit as hard as the bottle from the sky ends in a blast of white, as in the film
function OnDamaged(amount, kind, attacker)
    if kind == "weapon" then O.notePressure() end
    if phase == 1 and not O.opening and this.health.fraction <= DomainHealthFraction then
        O.wantsKnockback = true
    end
    -- The bottle from the sky: where it lands it becomes a block of baby oil around him (its hit effect, 'Oil Cube'),
    -- which bursts a moment later into a ball of oil and fire - no white screen, the blast is seen
    if kind == "spell" and amount >= 100 then
        this.world:slowMotion(0.9, 0.3)
        this.world:shakeCamera(0.4)
        local at = this.transform.position + vec3(0, 1.4, 0)
        after(O.CubeBurstsAfter, function()
            this.world:effect("Oil Detonation", at)
            this.world:impactFrame(1.0, 0.55)
            this.world:shakeCamera(0.9)
            Audio.play(BoomSound, 1.0)
        end)
    end
end

function OnStart()
    this.enemy:addState("Guard", GuardState)
    this.enemy:addState("Channel", ChannelState)
    reset()
end

function OnUpdate(deltaTime)
    local boss = this.boss
    O.updateSeal(deltaTime)
    -- His transformation runs through its own cutscenes and the moments between them: nothing else meanwhile
    if O.transform then
        O.updateTransform(deltaTime)
        updateTweens(deltaTime)
        updateLook(deltaTime)
        return
    end

    if boss.isDefeated or this.health.isDead then
        if not hasSeenDefeat then onDefeat() end
        updateLook(deltaTime)
        updateTemple(deltaTime)
        return
    end

    -- Resting (before the fight, or after the player died), or the film's O.opening still plays
    if not boss.isAwake then
        if phase ~= 0 then reset() end
        return
    end

    -- A cutscene holds the fight still, but the world goes on changing (the temple rising in the domain's reveal)
    if this.world.isCutscenePlaying then
        if phase > 0 then
            updateOpening()
            O.updateRise(deltaTime)
            O.updateKnock(deltaTime)
            O.updateSign(deltaTime)
            updateTweens(deltaTime)
            updateTimers(deltaTime)
            updateLook(deltaTime)
            updateTemple(deltaTime)
            rideTheDome(deltaTime)
        end
        return
    end

    if phase == 0 then startFight() end

    -- The O.opening of the domain goes on (between its cutscenes too): the fight waits
    if O.opening then
        updateOpening()
        O.updateRise(deltaTime)
        O.updateKnock(deltaTime)
        O.updateSign(deltaTime)
        updateTweens(deltaTime)
        updateTimers(deltaTime)
        updateLook(deltaTime)
        updateTemple(deltaTime)
        rideTheDome(deltaTime)
        return
    end

    -- A staged moment goes on: its next step now, the fight waits
    if show then
        updateShow(deltaTime)
        updateTimers(deltaTime)
        announceTechniques(deltaTime)
        updateLook(deltaTime)
        updateTemple(deltaTime)
        return
    end

    updateSilhouette(deltaTime)
    updateCameraDistance(deltaTime)

    seconds = seconds + deltaTime

    updateTimers(deltaTime)
    patternLeft = math.max(patternLeft - deltaTime, 0)
    announceTechniques(deltaTime)
    watchPlayerGreatSpell()
    watchPlayerDomain()
    updateInvulnerability(deltaTime)
    updatePhases()
    updateGuard()
    updateBlink(deltaTime)
    updateRing(deltaTime)
    updateLines(deltaTime)
    updateRain(deltaTime)
    watchJetSpells()
    updateGamble()
    O.updateAscension(deltaTime)
    O.updateArmored(deltaTime)
    updateDomain(deltaTime)
    updateTweens(deltaTime)
    O.updateRise(deltaTime)
    if not O.lyingUntilPress and not promptDetonation(deltaTime) then promptPlayerDomain(deltaTime) end
    updateLook(deltaTime)
    updateTemple(deltaTime)
end
