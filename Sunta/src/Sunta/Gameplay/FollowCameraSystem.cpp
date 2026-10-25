#include "Core/SuntaPreCompiled.h"
#include "FollowCameraSystem.h"

#include <optional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "CharacterControllerSystem.h"
#include "EntityName.h"
#include "GameplayMath.h"
#include "LockOnSystem.h"
#include "SightBlockers.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Physics/CollisionShapes.h"
#include "Physics/Raycast.h"

namespace Sunta
{

namespace
{

using namespace GameplayMath;

// ========== Numbers used below ==========

// A camera pushed by a wall stays this far from it, so the wall is not cut open by the near plane of the camera
constexpr float WallMargin = 0.25f;

// A camera pushed by a wall never comes closer to the player than this
constexpr float MinDistanceFromPivot = 0.4f;

// How quickly the camera moves back out after a wall stopped pushing it
constexpr float DistanceReturnSharpness = 6.0f;

// The view never looks straight up or down, there the "up" of the camera has no meaning
constexpr float MaxViewPitchDegrees = 89.0f;

// What every camera needs to know about the current frame
struct FrameInfo
{
	EntityManager&           entityManager;
	unsigned int             playerEntityID;
	glm::vec3                playerPosition;
	std::optional<glm::vec3> lockedTargetAimPoint; // the enemy the player is locked on to, if any
	glm::vec2                lookDelta;
	float                    deltaTime;
};

// ========== Where the camera looks ==========

// The limits are kept the right way round, even when someone types a minimum above the maximum
float GetClampedPitch(const FollowCameraComponent& follow, float pitch)
{
	const float lowest  = glm::min(follow.minPitchDegrees, follow.maxPitchDegrees);
	const float highest = glm::max(follow.minPitchDegrees, follow.maxPitchDegrees);

	return glm::clamp(pitch, lowest, highest);
}

// The first frame: the camera looks the way its entity was placed in the editor, and starts right at the player
void StartFromCurrentView(FollowCameraComponent& follow, const TransformComponent& cameraTransform, const glm::vec3& pivot)
{
	const glm::vec3 forward = glm::normalize(cameraTransform.rotationQuaternion) * DefaultForward;

	// The offsets of the view are not part of where the camera is turned to, so they are taken off
	follow.yawDegrees      = WrapDegrees(GetYawToFace(forward) - follow.viewYawOffsetDegrees);
	follow.pitchDegrees    = GetClampedPitch(follow, GetPitchOf(forward) - follow.viewPitchOffsetDegrees);
	follow.currentDistance = follow.distance;
	follow.pivot           = pivot;
	follow.isInitialized   = true;
}

void TurnWithMouse(FollowCameraComponent& follow, const glm::vec2& lookDelta)
{
	// The mouse moved up gives a positive delta, and looking up is a smaller pitch
	const float pitchDirection = follow.invertY ? 1.0f : -1.0f;

	const float yaw   = follow.yawDegrees   - lookDelta.x * follow.mouseSensitivity;
	const float pitch = follow.pitchDegrees + lookDelta.y * follow.mouseSensitivity * pitchDirection;

	follow.yawDegrees   = WrapDegrees(yaw);
	follow.pitchDegrees = GetClampedPitch(follow, pitch);
}

// Behind the player, looking at the locked enemy over the player's shoulder
void TurnTowardsLockedTarget(FollowCameraComponent& follow, const glm::vec3& targetAimPoint, float deltaTime)
{
	const glm::vec3 horizontalToTarget = KeepOnlyHorizontalPart(targetAimPoint - follow.pivot);

	// An enemy right above or below the player has no side to turn to
	if (glm::length(horizontalToTarget) < SmallestUsableLength)
		return;

	const float wantedYaw   = GetYawToFace(horizontalToTarget);
	const float wantedPitch = GetClampedPitch(follow, follow.lockOnPitchDegrees);

	follow.yawDegrees   = SmoothAngle(follow.yawDegrees,   wantedYaw,   follow.lockOnSharpness, deltaTime);
	follow.pitchDegrees = SmoothValue(follow.pitchDegrees, wantedPitch, follow.lockOnSharpness, deltaTime);
}

// ========== Walls ==========

// Where the camera wants to be, as a step away from the pivot (behind the player, and to the side with a shoulder offset)
glm::vec3 GetCameraOffset(const FollowCameraComponent& follow, const glm::vec3& forward)
{
	const glm::vec3 right = glm::cross(GetHorizontalDirectionOrDefault(forward), WorldUp);

	return right * follow.shoulderOffset - forward * follow.distance;
}

// How far from the pivot the camera can be without a wall in between, as a part of 'follow.distance'
// (the camera moves along the line from the pivot to 'cameraOffset', and the offset shrinks with it)
float GetFreeDistance(const FrameInfo& frame, const FollowCameraComponent& follow, const glm::vec3& cameraOffset)
{
	const float wantedLength = glm::length(cameraOffset);

	if (wantedLength < SmallestUsableLength)
		return follow.distance;

	Ray ray;
	ray.origin    = follow.pivot;
	ray.direction = cameraOffset / wantedLength;

	// The player is ignored: the pivot is inside its collider
	RaycastHit hit;
	if (!Physics::Raycast(frame.entityManager, ray, wantedLength + WallMargin, SightBlockingLayers, frame.playerEntityID, hit))
		return follow.distance;

	const float freeLength = glm::clamp(hit.distance - WallMargin, MinDistanceFromPivot, wantedLength);

	return follow.distance * freeLength / wantedLength;
}

// A wall pushes the camera in at once (the player must never be hidden), the camera moves back out smoothly
void UpdateDistance(FollowCameraComponent& follow, float freeDistance, float deltaTime)
{
	if (freeDistance < follow.currentDistance)
		follow.currentDistance = freeDistance;
	else
		follow.currentDistance = SmoothValue(follow.currentDistance, freeDistance, DistanceReturnSharpness, deltaTime);
}

// ========== Placing the camera ==========

// The rotation of an entity that looks along 'forward' (its -Z axis) with +Y up
glm::quat MakeRotationLookingAlong(const glm::vec3& forward)
{
	const glm::vec3 right = glm::normalize(glm::cross(forward, WorldUp));
	const glm::vec3 up    = glm::cross(right, forward);

	return glm::quat_cast(glm::mat3(right, up, -forward));
}

// The view of the camera: where it is turned to, plus the offsets that move the player around the screen
glm::vec3 GetViewDirection(const FollowCameraComponent& follow)
{
	const float yaw   = follow.yawDegrees + follow.viewYawOffsetDegrees;
	const float pitch = glm::clamp(follow.pitchDegrees + follow.viewPitchOffsetDegrees, -MaxViewPitchDegrees, MaxViewPitchDegrees);

	return GetDirectionFromYawAndPitch(yaw, pitch);
}

void PlaceCamera(TransformComponent& cameraTransform, const FollowCameraComponent& follow, const glm::vec3& cameraOffset)
{
	// 'currentDistance' is shorter than 'distance' while a wall pushes the camera, the whole offset shrinks with it
	const float shrink = follow.distance > SmallestUsableLength ? follow.currentDistance / follow.distance : 1.0f;

	cameraTransform.position           = follow.pivot + cameraOffset * shrink;
	cameraTransform.rotationQuaternion = MakeRotationLookingAlong(GetViewDirection(follow));

	// The Inspector shows the angles, so they follow the quaternion
	cameraTransform.SyncEulerFromQuaternion();
	cameraTransform.isDirty = true;
}

void UpdateCamera(const FrameInfo& frame, FollowCameraComponent& follow, TransformComponent& cameraTransform)
{
	const glm::vec3 wantedPivot = frame.playerPosition + WorldUp * follow.pivotHeight;

	if (!follow.isInitialized)
		StartFromCurrentView(follow, cameraTransform, wantedPivot);
	else
		follow.pivot = SmoothPoint(follow.pivot, wantedPivot, follow.followSharpness, frame.deltaTime);

	// While locked on, the mouse switches enemies, so it doesn't turn the camera
	if (frame.lockedTargetAimPoint)
		TurnTowardsLockedTarget(follow, *frame.lockedTargetAimPoint, frame.deltaTime);
	else
		TurnWithMouse(follow, frame.lookDelta);

	const glm::vec3 orbitDirection = GetDirectionFromYawAndPitch(follow.yawDegrees, follow.pitchDegrees);
	const glm::vec3 cameraOffset   = GetCameraOffset(follow, orbitDirection);

	UpdateDistance(follow, GetFreeDistance(frame, follow, cameraOffset), frame.deltaTime);
	PlaceCamera(cameraTransform, follow, cameraOffset);
}

}

void FollowCameraSystem::Update(EntityManager& entityManager, float deltaTime, const glm::vec2& lookDelta)
{
	unsigned int playerEntityID = 0;

	// The reason is reported once when the game starts (ReportProblemsBeforePlay), not every frame
	if (!CharacterControllerSystem::TryFindPlayer(entityManager, playerEntityID))
		return;

	const TransformComponent* playerTransform = entityManager.GetComponent<TransformComponent>(playerEntityID);
	if (!playerTransform)
		return;

	FrameInfo frame = { entityManager, playerEntityID, playerTransform->position, std::nullopt, lookDelta, deltaTime };

	glm::vec3 lockedTargetAimPoint;
	const LockOnComponent* lockOn = entityManager.GetComponent<LockOnComponent>(playerEntityID);

	if (lockOn && LockOnSystem::TryGetTargetAimPoint(entityManager, *lockOn, lockedTargetAimPoint))
		frame.lockedTargetAimPoint = lockedTargetAimPoint;

	for (FollowCameraComponent& follow : entityManager.GetAllComponents<FollowCameraComponent>())
	{
		const unsigned int cameraEntityID = entityManager.GetEntityIDForComponent(follow);

		// The camera moves its own entity, so on the player it would push the player around (reported at the start of the game)
		if (cameraEntityID == playerEntityID)
			continue;

		TransformComponent* cameraTransform = entityManager.GetComponent<TransformComponent>(cameraEntityID);
		if (!cameraTransform)
			continue;

		UpdateCamera(frame, follow, *cameraTransform);
	}
}

void FollowCameraSystem::AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID)
{
	entityManager.AddComponent<FollowCameraComponent>(entityID);

	if (!entityManager.GetComponent<CameraComponent>(entityID))
		entityManager.AddComponent<CameraComponent>(entityID);
}

