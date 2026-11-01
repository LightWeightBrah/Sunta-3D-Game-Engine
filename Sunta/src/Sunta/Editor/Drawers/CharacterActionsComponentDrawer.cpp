#include "Core/SuntaPreCompiled.h"
#include "CharacterActionsComponentDrawer.h"

#include <imgui/imgui.h>

#include "NameCombo.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

bool DrawAnimations(CharacterActionsComponent& actions, const std::map<std::string, Animation>& animations)
{
	bool edited = false;

	edited |= DrawAnimationCombo("Roll",           actions.rollAnimation,         animations);
	edited |= DrawAnimationCombo("Backstep",       actions.backstepAnimation,     animations);
	edited |= DrawAnimationCombo("Light Attack 1", actions.lightAttack1Animation, animations);
	edited |= DrawAnimationCombo("Light Attack 2", actions.lightAttack2Animation, animations);
	edited |= DrawAnimationCombo("Light Attack 3", actions.lightAttack3Animation, animations);
	edited |= DrawAnimationCombo("Heavy Attack",   actions.heavyAttackAnimation,  animations);
	edited |= DrawAnimationCombo("Leap Attack",    actions.leapAttackAnimation,   animations);
	edited |= DrawAnimationCombo("Hit Reaction",   actions.hitReactionAnimation,  animations);
	edited |= DrawAnimationCombo("Death",          actions.deathAnimation,        animations);
	edited |= DrawAnimationCombo("Rest",           actions.restAnimation,         animations);
	edited |= DrawAnimationCombo("Stand Up",       actions.standUpAnimation,      animations);
	edited |= DrawAnimationCombo("Block",          actions.blockAnimation,        animations);
	edited |= DrawAnimationCombo("Block Impact",   actions.blockImpactAnimation,  animations);
	edited |= DrawAnimationCombo("Parry",          actions.parryAnimation,        animations);
	edited |= DrawAnimationCombo("Parried",        actions.parriedAnimation,      animations);

	return edited;
}

bool DrawSpeeds(CharacterActionsComponent& actions)
{
	bool edited = false;

	edited |= ImGui::SliderFloat("Roll Speed",        &actions.rollSpeed,       0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= ImGui::SliderFloat("Backstep Speed",    &actions.backstepSpeed,   0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= ImGui::SliderFloat("Attack Step Speed", &actions.attackStepSpeed, 0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= ImGui::SliderFloat("Leap Speed",        &actions.leapSpeed,       0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= ImGui::SliderFloat("Heavy Damage Multiplier", &actions.heavyDamageMultiplier, 1.0f, CharacterActionsComponent::MaxDamageMultiplier);
	edited |= ImGui::SliderFloat("Riposte Damage Multiplier", &actions.riposteDamageMultiplier, 1.0f, CharacterActionsComponent::MaxDamageMultiplier);
	edited |= ImGui::SliderFloat("Shockwave Radius",   &actions.shockwaveRadius,   0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= ImGui::SliderFloat("Shockwave Damage",   &actions.shockwaveDamage,   0.0f, 500.0f);
	edited |= ImGui::SliderFloat("Shockwave Distance", &actions.shockwaveDistance, 0.0f, CharacterActionsComponent::MaxSpeed);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("A heavy attack or a leap hurts everyone around where it lands (0 radius = no shockwave)");

	return edited;
}

// The parts of an action, as fractions of its animation (0 = start, 1 = end)
bool DrawTiming(CharacterActionsComponent& actions)
{
	bool edited = false;

	edited |= ImGui::SliderFloat("Light Hit From",    &actions.lightHitFrom,     0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Light Hit To",      &actions.lightHitTo,       0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Heavy Hit From",    &actions.heavyHitFrom,     0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Heavy Hit To",      &actions.heavyHitTo,       0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Leap Hit From",     &actions.leapHitFrom,      0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Leap Hit To",       &actions.leapHitTo,        0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Parry From",        &actions.parryFrom,        0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Parry To",          &actions.parryTo,          0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Invulnerable From", &actions.invulnerableFrom, 0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Invulnerable To",   &actions.invulnerableTo,   0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Input Buffer From", &actions.inputBufferFrom,  0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Cancel From",       &actions.cancelFrom,       0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Blend Seconds",     &actions.blendSeconds,     0.0f, CharacterActionsComponent::MaxBlendSeconds);

	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Parts of an action are fractions of its animation: 0 = start, 1 = end");

	return edited;
}

}

bool CharacterActionsComponentDrawer::Draw(void* componentData, unsigned int entityID, EntityManager& entityManager)
{
	auto* actions = static_cast<CharacterActionsComponent*>(componentData);

	auto* modelComponent = entityManager.GetComponent<ModelComponent>(entityID);
	if (!modelComponent || !modelComponent->modelData || modelComponent->modelData->animations.empty())
	{
		ImGui::TextDisabled("Needs a Model with animations");
		return false;
	}

	bool edited = DrawAnimations(*actions, modelComponent->modelData->animations);

	ImGui::Separator();
	edited |= DrawSpeeds(*actions);

	ImGui::Separator();
	edited |= DrawTiming(*actions);

	return edited;
}

}
