#include "Core/SuntaPreCompiled.h"
#include "CharacterActionsSystem.h"

#include <random>
#include <glm/glm.hpp>

#include "AnimationNames.h"
#include "CharacterSoundsSystem.h"
#include "Animation/Animation.h"
#include "EntityName.h"
#include "GameplayMath.h"
#include "LockOnSystem.h"
#include "StaminaSystem.h"
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
constexpr int AttackButton      = SUNTA_MOUSE_BUTTON_LEFT;
constexpr int HeavyAttackKey    = SUNTA_KEY_LEFT_CONTROL; // held with the attack button
constexpr int BlockButton       = SUNTA_MOUSE_BUTTON_RIGHT;
constexpr int ParryKey          = SUNTA_KEY_F;

// The upper body fades into the block (and out of it) in this time
constexpr float BlockFadeSeconds = 0.15f;

// Using an item (drinking from a flask) the character walks at this part of its walking speed
constexpr float ItemWalkSpeedFactor = 0.5f;

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
constexpr MoveWindow LeapMove       = { 0.10f, 0.30f, 0.36f };  // In the air from the take-off until the landing

// A leap lands this far in front of its target (meters): close enough for the blow, not on top of it
constexpr float LeapLandingDistance = 1.5f;

// How fast a leap turns in the air towards where its target is now (degrees per second)
constexpr float LeapTurnDegreesPerSecond = 360.0f;

// A held (delayed) blow goes on slowly at this part of its speed: a long, tense wind-up instead of a freeze
constexpr float HeldAttackPlaybackSpeed = 0.25f;

std::mt19937& GetActionsRandom()
{
	static std::mt19937 random(std::random_device{}());
	return random;
}

// The keywords the animations are found by when the component is added (the names of the Knight's animations contain them)
constexpr const char* RollKeyword         = "roll";
constexpr const char* BackstepKeyword     = "backstep";
constexpr const char* LightAttackKeywords[LightAttackCount] = { "light_1", "light_2", "light_3" };
constexpr const char* HeavyAttackKeyword  = "heavy";
constexpr const char* LeapAttackKeyword   = "leap";
constexpr const char* HitReactionKeyword  = "hit";
constexpr const char* DeathKeyword        = "death";
constexpr const char* RestKeyword         = "rest";
constexpr const char* StandUpKeyword      = "stand_up";
constexpr const char* BlockKeyword        = "block";
constexpr const char* BlockImpactKeyword  = "block_impact";
constexpr const char* ParryKeyword        = "parry";
constexpr const char* ParriedKeyword      = "parried";

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
	outCharacter.lockOn             = entityManager.GetComponent<LockOnComponent>(entityID);

	return outCharacter.controller && outCharacter.body && outCharacter.transform && outCharacter.animator && outCharacter.animations;
}

// ========== Player input ==========

CharacterAction ReadPlayerRequest(const CharacterControllerComponent& controller, bool canUseMouse)
{
	if (controller.wasDodgeTapped)
		return CharacterAction::Roll;

	if (canUseMouse && InputManager::IsMouseButtonDown(AttackButton))
		return InputManager::IsKeyPressed(HeavyAttackKey) ? CharacterAction::HeavyAttack : CharacterAction::LightAttack;

	if (InputManager::IsKeyDown(ParryKey))
		return CharacterAction::Parry;

	// Held, not tapped: the block lasts as long as it is asked for every frame
	if (canUseMouse && InputManager::IsMouseButtonPressed(BlockButton))
		return CharacterAction::Block;

	return CharacterAction::None;
}

