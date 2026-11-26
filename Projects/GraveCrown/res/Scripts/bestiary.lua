-- The king's bestiary (as Cautious Hero's careful study of every foe): the first time the king locks on to a kind of
-- enemy, its name and one thing worth knowing about it show on the screen. Each kind is told once a game
-- (on the player; it reads what the player is locked on to)

local lore = {
    ["Hollow Knight of the Gods"] = "It guards. Press it until its posture breaks",
    ["Masked Cultist"]            = "Frail, but its staff burns. Close in fast",
    ["Ash Wolf"]                  = "It leaps from afar. Step aside, then strike",
    ["Obsidian Golem"]            = "Its leap can't be blocked. Watch for the red sign",
    ["Chain Warden, Hand of the Gods"] = "Deflect his cuts; dodge when the red sign shows",
}

local told = {}

function OnUpdate(deltaTime)
    if not this.lockOn.hasTarget then return end
    local target = this.lockOn.target
    if not target or not target.enemyHealthBar then return end
    local name = target.enemyHealthBar.displayName
    if told[name] or not lore[name] then return end
    told[name] = true
    this.world:notify(name .. " - " .. lore[name])
end
