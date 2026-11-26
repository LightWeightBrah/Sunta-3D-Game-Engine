#include "Core/SuntaPreCompiled.h"
#include "CharacterActionsSystem.h"
#include "CharacterControllerSystem.h"

#include <random>
#include <glm/glm.hpp>

#include "AnimationNames.h"
#include "CharacterSoundsSystem.h"
#include "Animation/Animation.h"
#include "EntityName.h"
#include "GameplayMath.h"
#include "LockOnSystem.h"
#include "StaminaSystem.h"
#include "HUDSystem.h"
#include "Effects/ParticleSystem.h"
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
constexpr int JumpKey           = SUNTA_KEY_SPACE;

// Dust under the feet of a character thrown back: a burst this often, of at most this many specks
constexpr float KnockbackDustEverySeconds = 0.05f;
constexpr float KnockbackDustMost         = 6.0f;

// The upper body fades into the block (and out of it) in this time
constexpr float BlockFadeSeconds = 0.15f;

// Using an item (drinking from a flask) the character walks at this part of its walking speed
constexpr float ItemWalkSpeedFactor = 0.5f;

// ========== Numbers used below ==========

constexpr int LightAttackCount = 3;

// A riposte is aimed at an enemy within this angle of where the attack goes (cosine of 60 degrees)
constexpr float RiposteFacingCosine = 0.5f;

// Faster than this part of the walking speed counts as running (a running attack)
constexpr float RunningSpeedFactor = 1.3f;

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

// A jump is over when its fall is stopped (the vertical speed rises by more than this in one frame: only the ground
// does that, gravity only takes speed away), never before 'MinAirSeconds', and at the latest after 'MaxAirSeconds'
constexpr float LandingSpeedJump = 1.5f;
constexpr float MinAirSeconds    = 0.12f;
constexpr float MaxAirSeconds    = 4.0f;
// A blow from the air reaches a locked-on enemy in about this long
constexpr float JumpAttackReachSeconds = 0.35f;
// ... and ends this close to it (meters between the two): a kick from the air is thrown from closer than a punch
constexpr float JumpAttackStopDistance = 0.45f;

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

	if (InputManager::IsKeyDown(JumpKey))
		return CharacterAction::Jump;

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

	character.actions->isBlockHeld = request == CharacterAction::Block;
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
	case CharacterAction::Jump:
		return HoldsDirection(*character.controller) ? GetHorizontalDirectionOrDefault(character.controller->moveDirection)
		                                             : GetFacingDirection(character);
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
	case 0:  return actions.lightAttack1Animation;
	case 1:  return actions.lightAttack2Animation;
	case 2:  return actions.lightAttack3Animation;
	default:
		if (step - LightAttackCount < static_cast<int>(actions.extraLightAttackAnimations.size()))
			return actions.extraLightAttackAnimations[step - LightAttackCount];
		return actions.lightAttack1Animation;
	}
}

// The three light attacks and the extra ones after them
int GetLightAttackCount(const CharacterActionsComponent& actions)
{
	return LightAttackCount + static_cast<int>(actions.extraLightAttackAnimations.size());
}

const std::string& GetHeavyAttackAnimation(const CharacterActionsComponent& actions, int step)
{
	return (step == 1 && !actions.heavyAttack2Animation.empty()) ? actions.heavyAttack2Animation : actions.heavyAttackAnimation;
}