// The keyboard of the player, or what a script asked for. Either way it is read only once
CharacterAction TakeRequest(Character& character, bool canUseMouse)
{
	CharacterAction request = character.actions->requestedAction;
	character.actions->requestedAction = CharacterAction::None;

	if (character.controller->isPlayerControlled && !character.controller->isHeldByCutscene)
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
	case CharacterAction::LeapAttack:  return actions.leapAttackAnimation;
	case CharacterAction::HitReaction: return actions.hitReactionAnimation;
	case CharacterAction::Death:       return actions.deathAnimation;
	case CharacterAction::Rest:        return actions.restAnimation;
	case CharacterAction::StandUp:     return actions.standUpAnimation;
	case CharacterAction::Block:       return actions.blockAnimation;
	case CharacterAction::BlockImpact: return actions.blockImpactAnimation;
	case CharacterAction::Parry:       return actions.parryAnimation;
	case CharacterAction::Parried:     return actions.parriedAnimation;
	case CharacterAction::Special:     return actions.specialAnimation;
	case CharacterAction::UseItem:     return actions.itemAnimation;
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

// Character Animation sees that its walking animation no longer plays, so it fades back in when the action ends
void PlayActionAnimation(Character& character, const std::string& name)
{
	character.animator->animator.CrossFadeTo(&character.animations->at(name), character.actions->blendSeconds, false);
	character.animator->animator.SetPlaybackSpeed(1.0f);
	character.animator->currentAnimationName = name;
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

	case CharacterAction::LightAttack:
	case CharacterAction::HeavyAttack:
		return actions.currentAttackStepSpeed * GetMoveFactor(AttackStepMove, progress);

	case CharacterAction::LeapAttack:
		return actions.currentLeapSpeed * GetMoveFactor(LeapMove, progress);

	// A hit character and a dead one stand where they are
	default:
		return 0.0f;
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

// A delayed blow: the animation creeps on very slowly for a moment in the wind-up (see attackDelayChance)
void UpdateAttackDelay(Character& character, float progress, float deltaTime)
{
	CharacterActionsComponent& actions = *character.actions;
	Animator& animator = character.animator->animator;

	const bool isAtHold = actions.IsAttacking() && actions.attackDelayLeft > 0.0f && progress >= actions.GetHitFrom() * actions.attackDelayAt;
	if (!isAtHold)
	{
		if (actions.isHoldingAttack)
			animator.SetPlaybackSpeed(1.0f);
		actions.isHoldingAttack = false;
		return;
	}

	actions.isHoldingAttack  = true;
	actions.attackDelayLeft -= deltaTime;
	animator.SetPlaybackSpeed(actions.attackDelayLeft > 0.0f ? HeldAttackPlaybackSpeed : 1.0f);
}

// An attack that keeps turning towards its target until it starts to hit (see attackTurnDegreesPerSecond)
void TrackTarget(EntityManager& entityManager, Character& character, float progress, float deltaTime)
{
	CharacterActionsComponent& actions = *character.actions;
	if (actions.attackTurnDegreesPerSecond <= 0.0f || !actions.IsAttacking() || progress >= actions.GetHitFrom())
		return;

	const float currentYaw = GetYawToFace(actions.actionDirection);
	const float difference = WrapDegrees(GetYawToFace(GetAttackDirection(entityManager, character)) - currentYaw);
	const float largestTurn = actions.attackTurnDegreesPerSecond * deltaTime;

	actions.actionDirection = GetDirectionFromYawAndPitch(currentYaw + glm::clamp(difference, -largestTurn, largestTurn), 0.0f);
	FaceDirection(character, actions.actionDirection);
}

bool IsRemembered(CharacterAction request)
{
	return request != CharacterAction::None && request != CharacterAction::Block;
}

bool IsDodge(CharacterAction action)
{
	return action == CharacterAction::Roll || action == CharacterAction::Backstep;
}

// Staggering after a hit can't be cut short, a rest lasts until the character stands up (and getting up until it stands),
// and nothing comes after death
bool CanBeInterrupted(CharacterAction action)
{
	switch (action)
	{
	case CharacterAction::HitReaction:
	case CharacterAction::Rest:
	case CharacterAction::StandUp:
	case CharacterAction::BlockImpact:
	case CharacterAction::Parried:
	case CharacterAction::Special:
	case CharacterAction::UseItem:
	case CharacterAction::Death:
		return false;

	default:
		return true;
	}
}

// A rest goes on after its animation ended (the character stays kneeling) until StandUp,
// and a block (the shield stays up) until the block key is let go
bool EndsWithItsAnimation(CharacterAction action)
{
	return action != CharacterAction::Rest && action != CharacterAction::Block;
}

// A roll or an attack picks its direction and turns the character, the actions started by the game don't
bool ChoosesDirection(CharacterAction action)
{
	return CanBeInterrupted(action);
}

// ========== Starting and ending actions ==========

// A roll without a held direction becomes a backstep (as in Dark Souls)
CharacterAction ResolveAction(const Character& character, CharacterAction action)
{
	if (action == CharacterAction::Roll && !HoldsDirection(*character.controller))
		return CharacterAction::Backstep;

	return action;
}

// The part of the animation's time a move covers at full speed (the slowing down counts half)
float GetMoveSeconds(const Animation& animation, const MoveWindow& window)
{
	const float seconds = animation.GetDuration() / glm::max(animation.GetTicksPerSecond(), 1.0f);
	return seconds * ((window.fullUntil - window.from) + 0.5f * (window.to - window.fullUntil));
}

// Just fast enough to land in front of the locked-on target, never faster than 'leapSpeed'. Without a target: as far as it can
float ChooseLeapSpeed(EntityManager& entityManager, const Character& character, const std::string& animationName)
{
	const CharacterActionsComponent& actions = *character.actions;

	glm::vec3 targetPoint;
	if (!character.lockOn || !LockOnSystem::TryGetTargetAimPoint(entityManager, *character.lockOn, targetPoint))
		return actions.leapSpeed;

	const float distance = glm::length(KeepOnlyHorizontalPart(targetPoint - character.transform->position));
	const float travel   = glm::max(distance - LeapLandingDistance, 0.0f);
	const float seconds  = GetMoveSeconds(character.animations->at(animationName), LeapMove);

	return glm::min(travel / glm::max(seconds, 0.01f), actions.leapSpeed);
}

// The flight of a leap homes in on where its target is NOW: each frame the speed is chosen to land in front of it in the
// time the flight has left, and the leap turns towards it (a player who moved during the take-off is still met)
void SteerLeap(EntityManager& entityManager, Character& character, float progress, float deltaTime)
{
	CharacterActionsComponent& actions = *character.actions;
	if (actions.currentAction != CharacterAction::LeapAttack || progress < LeapMove.from || progress >= LeapMove.to)
		return;

	glm::vec3 targetPoint;
	if (!character.lockOn || !LockOnSystem::TryGetTargetAimPoint(entityManager, *character.lockOn, targetPoint))
		return;

	const glm::vec3 toTarget = KeepOnlyHorizontalPart(targetPoint - character.transform->position);
	const float     travel   = glm::max(glm::length(toTarget) - LeapLandingDistance, 0.0f);

	// The time the flight still moves at full speed: the rest of the full part, and half of the slowing down
	const Animation& animation = character.animations->at(GetAnimationName(actions, CharacterAction::LeapAttack));
	const float seconds    = animation.GetDuration() / glm::max(animation.GetTicksPerSecond(), 1.0f);
	const float slowPart   = LeapMove.to - LeapMove.fullUntil;
	const float fullLeft   = progress < LeapMove.fullUntil ? (LeapMove.fullUntil - progress) + 0.5f * slowPart
	                                                       : 0.5f * (LeapMove.to - progress) * (LeapMove.to - progress) / slowPart;
	actions.currentLeapSpeed = glm::min(travel / glm::max(fullLeft * seconds, 0.05f), actions.leapSpeed);

	if (glm::length(toTarget) < SmallestHeldDirection)
		return;

	const float currentYaw  = GetYawToFace(actions.actionDirection);
	const float difference  = WrapDegrees(GetYawToFace(glm::normalize(toTarget)) - currentYaw);
	const float largestTurn = LeapTurnDegreesPerSecond * deltaTime;
	actions.actionDirection = GetDirectionFromYawAndPitch(currentYaw + glm::clamp(difference, -largestTurn, largestTurn), 0.0f);
	FaceDirection(character, actions.actionDirection);
}

// Just fast enough to end the step 'attackReach' from the target, never slower than the usual step (see attackLungeSpeed)
float ChooseAttackStepSpeed(EntityManager& entityManager, const Character& character, const std::string& animationName)
{
	const CharacterActionsComponent& actions = *character.actions;

	glm::vec3 targetPoint;
	if (actions.attackLungeSpeed <= 0.0f || !character.lockOn || !LockOnSystem::TryGetTargetAimPoint(entityManager, *character.lockOn, targetPoint))
		return actions.attackStepSpeed;

	const float distance = glm::length(KeepOnlyHorizontalPart(targetPoint - character.transform->position));
	const float travel   = glm::max(distance - actions.attackReach, 0.0f);
	const float seconds  = GetMoveSeconds(character.animations->at(animationName), AttackStepMove);

	return glm::clamp(travel / glm::max(seconds, 0.01f), actions.attackStepSpeed, glm::max(actions.attackLungeSpeed, actions.attackStepSpeed));
}

// The dice for a delayed blow are thrown when the attack starts (see attackDelayChance)
float ChooseAttackDelay(const CharacterActionsComponent& actions)
{
	if (actions.attackDelayChance <= 0.0f)
		return 0.0f;

	const float roll = std::uniform_real_distribution<float>(0.0f, 1.0f)(GetActionsRandom());
	return roll < actions.attackDelayChance ? actions.attackDelaySeconds : 0.0f;
}

void StartAction(EntityManager& entityManager, Character& character, CharacterAction action, const std::string& animationName)
{
	CharacterActionsComponent& actions = *character.actions;

	if (ChoosesDirection(action))
	{
		actions.actionDirection = ChooseActionDirection(entityManager, character, action);

		if (action != CharacterAction::Backstep)
			FaceDirection(character, actions.actionDirection);
	}

	// The block and using an item play on the upper body only, the walking animation keeps the legs (and the controller
	// keeps moving the character). Every other action plays on the whole body
	const bool isBlock       = action == CharacterAction::Block;
	const bool isUpperBody   = isBlock || action == CharacterAction::UseItem;
	if (isUpperBody)
		character.animator->animator.SetLayer(&character.animations->at(animationName), actions.upperBodyBone, BlockFadeSeconds, isBlock);
	else
	{
		character.animator->animator.ClearLayer(actions.blendSeconds);
		PlayActionAnimation(character, animationName);
	}

	if (action == CharacterAction::LeapAttack)
		actions.currentLeapSpeed = ChooseLeapSpeed(entityManager, character, animationName);

	if (IsDodge(action))
		CharacterSoundsSystem::Play(entityManager, character.entityID, &CharacterSoundsComponent::rollSound);

	if (action == CharacterAction::LightAttack || action == CharacterAction::HeavyAttack || action == CharacterAction::LeapAttack)
		actions.attackNumber++;

	if (action == CharacterAction::LightAttack || action == CharacterAction::HeavyAttack)
	{
		actions.currentAttackStepSpeed = ChooseAttackStepSpeed(entityManager, character, animationName);
		actions.attackDelayLeft        = ChooseAttackDelay(actions);
	}
	actions.isHoldingAttack = false;

	actions.currentAction                  = action;
	actions.queuedAction                   = CharacterAction::None;
	actions.actionProgress                 = 0.0f;
	actions.isInvulnerable                 = false;
	character.controller->isDrivenByAction = !isUpperBody;
}

// An action asked for by a button or a script: it needs its animation and some stamina
bool TryStartAction(EntityManager& entityManager, Character& character, CharacterAction requestedAction)
{
	CharacterActionsComponent& actions = *character.actions;
	const CharacterAction action = ResolveAction(character, requestedAction);

	const int previousLightAttackStep = actions.lightAttackStep;
	if (action == CharacterAction::LightAttack)
		actions.lightAttackStep = GetNextLightAttackStep(character);

	const std::string& animationName = GetAnimationName(actions, action);

	if (!HasAnimation(character, animationName) || !StaminaSystem::TrySpendForAction(entityManager, character.entityID, action))
	{
		actions.lightAttackStep = previousLightAttackStep;
		return false;
	}

	StartAction(entityManager, character, action, animationName);
	return true;
}

// Started by the game itself (a hit, the death): interrupts whatever the character was doing. False when it has no such animation
bool ForceAction(EntityManager& entityManager, unsigned int entityID, CharacterAction action)
{
	CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);

	Character character;
	if (!actions || !TryGetCharacter(entityManager, *actions, character))
		return false;

	const std::string& animationName = GetAnimationName(*actions, action);
	if (!HasAnimation(character, animationName))
		return false;

	StartAction(entityManager, character, action, animationName);
	return true;
}

void FinishAction(Character& character)
{
	CharacterActionsComponent& actions = *character.actions;

	if (actions.currentAction == CharacterAction::Block || actions.currentAction == CharacterAction::UseItem)
		character.animator->animator.ClearLayer(BlockFadeSeconds);

	actions.currentAction                  = CharacterAction::None;
	actions.queuedAction                   = CharacterAction::None;
	actions.isInvulnerable                 = false;
	character.controller->isDrivenByAction = false;
}

// Ends the action when its animation is over, or earlier (after 'cancelFrom') for the next action or for walking away
void EndOrChainAction(EntityManager& entityManager, Character& character, float progress)
{
	CharacterActionsComponent& actions = *character.actions;

	if (actions.currentAction == CharacterAction::Death)
		return;

	const bool canBeInterrupted = CanBeInterrupted(actions.currentAction) && progress >= actions.cancelFrom;

	if (canBeInterrupted && actions.queuedAction != CharacterAction::None)
	{
		const CharacterAction nextAction = actions.queuedAction;
		actions.queuedAction = CharacterAction::None;

		if (TryStartAction(entityManager, character, nextAction))
			return;
	}

	const bool walksAway       = canBeInterrupted && HoldsDirection(*character.controller);
	const bool animationIsOver = character.animator->animator.IsFinished() && EndsWithItsAnimation(actions.currentAction);

	if (animationIsOver || walksAway)
		FinishAction(character);
}

// Blocking lasts while it is asked for. Anything else asked for instead (a roll, an attack) starts at once
void UpdateBlock(EntityManager& entityManager, Character& character, CharacterAction request)
{
	if (request == CharacterAction::Block)
		return;

	if (request == CharacterAction::None || !TryStartAction(entityManager, character, request))
		FinishAction(character);
}

// The controller moves the character during an upper body action, but never faster than this
void LimitWalkingSpeed(Character& character, float maxSpeed)
{
	character.controller->wantsToRun = false;
	glm::vec3& velocity = character.body->velocity;
	const float speed = glm::length(glm::vec2(velocity.x, velocity.z));
	if (speed > maxSpeed)
	{
		velocity.x *= maxSpeed / speed;
		velocity.z *= maxSpeed / speed;
	}
}

// Using an item lasts as long as its animation on the upper body, whatever is pressed meanwhile
void UpdateItemUse(Character& character)
{
	const Animator& animator = character.animator->animator;

	LimitWalkingSpeed(character, character.controller->walkSpeed * ItemWalkSpeedFactor);
	character.actions->actionProgress = animator.GetLayerProgress();

	if (animator.IsLayerFinished())
		FinishAction(character);
}

void UpdateCurrentAction(EntityManager& entityManager, Character& character, CharacterAction request, float deltaTime)
{
	CharacterActionsComponent& actions = *character.actions;
	const float progress = character.animator->animator.GetProgress();

	if (actions.currentAction == CharacterAction::Block)
	{
		// Walking behind the shield
		LimitWalkingSpeed(character, character.controller->walkSpeed);
		UpdateBlock(entityManager, character, request);
		return;
	}

	if (actions.currentAction == CharacterAction::UseItem)
	{
		UpdateItemUse(character);
		return;
	}

	actions.actionProgress = progress;
	actions.isInvulnerable = IsDodge(actions.currentAction)
		&& progress >= actions.invulnerableFrom && progress <= actions.invulnerableTo;

	// Pressed during the action: remembered, so a combo doesn't need frame-perfect timing (a held block is not remembered:
	// it starts by itself once the action is over and the key is still held)
	if (IsRemembered(request) && progress >= actions.inputBufferFrom && CanBeInterrupted(actions.currentAction))
		actions.queuedAction = request;

	TrackTarget(entityManager, character, progress, deltaTime);
	SteerLeap(entityManager, character, progress, deltaTime);
	UpdateAttackDelay(character, progress, deltaTime);
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
	FillAnimationName(actions.leapAttackAnimation,   animations, LeapAttackKeyword);
	FillAnimationName(actions.hitReactionAnimation,  animations, HitReactionKeyword);
	FillAnimationName(actions.deathAnimation,        animations, DeathKeyword);
	FillAnimationName(actions.restAnimation,         animations, RestKeyword);
	FillAnimationName(actions.standUpAnimation,      animations, StandUpKeyword);
	FillAnimationName(actions.blockAnimation,        animations, BlockKeyword);
	FillAnimationName(actions.blockImpactAnimation,  animations, BlockImpactKeyword);
	FillAnimationName(actions.parryAnimation,        animations, ParryKeyword);
	FillAnimationName(actions.parriedAnimation,      animations, ParriedKeyword);
}

void WarnAboutMissingAnimation(const std::string& entityName, const char* label, const std::string& animationName, const AnimationMap& animations)
{
	if (animationName.empty())
		SUNTA_ENGINE_LOG_WARNING("Character Actions on '{0}': no {1} animation is chosen, that action is turned off", entityName, label);
	else if (animations.count(animationName) == 0)
		SUNTA_ENGINE_LOG_WARNING("Character Actions on '{0}': the {1} animation '{2}' doesn't exist in the Model", entityName, label, animationName);
}

}

