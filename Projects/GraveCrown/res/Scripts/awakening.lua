-- The dead king's awakening: the first moment of the game is the cutscene on this entity (the camera down the crypt's
-- nave to the king rising from his knees, the game's name over it). Once: a death and a rest at the bonfire don't
-- show it again (play() plays a cutscene only once)
-- After it, short lessons of the fight, a few seconds apart (the deflect, the rally, the wrath, the court)

local hints = {
    "Raise your guard just before a blow lands to deflect it",
    "Strike back at once to win back the health you lost",
    "V sets your wrath loose when its violet bar is full",
    "G calls the knights of your court to your side",
}
local hintEvery = 6.0

local since = -1.0
local shown = 0

function OnStart()
    this.cutscene:play()
end

function OnUpdate(deltaTime)
    if shown >= #hints or this.cutscene.isPlaying then return end
    since = since + deltaTime
    if since >= hintEvery * shown + 1.5 then
        shown = shown + 1
        this.world:notify(hints[shown])
    end
end
