#include "Core/SuntaPreCompiled.h"
#include "EnemySystem.h"

#include "BaseEnemyStates.h"
#include "CharacterControllerSystem.h"
#include "EnemyContext.h"
#include "EnemyStateMachine.h"
#include "EntityName.h"
#include "MeleeEnemy.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

// The enemy faces the player through its own Lock On, which keeps the player until it is this far away
constexpr float LockOnKeepDistance = 20.0f;

// ========== Kinds of enemies ==========

// A kind of enemy: the component that marks it and the states it brings
struct EnemyKind
{
	bool (*isOfKind)(EntityManager& entityManager, unsigned int entityID);
	void (*addStates)(EnemyStateMachine& stateMachine);
};

template<typename TKindComponent>
bool Has(EntityManager& entityManager, unsigned int entityID)
{
	return entityManager.GetComponent<TKindComponent>(entityID) != nullptr;
}

// Every kind the engine knows. A new kind = one line here
const EnemyKind EnemyKinds[] =
{
	{ Has<MeleeEnemyComponent>, MeleeEnemy::AddStates }
};

std::shared_ptr<EnemyStateMachine> MakeStateMachine(EntityManager& entityManager, unsigned int entityID)
{
	auto stateMachine = std::make_shared<EnemyStateMachine>(EnemyStateNames::Idle);
	AddBaseEnemyStates(*stateMachine);

	for (const EnemyKind& kind : EnemyKinds)
	{
		if (kind.isOfKind(entityManager, entityID))
			kind.addStates(*stateMachine);
	}

	return stateMachine;
}

// ========== Running the enemies ==========

void SwitchToDeadWhenDying(EnemyContext& context, EnemyStateMachine& stateMachine)
{
	const bool isAlreadyDead = stateMachine.GetCurrentStateName() == EnemyStateNames::Dead;

	if (context.IsDead() && !isAlreadyDead)
		stateMachine.RequestState(EnemyStateNames::Dead);
}

void UpdateEnemy(EntityManager& entityManager, EnemyComponent& enemy, float deltaTime)
{
	const unsigned int entityID = entityManager.GetEntityIDForComponent(enemy);

	EnemyContext context(entityManager, entityID, deltaTime);

	// The reason is reported once when the game starts (ReportProblemsBeforePlay), not every frame
	if (!context.IsValid())
		return;

	enemy.attackCooldownLeft = glm::max(enemy.attackCooldownLeft - deltaTime, 0.0f);

	EnemyStateMachine& stateMachine = *EnemySystem::GetOrCreateStateMachine(entityManager, entityID);

	SwitchToDeadWhenDying(context, stateMachine);
	stateMachine.Update(context);
}

void AddLockOnForFacingThePlayer(EntityManager& entityManager, unsigned int entityID)
{
	if (entityManager.GetComponent<LockOnComponent>(entityID))
		return;

	LockOnComponent& lockOn = entityManager.AddComponent<LockOnComponent>(entityID);
	lockOn.isPlayerControlled = false;
	lockOn.maxDistance        = LockOnKeepDistance;
}

}

void EnemySystem::PrepareForPlay(EntityManager& entityManager)
{
	for (EnemyComponent& enemy : entityManager.GetAllComponents<EnemyComponent>())
	{
		const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityManager.GetEntityIDForComponent(enemy));

		enemy.homePosition       = transform ? transform->position : glm::vec3(0.0f);
		enemy.attackCooldownLeft = 0.0f;
		enemy.stateMachine.reset();
	}
}

void EnemySystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (EnemyComponent& enemy : entityManager.GetAllComponents<EnemyComponent>())
		UpdateEnemy(entityManager, enemy, deltaTime);
}

EnemyStateMachine* EnemySystem::GetOrCreateStateMachine(EntityManager& entityManager, unsigned int entityID)
{
	EnemyComponent* enemy = entityManager.GetComponent<EnemyComponent>(entityID);
	if (!enemy)
		return nullptr;

	if (!enemy->stateMachine)
		enemy->stateMachine = MakeStateMachine(entityManager, entityID);

	return enemy->stateMachine.get();
}

void EnemySystem::AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID)
{
	entityManager.AddComponent<EnemyComponent>(entityID);

	if (!entityManager.GetComponent<CharacterControllerComponent>(entityID))
		CharacterControllerSystem::AddWithRequiredComponents(entityManager, entityID);

	entityManager.GetComponent<CharacterControllerComponent>(entityID)->isPlayerControlled = false;

	AddLockOnForFacingThePlayer(entityManager, entityID);
}

void EnemySystem::ReportProblemsBeforePlay(EntityManager& entityManager)
{
	for (EnemyComponent& enemy : entityManager.GetAllComponents<EnemyComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(enemy);
		const std::string  name     = GetEntityName(entityManager, entityID);

		const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(entityID);

		if (!controller)
			SUNTA_ENGINE_LOG_ERROR("Enemy '{0}' needs a Character Controller, it will not move", name);
		else if (controller->isPlayerControlled)
			SUNTA_ENGINE_LOG_ERROR("Enemy '{0}': turn off 'isPlayerControlled' on its Character Controller, or the keyboard moves the enemy", name);

		if (!entityManager.GetComponent<CharacterActionsComponent>(entityID))
			SUNTA_ENGINE_LOG_WARNING("Enemy '{0}' has no Character Actions, it will never attack", name);

		if (!entityManager.GetComponent<LockOnComponent>(entityID))
			SUNTA_ENGINE_LOG_WARNING("Enemy '{0}' has no Lock On, it will not face the player while it walks", name);
	}
}

}