void FollowCameraSystem::ReportProblemsBeforePlay(EntityManager& entityManager)
{
	bool hasFollowCamera = false;

	unsigned int playerEntityID = 0;
	const bool   hasPlayer      = CharacterControllerSystem::TryFindPlayer(entityManager, playerEntityID);

	for (FollowCameraComponent& follow : entityManager.GetAllComponents<FollowCameraComponent>())
	{
		hasFollowCamera = true;

		const unsigned int entityID = entityManager.GetEntityIDForComponent(follow);
		const std::string  name     = GetEntityName(entityManager, entityID);

		const CameraComponent* camera = entityManager.GetComponent<CameraComponent>(entityID);

		if (hasPlayer && entityID == playerEntityID)
			SUNTA_ENGINE_LOG_ERROR("Follow Camera on '{0}' is on the player itself: it would move the player. Put it on a separate entity with a Camera component", name);
		else if (!camera)
			SUNTA_ENGINE_LOG_ERROR("Follow Camera on '{0}' needs a Camera component, the game will not be drawn from it", name);
		else if (!camera->isActive)
			SUNTA_ENGINE_LOG_WARNING("Follow Camera on '{0}': its Camera component is not active, the game will not be drawn from it", name);
	}

	if (hasFollowCamera && !hasPlayer)
		SUNTA_ENGINE_LOG_ERROR("Follow Camera has nothing to follow: no entity has a Character Controller with 'isPlayerControlled' turned on");
}

}
