-- Example: an enemy with its own states, built on the ready Melee Enemy
-- Put this script on an enemy (e.g. Entity > Create Rusted Knight, then Add Component > Script)
--
-- Every enemy has the states Idle and Dead, a Melee Enemy (like the Rusted Knight) also Chase, Attack, Recover and ReturnHome
-- A state added under one of those names replaces it, any other name adds a new state
-- A state is a table with enter(entity), update(entity, deltaTime) and exit(entity), all optional (entity = this enemy)
-- update returns the name of the next state, or nothing to stay

local TauntSeconds = 1.5

function OnStart()
    -- Replaces the ready Idle: the enemy no longer waits to see the player, it taunts as soon as the player is near
    this.enemy:addState("Idle", {
        update = function(entity, deltaTime)
            local ai = entity.enemy
            ai:stop()

            if ai.hasTarget and ai.distanceToTarget < ai.sightDistance then
                return "Taunt"
            end
        end
    })

    -- A new state: stands facing the player for a moment, then fights as usual (the ready Chase)
    this.enemy:addState("Taunt", {
        enter = function(entity)
            print(entity.name .. " noticed the player")
            entity.enemy:lookAtTarget(true)
            entity.enemy:stop()
        end,

        update = function(entity, deltaTime)
            if entity.enemy.secondsInState > TauntSeconds then
                return "Chase"
            end
        end
    })
end
