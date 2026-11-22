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
local HeldOrbSize = 0.62

-- His domain: the light he gives off at the height of the sign, and the blast when it opens
-- (a warm sheen on him, not a figure of light: his eyes are what burns)
-- (in the film, 2:19, only his eyes burn as his domain opens: a faint warm edge, no more - stronger, against the
-- sky of the low shot he became a gold silhouette)
local DomainRim          = vec3(0.22, 0.14, 0.04)
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
-- The light he gives off while calling Imaginary Technique (see updateGreatShow)
local techniqueGlow = vec3(0, 0, 0)
local techniqueRim = vec3(0, 0, 0)
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
-- (declared here, before the functions that use it: declared further down, the start of the waves set a global
-- of the same name and never let him come down)
local domainAir = nil
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
local flyGrowth = 0
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

    -- (flying in the fight in the air the ball floats over his palm the whole time: the boss's script stages it)
    local actions = this.characterActions
    if actions and actions.animation == "Fly_Orb" then
        flyGrowth = math.min((flyGrowth or 0) + deltaTime * 1.2, 1)
        held.transform.scale = vec3(1, 1, 1) * (HeldOrbSize * 0.75 * flyGrowth)
        held.liquidOrb.isVisible = true
        return
    end
    flyGrowth = 0

    held.liquidOrb.isVisible = isHolding and heldGrowth > 0.05
end

-- His guard is a wall of baby oil: it swells in front of him while he blocks, and flares up (bigger, brighter) for
-- the moment his parry turns blows away - the player sees when the parry is live
local OilGuard = "Player Oil Guard"
-- (a thick lens, not a flat disc: flattened that far, the ball of liquid showed only a ragged rim from the front)
local GuardSize = vec3(1.0, 1.15, 0.5)
local guardAmount, parryFlare = 0, 0
local function updateOilGuard(deltaTime)
    local guard = find(OilGuard)
    local actions = this.characterActions
    if not guard or not guard.liquidOrb or not actions then return end

    -- (with his sword of oil he blocks with the blade: no wall of oil)
    local hasSword = this.model and (this.model.modelName == "sih_player_sword" or this.model.modelName == "sih_player_armored")
    local target = (actions.isGuarding and not hasSword) and 1 or 0
    guardAmount = guardAmount + (target - guardAmount) * math.min(deltaTime * 14, 1)
    if actions.isParrying and not hasSword then parryFlare = 1 else parryFlare = math.max(parryFlare - deltaTime * 4, 0) end

    local amount = math.max(guardAmount, parryFlare)
    local size = 0.6 + 0.4 * amount + 0.35 * parryFlare
    guard.transform.scale = vec3(GuardSize.x * size, GuardSize.y * size, GuardSize.z * size)
    guard.liquidOrb.brightness = 0.9 + 1.2 * parryFlare
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
    WavesAt = 2.5, WaveCrashAt = 4.0, WaveFrom = 12, WaveLength = 14, WaveHeight = 7.0, WaveThickness = 2.4,
    -- (the wave is a ball of oil stretched: half of it under the ground, so 7 is a wall of 3.5 m over the enemy)
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

    -- (he stays up there until the waves have met: see updateDomain)
    -- (the whirl from the ground under him up around him in the air)
    local feet = this.transform.position
    this.world:effect(Domain.Vortex, vec3(feet.x, 0.05, feet.z), this.transform.forward)
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
-- Imaginary Technique: Baby Oil Detonation, as in the film (1:47-1:50): while he calls it a golden light outlines
-- him (his edges, not his whole body: he stays himself inside it), the outline turns white and for a blink as it leaves
-- his hands he is a figure of white light - and as it leaves his hands rays of light shoot out of him in every
-- direction and he bursts with light; then the light dies away
local Great = {
    GoldSeconds = 0.6, WhiteSeconds = 0.4, FadeSeconds = 0.9,
    -- (a sheen over him, not a figure of light: stronger, he became a flat silhouette)
    -- (no pink or violet anywhere: the film's light is gold, then white)
    GoldRim = vec3(1.1, 0.66, 0.16), WhiteRim = vec3(1.3, 1.26, 1.18),
    -- (a warm sheen over him under the outline, and the white of the blink at the release)
    GoldFill = vec3(0.06, 0.035, 0.0), WhiteFill = vec3(0.55, 0.53, 0.5),
    Rays = 16, RaySpreadSeconds = 0.3, RayEffect = "Technique Ray", BurstEffect = "Technique Burst",
}
local greatShow = nil
local greatReleases = 0

local function updateGreatShow(deltaTime)
    local caster = casterOf(GreatSpell)
    if not caster then return end

    if caster.isCasting and not greatShow then
        greatShow = { t = 0, raysLeft = 0, rayIn = 0, fade = nil }
        greatReleases = caster.timesReleased
    end
    if not greatShow then
        techniqueGlow, techniqueRim = vec3(0, 0, 0), vec3(0, 0, 0)
        return
    end

    local show = greatShow
    show.t = show.t + deltaTime
    local center = this.transform.position + vec3(0, 1.1, 0)

    -- It left his hands: the rays and the burst, then the light fades
    if caster.timesReleased > greatReleases then
        greatReleases = caster.timesReleased
        show.raysLeft, show.rayIn = Great.Rays, 0
        show.fade = 0
        -- (the burst lights the world around him: no flash of the whole screen)
        this.world:effect(Great.BurstEffect, center, this.transform.forward)
        this.world:shakeCamera(0.5)
    elseif not caster.isCasting and not show.fade then
        show.fade = 0   -- cut short (a hit, the end of the fight): the light only dies away
    end

    if show.raysLeft > 0 then
        show.rayIn = show.rayIn - deltaTime
        while show.rayIn <= 0 and show.raysLeft > 0 do
            show.raysLeft = show.raysLeft - 1
            show.rayIn = show.rayIn + Great.RaySpreadSeconds / Great.Rays
            this.world:effect(Great.RayEffect, center, this.transform.forward)
        end
    end

    local glow, rim
    if show.fade then
        -- the blink of white light at the release, then the outline dies away
        show.fade = show.fade + deltaTime
        local left = math.max(1 - show.fade / Great.FadeSeconds, 0)
        local blink = math.max(1 - show.fade / 0.25, 0)
        glow = Great.WhiteFill * (blink * blink)
        rim = Great.WhiteRim * (left * left)
        if left <= 0 and show.raysLeft <= 0 then greatShow = nil end
    elseif show.t < Great.GoldSeconds then
        local k = show.t / Great.GoldSeconds
        glow = Great.GoldFill * k
        rim = Great.GoldRim * (k * (2 - k))
    else
        local k = math.min((show.t - Great.GoldSeconds) / Great.WhiteSeconds, 1)
        local s = k * k * (3 - 2 * k)
        glow = Great.GoldFill * (1 - s)
        rim = Great.GoldRim + (Great.WhiteRim - Great.GoldRim) * s
    end
    techniqueGlow, techniqueRim = glow, rim
end

-- As in the film (2:19-2:22) he makes the sign up in the sky: he rises while he calls it, the whirl of oil turns
-- around him up there, and he comes down once the waves have risen
local DomainAir = { Height = 5.0, RiseSeconds = 1.4, StaySeconds = 3.2, FallSeconds = 0.9, AfterCrashSeconds = 0.6,
                    LongestSeconds = 14.0 }
local floatingIdles = nil
local function setFloating(isOn)
    if this.physicsBody then
        this.physicsBody.useGravity = not isOn
        this.physicsBody.velocity = vec3(0, 0, 0)
    end
    -- Hanging in the air he floats calmly (not a boxer's bounce in the sky), his own stance back on the ground
    local animation = this.characterAnimation
    if not animation then return end
    if isOn and not floatingIdles then
        floatingIdles = { animation.idleAnimation, animation.lockedOnIdleAnimation }
        animation.idleAnimation, animation.lockedOnIdleAnimation = "Float", "Float"
    elseif not isOn and floatingIdles then
        animation.idleAnimation, animation.lockedOnIdleAnimation = floatingIdles[1], floatingIdles[2]
        floatingIdles = nil
    end
end
local function updateDomainAir(deltaTime)
    local a = domainAir
    if not a then return end
    a.t = a.t + deltaTime
    local A = DomainAir
    local h
    if not a.releasedAt then
        local k = math.min(a.t / A.RiseSeconds, 1)
        h = A.Height * (1 - (1 - k) * (1 - k))
    else
        local since = a.t - a.releasedAt
        if since < A.StaySeconds then
            h = A.Height + math.sin(a.t * 2.0) * 0.15
        else
            local k = math.min((since - A.StaySeconds) / A.FallSeconds, 1)
            h = A.Height * (1 - k * k)
            if k >= 1 then
                domainAir = nil
                setFloating(false)
            end
        end
    end
    local here = this.transform.position
    this.transform.position = vec3(here.x, a.ground + h, here.z)
end

local wasDomainCasting = false
local function updateDomain(deltaTime)
    local casting = isCasting(DomainSpell)
    if casting and not wasDomainCasting then
        playShot(Domain.GestureShot)
        domainAir = { t = 0, ground = this.transform.position.y }
        setFloating(true)
    end
    wasDomainCasting = casting
    -- Down once the cast is over and the waves have met (the oil left lying on the floor afterwards doesn't keep
    -- him up there: he hung in the sky for as long as it lay); a cast cut short before it left: down at once
    if domainAir and not casting and not domainAir.releasedAt then
        if not domainShow then
            domainAir.releasedAt = domainAir.t - DomainAir.StaySeconds
        elseif domainShow.crashed or domainAir.t > DomainAir.LongestSeconds then
            domainAir.releasedAt = domainAir.t - DomainAir.StaySeconds + DomainAir.AfterCrashSeconds
        end
    end
    updateDomainAir(deltaTime)

    if casting then
        domainGlow = math.min(domainGlow + deltaTime * 1.4, 1)
    else
        domainGlow = math.max(domainGlow - deltaTime * 0.6, 0)
    end

    if this.model then
        this.model.glow = techniqueGlow
        this.model.rimGlow = DomainRim * (domainGlow * domainGlow) + techniqueRim
    end

    if hasJustReleased(DomainSpell) then
        startDomainShow()
    end
    updateDomainShow(deltaTime)
end

-- The name of the technique, the moment he starts calling it
local function speakTechniques()
    for spell, voice in pairs(Voices) do
        local casting = isCasting(spell)
        if casting and not wasCasting[spell] then Audio.playVoice(voice, VoiceVolume) end
        wasCasting[spell] = casting
    end
end

-- Solid Baby Oil thrown in the air (film 0:40-0:46, the fight in the air): [1] in a jump throws the ball of oil from
-- his hand at what he is locked on to (or straight ahead), the jump goes on - no cast of its own, which would end it
local AirThrow = { Projectile = "Player Orb", Hand = vec3(0, 1.35, 0), Ahead = 0.55, AimHeight = 1.2 }
local function updateAirThrow()
    if not Input.isKeyDown(Key.Num1) then return end
    local actions = this.characterActions
    local action = actions and actions.action or ""
    if action ~= "jump" and action ~= "jump_attack" then return end
    local caster = casterOf(OrbSpell)
    if not caster or caster.cooldownLeft > 0 then return end

    local from = this.transform.position + AirThrow.Hand + this.transform.forward * AirThrow.Ahead
    local direction = this.transform.forward
    local lock = this.lockOn
    if lock and lock.hasTarget then
        direction = (lock.target.transform.position + vec3(0, AirThrow.AimHeight, 0) - from):normalized()
    end
    if this.world:launch(AirThrow.Projectile, this, from, direction) then
        caster.cooldownLeft = caster.cooldownSeconds
        -- (a splash of oil leaving the hand: the cube of solid oil is for where it lands)
        this.world:burst("Oil Splash", from, 40)
        Audio.playVoice(Voices[OrbSpell], VoiceVolume)
        eyesLeft = EyesStaySeconds
    end
end

-- Divine Baby Oil: Tide - his technique in his armor of oil ([4]): a glowing pool of oil spreads under his feet, rings of
-- light racing over it and drops rising, then columns of oil burst out of the ground one after another towards what he
-- is locked on to, the last one under it (the user's idea, after an anime's water technique: oil instead of water)
local Tide = { Key = Key.Num4, Cooldown = 14, Columns = 5, FirstAt = 0.7, Every = 0.22, Reach = 2.2, Damage = 45,
               Name = "Divine Baby Oil: Tide" }
local tideLeft, tide = 0, nil
local function updateTide(deltaTime)
    tideLeft = math.max(tideLeft - deltaTime, 0)
    local model = this.model and this.model.modelName or ""
    local isArmored = model == "sih_player_armored"
    if not tide and isArmored and tideLeft <= 0 and Input.isKeyDown(Tide.Key) and not this.world.isCutscenePlaying then
        local lock = this.lockOn
        local target = lock and lock.hasTarget and lock.target or nil
        local from = this.transform.position
        local to = target and target.transform.position or (from + this.transform.forward * 9)
        tide = { t = 0, from = from, to = to, target = target, next = 1 }
        tideLeft = Tide.Cooldown
        this.world:effect("Oil Circle", from + vec3(0, 0.05, 0), this.transform.forward)
        this.world:notify(string.upper(Tide.Name))
        Audio.play("@game/Sounds/Spells/magic_whoosh.ogg", 1.0)
        eyesLeft = EyesStaySeconds * 2
        if this.characterActions then this.characterActions:play("Cast_Orb") end
    end
    if not tide then return end

    tide.t = tide.t + deltaTime
    local at = Tide.FirstAt + (tide.next - 1) * Tide.Every
    if tide.t >= at and tide.next <= Tide.Columns then
        local k = tide.next / Tide.Columns
        local where = tide.from + (tide.to - tide.from) * k
        if tide.target then where = tide.from + (tide.target.transform.position - tide.from) * k end
        where = vec3(where.x, 0.05, where.z)
        this.world:effect("Oil Geyser", where, this.transform.forward)
        this.world:shakeCamera(0.25 + 0.1 * tide.next)
        Audio.play("@game/Sounds/Spells/deep_boom.ogg", 0.5 + 0.1 * tide.next)
        local target = tide.target
        if target and target.health and not target.health.isDead then
            local d = target.transform.position - where
            d.y = 0
            if d:length() <= Tide.Reach then target.health:damage(Tide.Damage) end
        end
        tide.next = tide.next + 1
    end
    if tide.next > Tide.Columns then tide = nil end
end

-- For testing the boss fight: true starts the player right before the fog of the arena instead of at the far end of
-- the street (ON while the user tests the boss fight; false for the finished game)
local StartAtArena = true
local hasStarted = false

function OnUpdate(deltaTime)
    if not hasStarted then
        hasStarted = true
        if StartAtArena then
            this.transform.position = vec3(0, 0.1, -15)
            this.transform.rotation = vec3(0, 0, 0)
        end
    end
    -- For testing: F2 puts the player right before the fog of the arena (the street skipped)
    if Input.isKeyDown(Key.F2) and not this.world.isCutscenePlaying then
        this.transform.position = vec3(0, 0.1, -15)
        this.transform.rotation = vec3(0, 0, 0)
        this.world:notify("TELEPORTED TO THE ARENA (F2)")
    end
    updateAirThrow()
    updateTide(deltaTime)
    speakTechniques()
    updateEyes(deltaTime)
    updateHeldOrb(deltaTime)
    updateOilGuard(deltaTime)
    updateGreatShow(deltaTime)
    updateDomain(deltaTime)

    -- The bottle of his great technique: the camera looks up at it coming down on the boss (the boss's own script
    -- answers its hit with the white blast)
    if hasJustReleased(GreatSpell) then
        local fall = find(BottleFall)
        if fall and fall.cutscene then fall.cutscene:play() end
    end
end
