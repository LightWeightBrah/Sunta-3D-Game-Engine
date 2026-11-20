-- How the player's techniques look, as in the film: his eyes glow yellow while he calls one, a ball of baby oil
-- gathers over his palm before he throws it, and his own domain lights him up from inside before the white blast
--
-- Put on the player. Everything it uses is general: Glow Sprites on the head bone (the eyes), a Liquid Orb on the
-- hand bone, the Model's 'glow', a flash of the screen, and what the Spell Casters tell about themselves

local OrbSpell    = "Player Orb Spell"
local GreatSpell  = "Player Great Orb Spell"
local DomainSpell = "Player Domain Spell"
local HeldOrb     = "Player Held Orb"
local BottleFall  = "Bottle Fall"
local Eyes        = { "Player Eye R", "Player Eye L" }

-- The eyes stay lit this long after a technique left his hands
local EyesStaySeconds = 0.9
local EyeIntensity    = 9

-- The ball over his palm grows to this size (meters across) by the time it is thrown
local HeldOrbSize = 0.5

-- His domain: the light he gives off at the height of the sign, and the blast when it opens
-- (a warm sheen on him, not a figure of light: his eyes are what burns)
local DomainGlow         = vec3(0.0, 0.0, 0.0)
local DomainFlashSeconds = 1.3

-- He calls each technique by its name, in his own voice (cut from the film's soundtrack: the voices alone, the music
-- taken out of them by a separation of the sources - see Sounds/CREDITS.md)
local Voices = {
    [OrbSpell]    = "@game/Sounds/Voice/solid_baby_oil.ogg",
    [DomainSpell] = "@game/Sounds/Voice/divine_baby_oil_evolve.ogg",
    [GreatSpell]  = "@game/Sounds/Voice/baby_oil_detonation.ogg",
}
local VoiceVolume = 1.3

local eyesLeft = 0
local wasCasting = {}
local releasedBefore = {}
local domainGlow = 0

local function find(name) return this.world:find(name) end

local function casterOf(name)
    local spell = find(name)
    return spell and spell.spellCaster or nil
end

-- True in the frame the spell's projectiles left
local function hasJustReleased(name)
    local caster = casterOf(name)
    if not caster then return false end

    local released = caster.timesReleased
    local hasJust = released > (releasedBefore[name] or 0)
    releasedBefore[name] = released
    return hasJust
end

local function isCasting(name)
    local caster = casterOf(name)
    return caster ~= nil and caster.isCasting
end

local function setEyes(intensity)
    for _, name in ipairs(Eyes) do
        local eye = find(name)
        if eye and eye.glowSprite then
            eye.glowSprite.isVisible = intensity > 0.01
            eye.glowSprite.intensity = intensity
        end
    end
end

-- (the steps of his domain while it plays, see updateDomain below: his eyes keep burning until the waves meet)
local domainShow = nil
local function updateEyes(deltaTime)
    if isCasting(OrbSpell) or isCasting(GreatSpell) or isCasting(DomainSpell) or (domainShow and not domainShow.crashed) then
        eyesLeft = EyesStaySeconds
    else
        eyesLeft = math.max(eyesLeft - deltaTime, 0)
    end

    setEyes(EyeIntensity * math.min(eyesLeft / EyesStaySeconds * 2, 1))
end

-- The oil gathers over his palm while he winds up, and is gone from the hand the moment it flies
local heldGrowth = 0
local function updateHeldOrb(deltaTime)
    local held = find(HeldOrb)
    if not held or not held.liquidOrb then return end

    local caster = casterOf(OrbSpell)
    local isHolding = caster ~= nil and caster.isCasting and not hasJustReleased(OrbSpell) and heldGrowth >= 0

    if caster and caster.isCasting and heldGrowth < 0 then
        -- thrown already in this cast
        isHolding = false
    elseif not (caster and caster.isCasting) then
        heldGrowth = 0
    end

    if isHolding then
        heldGrowth = math.min(heldGrowth + deltaTime * 2.5, 1)
        held.transform.scale = vec3(1, 1, 1) * (HeldOrbSize * heldGrowth)
    elseif caster and caster.isCasting then
        heldGrowth = -1
    end

    held.liquidOrb.isVisible = isHolding and heldGrowth > 0.05
end

