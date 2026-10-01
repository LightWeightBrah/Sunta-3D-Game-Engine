#pragma once

#include "Scene/Scene.h"
#include "ECS/Component.h"
#include "Scripting/ScriptingEngine.h"

namespace Sunta
{

class Entity
{
public:
	Entity() = default;
	Entity(unsigned int entityID, Scene* scene)
		: entityID(entityID)
		, scene(scene) { }

	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		return scene->GetEntityManager().AddComponent<T>(entityID, std::forward<Args>(args)...);
	}

	template<typename T>
	T* GetComponent()
	{
		return scene->GetEntityManager().GetComponent<T>(entityID);
	}

	void AttachScript(const std::string& filepath)
	{
		auto* scriptComponent = scene->GetEntityManager().GetComponent<ScriptComponent>(entityID);
		if (!scriptComponent)
		{
			scriptComponent = &scene->GetEntityManager().AddComponent<ScriptComponent>(entityID);
		}

		scriptComponent->LoadScript(ScriptingEngine::GetState(), filepath, entityID);
	}

	unsigned int GetID() const { return entityID; }

private:
	unsigned int entityID = 0;
	Scene* scene = nullptr;
};


}