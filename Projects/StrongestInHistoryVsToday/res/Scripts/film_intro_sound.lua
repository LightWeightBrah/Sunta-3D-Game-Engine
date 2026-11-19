-- The sound of a video cutscene (the engine plays videos without sound)
--
-- Put on an entity with an Audio Source (not playing on start), in a scene whose first video cutscene is the one it
-- belongs to (e.g. the one a fog gate plays): the sound starts with the cutscene and fades out when it ends or is skipped

local FadeOutSeconds = 0.4

local hasCutsceneStarted = false
local hasStopped = false

function OnUpdate(deltaTime)
    if hasStopped then return end

    if this.world.isCutscenePlaying then
        if not hasCutsceneStarted then this.audioSource:play() end
        hasCutsceneStarted = true
    elseif hasCutsceneStarted then
        this.audioSource:stop(FadeOutSeconds)
        hasStopped = true
    end
end
