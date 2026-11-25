function OnStart()
	print("Script Started working from entity with ID: ", entityID)
end

function OnUpdate(deltaTime)
	--print("Script Update, frame lasted: ", deltaTime)
end

function OnTriggerEnter(otherEntityID)
	print("Something Entered Trigger! ID:", otherEntityID)
end

function OnTriggerExit(otherEntityID)
	print("Something Exited Trigger! ID:", otherEntityID)
end
