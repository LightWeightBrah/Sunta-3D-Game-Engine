#include "Core/SuntaPreCompiled.h"
#include "FollowCameraSystem.h"

#include <cmath>
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
// A hit nearer than this is a collider the pivot is inside of (see GetFreeDistance)
constexpr float StartsInsideDistance = 0.05f;
// How many see-through colliders one look past the player may go through
constexpr int   MostPassedThrough    = 8;

// How quickly the camera moves back out after a wall stopped pushing it
constexpr float DistanceReturnSharpness = 6.0f;

// Something low behind the player (a heap of rubble, a low wall) would push the camera into the player's head: it rises
// over it instead, looking down at the player, when rising one of these heights clears most of the view. It goes up
// quickly and comes down slowly
constexpr float RiseSteps[]          = { 0.6f, 1.2f, 1.8f, 2.4f, 3.2f, 4.0f }; // (high heaps of rubble: 2.4 m left it at the neck)
constexpr float RiseIsWorthFrom      = 0.75f; // pushed in to less than this part of the distance: try rising
constexpr float RiseClearsFrom       = 0.9f;  // rising helps when the view is then this free
constexpr float RiseBetterBy         = 1.5f;  // ...or, when nothing clears it, when it frees this much more of it
constexpr float RiseUpSharpness      = 7.0f;
constexpr float RiseDownSharpness    = 2.5f;

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

	// (swung around the player by lockOnSideDegrees: the enemy is seen beside him, not behind his back)
	const float wantedYaw   = WrapDegrees(GetYawToFace(horizontalToTarget) + follow.lockOnSideDegrees);
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

	// The player is ignored: the pivot is inside its collider. So is the fog of a boss arena: it is a wall for the
	// player, not for the camera (fighting just inside the gate, the camera would come up behind the player's head),
	// and so are colliders that don't block the camera (unseen walls at the edge of an arena): up to two of them
	auto isSeeThrough = [&frame](unsigned int entityID)
	{
		const BoxColliderComponent* collider = frame.entityManager.GetComponent<BoxColliderComponent>(entityID);
		return frame.entityManager.GetComponent<FogGateComponent>(entityID) || (collider && !collider->blocksCamera);
	};
	const float reach = wantedLength + WallMargin;
	// (and so is what the pivot itself is inside: the player standing on a heap of rubble has its collider around his
	// head, every ray "hit" it at once and the camera sat at his neck)
	auto isPassedThrough = [&isSeeThrough](const RaycastHit& hit) { return hit.distance < StartsInsideDistance || isSeeThrough(hit.entityID); };
	// (any number of them in a row, up to a few: in a corner of an arena two unseen walls overlap a heap of rubble)
	RaycastHit hit;
	std::vector<unsigned int> ignored = { frame.playerEntityID };
	for (int passed = 0; ; ++passed)
	{
		if (!Physics::Raycast(frame.entityManager, ray, reach, SightBlockingLayers, ignored, hit))
			return follow.distance;
		if (passed >= MostPassedThrough || !isPassedThrough(hit))
			break;
		ignored.push_back(hit.entityID);
	}

	const float freeLength = glm::clamp(hit.distance - WallMargin, MinDistanceFromPivot, wantedLength);

	return follow.distance * freeLength / wantedLength;
}

// How high the camera should rise over something low behind the player (0: not at all), and how far it may be then
float ChooseRise(const FrameInfo& frame, const FollowCameraComponent& follow, const glm::vec3& cameraOffset, float freeDistance,
	float& outFreeDistance)
{
	outFreeDistance = freeDistance;
	if (follow.distance < SmallestUsableLength || freeDistance >= follow.distance * RiseIsWorthFrom)
		return 0.0f;

	// (when no height clears the view, the one that frees it most - backed into a corner between heaps of rubble the
	// camera stayed at the player's neck)
	float bestRise = 0.0f;
	float bestFree = freeDistance;
	for (float rise : RiseSteps)
	{
		// (nothing above the player's head either: a ceiling or an arch)
		Ray up;
		up.origin    = follow.pivot;
		up.direction = WorldUp;
		RaycastHit hit;
		// (what the pivot is inside of is no ceiling: standing in the box of a heap of rubble the camera never rose)
		if (Physics::Raycast(frame.entityManager, up, rise + WallMargin, SightBlockingLayers, frame.playerEntityID, hit)
		    && hit.distance >= StartsInsideDistance)
			break;

		const float raisedFree = GetFreeDistance(frame, follow, cameraOffset + WorldUp * rise);
		if (raisedFree >= follow.distance * RiseClearsFrom)
		{
			outFreeDistance = raisedFree;
			return rise;
		}
		if (raisedFree > bestFree * RiseBetterBy)
		{
			bestFree = raisedFree;
			bestRise = rise;
		}
	}
	outFreeDistance = bestFree;
	return bestRise;
}

void UpdateRise(FollowCameraComponent& follow, float wantedRise, float deltaTime)
{
	const float sharpness = wantedRise > follow.currentRise ? RiseUpSharpness : RiseDownSharpness;
	follow.currentRise = SmoothValue(follow.currentRise, wantedRise, sharpness, deltaTime);
}

