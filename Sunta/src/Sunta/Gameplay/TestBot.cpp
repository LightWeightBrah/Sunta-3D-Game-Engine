#include "Core/SuntaPreCompiled.h"
#include "TestBot.h"

#include "CombatLog.h"
#include "LockOnComponent.h"
#include "LockOnSystem.h"
#include "LockOnTargetComponent.h"
#include "EnemyComponent.h"
#include "CharacterControllerComponent.h"
#include "Core/InputPlayback.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

constexpr float AttackEverySeconds = 0.35f;
constexpr float AttackReachExtra   = 0.6f; // attacks start this much farther than the distance it keeps

bool  isRegistered	 = false;
bool  keepsLock		 = false;
float now			 = 0.0f;
float approachUntil	 = -1.0f;
float approachDistance = 1.2f;
float fightUntil	 = -1.0f;
float nextAttackAt	 = 0.0f;
bool  isWalking		 = false;
bool  unlockRequested = false;
int   freezeChange	 = 0; // +1 freeze the enemies, -1 let them think again
bool  hasTeleport	 = false;
glm::vec3 teleportTo{ 0.0f };

float ToFloat(const std::string& text, float fallback)
{
	try { return text.empty() ? fallback : std::stof(text); }
	catch (...) { return fallback; }
}

bool Handle(const std::string& command, const std::string& a, const std::string& b, const std::string& c, float)
{
	if (command == "lockon")
		keepsLock = true;
	else if (command == "unlock")
	{
		keepsLock = false;
		unlockRequested = true;
	}
	else if (command == "teleport")
	{
		hasTeleport = true;
		teleportTo	= { ToFloat(a, 0.0f), ToFloat(b, 0.0f), ToFloat(c, 0.0f) };
	}
	else if (command == "freeze")
		freezeChange = 1;
	else if (command == "unfreeze")
		freezeChange = -1;
	else if (command == "approach")
	{
		approachDistance = ToFloat(a, 1.2f);
		approachUntil	 = now + ToFloat(b, 5.0f);
	}
	else if (command == "fight")
	{
		keepsLock		 = c != "free"; // "free": attack without locking on (the camera can look from aside)
		fightUntil		 = now + ToFloat(a, 10.0f);
		approachDistance = ToFloat(b, 1.1f);
		approachUntil	 = fightUntil;
	}
	else
		return false;

	CombatLog::Event("bot," + command + " " + a + " " + b);
	return true;
}

glm::vec3 PositionOf(EntityManager& entityManager, unsigned int entityID)
{
	const WorldMatrixComponent* world = entityManager.GetComponent<WorldMatrixComponent>(entityID);
	return world ? glm::vec3(world->matrix[3]) : glm::vec3(0.0f);
}

bool IsAlive(EntityManager& entityManager, unsigned int entityID)
{
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID);
	return !health || !health->isDead;
}

// The player's Lock On (the one the keyboard controls)
LockOnComponent* FindPlayerLockOn(EntityManager& entityManager, unsigned int& outPlayerID)
{
	for (LockOnComponent& lockOn : entityManager.GetAllComponents<LockOnComponent>())
	{
		if (lockOn.isPlayerControlled)
		{
			outPlayerID = entityManager.GetEntityIDForComponent(lockOn);
			return &lockOn;
		}
	}
	return nullptr;
}

void KeepLocked(EntityManager& entityManager, LockOnComponent& lockOn, unsigned int playerID)
{
	if (lockOn.hasTarget && IsAlive(entityManager, lockOn.targetEntityID))
		return;

	const glm::vec3 player = PositionOf(entityManager, playerID);
	unsigned int	nearest = 0;
	float			nearestDistance = lockOn.maxDistance * 2.0f;

	for (LockOnTargetComponent& target : entityManager.GetAllComponents<LockOnTargetComponent>())
	{
		const unsigned int targetID = entityManager.GetEntityIDForComponent(target);
		if (targetID == playerID || !IsAlive(entityManager, targetID))
			continue;

		const float distance = glm::length(PositionOf(entityManager, targetID) - player);
		if (distance < nearestDistance)
		{
			nearestDistance = distance;
			nearest			= targetID;
		}
	}

	if (nearest != 0)
		LockOnSystem::TrySetTarget(entityManager, lockOn, nearest);
}

void SetWalking(bool walk)
{
	if (walk == isWalking)
		return;

	isWalking = walk;
	if (walk)
		InputPlayback::PressKey("W");
	else
		InputPlayback::ReleaseKey("W");
}

}

void TestBot::Update(EntityManager& entityManager, float deltaTime)
{
	if (!InputPlayback::IsActive())
		return;

	if (!isRegistered)
	{
		isRegistered = true;
		InputPlayback::SetCommandHandler(Handle);
	}

	now += deltaTime;

	// Frozen enemies stand still (a fixed target to compare blows against)
	if (freezeChange != 0)
	{
		for (EnemyComponent& enemy : entityManager.GetAllComponents<EnemyComponent>())
		{
			enemy.isDormant = freezeChange > 0;
			if (CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(entityManager.GetEntityIDForComponent(enemy)))
				controller->moveDirection = glm::vec3(0.0f);
		}
		freezeChange = 0;
	}

	unsigned int	 playerID = 0;
	LockOnComponent* lockOn	  = FindPlayerLockOn(entityManager, playerID);
	if (!lockOn)
		return;

	if (hasTeleport)
	{
		hasTeleport = false;
		if (TransformComponent* transform = entityManager.GetComponent<TransformComponent>(playerID))
		{
			transform->position = teleportTo;
			transform->isDirty	= true;
		}
	}

	if (keepsLock)
		KeepLocked(entityManager, *lockOn, playerID);
	else if (unlockRequested)
		LockOnSystem::ClearTarget(*lockOn);
	unlockRequested = false;

	const bool hasEnemy = lockOn->hasTarget && IsAlive(entityManager, lockOn->targetEntityID);
	float distance = 1e9f;
	if (hasEnemy)
	{
		glm::vec3 between = PositionOf(entityManager, lockOn->targetEntityID) - PositionOf(entityManager, playerID);
		between.y = 0.0f;
		distance  = glm::length(between);
	}
	else
	{
		// Unlocked (to look at the fight from aside): the nearest enemy still decides when to attack
		for (LockOnTargetComponent& target : entityManager.GetAllComponents<LockOnTargetComponent>())
		{
			const unsigned int targetID = entityManager.GetEntityIDForComponent(target);
			if (targetID == playerID || !IsAlive(entityManager, targetID))
				continue;
			glm::vec3 between = PositionOf(entityManager, targetID) - PositionOf(entityManager, playerID);
			between.y = 0.0f;
			distance  = glm::min(distance, glm::length(between));
		}
	}

	// Locked on, W walks towards the enemy (the character faces it)
	SetWalking(now < approachUntil && hasEnemy && distance > approachDistance);

	if (now < fightUntil && distance < approachDistance + AttackReachExtra && now >= nextAttackAt)
	{
		InputPlayback::PressKey("Mouse1");
		InputPlayback::ReleaseKey("Mouse1"); // a click within one frame still counts (IsMouseButtonDown)
		nextAttackAt = now + AttackEverySeconds;
	}
}

}
