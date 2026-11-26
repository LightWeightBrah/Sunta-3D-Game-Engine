#include "Core/SuntaPreCompiled.h"
#include "SummonSystem.h"

#include <glm/glm.hpp>

#include "BonfireSystem.h"
#include "EnemySystem.h"
#include "HUDSystem.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Effects/ParticleSystem.h"
#include "Scene/WorldSpawner.h"

namespace Sunta
{

namespace
{

constexpr int   CallKey           = SUNTA_KEY_G;
constexpr float SideMeters        = 2.2f; // the allies appear this far to the player's sides
constexpr float FallenFadeSeconds = 4.0f; // a fallen ally lies this long before it is gone
constexpr int   AppearBurst       = 70;

std::vector<std::string> SplitNames(const std::string& text)
{
	std::vector<std::string> names;
	std::string current;
	for (char c : text + ",")
	{
		if (c == ',')
		{
			const size_t first = current.find_first_not_of(' ');
			const size_t last  = current.find_last_not_of(' ');
			if (first != std::string::npos)
				names.push_back(current.substr(first, last - first + 1));
			current.clear();
		}
		else
			current += c;
	}
	return names;
}

bool IsDead(EntityManager& entityManager, unsigned int entityID)
{
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID);
	return health && health->isDead;
}

void SendAway(EntityManager& entityManager, SummonComponent& summon)
{
	for (const SummonComponent::Called& ally : summon.called)
		if (entityManager.IsAlive(ally.entityID))
			WorldSpawner::RequestDestroy(ally.entityID);
	summon.called.clear();
}

void Call(EntityManager& entityManager, unsigned int playerID, SummonComponent& summon)
{
	const TransformComponent* player = entityManager.GetComponent<TransformComponent>(playerID);
	if (!player)
		return;

	const std::vector<std::string> names = SplitNames(summon.allies);
	const float yaw = glm::radians(player->rotation.y);
	const glm::vec3 right = glm::vec3(std::cos(yaw), 0.0f, -std::sin(yaw));
	for (size_t i = 0; i < names.size(); ++i)
	{
		const float side = names.size() == 1 ? 1.0f : (i % 2 == 0 ? -1.0f : 1.0f) * (1.0f + 0.6f * (float)(i / 2));
		const glm::vec3 place = player->position + right * (side * SideMeters);

		unsigned int allyID = 0;
		if (!WorldSpawner::Spawn(entityManager, names[i], place, player->rotation.y, allyID))
		{
			SUNTA_ENGINE_LOG_WARNING("Summon: no ally named '{0}' in the scene", names[i]);
			continue;
		}
		EnemySystem::SetDormant(entityManager, allyID, false);
		if (EnemyComponent* enemy = entityManager.GetComponent<EnemyComponent>(allyID))
		{
			enemy->isAlly        = true;
			enemy->startsDormant = false;
		}
		// The player never locks on to his own
		if (LockOnTargetComponent* lockOnTarget = entityManager.GetComponent<LockOnTargetComponent>(allyID))
			lockOnTarget->isLockable = false;
		if (!summon.summonEffect.empty())
			ParticleSystem::BurstAt(entityManager, summon.summonEffect, place + glm::vec3(0.0f, 1.0f, 0.0f), AppearBurst);
		summon.called.push_back({ allyID, summon.lifetimeSeconds });
	}

	summon.charges--;
	if (!summon.calledText.empty())
		HUDSystem::ShowNotification(entityManager, summon.calledText);
	SUNTA_ENGINE_LOG_INFO("Summon: {0} allies called, {1} calls left", summon.called.size(), summon.charges);
}

}

void SummonSystem::Update(EntityManager& entityManager, float deltaTime)
{
	const bool isResting = BonfireSystem::IsPlayerResting(entityManager);

	for (SummonComponent& summon : entityManager.GetAllComponents<SummonComponent>())
	{
		const unsigned int playerID = entityManager.GetEntityIDForComponent(summon);
		if (summon.charges < 0)
			summon.charges = summon.maxCharges;

		// A rest at a bonfire: the allies go back, the calls come back
		if (isResting)
		{
			SendAway(entityManager, summon);
			summon.charges = summon.maxCharges;
			continue;
		}

		// Their time runs out; a fallen one lies a moment, then it is gone
		for (SummonComponent::Called& ally : summon.called)
		{
			if (!entityManager.IsAlive(ally.entityID))
			{
				ally.secondsLeft = 0.0f;
				continue;
			}
			if (IsDead(entityManager, ally.entityID))
				ally.secondsLeft = glm::min(ally.secondsLeft, FallenFadeSeconds);
			ally.secondsLeft -= deltaTime;
			if (ally.secondsLeft <= 0.0f)
			{
				const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(ally.entityID);
				if (transform && !summon.summonEffect.empty())
					ParticleSystem::BurstAt(entityManager, summon.summonEffect, transform->position + glm::vec3(0.0f, 1.0f, 0.0f), AppearBurst);
				WorldSpawner::RequestDestroy(ally.entityID);
			}
		}
		summon.called.erase(std::remove_if(summon.called.begin(), summon.called.end(),
			[](const SummonComponent::Called& ally) { return ally.secondsLeft <= 0.0f; }), summon.called.end());

		const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(playerID);
		const bool canCall = controller && controller->isPlayerControlled && !controller->isHeldByCutscene
		                     && summon.called.empty() && summon.charges > 0 && !IsDead(entityManager, playerID);
		if (canCall && InputManager::IsKeyDown(CallKey))
			Call(entityManager, playerID, summon);
	}
}

}
