#include "Core/SuntaPreCompiled.h"
#include "LockOnSystem.h"

#include <algorithm>
#include <limits>
#include <vector>
#include <glm/glm.hpp>

#include "EntityName.h"
#include "GameplayMath.h"
#include "SightBlockers.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
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

// ========== Key of the player ==========

constexpr int LockOnKey = SUNTA_KEY_Q;

// ========== Numbers used below ==========

// The lock is dropped when the enemy gets farther than the distance it can be locked from, times this
// (without the extra room an enemy standing right at the edge would be locked and dropped again and again)
constexpr float BreakDistanceFactor = 1.25f;

// An enemy in the middle of the screen is a better pick than a closer one at the side, but distance counts too
constexpr float DistanceScoreWeight = 0.5f;

// Mouse movement that didn't lead to a switch fades away at this speed (pixels per second),
// so a slow aiming never adds up to a switch, only a quick flick does
constexpr float MouseTravelFadePerSecond = 300.0f;

// One flick of the mouse switches to one enemy, not to three in a row
constexpr float SwitchCooldownSeconds = 0.35f;

// Two enemies that are less than this many degrees apart on the screen are not told apart when switching
constexpr float SmallestAngleToSwitch = 1.0f;

// How long the drawn line of sight stays on the screen after a search
constexpr float SightRayShowSeconds = 3.0f;

// ========== The character that locks on ==========

struct Locker
{
	EntityManager&   entityManager;
	unsigned int     entityID;
	LockOnComponent& lockOn;
	glm::vec3        eyePoint;
};

// An enemy that can be locked on to right now
struct Candidate
{
	unsigned int entityID;
	float        distance;
	float        angleFromCamera; // degrees: positive to the right of where the camera looks, negative to the left
};

std::vector<LockOnSystem::SightRay> sightRays;
bool isRecordingSightRays = false;

// ========== Small helpers ==========

glm::vec3 GetEyePoint(const TransformComponent& transform, const LockOnComponent& lockOn)
{
	return transform.position + WorldUp * lockOn.eyeHeight;
}

// False when the entity is not an enemy that can be locked on to (or no longer exists)
bool TryGetAimPoint(EntityManager& entityManager, unsigned int targetEntityID, glm::vec3& outAimPoint)
{
	const TransformComponent*    transform = entityManager.GetComponent<TransformComponent>(targetEntityID);
	const LockOnTargetComponent* target    = entityManager.GetComponent<LockOnTargetComponent>(targetEntityID);

	if (!transform || !target)
		return false;

	outAimPoint = transform->position + WorldUp * target->aimHeight;
	return true;
}

// Degrees from where the camera looks to the direction, seen from above: positive to the right, negative to the left
float GetAngleFromCamera(const glm::vec3& cameraForward, const glm::vec3& direction)
{
	const glm::vec3 forward    = GetHorizontalDirectionOrDefault(cameraForward);
	const glm::vec3 right      = glm::cross(forward, WorldUp);
	const glm::vec3 horizontal = KeepOnlyHorizontalPart(direction);

	return glm::degrees(std::atan2(glm::dot(horizontal, right), glm::dot(horizontal, forward)));
}

float MoveTowardsZero(float value, float maxStep)
{
	if (std::abs(value) <= maxStep)
		return 0.0f;

	return value > 0.0f ? value - maxStep : value + maxStep;
}

void SetTarget(LockOnComponent& lockOn, unsigned int targetEntityID)
{
	lockOn.hasTarget      = true;
	lockOn.targetEntityID = targetEntityID;
	lockOn.mouseTravel    = 0.0f;
}

// ========== Finding enemies ==========

void RememberSightRay(const glm::vec3& from, const glm::vec3& to, bool isBlocked, const glm::vec3& hitPoint)
{
	sightRays.push_back({ from, to, isBlocked, hitPoint, SightRayShowSeconds });
}

void ReportHiddenEnemy(Locker& locker, unsigned int targetEntityID, const RaycastHit& hit)
{
	SUNTA_ENGINE_LOG_INFO("Lock On: '{0}' is hidden behind '{1}' ({2} m from the eyes, the wall is on the Environment layer)",
		GetEntityName(locker.entityManager, targetEntityID), GetEntityName(locker.entityManager, hit.entityID), hit.distance);
}

// True when a wall (or anything else that blocks the sight) is between the two points
bool IsPathBlocked(Locker& locker, unsigned int targetEntityID, const glm::vec3& aimPoint)
{
	const glm::vec3 path   = aimPoint - locker.eyePoint;
	const float     length = glm::length(path);

	if (length < SmallestUsableLength)
		return false;

	Ray ray;
	ray.origin    = locker.eyePoint;
	ray.direction = path / length;

	// The characters themselves don't block the sight, they only stand in the middle of it
	RaycastHit hit;
	const bool isBlocked = Physics::Raycast(locker.entityManager, ray, length, SightBlockingLayers, { locker.entityID, targetEntityID }, hit);

	if (isRecordingSightRays)
	{
		RememberSightRay(locker.eyePoint, aimPoint, isBlocked, hit.point);

		if (isBlocked)
			ReportHiddenEnemy(locker, targetEntityID, hit);
	}

	return isBlocked;
}

