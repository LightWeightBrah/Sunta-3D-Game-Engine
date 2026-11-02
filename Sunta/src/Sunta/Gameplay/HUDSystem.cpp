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

constexpr float PromptHeight        = 28.0f;
constexpr float PromptBottomMargin  = 150.0f;
constexpr float PromptPadding       = 14.0f;
const glm::vec4 PromptColor         = glm::vec4(0.92f, 0.88f, 0.8f, 1.0f);
const glm::vec4 PromptBackground    = glm::vec4(0.0f, 0.0f, 0.0f, 0.55f);

constexpr float NotificationSeconds = 3.5f;
constexpr float NotificationFadeOut = 1.2f;
constexpr float NotificationHeight  = 72.0f;
constexpr float NotificationAreaY   = 0.3f; // part of the HUD area from the top
const glm::vec4 NotificationColor   = glm::vec4(0.95f, 0.78f, 0.38f, 1.0f);

// The flask in the bottom left corner, the currency in the bottom right one (margins from those corners)
const glm::vec2 CornerMargin        = glm::vec2(40.0f, 40.0f);
constexpr float CornerTextHeight    = 26.0f;
constexpr float CornerLabelHeight   = 18.0f;
constexpr float CornerPadding       = 10.0f;
constexpr float GainTextHeight      = 22.0f;
constexpr float GainFadeOutSeconds  = 0.8f;
const glm::vec4 CornerBackground    = glm::vec4(0.0f, 0.0f, 0.0f, 0.55f);
const glm::vec4 CornerTextColor     = glm::vec4(0.92f, 0.88f, 0.8f, 1.0f);
const glm::vec4 CornerLabelColor    = glm::vec4(0.7f, 0.65f, 0.56f, 1.0f);
const glm::vec4 EmptyFlaskColor     = glm::vec4(0.45f, 0.42f, 0.38f, 1.0f);
const glm::vec4 GainColor           = glm::vec4(0.95f, 0.78f, 0.38f, 1.0f);

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

	// Back to zero once the player is alive again (brought back at a bonfire)
	hud->secondsSinceDeath = health->isDead ? hud->secondsSinceDeath + deltaTime : 0.0f;

	hud->notificationSecondsLeft = glm::max(hud->notificationSecondsLeft - deltaTime, 0.0f);
}

