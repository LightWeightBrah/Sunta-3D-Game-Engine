#include "Core/SuntaPreCompiled.h"
#include "HUDSystem.h"

#include "BaseEnemyStates.h"
#include "CharacterControllerSystem.h"
#include "EnemyStateMachine.h"
#include "EntityName.h"
#include "HUDStyle.h"
#include "LevelUpSystem.h"
#include "LockOnSystem.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Renderer/Texture.h"
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

// ========== Bars with pictures (a HUD style) ==========

// A bar of a HUD style: the dark channel, the pale trail, the fill, then the frame over them (its ends keep their shape,
// only its middle grows with 'channelLength'). Returns the size of the whole frame on the screen
glm::vec2 DrawStyledBar(const HUDStyle& style, const HUDBarStyle& bar, const glm::vec2& topLeft, float channelLength,
	float fraction, float trailFraction, const glm::vec4& fillColor)
{
	const float scale = bar.height / (float)glm::max(bar.frame->GetHeight(), 1);
	const glm::vec2 frameSize = glm::vec2(channelLength + (bar.inner.x + bar.inner.z) * scale, bar.height);

	const glm::vec2 channelTopLeft = topLeft + glm::vec2(bar.inner.x, bar.inner.y) * scale;
	const float     channelHeight  = bar.height - (bar.inner.y + bar.inner.w) * scale;

	UIRenderer::DrawRectangle(channelTopLeft, glm::vec2(channelLength, channelHeight), style.channelColor);

	const auto drawFill = [&](float part, const glm::vec4& color)
	{
		const glm::vec2 size = glm::vec2(channelLength * glm::clamp(part, 0.0f, 1.0f), channelHeight);
		if (size.x <= 0.0f)
			return;

		if (style.fill)
			UIRenderer::DrawImage(channelTopLeft, size, *style.fill, color);
		else
			UIRenderer::DrawRectangle(channelTopLeft, size, color);
	};

	drawFill(trailFraction, style.trailColor);
	drawFill(fraction, fillColor);

	UIRenderer::DrawNineSlice(topLeft, frameSize, *bar.frame, glm::vec4(bar.capLeft, 0.0f, bar.capRight, 0.0f), scale);

	if (bar.ornament)
	{
		const float width = bar.ornament->GetWidth() * scale;
		UIRenderer::DrawImage(glm::vec2(topLeft.x + (frameSize.x - width) * 0.5f, topLeft.y), glm::vec2(width, bar.height), *bar.ornament);
	}

	return frameSize;
}

