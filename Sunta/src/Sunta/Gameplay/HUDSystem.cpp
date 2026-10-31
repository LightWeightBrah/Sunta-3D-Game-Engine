#include "Core/SuntaPreCompiled.h"
#include "HUDSystem.h"

#include "BaseEnemyStates.h"
#include "CharacterControllerSystem.h"
#include "EnemyStateMachine.h"
#include "EntityName.h"
#include "LockOnSystem.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Renderer/UIRenderer.h"

namespace Sunta
{

namespace
{

// ========== Look of the HUD (pixels and colors) ==========

// From the top left corner of the HUD area
const glm::vec2 PlayerBarsOffset  = glm::vec2(40.0f, 34.0f);
constexpr float HealthBarHeight   = 14.0f;
constexpr float StaminaBarHeight  = 10.0f;
constexpr float GapBetweenBars    = 7.0f;
constexpr float BarBorder         = 2.0f;

const glm::vec2 EnemyBarSize = glm::vec2(90.0f, 6.0f);

constexpr float BossBarWidthOfScreen = 0.6f;
constexpr float BossBarHeight        = 12.0f;
constexpr float BossBarBottomMargin  = 70.0f;
constexpr float BossNameHeight       = 26.0f;
constexpr float BossNameGap          = 6.0f;

constexpr float LockOnMarkerSize    = 8.0f;
constexpr float LockOnMarkerOutline = 2.0f;

constexpr float DeathBandHeight       = 150.0f;
constexpr float DeathMessageHeight    = 96.0f;
constexpr float DeathMessageDelay     = 1.2f;
constexpr float DeathMessageFadeIn    = 1.5f;

const glm::vec4 BarBackgroundColor = glm::vec4(0.05f, 0.04f, 0.04f, 0.75f);
const glm::vec4 BarBorderColor     = glm::vec4(0.45f, 0.39f, 0.28f, 0.9f);
const glm::vec4 TrailColor         = glm::vec4(0.85f, 0.74f, 0.45f, 0.9f);
const glm::vec4 HealthColor        = glm::vec4(0.62f, 0.08f, 0.06f, 1.0f);
const glm::vec4 StaminaColor       = glm::vec4(0.24f, 0.52f, 0.18f, 1.0f);
const glm::vec4 BossNameColor      = glm::vec4(0.92f, 0.88f, 0.8f, 1.0f);
const glm::vec4 MarkerColor        = glm::vec4(1.0f, 1.0f, 1.0f, 0.95f);
const glm::vec4 MarkerOutlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.8f);
const glm::vec4 DeathBandColor     = glm::vec4(0.0f, 0.0f, 0.0f, 0.65f);
const glm::vec4 DeathMessageColor  = glm::vec4(0.62f, 0.05f, 0.04f, 1.0f);

// How fast the pale part of a bar (the health just lost) shrinks: this part of the whole bar per second
constexpr float TrailShrinkPerSecond = 0.35f;

// ========== Small helpers ==========

float GetFraction(float value, float maximum)
{
	return maximum > 0.0f ? glm::clamp(value / maximum, 0.0f, 1.0f) : 0.0f;
}

// Follows the value down slowly, and up at once (healing)
float ShrinkTrailTowards(float trailing, float value, float maximum, float deltaTime)
{
	return glm::max(value, trailing - maximum * TrailShrinkPerSecond * deltaTime);
}

glm::vec4 WithAlpha(const glm::vec4& color, float alpha)
{
	return glm::vec4(glm::vec3(color), color.a * alpha);
}

// A frame, a dark background, the pale trail and the filled part
void DrawBar(const glm::vec2& topLeft, const glm::vec2& size, float fraction, float trailFraction, const glm::vec4& fillColor)
{
	UIRenderer::DrawRectangle(topLeft - glm::vec2(BarBorder), size + glm::vec2(BarBorder * 2.0f), BarBorderColor);
	UIRenderer::DrawRectangle(topLeft, size, BarBackgroundColor);
	UIRenderer::DrawRectangle(topLeft, glm::vec2(size.x * trailFraction, size.y), TrailColor);
	UIRenderer::DrawRectangle(topLeft, glm::vec2(size.x * fraction, size.y), fillColor);
}

// ========== The player ==========

void UpdatePlayer(EntityManager& entityManager, float deltaTime)
{
	unsigned int playerID = 0;
	if (!CharacterControllerSystem::TryFindPlayer(entityManager, playerID))
		return;

	PlayerHUDComponent* hud = entityManager.GetComponent<PlayerHUDComponent>(playerID);
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(playerID);
	if (!hud || !health)
		return;

	hud->trailingHealth = ShrinkTrailTowards(hud->trailingHealth, health->currentHealth, health->maxHealth, deltaTime);

	if (health->isDead)
		hud->secondsSinceDeath += deltaTime;
}

void DrawPlayerBars(const HUDArea& area, const PlayerHUDComponent& hud, const HealthComponent* health, const StaminaComponent* stamina)
{
	glm::vec2 topLeft = area.topLeft + PlayerBarsOffset;

	if (health)
	{
		const glm::vec2 size = glm::vec2(health->maxHealth * hud.pixelsPerHealthPoint, HealthBarHeight);
		DrawBar(topLeft, size, GetFraction(health->currentHealth, health->maxHealth), GetFraction(hud.trailingHealth, health->maxHealth), HealthColor);
		topLeft.y += HealthBarHeight + GapBetweenBars;
	}

	if (stamina)
	{
		const glm::vec2 size = glm::vec2(stamina->maxStamina * hud.pixelsPerStaminaPoint, StaminaBarHeight);
		const float fraction = GetFraction(stamina->currentStamina, stamina->maxStamina);
		DrawBar(topLeft, size, fraction, fraction, StaminaColor);
	}
}

void DrawLockOnMarker(EntityManager& entityManager, const LockOnComponent& lockOn, const glm::mat4& viewProjection)
{
	glm::vec3 aimPoint;
	glm::vec2 pixel;

	if (!LockOnSystem::TryGetTargetAimPoint(entityManager, lockOn, aimPoint) || !UIRenderer::TryProjectToScreen(aimPoint, viewProjection, pixel))
		return;

	const glm::vec2 outlineSize = glm::vec2(LockOnMarkerSize + LockOnMarkerOutline * 2.0f);

	UIRenderer::DrawRectangle(pixel - outlineSize * 0.5f, outlineSize, MarkerOutlineColor);
	UIRenderer::DrawRectangle(pixel - glm::vec2(LockOnMarkerSize * 0.5f), glm::vec2(LockOnMarkerSize), MarkerColor);
}

void DrawDeathMessage(const HUDArea& area, const PlayerHUDComponent& hud)
{
	const float fadeIn = glm::clamp((hud.secondsSinceDeath - DeathMessageDelay) / DeathMessageFadeIn, 0.0f, 1.0f);
	if (fadeIn <= 0.0f)
		return;

	const glm::vec2 center = area.topLeft + area.size * 0.5f;
	const glm::vec2 bandTopLeft = glm::vec2(area.topLeft.x, center.y - DeathBandHeight * 0.5f);

	UIRenderer::DrawRectangle(bandTopLeft, glm::vec2(area.size.x, DeathBandHeight), WithAlpha(DeathBandColor, fadeIn));
	UIRenderer::DrawTextCentered(hud.deathMessage, center, DeathMessageHeight, WithAlpha(DeathMessageColor, fadeIn));
}

void DrawPlayer(EntityManager& entityManager, const glm::mat4& viewProjection, const HUDArea& area)
{
	unsigned int playerID = 0;
	if (!CharacterControllerSystem::TryFindPlayer(entityManager, playerID))
		return;

	const PlayerHUDComponent* hud = entityManager.GetComponent<PlayerHUDComponent>(playerID);
	if (!hud)
		return;

	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(playerID);
	DrawPlayerBars(area, *hud, health, entityManager.GetComponent<StaminaComponent>(playerID));

	const LockOnComponent* lockOn = entityManager.GetComponent<LockOnComponent>(playerID);
	if (hud->showLockOnMarker && lockOn)
		DrawLockOnMarker(entityManager, *lockOn, viewProjection);

	if (health && health->isDead)
		DrawDeathMessage(area, *hud);
}

// ========== The enemies ==========

bool HasNoticedPlayer(EntityManager& entityManager, unsigned int enemyID)
{
	const EnemyComponent* enemy = entityManager.GetComponent<EnemyComponent>(enemyID);
	return enemy && enemy->stateMachine && enemy->stateMachine->GetCurrentStateName() != EnemyStateNames::Idle;
}

void UpdateEnemyBar(EntityManager& entityManager, EnemyHealthBarComponent& bar, float deltaTime)
{
	const unsigned int enemyID = entityManager.GetEntityIDForComponent(bar);
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(enemyID);
	if (!health)
		return;

	const bool wasHit = health->currentHealth < bar.healthLastFrame;

	bar.secondsSinceHit = wasHit ? 0.0f : glm::min(bar.secondsSinceHit + deltaTime, EnemyHealthBarComponent::MaxSeconds);
	bar.isEngaged       = bar.isEngaged || wasHit || HasNoticedPlayer(entityManager, enemyID);
	bar.trailingHealth  = ShrinkTrailTowards(bar.trailingHealth, health->currentHealth, health->maxHealth, deltaTime);
	bar.healthLastFrame = health->currentHealth;
}

bool IsLockedOnByPlayer(EntityManager& entityManager, unsigned int enemyID)
{
	unsigned int playerID = 0;
	if (!CharacterControllerSystem::TryFindPlayer(entityManager, playerID))
		return false;

	const LockOnComponent* lockOn = entityManager.GetComponent<LockOnComponent>(playerID);
	return lockOn && lockOn->hasTarget && lockOn->targetEntityID == enemyID;
}

void DrawFloatingBar(EntityManager& entityManager, const EnemyHealthBarComponent& bar, unsigned int enemyID,
	const HealthComponent& health, const glm::mat4& viewProjection)
{
	const bool isWanted = bar.secondsSinceHit < bar.showSecondsAfterHit || IsLockedOnByPlayer(entityManager, enemyID);
	const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(enemyID);

	glm::vec2 pixel;
	if (!isWanted || !transform || !UIRenderer::TryProjectToScreen(transform->position + glm::vec3(0.0f, bar.heightAboveFeet, 0.0f), viewProjection, pixel))
		return;

	const glm::vec2 topLeft = pixel - glm::vec2(EnemyBarSize.x * 0.5f, EnemyBarSize.y);
	DrawBar(topLeft, EnemyBarSize, GetFraction(health.currentHealth, health.maxHealth), GetFraction(bar.trailingHealth, health.maxHealth), HealthColor);
}

void DrawBossBar(EntityManager& entityManager, const HUDArea& area, const EnemyHealthBarComponent& bar, unsigned int enemyID, const HealthComponent& health)
{
	if (!bar.isEngaged)
		return;

	const glm::vec2 size    = glm::vec2(area.size.x * BossBarWidthOfScreen, BossBarHeight);
	const glm::vec2 topLeft = area.topLeft + glm::vec2((area.size.x - size.x) * 0.5f, area.size.y - BossBarBottomMargin - size.y);

	const std::string name = bar.displayName.empty() ? GetEntityName(entityManager, enemyID) : bar.displayName;

	UIRenderer::DrawTextAt(name, topLeft - glm::vec2(0.0f, BossNameHeight + BossNameGap), BossNameHeight, BossNameColor);
	DrawBar(topLeft, size, GetFraction(health.currentHealth, health.maxHealth), GetFraction(bar.trailingHealth, health.maxHealth), HealthColor);
}

void DrawEnemies(EntityManager& entityManager, const glm::mat4& viewProjection, const HUDArea& area)
{
	for (EnemyHealthBarComponent& bar : entityManager.GetAllComponents<EnemyHealthBarComponent>())
	{
		const unsigned int enemyID = entityManager.GetEntityIDForComponent(bar);
		const HealthComponent* health = entityManager.GetComponent<HealthComponent>(enemyID);

		if (!health || health->isDead)
			continue;

		if (bar.isBoss)
			DrawBossBar(entityManager, area, bar, enemyID, *health);
		else
			DrawFloatingBar(entityManager, bar, enemyID, *health, viewProjection);
	}
}

}

void HUDSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (PlayerHUDComponent& hud : entityManager.GetAllComponents<PlayerHUDComponent>())
	{
		const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityManager.GetEntityIDForComponent(hud));

		hud.trailingHealth    = health ? health->maxHealth : 0.0f;
		hud.secondsSinceDeath = 0.0f;
	}

	for (EnemyHealthBarComponent& bar : entityManager.GetAllComponents<EnemyHealthBarComponent>())
	{
		const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityManager.GetEntityIDForComponent(bar));
		const float maxHealth = health ? health->maxHealth : 0.0f;

		bar.trailingHealth  = maxHealth;
		bar.healthLastFrame = maxHealth;
		bar.secondsSinceHit = EnemyHealthBarComponent::MaxSeconds;
		bar.isEngaged       = false;
	}
}

void HUDSystem::Update(EntityManager& entityManager, float deltaTime)
{
	UpdatePlayer(entityManager, deltaTime);

	for (EnemyHealthBarComponent& bar : entityManager.GetAllComponents<EnemyHealthBarComponent>())
		UpdateEnemyBar(entityManager, bar, deltaTime);
}

void HUDSystem::Draw(EntityManager& entityManager, const glm::mat4& viewProjection, const glm::vec2& screenSize, const HUDArea& area)
{
	UIRenderer::Begin(screenSize);

	// The enemies first: the player's own bars and the death message stay on top
	DrawEnemies(entityManager, viewProjection, area);
	DrawPlayer(entityManager, viewProjection, area);

	UIRenderer::End();
}

}
