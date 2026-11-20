-- The summon adapts to what it is hit with (the wheel of the film's monster): a few hits of one kind in a row and that
-- kind barely hurts it any more. It knows three kinds - fists (the light combo), kicks (heavy blows, the running
-- attack, everything from a jump) and techniques (spells) - and is adapted to ONE at a time, so the answer is always
-- there: switch to another kind. Adapting takes it a moment (it roars, the wheel turns): free hits for a player who
-- already switched. And what it has never seen breaks the wheel: the great technique (Baby Oil Detonation) always
-- hurts in full and makes it forget everything it learnt
--
-- Put on the summoned enemy. Everything it uses is general: the callback OnDamaged tells the kind of every hit, the
-- attacker's Character Actions tell the blow ('action'), and the Health lets through a part of each kind
-- ('weaponDamageTaken', 'spellDamageTaken')

local HitsToAdapt   = 4      -- of one kind in a row
local AdaptedTaken  = 0.2    -- the part of the damage that still gets through
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

local lastKind = nil
local hitsInARow = 0
local adaptedTo = nil
local sinceAdaptedHit = 0
local hintLeft = 0
local greatReleases = 0
local greatFlightLeft = 0

local function find(name) return this.world:find(name) end

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
    this.world:notify("IT ADAPTED TO YOUR " .. NameOfKind[kind])
    this.world:shakeCamera(0.4)
    Audio.playAt(AdaptSound, this.transform.position, 0.9)
    hintLeft = HintSeconds
    -- The wheel turns: a moment of roaring (it doesn't attack meanwhile)
    if this.characterActions then this.characterActions:play("Taunt") end
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

function OnStart()
    forget()
end

function OnUpdate(deltaTime)
    -- Asleep again (the player died and the fight starts over): it has learnt nothing yet
    if this.enemy.isDormant then
        if adaptedTo ~= nil or hitsInARow > 0 then forget() end
        return
    end

    watchGreatSpell(deltaTime)

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
