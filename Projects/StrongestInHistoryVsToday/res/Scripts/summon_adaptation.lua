-- The summon adapts to what it is hit with: after a few hits of one kind in a row (spells, or blows) that kind barely
-- hurts it any more, until the player switches to the other one. Fists and orbs have to take turns
--
-- Put on the summoned enemy. Everything it uses is general: the callback OnDamaged tells the kind of every hit and
-- the Health lets through a part of each kind ('weaponDamageTaken', 'spellDamageTaken')

local HitsToAdapt   = 4      -- of one kind in a row
local AdaptedTaken  = 0.15   -- the part of the damage that still gets through
local AdaptEffect   = "Adapt Burst"
local AdaptSound    = "@game/Sounds/Boss/roar.ogg"

local NameOfKind = { weapon = "FISTS", spell = "SPELLS" }

local lastKind = nil
local hitsInARow = 0
local adaptedTo = nil

local function forget()
    lastKind, hitsInARow, adaptedTo = nil, 0, nil
    this.health.weaponDamageTaken = 1
    this.health.spellDamageTaken = 1
end

local function adaptTo(kind)
    adaptedTo = kind
    this.health.weaponDamageTaken = (kind == "weapon") and AdaptedTaken or 1
    this.health.spellDamageTaken  = (kind == "spell") and AdaptedTaken or 1

    this.world:burst(AdaptEffect, this.transform.position + vec3(0, 1.6, 0), 220)
    this.world:notify("IT ADAPTED TO YOUR " .. NameOfKind[kind])
    this.world:shakeCamera(0.4)
    Audio.playAt(AdaptSound, this.transform.position, 0.9)
end

function OnStart()
    forget()
end

function OnUpdate(deltaTime)
    -- Asleep again (the player died and the fight starts over): it has learnt nothing yet
    if this.enemy.isDormant and (adaptedTo ~= nil or hitsInARow > 0) then forget() end
end

function OnDamaged(amount, kind, attacker)
    if NameOfKind[kind] == nil or this.health.isDead then return end

    hitsInARow = (kind == lastKind) and hitsInARow + 1 or 1
    lastKind = kind

    if hitsInARow >= HitsToAdapt and adaptedTo ~= kind then adaptTo(kind) end
end
