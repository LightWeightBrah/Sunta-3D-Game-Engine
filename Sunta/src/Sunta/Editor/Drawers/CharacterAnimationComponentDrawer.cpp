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

	ImGui::Separator();
	ImGui::TextDisabled("While locked on");
	edited |= DrawAnimationCombo("Walk Back",    component->walkBackAnimation,    animations);
	edited |= DrawAnimationCombo("Strafe Left",  component->strafeLeftAnimation,  animations);
	edited |= DrawAnimationCombo("Strafe Right", component->strafeRightAnimation, animations);

	edited |= ImGui::SliderFloat("Blend Seconds", &component->blendSeconds, 0.0f, CharacterAnimationComponent::MaxBlendSeconds);
	edited |= ImGui::SliderFloat("Walk Animation Speed", &component->walkAnimationSpeed, 0.0f, 10.0f);
	edited |= ImGui::SliderFloat("Run Animation Speed",  &component->runAnimationSpeed,  0.0f, 15.0f);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Meters per second the character moves in the animation: it plays faster or slower with the real speed (0 = off)");

	return edited;
}

}
