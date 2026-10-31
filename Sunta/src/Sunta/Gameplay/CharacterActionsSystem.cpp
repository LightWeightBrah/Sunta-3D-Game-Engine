#include "Core/SuntaPreCompiled.h"
#include "CharacterActionsSystem.h"

#include <glm/glm.hpp>

#include "AnimationNames.h"
#include "EntityName.h"
#include "GameplayMath.h"
#include "LockOnSystem.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Physics/PhysicsSystem.h"

namespace Sunta
{

namespace
{

using namespace GameplayMath;

// ========== Keys and buttons of the player ==========

// The roll is a tap of the run key, the Character Controller reads that key (wasDodgeTapped)
constexpr int LightAttackButton = SUNTA_MOUSE_BUTTON_LEFT;
constexpr int HeavyAttackButton = SUNTA_MOUSE_BUTTON_RIGHT;

// ========== Numbers used below ==========

constexpr int LightAttackCount = 3;

// A push on the stick (or keys) shorter than this doesn't choose a direction
constexpr float SmallestHeldDirection = 0.1f;

// When an action moves the character, as fractions of its animation:
// not at all before 'from', at full speed until 'fullUntil', slowing down until it stops at 'to'
struct MoveWindow
{
	float from;
	float fullUntil;
	float to;
};

constexpr MoveWindow RollMove       = { 0.05f, 0.55f, 0.80f };
constexpr MoveWindow BackstepMove   = { 0.05f, 0.40f, 0.60f };
constexpr MoveWindow AttackStepMove = { 0.20f, 0.35f, 0.50f };

// The keywords the animations are found by when the component is added (the names of the Knight's animations contain them)
constexpr const char* RollKeyword         = "roll";
constexpr const char* BackstepKeyword     = "backstep";
constexpr const char* LightAttackKeywords[LightAttackCount] = { "light_1", "light_2", "light_3" };
constexpr const char* HeavyAttackKeyword  = "heavy";

// ========== The parts of one character ==========

// Everything an action needs from the character, looked up once per frame
struct Character
{
	unsigned int                  entityID           = 0;
	CharacterActionsComponent*    actions            = nullptr;
	CharacterControllerComponent* controller         = nullptr;
	PhysicsBodyComponent*         body               = nullptr;
	TransformComponent*           transform          = nullptr;
	AnimatorComponent*            animator           = nullptr;
	AnimationMap*                 animations         = nullptr;
	CharacterAnimationComponent*  characterAnimation = nullptr;
	LockOnComponent*              lockOn             = nullptr;
};

// False when a part needed to act is missing (the reason is logged once by ReportProblemsBeforePlay)
bool TryGetCharacter(EntityManager& entityManager, CharacterActionsComponent& actions, Character& outCharacter)
{
	const unsigned int entityID = entityManager.GetEntityIDForComponent(actions);

	ModelComponent* model = entityManager.GetComponent<ModelComponent>(entityID);

	outCharacter.entityID           = entityID;
	outCharacter.actions            = &actions;
	outCharacter.controller         = entityManager.GetComponent<CharacterControllerComponent>(entityID);
	outCharacter.body               = entityManager.GetComponent<PhysicsBodyComponent>(entityID);
	outCharacter.transform          = entityManager.GetComponent<TransformComponent>(entityID);
	outCharacter.animator           = entityManager.GetComponent<AnimatorComponent>(entityID);
	outCharacter.animations         = (model && model->modelData) ? &model->modelData->animations : nullptr;
	outCharacter.characterAnimation = entityManager.GetComponent<CharacterAnimationComponent>(entityID);
	outCharacter.lockOn             = entityManager.GetComponent<LockOnComponent>(entityID);

	return outCharacter.controller && outCharacter.body && outCharacter.transform && outCharacter.animator && outCharacter.animations;
}

// ========== Player input ==========

CharacterAction ReadPlayerRequest(const CharacterControllerComponent& controller, bool canUseMouse)
{
	if (controller.wasDodgeTapped)
		return CharacterAction::Roll;

	if (canUseMouse && InputManager::IsMouseButtonDown(LightAttackButton))
		return CharacterAction::LightAttack;

	if (canUseMouse && InputManager::IsMouseButtonDown(HeavyAttackButton))
		return CharacterAction::HeavyAttack;

	return CharacterAction::None;
}

// The keyboard of the player, or what a script asked for. Either way it is read only once
CharacterAction TakeRequest(Character& character, bool canUseMouse)
{
	CharacterAction request = character.actions->requestedAction;
	character.actions->requestedAction = CharacterAction::None;

	if (character.controller->isPlayerControlled)
	{
		const CharacterAction playerRequest = ReadPlayerRequest(*character.controller, canUseMouse);
		if (playerRequest != CharacterAction::None)
			request = playerRequest;
	}

	return request;
}

// ========== Directions ==========

bool HoldsDirection(const CharacterControllerComponent& controller)
{
	return glm::length(KeepOnlyHorizontalPart(controller.moveDirection)) >= SmallestHeldDirection;
}

glm::vec3 GetFacingDirection(const Character& character)
{
	const float yaw = character.transform->rotation.y - character.controller->facingOffsetDegrees;
	return GetDirectionFromYawAndPitch(yaw, 0.0f);
}

// Attacks go at the locked-on enemy, otherwise where the player holds the stick, otherwise straight ahead
glm::vec3 GetAttackDirection(EntityManager& entityManager, const Character& character)
{
	glm::vec3 targetPoint;
	if (character.lockOn && LockOnSystem::TryGetTargetAimPoint(entityManager, *character.lockOn, targetPoint))
		return GetHorizontalDirectionOrDefault(targetPoint - character.transform->position);

	if (HoldsDirection(*character.controller))
		return GetHorizontalDirectionOrDefault(character.controller->moveDirection);

	return GetFacingDirection(character);
}

glm::vec3 ChooseActionDirection(EntityManager& entityManager, const Character& character, CharacterAction action)
{
	switch (action)
	{
	case CharacterAction::Roll:     return GetHorizontalDirectionOrDefault(character.controller->moveDirection);
	case CharacterAction::Backstep: return -GetFacingDirection(character);
	default:                        return GetAttackDirection(entityManager, character);
	}
}

// Turns at once: an action commits to its direction when it starts
void FaceDirection(Character& character, const glm::vec3& direction)
{
	TransformComponent& transform = *character.transform;

	transform.rotation = glm::vec3(0.0f, WrapDegrees(GetYawToFace(direction) + character.controller->facingOffsetDegrees), 0.0f);
	transform.SyncQuaternionFromEuler();
	transform.isDirty = true;
}

// ========== Animations ==========

std::string& GetLightAttackAnimation(CharacterActionsComponent& actions, int step)
{
	switch (step)
	{
	case 1:  return actions.lightAttack2Animation;
	case 2:  return actions.lightAttack3Animation;
	default: return actions.lightAttack1Animation;
	}
}

const std::string& GetAnimationName(CharacterActionsComponent& actions, CharacterAction action)
{
	static const std::string noAnimation;

	switch (action)
	{
	case CharacterAction::Roll:        return actions.rollAnimation;
	case CharacterAction::Backstep:    return actions.backstepAnimation;
	case CharacterAction::LightAttack: return GetLightAttackAnimation(actions, actions.lightAttackStep);
	case CharacterAction::HeavyAttack: return actions.heavyAttackAnimation;
	default:                           return noAnimation;
	}
}

bool HasAnimation(const Character& character, const std::string& name)
{
	return !name.empty() && character.animations->count(name) > 0;
}

// The next attack of the combo when a light attack is still playing, otherwise the first one.
// A combo shorter than three attacks (an animation not set) starts over
int GetNextLightAttackStep(const Character& character)
{
	CharacterActionsComponent& actions = *character.actions;

	if (actions.currentAction != CharacterAction::LightAttack)
		return 0;

	const int nextStep = (actions.lightAttackStep + 1) % LightAttackCount;
	return HasAnimation(character, GetLightAttackAnimation(actions, nextStep)) ? nextStep : 0;
}

void PlayActionAnimation(Character& character, const std::string& name)
{
	character.animator->animator.CrossFadeTo(&character.animations->at(name), character.actions->blendSeconds, false);
	character.animator->currentAnimationName = name;

	// Character Animation sees that its walking animation no longer plays, so it fades back in when the action ends
	if (character.characterAnimation)
		character.characterAnimation->playingAnimation = name;
}

// ========== Moving ==========

float GetMoveFactor(const MoveWindow& window, float progress)
{
	if (progress < window.from || progress >= window.to)
		return 0.0f;

	if (progress <= window.fullUntil)
		return 1.0f;

	return 1.0f - (progress - window.fullUntil) / (window.to - window.fullUntil);
}

float GetActionSpeed(const CharacterActionsComponent& actions, CharacterAction action, float progress)
{
	switch (action)
	{
	case CharacterAction::Roll:     return actions.rollSpeed       * GetMoveFactor(RollMove,       progress);
	case CharacterAction::Backstep: return actions.backstepSpeed   * GetMoveFactor(BackstepMove,   progress);
	default:                        return actions.attackStepSpeed * GetMoveFactor(AttackStepMove, progress);
	}
}

// Only the speed along the ground is set, falling stays in the hands of gravity
void MoveBody(Character& character, float progress)
{
	const CharacterActionsComponent& actions = *character.actions;
	const glm::vec3 velocity = actions.actionDirection * GetActionSpeed(actions, actions.currentAction, progress);

	character.body->velocity.x = velocity.x;
	character.body->velocity.z = velocity.z;

	PhysicsSystem::WakeUp(*character.body);
}

bool IsDodge(CharacterAction action)
{
	return action == CharacterAction::Roll || action == CharacterAction::Backstep;
}

// ========== Starting and ending actions ==========

// A roll without a held direction becomes a backstep (as in Dark Souls)
CharacterAction ResolveAction(const Character& character, CharacterAction action)
{
	if (action == CharacterAction::Roll && !HoldsDirection(*character.controller))
		return CharacterAction::Backstep;

	return action;
}

bool TryStartAction(EntityManager& entityManager, Character& character, CharacterAction requestedAction)
{
	CharacterActionsComponent& actions = *character.actions;
	const CharacterAction action = ResolveAction(character, requestedAction);

	if (action == CharacterAction::LightAttack)
		actions.lightAttackStep = GetNextLightAttackStep(character);

	const std::string& animationName = GetAnimationName(actions, action);
	if (!HasAnimation(character, animationName))
		return false;

	actions.actionDirection = ChooseActionDirection(entityManager, character, action);

	if (action != CharacterAction::Backstep)
		FaceDirection(character, actions.actionDirection);

	PlayActionAnimation(character, animationName);

	actions.currentAction                  = action;
	actions.queuedAction                   = CharacterAction::None;
	character.controller->isDrivenByAction = true;

	return true;
}

void FinishAction(Character& character)
{
	CharacterActionsComponent& actions = *character.actions;

	actions.currentAction                  = CharacterAction::None;
	actions.queuedAction                   = CharacterAction::None;
	actions.isInvulnerable                 = false;
	character.controller->isDrivenByAction = false;
}

// Ends the action when its animation is over, or earlier (after 'cancelFrom') for the next action or for walking away
void EndOrChainAction(EntityManager& entityManager, Character& character, float progress)
{
	CharacterActionsComponent& actions = *character.actions;
	const bool canBeInterrupted = progress >= actions.cancelFrom;

	if (canBeInterrupted && actions.queuedAction != CharacterAction::None)
	{
		const CharacterAction nextAction = actions.queuedAction;
		actions.queuedAction = CharacterAction::None;

		if (TryStartAction(entityManager, character, nextAction))
			return;
	}

	const bool walksAway = canBeInterrupted && HoldsDirection(*character.controller);

	if (character.animator->animator.IsFinished() || walksAway)
		FinishAction(character);
}

void UpdateCurrentAction(EntityManager& entityManager, Character& character, CharacterAction request)
{
	CharacterActionsComponent& actions = *character.actions;
	const float progress = character.animator->animator.GetProgress();

	actions.isInvulnerable = IsDodge(actions.currentAction)
		&& progress >= actions.invulnerableFrom && progress <= actions.invulnerableTo;

	// Pressed during the action: remembered, so a combo doesn't need frame-perfect timing
	if (request != CharacterAction::None && progress >= actions.inputBufferFrom)
		actions.queuedAction = request;

	MoveBody(character, progress);
	EndOrChainAction(entityManager, character, progress);
}

// ========== Picking names when the component is added ==========

void FillAnimationName(std::string& name, const AnimationMap& animations, const char* keyword)
{
	std::string foundName;
	if (TryFindAnimationContaining(animations, keyword, foundName))
		name = foundName;
}

void FillNamesFromModel(CharacterActionsComponent& actions, const AnimationMap& animations)
{
	FillAnimationName(actions.rollAnimation,         animations, RollKeyword);
	FillAnimationName(actions.backstepAnimation,     animations, BackstepKeyword);
	FillAnimationName(actions.lightAttack1Animation, animations, LightAttackKeywords[0]);
	FillAnimationName(actions.lightAttack2Animation, animations, LightAttackKeywords[1]);
	FillAnimationName(actions.lightAttack3Animation, animations, LightAttackKeywords[2]);
	FillAnimationName(actions.heavyAttackAnimation,  animations, HeavyAttackKeyword);
}

void WarnAboutMissingAnimation(const std::string& entityName, const char* label, const std::string& animationName, const AnimationMap& animations)
{
	if (animationName.empty())
		SUNTA_ENGINE_LOG_WARNING("Character Actions on '{0}': no {1} animation is chosen, that action is turned off", entityName, label);
	else if (animations.count(animationName) == 0)
		SUNTA_ENGINE_LOG_WARNING("Character Actions on '{0}': the {1} animation '{2}' doesn't exist in the Model", entityName, label, animationName);
}

}

void CharacterActionsSystem::Update(EntityManager& entityManager, bool canUseMouse)
{
	for (CharacterActionsComponent& actions : entityManager.GetAllComponents<CharacterActionsComponent>())
	{
		Character character;
		if (!TryGetCharacter(entityManager, actions, character))
			continue;

		const CharacterAction request = TakeRequest(character, canUseMouse);

		if (actions.IsBusy())
			UpdateCurrentAction(entityManager, character, request);
		else if (request != CharacterAction::None && TryStartAction(entityManager, character, request))
			MoveBody(character, 0.0f);
	}
}

void CharacterActionsSystem::AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID)
{
	CharacterActionsComponent& actions = entityManager.AddComponent<CharacterActionsComponent>(entityID);

	if (!entityManager.GetComponent<AnimatorComponent>(entityID))
		entityManager.AddComponent<AnimatorComponent>(entityID);

	const ModelComponent* model = entityManager.GetComponent<ModelComponent>(entityID);

	if (model && model->modelData)
		FillNamesFromModel(actions, model->modelData->animations);
}

