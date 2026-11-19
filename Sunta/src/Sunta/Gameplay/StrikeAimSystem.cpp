#include "Core/SuntaPreCompiled.h"
#include "StrikeAimSystem.h"

#include <glm/gtc/quaternion.hpp>

#include "CharacterActionsComponent.h"
#include "LockOnComponent.h"
#include "StrikeAimComponent.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

constexpr float InFrontDot = 0.25f; // how much in front a character must stand to be aimed at without a lock

std::vector<std::string> SplitBones(const std::string& text)
{
	std::vector<std::string> bones;
	std::stringstream stream(text);
	std::string bone;
	while (std::getline(stream, bone, ','))
	{
		bone.erase(0, bone.find_first_not_of(" \t"));
		bone.erase(bone.find_last_not_of(" \t") + 1);
		if (!bone.empty())
			bones.push_back(bone);
	}
	return bones;
}

bool IsAlive(EntityManager& entityManager, unsigned int entityID)
{
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID);
	return health && !health->isDead;
}

// The enemy to aim at: the one locked on to, or the nearest living character in front
bool TryFindTarget(EntityManager& entityManager, unsigned int selfID, const glm::mat4& self, float searchDistance, unsigned int& outTargetID)
{
	if (const LockOnComponent* lockOn = entityManager.GetComponent<LockOnComponent>(selfID))
	{
		if (lockOn->hasTarget && IsAlive(entityManager, lockOn->targetEntityID))
		{
			outTargetID = lockOn->targetEntityID;
			return true;
		}
	}

	const glm::vec3 position = glm::vec3(self[3]);
	glm::vec3 forward = -glm::vec3(self[2]);
	forward.y = 0.0f;
	forward = glm::length(forward) > 0.0001f ? glm::normalize(forward) : glm::vec3(0.0f, 0.0f, 1.0f);

	float best = searchDistance;
	bool  found = false;
	for (CharacterActionsComponent& other : entityManager.GetAllComponents<CharacterActionsComponent>())
	{
		const unsigned int otherID = entityManager.GetEntityIDForComponent(other);
		const WorldMatrixComponent* world = entityManager.GetComponent<WorldMatrixComponent>(otherID);
		if (otherID == selfID || !world || !IsAlive(entityManager, otherID))
			continue;

		glm::vec3 between = glm::vec3(world->matrix[3]) - position;
		between.y = 0.0f;
		const float distance = glm::length(between);
		if (distance < 0.01f || distance > best || glm::dot(between / distance, forward) < InFrontDot)
			continue;

		best		= distance;
		outTargetID = otherID;
		found		= true;
	}
	return found;
}

// The height of the target's body to aim at (its body box, or a guess from its position)
float GetAimHeight(EntityManager& entityManager, unsigned int targetID, float fraction)
{
	if (const BoxColliderComponent* body = entityManager.GetComponent<BoxColliderComponent>(targetID))
	{
		const OBB& box = body->worldOBB;
		const float half = std::abs(box.orientation[0].y) * box.halfExtents.x
						 + std::abs(box.orientation[1].y) * box.halfExtents.y
						 + std::abs(box.orientation[2].y) * box.halfExtents.z;
		return box.center.y - half + 2.0f * half * fraction;
	}

	const WorldMatrixComponent* world = entityManager.GetComponent<WorldMatrixComponent>(targetID);
	return (world ? world->matrix[3].y : 0.0f) + 1.8f * fraction;
}

float MoveTowards(float value, float target, float maxStep)
{
	return value < target ? glm::min(value + maxStep, target) : glm::max(value - maxStep, target);
}

}

void StrikeAimSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (StrikeAimComponent& aim : entityManager.GetAllComponents<StrikeAimComponent>())
	{
		const unsigned int selfID = entityManager.GetEntityIDForComponent(aim);
		const CharacterActionsComponent* actions = entityManager.GetComponent<CharacterActionsComponent>(selfID);
		AnimatorComponent*				 animator = entityManager.GetComponent<AnimatorComponent>(selfID);
		const WorldMatrixComponent*		 world	  = entityManager.GetComponent<WorldMatrixComponent>(selfID);
		if (!actions || !animator || !world)
			continue;

		const std::vector<std::string> bones = SplitBones(aim.spineBones);
		if (bones.empty())
			continue;

		// The aim follows the enemy while the attack lasts, and is kept (fading out) after it
		unsigned int targetID = 0;
		const bool isAttacking = actions->IsAttacking() && IsAlive(entityManager, selfID);
		if (isAttacking && TryFindTarget(entityManager, selfID, world->matrix, aim.searchDistance, targetID))
		{
			const WorldMatrixComponent* targetWorld = entityManager.GetComponent<WorldMatrixComponent>(targetID);
			const glm::vec3 self	= glm::vec3(world->matrix[3]);
			const glm::vec3 target	= targetWorld ? glm::vec3(targetWorld->matrix[3]) : self;

			// Into the character's own space: an entity looks along -Z, up is +Y (scale removed)
			const glm::mat3 axes	= glm::mat3(glm::normalize(glm::vec3(world->matrix[0])), glm::normalize(glm::vec3(world->matrix[1])),
												glm::normalize(glm::vec3(world->matrix[2])));
			glm::vec3 local = glm::transpose(axes) * (target - self);
			local.y = 0.0f;
			const float distance = glm::max(glm::length(local), 0.3f);

			const AttackMove* move = actions->FindCurrentMove();
			const float clipHeight = (move && move->strikeHeight > 0.0f) ? move->strikeHeight : aim.clipStrikeHeight;
			const float rise = GetAimHeight(entityManager, targetID, aim.targetHeight) - (self.y + clipHeight);
			aim.pitchDegrees = glm::clamp(glm::degrees(std::atan2(rise, distance)), -aim.maxPitchDegrees, aim.maxPitchDegrees);
			aim.yawDegrees	 = glm::clamp(glm::degrees(std::atan2(-local.x, -local.z)), -aim.maxYawDegrees, aim.maxYawDegrees);
		}

		const float target = (isAttacking && targetID != 0) ? 1.0f : 0.0f;
		aim.weight = aim.fadeSeconds <= 0.0f ? target : MoveTowards(aim.weight, target, deltaTime / aim.fadeSeconds);

		if (aim.weight <= 0.0f)
		{
			if (aim.isApplied)
			{
				for (const std::string& bone : bones)
					animator->animator.SetExtraRotation(bone, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
				aim.isApplied = false;
			}
			continue;
		}

		// Shared between the bones; pitching up turns forward (-Z) towards up (+Y): a positive turn around +X
		// (the model is drawn in the space of its entity)
		const float eased = aim.weight * aim.weight * (3.0f - 2.0f * aim.weight);
		const float share = eased / (float)bones.size();
		const glm::quat turn = glm::angleAxis(glm::radians(aim.yawDegrees * share), glm::vec3(0.0f, 1.0f, 0.0f))
							 * glm::angleAxis(glm::radians(aim.pitchDegrees * share), glm::vec3(1.0f, 0.0f, 0.0f));
		for (const std::string& bone : bones)
			animator->animator.SetExtraRotation(bone, turn);
		aim.isApplied = true;
	}
}

}
