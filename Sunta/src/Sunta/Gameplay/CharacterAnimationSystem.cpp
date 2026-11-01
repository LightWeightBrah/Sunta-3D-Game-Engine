#include "Core/SuntaPreCompiled.h"
#include "CharacterAnimationSystem.h"

#include <cmath>
#include <string>

#include "AnimationNames.h"
#include "CharacterControllerSystem.h"
#include "EntityName.h"
#include "GameplayMath.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

using namespace GameplayMath;

// ========== Numbers used below ==========

// A character slower than this (meters per second) is standing
constexpr float StandingSpeedLimit = 0.1f;

// A push on the stick (or keys) shorter than this is not a wish to move
constexpr float SmallestMoveIntent = 0.1f;

// What the character is doing
enum class MoveState
{
	Idle,
	Walk,
	Run,
	WalkBack,
	StrafeLeft,
	StrafeRight
};

constexpr MoveState AllMoveStates[] = { MoveState::Idle, MoveState::Walk, MoveState::Run, MoveState::WalkBack, MoveState::StrafeLeft, MoveState::StrafeRight };

// ========== Small helpers ==========

std::string& GetAnimationName(CharacterAnimationComponent& component, MoveState state)
{
	switch (state)
	{
	case MoveState::Walk:        return component.walkAnimation;
	case MoveState::Run:         return component.runAnimation;
	case MoveState::WalkBack:    return component.walkBackAnimation;
	case MoveState::StrafeLeft:  return component.strafeLeftAnimation;
	case MoveState::StrafeRight: return component.strafeRightAnimation;
	default:                     return component.idleAnimation;
	}
}

// The part of the animation's name it is found by. The first name in alphabetical order wins,
// so "walk" finds Walk before Walk_Back
const char* GetKeyword(MoveState state)
{
	switch (state)
	{
	case MoveState::Walk:        return "walk";
	case MoveState::Run:         return "run";
	case MoveState::WalkBack:    return "walk_back";
	case MoveState::StrafeLeft:  return "strafe_left";
	case MoveState::StrafeRight: return "strafe_right";
	default:                     return "idle";
	}
}

const char* GetLabel(MoveState state)
{
	switch (state)
	{
	case MoveState::Walk:        return "Walk";
	case MoveState::Run:         return "Run";
	case MoveState::WalkBack:    return "Walk Back";
	case MoveState::StrafeLeft:  return "Strafe Left";
	case MoveState::StrafeRight: return "Strafe Right";
	default:                     return "Idle";
	}
}

// The state whose animation plays when this one has none: running and stepping are a kind of walking, walking is a kind of standing
bool TryGetReplacement(MoveState state, MoveState& outReplacement)
{
	switch (state)
	{
	case MoveState::Idle: return false;
	case MoveState::Walk: outReplacement = MoveState::Idle; return true;
	default:              outReplacement = MoveState::Walk; return true;
	}
}

// ========== Choosing the animation ==========

float GetHorizontalSpeed(const PhysicsBodyComponent* body)
{
	return body ? glm::length(KeepOnlyHorizontalPart(body->velocity)) : 0.0f;
}

bool WantsToMove(const CharacterControllerComponent& controller)
{
	return glm::length(KeepOnlyHorizontalPart(controller.moveDirection)) >= SmallestMoveIntent;
}

// Walk or run follows what the character WANTS (Shift held), not the measured speed,
// because the speed dips whenever the direction changes (turning the camera, reversing)
// and a dip below the walk/run limit would switch the animation for a moment
MoveState GetMoveState(float speed, const CharacterControllerComponent* controller)
{
	if (!controller)
		return speed < StandingSpeedLimit ? MoveState::Idle : MoveState::Walk;

	// Still slowing down after the keys were released counts as moving
	if (!WantsToMove(*controller) && speed < StandingSpeedLimit)
		return MoveState::Idle;

	return controller->wantsToRun ? MoveState::Run : MoveState::Walk;
}

// Facing a locked-on enemy the character doesn't turn to where it walks: the way it steps decides the animation
MoveState GetStepState(const CharacterControllerComponent& controller, const TransformComponent& transform)
{
	const glm::vec3 forward = GetDirectionFromYawAndPitch(transform.rotation.y - controller.facingOffsetDegrees, 0.0f);
	const glm::vec3 right   = glm::cross(forward, WorldUp);
	const glm::vec3 move    = KeepOnlyHorizontalPart(controller.moveDirection);

	const float forwardAmount = glm::dot(move, forward);
	const float sideAmount    = glm::dot(move, right);

	if (std::abs(forwardAmount) >= std::abs(sideAmount))
		return forwardAmount >= 0.0f ? MoveState::Walk : MoveState::WalkBack;

	return sideAmount >= 0.0f ? MoveState::StrafeRight : MoveState::StrafeLeft;
}