bool IsCandidateAllowed(Locker& locker, const Candidate& candidate, const glm::vec3& aimPoint)
{
	const LockOnComponent& lockOn = locker.lockOn;

	if (candidate.distance > lockOn.maxDistance)
		return false;

	if (std::abs(candidate.angleFromCamera) > lockOn.searchAngleDegrees)
		return false;

	return !lockOn.requireLineOfSight || !IsPathBlocked(locker, candidate.entityID, aimPoint);
}

// Every enemy the character could lock on to from where it stands, looking the way the camera looks
std::vector<Candidate> CollectCandidates(Locker& locker, const glm::vec3& cameraForward)
{
	std::vector<Candidate> candidates;

	// Only the latest search is drawn
	if (isRecordingSightRays)
		sightRays.clear();

	for (LockOnTargetComponent& target : locker.entityManager.GetAllComponents<LockOnTargetComponent>())
	{
		const unsigned int targetEntityID = locker.entityManager.GetEntityIDForComponent(target);

		if (targetEntityID == locker.entityID || !target.isLockable)
			continue;

		glm::vec3 aimPoint;
		if (!TryGetAimPoint(locker.entityManager, targetEntityID, aimPoint))
			continue;

		const glm::vec3 toTarget = aimPoint - locker.eyePoint;

		const Candidate candidate = { targetEntityID, glm::length(toTarget), GetAngleFromCamera(cameraForward, toTarget) };

		if (IsCandidateAllowed(locker, candidate, aimPoint))
			candidates.push_back(candidate);
	}

	return candidates;
}

// Lower is better
float GetScore(const Candidate& candidate, const LockOnComponent& lockOn)
{
	const float angleFraction    = std::abs(candidate.angleFromCamera) / glm::max(lockOn.searchAngleDegrees, SmallestUsableLength);
	const float distanceFraction = candidate.distance / glm::max(lockOn.maxDistance, SmallestUsableLength);

	return angleFraction + distanceFraction * DistanceScoreWeight;
}

bool TryPickBest(const std::vector<Candidate>& candidates, const LockOnComponent& lockOn, unsigned int& outEntityID)
{
	bool  isFound   = false;
	float bestScore = std::numeric_limits<float>::max();

	for (const Candidate& candidate : candidates)
	{
		const float score = GetScore(candidate, lockOn);

		if (score >= bestScore)
			continue;

		isFound     = true;
		bestScore   = score;
		outEntityID = candidate.entityID;
	}

	return isFound;
}

// 'side' is +1 for the enemies to the right of the current one and -1 for the ones to the left, the nearest one on that side wins
bool TryPickNextOnSide(const std::vector<Candidate>& candidates, unsigned int currentEntityID, float currentAngle, float side, unsigned int& outEntityID)
{
	bool  isFound      = false;
	float smallestStep = std::numeric_limits<float>::max();

	for (const Candidate& candidate : candidates)
	{
		if (candidate.entityID == currentEntityID)
			continue;

		const float step = (candidate.angleFromCamera - currentAngle) * side;

		if (step < SmallestAngleToSwitch || step >= smallestStep)
			continue;

		isFound      = true;
		smallestStep = step;
		outEntityID  = candidate.entityID;
	}

	return isFound;
}

// ========== Reacting to the player ==========

void LockOnBestTarget(Locker& locker, const glm::vec3& cameraForward)
{
	const std::vector<Candidate> candidates = CollectCandidates(locker, cameraForward);

	unsigned int bestEntityID = 0;
	if (TryPickBest(candidates, locker.lockOn, bestEntityID))
		SetTarget(locker.lockOn, bestEntityID);
}

bool TrySwitchTargetToSide(Locker& locker, const glm::vec3& cameraForward, float side)
{
	glm::vec3 currentAimPoint;
	if (!TryGetAimPoint(locker.entityManager, locker.lockOn.targetEntityID, currentAimPoint))
		return false;

	const float currentAngle = GetAngleFromCamera(cameraForward, currentAimPoint - locker.eyePoint);

	const std::vector<Candidate> candidates = CollectCandidates(locker, cameraForward);

	unsigned int nextEntityID = 0;
	if (!TryPickNextOnSide(candidates, locker.lockOn.targetEntityID, currentAngle, side, nextEntityID))
		return false;

	SetTarget(locker.lockOn, nextEntityID);
	return true;
}

