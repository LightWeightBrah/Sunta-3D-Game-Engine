#include "Core/SuntaPreCompiled.h"
#include "CharacterAnimationComponentDrawer.h"

#include <map>
#include <string>

#include <imgui/imgui.h>

#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

constexpr const char* NoAnimationLabel = "(none)";

// A dropdown of the animations. Returns true when another one was chosen
bool DrawAnimationCombo(const char* label, std::string& chosenName, const std::map<std::string, Animation>& animations)
{
	bool changed = false;

	const std::string preview = chosenName.empty() ? NoAnimationLabel : chosenName;

	if (ImGui::BeginCombo(label, preview.c_str()))
	{
		if (ImGui::Selectable(NoAnimationLabel, chosenName.empty()))
		{
			chosenName.clear();
			changed = true;
		}

		for (const auto& [animationName, animation] : animations)
		{
			const bool isSelected = (chosenName == animationName);

			if (ImGui::Selectable(animationName.c_str(), isSelected))
			{
				chosenName = animationName;
				changed    = true;
			}

			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	return changed;
}

}

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