void CharacterActionsSystem::Update(EntityManager& entityManager, bool canUseMouse, float deltaTime)
{
	for (CharacterActionsComponent& actions : entityManager.GetAllComponents<CharacterActionsComponent>())
	{
		Character character;
		if (!TryGetCharacter(entityManager, actions, character))
			continue;

		const CharacterAction request = TakeRequest(character, canUseMouse);

		if (actions.IsBusy())
			UpdateCurrentAction(entityManager, character, request, deltaTime);
		else if (request != CharacterAction::None && TryStartAction(entityManager, character, request))
			MoveBody(character, 0.0f);
	}
}

void CharacterActionsSystem::PlayHitReaction(EntityManager& entityManager, unsigned int entityID)
{
	// A special animation (a boss roaring into its second phase) goes on, the hit only hurts
	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);
	if (actions && actions->currentAction == CharacterAction::Special)
		return;

	ForceAction(entityManager, entityID, CharacterAction::HitReaction);
}

bool CharacterActionsSystem::PlaySpecial(EntityManager& entityManager, unsigned int entityID, const std::string& animationName)
{
	CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);
	if (!actions || actions->currentAction == CharacterAction::Death)
		return false;

	actions->specialAnimation = animationName;
	return ForceAction(entityManager, entityID, CharacterAction::Special);
}