// A wall pushes the camera in at once (the player must never be hidden), the camera moves back out smoothly
void UpdateDistance(FollowCameraComponent& follow, float freeDistance, float deltaTime)
{
	if (freeDistance < follow.currentDistance)
		follow.currentDistance = freeDistance;
	else
		follow.currentDistance = SmoothValue(follow.currentDistance, freeDistance, DistanceReturnSharpness, deltaTime);
}

// ========== Looking along a direction ==========

// The rotation of an entity that looks along 'forward' (its -Z axis) with +Y up
glm::quat MakeRotationLookingAlong(const glm::vec3& forward)
{
	const glm::vec3 right = glm::normalize(glm::cross(forward, WorldUp));
	const glm::vec3 up    = glm::cross(right, forward);

	return glm::quat_cast(glm::mat3(right, up, -forward));
}

// The view of the camera: where it is turned to, plus the offsets that move the player around the screen (and the shake)
glm::vec3 GetViewDirection(const FollowCameraComponent& follow, const glm::vec2& shakeDegrees = glm::vec2(0.0f))
{
	const float yaw   = follow.yawDegrees + follow.viewYawOffsetDegrees + shakeDegrees.x;
	const float pitch = glm::clamp(follow.pitchDegrees + follow.viewPitchOffsetDegrees + shakeDegrees.y, -MaxViewPitchDegrees, MaxViewPitchDegrees);

	return GetDirectionFromYawAndPitch(yaw, pitch);
}

// ========== Shaking ==========

// A smooth wobble between -1 and 1: waves of unrelated speeds added up, so it never repeats visibly
float Wobble(float seconds, float frequency, float seed)
{
	const float t = seconds * frequency;
	return (std::sin(t + seed) + 0.6f * std::sin(t * 2.13f + seed * 3.1f) + 0.3f * std::sin(t * 4.37f + seed * 7.7f)) / 1.9f;
}

// How far the shake turns the view now (degrees: yaw, pitch, roll). The shake fades away by itself
glm::vec3 UpdateShake(FollowCameraComponent& follow, float deltaTime)
{
	if (follow.shakeTrauma <= 0.0f)
		return glm::vec3(0.0f);

	follow.shakeSeconds += deltaTime;
	const float strength = follow.shakeTrauma * follow.shakeTrauma * follow.shakeDegrees;
	follow.shakeTrauma   = glm::max(follow.shakeTrauma - follow.shakeFadePerSecond * deltaTime, 0.0f);

	return strength * glm::vec3(Wobble(follow.shakeSeconds, follow.shakeFrequency, 0.0f),
	                            Wobble(follow.shakeSeconds, follow.shakeFrequency, 11.0f),
	                            0.5f * Wobble(follow.shakeSeconds, follow.shakeFrequency, 23.0f));
}

// ========== Placing the camera ==========

void PlaceCamera(TransformComponent& cameraTransform, const FollowCameraComponent& follow, const glm::vec3& cameraOffset,
	const glm::vec3& shakeDegrees)
{
	// 'currentDistance' is shorter than 'distance' while a wall pushes the camera, the whole offset shrinks with it
	const float shrink = follow.distance > SmallestUsableLength ? follow.currentDistance / follow.distance : 1.0f;

	// (risen over something, it looks down at the pivot as much more as it rose)
	const float riseDegrees = glm::degrees(std::atan2(follow.currentRise, glm::max(follow.currentDistance, SmallestUsableLength)));
	const glm::vec3 viewDirection = GetViewDirection(follow, glm::vec2(shakeDegrees.x, shakeDegrees.y + riseDegrees));

	cameraTransform.position           = follow.pivot + cameraOffset * shrink + WorldUp * follow.currentRise;
	cameraTransform.rotationQuaternion = glm::angleAxis(glm::radians(shakeDegrees.z), viewDirection) * MakeRotationLookingAlong(viewDirection);

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

	float freeDistance = GetFreeDistance(frame, follow, cameraOffset);
	const float rise = ChooseRise(frame, follow, cameraOffset, freeDistance, freeDistance);
	UpdateRise(follow, rise, frame.deltaTime);
	// (while it rises the old pushed-in distance holds, so it doesn't swing out before it is up)
	if (follow.currentRise > rise * 0.6f || rise <= 0.0f)
		UpdateDistance(follow, freeDistance, frame.deltaTime);
	else
		UpdateDistance(follow, glm::min(freeDistance, GetFreeDistance(frame, follow, cameraOffset + WorldUp * follow.currentRise)), frame.deltaTime);
	PlaceCamera(cameraTransform, follow, cameraOffset, UpdateShake(follow, frame.deltaTime));
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

void FollowCameraSystem::Shake(EntityManager& entityManager, float strength)
{
	for (FollowCameraComponent& follow : entityManager.GetAllComponents<FollowCameraComponent>())
		follow.shakeTrauma = glm::clamp(follow.shakeTrauma + strength, 0.0f, 1.0f);
}

void FollowCameraSystem::ShakeFrom(EntityManager& entityManager, float strength, const glm::vec3& worldPosition, float radius)
{
	unsigned int playerEntityID = 0;
	if (radius <= 0.0f || !CharacterControllerSystem::TryFindPlayer(entityManager, playerEntityID))
		return;

	const TransformComponent* player = entityManager.GetComponent<TransformComponent>(playerEntityID);
	if (!player)
		return;

	const float closeness = 1.0f - glm::length(player->position - worldPosition) / radius;
	if (closeness > 0.0f)
		Shake(entityManager, strength * closeness);
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
