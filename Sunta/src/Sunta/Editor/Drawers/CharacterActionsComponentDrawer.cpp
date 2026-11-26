#include "Core/SuntaPreCompiled.h"
#include "CharacterActionsComponentDrawer.h"

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

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
	edited |= DrawAnimationCombo("Jump",           actions.jumpAnimation,         animations);
	edited |= DrawAnimationCombo("Jump Attack",    actions.jumpAttackAnimation,   animations);
	edited |= DrawAnimationCombo("Jump Heavy Attack", actions.jumpHeavyAttackAnimation, animations);

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
	edited |= ImGui::SliderFloat("Attack Speed", &actions.attackSpeed, CharacterActionsComponent::MinAttackSpeed, CharacterActionsComponent::MaxAttackSpeed);
	edited |= ImGui::SliderFloat("Riposte Damage Multiplier", &actions.riposteDamageMultiplier, 1.0f, CharacterActionsComponent::MaxDamageMultiplier);
	edited |= ImGui::SliderFloat("Shockwave Radius",   &actions.shockwaveRadius,   0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= ImGui::SliderFloat("Shockwave Damage",   &actions.shockwaveDamage,   0.0f, 500.0f);
	edited |= ImGui::SliderFloat("Shockwave Distance", &actions.shockwaveDistance, 0.0f, CharacterActionsComponent::MaxSpeed);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("A heavy attack or a leap hurts everyone around where it lands (0 radius = no shockwave)");
	edited |= ImGui::SliderFloat("Shockwave Camera Shake", &actions.shockwaveCameraShake, 0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Attack Turn Speed", &actions.attackTurnDegreesPerSecond, 0.0f, 720.0f);
	edited |= ImGui::Checkbox("Heavy Attacks Are Perilous", &actions.areHeavyAttacksPerilous);
	edited |= ImGui::InputText("Perilous Warning Effect", &actions.perilousWarningEffect);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Degrees per second an attack keeps turning towards its target until it starts to hit (0 = commits at once)");
	edited |= ImGui::SliderFloat("Attack Lunge Speed", &actions.attackLungeSpeed, 0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= ImGui::SliderFloat("Attack Reach",       &actions.attackReach,      0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= ImGui::SliderFloat("Attack Lunge Max Distance", &actions.attackLungeMaxDistance, 0.0f, 100.0f);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("An attack steps in (up to the lunge speed) to end this far from its target (0 lunge speed = a fixed step)");
	edited |= ImGui::SliderFloat("Delayed Blow Chance",  &actions.attackDelayChance,  0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Delayed Blow Seconds", &actions.attackDelaySeconds, 0.0f, 2.0f);
	edited |= ImGui::SliderFloat("Delayed Blow At",      &actions.attackDelayAt,      0.0f, 1.0f);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("An attack sometimes holds still in its wind-up (at this part of the way to its hit), then strikes");

	return edited;
}

// A list of animation names that grows and shrinks (the combo's extra blows, the extra staggers)
bool DrawAnimationList(const char* label, std::vector<std::string>& names, const std::map<std::string, Animation>& animations)
{
	bool edited = false;
	ImGui::PushID(label);
	ImGui::TextUnformatted(label);
	for (size_t i = 0; i < names.size(); i++)
	{
		ImGui::PushID(static_cast<int>(i));
		edited |= DrawAnimationCombo("##name", names[i], animations);
		ImGui::SameLine();
		if (ImGui::SmallButton("x"))
		{
			names.erase(names.begin() + i);
			edited = true;
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}
	if (ImGui::SmallButton("Add"))
	{
		names.emplace_back();
		edited = true;
	}
	ImGui::PopID();
	return edited;
}

// More ways to attack: the longer combo, the follow-up heavy blow, from a run, out of a dodge, the riposte, the staggers
bool DrawMoreAttacks(CharacterActionsComponent& actions, const std::map<std::string, Animation>& animations)
{
	bool edited = false;

	edited |= DrawAnimationList("Extra Light Attacks", actions.extraLightAttackAnimations, animations);
	edited |= DrawAnimationCombo("Heavy Attack 2",  actions.heavyAttack2Animation,  animations);
	edited |= DrawAnimationCombo("Running Attack",  actions.runningAttackAnimation, animations);
	edited |= ImGui::SliderFloat("Running Attack Speed", &actions.runningAttackSpeed, 0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= DrawAnimationCombo("Dodge Attack",    actions.dodgeAttackAnimation,   animations);
	edited |= DrawAnimationCombo("Riposte",         actions.riposteAnimation,       animations);
	edited |= ImGui::SliderFloat("Riposte Reach", &actions.riposteReach, 0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= DrawAnimationList("Extra Hit Reactions", actions.extraHitReactionAnimations, animations);
	edited |= DrawAnimationCombo("Heavy Hit Reaction", actions.heavyHitReactionAnimation, animations);

	// Timing of single attack animations (each blow of a combo lands at its own moment)
	ImGui::TextUnformatted("Attack Moves");
	for (size_t i = 0; i < actions.attackMoves.size(); i++)
	{
		AttackMove& move = actions.attackMoves[i];
		ImGui::PushID(static_cast<int>(i));
		edited |= DrawAnimationCombo("Move", move.animation, animations);
		edited |= ImGui::SliderFloat("Hit From", &move.hitFrom, 0.0f, 1.0f);
		edited |= ImGui::SliderFloat("Hit To",   &move.hitTo,   0.0f, 1.0f);
		edited |= ImGui::SliderFloat("Damage",   &move.damageMultiplier, 0.0f, CharacterActionsComponent::MaxDamageMultiplier);
		edited |= ImGui::SliderFloat("Cancel From", &move.cancelFrom, -1.0f, 1.0f);
		edited |= ImGui::SliderFloat("Strike Height", &move.strikeHeight, 0.0f, 3.0f);
		edited |= ImGui::Checkbox("Perilous", &move.isPerilous);
		if (ImGui::SmallButton("Remove Move"))
		{
			actions.attackMoves.erase(actions.attackMoves.begin() + i);
			edited = true;
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}
	if (ImGui::SmallButton("Add Move"))
	{
		actions.attackMoves.emplace_back();
		edited = true;
	}

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
	edited |= ImGui::SliderFloat("Dodge Cancel From", &actions.dodgeCancelFrom,  0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Hit Reaction Dodge From", &actions.hitReactionDodgeFrom, 0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Jump Speed",        &actions.jumpSpeed,        0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= ImGui::SliderFloat("Jump Move Speed",   &actions.jumpMoveSpeed,    0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= ImGui::SliderFloat("Jump Plunge Speed", &actions.jumpPlungeSpeed,  0.0f, 40.0f);
	edited |= ImGui::SliderFloat("Jump Plunge From",  &actions.jumpPlungeFrom,   0.0f, 1.0f);
	edited |= ImGui::SliderFloat("Jump Attack Damage Multiplier", &actions.jumpAttackDamageMultiplier, 0.0f, CharacterActionsComponent::MaxDamageMultiplier);
	edited |= ImGui::SliderFloat("Jump Plunge Shockwave Radius", &actions.jumpPlungeShockwaveRadius, 0.0f, CharacterActionsComponent::MaxSpeed);
	edited |= ImGui::SliderFloat("Jump Plunge Shockwave Damage", &actions.jumpPlungeShockwaveDamage, 0.0f, 200.0f);
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
	edited |= DrawMoreAttacks(*actions, modelComponent->modelData->animations);

	ImGui::Separator();
	edited |= DrawSpeeds(*actions);

	ImGui::Separator();
	edited |= DrawTiming(*actions);

	return edited;
}

}
