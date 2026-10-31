#include "Core/SuntaPreCompiled.h"
#include "CharacterAnimationSystem.h"

#include <string>

#include "AnimationNames.h"
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

// What the character is doing. The order matters: a missing animation is replaced by the one before it
enum class MoveState
{
	Idle,
	Walk,
	Run
};

constexpr MoveState AllMoveStates[] = { MoveState::Idle, MoveState::Walk, MoveState::Run };

// ========== Small helpers ==========

std::string& GetAnimationName(CharacterAnimationComponent& component, MoveState state)
{
	switch (state)
	{
	case MoveState::Walk: return component.walkAnimation;
	case MoveState::Run:  return component.runAnimation;
	default:              return component.idleAnimation;
	}
}

const char* GetKeyword(MoveState state)
{
	switch (state)
	{
	case MoveState::Walk: return "walk";
	case MoveState::Run:  return "run";
	default:              return "idle";
	}
}

const char* GetLabel(MoveState state)
{
	switch (state)
	{
	case MoveState::Walk: return "Walk";
	case MoveState::Run:  return "Run";
	default:              return "Idle";
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

// The animation for the state, or the closest one below it when that one is not set (running -> walking -> standing)
bool TryFindAnimationName(CharacterAnimationComponent& component, const AnimationMap& animations, MoveState state, std::string& outName)
{
	for (int stateIndex = static_cast<int>(state); stateIndex >= 0; stateIndex--)
	{
		const std::string& name = GetAnimationName(component, AllMoveStates[stateIndex]);

		if (!name.empty() && animations.count(name) > 0)
		{
			outName = name;
			return true;
		}
	}

	return false;
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

		const float     speed = GetHorizontalSpeed(entityManager.GetComponent<PhysicsBodyComponent>(entityID));
		const MoveState state = GetMoveState(speed, controller);

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

		if (component.idleAnimation.empty() && component.walkAnimation.empty() && component.runAnimation.empty())
			SUNTA_ENGINE_LOG_WARNING("Character Animation on '{0}': no animation is chosen, pick them in the Inspector", name);
	}
}

}