void CharacterActionsSystem::ReportProblemsBeforePlay(EntityManager& entityManager)
{
	for (CharacterActionsComponent& actions : entityManager.GetAllComponents<CharacterActionsComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(actions);
		const std::string  name     = GetEntityName(entityManager, entityID);

		if (!entityManager.GetComponent<CharacterControllerComponent>(entityID))
			SUNTA_ENGINE_LOG_ERROR("Character Actions on '{0}' needs a Character Controller, the character will not roll or attack", name);

		if (!entityManager.GetComponent<AnimatorComponent>(entityID))
			SUNTA_ENGINE_LOG_ERROR("Character Actions on '{0}' needs an Animator, the character will not roll or attack", name);

		const ModelComponent* model = entityManager.GetComponent<ModelComponent>(entityID);
		if (!model || !model->modelData)
		{
			SUNTA_ENGINE_LOG_ERROR("Character Actions on '{0}' needs a Model with animations, the character will not roll or attack", name);
			continue;
		}

		const AnimationMap& animations = model->modelData->animations;

		WarnAboutMissingAnimation(name, "Roll",           actions.rollAnimation,         animations);
		WarnAboutMissingAnimation(name, "Backstep",       actions.backstepAnimation,     animations);
		WarnAboutMissingAnimation(name, "Light Attack 1", actions.lightAttack1Animation, animations);
		WarnAboutMissingAnimation(name, "Heavy Attack",   actions.heavyAttackAnimation,  animations);
	}
}

}
