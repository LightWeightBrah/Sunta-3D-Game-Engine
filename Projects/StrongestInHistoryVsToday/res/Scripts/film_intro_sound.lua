-- The sound of a video cutscene (the engine plays videos without sound)
--
-- Put on an entity with an Audio Source that plays on start, in a scene whose video cutscene starts with the game:
-- the sound goes on while the cutscene plays and fades out when it ends or the player skips it

local FadeOutSeconds = 0.4

local hasCutsceneStarted = false
local hasStopped = false

function OnUpdate(deltaTime)
    if hasStopped then return end

    if this.world.isCutscenePlaying then
        hasCutsceneStarted = true
    elseif hasCutsceneStarted then
        this.audioSource:stop(FadeOutSeconds)
        hasStopped = true
    end
end