const std::string& GetAnimationName(CharacterActionsComponent& actions, CharacterAction action)
{
	static const std::string noAnimation;

	switch (action)
	{
	case CharacterAction::Roll:        return actions.rollAnimation;
	case CharacterAction::Backstep:    return actions.backstepAnimation;
	case CharacterAction::LightAttack: return GetLightAttackAnimation(actions, actions.lightAttackStep);
	case CharacterAction::HeavyAttack: return GetHeavyAttackAnimation(actions, actions.heavyAttackStep);
	case CharacterAction::LeapAttack:  return actions.leapAttackAnimation;
	case CharacterAction::HitReaction: return actions.chosenHitReaction.empty() ? actions.hitReactionAnimation : actions.chosenHitReaction;
	case CharacterAction::Death:       return actions.deathAnimation;
	case CharacterAction::Rest:        return actions.restAnimation;
	case CharacterAction::StandUp:     return actions.standUpAnimation;
	case CharacterAction::Block:       return actions.blockAnimation;
	case CharacterAction::BlockImpact: return actions.blockImpactAnimation;
	case CharacterAction::Parry:       return actions.parryAnimation;
	case CharacterAction::Parried:     return actions.parriedAnimation;
	case CharacterAction::Special:     return actions.specialAnimation;
	case CharacterAction::UseItem:     return actions.itemAnimation;
	case CharacterAction::Cast:        return actions.castAnimation;
	case CharacterAction::Jump:        return actions.jumpAnimation;
	case CharacterAction::JumpAttack:  return actions.isPlunging ? actions.jumpHeavyAttackAnimation : actions.jumpAttackAnimation;
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

	const int nextStep = (actions.lightAttackStep + 1) % GetLightAttackCount(actions);
	return HasAnimation(character, GetLightAttackAnimation(actions, nextStep)) ? nextStep : 0;
}

// A heavy attack right after a heavy one is the follow-up (when there is one), otherwise the first
int GetNextHeavyAttackStep(const Character& character)
{
	const CharacterActionsComponent& actions = *character.actions;
	if (actions.currentAction != CharacterAction::HeavyAttack || actions.heavyAttackStep != 0)
		return 0;
	return HasAnimation(character, actions.heavyAttack2Animation) ? 1 : 0;
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

	// In the air the jump keeps carrying the character (the plunge goes straight down; on the ground it stands)
	case CharacterAction::Jump:
		return actions.currentJumpMoveSpeed;
	case CharacterAction::JumpAttack:
		return (actions.isAirborne && !actions.isPlunging) ? actions.currentJumpMoveSpeed : 0.0f;

	// Thrown back: fast at first, slowing to a stop (the direction is the action's, set by PlayKnockback)
	case CharacterAction::HitReaction:
	{
		if (actions.knockbackStartSpeed <= 0.0f)
			return 0.0f;
		const float part = glm::clamp(actions.knockbackElapsed / glm::max(actions.knockbackSlideSeconds, 0.05f), 0.0f, 1.0f);
		return actions.knockbackStartSpeed * (1.0f - part) * (1.0f - part);
	}

	// A dead character stands where it is
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

float GetAttackPlaybackSpeed(const CharacterActionsComponent& actions)
{
	return glm::clamp(actions.attackSpeed, CharacterActionsComponent::MinAttackSpeed, CharacterActionsComponent::MaxAttackSpeed);
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
			animator.SetPlaybackSpeed(GetAttackPlaybackSpeed(actions));
		actions.isHoldingAttack = false;
		return;
	}

	actions.isHoldingAttack  = true;
	actions.attackDelayLeft -= deltaTime;
	animator.SetPlaybackSpeed(actions.attackDelayLeft > 0.0f ? HeldAttackPlaybackSpeed : GetAttackPlaybackSpeed(actions));
}

// An attack that keeps turning towards its target until it starts to hit (see attackTurnDegreesPerSecond)
void TrackTarget(EntityManager& entityManager, Character& character, float progress, float deltaTime)
{
	CharacterActionsComponent& actions = *character.actions;

	// A spell turns after its target until it leaves, an attack until it starts to hit
	const bool  isCasting = actions.currentAction == CharacterAction::Cast;
	const float commitAt  = isCasting ? actions.castReleaseAt : actions.GetHitFrom();
	if (actions.attackTurnDegreesPerSecond <= 0.0f || (!actions.IsAttacking() && !isCasting) || progress >= commitAt)
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

bool IsAirAction(CharacterAction action)
{
	return action == CharacterAction::Jump || action == CharacterAction::JumpAttack;
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
	case CharacterAction::Cast:
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
	const float travel   = glm::clamp(distance - actions.attackReach, 0.0f, actions.attackLungeMaxDistance);
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
	// (only PlayKnockback makes a stagger a knockback, after it started)
	actions.knockbackStartSpeed = 0.0f;
	actions.knockbackElapsed    = 0.0f;

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

	// Off the ground: up, and along the held direction (standing still it goes straight up)
	if (action == CharacterAction::Jump)
	{
		character.body->velocity.y   = actions.jumpSpeed;
		actions.currentJumpMoveSpeed = HoldsDirection(*character.controller) ? actions.jumpMoveSpeed : 0.0f;
		actions.isAirborne           = true;
		actions.airSeconds           = 0.0f;
		actions.lastVerticalSpeed    = actions.jumpSpeed;
		actions.isPlunging           = false;
		actions.hasPlungeLanded      = false;
		CharacterSoundsSystem::Play(entityManager, character.entityID, &CharacterSoundsComponent::rollSound);
	}
	else if (action != CharacterAction::JumpAttack)
	{
		actions.isAirborne = false;
		actions.isPlunging = false;
	}

	if (IsDodge(action))
		CharacterSoundsSystem::Play(entityManager, character.entityID, &CharacterSoundsComponent::rollSound);

	if (action == CharacterAction::LightAttack || action == CharacterAction::HeavyAttack || action == CharacterAction::LeapAttack
	    || action == CharacterAction::JumpAttack)
	{
		actions.attackNumber++;
		character.animator->animator.SetPlaybackSpeed(GetAttackPlaybackSpeed(actions));
	}

	if (action == CharacterAction::LightAttack || action == CharacterAction::HeavyAttack)
	{
		actions.currentAttackStepSpeed = ChooseAttackStepSpeed(entityManager, character, animationName);
		if (actions.isRunningAttack)
			actions.currentAttackStepSpeed = glm::max(actions.currentAttackStepSpeed, actions.runningAttackSpeed);
		actions.attackDelayLeft        = ChooseAttackDelay(actions);
	}
	actions.currentAnimation = animationName;
	actions.isHoldingAttack = false;

	actions.currentAction                  = action;
	actions.queuedAction                   = CharacterAction::None;
	actions.actionProgress                 = 0.0f;
	actions.isInvulnerable                 = false;
	character.controller->isDrivenByAction = !isUpperBody;

	// A perilous blow warns as it starts: a burst over the head, a red glint of the screen (not for the player's own)
	if (actions.IsPerilousAttack() && !character.controller->isPlayerControlled)
	{
		if (!actions.perilousWarningEffect.empty())
			ParticleSystem::BurstAt(entityManager, actions.perilousWarningEffect, character.transform->position + glm::vec3(0.0f, 2.0f, 0.0f), 40);
		HUDSystem::FlashScreen(glm::vec3(0.9f, 0.08f, 0.04f), 0.35f, 0.05f, 0.22f);
	}
}

struct SpecialAttack
{
	std::string     animation;
	CharacterAction action    = CharacterAction::None;
	bool            isRunning = false;
};

// An enemy in front, near enough, staggering after a parry: open to a riposte
bool HasRiposteTarget(EntityManager& entityManager, const Character& character)
{
	const CharacterActionsComponent& actions = *character.actions;
	const glm::vec3 position = character.transform->position;
	const glm::vec3 facing   = GetAttackDirection(entityManager, character);

	for (CharacterActionsComponent& other : entityManager.GetAllComponents<CharacterActionsComponent>())
	{
		if (&other == &actions || !other.IsOpenToRiposte())
			continue;
		const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityManager.GetEntityIDForComponent(other));
		if (!transform)
			continue;
		const glm::vec3 toOther  = KeepOnlyHorizontalPart(transform->position - position);
		const float     distance = glm::length(toOther);
		if (distance <= actions.riposteReach && (distance < 0.01f || glm::dot(toOther / distance, facing) > RiposteFacingCosine))
			return true;
	}
	return false;
}

// Running: the run key held, a direction held and the body already moving faster than a walk
bool IsRunning(const Character& character)
{
	const glm::vec3& velocity = character.body->velocity;
	const float speed = glm::length(glm::vec2(velocity.x, velocity.z));
	return character.controller->wantsToRun && HoldsDirection(*character.controller) && speed > character.controller->walkSpeed * RunningSpeedFactor;
}

// Which special attack an attack request becomes now, if any: a riposte first, then one out of a dodge, then from a run
SpecialAttack ChooseSpecialAttack(EntityManager& entityManager, const Character& character)
{
	const CharacterActionsComponent& actions = *character.actions;
	SpecialAttack special;

	if (HasAnimation(character, actions.riposteAnimation) && HasRiposteTarget(entityManager, character))
	{
		special.animation = actions.riposteAnimation;
		special.action    = CharacterAction::HeavyAttack;
	}
	else if (HasAnimation(character, actions.dodgeAttackAnimation) && IsDodge(actions.currentAction))
	{
		special.animation = actions.dodgeAttackAnimation;
		special.action    = CharacterAction::LightAttack;
	}
	else if (HasAnimation(character, actions.runningAttackAnimation) && !actions.IsBusy() && IsRunning(character))
	{
		special.animation = actions.runningAttackAnimation;
		special.action    = CharacterAction::HeavyAttack;
		special.isRunning = true;
	}
	return special;
}

// An action asked for by a button or a script: it needs its animation and some stamina
bool TryStartAction(EntityManager& entityManager, Character& character, CharacterAction requestedAction)
{
	CharacterActionsComponent& actions = *character.actions;
	const CharacterAction action = ResolveAction(character, requestedAction);

	const int previousLightAttackStep = actions.lightAttackStep;
	const int previousHeavyAttackStep = actions.heavyAttackStep;
	if (action == CharacterAction::LightAttack)
		actions.lightAttackStep = GetNextLightAttackStep(character);
	if (action == CharacterAction::HeavyAttack)
		actions.heavyAttackStep = GetNextHeavyAttackStep(character);

	// A special attack instead of the usual blow (a riposte, out of a dodge, from a run), when the character has it
	CharacterAction startedAction = action;
	std::string animationName = GetAnimationName(actions, action);
	bool isRunningAttack = false;
	if (action == CharacterAction::LightAttack || action == CharacterAction::HeavyAttack)
	{
		const SpecialAttack special = ChooseSpecialAttack(entityManager, character);
		if (!special.animation.empty())
		{
			animationName   = special.animation;
			startedAction   = special.action;
			isRunningAttack = special.isRunning;
			actions.lightAttackStep = 0;
			actions.heavyAttackStep = 0;
		}
	}

	if (!HasAnimation(character, animationName) || !StaminaSystem::TrySpendForAction(entityManager, character.entityID, startedAction))
	{
		actions.lightAttackStep = previousLightAttackStep;
		actions.heavyAttackStep = previousHeavyAttackStep;
		return false;
	}

	actions.isRunningAttack = isRunningAttack;
	StartAction(entityManager, character, startedAction, animationName);
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
	actions.isRunningAttack                = false;
	actions.isAirborne                     = false;
	actions.isPlunging                     = false;
	character.controller->isDrivenByAction = false;
}

// Ends the action when its animation is over, or earlier (after 'cancelFrom') for the next action or for walking away
void EndOrChainAction(EntityManager& entityManager, Character& character, float progress)
{
	CharacterActionsComponent& actions = *character.actions;

	if (actions.currentAction == CharacterAction::Death || (IsAirAction(actions.currentAction) && actions.isAirborne))
		return;

	const AttackMove* move      = actions.FindCurrentMove();
	const float cancelFrom      = (move && move->cancelFrom >= 0.0f) ? move->cancelFrom : actions.cancelFrom;
	const bool canBeInterrupted = CanBeInterrupted(actions.currentAction) && progress >= cancelFrom;

	if (canBeInterrupted && actions.queuedAction != CharacterAction::None)
	{
		const CharacterAction nextAction = actions.queuedAction;
		actions.queuedAction = CharacterAction::None;

		if (TryStartAction(entityManager, character, nextAction))
			return;
	}

	const bool dodgeIsDone     = !IsDodge(actions.currentAction) || progress >= actions.dodgeCancelFrom;
	const bool walksAway       = canBeInterrupted && dodgeIsDone && HoldsDirection(*character.controller);
	const bool animationIsOver = character.animator->animator.IsFinished() && EndsWithItsAnimation(actions.currentAction);

	if (animationIsOver || walksAway)
	{
		// Knocked off its feet: it gets up before anything else
		const bool getsUp = actions.currentAction == CharacterAction::HitReaction && actions.getsUpAfterThrow
			&& !actions.getUpAnimation.empty();
		actions.getsUpAfterThrow = false;
		FinishAction(character);
		if (getsUp)
		{
			actions.specialAnimation = actions.getUpAnimation;
			TryStartAction(entityManager, character, CharacterAction::Special);
		}
	}
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

// True in the frame the fall is stopped: only the ground makes the vertical speed rise (gravity only takes it away)
bool HasLanded(CharacterActionsComponent& actions, const PhysicsBodyComponent& body, float deltaTime)
{
	actions.airSeconds += deltaTime;
	if (actions.airSeconds >= MaxAirSeconds)
		return true;
	if (actions.airSeconds < MinAirSeconds)
		return false;

	const float speed = body.velocity.y;
	const bool  wasFalling = actions.lastVerticalSpeed < -LandingSpeedJump * 0.5f;
	return (wasFalling && speed > actions.lastVerticalSpeed + LandingSpeedJump * 0.5f)
	    || (actions.lastVerticalSpeed <= 0.0f && std::abs(speed) < 0.02f);
}

// A jump and the attacks from it: an attack pressed in the air starts at once, the plunge hangs for its wind-up and
// then drives down, and the landing ends the jump (a jump attack plays out on the ground, see EndOrChainAction)
void UpdateAir(EntityManager& entityManager, Character& character, CharacterAction request, float progress, float deltaTime)
{
	CharacterActionsComponent& actions = *character.actions;
	PhysicsBodyComponent&      body    = *character.body;

	if (actions.currentAction == CharacterAction::Jump
	    && (request == CharacterAction::LightAttack || request == CharacterAction::HeavyAttack))
	{
		const bool isHeavy = request == CharacterAction::HeavyAttack;
		const std::string& name = isHeavy ? actions.jumpHeavyAttackAnimation : actions.jumpAttackAnimation;
		if (HasAnimation(character, name) && StaminaSystem::TrySpendForAction(entityManager, character.entityID, CharacterAction::JumpAttack))
		{
			actions.isPlunging      = isHeavy;
			actions.hasPlungeLanded = false;
			StartAction(entityManager, character, CharacterAction::JumpAttack, name);
			actions.lastVerticalSpeed = body.velocity.y;

			// At a locked-on enemy the blow from the air carries the character to it (as an attack's lunge)
			glm::vec3 targetPoint;
			if (character.lockOn && LockOnSystem::TryGetTargetAimPoint(entityManager, *character.lockOn, targetPoint))
			{
				const float distance = glm::length(KeepOnlyHorizontalPart(targetPoint - character.transform->position));
				const float travel   = glm::max(distance - JumpAttackStopDistance, 0.0f);
				actions.currentJumpMoveSpeed = glm::min(travel / JumpAttackReachSeconds, actions.jumpMoveSpeed * 1.5f);
			}
			return;
		}
	}

	actions.actionProgress = progress;

	// (what the last frame's physics did to the fall, before this frame steers it)
	// (on or against another character it is still sliding off it: the ground is further down)
	const bool hasLanded = HasLanded(actions, body, deltaTime)
		&& (actions.airSeconds >= MaxAirSeconds || !CharacterControllerSystem::IsOnTopOfCharacter(entityManager, character.entityID));

	// The plunge: a moment held in the air, then straight down
	if (!hasLanded && actions.isPlunging && actions.currentAction == CharacterAction::JumpAttack)
		body.velocity.y = progress >= actions.jumpPlungeFrom ? -actions.jumpPlungeSpeed : 0.4f;
	actions.lastVerticalSpeed = body.velocity.y;
	MoveBody(character, progress);

	if (!hasLanded)
		return;

	actions.isAirborne = false;
	SUNTA_ENGINE_LOG_INFO("Actions: '{0}' landed after {1} s (vertical speed {2}, at height {3})", GetEntityName(entityManager, character.entityID),
		actions.airSeconds, body.velocity.y, character.transform->position.y);
	CharacterSoundsSystem::Play(entityManager, character.entityID, &CharacterSoundsComponent::footstepSound);
	if (actions.currentAction == CharacterAction::Jump)
	{
		FinishAction(character);
		return;
	}
	if (actions.isPlunging)
		actions.hasPlungeLanded = true;
	MoveBody(character, progress);
}

// Thrown back: the time of the slide, and dust at the feet while it is fast
void UpdateKnockback(EntityManager& entityManager, Character& character, float deltaTime)
{
	CharacterActionsComponent& actions = *character.actions;
	if (actions.currentAction != CharacterAction::HitReaction || actions.knockbackStartSpeed <= 0.0f)
		return;

	actions.knockbackElapsed += deltaTime;
	const float part = actions.knockbackElapsed / glm::max(actions.knockbackSlideSeconds, 0.05f);
	actions.knockbackDustIn -= deltaTime;
	if (!actions.knockbackDust.empty() && part < 0.85f && actions.knockbackDustIn <= 0.0f)
	{
		actions.knockbackDustIn = KnockbackDustEverySeconds;
		const int count = 1 + static_cast<int>(KnockbackDustMost * (1.0f - part));
		ParticleSystem::BurstAt(entityManager, actions.knockbackDust, character.transform->position, count, -actions.actionDirection);
	}
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

	if (IsAirAction(actions.currentAction) && actions.isAirborne)
	{
		UpdateAir(entityManager, character, request, progress, deltaTime);
		return;
	}

	actions.actionProgress = progress;
	actions.isInvulnerable = IsDodge(actions.currentAction)
		&& progress >= actions.invulnerableFrom && progress <= actions.invulnerableTo;

	// Pressed during the action: remembered, so a combo doesn't need frame-perfect timing (a held block is not remembered:
	// it starts by itself once the action is over and the key is still held)
	if (IsRemembered(request) && progress >= actions.inputBufferFrom && CanBeInterrupted(actions.currentAction))
		actions.queuedAction = request;

	// Rolling out of a stagger once its worst is over
	if (actions.currentAction == CharacterAction::HitReaction && progress >= actions.hitReactionDodgeFrom
	    && (request == CharacterAction::Roll || request == CharacterAction::Backstep)
	    && TryStartAction(entityManager, character, request))
		return;

	UpdateKnockback(entityManager, character, deltaTime);
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

void CharacterActionsSystem::PlayHitReaction(EntityManager& entityManager, unsigned int entityID, bool isHeavyBlow)
{
	// A special animation (a boss roaring into its second phase) goes on, the hit only hurts
	CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);
	if (actions && actions->currentAction == CharacterAction::Special)
		return;

	if (actions)
	{
		// A heavy blow throws the character harder; otherwise one of its staggers at random
		if (isHeavyBlow && !actions->heavyHitReactionAnimation.empty())
			actions->chosenHitReaction = actions->heavyHitReactionAnimation;
		else
		{
			const int count = 1 + static_cast<int>(actions->extraHitReactionAnimations.size());
			const int pick  = std::uniform_int_distribution<int>(0, count - 1)(GetActionsRandom());
			actions->chosenHitReaction = pick == 0 ? actions->hitReactionAnimation : actions->extraHitReactionAnimations[pick - 1];
		}
	}

	if (!ForceAction(entityManager, entityID, CharacterAction::HitReaction) && actions && !actions->chosenHitReaction.empty())
	{
		// The chosen one is missing from the Model: the usual stagger
		actions->chosenHitReaction.clear();
		ForceAction(entityManager, entityID, CharacterAction::HitReaction);
	}
}

bool CharacterActionsSystem::PlayKnockback(EntityManager& entityManager, unsigned int entityID, const glm::vec3& direction, float meters,
	bool mayKnockDown)
{
	CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);
	if (!actions || actions->currentAction == CharacterAction::Special || actions->currentAction == CharacterAction::Death)
		return false;

	glm::vec3 along = glm::vec3(direction.x, 0.0f, direction.z);
	if (glm::length(along) < 0.001f)
		return false;
	along = glm::normalize(along);

	// Now and then a heavy blow knocks it off its feet (thrown farther, on its back, getting up after)
	static std::mt19937 knockdownRandom(std::random_device{}());
	const bool knocksDown = mayKnockDown && !actions->knockdownAnimation.empty() && actions->knockdownChance > 0.0f
		&& std::uniform_real_distribution<float>(0.0f, 1.0f)(knockdownRandom) < actions->knockdownChance;

	actions->chosenHitReaction = knocksDown ? actions->knockdownAnimation
		: !actions->knockbackAnimation.empty() ? actions->knockbackAnimation : actions->heavyHitReactionAnimation;
	if (!ForceAction(entityManager, entityID, CharacterAction::HitReaction))
		return false;

	actions->knockbackSlideSeconds = knocksDown ? actions->knockdownSeconds : actions->knockbackSeconds;
	SUNTA_ENGINE_LOG_INFO("CharacterActions: thrown {0} m{1}", meters, knocksDown ? " - knocked down" : "");
	actions->getsUpAfterThrow      = knocksDown;
	if (knocksDown)
		meters *= actions->knockdownDistanceMultiplier;
	// (the distance of a speed falling to 0 as (1 - t)^2 over T seconds is speed * T / 3)
	actions->knockbackStartSpeed = 3.0f * glm::max(meters, 0.0f) / glm::max(actions->knockbackSlideSeconds, 0.05f);
	actions->knockbackDustIn     = 0.0f;
	actions->actionDirection     = along;
	// (facing the blow: it slides back, not sideways)
	Character character;
	if (TryGetCharacter(entityManager, *actions, character))
		FaceDirection(character, -along);
	return true;
}

void CharacterActionsSystem::RestartAction(EntityManager& entityManager, unsigned int entityID)
{
	const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);
	if (!actions || actions->currentAction == CharacterAction::None)
		return;

	ForceAction(entityManager, entityID, actions->currentAction);
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

bool CharacterActionsSystem::Cast(EntityManager& entityManager, unsigned int entityID, const std::string& animationName, float releaseAt,
	bool isForced)
{
	CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(entityID);

	Character character;
	if (!actions || !TryGetCharacter(entityManager, *actions, character) || !HasAnimation(character, animationName))
		return false;

	// Only a character free to act (unless a script forces it)
	if (!isForced && (actions->IsBusy() || !StaminaSystem::TrySpendForAction(entityManager, entityID, CharacterAction::Cast)))
		return false;

	actions->castAnimation = animationName;
	actions->castReleaseAt = releaseAt;
	StartAction(entityManager, character, CharacterAction::Cast, animationName);

	// Facing the target from the start, then turning after it (TrackTarget)
	actions->actionDirection = GetAttackDirection(entityManager, character);
	FaceDirection(character, actions->actionDirection);
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
