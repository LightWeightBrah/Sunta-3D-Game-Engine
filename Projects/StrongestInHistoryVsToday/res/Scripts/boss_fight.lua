-- The fight of this project: one boss in four phases, loosely following the film's soundtrack
--
-- Put on the boss (an enemy with Boss, Melee Enemy and Spell Caster children). Everything it uses is general:
-- the Health floor ('minimum') keeps the boss alive until its last phase, 'isInvulnerable' covers the changes of phase,
-- a dormant enemy is woken as the summon, the sky belongs to the Environment and the temple is moved by its Transform
--
--   Phase 1  Strongest in History : blows and volleys of orbs
--   Phase 2  Domain Expansion     : the sky turns, the temple rises, fire under the player's feet, the domain heals him.
--                                   The player's own domain (key 3) shatters it for a while
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
local OpeningHitDamage = 110

local PhaseChangeInvulnerableSeconds = 3.0
local DomainHealPerSecond   = 9
local DomainSpellEverySeconds = 6.5
local DomainBrokenSeconds   = 18
local ChannelSpellEverySeconds = 14
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
local RingEverySeconds = 20
local RingHeight       = 1.1

-- Lines of fire across the domain: three rows, one after another like a wave, each this long
local LineEverySeconds = 15
local LineFires        = 9
local LineSpacing      = 2.3
local LineGap          = 3.6
local LineWaveSeconds  = 0.4

-- The jet that flies over the arena before fire comes down: how high, and how far before its target it appears
local JetHeight      = 8   -- (low over the fight: the player sees it pass)
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
local PatternGapSeconds = 9
local LastStandGapSeconds = 6

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