-- His guard is a wall of baby oil: it swells in front of him while he blocks, and flares up (bigger, brighter) for
-- the moment his parry turns blows away - the player sees when the parry is live
local OilGuard = "Player Oil Guard"
local GuardSize = vec3(1.0, 1.15, 0.22)
local guardAmount, parryFlare = 0, 0
local function updateOilGuard(deltaTime)
    local guard = find(OilGuard)
    local actions = this.characterActions
    if not guard or not guard.liquidOrb or not actions then return end

    local target = actions.isGuarding and 1 or 0
    guardAmount = guardAmount + (target - guardAmount) * math.min(deltaTime * 14, 1)
    if actions.isParrying then parryFlare = 1 else parryFlare = math.max(parryFlare - deltaTime * 4, 0) end

    local amount = math.max(guardAmount, parryFlare)
    local size = 0.6 + 0.4 * amount + 0.35 * parryFlare
    guard.transform.scale = vec3(GuardSize.x * size, GuardSize.y * size, GuardSize.z * size)
    guard.liquidOrb.brightness = 0.5 + 1.3 * parryFlare
    guard.liquidOrb.isVisible = amount > 0.05
end

-- His own domain, Divine Baby Oil Evolve, as in the film (2:19-2:26): close on his clasped hands and his glowing
-- eyes; a huge whirl of baby oil around him that grows; oil all over the floor of the arena; then two long waves of
-- it rise on both sides of the enemy and crash together on him (the blast of the spell lands then: its fuse)
local Domain = {
    GestureShot = "Domain Gesture", VortexShot = "Domain Vortex", WavesShot = "Domain Waves",
    Vortex = "Oil Vortex", Crash = "Oil Wave Crash",
    Pools = 9, PoolPrefix = "Oil Pool ",
    -- How big each pool grows (meters across; the same list as OIL_POOLS of Tools/build_scene.py) and how deep
    PoolSizes = { 34, 18, 20, 19, 18, 16, 18, 14, 14 }, PoolDepth = 0.05,
    PoolGrowSeconds = 2.4, PoolStaySeconds = 14, PoolSinkSeconds = 2.5,
    -- The waves: when they rise (seconds after the spell left), when they meet (PLAYER_DOMAIN_CRASH_AT of the
    -- scene: the blast's fuse), how far to each side of the enemy they start, how big they are
    WavesAt = 2.5, WaveCrashAt = 4.0, WaveFrom = 12, WaveLength = 12, WaveHeight = 3.4, WaveThickness = 1.6,
    -- The crests curling over along each wave, how often
    Crest = "Oil Wave Crest", CrestEverySeconds = 0.12,
    BoomSound = "@game/Sounds/Spells/deep_boom.ogg", SplashSound = "@game/Sounds/Spells/glass_break.ogg",
}

local function playShot(name)
    local shot = find(name)
    if shot and shot.cutscene then shot.cutscene:play() end
end

local function setPools(grown)
    for i = 1, Domain.Pools do
        local pool = find(Domain.PoolPrefix .. i)
        if pool and pool.liquidOrb then
            local size = Domain.PoolSizes[i] * math.max(grown, 0.001)
            pool.transform.scale = vec3(size, Domain.PoolDepth * math.max(grown, 0.05), size * 0.82)
            pool.liquidOrb.isVisible = grown > 0.01
        end
    end
end

local function setWave(side, place, yaw, grown)
    local wave = find("Oil Wave " .. side)
    if not wave or not wave.liquidOrb then return end
    wave.transform.position = place
    wave.transform.rotation = vec3(0, yaw, 0)
    local k = math.max(grown, 0.001)
    wave.transform.scale = vec3(Domain.WaveThickness * k, Domain.WaveHeight * k, Domain.WaveLength * k)
    wave.liquidOrb.isVisible = grown > 0.01
    for _, part in ipairs({ " Spray", " Foam" }) do
        local emitter = find("Oil Wave " .. side .. part)
        if emitter and emitter.particles then emitter.particles.isEmitting = grown > 0.2 end
    end
end

-- The enemy the waves are for: the boss (or the summon fighting for him, when it is closer)
local function domainTarget()
    local best, bestDistance = nil, 1e9
    for _, name in ipairs({ "Boss", "Summon" }) do
        local enemy = find(name)
        if enemy and enemy.health and not enemy.health.isDead and not (enemy.enemy and enemy.enemy.isDormant) then
            local d = (enemy.transform.position - this.transform.position):length()
            if d < bestDistance then best, bestDistance = enemy, d end
        end
    end
    return best
end

local function startDomainShow()
    local target = domainTarget()
    local center = target and target.transform.position or (this.transform.position + this.transform.forward * 8)
    local along = center - this.transform.position
    along.y = 0
    if along:length() < 0.1 then along = this.transform.forward end
    along = along:normalized()
    domainShow = { t = 0, center = center, side = vec3(along.z, 0, -along.x), yaw = math.deg(math.atan(along.x, along.z)),
                   wavesStarted = false, crashed = false, poolsLeft = nil }

    this.world:effect(Domain.Vortex, this.transform.position, this.transform.forward)
    this.world:shakeCamera(0.6)
    Audio.play(Domain.BoomSound, 0.9)
    playShot(Domain.VortexShot)
end

local function updateDomainShow(deltaTime)
    local show = domainShow
    if not show then return end
    show.t = show.t + deltaTime

    -- The oil spreads over the floor while the whirl grows
    if show.t <= Domain.PoolGrowSeconds then
        local k = show.t / Domain.PoolGrowSeconds
        setPools(k * k * (3 - 2 * k))
    elseif not show.poolsLeft then
        setPools(1)
    end

    -- The waves rise on both sides of the enemy and run at him
    if show.t >= Domain.WavesAt and show.t < Domain.WaveCrashAt then
        if not show.wavesStarted then
            show.wavesStarted = true
            playShot(Domain.WavesShot)
            Audio.play(Domain.BoomSound, 0.7)
        end
        local k = (show.t - Domain.WavesAt) / (Domain.WaveCrashAt - Domain.WavesAt)
        local rise = math.min(k * 3, 1)
        local distance = Domain.WaveFrom * (1 - k * k) + 1.2
        show.crestIn = (show.crestIn or 0) - deltaTime
        local along = vec3(-show.side.z, 0, show.side.x)
        for _, entry in ipairs({ { "L", -1 }, { "R", 1 } }) do
            local place = show.center + show.side * (entry[2] * distance)
            place.y = -Domain.WaveHeight * 0.25 * (1 - rise)
            setWave(entry[1], place, show.yaw, rise)
            -- its crest curls over towards the enemy, all along it
            if show.crestIn <= 0 and rise > 0.5 then
                for _, offset in ipairs({ -4.5, -1.5, 1.5, 4.5 }) do
                    local crest = place + along * offset + vec3(0, Domain.WaveHeight * 0.55, 0)
                    this.world:effect(Domain.Crest, crest, show.side * (-entry[2]))
                end
            end
        end
        if show.crestIn <= 0 then show.crestIn = Domain.CrestEverySeconds end
        this.world:shakeCamera(0.08)
    end

    -- They meet on him: a burst of oil, and they are gone
    if show.t >= Domain.WaveCrashAt and not show.crashed then
        show.crashed = true
        setWave("L", show.center, show.yaw, 0)
        setWave("R", show.center, show.yaw, 0)
        this.world:effect(Domain.Crash, show.center + vec3(0, 1.2, 0), this.transform.forward)
        this.world:impactFrame(0.8, 0.4)
        this.world:shakeCamera(1.0)
        this.world:slowMotion(0.6, 0.35)
        Audio.play(Domain.SplashSound, 1.2)
        Audio.play(Domain.BoomSound, 1.1)
        show.poolsLeft = Domain.PoolStaySeconds
    end

    -- The oil lies on the floor for a while, then sinks away
    if show.poolsLeft then
        show.poolsLeft = show.poolsLeft - deltaTime
        if show.poolsLeft <= 0 then
            local k = math.min(-show.poolsLeft / Domain.PoolSinkSeconds, 1)
            setPools(1 - k)
            if k >= 1 then domainShow = nil end
        end
    end
end

-- He lights up as he holds the sign (the camera close on his hands and eyes), then his domain opens
local wasDomainCasting = false
local function updateDomain(deltaTime)
    local casting = isCasting(DomainSpell)
    if casting and not wasDomainCasting then playShot(Domain.GestureShot) end
    wasDomainCasting = casting

    if casting then
        domainGlow = math.min(domainGlow + deltaTime * 1.4, 1)
    else
        domainGlow = math.max(domainGlow - deltaTime * 0.6, 0)
    end

    if this.model then this.model.glow = DomainGlow * (domainGlow * domainGlow) end

    if hasJustReleased(DomainSpell) then
        startDomainShow()
    end
    updateDomainShow(deltaTime)
end

-- The name of the technique, the moment he starts calling it
local function speakTechniques()
    for spell, voice in pairs(Voices) do
        local casting = isCasting(spell)
        if casting and not wasCasting[spell] then Audio.play(voice, VoiceVolume) end
        wasCasting[spell] = casting
    end
end

function OnUpdate(deltaTime)
    speakTechniques()
    updateEyes(deltaTime)
    updateHeldOrb(deltaTime)
    updateOilGuard(deltaTime)
    updateDomain(deltaTime)

    -- The bottle of his great technique: the camera looks up at it coming down on the boss (the boss's own script
    -- answers its hit with the white blast)
    if hasJustReleased(GreatSpell) then
        local fall = find(BottleFall)
        if fall and fall.cutscene then fall.cutscene:play() end
    end
end