// The animation for the state, or the closest one when that one is not set (stepping -> walking -> standing)
bool TryFindAnimationName(CharacterAnimationComponent& component, const AnimationMap& animations, MoveState state, std::string& outName)
{
	while (true)
	{
		const std::string& name = GetAnimationName(component, state);

		if (!name.empty() && animations.count(name) > 0)
		{
			outName = name;
			return true;
		}

		if (!TryGetReplacement(state, state))
			return false;
	}
}

// Fades from whatever plays now: the previous walking animation, or the end of a roll or an attack (Character Actions)
void StartAnimation(const CharacterAnimationComponent& component, AnimatorComponent& animatorComponent, AnimationMap& animations, const std::string& name)
{
	animatorComponent.animator.CrossFadeTo(&animations.at(name), component.blendSeconds);
	animatorComponent.currentAnimationName = name;
}

// ========== Picking names when the component is added ==========

void FillNamesFromModel(CharacterAnimationComponent& component, const AnimationMap& animations)
{
	for (MoveState state : AllMoveStates)
	{
		std::string name;

		if (TryFindAnimationContaining(animations, GetKeyword(state), name))
			GetAnimationName(component, state) = name;
	}
}

}

void CharacterAnimationSystem::Update(EntityManager& entityManager)
{
	for (CharacterAnimationComponent& component : entityManager.GetAllComponents<CharacterAnimationComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(component);

		AnimatorComponent* animatorComponent = entityManager.GetComponent<AnimatorComponent>(entityID);
		ModelComponent*    modelComponent    = entityManager.GetComponent<ModelComponent>(entityID);

		// The reason is reported once when the game starts (ReportProblemsBeforePlay), not every frame
		if (!animatorComponent || !modelComponent || !modelComponent->modelData)
			continue;

		const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(entityID);

		// A roll or an attack plays its own animation (Character Actions), walking takes over again when it ends
		if (controller && controller->isDrivenByAction)
			continue;

		const float speed = GetHorizontalSpeed(entityManager.GetComponent<PhysicsBodyComponent>(entityID));
		MoveState   state = GetMoveState(speed, controller);

		const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);

		if (state == MoveState::Walk && transform && CharacterControllerSystem::IsFacingLockedTarget(entityManager, entityID))
			state = GetStepState(*controller, *transform);

		AnimationMap& animations = modelComponent->modelData->animations;

		std::string name;
		if (!TryFindAnimationName(component, animations, state, name) || name == animatorComponent->currentAnimationName)
			continue;

		StartAnimation(component, *animatorComponent, animations, name);
	}
}

void CharacterAnimationSystem::AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID)
{
	CharacterAnimationComponent& component = entityManager.AddComponent<CharacterAnimationComponent>(entityID);

	if (!entityManager.GetComponent<AnimatorComponent>(entityID))
		entityManager.AddComponent<AnimatorComponent>(entityID);

	const ModelComponent* modelComponent = entityManager.GetComponent<ModelComponent>(entityID);

	if (modelComponent && modelComponent->modelData)
		FillNamesFromModel(component, modelComponent->modelData->animations);
}

void CharacterAnimationSystem::ReportProblemsBeforePlay(EntityManager& entityManager)
{
	for (CharacterAnimationComponent& component : entityManager.GetAllComponents<CharacterAnimationComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(component);
		const std::string  name     = GetEntityName(entityManager, entityID);

		const ModelComponent* modelComponent = entityManager.GetComponent<ModelComponent>(entityID);

		if (!entityManager.GetComponent<AnimatorComponent>(entityID))
		{
			SUNTA_ENGINE_LOG_ERROR("Character Animation on '{0}' needs an Animator component, the character will not animate", name);
			continue;
		}

		if (!modelComponent || !modelComponent->modelData)
		{
			SUNTA_ENGINE_LOG_ERROR("Character Animation on '{0}' needs a Model with animations, the character will not animate", name);
			continue;
		}

		if (!entityManager.GetComponent<PhysicsBodyComponent>(entityID))
			SUNTA_ENGINE_LOG_WARNING("Character Animation on '{0}' has no Physics Body to read the speed from, the character will always stand", name);

		const AnimationMap& animations = modelComponent->modelData->animations;

		for (MoveState state : AllMoveStates)
		{
			const std::string& animationName = GetAnimationName(component, state);

			if (!animationName.empty() && animations.count(animationName) == 0)
				SUNTA_ENGINE_LOG_WARNING("Character Animation on '{0}': the {1} animation '{2}' doesn't exist in the Model", name, GetLabel(state), animationName);
		}

		if (component.idleAnimation.empty() && component.walkAnimation.empty())
			SUNTA_ENGINE_LOG_WARNING("Character Animation on '{0}': no animation is chosen, pick them in the Inspector", name);
	}
}

}
