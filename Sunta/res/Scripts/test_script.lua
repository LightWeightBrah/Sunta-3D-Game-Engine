function OnStart()
	print("Script Started working from entity with ID: ", entityID)
end

function OnUpdate(deltaTime)
	print("Script Update, frame lasted: ", deltaTime)
end



function OnTriggerEnter(otherEntityID)
	print("Something Entered Trigger! ID:", otherEntityID)
end

function OnTriggerStay(otherEntityID)
	print("Something Stayed in Trigger! ID:", otherEntityID)
end

function OnTriggerExit(otherEntityID)
	print("Something Exited Trigger! ID:", otherEntityID)
end



function OnCollisionEnter(otherEntityID)
    print("Something Entered Collision! ID: ", otherEntityID)
end

function OnCollisionStay(otherEntityID)
	print("Something Stayed in Collision! ID:", otherEntityID)
end

function OnCollisionExit(otherEntityID)
    print("Something Exited Collision! ID: ", otherEntityID)
end
