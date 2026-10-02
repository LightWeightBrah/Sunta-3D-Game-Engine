#include "Core/SuntaPreCompiled.h"
#include "AnimatorComponentDrawer.h"

#include <imgui/imgui.h>
#include <imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

void AnimatorComponentDrawer::Draw(void* componentData, unsigned int entityID, EntityManager& entityManager)
{
	auto* animatorComponent = static_cast<AnimatorComponent*>(componentData);

	auto* modelComponent = entityManager.GetComponent<ModelComponent>(entityID);
	if (!modelComponent || !modelComponent->modelData)
	{
		ImGui::TextDisabled("No Model assigned - Add a Model Component first!");
		return;
	}

	auto& animations = modelComponent->modelData->animations;

	if (animations.empty())
	{
		ImGui::TextDisabled("This Model has NO ANIMATIONS!");
		return;
	}

	std::string previewName = animatorComponent->currentAnimationName.empty()
		? "None" : animatorComponent->currentAnimationName;

	if (ImGui::BeginCombo("Animation", previewName.c_str()))
	{
		for (const auto& [animationName, animation] : animations)
		{
			bool isSelected = (animatorComponent->currentAnimationName == animationName);

			if (ImGui::Selectable(animationName.c_str(), isSelected))
			{
				if (animatorComponent->currentAnimationName != animationName)
				{
					animatorComponent->currentAnimationName = animationName;
					animatorComponent->animator.PlayAnimation(&modelComponent->modelData->animations.at(animationName));

					// Show the first frame of the new animation in the editor (bones are only updated during Play)
					animatorComponent->animator.SampleAtTime(0.0f);
				}
			}

			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}
}

}