PlayerHUDComponent* TryGetPlayerHUD(EntityManager& entityManager)
{
	unsigned int playerID = 0;
	return CharacterControllerSystem::TryFindPlayer(entityManager, playerID) ? entityManager.GetComponent<PlayerHUDComponent>(playerID) : nullptr;
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

void DrawPrompt(const HUDArea& area, const PlayerHUDComponent& hud)
{
	if (hud.promptText.empty())
		return;

	const glm::vec2 center = glm::vec2(area.topLeft.x + area.size.x * 0.5f, area.topLeft.y + area.size.y - PromptBottomMargin);
	const glm::vec2 size   = glm::vec2(UIRenderer::MeasureText(hud.promptText, PromptHeight) + PromptPadding * 2.0f, PromptHeight + PromptPadding);

	UIRenderer::DrawRectangle(center - size * 0.5f, size, PromptBackground);
	UIRenderer::DrawTextCentered(hud.promptText, center, PromptHeight, PromptColor);
}

void DrawNotification(const HUDArea& area, const PlayerHUDComponent& hud)
{
	if (hud.notificationSecondsLeft <= 0.0f)
		return;

	const float alpha  = glm::clamp(hud.notificationSecondsLeft / NotificationFadeOut, 0.0f, 1.0f);
	const glm::vec2 center = glm::vec2(area.topLeft.x + area.size.x * 0.5f, area.topLeft.y + area.size.y * NotificationAreaY);

	UIRenderer::DrawTextCentered(hud.notificationText, center, NotificationHeight, WithAlpha(NotificationColor, alpha));
}

// The flask's name and the drinks left, e.g. "Estus Flask  3"
void DrawFlask(const HUDArea& area, const HealingFlaskComponent& flask)
{
	const std::string count = std::to_string(flask.charges);
	const float labelWidth  = UIRenderer::MeasureText(flask.displayName, CornerLabelHeight);
	const float countWidth  = UIRenderer::MeasureText(count, CornerTextHeight);
	const glm::vec2 size    = glm::vec2(labelWidth + countWidth + CornerPadding * 3.0f, CornerTextHeight + CornerPadding * 2.0f);
	const glm::vec2 topLeft = glm::vec2(area.topLeft.x + CornerMargin.x, area.topLeft.y + area.size.y - CornerMargin.y - size.y);

	const glm::vec4 color = flask.charges > 0 ? CornerTextColor : EmptyFlaskColor;
	const float labelTop  = topLeft.y + CornerPadding + (CornerTextHeight - CornerLabelHeight);

	UIRenderer::DrawRectangle(topLeft, size, CornerBackground);
	UIRenderer::DrawTextAt(flask.displayName, glm::vec2(topLeft.x + CornerPadding, labelTop), CornerLabelHeight, CornerLabelColor);
	UIRenderer::DrawTextAt(count, glm::vec2(topLeft.x + labelWidth + CornerPadding * 2.0f, topLeft.y + CornerPadding), CornerTextHeight, color);
}

// The amount counting up to its real value, the name next to it, and what was just gained above it
void DrawCurrency(const HUDArea& area, const CurrencyComponent& currency)
{
	const std::string amount = std::to_string((int)currency.shownAmount);
	const float labelWidth   = UIRenderer::MeasureText(currency.displayName, CornerLabelHeight);
	const float amountWidth  = UIRenderer::MeasureText(amount, CornerTextHeight);
	const glm::vec2 size     = glm::vec2(labelWidth + amountWidth + CornerPadding * 3.0f, CornerTextHeight + CornerPadding * 2.0f);
	const glm::vec2 topLeft  = area.topLeft + area.size - CornerMargin - size;
	const float labelTop     = topLeft.y + CornerPadding + (CornerTextHeight - CornerLabelHeight);

	UIRenderer::DrawRectangle(topLeft, size, CornerBackground);
	UIRenderer::DrawTextAt(currency.displayName, glm::vec2(topLeft.x + CornerPadding, labelTop), CornerLabelHeight, CornerLabelColor);
	UIRenderer::DrawTextAt(amount, glm::vec2(topLeft.x + labelWidth + CornerPadding * 2.0f, topLeft.y + CornerPadding), CornerTextHeight, CornerTextColor);

	if (currency.lastGainSecondsLeft <= 0.0f)
		return;

	const std::string gain = "+" + std::to_string(currency.lastGain);
	const float alpha      = glm::clamp(currency.lastGainSecondsLeft / GainFadeOutSeconds, 0.0f, 1.0f);
	const glm::vec2 gainTopLeft = glm::vec2(topLeft.x + size.x - UIRenderer::MeasureText(gain, GainTextHeight) - CornerPadding,
		topLeft.y - GainTextHeight - CornerPadding * 0.5f);

	UIRenderer::DrawTextAt(gain, gainTopLeft, GainTextHeight, WithAlpha(GainColor, alpha));
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

	if (const HealingFlaskComponent* flask = entityManager.GetComponent<HealingFlaskComponent>(playerID))
		DrawFlask(area, *flask);

	if (const CurrencyComponent* currency = entityManager.GetComponent<CurrencyComponent>(playerID))
		DrawCurrency(area, *currency);

	DrawPrompt(area, *hud);
	DrawNotification(area, *hud);

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
	// Engaged while it fights the player or is hurt: an enemy brought back at a bonfire hides its boss bar again
	const bool isHurt = health->currentHealth < health->maxHealth;
	bar.isEngaged       = wasHit || HasNoticedPlayer(entityManager, enemyID) || (bar.isEngaged && isHurt);
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

void HUDSystem::ShowPrompt(EntityManager& entityManager, const std::string& text)
{
	if (PlayerHUDComponent* hud = TryGetPlayerHUD(entityManager))
		hud->promptText = text;
}

void HUDSystem::ShowNotification(EntityManager& entityManager, const std::string& text)
{
	if (PlayerHUDComponent* hud = TryGetPlayerHUD(entityManager))
	{
		hud->notificationText        = text;
		hud->notificationSecondsLeft = NotificationSeconds;
	}
}

void HUDSystem::Draw(EntityManager& entityManager, const glm::mat4& viewProjection, const glm::vec2& screenSize, const HUDArea& area)
{
	UIRenderer::Begin(screenSize);

	// The enemies first: the player's own bars and the death message stay on top
	DrawEnemies(entityManager, viewProjection, area);
	DrawPlayer(entityManager, viewProjection, area);

	UIRenderer::End();

	// The prompt is shown only while some system keeps asking for it, every frame
	if (PlayerHUDComponent* hud = TryGetPlayerHUD(entityManager))
		hud->promptText.clear();
}

}
