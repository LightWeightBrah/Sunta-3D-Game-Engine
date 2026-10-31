#include "Core/SuntaPreCompiled.h"
#include "CharacterAnimationComponentDrawer.h"

#include <imgui/imgui.h>

#include "NameCombo.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

bool CharacterAnimationComponentDrawer::Draw(void* componentData, unsigned int entityID, EntityManager& entityManager)
{
	auto* component = static_cast<CharacterAnimationComponent*>(componentData);

	auto* modelComponent = entityManager.GetComponent<ModelComponent>(entityID);
	if (!modelComponent || !modelComponent->modelData)
	{
		ImGui::TextDisabled("No Model assigned - Add a Model Component first!");
		return false;
	}

	const auto& animations = modelComponent->modelData->animations;
	if (animations.empty())
	{
		ImGui::TextDisabled("This Model has NO ANIMATIONS!");
		return false;
	}

	bool edited = false;

	edited |= DrawAnimationCombo("Idle", component->idleAnimation, animations);
	edited |= DrawAnimationCombo("Walk", component->walkAnimation, animations);
	edited |= DrawAnimationCombo("Run",  component->runAnimation,  animations);

	edited |= ImGui::SliderFloat("Blend Seconds", &component->blendSeconds, 0.0f, CharacterAnimationComponent::MaxBlendSeconds);

	return edited;
}

}