// A quick flick of the mouse to the right (or to the left) locks on to the next enemy on that side
void SwitchTargetWithMouse(Locker& locker, const glm::vec3& cameraForward, const glm::vec2& lookDelta, float deltaTime)
{
	LockOnComponent& lockOn = locker.lockOn;

	lockOn.switchCooldownLeft = glm::max(lockOn.switchCooldownLeft - deltaTime, 0.0f);
	lockOn.mouseTravel        = MoveTowardsZero(lockOn.mouseTravel + lookDelta.x, MouseTravelFadePerSecond * deltaTime);

	if (lockOn.switchCooldownLeft > 0.0f || std::abs(lockOn.mouseTravel) < lockOn.switchMouseDistance)
		return;

	const float side = lockOn.mouseTravel > 0.0f ? 1.0f : -1.0f;
	lockOn.mouseTravel = 0.0f;

	if (TrySwitchTargetToSide(locker, cameraForward, side))
		lockOn.switchCooldownLeft = SwitchCooldownSeconds;
}

void ReadPlayerInput(Locker& locker, const glm::vec3& cameraForward, const glm::vec2& lookDelta, float deltaTime)
{
	if (InputManager::IsKeyDown(LockOnKey))
	{
		if (locker.lockOn.hasTarget)
			LockOnSystem::ClearTarget(locker.lockOn);
		else
			LockOnBestTarget(locker, cameraForward);

		return;
	}

	if (locker.lockOn.hasTarget)
		SwitchTargetWithMouse(locker, cameraForward, lookDelta, deltaTime);
}

void ForgetOldSightRays(float deltaTime)
{
	for (LockOnSystem::SightRay& sightRay : sightRays)
		sightRay.secondsLeft -= deltaTime;

	sightRays.erase(std::remove_if(sightRays.begin(), sightRays.end(),
		[](const LockOnSystem::SightRay& sightRay) { return sightRay.secondsLeft <= 0.0f; }), sightRays.end());
}

// ========== Letting go ==========

// The enemy died (was destroyed), can't be locked on to anymore, or is too far away
void DropTargetIfNoLongerValid(Locker& locker)
{
	LockOnComponent& lockOn = locker.lockOn;

	if (!lockOn.hasTarget)
		return;

	const LockOnTargetComponent* target = locker.entityManager.GetComponent<LockOnTargetComponent>(lockOn.targetEntityID);

	glm::vec3 aimPoint;
	if (!target || !target->isLockable || !TryGetAimPoint(locker.entityManager, lockOn.targetEntityID, aimPoint))
	{
		LockOnSystem::ClearTarget(lockOn);
		return;
	}

	if (glm::distance(locker.eyePoint, aimPoint) > lockOn.maxDistance * BreakDistanceFactor)
		LockOnSystem::ClearTarget(lockOn);
}

}

void LockOnSystem::Update(EntityManager& entityManager, float deltaTime, const glm::vec3& cameraForward, const glm::vec2& lookDelta)
{
	ForgetOldSightRays(deltaTime);

	for (LockOnComponent& lockOn : entityManager.GetAllComponents<LockOnComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(lockOn);

		const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);
		if (!transform)
			continue;

		Locker locker = { entityManager, entityID, lockOn, GetEyePoint(*transform, lockOn) };

		DropTargetIfNoLongerValid(locker);

		if (lockOn.isPlayerControlled)
			ReadPlayerInput(locker, cameraForward, lookDelta, deltaTime);
	}
}

bool LockOnSystem::TryGetTargetAimPoint(EntityManager& entityManager, const LockOnComponent& lockOn, glm::vec3& outAimPoint)
{
	return lockOn.hasTarget && TryGetAimPoint(entityManager, lockOn.targetEntityID, outAimPoint);
}

bool LockOnSystem::TrySetTarget(EntityManager& entityManager, LockOnComponent& lockOn, unsigned int targetEntityID)
{
	glm::vec3 aimPoint;
	if (!TryGetAimPoint(entityManager, targetEntityID, aimPoint))
	{
		SUNTA_ENGINE_LOG_WARNING("Lock On: the entity {0} can't be locked on to, it needs a Lock On Target component", targetEntityID);
		return false;
	}

	SetTarget(lockOn, targetEntityID);
	return true;
}

void LockOnSystem::ClearTarget(LockOnComponent& lockOn)
{
	lockOn.hasTarget          = false;
	lockOn.targetEntityID     = 0;
	lockOn.mouseTravel        = 0.0f;
	lockOn.switchCooldownLeft = 0.0f;
}

void LockOnSystem::SetRecordingSightRays(bool isRecording)
{
	isRecordingSightRays = isRecording;

	if (!isRecording)
		sightRays.clear();
}

const std::vector<LockOnSystem::SightRay>& LockOnSystem::GetSightRays()
{
	return sightRays;
}

void LockOnSystem::ClearSightRays()
{
	sightRays.clear();
}

}
