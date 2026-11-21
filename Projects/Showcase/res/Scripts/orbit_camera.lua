-- The camera of a showcase scene circles slowly around a point, looking at it
local Center, Radius, Height, Pitch = vec3(0, 1.0, 0), 6.0, 1.6, -9
local DegreesPerSecond = 18
local angle = 20

function OnUpdate(deltaTime)
    angle = angle + DegreesPerSecond * deltaTime
    local radians = math.rad(angle)
    this.transform.position = Center + vec3(math.sin(radians) * Radius, Height, math.cos(radians) * Radius)
    this.transform.rotation = vec3(Pitch, angle, 0)
end