bool CharacterActionsSystem::UseItem(EntityManager& entityManager, unsigned int entityID, const std::string& animationName)
{
	CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);

	Character character;
	if (!actions || !TryGetCharacter(entityManager, *actions, character) || !HasAnimation(character, animationName))
		return false;

	// Only a character free to act (or behind its shield: drinking lowers it)
	if (actions->IsBusy() && actions->currentAction != CharacterAction::Block)
		return false;

	actions->itemAnimation = animationName;
	StartAction(entityManager, character, CharacterAction::UseItem, animationName);
	return true;
}

void CharacterActionsSystem::PlayDeath(EntityManager& entityManager, unsigned int entityID)
{
	ForceAction(entityManager, entityID, CharacterAction::Death);
}

void CharacterActionsSystem::PlayBlockImpact(EntityManager& entityManager, unsigned int entityID)
{
	ForceAction(entityManager, entityID, CharacterAction::BlockImpact);
}

void CharacterActionsSystem::PlayParried(EntityManager& entityManager, unsigned int entityID)
{
	ForceAction(entityManager, entityID, CharacterAction::Parried);
}

void CharacterActionsSystem::PlayRest(EntityManager& entityManager, unsigned int entityID)
{
	ForceAction(entityManager, entityID, CharacterAction::Rest);
}

void CharacterActionsSystem::StandUp(EntityManager& entityManager, unsigned int entityID)
{
	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);
	if (!actions || !actions->IsResting())
		return;

	// Without an animation for getting up the character is simply standing again
	if (!ForceAction(entityManager, entityID, CharacterAction::StandUp))
		Reset(entityManager, entityID);
}

void CharacterActionsSystem::Reset(EntityManager& entityManager, unsigned int entityID)
{
	CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);

	Character character;
	if (!actions || !TryGetCharacter(entityManager, *actions, character))
		return;

	FinishAction(character);
	actions->actionProgress = 0.0f;

	// Nothing plays according to the walking animation any more, so Character Animation starts its animation again
	character.animator->currentAnimationName.clear();
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
