#include "Core/SuntaPreCompiled.h"
#include "CharacterControllerSystem.h"

#include <glm/glm.hpp>

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

// ========== Keys of the player ==========

constexpr int MoveForwardKey  = SUNTA_KEY_W;
constexpr int MoveBackwardKey = SUNTA_KEY_S;
constexpr int MoveLeftKey     = SUNTA_KEY_A;
constexpr int MoveRightKey    = SUNTA_KEY_D;
constexpr int RunAndDodgeKey  = SUNTA_KEY_LEFT_SHIFT;

// Holding the key longer than this runs, a shorter press is a tap that dodges (as in Dark Souls)
constexpr float RunAfterHoldingSeconds = 0.25f;

// ========== Numbers used below ==========

// A weak push on a stick should not make the character spin on the spot, so turning starts above this
constexpr float SmallestMoveToTurn = 0.1f;

// The body given to a new character (about the size and weight of a person, standing on the entity position)
constexpr float CharacterMass = 80.0f;
const glm::vec3 CharacterColliderHalfExtents = glm::vec3(0.3f, 0.9f, 0.3f);
const glm::vec3 CharacterColliderOffset      = glm::vec3(0.0f, CharacterColliderHalfExtents.y, 0.0f);

// ========== Small helpers ==========

// A vector longer than 1 is shortened to 1, a shorter one stays as it is (so a gentle push means a slow walk)
glm::vec3 LimitLengthToOne(const glm::vec3& vector)
{
	return glm::length(vector) > 1.0f ? glm::normalize(vector) : vector;
}

// Moves 'current' towards 'target' by at most 'maxStep' (and never past it)
glm::vec3 MoveTowards(const glm::vec3& current, const glm::vec3& target, float maxStep)
{
	const glm::vec3 difference = target - current;
	const float distance = glm::length(difference);

	if (distance <= maxStep || distance < SmallestUsableLength)
		return target;

	return current + difference / distance * maxStep;
}

// ========== Player input ==========

// +1 when only the positive key is held, -1 when only the negative one, 0 when none or both
float GetKeyAxis(int positiveKey, int negativeKey)
{
	float axis = 0.0f;

	if (InputManager::IsKeyPressed(positiveKey)) axis += 1.0f;
	if (InputManager::IsKeyPressed(negativeKey)) axis -= 1.0f;

	return axis;
}

// W is "away from the camera", D is "to the right of the camera"
glm::vec3 ReadPlayerMoveDirection(const glm::vec3& cameraForward)
{
	const glm::vec3 forward = GetHorizontalDirectionOrDefault(cameraForward);
	const glm::vec3 right   = glm::cross(forward, WorldUp);

	const float forwardAmount = GetKeyAxis(MoveForwardKey, MoveBackwardKey);
	const float rightAmount   = GetKeyAxis(MoveRightKey,   MoveLeftKey);

	// Holding two keys at once (W and D) would be faster than one, the limit prevents it
	return LimitLengthToOne(forward * forwardAmount + right * rightAmount);
}

// The dodge happens when a short press is let go, so a press that turns out long becomes running instead
void ReadRunAndDodgeKey(CharacterControllerComponent& controller, float deltaTime)
{
	const bool isHeld        = InputManager::IsKeyPressed(RunAndDodgeKey);
	const bool wasHeldBefore = controller.runKeyHeldSeconds > 0.0f;

	controller.wasDodgeTapped    = !isHeld && wasHeldBefore && controller.runKeyHeldSeconds < RunAfterHoldingSeconds;
	controller.runKeyHeldSeconds = isHeld ? controller.runKeyHeldSeconds + deltaTime : 0.0f;
	controller.wantsToRun        = controller.runKeyHeldSeconds >= RunAfterHoldingSeconds;
}

void ReadPlayerInput(CharacterControllerComponent& controller, const glm::vec3& cameraForward, float deltaTime)
{
	controller.moveDirection = ReadPlayerMoveDirection(cameraForward);
	ReadRunAndDodgeKey(controller, deltaTime);
}

// ========== Movement ==========

float GetTopSpeed(const CharacterControllerComponent& controller)
{
	return controller.wantsToRun ? controller.runSpeed : controller.walkSpeed;
}

glm::vec3 GetWantedHorizontalVelocity(const CharacterControllerComponent& controller)
{
	const glm::vec3 direction = LimitLengthToOne(KeepOnlyHorizontalPart(controller.moveDirection));

	return direction * GetTopSpeed(controller);
}

// Only the speed along the ground is set here. Falling (the speed up and down) stays in the hands of gravity
void MoveBody(PhysicsBodyComponent& body, const CharacterControllerComponent& controller, float deltaTime)
{
	const glm::vec3 currentVelocity = KeepOnlyHorizontalPart(body.velocity);
	const glm::vec3 wantedVelocity  = GetWantedHorizontalVelocity(controller);
	const float maxSpeedChange      = controller.acceleration * deltaTime;

	const glm::vec3 newVelocity = MoveTowards(currentVelocity, wantedVelocity, maxSpeedChange);

	body.velocity.x = newVelocity.x;
	body.velocity.z = newVelocity.z;

	// A sleeping body ignores its velocity, so a moving character must be awake
	if (newVelocity != glm::vec3(0.0f))
		PhysicsSystem::WakeUp(body);
}

// The character stays upright and only turns around the up axis
void TurnTowards(TransformComponent& transform, const CharacterControllerComponent& controller, const glm::vec3& direction, float deltaTime)
{
	const float currentYaw = transform.rotation.y;
	const float wantedYaw  = GetYawToFace(direction) + controller.facingOffsetDegrees;

	const float yawLeftToTurn = WrapDegrees(wantedYaw - currentYaw);
	const float maxTurn       = controller.turnSpeed * deltaTime;
	const float turn          = glm::clamp(yawLeftToTurn, -maxTurn, maxTurn);

	transform.rotation = glm::vec3(0.0f, WrapDegrees(currentYaw + turn), 0.0f);
	transform.SyncQuaternionFromEuler();
	transform.isDirty = true;
}