// The panel of a corner of a HUD style with its icon, the name and the number, e.g. the flask: "Estus Flask" over "3"
// 'isOnRight' puts it in the bottom right corner. Returns the top left corner of the panel
glm::vec2 DrawStyledCorner(const HUDStyle& style, const HUDArea& area, bool isOnRight, const Texture* icon,
	const std::string& label, const std::string& value, const glm::vec4& valueColor)
{
	const glm::vec2 size    = style.cornerSize;
	const float     x       = isOnRight ? area.topLeft.x + area.size.x - CornerMargin.x - size.x : area.topLeft.x + CornerMargin.x;
	const glm::vec2 topLeft = glm::vec2(x, area.topLeft.y + area.size.y - CornerMargin.y - size.y);

	UIRenderer::DrawNineSlice(topLeft, size, *style.panel, glm::vec4(style.panelBorder), 1.0f);

	float textLeft = topLeft.x + CornerPadding * 1.5f;
	if (icon)
	{
		const float iconWidth = icon->GetWidth() * style.iconHeight / (float)glm::max(icon->GetHeight(), 1);
		UIRenderer::DrawImage(glm::vec2(textLeft, topLeft.y + (size.y - style.iconHeight) * 0.5f), glm::vec2(iconWidth, style.iconHeight), *icon);
		textLeft += iconWidth + CornerPadding;
	}

	const float textTop = topLeft.y + (size.y - CornerLabelHeight - CornerTextHeight) * 0.5f;
	UIRenderer::DrawTextAt(label, glm::vec2(textLeft, textTop), CornerLabelHeight, CornerLabelColor);
	UIRenderer::DrawTextAt(value, glm::vec2(textLeft, textTop + CornerLabelHeight), CornerTextHeight, valueColor);
	return topLeft;
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

// Stamina first: the health bar's ornate end may reach over it
void DrawStyledPlayerBars(const HUDStyle& style, const HUDArea& area, const PlayerHUDComponent& hud, const HealthComponent* health, const StaminaComponent* stamina)
{
	if (stamina && style.stamina.IsUsable())
	{
		const float fraction = GetFraction(stamina->currentStamina, stamina->maxStamina);
		DrawStyledBar(style, style.stamina, area.topLeft + style.stamina.offset, stamina->maxStamina * hud.pixelsPerStaminaPoint,
			fraction, fraction, style.staminaColor);
	}

	if (health)
	{
		DrawStyledBar(style, style.health, area.topLeft + style.health.offset, health->maxHealth * hud.pixelsPerHealthPoint,
			GetFraction(health->currentHealth, health->maxHealth), GetFraction(hud.trailingHealth, health->maxHealth), style.healthColor);
	}
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

// What was just gained ("+60"), fading above the panel of the currency
void DrawCurrencyGain(const CurrencyComponent& currency, const glm::vec2& panelTopLeft, const glm::vec2& panelSize)
{
	if (currency.lastGainSecondsLeft <= 0.0f)
		return;

	const std::string gain = "+" + std::to_string(currency.lastGain);
	const float alpha      = glm::clamp(currency.lastGainSecondsLeft / GainFadeOutSeconds, 0.0f, 1.0f);
	const glm::vec2 gainTopLeft = glm::vec2(panelTopLeft.x + panelSize.x - UIRenderer::MeasureText(gain, GainTextHeight) - CornerPadding,
		panelTopLeft.y - GainTextHeight - CornerPadding * 0.5f);

	UIRenderer::DrawTextAt(gain, gainTopLeft, GainTextHeight, WithAlpha(GainColor, alpha));
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

	DrawCurrencyGain(currency, topLeft, size);
}

void DrawStyledCurrency(const HUDStyle& style, const HUDArea& area, const CurrencyComponent& currency)
{
	const glm::vec2 topLeft = DrawStyledCorner(style, area, true, style.currencyIcon.get(), currency.displayName,
		std::to_string((int)currency.shownAmount), CornerTextColor);
	DrawCurrencyGain(currency, topLeft, style.cornerSize);
}

void DrawPlayer(EntityManager& entityManager, const glm::mat4& viewProjection, const HUDArea& area)
{
	unsigned int playerID = 0;
	if (!CharacterControllerSystem::TryFindPlayer(entityManager, playerID))
		return;

	const PlayerHUDComponent* hud = entityManager.GetComponent<PlayerHUDComponent>(playerID);
	if (!hud)
		return;

	const HUDStyle* style = HUDStyle::Get(hud->style);
	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(playerID);

	if (style && style->health.IsUsable())
		DrawStyledPlayerBars(*style, area, *hud, health, entityManager.GetComponent<StaminaComponent>(playerID));
	else
		DrawPlayerBars(area, *hud, health, entityManager.GetComponent<StaminaComponent>(playerID));

	const LockOnComponent* lockOn = entityManager.GetComponent<LockOnComponent>(playerID);
	if (hud->showLockOnMarker && lockOn)
		DrawLockOnMarker(entityManager, *lockOn, viewProjection);

	const bool hasStyledCorners = style && style->panel;

	if (const HealingFlaskComponent* flask = entityManager.GetComponent<HealingFlaskComponent>(playerID))
	{
		if (hasStyledCorners)
			DrawStyledCorner(*style, area, false, style->flaskIcon.get(), flask->displayName, std::to_string(flask->charges),
				flask->charges > 0 ? CornerTextColor : EmptyFlaskColor);
		else
			DrawFlask(area, *flask);
	}

	if (const CurrencyComponent* currency = entityManager.GetComponent<CurrencyComponent>(playerID))
	{
		if (hasStyledCorners)
			DrawStyledCurrency(*style, area, *currency);
		else
			DrawCurrency(area, *currency);
	}

	LevelUpSystem::DrawMenu(entityManager, area);
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

void DrawBossBar(EntityManager& entityManager, const HUDArea& area, const EnemyHealthBarComponent& bar, unsigned int enemyID,
	const HealthComponent& health, const HUDStyle* style)
{
	if (!bar.isEngaged)
		return;

	const std::string name = bar.displayName.empty() ? GetEntityName(entityManager, enemyID) : bar.displayName;

	// With a style: the framed bar centered at the bottom, the name above its channel
	if (style && style->boss.IsUsable())
	{
		const HUDBarStyle& boss = style->boss;
		const float channelLength = area.size.x * BossBarWidthOfScreen;
		const float scale         = boss.height / (float)glm::max(boss.frame->GetHeight(), 1);
		const float frameWidth    = channelLength + (boss.inner.x + boss.inner.z) * scale;
		const glm::vec2 topLeft   = area.topLeft + glm::vec2((area.size.x - frameWidth) * 0.5f, area.size.y - BossBarBottomMargin - boss.height) + boss.offset;

		DrawStyledBar(*style, boss, topLeft, channelLength, GetFraction(health.currentHealth, health.maxHealth),
			GetFraction(bar.trailingHealth, health.maxHealth), style->healthColor);

		const glm::vec2 nameTopLeft = glm::vec2(topLeft.x + boss.inner.x * scale, topLeft.y + boss.inner.y * scale - BossNameHeight - BossNameGap);
		UIRenderer::DrawTextAt(name, nameTopLeft, BossNameHeight, BossNameColor);
		return;
	}

	const glm::vec2 size    = glm::vec2(area.size.x * BossBarWidthOfScreen, BossBarHeight);
	const glm::vec2 topLeft = area.topLeft + glm::vec2((area.size.x - size.x) * 0.5f, area.size.y - BossBarBottomMargin - size.y);

	UIRenderer::DrawTextAt(name, topLeft - glm::vec2(0.0f, BossNameHeight + BossNameGap), BossNameHeight, BossNameColor);
	DrawBar(topLeft, size, GetFraction(health.currentHealth, health.maxHealth), GetFraction(bar.trailingHealth, health.maxHealth), HealthColor);
}

void DrawEnemies(EntityManager& entityManager, const glm::mat4& viewProjection, const HUDArea& area)
{
	const PlayerHUDComponent* hud = TryGetPlayerHUD(entityManager);
	const HUDStyle* style = hud ? HUDStyle::Get(hud->style) : nullptr;

	for (EnemyHealthBarComponent& bar : entityManager.GetAllComponents<EnemyHealthBarComponent>())
	{
		const unsigned int enemyID = entityManager.GetEntityIDForComponent(bar);
		const HealthComponent* health = entityManager.GetComponent<HealthComponent>(enemyID);

		if (!health || health->isDead)
			continue;

		if (bar.isBoss)
			DrawBossBar(entityManager, area, bar, enemyID, *health, style);
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
