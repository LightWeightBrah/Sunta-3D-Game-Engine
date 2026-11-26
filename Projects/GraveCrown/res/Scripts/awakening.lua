-- The dead king's awakening: the first moment of the game is the cutscene on this entity (the camera down the crypt's
-- nave to the king rising from his knees, the game's name over it). Once: a death and a rest at the bonfire don't
-- show it again (play() plays a cutscene only once)
function OnStart()
    this.cutscene:play()
end