-- (not used any more: bundles thrown from somewhere over the far wall hit the player in the back out of nowhere, and
-- a soulslike boss throws little - the domain keeps its lines of fire and its stars)
local function updateRain(deltaTime)
    do return end
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
        -- (after his summon not more often but less: a soulslike throws few things at the player)
        lineIn = (phase == 4) and LineEverySeconds * 1.3 or LineEverySeconds
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

    -- (in his armor: O.updateSwordCamera - this function comes before the table O, which it can't see)
    if phase >= 5 then return end
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
    KnockSkidMeters = 8,
    KnockFlySeconds = 1.4,
    KnockSkidSeconds = 1.7,
    -- (as in the film he doesn't lie: he lands crouched and skids, and rises with the sign while still sliding)
    KnockRiseAt = 0.55,
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

    -- (the first time the hand rises into the sign; then the sign is held, played again only once it is over: a
    -- clip started again in the middle jumps)
    O.holdSignIn = O.holdSignIn - deltaTime
    if O.holdSignIn <= 0 and (not this.characterActions.isBusy or not O.hasRaisedSign) then
        this.characterActions:play(O.hasRaisedSign and "Sign_Hold" or "Sign_Raise")
        O.hasRaisedSign = true
        O.holdSignIn = 0.5
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

    -- (it burns while the temple rises out of it, carrying him up on its top, then it dies down into the temple)
    after(3.6, function()
        O.tween(function(v) sun.particles.particlesPerSecond = v end, O.SunRate, 0, 1.6)
        after(2.3, function() sun.particles.isEmitting = false end)
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
    O.tween(function(v) stars.gathered = v end, 0, 1, 0.8)
    O.tween(function(v) stars.radius = v end, 5, 36, 3.2)
end

-- The camera flies through them, while they spread over the whole sky above the player (film 1:30.75-1:32.75)
function O.flyThroughStars()
    local stars, entity = O.stars()
    local player = O.playerEntity()
    if not stars or not player then return end

    local center = entity.transform.position
    -- The camera flies over the sphere of stars and out among them (as the user liked it: kept)
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

    -- (thrown flat, as the user liked it; then he lands into a skid on his feet)
    this.characterActions:play("Knocked_Down")
    -- (no white flash of the whole screen: the jolt of the picture and the slow motion carry the blow)
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
            this.characterActions:play("Skid_Crouch")
            this.world:shakeCamera(0.7)
            this.world:burst(O.SkidDust, k.land, 24)
            this.world:effect("Crash Impact", k.land, this.transform.forward)
        end
        local f = (k.t - fly) / skid
        this.transform.position = mix(k.land, k.skid, 1 - (1 - f) * (1 - f))
        -- (a furrow of dust behind his feet, thinning as he slows)
        k.dustIn = k.dustIn - deltaTime
        if k.dustIn <= 0 then
            k.dustIn = 0.05
            this.world:burst(O.SkidDust, this.transform.position, math.floor(6 * (1 - f)) + 1)
        end
        -- Still sliding, he straightens up and raises the sign
        if not k.hasRisen and f >= O.KnockRiseAt then
            k.hasRisen = true
            O.isHoldingSign, O.holdSignIn, O.hasRaisedSign = true, 0, false
        end
        return
    end

    this.transform.position = k.skid
    O.knock = nil
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
    -- (the dome comes up under his feet where he stands, if that is far enough: he isn't moved in the middle of
    -- his sign's close shot, the fire swelling behind him)
    if math.abs(away:length() - distance) < 0.5 then
        local offset = O.turned(OnTheDome, yaw)
        local under = clampToArena(vec3(this.transform.position.x - offset.x, 0, this.transform.position.z - offset.z))
        feet = vec3(under.x, 0, under.z)
    end
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
            -- The film's own sound from here to the player lying down (his words, the music, the blasts: the user
            -- found the cut-out voices worse and off the film's timing); the soundtrack goes on silent under it
            O.playFilmAudio(O.DomainFilmAudio, O.DomainFilmAudioSeconds)
        end },
        { 44.6, function()
            O.knock = nil
            O.isHoldingSign = false
            playCutscene(SignCutscene)
        end },
        -- The hill of fire swells while he still speaks (film 1:22.3), the temple rising out of it carries him up: it stands as his words end (1:24.7), as in the film
        { 46.1, function()
            changeLook(Domain, 1.8)
            setAsh(true)
            this.world:shakeCamera(0.5)
            -- (quieter than his words)
            Audio.playAt(RoarSound, this.transform.position, 0.6)
            O.placeTempleUnderBoss()
            isOnTheDome = true
            O.isHoldingSign, O.holdSignIn = true, 0
            moveTemple(TempleRisenY, 4.6)
            Audio.play(RumbleSound, 0.8)
        end },
        -- (the fire with the wide shot: close to him the camera was inside it, the screen white)
        { 47.0, function()
            O.startSun(find(TempleName).transform.position)
            playCutscene(RevealCutscene)
        end },
        -- Out of the dome under his feet the stars burst into a turning sphere
        -- (only once the temple stands, as in the film)
        -- (film 1:28.3: they burst out of the dome at once - grown over two seconds they were seen a second late)
        { 51.9, function() O.burstStars() end },
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
            O.runIntoStars()
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

        -- (he stays down: the player gets him up with [2], see O.updateRise)
        -- The temple sinks again, he is back on the ground before the player, still black with white eyes
        { 65.8, function()
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
        -- The player's turn: early enough that he is up for the sign of the film (1:47.5) with the presses it takes
        { 66.0, function()
            awaitingDetonation = true
            O.detonationHintLeft = O.DetonationHintSeconds
            O.lyingUntilPress, O.lyingReplayIn = true, 0
            O.starsWaitForTechnique = true
            playCutscene(O.DownCutscene)
            invulnerableLeft = 0.5
            this.enemy:startAttackCooldown(2.5)
            rainIn = RainEverySeconds
        end },
        -- Sunk out of sight: the temple goes back behind the far wall and stands there for the rest of the domain
        { 74.0, function()
            -- (the stars that threw him down stay until his answer has landed: O.endTechniqueHold)
            local stars = O.stars()
            if stars and not O.starsWaitForTechnique then stars:reset(); stars.isVisible = false end
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
-- (the technique is called once he is nearly on his feet: called earlier his hands made the sign half way up)
O.DownCutscene, O.RiseCutscene, O.RiseSeconds = "Down Wait", "Technique Rise", 2.6
O.RiseClearsStars = 6.5

function O.updateRise(deltaTime)
    if not O.lyingUntilPress then return end

    -- Nobody strikes a man who is down
    this.enemy:startAttackCooldown(1)
    rainIn, lineIn = math.max(rainIn, 2), math.max(lineIn, 2)

    -- (played again only near its end: started again in the middle it jumped)
    O.lyingReplayIn = O.lyingReplayIn - deltaTime
    if O.lyingReplayIn <= 0 then
        O.lyingReplayIn = 3.8
        O.playPlayer("Lying")
    end

    local down = find(O.DownCutscene)
    if not down or not down.cutscene or down.cutscene.isPlaying or not down.cutscene.wasAnswered then return end

    O.lyingUntilPress = false
    O.playPlayer("Get_Up")
    playCutscene(O.RiseCutscene)
    -- Getting up he throws off the stars stuck right around him (the camera of his rise stood behind one of them and
    -- saw nothing else for three seconds); the rest stay for his technique's blast to break
    local stars = O.stars()
    local player = O.playerEntity()
    if stars and stars.isVisible and player then
        for _, at in ipairs(stars:breakWithin(player.transform.position, O.RiseClearsStars)) do
            this.world:burst(O.StarWave.Sparks, at + vec3(0, 0.3, 0), 3)
        end
        this.world:effect("Solid Oil Burst", player.transform.position + vec3(0, 1.0, 0), player.transform.forward)
    end
    -- He doesn't strike until the player's technique has landed (O.holdingForTechnique ends with its blast)
    O.holdingForTechnique = true
    O.techniqueEndsIn = nil
    local great = find(PlayerGreatSpell)
    O.techniqueReleasesBefore = great and great.spellCaster and great.spellCaster.timesReleased or 0
    after(O.RiseSeconds, function()
        -- (forced: still getting up he is busy, and a refused cast left him silent and without his technique)
        local spell = find(PlayerGreatSpell)
        if spell and spell.spellCaster then spell.spellCaster:cast(true) end
    end)
end

-- After the domain's blow the boss waits, and the stars that threw the player down stay where they fell, until
-- the player's answer has landed: then the stars are gone with the blast (O.endTechniqueHold)
function O.holdForTechnique()
    if not O.holdingForTechnique then return end
    -- (he stands where he is and takes it: he walked up and struck the player in the middle of the sign)
    if not isChanneling then
        isChanneling = true
        O.isChannelingForTechnique = true
        this.enemy:changeState("Channel")
    end
    this.enemy:startAttackCooldown(1)
    rainIn, lineIn = math.max(rainIn, 2), math.max(lineIn, 2)

    -- The technique left his hands: the hold ends when its bottle has burst (see OnDamaged)
    local spell = find(PlayerGreatSpell)
    local released = spell and spell.spellCaster and spell.spellCaster.timesReleased or 0
    if not O.techniqueEndsIn and released > (O.techniqueReleasesBefore or 0) then
        O.techniqueEndsIn = true
        after(2.4 + O.CubeBurstsAfter, O.endTechniqueHold)
    end
end

-- ========== Stars from his fingers (film 0:42-0:46) ==========

-- Two fingers raised before his face, then both hands point at the player and a fan of the stars of his domain flies
-- low across the arena at them (in the phases without his sword)
O.Stars = { EverySeconds = 18, FirstAfter = 12, Count = 5, SpreadDegrees = 15, ReleaseAfter = 1.5, Height = 1.4,
            Bolt = "Boss Star Bolt", Caption = "Island Stars: Constellation", MinDistance = 4, MaxDistance = 18 }
O.starsIn = O.Stars.FirstAfter

function O.updateStarVolley(deltaTime)
    O.starsIn = O.starsIn - deltaTime
    if O.starsIn > 0 or phase < 1 or phase >= 5 or phase == 3 or not isFreeToAct() then return end
    local distance = this.enemy.distanceToTarget
    if distance < O.Stars.MinDistance or distance > O.Stars.MaxDistance or not takePatternTurn() then return end

    O.starsIn = O.Stars.EverySeconds
    if not this.characterActions:play("Cast_Stars") then return end
    this.enemy:lookAtTarget(true)
    showCaption(O.Stars.Caption, 2.0)
    after(O.Stars.ReleaseAfter, function()
        local player = find(PlayerName)
        if not player then return end
        local from = this.transform.position + vec3(0, O.Stars.Height, 0) + this.transform.forward * 0.6
        local toPlayer = player.transform.position + vec3(0, 1.0, 0) - from
        toPlayer.y = 0
        local ahead = toPlayer:normalized()
        local yaw = math.atan(ahead.x, ahead.z)
        for i = 1, O.Stars.Count do
            local turn = math.rad((i - (O.Stars.Count + 1) / 2) * O.Stars.SpreadDegrees)
            local direction = vec3(math.sin(yaw + turn), 0, math.cos(yaw + turn))
            this.world:launch(O.Stars.Bolt, this, from + direction * 0.3, direction)
        end
        this.world:shakeCamera(0.3)
        Audio.playAt(BoomSound, from, 0.7)
    end)
end

function O.endTechniqueHold()
    O.holdingForTechnique = false
    if O.isChannelingForTechnique then
        O.isChannelingForTechnique = false
        isChanneling = false
    end
    if O.starsWaitForTechnique then
        O.starsWaitForTechnique = false
        -- (the wave of the bottle's blast breaks them, from the blast outwards; the bottle may still be falling:
        -- they are cleared without it only if no blast comes)
        O.starsAwaitWave = true
        if O.starWave then O.starWave.resetAfter = true end
        after(6, function()
            if O.starsAwaitWave and not O.starWave then
                O.starsAwaitWave = false
                local stars = O.stars()
                if stars then stars:reset(); stars.isVisible = false end
            end
        end)
    end
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
    -- (carried, not falling: no gravity and no push of the rising walls against his body, or he jerked up and down
    -- every frame between the script's place and the physics'; given back the moment he leaves the dome)
    local body = this.physicsBody
    if O.isRidingDome ~= isOnTheDome and body then
        body.useGravity = not isOnTheDome
        body.isKinematic = isOnTheDome
        body.velocity = vec3(0, 0, 0)
    end
    O.isRidingDome = isOnTheDome
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
        domainSpellIn = (phase == 4) and DomainSpellEverySeconds * 1.3 or DomainSpellEverySeconds
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
    -- (no red flash of the whole screen: it hurt the eyes - the seal's burst of light is enough)
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
            this.world:impactFrame(0.6, 0.25)
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
-- (the scene takes its time: every moment of it has a shot of its own, his face close twice - the smile as his health
-- comes back, the smile again as his eyes catch fire)
-- (the rift is seen opening in the sky and the pieces seen coming out of it, before they land around him)
O.PieceFallAt = { torso = 5.6, greave_l = 5.85, greave_r = 6.1, bracer_l = 6.35, bracer_r = 6.6 }
O.FallSeconds, O.FallFrom = 1.1, 16
O.SwordFallAt, O.SwordInGround = 8.6, 1.45
-- The two beats of the snap: the torso and the greaves, then the bracers (and his armored model put on)
O.FirstSnapAt, O.SecondSnapAt = 10.8, 11.8
O.SnapPose = "Hover"
O.LiftFrom, O.BracersLiftFrom = 9.9, 11.0
O.SlideSeconds, O.SmileAt, O.RefillAt, O.RiftAt = 1.2, 2.4, 3.6, 4.8
O.WalkAt, O.GripAt, O.DrawAt, O.IgniteAt, O.StompAt, O.TransformEnds = 12.6, 13.25, 13.7, 15.4, 17.2, 18.8
O.Shots = { { 0.0, "Armor Fakeout" }, { 1.2, "Armor Face" }, { 4.8, "Armor Rift Sky" }, { 6.5, "Armor Rift Shot" },
            { 8.5, "Armor Sword Fall" }, { 9.5, "Armor Snap" }, { 12.5, "Armor Draw" }, { 15.0, "Armor Ignite Face" },
            { 17.0, "Armor Ignite" } }

-- His smile (the expression shape of his face): on over this long
O.SmileSeconds = 0.9
-- (part of the way: the full shape looked strange in the close shots)
O.SmileAmount = 0.55
O.ArmorTitle = "Jeffrey Epstein, Armored in Sin"
O.Blood = {
    -- (red, but light enough that he is seen: the close shots of his face are in it)
    light = vec3(1.0, 0.62, 0.55), ambient = vec3(0.3, 0.17, 0.16), fog = vec3(0.17, 0.05, 0.05),
    fogDensity = 0.011, sky = 0.9, exposure = 1.0, bloom = 0.9
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
-- Which way he faces (degrees about Y, 0 = his front along -Z): from where he looks, not from his rotation's Euler
-- angles - read back from his rotation they can come out as another set (180, y, 180), the plates were put on him
-- turned a quarter, a shoulder plate hiding his head
function O.facingYaw()
    local f = this.transform.forward
    return math.deg(math.atan(-f.x, -f.z))
end

function O.onHim(offset)
    return this.transform.position + O.turned(offset, O.facingYaw())
end

function O.floorSpot(piece)
    local spot = O.PieceFloor[piece]
    local angle = math.rad(spot[1])
    return O.onHim(vec3(math.sin(angle) * spot[2], 0.25, -math.cos(angle) * spot[2]))
end

function O.hideArmorProps()
    for _, piece in ipairs(O.Pieces) do
        local entity = O.pieceEntity(piece)
        if entity and entity.model then entity.model.isVisible = false; entity.model.rimGlow = vec3(0, 0, 0) end
    end
    local dropped = find("Armor Drop Sword")
    if dropped and dropped.model then dropped.model.isVisible = false end
end

-- Back to the man of the first phases (the player died: the fight starts over)
function O.resetArmor()
    O.transform = nil
    O.vessel = nil
    local player = find(PlayerName)
    local bottle = player and player:findChild("Vessel Bottle")
    if bottle and bottle.model then bottle.model.isVisible = false end
    if O.setPlayerSword then O.setPlayerSword(false) end
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
            local yaw = O.facingYaw()
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
                -- (outlined in red as they fly: dark plates in a red dusk were hard to see)
                entity.model.rimGlow = vec3(1.0, 0.18, 0.08) * (0.6 + 0.8 * k)
                entity.transform.rotation = vec3(12 * (1 - e), yaw + O.PieceFloor[piece][1] * (1 - e), 0)
            elseif tr.t >= snapAt then
                entity.transform.position = onBody
                entity.transform.rotation = vec3(0, yaw, 0)
                if not tr.snapped[piece] then
                    tr.snapped[piece] = true
                    entity.model.rimGlow = vec3(0, 0, 0)
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

    -- the shots of the cutscene, one after another (each cuts in on the one before at its moment)
    local shot = O.Shots[tr.shot]
    if shot and tr.t >= shot[1] then
        playCutscene(shot[2])
        tr.shot = tr.shot + 1
    end

    -- 0-4: thrown back, skidding on his boots; close on his face: a beat of silence, the smile, his health fills
    if tr.t < O.SlideSeconds then
        local k = tr.t / O.SlideSeconds
        this.transform.position = mix(tr.slideFrom, tr.slideTo, 1 - (1 - k) * (1 - k))
        tr.dustIn = (tr.dustIn or 0) - deltaTime
        if tr.dustIn <= 0 then
            tr.dustIn = 0.08
            this.world:burst(O.SkidDust, this.transform.position, 3)
        end
    end
    if at(O.SlideSeconds) then this.characterActions:play("Idle") end
    if at(O.SmileAt) and this.model then
        O.tween(function(v) if this.model then this.model.expression = v end end, 0, O.SmileAmount, O.SmileSeconds)
    end
    if at(O.RefillAt) then
        this.world:impactFrame(0.5, 0.2)
        this.world:shakeCamera(0.6)
        Audio.play(BoomSound, 1.1)
        this.world:notify(string.upper(O.ArmorTitle))
        -- (his bar carries his new name)
        if this.enemyHealthBar then
            O.BaseTitle = O.BaseTitle or this.enemyHealthBar.displayName
            this.enemyHealthBar.displayName = O.ArmorTitle
        end
        O.tween(function(v)
            local wanted = this.health.max * v
            if this.health.current < wanted then this.health:heal(wanted - this.health.current) end
        end, 0.05, 1, 0.9)
    end

    -- 4-7.4: the rift over the arena, the pieces and the sword falling out of it
    if at(O.RiftAt) then
        if this.model then this.model.expression = 0 end
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

    -- 7.4-10.2: arms spread, the plates snap onto him in two beats
    -- (squarely facing his front while the plates fly on: in his fighting stance (Idle) his body is turned a quarter
    -- and the still plates sat across it, a shoulder plate over his head)
    if at(O.LiftFrom) then this.characterActions:play(O.SnapPose) end
    if at(O.FirstSnapAt) then
        -- The torso and the greaves are on him: his armored body from now (the loose plates never sat exactly on
        -- his moving body - a shoulder plate stood over his head); only the bracers still fly on
        if this.model then this.model.modelName = O.ArmoredModel end
        for _, piece in ipairs({ "torso", "greave_l", "greave_r" }) do
            local entity = O.pieceEntity(piece)
            if entity and entity.model then entity.model.isVisible = false end
        end
        this.world:effect("Armor Clank", O.onHim(O.PieceOffsets.torso), this.transform.forward)
        this.world:shakeCamera(0.55)
        Audio.playAt(O.MetalSound, this.transform.position, 1.4)
    end
    if at(O.SecondSnapAt) then
        -- the bracers click onto his forearms: he is in his armor now
        if this.model then this.model.modelName = O.ArmoredModel end
        O.hideArmorProps()
        if dropped and dropped.model then dropped.model.isVisible = true end
        this.characterActions:play("Idle")
        this.world:shakeCamera(0.7)
        this.world:effect("Armor Clank", O.onHim(O.PieceOffsets.bracer_l), this.transform.forward)
        this.world:effect("Armor Clank", O.onHim(O.PieceOffsets.bracer_r), this.transform.forward)
        Audio.playAt(O.MetalSound, this.transform.position, 1.5)
    end

    -- 10.2-12.6: a heavy step to the sword, it is ripped out of the stone
    if at(O.WalkAt) then this.characterActions:play("Walk") end
    if tr.t >= O.WalkAt and tr.t < O.GripAt then
        this.transform.position = this.transform.position + this.transform.forward * (deltaTime * 0.9)
    end
    if at(O.GripAt) then this.characterActions:play("Attack_Light_3") end
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

    -- 12.6-16.2: close on his face, his eyes burn red and he smiles again; then a stamp, a wave of dust - and he fights
    if at(O.IgniteAt) then
        O.setRedEyes(8.0)
        O.sealSpin = 900
        O.sealTarget = 1.2
        O.setSwordFire(28)
        Audio.playAt(RoarSound, this.transform.position, 1.3)
        O.tween(function(v) if this.model then this.model.expression = v end end, 0, O.SmileAmount, O.SmileSeconds * 0.7)
    end
    if at(O.StompAt) then
        this.world:effect("Armor Stomp", this.transform.position + vec3(0, 0.1, 0), this.transform.forward)
        this.world:shakeCamera(0.8)
        Audio.play(BoomSound, 1.2)
    end
    if tr.t >= O.TransformEnds then
        O.transform = nil
        if this.model then this.model.expression = 0 end
        O.startArmoredPhase()
    end
end

-- ========== The player's answer: Divine Baby Oil: Crystal Vessel ==========

-- As the boss takes up his sword, the player takes up his: his model with the sword in its fist (the same man, the
-- same clips by name), the sword drawn out of a burst of oil, his fists no longer the weapon
-- (his sword comes with his armor of baby oil: crystal plates on his chest, shoulders, forearms and shins)
O.PlayerPlainModel, O.PlayerSwordModel = "sih_player", "sih_player_armored"
O.PlayerSwordTitle = "Divine Baby Oil: Crystal Vessel"

function O.setPlayerSword(isOn)
    local player = find(PlayerName)
    if not player then return end
    if player.model then player.model.modelName = isOn and O.PlayerSwordModel or O.PlayerPlainModel end
    local sword = player:findChild("Player Sword")
    if sword then
        if sword.model then sword.model.isVisible = isOn end
        if sword.hitbox then sword.hitbox.isEnabled = isOn end
    end
    for _, name in ipairs({ "Right Fist", "Left Fist", "Right Foot" }) do
        local limb = player:findChild(name)
        if limb and limb.hitbox then limb.hitbox.isEnabled = not isOn end
    end
    local actions = player.characterActions
    if not actions then return end
    if isOn then
        -- (the blade strikes in the middle of each slash: the windows of the knight's sword, see knight_generator)
        for _, name in ipairs({ "Attack_Light_1", "Attack_Light_2", "Attack_Light_3", "Attack_Light_4",
                                "Attack_Running", "Attack_Riposte" }) do actions:setMove(name, 0.28, 0.58, 1.0) end
        for _, name in ipairs({ "Attack_Heavy", "Attack_Heavy_2" }) do actions:setMove(name, 0.42, 0.68, 1.6) end
        actions.swingEffect, actions.heavySwingEffect = "", ""
    end
end

-- The player's answer as a short sequence of shots (the user's scenario without the armor yet): over the boss's
-- shoulder at him; close on his hands making the sign of his great technique, rays of light out of him; beside him as
-- the sword of oil comes out of a burst of oil in slow motion; wide on both of them with the name of it. The boss waits
-- Each shot starts the moment the one before it ends (the shots keep real time, the game's time is slowed meanwhile)
O.Vessel = { { "Vessel Answer", "answer" }, { "Vessel Bottle", "bottle" }, { "Vessel Gesture", "gesture" }, { "Vessel Armor", "armor" },
             { "Vessel Draw", "draw" }, { "Vessel Face-off", "faceoff" } }

function O.startVessel()
    O.vessel = { step = 0, shot = nil }
    isChanneling = true
    this.enemy:startAttackCooldown(10)
end

function O.vesselBeat(beat)
    local player = find(PlayerName)
    local bottle = player and player:findChild("Vessel Bottle")
    if beat == "bottle" and player then
        -- His palm opens and a bottle of baby oil gathers over it out of drops of light
        if player.characterActions then player.characterActions:play("Cast_Orb") end
        after(0.1, function()
            if bottle and bottle.model then bottle.model.isVisible = true end
            this.world:burst("Adapt Burst", player.transform.position + vec3(0, 1.2, 0), 40)
            Audio.play("@game/Sounds/Spells/magic_whoosh.ogg", 0.6)
        end)
    elseif beat == "gesture" and player then
        -- (the fist closes: the bottle bursts into the oil that becomes his armor)
        if bottle and bottle.model then
            bottle.model.isVisible = false
            this.world:effect("Solid Oil Burst", player.transform.position + vec3(0, 1.3, 0), player.transform.forward)
            Audio.play("@game/Sounds/Spells/glass_break.ogg", 1.0)
        end
        if player.characterActions then player.characterActions:play("Cast_Great_Orb") end
        local center = player.transform.position + vec3(0, 1.1, 0)
        for i = 1, 10 do
            after(i * 0.08, function() this.world:effect("Technique Ray", center, player.transform.forward) end)
        end
        Audio.play("@game/Sounds/Spells/magic_whoosh.ogg", 1.0)
    elseif beat == "armor" and player then
        -- The oil turns to crystal on him in two beats (the user's scenario): the chest and the shoulders, then the
        -- forearms and the shins - the crystal plates are on him from the first beat on
        local body = player.transform.position
        this.world:effect("Solid Oil Burst", body + vec3(0, 0.6, 0), player.transform.forward)
        Audio.play("@game/Sounds/Spells/magic_whoosh.ogg", 0.9)
        after(0.5, function()
            if player.model then player.model.modelName = O.PlayerSwordModel end
            this.world:effect("Solid Oil Burst", body + vec3(0, 1.35, 0), player.transform.forward)
            this.world:shakeCamera(0.3)
            Audio.play("@game/Sounds/Spells/glass_break.ogg", 0.9)
        end)
        after(1.1, function()
            this.world:effect("Solid Oil Burst", body + vec3(0.35, 1.1, 0), player.transform.forward)
            this.world:effect("Solid Oil Burst", body + vec3(-0.35, 0.4, 0), player.transform.forward)
            Audio.play("@game/Sounds/Spells/glass_break.ogg", 0.7)
        end)
    elseif beat == "draw" and player then
        O.setPlayerSword(true)
        if player.characterActions then player.characterActions:play("Draw_Sword") end
        local hand = player.transform.position + vec3(0, 1.1, 0) + player.transform.forward * 0.4
        this.world:effect("Solid Oil Burst", hand, player.transform.forward)
        this.world:slowMotion(1.2, 0.4)
        Audio.play("@game/Sounds/Spells/glass_break.ogg", 1.0)
    elseif beat == "faceoff" then
        -- (its name is the title of this shot)
        Audio.playAt(RoarSound, this.transform.position, 1.0)
    end
end

function O.updateVessel(deltaTime)
    local v = O.vessel
    if not v then return end
    this.enemy:startAttackCooldown(1)
    local current = v.shot and find(v.shot)
    if current and current.cutscene and current.cutscene.isPlaying then return end

    v.step = v.step + 1
    local next = O.Vessel[v.step]
    if not next then
        O.vessel = nil
        isChanneling = false
        return
    end
    v.shot = next[1]
    playCutscene(next[1])
    O.vesselBeat(next[2])
end

function O.armPlayer()
    local player = find(PlayerName)
    if not player then return end
    O.setPlayerSword(true)
    if player.characterActions then player.characterActions:play("Draw_Sword") end
    local hand = player.transform.position + vec3(0, 1.1, 0) + player.transform.forward * 0.4
    this.world:effect("Solid Oil Burst", hand, player.transform.forward)
    this.world:notify(O.PlayerSwordTitle)
end

function O.startArmoredPhase(isResumed)
    phase = 5
    O.refillFlasks()
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
    -- (right away: the player's answer, his own sword of oil, as a short sequence of shots - seen once: a try that
    -- resumes here gives him his armor and sword at once)
    if isResumed then O.armPlayer() else O.startVessel() end
end

function O.startPhaseSix()
    phase = 6
    setHealthFloor(0)
    becomeInvulnerable(2.2)
    this.characterActions:play("Sword_Power_Up")
    this.world:notify("THE BLADE OF THE ISLAND BURNS")
    this.world:slowMotion(0.8, 0.4)
    this.world:shakeCamera(0.9)
    this.world:impactFrame(0.7, 0.3)
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
    -- (a brawl: he comes at the player again and again, short breaths between his combos)
    this.meleeEnemy.attackCooldownSeconds = O.BrawlCooldown
    this.meleeEnemy.recoverSeconds = 0.45
    this.meleeEnemy.maxLightCombo = 4
    seconds = 0
    hasSeenDefeat = false
    setHealthFloor(DomainHealthFraction - FloorMargin)
    Audio.playMusic(FightMusic, 0.9, 0.2)

    -- A try after dying in his armor starts in his armor (a checkpoint): no opening, no phases before it again
    if O.resumeArmored then
        this.health.isInvulnerable = true
        after(1.0, function()
            if phase ~= 1 then return end
            if this.model then this.model.modelName = O.ArmoredModel end
            O.hideArmorProps()
            O.setRedEyes(8.0)
            O.showSeal(1.2)
            O.startArmoredPhase(true)
            this.health.isInvulnerable = false
            this.world:notify("THE FIGHT RESUMES: " .. string.upper(O.ArmorTitle))
            if this.enemyHealthBar then
                O.BaseTitle = O.BaseTitle or this.enemyHealthBar.displayName
                this.enemyHealthBar.displayName = O.ArmorTitle
            end
        end)
        return
    end

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
        -- He leaps away from the player into the air, seen in the game, before the film's pictures fade in
        function()
            local player = find(PlayerName)
            local away = player and (ground - player.transform.position) or vec3(0, 0, -1)
            away.y = 0
            away = away:length() > 0.1 and away:normalized() or vec3(0, 0, -1)
            -- (a backflip pushed off the ground, not a statue gliding up: up fast and slowing at the top, back all the
            -- way, the hover once he has turned over)
            this.characterActions:play("Leap_Back")
            -- (carried by the script up there: the physics would pull him down between its frames)
            if this.physicsBody then this.physicsBody.useGravity = false; this.physicsBody.velocity = vec3(0, 0, 0) end
            this.world:burst(O.SkidDust, ground, 30)
            this.world:shakeCamera(0.5)
            playCutscene(O.LeapAwayCutscene)
            local leap, hovering = 0, false
            show.during = function(deltaTime)
                leap = math.min(leap + deltaTime / 1.4, 1)
                local back = 1 - (1 - leap) ^ 3
                local up = math.sin(math.min(leap / 0.8, 1) * math.pi * 0.5)
                this.transform.position = clampToArena(ground + away * (O.FloatBackMeters * back)) + vec3(0, 3.5 * up, 0)
                if leap >= 0.75 and not hovering then
                    hovering = true
                    this.characterActions:play("Hover")
                end
            end
            return 1.4
        end,
        -- (film 2:31) "He is doing better than I expected." - the film's own pictures of him floating on his back
        -- over the burning city as he speaks (faded in from black), and in the game he keeps hanging up there
        function()
            local hangAt = this.transform.position
            ground = vec3(hangAt.x, 0, hangAt.z)
            floated = 1
            -- The film's own sound of the three lines and the wheel (film 2:30.6-2:41), its pictures in step with it
            O.playFilmAudio(O.SummonFilmAudio, O.SummonFilmAudioSeconds)
            after(O.SummonFloatVideoAfter, function() playCutscene(O.FloatCutscene) end)
            this.characterActions:play("Hover")
            local player = find(PlayerName)
            local away = player and (ground - player.transform.position) or vec3(0, 0, -1)
            away.y = 0
            away = away:length() > 0.1 and away:normalized() or vec3(0, 0, -1)
            local hovered = 0
            O.floatDuring = function(deltaTime)
                floated = math.min(floated + deltaTime / O.FloatRiseSeconds, 1)
                hovered = hovered + deltaTime
                local eased = floated * floated * (3 - 2 * floated)
                this.transform.position = clampToArena(ground + away * (O.FloatBackMeters * eased))
                    + vec3(0, FloatHeight * eased + math.sin(hovered * 2.2) * 0.12 * eased, 0)
            end
            show.during = O.floatDuring
            return 3.0
        end,
        -- (film 2:34) "Well, with this treasure I summon..." (in the film's sound)
        function()
            show.during = O.floatDuring
            return O.SummonWheelAfter - 3.0
        end,
        function()
            this.world:flash(vec3(1, 1, 1), 0.4, 0.1)
            playCutscene(WheelCutscene)
            -- (film 2:37) "47th President of the United States, Donald J. Trump." over the turning wheel
            -- (the film's pictures to the end of his words: him falling into the white after the wheel; his words in
            -- the film's sound)
            return 4.4
        end,
        function()
            this.transform.position = ground
            if this.physicsBody then this.physicsBody.useGravity = true; this.physicsBody.velocity = vec3(0, 0, 0) end
            this.world:flash(vec3(0.85, 0.85, 0.85), 1.0, 0.1)
            callTheSummon()
        end,
    })
end

callTheSummon = function()
    this.characterActions:play("Summon")
    -- (while his summon fights he can't be locked on to: locked on to him behind it, the monster filled the screen)
    if this.lockOnTarget then this.lockOnTarget.isLockable = false end
    local lockedPlayer = find(PlayerName)
    if lockedPlayer and lockedPlayer.lockOn then lockedPlayer.lockOn:clear() end

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

    this.world:burst("Summon Burst", place + vec3(0, 1.0, 0), 160)
    this.world:shakeCamera(0.9)
    this.world:slowMotion(1.5, 0.3)
    flash = 0.6

    this.world:notify("WITH THIS TREASURE, I SUMMON...")
    -- The seal burns behind his back from now on
    O.showSeal(1)
    -- A look at what came: low before the monster as it roars, its title over it
    -- (it faces the player first: the shot is in front of it)
    if player then summon.transform.rotation = vec3(0, O.yawToward(place, player.transform.position) + 180, 0) end
    after(0.3, function() playCutscene(O.SummonRevealCutscene) end)
end

-- A breath between the great parts of the fight: the player's champagne is full again
function O.refillFlasks()
    local player = find(PlayerName)
    local flask = player and player.healingFlask
    if not flask or flask.charges >= flask.maxCharges then return end
    flask.charges = flask.maxCharges
    this.world:notify("CHAMPAGNE REFILLED")
end

local function startLastPhase()
    phase = 4
    if this.lockOnTarget then this.lockOnTarget.isLockable = true end
    O.refillFlasks()
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

-- ========== The fight in the air (film 0:40-0:50) ==========

-- Once in the first phase: both rise into the air facing each other; he makes his sign and shoots his stars, the
-- player forms a ball of baby oil over his palm and throws it; they meet in the middle in a blast; both come down.
-- (off: the user found it random, out of nowhere at the start of the fight)
O.AirFight = { IsOn = false, At = 10.5, Height = 4.5, Distance = 11, RiseSeconds = 1.2,
               Shots = { "Air Rise", "Air Diddy", "Air Clash" }, Seconds = { 1.6, 2.0, 1.8 } }

function O.startAirFight()
    local player = find(PlayerName)
    if not player then return end
    O.hasFoughtInAir = true
    isChanneling = true
    this.enemy:changeState("Channel")
    becomeInvulnerable(8)
    local A = O.AirFight

    local from = this.transform.position
    local toPlayer = player.transform.position - from
    toPlayer.y = 0
    local along = toPlayer:length() > 0.5 and toPlayer:normalized() or vec3(0, 0, 1)
    local mid = clampToArena(from + toPlayer * 0.5)
    local bossGround = clampToArena(mid - along * (A.Distance * 0.5))
    local playerGround = clampToArena(mid + along * (A.Distance * 0.5))
    bossGround.y, playerGround.y = 0, 0
    local up = vec3(0, A.Height, 0)
    local risen = 0

    local function face()
        local b, p = this.transform.position, player.transform.position
        this.transform.rotation = vec3(0, O.yawToward(b, p) + 180, 0)
        player.transform.rotation = vec3(0, O.yawToward(p, b) + 180, 0)
    end
    -- (in the air nothing pulls them down while the scene holds them there)
    local function float(isOn)
        for _, who in ipairs({ this, player }) do
            if who.physicsBody then
                who.physicsBody.useGravity = not isOn
                who.physicsBody.velocity = vec3(0, 0, 0)
            end
        end
    end
    float(true)
    local function hold(deltaTime)
        -- (the floating clips again when they ended: both stay in the air until they come down)
        if not this.characterActions.isBusy then this.characterActions:play("Hover") end
        if player.characterActions and not player.characterActions.isBusy then O.playPlayer("Fly_Orb") end
        risen = math.min(risen + deltaTime / A.RiseSeconds, 1)
        local e = risen * risen * (3 - 2 * risen)
        local bob = math.sin(seconds * 2.5) * 0.12
        this.transform.position = bossGround + up * e + vec3(0, bob, 0)
        player.transform.position = playerGround + up * e + vec3(0, -bob, 0)
        face()
    end

    startShow({
        function()
            this.world:burst(O.SkidDust, bossGround, 20)
            this.world:burst(O.SkidDust, playerGround, 20)
            this.characterActions:play("Hover")
            O.playPlayer("Fly_Orb")
            playCutscene(A.Shots[1])
            show.during = hold
            return A.Seconds[1]
        end,
        -- He makes his sign (film 0:41), the player's ball of oil grows over his palm (0:43)
        function()
            this.characterActions:play("Cast_Stars")
            playCutscene(A.Shots[2])
            show.during = hold
            return A.Seconds[2]
        end,
        -- Both throw: his stars and the ball of oil meet in the middle and burst (0:47-0:50)
        function()
            playCutscene(A.Shots[3])
            local b, p = this.transform.position + vec3(0, 1.4, 0), player.transform.position + vec3(0, 1.4, 0)
            local middle = (b + p) * 0.5
            O.playPlayer("Cast_Orb")
            for i = 1, 5 do
                local turn = math.rad((i - 3) * 6)
                local dir = (p - b):normalized()
                this.world:effect("Technique Ray", b, vec3(dir.x * math.cos(turn) - dir.z * math.sin(turn), 0, dir.x * math.sin(turn) + dir.z * math.cos(turn)))
            end
            after(0.45, function()
                this.world:effect("Solid Oil Burst", middle, (b - p):normalized())
                this.world:effect("Liquid Burst", middle)
                this.world:flash(vec3(1, 0.97, 0.9), 0.12, 0.0)
                this.world:impactFrame(1.0, 0.4)
                this.world:shakeCamera(1.0)
                Audio.play(BoomSound, 1.0)
            end)
            show.during = hold
            return A.Seconds[3]
        end,
        -- Both come down
        function()
            local fall = 0
            local bFrom, pFrom = this.transform.position, player.transform.position
            show.during = function(deltaTime)
                fall = math.min(fall + deltaTime / 0.5, 1)
                this.transform.position = mix(bFrom, bossGround, fall * fall)
                player.transform.position = mix(pFrom, playerGround, fall * fall)
            end
            return 0.5
        end,
        function()
            this.world:burst(O.SkidDust, bossGround, 16)
            this.world:burst(O.SkidDust, playerGround, 16)
            float(false)
            O.restorePlayer()
            isChanneling = false
        end,
    })
end

-- ========== The fist fight in the sky (film 2:11-2:16) ==========

-- Once in his domain: both leap into the clouds and trade blows there; the player mashes [Space] (a quick-time event,
-- a blow of his for each press, him reeling), the last blow sends him down to the ground where he skids
O.SkyBrawl = { At = 95, Height = 75, Gap = 1.35, ArcSide = 5, CircleDegreesPerSecond = 22, DriftRadius = 5, CloudsEvery = 0.45, RiseSeconds = 1.2, FallSeconds = 1.3, Damage = 140, Punches = { "Attack_Light_1", "Attack_Light_2",
               "Attack_Light_3", "Attack_Light_4" } }

function O.startSkyBrawl()
    local player = find(PlayerName)
    if not player then return end
    O.hasSkyBrawled = true
    isChanneling = true
    this.enemy:changeState("Channel")
    becomeInvulnerable(30)
    local B = O.SkyBrawl
    local ground = this.transform.position
    local toPlayer = player.transform.position - ground
    toPlayer.y = 0
    local along = toPlayer:length() > 0.5 and toPlayer:normalized() or vec3(0, 0, 1)
    local playerGround = clampToArena(ground + along * B.Gap)
    playerGround.y = 0
    local up = vec3(0, B.Height, 0)
    local function float(isOn)
        for _, who in ipairs({ this, player }) do
            if who.physicsBody then who.physicsBody.useGravity = not isOn; who.physicsBody.velocity = vec3(0, 0, 0) end
        end
    end
    local function face()
        local b, p = this.transform.position, player.transform.position
        this.transform.rotation = vec3(0, O.yawToward(b, p) + 180, 0)
        player.transform.rotation = vec3(0, O.yawToward(p, b) + 180, 0)
    end
    float(true)
    -- As in the film (2:11-2:16) they are thrown up together in an arc to one side, not lifted straight up, and up
    -- there they fly on through the clouds close to each other, circling as they trade blows (they stood still
    -- facing each other, the camera fixed on them)
    local risen, punch, pushed = 0, 0, 0
    local middle = (ground + playerGround) * 0.5
    local side = vec3(along.z, 0, -along.x)
    local angle = math.atan(along.x, along.z)
    local drift = vec3(0, 0, 0)
    local function inTheSky(deltaTime)
        risen = math.min(risen + deltaTime / B.RiseSeconds, 1)
        local e = 1 - (1 - risen) ^ 3
        local arc = math.sin(risen * math.pi) * B.ArcSide
        angle = angle + math.rad(B.CircleDegreesPerSecond) * deltaTime * risen
        -- (the drift goes round a circle, never far: drifting one way the long brawl left the arena)
        local driftAngle = (angle - math.atan(along.x, along.z)) * 0.6
        drift = vec3(math.sin(driftAngle), 0, math.cos(driftAngle) - 1) * B.DriftRadius
        local center = clampToArena(middle + side * arc + drift) + up * e
        local toPlayer = vec3(math.sin(angle), 0, math.cos(angle)) * (B.Gap * 0.5)
        local sway = math.sin(seconds * 2.3) * 0.25
        this.transform.position = center - toPlayer + vec3(0, sway, 0)
        player.transform.position = center + toPlayer + vec3(0, -sway * 0.6, 0)
        face()
    end

    startShow({
        function()
            this.world:burst(O.SkidDust, ground, 30)
            this.world:effect("Crash Impact", ground, this.transform.forward)
            this.world:shakeCamera(0.8)
            -- (both leap off the ground, the player already swinging)
            O.playPlayer(B.Punches[1])
            this.characterActions:play("Hit")
            playCutscene("Sky Leap")
            show.during = inTheSky
            return 1.2
        end,
        -- The brawl: held until the player has struck enough (a blow and a reel for every press)
        function()
            playCutscene("Sky Brawl")
            local cloudsIn = 0
            show.during = function(deltaTime)
                inTheSky(deltaTime)
                -- Clouds drift under and past them the whole time (the film: they fight among the clouds)
                cloudsIn = cloudsIn - deltaTime
                if cloudsIn <= 0 then
                    cloudsIn = B.CloudsEvery
                    local mid = (this.transform.position + player.transform.position) * 0.5
                    local a = math.random() * math.pi * 2
                    local r = 5 + math.random() * 7
                    this.world:effect("Sky Clouds", mid + vec3(math.cos(a) * r, -2.5 - math.random() * 3, math.sin(a) * r),
                        this.transform.forward)
                    this.world:effect("Sky Clouds", mid + vec3(0, -4.0, 0), this.transform.forward)
                end
                -- (an exchange, not a beating: every third press he blocks and strikes back, the player reels)
                pushed = math.max(pushed - deltaTime * 3, 0)
                local away = this.transform.position - player.transform.position
                away.y = 0
                if away:length() > 0.01 then this.transform.position = this.transform.position + away:normalized() * pushed end
                if Input.isKeyDown(Key.Space) then
                    punch = punch + 1
                    local at = (this.transform.position + player.transform.position) * 0.5 + vec3(0, 1.4, 0)
                    if punch % 3 == 0 then
                        this.characterActions:play(B.Punches[(punch % #B.Punches) + 1])
                        O.playPlayer("Hit")
                        this.world:effect("Punch Impact", at, -player.transform.forward)
                        this.world:shakeCamera(0.35)
                    else
                        O.playPlayer(B.Punches[(punch % #B.Punches) + 1])
                        this.characterActions:play("Hit")
                        this.world:effect("Punch Impact", at, player.transform.forward)
                        this.world:impactFrame(0.5, 0.15)
                        this.world:shakeCamera(0.25)
                        pushed = 0.6
                    end
                end
                local shot = find("Sky Brawl")
                if shot and shot.cutscene and shot.cutscene.wasAnswered then show.waitLeft = 0 end
            end
            return 999
        end,
        -- The last blow: he is hurled down to the ground
        function()
            O.playPlayer("Attack_Heavy")
            this.characterActions:play("Knock_Flight")
            this.world:flash(vec3(1, 0.95, 0.85), 0.15, 0.0)
            this.world:impactFrame(1.0, 0.5)
            this.world:shakeCamera(1.0)
            Audio.play(BoomSound, 1.0)
            playCutscene("Sky Fall")
            local fall = 0
            local from, pFrom = this.transform.position, player.transform.position
            local land = clampToArena(ground - along * 6)
            land.y = 0
            show.during = function(deltaTime)
                fall = math.min(fall + deltaTime / B.FallSeconds, 1)
                local across = mix(from, land, fall)
                this.transform.position = vec3(across.x, from.y * (1 - fall * fall), across.z)
                player.transform.position = mix(pFrom, playerGround, fall * fall)
            end
            return B.FallSeconds
        end,
        -- He crashes into the ground and skids
        function()
            this.world:effect("Crash Impact", this.transform.position, this.transform.forward)
            this.world:burst(O.SkidDust, this.transform.position, 30)
            this.world:shakeCamera(0.9)
            this.characterActions:play("Skid_Crouch")
            float(false)
            invulnerableLeft = 0
            this.health.isInvulnerable = false
            this.health:damage(O.SkyBrawl.Damage)
            return 0.8
        end,
        function()
            O.restorePlayer()
            isChanneling = false
        end,
    })
end

-- After a staged moment, and whenever the fight starts over: the player falls again and acts again
function O.restorePlayer()
    local player = find(PlayerName)
    if not player then return end
    if player.physicsBody then player.physicsBody.useGravity = true end
    if this.physicsBody then this.physicsBody.useGravity = true end
end

-- ========== The stand-off (film 0:58-1:05) ==========

-- The fight stops for a breath: over his shoulder at the player across the street, both pacing; the other way, over
-- the player's shoulder at him; close on his face as he smiles; and both run at each other - then the fight goes on.
-- Once, at this second of the soundtrack (the film's 0:58), in the first phase
-- (off: the user found it random and it broke the flow of the fight - a game, not a film)
O.Standoff = { IsOn = false, At = 21.8, Distance = 12, Shots = { "Standoff Boss Side", "Standoff Player Side", "Standoff Smile",
               "Standoff Charge" }, Seconds = { 2.8, 2.0, 1.6, 1.3 }, ChargeSpeed = 7.5, ClashGap = 1.7 }

function O.startStandoff()
    local player = find(PlayerName)
    if not player then return end
    O.hasStoodOff = true
    isChanneling = true
    this.enemy:changeState("Channel")
    becomeInvulnerable(9)

    -- Across the street from each other, face to face
    local from = this.transform.position
    local toPlayer = player.transform.position - from
    toPlayer.y = 0
    local along = toPlayer:length() > 0.5 and toPlayer:normalized() or vec3(0, 0, 1)
    local mid = clampToArena(from + toPlayer * 0.5)
    local bossAt = clampToArena(mid - along * (O.Standoff.Distance * 0.5))
    local playerAt = clampToArena(mid + along * (O.Standoff.Distance * 0.5))
    local S = O.Standoff

    local function face()
        local b, p = this.transform.position, player.transform.position
        this.transform.rotation = vec3(0, O.yawToward(b, p) + 180, 0)
        player.transform.rotation = vec3(0, O.yawToward(p, b) + 180, 0)
    end

    startShow({
        function()
            this.transform.position = vec3(bossAt.x, from.y, bossAt.z)
            player.transform.position = vec3(playerAt.x, player.transform.position.y, playerAt.z)
            face()
            this.characterActions:play("Strafe_Left")
            O.playPlayer("Strafe_Left")
            playCutscene(S.Shots[1])
            return S.Seconds[1]
        end,
        function()
            face()
            playCutscene(S.Shots[2])
            return S.Seconds[2]
        end,
        function()
            this.characterActions:play("Idle")
            playCutscene(S.Shots[3])
            O.tween(function(v) if this.model then this.model.expression = v end end, 0, O.SmileAmount, 0.7)
            return S.Seconds[3]
        end,
        -- Both run at each other (film 1:04-1:05)
        function()
            if this.model then this.model.expression = 0 end
            face()
            this.characterActions:play("Run")
            O.playPlayer("Run")
            playCutscene(S.Shots[4])
            local hasClashed = false
            show.during = function(deltaTime)
                local b, p = this.transform.position, player.transform.position
                local between = p - b
                between.y = 0
                if between:length() > S.ClashGap then
                    local step = between:normalized() * math.min(S.ChargeSpeed * deltaTime, (between:length() - S.ClashGap) * 0.5)
                    this.transform.position = b + step
                    player.transform.position = p - step
                elseif not hasClashed then
                    -- (they meet: fist against fist, a blast between them - not walking into each other)
                    hasClashed = true
                    this.characterActions:play("Attack_Light_2")
                    O.playPlayer("Attack_Light_2")
                    after(0.25, function()
                        local mid = (this.transform.position + player.transform.position) * 0.5 + vec3(0, 1.4, 0)
                        this.world:effect("Punch Impact", mid, between:normalized())
                        this.world:impactFrame(0.8, 0.3)
                        this.world:shakeCamera(0.7)
                        Audio.play(BoomSound, 0.8)
                    end)
                end
            end
            return S.Seconds[4] + 0.4
        end,
        function()
            isChanneling = false
            O.restorePlayer()
        end,
    })
end

local function updatePhases()
    -- The domain opens when the player has hurt him enough (or, at the latest, late in the soundtrack): the
    -- soundtrack is moved to the film's moment of that blow, so the picture and the sound stay together
    if phase == 2 and not O.hasSkyBrawled and clock() >= O.SkyBrawl.At and domainIsUp and isFreeToAct() then
        O.startSkyBrawl()
        return
    end
    if O.AirFight.IsOn and phase == 1 and not O.hasFoughtInAir and not O.wantsKnockback and clock() >= O.AirFight.At and isFreeToAct() then
        O.startAirFight()
        return
    end
    if O.Standoff.IsOn and phase == 1 and not O.hasStoodOff and not O.wantsKnockback and clock() >= O.Standoff.At and isFreeToAct() then
        O.startStandoff()
        return
    end
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
    -- (died in his armored phases: the next try starts there - the fight before it is long, and was already won)
    if phase >= 5 then O.resumeArmored = true end
    O.hasStoodOff, O.hasFoughtInAir, O.hasSkyBrawled = false, false, false
    if O.restorePlayer then O.restorePlayer() end
    if this.lockOnTarget then this.lockOnTarget.isLockable = true end
    phase = 0
    seconds = 0
    invulnerableLeft = 0
    domainIsUp = false
    if this.model then this.model.expression = 0 end
    if O.BaseTitle and this.enemyHealthBar then this.enemyHealthBar.displayName = O.BaseTitle end
    domainBrokenLeft = 0
    isChanneling = false
    hasAnnouncedSummon = false
    flash = 0
    playerDomainCasts = 0

    this.meleeEnemy.attackCooldownSeconds = O.BrawlCooldown or 0.6
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
    O.holdingForTechnique, O.starsWaitForTechnique, O.starWave = false, false, nil
    O.starsIn = O.Stars and O.Stars.FirstAfter or 9
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

        -- (while his summon fights he only watches: a monster in front and money from behind left no room to act)
        do return end
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

-- The bottle from the sky become oil (film 2:28-2:30): a capsule of oil the shape of the bottle for a moment, swelling
-- into a ball, and the ball bursting like a liquid - seconds after it landed; sizes in meters (the bottle is ~5 m tall)
O.SwordFightCameraDistance = 5.6
O.SwordCamera = { Shoulder = -0.75, Pitch = 9 }
O.BrawlCooldown = 0.6
O.CubeBurstsAfter = 1.15
O.Liquid = { Entity = "Detonation Oil", CapsuleSeconds = 0.35, Capsule = vec3(2.2, 4.6, 2.2), Ball = 5.2,
             Effect = "Liquid Burst" }
O.PlayerDomainCrashAt = 4.0

-- His lines from the film (the music no longer carries them)
O.VoiceDomain    = "@game/Sounds/Voice/epstein_domain_expansion.ogg"
O.VoiceBetter    = "@game/Sounds/Voice/epstein_better_than_expected.ogg"
-- The film's pictures over his two lines before the summoning; meanwhile he rises up and back over this long, this far
O.FloatCutscene, O.FloatRiseSeconds, O.FloatBackMeters = "Summon Float", 4.5, 6
O.LeapAwayCutscene, O.SummonRevealCutscene = "Summon Leap Away", "Summon Reveal"
O.VoiceTreasure  = "@game/Sounds/Voice/epstein_treasure_summon.ogg"
O.VoicePresident = "@game/Sounds/Voice/epstein_president_trump.ogg"
O.VoiceVolume    = 1.5
-- The film's own sound (voices, music, blasts) for the staged moments: from the blow before his domain (film 1:14.8)
-- to the player lying down, and the lines before his summoning (2:30.6-2:41). The music goes silent under it (it
-- keeps its clock); as loud as the music (the film's mix is louder)
O.DomainFilmAudio, O.DomainFilmAudioSeconds = "@game/Sounds/Film/domain_opening_film.ogg", 27.8
O.SummonFilmAudio, O.SummonFilmAudioSeconds = "@game/Sounds/Film/summoning_film.ogg", 10.4
O.SummonFloatVideoAfter, O.SummonWheelAfter = 0.8, 6.0
O.FilmAudioVolume = 0.55
function O.playFilmAudio(sound, seconds)
    Audio.setMusicDucking(0)
    Audio.playVoice(sound, O.FilmAudioVolume)
    after(seconds + 0.5, function() Audio.setMusicDucking(0.35) end)
end

-- While the player fights his summon up close he only watches: his storms of money are for a player who runs from it
-- (further than this from the summon), so the summon is never fought with a rain of money in the back
-- (far enough that a player fighting the summon - rolling around it, backing off to drink - is never shot at)
O.ChannelHoldsWithinMeters = 13

-- ========== Called by the engine ==========

-- A hit as hard as the bottle from the sky ends in a blast of white, as in the film
function OnDamaged(amount, kind, attacker)
    if kind == "weapon" then O.notePressure() end
    if phase == 1 and not O.opening and this.health.fraction <= DomainHealthFraction then
        O.wantsKnockback = true
    end
end

-- The bottle from the sky landed (wherever: hurt or not, the boss may be out of reach of harm then): it turns into
-- the oil itself and bursts
function O.watchBottle()
    local bottle = find("Player Great Orb")
    local projectile = bottle and bottle.projectile
    if not projectile then return end
    local burst = projectile.timesBurst
    if burst > (O.bottleBursts or 0) then
        local at = projectile.lastBurstPosition + vec3(0, 1.4, 0)
        O.liquid = { t = 0, at = at }
        this.world:slowMotion(0.9, 0.3)
        this.world:shakeCamera(0.4)
        after(O.CubeBurstsAfter, function()
            O.liquid = nil
            local oil = find(O.Liquid.Entity)
            if oil and oil.liquidOrb then oil.liquidOrb.isVisible = false end
            this.world:effect(O.Liquid.Effect, at)
            this.world:flash(vec3(1, 1, 0.95), 0.12, 0.0)
            this.world:impactFrame(1.0, 0.55)
            this.world:shakeCamera(0.9)
            Audio.play(BoomSound, 1.0)
            O.startStarWave(at)
        end)
    end
    O.bottleBursts = burst
end

-- The blast runs over the ground as a wave and the stars stuck in it burst one after another as it reaches them
-- (it is his answer that breaks them: the player sees them go from the blast outwards)
O.StarWave = { Speed = 16, Farthest = 50, Effect = "Solid Oil Burst", EffectEvery = 0.08, Sparks = "Star Impact" }
-- As in the film (1:37.5-1:39.5) he runs at the boss into the falling stars, until they throw him off his feet
-- (he stood still waiting for them; the camera of the shot is his own child and runs with him)
O.RunIntoStars = { Meters = 7, Seconds = 1.6 }
function O.runIntoStars()
    local player = O.playerEntity()
    if not player then return end
    local from = player.transform.position
    local toBoss = this.transform.position - from
    toBoss.y = 0
    if toBoss:length() < 2 then return end
    local along = toBoss:normalized()
    local meters = math.min(O.RunIntoStars.Meters, toBoss:length() - 3)
    player.transform.rotation = vec3(0, O.yawToward(from, this.transform.position) + 180, 0)
    -- (carried by the script his body doesn't move by itself: running is his standing clip for the run)
    local animation = player.characterAnimation
    if animation then
        local idles = { animation.idleAnimation, animation.lockedOnIdleAnimation }
        animation.idleAnimation, animation.lockedOnIdleAnimation = "Run", "Run"
        after(O.RunIntoStars.Seconds, function()
            animation.idleAnimation, animation.lockedOnIdleAnimation = idles[1], idles[2]
        end)
    end
    O.tween(function(v)
        if O.playerEntity() then
            local at = clampToArena(from + along * v)
            player.transform.position = vec3(at.x, from.y, at.z)
        end
    end, 0, meters, O.RunIntoStars.Seconds)
end

function O.startStarWave(at)
    local stars = O.stars()
    if not stars or not stars.isVisible then return end
    O.starWave = { center = vec3(at.x, 0, at.z), radius = 0, effectIn = 0, resetAfter = O.starsAwaitWave }
    O.starsAwaitWave = false
    this.world:effect("Oil Circle", vec3(at.x, 0.05, at.z), this.transform.forward)
end

function O.updateStarWave(deltaTime)
    local w = O.starWave
    if not w then return end
    local stars = O.stars()
    if not stars then O.starWave = nil; return end
    w.radius = w.radius + O.StarWave.Speed * deltaTime
    w.effectIn = w.effectIn - deltaTime
    for _, at in ipairs(stars:breakWithin(w.center, w.radius)) do
        this.world:burst(O.StarWave.Sparks, at + vec3(0, 0.3, 0), 3)
        -- (a burst of oil on as many of them as the eye can follow, sparks on all)
        if w.effectIn <= 0 then
            w.effectIn = O.StarWave.EffectEvery
            this.world:effect(O.StarWave.Effect, at + vec3(0, 0.4, 0), this.transform.forward)
        end
    end
    if w.radius >= O.StarWave.Farthest then
        O.starWave = nil
        if w.resetAfter then stars:reset(); stars.isVisible = false end
    end
end

function OnStart()
    this.enemy:addState("Guard", GuardState)
    this.enemy:addState("Channel", ChannelState)
    reset()
end

-- The oil of the bottle: the bottle's shape for a moment, then swelling into a ball that trembles harder and burns
-- brighter until it bursts (see OnDamaged)
function O.updateLiquid(deltaTime)
    local l = O.liquid
    if not l then return end
    local oil = find(O.Liquid.Entity)
    if not oil or not oil.liquidOrb then return end
    l.t = l.t + deltaTime
    oil.liquidOrb.isVisible = true
    oil.transform.position = l.at
    local L = O.Liquid
    local size
    if l.t < L.CapsuleSeconds then
        size = L.Capsule
    else
        local k = math.min((l.t - L.CapsuleSeconds) / (O.CubeBurstsAfter - L.CapsuleSeconds), 1)
        local e = k * k * (3 - 2 * k)
        local ball = vec3(L.Ball, L.Ball, L.Ball)
        size = L.Capsule + (ball - L.Capsule) * e
        oil.liquidOrb.wobble = 0.1 + 0.25 * k
        oil.liquidOrb.brightness = 2.4 + 3.0 * k * k
    end
    oil.transform.scale = size
end

-- Against his great sword the camera stands lower and over the player's left shoulder (his sword is in the right hand: the camera caught on it): from right behind and
-- above him the armored boss stood hidden behind the player's back for most of the fight
function O.updateSwordCamera(deltaTime)
    local camera = find(CameraName)
    if not camera or not camera.followCamera then return end
    local follow = camera.followCamera
    -- (in his armor he stands taller with a great sword: farther back, he was hidden behind the player)
    if phase >= 5 then
        local step = CameraZoomPerSecond * deltaTime
        follow.distance = follow.distance + math.max(-step, math.min(step, O.SwordFightCameraDistance - follow.distance))
    end
    if not O.cameraHome then O.cameraHome = { follow.shoulderOffset, follow.lockOnPitchDegrees } end
    local isSwordFight = phase >= 5
    local shoulder = isSwordFight and O.SwordCamera.Shoulder or O.cameraHome[1]
    local pitch = isSwordFight and O.SwordCamera.Pitch or O.cameraHome[2]
    local glide = math.min(deltaTime * 1.5, 1)
    follow.shoulderOffset = follow.shoulderOffset + (shoulder - follow.shoulderOffset) * glide
    follow.lockOnPitchDegrees = follow.lockOnPitchDegrees + (pitch - follow.lockOnPitchDegrees) * glide
end

function OnUpdate(deltaTime)
    local boss = this.boss
    O.updateSeal(deltaTime)
    O.watchBottle()
    O.updateLiquid(deltaTime)
    O.updateStarWave(deltaTime)
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
        -- (a staged moment's steps are timed to the voices: they go on under the film's pictures)
        if show then updateShow(deltaTime) end
        if phase > 0 then
            updateOpening()
            O.updateRise(deltaTime)
            O.holdForTechnique()
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
        O.holdForTechnique()
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
    O.updateSwordCamera(deltaTime)

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
    O.updateStarVolley(deltaTime)
    updateLines(deltaTime)
    updateRain(deltaTime)
    watchJetSpells()
    updateGamble()
    O.updateAscension(deltaTime)
    O.updateVessel(deltaTime)
    -- (his sword waits while the player answers)
    if not O.vessel then O.updateArmored(deltaTime) end
    updateDomain(deltaTime)
    updateTweens(deltaTime)
    O.updateRise(deltaTime)
    O.holdForTechnique()
    if not O.lyingUntilPress and not promptDetonation(deltaTime) then promptPlayerDomain(deltaTime) end
    updateLook(deltaTime)
    updateTemple(deltaTime)
end
