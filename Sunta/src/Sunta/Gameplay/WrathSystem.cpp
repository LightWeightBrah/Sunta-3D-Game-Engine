#include "Core/SuntaPreCompiled.h"
#include "WrathSystem.h"

#include <glm/glm.hpp>

#include "HUDSystem.h"
#include "FollowCameraSystem.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Effects/ParticleSystem.h"

namespace Sunta
{

namespace
{

constexpr int ReleaseKey = SUNTA_KEY_V;

// The figure burning: its rim this strong, an aura this far out (meters), the aura's light a part of the rim's
constexpr float RimStrength  = 1.1f;
constexpr float AuraSize     = 0.08f;
constexpr float AuraStrength = 0.6f;
// The moment it is set loose: a burst of this many specks, a shake and a flash of its color
constexpr int   ReleaseBurst      = 120;
constexpr float ReleaseShake      = 0.35f;
constexpr float ReleaseFlashAlpha = 0.25f;

WrathComponent* GetWrath(EntityManager& entityManager, unsigned int entityID)
{
	return entityManager.GetComponent<WrathComponent>(entityID);
}

void Fill(WrathComponent& wrath, float amount)
{
	// (it doesn't fill while it burns: a new one is earned after)
	if (amount <= 0.0f || wrath.IsActive())
		return;
	wrath.currentWrath     = glm::min(wrath.currentWrath + amount, wrath.maxWrath);
	wrath.secondsSinceFill = 0.0f;
}

void SetBurning(EntityManager& entityManager, unsigned int entityID, const WrathComponent& wrath, bool isBurning)
{
	if (ModelComponent* model = entityManager.GetComponent<ModelComponent>(entityID))
	{
		model->rimGlow = isBurning ? wrath.glowColor * RimStrength : glm::vec3(0.0f);
		model->aura    = isBurning ? wrath.glowColor * AuraStrength : glm::vec3(0.0f);
		if (isBurning)
			model->auraSize = AuraSize;
	}
}

bool WantsToRelease(EntityManager& entityManager, unsigned int entityID)
{
	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(entityID);
	return controller && controller->isPlayerControlled && !controller->isHeldByCutscene && InputManager::IsKeyDown(ReleaseKey);
}

void Release(EntityManager& entityManager, unsigned int entityID, WrathComponent& wrath)
{
	wrath.activeLeft   = wrath.durationSeconds;
	wrath.currentWrath = wrath.maxWrath;
	SetBurning(entityManager, entityID, wrath, true);

	if (const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID); transform && !wrath.releaseEffect.empty())
		ParticleSystem::BurstAt(entityManager, wrath.releaseEffect, transform->position + glm::vec3(0.0f, 1.2f, 0.0f), ReleaseBurst);
	FollowCameraSystem::Shake(entityManager, ReleaseShake);
	HUDSystem::FlashScreen(wrath.glowColor, 0.5f, 0.05f, ReleaseFlashAlpha);
	SUNTA_ENGINE_LOG_INFO("Wrath: set loose for {0} s", wrath.durationSeconds);
}

}

void WrathSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (WrathComponent& wrath : entityManager.GetAllComponents<WrathComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(wrath);

		if (wrath.IsActive())
		{
			// It burns down with its bar
			wrath.activeLeft  -= deltaTime;
			wrath.currentWrath = wrath.maxWrath * glm::max(wrath.activeLeft, 0.0f) / glm::max(wrath.durationSeconds, 0.01f);
			if (!wrath.IsActive())
			{
				wrath.activeLeft   = 0.0f;
				wrath.currentWrath = 0.0f;
				SetBurning(entityManager, entityID, wrath, false);
			}
			continue;
		}

		if (wrath.IsFull() && WantsToRelease(entityManager, entityID))
		{
			Release(entityManager, entityID, wrath);
			continue;
		}

		wrath.secondsSinceFill += deltaTime;
		if (wrath.secondsSinceFill > wrath.drainDelaySeconds && !wrath.IsFull())
			wrath.currentWrath = glm::max(wrath.currentWrath - wrath.drainPerSecond * deltaTime, 0.0f);
	}
}

void WrathSystem::OnDamageDealt(EntityManager& entityManager, unsigned int entityID, float damage)
{
	if (WrathComponent* wrath = GetWrath(entityManager, entityID))
		Fill(*wrath, damage * wrath->fromDamageDealt);
}

void WrathSystem::OnDamageTaken(EntityManager& entityManager, unsigned int entityID, float damage)
{
	if (WrathComponent* wrath = GetWrath(entityManager, entityID))
		Fill(*wrath, damage * wrath->fromDamageTaken);
}

void WrathSystem::OnDeflect(EntityManager& entityManager, unsigned int entityID)
{
	if (WrathComponent* wrath = GetWrath(entityManager, entityID))
		Fill(*wrath, wrath->fromDeflects);
}

float WrathSystem::GetDamageMultiplier(EntityManager& entityManager, unsigned int entityID)
{
	const WrathComponent* wrath = GetWrath(entityManager, entityID);
	return wrath && wrath->IsActive() ? wrath->damageMultiplier : 1.0f;
}

float WrathSystem::GetPostureMultiplier(EntityManager& entityManager, unsigned int entityID)
{
	const WrathComponent* wrath = GetWrath(entityManager, entityID);
	return wrath && wrath->IsActive() ? wrath->postureMultiplier : 1.0f;
}

}
