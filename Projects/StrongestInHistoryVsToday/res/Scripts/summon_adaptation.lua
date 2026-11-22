-- The summon adapts to what it is hit with (the wheel of the film's monster): a few hits of one kind in a row and that
-- kind barely hurts it any more. It knows three kinds - fists (the light combo), kicks (heavy blows, the running
-- attack, everything from a jump) and techniques (spells) - and is adapted to ONE at a time, so the answer is always
-- there: switch to another kind. Adapting takes it a moment (it roars, the wheel turns): free hits for a player who
-- already switched. And what it has never seen breaks the wheel: the great technique (Baby Oil Detonation) always
-- hurts in full and makes it forget everything it learnt
--
-- The eight-spoked wheel over its head (the film's last frames) turns an eighth every time it adapts, and its wounds
-- close as it turns. Its own moves besides the engine's
-- melee:
--   Rush          - a player who keeps away: it crouches for a moment (dust, the wheel flares), then crosses the
--                   arena in a blink straight through him - roll through it
--   Blade of Light - from half its health on: the blade of light grows out of its right fist (a hitbox of its own),
--                   it fights faster and leaps from farther, and close up it brings both fists down three times in a
--                   row, the last one breaking the ground wide around it
--
-- Put on the summoned enemy. Everything it uses is general: the callback OnDamaged tells the kind of every hit, the
-- attacker's Character Actions tell the blow ('action'), the Health lets through a part of each kind
-- ('weaponDamageTaken', 'spellDamageTaken'), a state of its own ('Own Move') holds the engine's melee while it rushes
-- or slams, and children of its entity are the wheel ("Summon Halo") and the blade ("Summon Blade")

local HitsToAdapt   = 9      -- of one kind in a row (the player first sees it hurt normally)
local AdaptedTaken  = 0.08   -- the part of the damage that still gets through
local ForgetSeconds = 14     -- not hit by the kind it adapted to for this long: it forgets it
local AdaptEffect   = "Adapt Burst"
local AdaptSound    = "@game/Sounds/Boss/roar.ogg"
local GreatSpell    = "Player Great Orb Spell"
-- The great technique is in the air this long after it left the hands
local GreatSpellFlightSeconds = 5

local NameOfKind = { fists = "FISTS", kicks = "KICKS", spells = "TECHNIQUES" }
local HintOfKind = {
    fists  = "Fists barely hurt it now - kick (Ctrl + LMB, from a jump) or use your techniques",
    kicks  = "Kicks barely hurt it now - punch (LMB) or use your techniques",
    spells = "Techniques barely hurt it now - punch and kick it",
}
local HintSeconds = 4

-- The wheel over its head: an eighth of a turn each time it adapts, and this part of its whole health comes back
local WheelName         = "Summon Halo"
local WheelTurnSeconds  = 0.6
local WheelSound        = "@game/Sounds/Combat/Shockwave"
local WheelGlow         = vec3(1.0, 0.75, 0.3)
local AdaptHealFraction = 0.06

-- Rush: from this far, now and then; this fast (meters a second), this far past the player, hurting this much
local Rush = { From = 7.5, Every = 9.0, Windup = 0.75, Speed = 34.0, Overshoot = 3.0, Damage = 28, HitMeters = 1.9,
               RecoverSeconds = 0.45 }
-- The arena it may not leave (the same as boss_fight.lua's)
local ArenaMin, ArenaMax = vec3(-26, 0, -76), vec3(26, 0, -24)

-- Blade of Light: below this part of its health; its blade and how it fights then; the slams in a row
local BladeOfLight = { AtFraction = 0.5, Blade = "Summon Blade", BladeGlow = vec3(1.4, 1.25, 0.9),
                      CooldownSeconds = 0.85, AttackSpeed = 1.15, LeapDistance = 14.0,
                      SlamsWithin = 5.5, SlamsEvery = 12.0, Slams = 3, BlastDelay = 0.55, BlastRadius = 7.0,
                      BlastDamage = 28 }

local lastKind = nil
local hitsInARow = 0
local adaptedTo = nil
local sinceAdaptedHit = 0
local hintLeft = 0
local greatReleases = 0
local greatFlightLeft = 0

-- Its own moves
local wheelTurn, wheelFrom, wheelTo, wheelT = 0, 0, 0, 1
local glowLeft = 0
local rushIn, rush = Rush.Every * 0.5, nil
local hasBlade, slamsIn, slams = false, 0, nil
local base = nil   -- its melee as the scene sets it up (back when the fight starts over)

local function find(name) return this.world:find(name) end
local function child(name) return this:findChild(name) end
local function flat(v) return vec3(v.x, 0, v.z) end

local function turnWheel()
    wheelFrom, wheelTo, wheelT = wheelTurn, wheelTurn + 45, 0
end

-- The part of each kind let through now: a weapon hit is told fists or kicks by the blow the player is throwing at
-- this moment (set every frame, before the blow lands)
local playerBlow = "fists"
local function applyTaken()
    this.health.weaponDamageTaken = (adaptedTo ~= nil and adaptedTo == playerBlow) and AdaptedTaken or 1
    this.health.spellDamageTaken  = (adaptedTo == "spells" and greatFlightLeft <= 0) and AdaptedTaken or 1
end

local function forget()
    lastKind, hitsInARow, adaptedTo = nil, 0, nil
    applyTaken()
end

local function adaptTo(kind)
    adaptedTo = kind
    sinceAdaptedHit = 0
    hitsInARow = 0
    applyTaken()

    this.world:burst(AdaptEffect, this.transform.position + vec3(0, 2.6, 0), 220)
    this.world:notify("THE WHEEL TURNS: IT ADAPTED TO YOUR " .. NameOfKind[kind])
    this.world:shakeCamera(0.4)
    Audio.playAt(AdaptSound, this.transform.position, 0.9)
    Audio.playAt(WheelSound, this.transform.position, 0.7)
    hintLeft = HintSeconds
    -- The wheel turns an eighth and its wounds close
    turnWheel()
    glowLeft = 0.8
    this.health:heal(this.health.max * AdaptHealFraction)
    -- A moment of roaring (it doesn't attack meanwhile)
    if this.characterActions and not rush and not slams then this.characterActions:play("Taunt") end
end

-- Which of the three kinds a hit was
local function kindOf(damageKind, attacker)
    if damageKind == "spell" then return "spells" end
    if damageKind ~= "weapon" then return nil end

    local action = attacker and attacker.characterActions and attacker.characterActions.action or ""
    if action == "light" then return "fists" end
    return "kicks"
end

-- The great technique left the player's hands: while it flies it will hurt in full
local function watchGreatSpell(deltaTime)
    greatFlightLeft = math.max(greatFlightLeft - deltaTime, 0)

    local spell = find(GreatSpell)
    if not spell or not spell.spellCaster then return end
    local released = spell.spellCaster.timesReleased
    if released > greatReleases then
        greatFlightLeft = GreatSpellFlightSeconds
        applyTaken()
    end
    greatReleases = released
end

-- ========== The wheel and the blade ==========

-- Slain, it lies a moment, then sinks into the ground in a cloud of dust and is gone, as a summoned thing does (its
-- body, three meters of it, lay in the middle of the arena for the rest of the fight, in the way of the camera)
local SinkAfterSeconds, SinkSeconds, SinkDepth = 3.0, 2.2, 3.5
local deadFor = 0
local function sinkWhenDead(deltaTime)
    local before = deadFor
    deadFor = deadFor + deltaTime
    if deadFor < SinkAfterSeconds then return end
    if before < SinkAfterSeconds then
        if this.physicsBody then this.physicsBody.useGravity = false; this.physicsBody.isKinematic = true end
        this.world:burst("Skid Dust", this.transform.position + vec3(0, 0.5, 0), 60)
    end
    if deadFor < SinkAfterSeconds + SinkSeconds then
        local here = this.transform.position
        this.transform.position = vec3(here.x, here.y - SinkDepth / SinkSeconds * deltaTime, here.z)
        if math.floor(deadFor * 4) ~= math.floor(before * 4) then
            this.world:burst("Skid Dust", vec3(here.x, 0.3, here.z), 20)
        end
    elseif this.model and this.model.isVisible then
        this.model.isVisible = false
    end
end

-- The wheel right before the camera (the monster circling behind the player) filled half the screen: hidden then
local function wheelNearCamera()
    local camera = find("Camera")
    if not camera then return false end
    local wheel = this.transform.position + vec3(0, 2.75 * this.transform.scale.y, 0)
    return (camera.transform.position - wheel):length() < 4.5
end

local function showWheel(isOn)
    local wheel = child(WheelName)
    if wheel and wheel.model then wheel.model.isVisible = isOn end
end

local function updateWheel(deltaTime)
    local wheel = child(WheelName)
    if not wheel or not wheel.model then return end
    if wheelT < 1 then
        -- (a heavy wheel: it starts slowly and stops with a jolt)
        wheelT = math.min(wheelT + deltaTime / WheelTurnSeconds, 1)
        wheelTurn = wheelFrom + (wheelTo - wheelFrom) * (wheelT * wheelT)
    end
    wheel.transform.rotation = vec3(0, wheelTurn, 0)
    glowLeft = math.max(glowLeft - deltaTime, 0)
    wheel.model.glow = WheelGlow * math.min(glowLeft * 2.0, 1.5)
end

local function setBlade(isOn)
    local blade = child(BladeOfLight.Blade)
    if not blade then return end
    if blade.model then
        blade.model.isVisible = isOn
        blade.model.glow = isOn and BladeOfLight.BladeGlow or vec3(0, 0, 0)
    end
    if blade.hitbox then blade.hitbox.isEnabled = isOn end
end

-- ========== Its own moves ==========

local function clampToArena(p)
    return vec3(math.max(ArenaMin.x, math.min(ArenaMax.x, p.x)), p.y, math.max(ArenaMin.z, math.min(ArenaMax.z, p.z)))
end

-- (its front is its own -Z)
local function faceTowards(point)
    local d = point - this.transform.position
    this.transform.rotation = vec3(0, math.deg(math.atan(d.x, d.z)) + 180, 0)
end

local function isFree()
    local state = this.enemy.state
    return not this.characterActions.isBusy and (state == "Chase" or state == "Recover")
end

local function growBlade()
    hasBlade = true
    slamsIn = 2.0
    setBlade(true)
    this.meleeEnemy.attackCooldownSeconds = BladeOfLight.CooldownSeconds
    this.meleeEnemy.leapAttackDistance = BladeOfLight.LeapDistance
    this.characterActions.attackSpeed = BladeOfLight.AttackSpeed
    this.characterActions:play("Taunt")
    this.world:burst(AdaptEffect, this.transform.position + vec3(0, 2.0, 0), 400)
    this.world:flash(vec3(1.0, 0.9, 0.6), 0.15, 0.05)
    this.world:shakeCamera(0.7)
    this.world:notify("IT GROWS A BLADE OF LIGHT")
    Audio.playAt(AdaptSound, this.transform.position, 1.0)
    turnWheel()
    glowLeft = 1.2
end

local function stopOwnMoves()
    rush, slams = nil, nil
    hasBlade = false
    rushIn, slamsIn = Rush.Every * 0.5, 0
    setBlade(false)
    if base then
        this.meleeEnemy.attackCooldownSeconds = base.cooldown
        this.meleeEnemy.leapAttackDistance = base.leap
        this.characterActions.attackSpeed = base.speed
    end
end

-- Rush: a moment of crouching (dust at its feet, the wheel flares), then through the player in a blink
local function startRush(player)
    local from = this.transform.position
    local toPlayer = flat(player.transform.position - from)
    local direction = toPlayer:normalized()
    local to = clampToArena(from + direction * (toPlayer:length() + Rush.Overshoot))
    rush = { t = 0, from = from, to = to, direction = direction, hasHit = false,
             seconds = flat(to - from):length() / Rush.Speed }
    this.enemy:changeState("Own Move")
    faceTowards(player.transform.position)
    this.world:effect("Ground Slam", from, direction)
    this.world:burst(AdaptEffect, from + vec3(0, 0.4, 0), 120)
    glowLeft = Rush.Windup
    Audio.playAt(AdaptSound, from, 0.6)
end

local function updateRush(deltaTime, player)
    local r = rush
    r.t = r.t + deltaTime
    if r.t < Rush.Windup then return end

    local k = math.min((r.t - Rush.Windup) / math.max(r.seconds, 0.05), 1)
    this.transform.position = r.from + (r.to - r.from) * k
    r.streakIn = (r.streakIn or 0) - deltaTime
    if r.streakIn <= 0 and k < 1 then
        r.streakIn = 0.03
        this.world:effect("Dash Streak", this.transform.position + vec3(0, 0.1, 0), r.direction)
    end
    if player and not r.hasHit and flat(player.transform.position - this.transform.position):length() < Rush.HitMeters then
        r.hasHit = true
        player.health:damage(Rush.Damage)
        this.world:impactFrame(0.7, 0.25)
        this.world:shakeCamera(0.6)
    end
    if k >= 1 and not r.hasEnded then
        r.hasEnded = true
        this.world:effect("Ground Slam", this.transform.position, -r.direction)
        this.world:shakeCamera(0.5)
        if player then faceTowards(player.transform.position) end
    end
    if r.t >= Rush.Windup + r.seconds + Rush.RecoverSeconds then
        rush = nil
        rushIn = Rush.Every
        this.enemy:changeState("Chase")
    end
end

-- Blade of Light: both fists into the ground three times, turning after the player, the last one a wide blast
local function startSlams()
    slams = { left = BladeOfLight.Slams, waitFor = 0 }
    this.enemy:changeState("Own Move")
end

local function updateSlams(deltaTime, player)
    local s = slams
    if s.blastIn then
        s.blastIn = s.blastIn - deltaTime
        if s.blastIn <= 0 then
            s.blastIn = nil
            local center = this.transform.position
            this.world:effect("Cleaver Slam Burst", center, this.transform.forward)
            this.world:burst(AdaptEffect, center + vec3(0, 0.3, 0), 300)
            this.world:shakeCamera(1.0)
            if player and flat(player.transform.position - center):length() <= BladeOfLight.BlastRadius then
                player.health:damage(BladeOfLight.BlastDamage)
            end
        end
    end

    s.waitFor = s.waitFor - deltaTime
    if this.characterActions.isBusy or s.waitFor > 0 or s.blastIn then return end
    if s.left <= 0 then
        slams = nil
        slamsIn = BladeOfLight.SlamsEvery
        this.enemy:changeState("Chase")
        return
    end
    if player then faceTowards(player.transform.position) end
    this.characterActions:play("Attack_Heavy")
    s.left = s.left - 1
    s.waitFor = 0.15
    if s.left == 0 then s.blastIn = BladeOfLight.BlastDelay end
end

-- The engine's melee waits while it rushes or slams
local OwnMoveState = {
    enter = function(entity)
        entity.enemy:stop()
    end,
    update = function(entity, deltaTime)
        if not rush and not slams then return "Chase" end
        entity.enemy:stop()
    end,
}

local function updateOwnMoves(deltaTime)
    local player = find("Player")
    if rush then updateRush(deltaTime, player) return end
    if slams then updateSlams(deltaTime, player) return end
    if not player or (player.health and player.health.isDead) then return end

    if not hasBlade and this.health.fraction <= BladeOfLight.AtFraction and isFree() then
        growBlade()
        return
    end

    local distance = flat(player.transform.position - this.transform.position):length()
    rushIn = rushIn - deltaTime
    slamsIn = slamsIn - deltaTime
    if not isFree() then return end
    if hasBlade and slamsIn <= 0 and distance <= BladeOfLight.SlamsWithin then
        startSlams()
    elseif rushIn <= 0 and distance >= Rush.From then
        startRush(player)
    end
end

function OnStart()
    this.enemy:addState("Own Move", OwnMoveState)
    base = { cooldown = this.meleeEnemy.attackCooldownSeconds, leap = this.meleeEnemy.leapAttackDistance,
             speed = this.characterActions.attackSpeed }
    forget()
    stopOwnMoves()
    showWheel(false)
end

function OnUpdate(deltaTime)
    -- Asleep again (the player died and the fight starts over): it has learnt nothing yet
    if this.enemy.isDormant then
        if adaptedTo ~= nil or hitsInARow > 0 then forget() end
        if hasBlade or rush or slams then stopOwnMoves() end
        showWheel(false)
        if deadFor > 0 then
            deadFor = 0
            if this.model then this.model.isVisible = true end
            if this.physicsBody then this.physicsBody.isKinematic = false; this.physicsBody.useGravity = true end
        end
        return
    end

    showWheel(not this.health.isDead and not wheelNearCamera())
    updateWheel(deltaTime)
    if this.health.isDead then
        if hasBlade then setBlade(false); hasBlade = false end
        sinkWhenDead(deltaTime)
        return
    end

    watchGreatSpell(deltaTime)
    updateOwnMoves(deltaTime)

    local player = find("Player")
    local action = player and player.characterActions and player.characterActions.action or ""
    if action == "light" then playerBlow = "fists" elseif action ~= "" then playerBlow = "kicks" end
    applyTaken()

    if adaptedTo then
        sinceAdaptedHit = sinceAdaptedHit + deltaTime
        if sinceAdaptedHit >= ForgetSeconds then
            this.world:notify("IT FORGOT YOUR " .. NameOfKind[adaptedTo])
            forget()
        end
    end

    if hintLeft > 0 and adaptedTo then
        hintLeft = hintLeft - deltaTime
        this.world:prompt(HintOfKind[adaptedTo])
    end
end

function OnDamaged(amount, kind, attacker)
    if this.health.isDead then return end

    -- What it has never seen: the bottle from the sky breaks the wheel
    if kind == "spell" and greatFlightLeft > 0 and amount >= 60 then
        if adaptedTo then this.world:notify("THE WHEEL BREAKS") end
        forget()
        greatFlightLeft = 0
        return
    end

    local hitKind = kindOf(kind, attacker)
    if not hitKind then return end

    if hitKind == adaptedTo then sinceAdaptedHit = 0 end

    hitsInARow = (hitKind == lastKind) and hitsInARow + 1 or 1
    lastKind = hitKind

    if hitsInARow >= HitsToAdapt and adaptedTo ~= hitKind then adaptTo(hitKind) end
end
