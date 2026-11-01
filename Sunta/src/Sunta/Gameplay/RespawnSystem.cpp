#include "Core/SuntaPreCompiled.h"
#include "RespawnSystem.h"

#include "BossSystem.h"
#include "CharacterActionsSystem.h"
#include "LockOnSystem.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Physics/CollisionSystem.h"

namespace Sunta
{

namespace
{

// Moves the character at once (not walking there): no leftover speed, no remembered contacts with the old place
void Teleport(EntityManager& entityManager, unsigned int entityID, const glm::vec3& position, float yawDegrees)
{
	if (TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID))
	{
		transform->position = position;
		transform->rotation = glm::vec3(0.0f, yawDegrees, 0.0f);
		transform->SyncQuaternionFromEuler();
		transform->isDirty = true;
	}

	if (PhysicsBodyComponent* body = entityManager.GetComponent<PhysicsBodyComponent>(entityID))
	{
		body->velocity        = glm::vec3(0.0f);
		body->angularVelocity = glm::vec3(0.0f);
	}

	CollisionSystem::ForgetEntity(entityID);
}

void ForgetLockOn(EntityManager& entityManager, unsigned int entityID)
{
	if (LockOnComponent* lockOn = entityManager.GetComponent<LockOnComponent>(entityID))
		LockOnSystem::ClearTarget(*lockOn);

	if (LockOnTargetComponent* lockOnTarget = entityManager.GetComponent<LockOnTargetComponent>(entityID))
		lockOnTarget->isLockable = true;
}

void Revive(EntityManager& entityManager, unsigned int entityID, const glm::vec3& position, float yawDegrees)
{
	Teleport(entityManager, entityID, position, yawDegrees);
	RespawnSystem::RestoreCharacter(entityManager, entityID);

	if (HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID))
		health->isDead = false;

	ForgetLockOn(entityManager, entityID);
	CharacterActionsSystem::Reset(entityManager, entityID);
}

void RespawnPlayer(EntityManager& entityManager, RespawnComponent& respawn)
{
	Revive(entityManager, entityManager.GetEntityIDForComponent(respawn), respawn.respawnPosition, respawn.respawnYawDegrees);
	respawn.secondsDead = 0.0f;
}

bool IsDead(EntityManager& entityManager, unsigned int entityID)
{
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID);
	return health && health->isDead;
}

}

void RespawnSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (RespawnComponent& respawn : entityManager.GetAllComponents<RespawnComponent>())
	{
		const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityManager.GetEntityIDForComponent(respawn));

		respawn.respawnPosition   = transform ? transform->position : glm::vec3(0.0f);
		respawn.respawnYawDegrees = transform ? transform->rotation.y : 0.0f;
		respawn.secondsDead       = 0.0f;
	}
}

void RespawnSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (RespawnComponent& respawn : entityManager.GetAllComponents<RespawnComponent>())
	{
		if (!IsDead(entityManager, entityManager.GetEntityIDForComponent(respawn)))
			continue;

		respawn.secondsDead += deltaTime;

		if (respawn.secondsDead < respawn.respawnDelaySeconds)
			continue;

		// As in Dark Souls: the player and every enemy come back together
		RespawnPlayer(entityManager, respawn);
		RespawnEnemies(entityManager);
	}
}

void RespawnSystem::SetRespawnPoint(EntityManager& entityManager, unsigned int playerID, const glm::vec3& position, float yawDegrees)
{
	if (RespawnComponent* respawn = entityManager.GetComponent<RespawnComponent>(playerID))
	{
		respawn->respawnPosition   = position;
		respawn->respawnYawDegrees = yawDegrees;
	}
}

void RespawnSystem::RestoreCharacter(EntityManager& entityManager, unsigned int entityID)
{
	if (HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID))
		health->currentHealth = health->maxHealth;

	if (StaminaComponent* stamina = entityManager.GetComponent<StaminaComponent>(entityID))
		stamina->currentStamina = stamina->maxStamina;
}

void RespawnSystem::RespawnEnemies(EntityManager& entityManager)
{
	for (EnemyComponent& enemy : entityManager.GetAllComponents<EnemyComponent>())
	{
		if (!BossSystem::CanBeBroughtBack(entityManager, entityManager.GetEntityIDForComponent(enemy)))
			continue;

		Revive(entityManager, entityManager.GetEntityIDForComponent(enemy), enemy.homePosition, enemy.homeYawDegrees);

		enemy.stateMachine.reset();
		enemy.attackCooldownLeft = 0.0f;
	}
}

}
