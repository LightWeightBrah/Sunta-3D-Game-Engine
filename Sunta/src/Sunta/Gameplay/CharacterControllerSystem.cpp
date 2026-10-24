#include "Core/SuntaPreCompiled.h"
#include "CharacterControllerSystem.h"

#include <glm/glm.hpp>

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

// ========== Keys of the player ==========

constexpr int MoveForwardKey  = SUNTA_KEY_W;
constexpr int MoveBackwardKey = SUNTA_KEY_S;
constexpr int MoveLeftKey     = SUNTA_KEY_A;
constexpr int MoveRightKey    = SUNTA_KEY_D;
constexpr int RunKey          = SUNTA_KEY_LEFT_SHIFT;

// ========== Numbers used below ==========

// Anything shorter than this is treated as "no direction at all" (a direction can't be made of nothing)
constexpr float SmallestUsableLength = 0.0001f;

// A weak push on a stick should not make the character spin on the spot, so turning starts above this
constexpr float SmallestMoveToTurn = 0.1f;

const glm::vec3 WorldUp         = glm::vec3(0.0f, 1.0f,  0.0f);
const glm::vec3 DefaultForward  = glm::vec3(0.0f, 0.0f, -1.0f);

// The body given to a new character (about the size and weight of a person, standing on the entity position)
constexpr float CharacterMass = 80.0f;
const glm::vec3 CharacterColliderHalfExtents = glm::vec3(0.3f, 0.9f, 0.3f);
const glm::vec3 CharacterColliderOffset      = glm::vec3(0.0f, CharacterColliderHalfExtents.y, 0.0f);

// ========== Small helpers ==========

glm::vec3 KeepOnlyHorizontalPart(const glm::vec3& vector)
{
	return glm::vec3(vector.x, 0.0f, vector.z);
}

// A vector longer than 1 is shortened to 1, a shorter one stays as it is (so a gentle push means a slow walk)
glm::vec3 LimitLengthToOne(const glm::vec3& vector)
{
	return glm::length(vector) > 1.0f ? glm::normalize(vector) : vector;
}

glm::vec3 GetHorizontalDirectionOrDefault(const glm::vec3& direction)
{
	const glm::vec3 horizontalPart = KeepOnlyHorizontalPart(direction);

	if (glm::length(horizontalPart) < SmallestUsableLength)
		return DefaultForward;

	return glm::normalize(horizontalPart);
}

// Angles are kept between -180 and 180 degrees, so "turn left or right?" always has a short answer
float WrapDegrees(float angle)
{
	return glm::mod(angle + 180.0f, 360.0f) - 180.0f;
}

// The angle (around the up axis) that makes an entity look along the given direction
// An entity looks along its -Z axis when the angle is 0
float GetYawToFace(const glm::vec3& direction)
{
	return glm::degrees(std::atan2(-direction.x, -direction.z));
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

std::string GetEntityName(EntityManager& entityManager, unsigned int entityID)
{
	const TagComponent* tag = entityManager.GetComponent<TagComponent>(entityID);
	return tag ? tag->name : "Unnamed entity";
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

void ReadPlayerInput(CharacterControllerComponent& controller, const glm::vec3& cameraForward)
{
	controller.moveDirection = ReadPlayerMoveDirection(cameraForward);
	controller.wantsToRun    = InputManager::IsKeyPressed(RunKey);
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

// The character stays upright and only turns around the up axis, towards the way it walks
void TurnTowardsMoveDirection(TransformComponent& transform, const CharacterControllerComponent& controller, float deltaTime)
{
	const glm::vec3 horizontalMove = KeepOnlyHorizontalPart(controller.moveDirection);

	if (glm::length(horizontalMove) < SmallestMoveToTurn)
		return;

	const float currentYaw = transform.rotation.y;
	const float wantedYaw  = GetYawToFace(horizontalMove) + controller.facingOffsetDegrees;

	const float yawLeftToTurn = WrapDegrees(wantedYaw - currentYaw);
	const float maxTurn       = controller.turnSpeed * deltaTime;
	const float turn          = glm::clamp(yawLeftToTurn, -maxTurn, maxTurn);

	transform.rotation = glm::vec3(0.0f, WrapDegrees(currentYaw + turn), 0.0f);
	transform.SyncQuaternionFromEuler();
	transform.isDirty = true;
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
			ReadPlayerInput(controller, cameraForward);

		MoveBody(*body, controller, deltaTime);
		TurnTowardsMoveDirection(*transform, controller, deltaTime);
	}
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