// Towards the way it walks
void TurnTowardsMoveDirection(TransformComponent& transform, const CharacterControllerComponent& controller, float deltaTime)
{
	const glm::vec3 horizontalMove = KeepOnlyHorizontalPart(controller.moveDirection);

	if (glm::length(horizontalMove) < SmallestMoveToTurn)
		return;

	TurnTowards(transform, controller, horizontalMove, deltaTime);
}

// Towards a point, even when standing still (the character locked on to an enemy walks around it and keeps facing it)
void TurnTowardsPoint(TransformComponent& transform, const CharacterControllerComponent& controller, const glm::vec3& point, float deltaTime)
{
	const glm::vec3 horizontalDirection = KeepOnlyHorizontalPart(point - transform.position);

	if (glm::length(horizontalDirection) < SmallestUsableLength)
		return;

	TurnTowards(transform, controller, horizontalDirection, deltaTime);
}

// The enemy the character is locked on to. While running the character faces the way it runs instead, so it can run away
bool TryGetPointToFace(EntityManager& entityManager, unsigned int entityID, const CharacterControllerComponent& controller, glm::vec3& outPoint)
{
	if (controller.wantsToRun)
		return false;

	const LockOnComponent* lockOn = entityManager.GetComponent<LockOnComponent>(entityID);

	return lockOn && LockOnSystem::TryGetTargetAimPoint(entityManager, *lockOn, outPoint);
}

// ========== Adding the required parts ==========

void AddPhysicsBodyIfMissing(EntityManager& entityManager, unsigned int entityID)
{
	if (entityManager.GetComponent<PhysicsBodyComponent>(entityID))
		return;

	PhysicsBodyComponent& body = entityManager.AddComponent<PhysicsBodyComponent>(entityID);

	body.mass        = CharacterMass;
	body.restitution = 0.0f; // a character doesn't bounce
	body.friction    = 0.0f; // no friction, so the character doesn't stick to walls or slow down on the floor

	// The controller turns the character itself, physics must never tip it over
	body.freezeRotationX = true;
	body.freezeRotationY = true;
	body.freezeRotationZ = true;
}

void AddBoxColliderIfMissing(EntityManager& entityManager, unsigned int entityID)
{
	if (entityManager.GetComponent<BoxColliderComponent>(entityID))
		return;

	BoxColliderComponent& collider = entityManager.AddComponent<BoxColliderComponent>(entityID);

	collider.halfExtents = CharacterColliderHalfExtents;
	collider.localOffset = CharacterColliderOffset;
}

}

void CharacterControllerSystem::Update(EntityManager& entityManager, float deltaTime, const glm::vec3& cameraForward)
{
	for (CharacterControllerComponent& controller : entityManager.GetAllComponents<CharacterControllerComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(controller);

		TransformComponent*   transform = entityManager.GetComponent<TransformComponent>(entityID);
		PhysicsBodyComponent* body      = entityManager.GetComponent<PhysicsBodyComponent>(entityID);

		// The reason is reported once when the game starts (ReportProblemsBeforePlay), not every frame
		if (!transform || !body || body->isKinematic)
			continue;

		if (controller.isPlayerControlled)
			ReadPlayerInput(controller, cameraForward, deltaTime);

		// An empty stamina bar stops running (walking is free)
		if (controller.wantsToRun && !StaminaSystem::HasStaminaLeft(entityManager, entityID))
			controller.wantsToRun = false;

		if (controller.isDrivenByAction)
			continue;

		MoveBody(*body, controller, deltaTime);

		glm::vec3 pointToFace;
		if (TryGetPointToFace(entityManager, entityID, controller, pointToFace))
			TurnTowardsPoint(*transform, controller, pointToFace, deltaTime);
		else
			TurnTowardsMoveDirection(*transform, controller, deltaTime);
	}
}

bool CharacterControllerSystem::TryFindPlayer(EntityManager& entityManager, unsigned int& outEntityID)
{
	for (CharacterControllerComponent& controller : entityManager.GetAllComponents<CharacterControllerComponent>())
	{
		if (!controller.isPlayerControlled)
			continue;

		outEntityID = entityManager.GetEntityIDForComponent(controller);
		return true;
	}

	return false;
}

void CharacterControllerSystem::AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID)
{
	entityManager.AddComponent<CharacterControllerComponent>(entityID);

	AddPhysicsBodyIfMissing(entityManager, entityID);
	AddBoxColliderIfMissing(entityManager, entityID);
}

void CharacterControllerSystem::ReportProblemsBeforePlay(EntityManager& entityManager)
{
	for (CharacterControllerComponent& controller : entityManager.GetAllComponents<CharacterControllerComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(controller);
		const std::string name      = GetEntityName(entityManager, entityID);

		const PhysicsBodyComponent* body = entityManager.GetComponent<PhysicsBodyComponent>(entityID);

		if (!body)
			SUNTA_ENGINE_LOG_ERROR("Character Controller on '{0}' needs a Physics Body component, the character will not move", name);
		else if (body->isKinematic)
			SUNTA_ENGINE_LOG_ERROR("Character Controller on '{0}': turn off 'isKinematic' on its Physics Body, the character will not move", name);

		if (!entityManager.GetComponent<BoxColliderComponent>(entityID))
			SUNTA_ENGINE_LOG_WARNING("Character Controller on '{0}' has no Box Collider, the character will fall through the floor", name);
	}
}

}
