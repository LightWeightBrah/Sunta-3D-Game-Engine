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
local HeldOrbSize = 0.34

-- His domain: the light he gives off at the height of the sign, and the blast when it opens
local DomainGlow         = vec3(1.6, 0.75, 0.15)
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

local function updateEyes(deltaTime)
    if isCasting(OrbSpell) or isCasting(GreatSpell) or isCasting(DomainSpell) then
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
        heldGrowth = math.min(heldGrowth + deltaTime * 4, 1)
        held.transform.scale = vec3(1, 1, 1) * (HeldOrbSize * heldGrowth)
    elseif caster and caster.isCasting then
        heldGrowth = -1
    end

    held.liquidOrb.isVisible = isHolding and heldGrowth > 0.05
end

-- His own domain: he lights up as he holds the sign, then everything goes white
local function updateDomain(deltaTime)
    if isCasting(DomainSpell) then
        domainGlow = math.min(domainGlow + deltaTime * 1.4, 1)
    else
        domainGlow = math.max(domainGlow - deltaTime * 2.5, 0)
    end

    if this.model then this.model.glow = DomainGlow * (domainGlow * domainGlow) end

    if hasJustReleased(DomainSpell) then
        this.world:flash(vec3(1, 1, 1), DomainFlashSeconds, 0.15)
    end
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
    updateDomain(deltaTime)

    -- The bottle of his great technique: the camera looks up at it coming down on the boss (the boss's own script
    -- answers its hit with the white blast)
    if hasJustReleased(GreatSpell) then
        local fall = find(BottleFall)
        if fall and fall.cutscene then fall.cutscene:play() end
    end
end
