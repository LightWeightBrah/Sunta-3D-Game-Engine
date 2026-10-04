local speed          = 5.0  -- top horizontal speed (meters / second)
local responsiveness = 10.0 -- how fast we reach that speed (higher = snappier)

function OnStart()
	print(this.name .. " started, id = " .. this.id)
end

function OnUpdate(deltaTime)
	local direction = vec3(0, 0, 0)

	if Input.isKeyPressed(Key.W) then direction = direction + vec3( 0, 0, -1) end
	if Input.isKeyPressed(Key.S) then direction = direction + vec3( 0, 0,  1) end
	if Input.isKeyPressed(Key.A) then direction = direction + vec3(-1, 0,  0) end
	if Input.isKeyPressed(Key.D) then direction = direction + vec3( 1, 0,  0) end

	local body = this.physicsBody -- nil if the entity has no PhysicsBody
	if not body then return end

	-- Move only horizontally, vertical speed is left alone, so gravity still works
	local velocity = body.velocity
	local currentHorizontalVelocity = vec3(velocity.x, 0, velocity.z)
	local desiredHorizontalVelocity = direction:normalized() * speed

	-- Closing only a part of the gap every frame (not the whole gap at once) keeps the movement smooth
	-- Without this limit, impulses added every frame would make the body faster and faster
	local partOfGapToClose = math.min(1.0, responsiveness * deltaTime)
	local velocityChange   = (desiredHorizontalVelocity - currentHorizontalVelocity) * partOfGapToClose

	-- The same change of velocity for every mass (addImpulse would change heavy bodies less)
	body:addVelocityChange(velocityChange)
end

function OnTriggerEnter(other)
	print(this.name .. " touched " .. other.name)

	local otherBody = other.physicsBody
	if otherBody then otherBody.velocity = vec3(0, 2, 0) end -- nil if the entity has no PhysicsBody
end

function OnTriggerStay(other)
	--print("Something Stayed in Trigger! ID:", other.id)
end

function OnTriggerExit(other)
	--print("Something Exited Trigger! ID:", other.id)
end

function OnCollisionEnter(other)
    --print("Something Entered Collision! ID: ", other.id)
end

function OnCollisionStay(other)
	--print("Something Stayed in Collision! ID:", other.id)
end

function OnCollisionExit(other)
    --print("Something Exited Collision! ID: ", other.id)
end