#include "Core/SuntaPreCompiled.h"
#include "HUDSystem.h"
#include "Core/EngineTime.h"
#include "BossComponent.h"

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
#include "Scene/EntityHierarchy.h"
#include "CutsceneSystem.h"

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
// Two bosses fighting at once: the bar of the second one sits this much above the first
constexpr float BossBarRowSpacing    = 84.0f;
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
// The victory banner: gold on the same dark band, in, held, out (seconds)
const glm::vec4 VictoryMessageColor = glm::vec4(0.95f, 0.78f, 0.38f, 1.0f);
constexpr float VictoryFadeIn  = 1.2f;
constexpr float VictoryHold    = 4.0f;
constexpr float VictoryFadeOut = 1.5f;

constexpr float PromptHeight        = 28.0f;
// Above the bars of two bosses fighting at once
constexpr float PromptBottomMargin  = 250.0f;
constexpr float PromptPadding       = 14.0f;
const glm::vec4 PromptColor         = glm::vec4(0.92f, 0.88f, 0.8f, 1.0f);
const glm::vec4 PromptBackground    = glm::vec4(0.0f, 0.0f, 0.0f, 0.62f);
const glm::vec4 PromptLineColor     = glm::vec4(0.85f, 0.68f, 0.36f, 0.75f);
const glm::vec4 BarSheenColor       = glm::vec4(1.0f, 0.95f, 0.85f, 0.14f);
const glm::vec4 BarShadeColor       = glm::vec4(0.0f, 0.0f, 0.0f, 0.28f);
const glm::vec4 BarEdgeColor        = glm::vec4(1.0f, 0.9f, 0.7f, 0.35f);

// The posture bar (Sekiro): a thin bar over the health bar that fills from its middle to both ends, gold turning to a
// hot orange as it nears full, white while the posture is broken; hidden while empty
constexpr float PostureBarHeight     = 5.0f;
constexpr float PostureBarGap        = 5.0f;
constexpr float BossPostureWidth     = 0.36f; // of the boss bar's length
const glm::vec3 RallyTint            = glm::vec3(1.0f, 0.62f, 0.25f); // the rally's part of the health bar: an ember orange
constexpr float WrathBarWidth        = 220.0f;
constexpr float WrathBarHeight       = 4.0f;
constexpr float WrathBarGap          = 7.0f;
constexpr float PlayerPostureWidth   = 0.26f; // of the screen's width
constexpr float PlayerPostureFromBottom = 128.0f; // above the boss bar and its name
const glm::vec4 PostureLowColor      = glm::vec4(0.85f, 0.66f, 0.28f, 1.0f);
const glm::vec4 PostureHighColor     = glm::vec4(1.0f, 0.36f, 0.08f, 1.0f);
const glm::vec4 PostureBrokenColor   = glm::vec4(1.0f, 0.96f, 0.86f, 1.0f);
const glm::vec4 PostureBackground    = glm::vec4(0.0f, 0.0f, 0.0f, 0.55f);

constexpr float NotificationSeconds = 3.5f;
constexpr float NotificationFadeOut = 1.2f;
constexpr float NotificationHeight  = 72.0f;
constexpr float NotificationAreaY   = 0.3f; // part of the HUD area from the top
constexpr float NotificationUnderReadingY = 0.72f;
constexpr float NotificationWidestPart   = 0.8f; // of the HUD area: a longer message is broken into lines
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

// The player's spells: a row of slots under the player's bars, each with its key, its name and a thin bar
// that fills while the spell comes back
constexpr float SpellSlotHeight      = 34.0f;
constexpr float SpellSlotGap         = 8.0f;
constexpr float SpellTextHeight      = 18.0f;
constexpr float SpellKeyBoxWidth     = 26.0f;
constexpr float SpellCooldownHeight  = 4.0f;
constexpr float SpellRowBelowBars    = 16.0f; // from the bottom of the player's bars to the top of the row
const glm::vec4 SpellKeyBackground   = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f);
const glm::vec4 SpellSlotBackground  = glm::vec4(0.03f, 0.025f, 0.02f, 0.62f);
const glm::vec4 SpellSlotFrame       = glm::vec4(0.55f, 0.45f, 0.28f, 0.45f);
const glm::vec4 SpellReadyColor      = glm::vec4(0.95f, 0.78f, 0.38f, 1.0f);
const glm::vec4 SpellWaitingColor    = glm::vec4(0.5f, 0.47f, 0.42f, 1.0f);

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
	float fraction, float trailFraction, const glm::vec4& fillColor, float rallyFraction = 0.0f)
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

	// The light on the filled part: a sheen along its top, a shade along its bottom, a bright edge where it ends
	const auto drawSheen = [&](float part)
	{
		const float width = channelLength * glm::clamp(part, 0.0f, 1.0f);
		if (width <= 1.0f)
			return;
		UIRenderer::DrawRectangle(channelTopLeft, glm::vec2(width, channelHeight * 0.3f), BarSheenColor);
		UIRenderer::DrawRectangle(channelTopLeft + glm::vec2(0.0f, channelHeight * 0.72f), glm::vec2(width, channelHeight * 0.28f), BarShadeColor);
		UIRenderer::DrawRectangle(channelTopLeft + glm::vec2(width - 2.0f, 0.0f), glm::vec2(2.0f, channelHeight), BarEdgeColor);
	};

	drawFill(trailFraction, style.trailColor);
	// The health a quick blow back can still win (the rally): the fill's color, paler
	if (rallyFraction > fraction)
		drawFill(rallyFraction, glm::vec4(glm::mix(glm::vec3(fillColor), RallyTint, 0.55f), 1.0f));
	drawFill(fraction, fillColor);
	drawSheen(fraction);

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
	hud->readingSecondsLeft      = glm::max(hud->readingSecondsLeft - deltaTime, 0.0f);

	if (!hud->victoryText.empty())
	{
		if (hud->victorySeconds < 0.0f)
		{
			hud->victoryWaitLeft = glm::max(hud->victoryWaitLeft - deltaTime, 0.0f);
			if (hud->victoryWaitLeft <= 0.0f && !CutsceneSystem::IsPlaying(entityManager))
				hud->victorySeconds = 0.0f;
		}
		else
		{
			hud->victorySeconds += deltaTime;
			if (hud->victorySeconds > VictoryFadeIn + VictoryHold + VictoryFadeOut)
			{
				hud->victoryText.clear();
				hud->victorySeconds = -1.0f;
			}
		}
	}
}

PlayerHUDComponent* TryGetPlayerHUD(EntityManager& entityManager)
{
	unsigned int playerID = 0;
	return CharacterControllerSystem::TryFindPlayer(entityManager, playerID) ? entityManager.GetComponent<PlayerHUDComponent>(playerID) : nullptr;
}

// Stamina first: the health bar's ornate end may reach over it
// Returns where the lower of the two bars ends on the screen (what is drawn under them starts there)
float DrawStyledPlayerBars(const HUDStyle& style, const HUDArea& area, const PlayerHUDComponent& hud, const HealthComponent* health, const StaminaComponent* stamina)
{
	float bottom = area.topLeft.y;

	if (stamina && style.stamina.IsUsable())
	{
		const float fraction = GetFraction(stamina->currentStamina, stamina->maxStamina);
		const glm::vec2 topLeft = area.topLeft + style.stamina.offset;
		bottom = glm::max(bottom, topLeft.y + DrawStyledBar(style, style.stamina, topLeft, stamina->maxStamina * hud.pixelsPerStaminaPoint,
			fraction, fraction, style.staminaColor).y);
	}

	if (health)
	{
		const glm::vec2 topLeft = area.topLeft + style.health.offset;
		bottom = glm::max(bottom, topLeft.y + DrawStyledBar(style, style.health, topLeft, health->maxHealth * hud.pixelsPerHealthPoint,
			GetFraction(health->currentHealth, health->maxHealth), GetFraction(hud.trailingHealth, health->maxHealth), style.healthColor,
			GetFraction(health->currentHealth + health->rallyHealth, health->maxHealth)).y);
	}

	return bottom;
}

void DrawPlayerBars(const HUDArea& area, const PlayerHUDComponent& hud, const HealthComponent* health, const StaminaComponent* stamina)
{
	glm::vec2 topLeft = area.topLeft + PlayerBarsOffset;

	if (health)
	{
		const glm::vec2 size = glm::vec2(health->maxHealth * hud.pixelsPerHealthPoint, HealthBarHeight);
		DrawBar(topLeft, size, GetFraction(health->currentHealth, health->maxHealth), GetFraction(hud.trailingHealth, health->maxHealth), HealthColor);
		if (health->rallyHealth > 0.0f)
			UIRenderer::DrawRectangle(topLeft + glm::vec2(size.x * GetFraction(health->currentHealth, health->maxHealth), 0.0f),
				glm::vec2(size.x * GetFraction(health->rallyHealth, health->maxHealth), size.y), glm::vec4(RallyTint, 0.8f));
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

void DrawVictoryMessage(const HUDArea& area, const PlayerHUDComponent& hud)
{
	if (hud.victoryText.empty() || hud.victorySeconds < 0.0f)
		return;

	const float t = hud.victorySeconds;
	const float alpha = t < VictoryFadeIn ? t / VictoryFadeIn
	                  : glm::clamp(1.0f - (t - VictoryFadeIn - VictoryHold) / VictoryFadeOut, 0.0f, 1.0f);
	const glm::vec2 center = area.topLeft + area.size * 0.5f;
	const glm::vec2 bandTopLeft = glm::vec2(area.topLeft.x, center.y - DeathBandHeight * 0.5f);

	UIRenderer::DrawRectangle(bandTopLeft, glm::vec2(area.size.x, DeathBandHeight), WithAlpha(DeathBandColor, alpha));
	UIRenderer::DrawTextCentered(hud.victoryText, center, DeathMessageHeight * 0.85f, WithAlpha(VictoryMessageColor, alpha));
}

void DrawPrompt(const HUDArea& area, const PlayerHUDComponent& hud)
{
	if (hud.promptText.empty())
		return;

	const glm::vec2 center = glm::vec2(area.topLeft.x + area.size.x * 0.5f, area.topLeft.y + area.size.y - PromptBottomMargin);
	const float     width  = UIRenderer::MeasureText(hud.promptText, PromptHeight) + PromptPadding * 8.0f;
	const float     height = PromptHeight + PromptPadding * 1.4f;

	// A dark band fading out to the sides, between two thin gold lines (no hard box)
	UIRenderer::DrawFadeBand(center, glm::vec2(width, height), PromptBackground);
	UIRenderer::DrawFadeLine(center - glm::vec2(0.0f, height * 0.5f), width * 0.8f, 1.5f, PromptLineColor);
	UIRenderer::DrawFadeLine(center + glm::vec2(0.0f, height * 0.5f), width * 0.8f, 1.5f, PromptLineColor);
	UIRenderer::DrawTextCentered(hud.promptText, center, PromptHeight, PromptColor);
}

std::vector<std::string> WrapText(const std::string& text, float width, float pixelHeight);

void DrawNotification(const HUDArea& area, const PlayerHUDComponent& hud)
{
	if (hud.notificationSecondsLeft <= 0.0f)
		return;

	const float alpha  = glm::clamp(hud.notificationSecondsLeft / NotificationFadeOut, 0.0f, 1.0f);
	// While a text is read (a book's panel in the upper part of the screen) the message goes low, under it
	const float areaY = hud.readingSecondsLeft > 0.0f ? NotificationUnderReadingY : NotificationAreaY;
	const glm::vec2 center = glm::vec2(area.topLeft.x + area.size.x * 0.5f, area.topLeft.y + area.size.y * areaY);

	// A long message is broken into lines (and a smaller text) so it never runs off the screen
	const float widest = area.size.x * NotificationWidestPart;
	float height = NotificationHeight;
	std::vector<std::string> lines = { hud.notificationText };
	if (UIRenderer::MeasureText(hud.notificationText, height) > widest)
	{
		height = NotificationHeight * 0.75f;
		lines  = WrapText(hud.notificationText, widest, height);
	}
	float lineWidth = 0.0f;
	for (const std::string& line : lines)
		lineWidth = glm::max(lineWidth, UIRenderer::MeasureText(line, height));
	const float bandHeight = height * (1.5f + 1.15f * (float)(lines.size() - 1));

	// (a faint dark band behind it and gold lines above and below, as the great messages of souls-likes)
	const float width = lineWidth + height * 3.0f;
	UIRenderer::DrawFadeBand(center, glm::vec2(width, bandHeight), WithAlpha(PromptBackground, alpha * 0.8f));
	UIRenderer::DrawFadeLine(center - glm::vec2(0.0f, bandHeight * 0.5f), width * 0.7f, 2.0f, WithAlpha(PromptLineColor, alpha));
	UIRenderer::DrawFadeLine(center + glm::vec2(0.0f, bandHeight * 0.5f), width * 0.7f, 2.0f, WithAlpha(PromptLineColor, alpha));
	for (size_t i = 0; i < lines.size(); ++i)
	{
		const float y = center.y + ((float)i - (float)(lines.size() - 1) * 0.5f) * height * 1.15f;
		UIRenderer::DrawTextCentered(lines[i], glm::vec2(center.x, y), height, WithAlpha(NotificationColor, alpha));
	}
}

// ========== A text read aloud (a book, a letter) ==========

constexpr float ReadingTextHeight  = 22.0f;
constexpr float ReadingTitleHeight = 30.0f;
constexpr float ReadingLineGap     = 7.0f;
constexpr float ReadingWidthOfArea = 0.5f;
constexpr float ReadingFadeSeconds = 0.8f;
constexpr float ReadingTopOfArea   = 0.08f;
const glm::vec4 ReadingBackground  = glm::vec4(0.02f, 0.015f, 0.02f, 0.72f);
const glm::vec4 ReadingTextColor   = glm::vec4(0.9f, 0.86f, 0.78f, 1.0f);

// The words broken into lines no wider than 'width' (a \n in the text always starts a new line)
std::vector<std::string> WrapText(const std::string& text, float width, float pixelHeight)
{
	std::vector<std::string> lines;
	std::string paragraph;
	std::stringstream paragraphs(text);
	while (std::getline(paragraphs, paragraph, '\n'))
	{
		std::stringstream words(paragraph);
		std::string word, line;
		while (words >> word)
		{
			const std::string longer = line.empty() ? word : line + " " + word;
			if (!line.empty() && UIRenderer::MeasureText(longer, pixelHeight) > width)
			{
				lines.push_back(line);
				line = word;
			}
			else
				line = longer;
		}
		lines.push_back(line);
	}
	return lines;
}

void DrawReading(const HUDArea& area, const PlayerHUDComponent& hud)
{
	if (hud.readingSecondsLeft <= 0.0f || hud.readingText.empty())
		return;

	const float alpha = glm::clamp(hud.readingSecondsLeft / ReadingFadeSeconds, 0.0f, 1.0f);
	const float width = area.size.x * ReadingWidthOfArea;
	const std::vector<std::string> lines = WrapText(hud.readingText, width, ReadingTextHeight);
	const float lineStep = ReadingTextHeight + ReadingLineGap;
	const float height = ReadingTitleHeight + ReadingLineGap * 3.0f + lineStep * static_cast<float>(lines.size());
	const glm::vec2 topLeft = glm::vec2(area.topLeft.x + (area.size.x - width) * 0.5f, area.topLeft.y + area.size.y * ReadingTopOfArea);
	const float padding = 18.0f;

	UIRenderer::DrawRectangle(topLeft - glm::vec2(padding), glm::vec2(width, height) + glm::vec2(padding * 2.0f), WithAlpha(ReadingBackground, ReadingBackground.a * alpha));
	UIRenderer::DrawFadeLine(glm::vec2(topLeft.x + width * 0.5f, topLeft.y - padding), width, 2.0f, WithAlpha(PromptLineColor, alpha));
	UIRenderer::DrawFadeLine(glm::vec2(topLeft.x + width * 0.5f, topLeft.y + height + padding), width, 2.0f, WithAlpha(PromptLineColor, alpha));
	UIRenderer::DrawTextCentered(hud.readingTitle, glm::vec2(topLeft.x + width * 0.5f, topLeft.y + ReadingTitleHeight * 0.5f),
		ReadingTitleHeight, WithAlpha(NotificationColor, alpha));
	float y = topLeft.y + ReadingTitleHeight + ReadingLineGap * 3.0f;
	for (const std::string& line : lines)
	{
		UIRenderer::DrawTextAt(line, glm::vec2(topLeft.x, y), ReadingTextHeight, WithAlpha(ReadingTextColor, alpha));
		y += lineStep;
	}
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

// True when the spell is one of this character's (a Spell Caster on it or anywhere below it)
bool BelongsTo(EntityManager& entityManager, unsigned int spellEntityID, unsigned int characterID)
{
	unsigned int entityID = spellEntityID;
	while (entityID != characterID)
	{
		if (!EntityHierarchy::TryGetParent(entityManager, entityID, entityID))
			return false;
	}

	return true;
}

// The spells the player casts with keys, in the order of their keys: "[1] Orb", dim with a filling bar while it comes back
void DrawSpells(EntityManager& entityManager, unsigned int playerID, const HUDArea& area, float barsBottom)
{
	std::vector<const SpellCasterComponent*> spells;
	for (SpellCasterComponent& spell : entityManager.GetAllComponents<SpellCasterComponent>())
	{
		if (!spell.castKey.empty() && BelongsTo(entityManager, entityManager.GetEntityIDForComponent(spell), playerID))
			spells.push_back(&spell);
	}

	std::sort(spells.begin(), spells.end(), [](const SpellCasterComponent* left, const SpellCasterComponent* right)
		{ return left->castKey < right->castKey; });

	float x = area.topLeft.x + CornerMargin.x;
	const float top = barsBottom + SpellRowBelowBars;

	for (const SpellCasterComponent* spell : spells)
	{
		const std::string key  = spell->castKey.substr(0, 1);
		const std::string name = spell->displayName.empty()
			? GetEntityName(entityManager, entityManager.GetEntityIDForComponent(*spell)) : spell->displayName;

		const bool  isReady   = spell->cooldownLeft <= 0.0f;
		const float nameWidth = UIRenderer::MeasureText(name, SpellTextHeight);
		const glm::vec2 size  = glm::vec2(SpellKeyBoxWidth + nameWidth + CornerPadding * 2.0f, SpellSlotHeight);
		const glm::vec2 slot  = glm::vec2(x, top);
		const float textTop   = top + (SpellSlotHeight - SpellCooldownHeight - SpellTextHeight) * 0.5f;

		// A dark slot with a thin frame; the key in a small square framed in gold when the spell is ready
		const glm::vec2 keySize = glm::vec2(SpellKeyBoxWidth, SpellSlotHeight - SpellCooldownHeight);
		UIRenderer::DrawRectangle(slot, size, SpellSlotBackground);
		UIRenderer::DrawRectangleOutline(slot, size, 1.0f, SpellSlotFrame);
		UIRenderer::DrawRectangle(slot + glm::vec2(3.0f), keySize - glm::vec2(6.0f), SpellKeyBackground);
		UIRenderer::DrawRectangleOutline(slot + glm::vec2(3.0f), keySize - glm::vec2(6.0f), 1.5f, isReady ? SpellReadyColor : SpellWaitingColor);
		UIRenderer::DrawTextCentered(key, slot + keySize * 0.5f, SpellTextHeight, isReady ? SpellReadyColor : SpellWaitingColor);
		UIRenderer::DrawTextAt(name, glm::vec2(x + SpellKeyBoxWidth + CornerPadding * 0.6f, textTop), SpellTextHeight, isReady ? CornerTextColor : SpellWaitingColor);

		// While it comes back: a thin line filling along its bottom
		const float ready = 1.0f - GetFraction(spell->cooldownLeft, spell->cooldownSeconds);
		if (!isReady)
			UIRenderer::DrawRectangle(glm::vec2(x, top + SpellSlotHeight - SpellCooldownHeight * 0.5f),
				glm::vec2(size.x * ready, SpellCooldownHeight * 0.5f), SpellWaitingColor);

		x += size.x + SpellSlotGap;
	}
}

void DrawPostureBar(EntityManager& entityManager, unsigned int entityID, const glm::vec2& center, float width);

// The Wrath (see WrathComponent): a thin bar in its color under the player's bars, pulsing once full ("V" beside it),
// burning down while it is set loose. Returns where it ends (nothing drawn while it is empty)
float DrawWrathBar(const WrathComponent& wrath, float left, float top)
{
	if (wrath.currentWrath <= 0.0f && !wrath.IsActive())
		return top;

	static float pulseSeconds = 0.0f;
	pulseSeconds += EngineTime::deltaTime;
	const float pulse = wrath.IsFull() && !wrath.IsActive() ? 0.65f + 0.35f * std::sin(pulseSeconds * 6.0f) : 1.0f;

	const glm::vec2 topLeft = glm::vec2(left, top + WrathBarGap);
	UIRenderer::DrawRectangle(topLeft - glm::vec2(1.0f), glm::vec2(WrathBarWidth + 2.0f, WrathBarHeight + 2.0f), PostureBackground);
	UIRenderer::DrawRectangle(topLeft, glm::vec2(WrathBarWidth * glm::clamp(wrath.GetFraction(), 0.0f, 1.0f), WrathBarHeight),
		glm::vec4(wrath.glowColor * pulse, 1.0f));
	if (wrath.IsFull() && !wrath.IsActive())
		UIRenderer::DrawTextAt("V", glm::vec2(topLeft.x + WrathBarWidth + 8.0f, topLeft.y - 5.0f), 14.0f, glm::vec4(wrath.glowColor, pulse));
	return topLeft.y + WrathBarHeight;
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

	// Where the two bars end: the spells are listed under them
	float barsBottom = area.topLeft.y + PlayerBarsOffset.y + HealthBarHeight + GapBetweenBars + StaminaBarHeight;

	if (style && style->health.IsUsable())
		barsBottom = DrawStyledPlayerBars(*style, area, *hud, health, entityManager.GetComponent<StaminaComponent>(playerID));
	else
		DrawPlayerBars(area, *hud, health, entityManager.GetComponent<StaminaComponent>(playerID));

	// The player's own posture (Sekiro): low in the middle of the screen, where the eyes are during a duel
	DrawPostureBar(entityManager, playerID, glm::vec2(area.topLeft.x + area.size.x * 0.5f, area.topLeft.y + area.size.y - PlayerPostureFromBottom),
		area.size.x * PlayerPostureWidth);

	if (const WrathComponent* wrath = entityManager.GetComponent<WrathComponent>(playerID))
		barsBottom = DrawWrathBar(*wrath, area.topLeft.x + PlayerBarsOffset.x, barsBottom);

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

	DrawSpells(entityManager, playerID, area, barsBottom);

	LevelUpSystem::DrawMenu(entityManager, area);
	DrawPrompt(area, *hud);
	DrawNotification(area, *hud);
	DrawReading(area, *hud);
	DrawVictoryMessage(area, *hud);

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
	// A boss shows its bar when its fight starts (the fog closed behind the player), not when it sees the player
	// from across its arena
	if (const BossComponent* boss = entityManager.GetComponent<BossComponent>(enemyID))
		bar.isEngaged = bar.isEngaged && boss->isAwake;
	// An enemy asleep (a summon not called yet) is not in the fight
	if (const EnemyComponent* enemy = entityManager.GetComponent<EnemyComponent>(enemyID); enemy && enemy->isDormant)
		bar.isEngaged = false;
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

// The posture bar centered on 'center' (its middle), 'width' long; nothing for a character without posture or at 0
void DrawPostureBar(EntityManager& entityManager, unsigned int entityID, const glm::vec2& center, float width)
{
	const PostureComponent* posture = entityManager.GetComponent<PostureComponent>(entityID);
	if (!posture || (posture->currentPosture <= 0.0f && !posture->IsBroken()))
		return;

	const float fraction = posture->IsBroken() ? 1.0f : glm::clamp(posture->GetFraction(), 0.0f, 1.0f);
	const glm::vec4 color = posture->IsBroken() ? PostureBrokenColor : glm::mix(PostureLowColor, PostureHighColor, fraction * fraction);

	const glm::vec2 size = glm::vec2(width, PostureBarHeight);
	UIRenderer::DrawRectangle(center - size * 0.5f - glm::vec2(1.0f), size + glm::vec2(2.0f), PostureBackground);
	const glm::vec2 filled = glm::vec2(width * fraction, PostureBarHeight);
	UIRenderer::DrawRectangle(center - filled * 0.5f, filled, color);
	// The two ends of the fill burn brighter
	if (filled.x > 4.0f)
	{
		UIRenderer::DrawRectangle(glm::vec2(center.x - filled.x * 0.5f, center.y - filled.y * 0.5f), glm::vec2(2.0f, filled.y), PostureBrokenColor);
		UIRenderer::DrawRectangle(glm::vec2(center.x + filled.x * 0.5f - 2.0f, center.y - filled.y * 0.5f), glm::vec2(2.0f, filled.y), PostureBrokenColor);
	}
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
	DrawPostureBar(entityManager, enemyID, glm::vec2(pixel.x, topLeft.y - PostureBarGap - PostureBarHeight * 0.5f), EnemyBarSize.x);
}

// 'row' 0 is the bar at the bottom, every next boss gets the row above. False when the bar is not shown yet
bool DrawBossBar(EntityManager& entityManager, const HUDArea& area, const EnemyHealthBarComponent& bar, unsigned int enemyID,
	const HealthComponent& health, const HUDStyle* style, int row)
{
	if (!bar.isEngaged)
		return false;

	// (the rows as far apart as the bars are tall: a thin styled bar doesn't leave a gap the size of a thick one)
	const float rowSpacing = (style && style->boss.IsUsable()) ? style->boss.height * 0.7f + BossNameHeight + BossNameGap : BossBarRowSpacing;
	const glm::vec2 rise = glm::vec2(0.0f, -rowSpacing * row);

	const std::string name = bar.displayName.empty() ? GetEntityName(entityManager, enemyID) : bar.displayName;

	// With a style: the framed bar centered at the bottom, the name above its channel
	if (style && style->boss.IsUsable())
	{
		const HUDBarStyle& boss = style->boss;
		const float channelLength = area.size.x * BossBarWidthOfScreen;
		const float scale         = boss.height / (float)glm::max(boss.frame->GetHeight(), 1);
		const float frameWidth    = channelLength + (boss.inner.x + boss.inner.z) * scale;
		const glm::vec2 topLeft   = area.topLeft + glm::vec2((area.size.x - frameWidth) * 0.5f, area.size.y - BossBarBottomMargin - boss.height) + boss.offset + rise;

		DrawStyledBar(*style, boss, topLeft, channelLength, GetFraction(health.currentHealth, health.maxHealth),
			GetFraction(bar.trailingHealth, health.maxHealth), style->healthColor);

		const glm::vec2 nameTopLeft = glm::vec2(topLeft.x + boss.inner.x * scale, topLeft.y + boss.inner.y * scale - BossNameHeight - BossNameGap);
		UIRenderer::DrawTextAt(name, nameTopLeft, BossNameHeight, BossNameColor);
		DrawPostureBar(entityManager, enemyID, glm::vec2(topLeft.x + frameWidth * 0.5f, nameTopLeft.y - PostureBarGap - PostureBarHeight),
			channelLength * BossPostureWidth);
		return true;
	}

	const glm::vec2 size    = glm::vec2(area.size.x * BossBarWidthOfScreen, BossBarHeight);
	const glm::vec2 topLeft = area.topLeft + glm::vec2((area.size.x - size.x) * 0.5f, area.size.y - BossBarBottomMargin - size.y) + rise;

	UIRenderer::DrawTextAt(name, topLeft - glm::vec2(0.0f, BossNameHeight + BossNameGap), BossNameHeight, BossNameColor);
	DrawBar(topLeft, size, GetFraction(health.currentHealth, health.maxHealth), GetFraction(bar.trailingHealth, health.maxHealth), HealthColor);
	DrawPostureBar(entityManager, enemyID, glm::vec2(topLeft.x + size.x * 0.5f, topLeft.y - BossNameHeight - BossNameGap - PostureBarGap - PostureBarHeight),
		size.x * BossPostureWidth);
	return true;
}

void DrawEnemies(EntityManager& entityManager, const glm::mat4& viewProjection, const HUDArea& area)
{
	const PlayerHUDComponent* hud = TryGetPlayerHUD(entityManager);
	const HUDStyle* style = hud ? HUDStyle::Get(hud->style) : nullptr;
	int bossBarsDrawn = 0;

	for (EnemyHealthBarComponent& bar : entityManager.GetAllComponents<EnemyHealthBarComponent>())
	{
		const unsigned int enemyID = entityManager.GetEntityIDForComponent(bar);
		const HealthComponent* health = entityManager.GetComponent<HealthComponent>(enemyID);

		if (!health || health->isDead)
			continue;

		if (bar.isBoss)
			bossBarsDrawn += DrawBossBar(entityManager, area, bar, enemyID, *health, style, bossBarsDrawn) ? 1 : 0;
		else
			DrawFloatingBar(entityManager, bar, enemyID, *health, viewProjection);
	}
}

}

namespace
{

// The flash of the screen going on now (see HUDSystem::FlashScreen)
struct ScreenFlash
{
	glm::vec3 color       = glm::vec3(1.0f);
	float     holdLeft    = 0.0f;
	float     fadeSeconds = 0.0f;
	float     fadeLeft    = 0.0f;
	float     strength    = 1.0f;
};

ScreenFlash& GetScreenFlash()
{
	static ScreenFlash flash;
	return flash;
}

}

void HUDSystem::PrepareForPlay(EntityManager& entityManager)
{
	GetScreenFlash() = ScreenFlash();

	for (PlayerHUDComponent& hud : entityManager.GetAllComponents<PlayerHUDComponent>())
	{
		const HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityManager.GetEntityIDForComponent(hud));

		hud.trailingHealth    = health ? health->maxHealth : 0.0f;
		hud.secondsSinceDeath = 0.0f;
		hud.victoryText.clear();
		hud.victorySeconds = -1.0f;
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
	ScreenFlash& flash = GetScreenFlash();
	if (flash.holdLeft > 0.0f)
		flash.holdLeft -= deltaTime;
	else
		flash.fadeLeft = glm::max(flash.fadeLeft - deltaTime, 0.0f);

	UpdatePlayer(entityManager, deltaTime);

	for (EnemyHealthBarComponent& bar : entityManager.GetAllComponents<EnemyHealthBarComponent>())
		UpdateEnemyBar(entityManager, bar, deltaTime);
}

void HUDSystem::FlashScreen(const glm::vec3& color, float seconds, float holdSeconds, float strength)
{
	ScreenFlash& flash = GetScreenFlash();
	flash.color       = color;
	flash.holdLeft    = glm::max(holdSeconds, 0.0f);
	flash.fadeSeconds = glm::max(seconds, 0.01f);
	flash.fadeLeft    = flash.fadeSeconds;
	flash.strength    = glm::clamp(strength, 0.0f, 1.0f);
}

namespace
{
constexpr float PerilousSignSeconds = 0.9f;
float perilousSignLeft = 0.0f;
}

void HUDSystem::ShowPerilousSign()
{
	perilousSignLeft = PerilousSignSeconds;
}

void HUDSystem::DrawScreenFlash(const glm::vec2& screenSize)
{
	if (perilousSignLeft > 0.0f)
	{
		// It swells in over the first fifth, then fades: a red diamond with a white "!" in it, as a warning sign
		perilousSignLeft = glm::max(perilousSignLeft - EngineTime::deltaTime, 0.0f);
		const float age   = 1.0f - perilousSignLeft / PerilousSignSeconds;
		const float swell = glm::smoothstep(0.0f, 0.2f, age);
		const float alpha = age < 0.2f ? swell : glm::clamp((1.0f - age) / 0.8f, 0.0f, 1.0f);
		const float size  = screenSize.y * (0.06f + 0.02f * swell);
		const glm::vec2 center = glm::vec2(screenSize.x * 0.5f, screenSize.y * 0.36f);

		UIRenderer::Begin(screenSize);
		// the diamond: stacked bars, widest in the middle
		const int rows = 24;
		for (int row = 0; row < rows; ++row)
		{
			const float t = (row + 0.5f) / rows * 2.0f - 1.0f;
			const float half = size * (1.0f - std::abs(t));
			const float y = center.y + t * size;
			UIRenderer::DrawRectangle(glm::vec2(center.x - half, y - size / rows), glm::vec2(half * 2.0f, size * 2.0f / rows + 0.5f),
				glm::vec4(0.75f, 0.03f, 0.02f, alpha * 0.85f));
		}
		UIRenderer::DrawTextCentered("!", center, size * 1.1f, glm::vec4(1.0f, 0.92f, 0.85f, alpha));
		UIRenderer::End();
	}

	const ScreenFlash& flash = GetScreenFlash();
	if (flash.fadeLeft <= 0.0f)
		return;

	const float alpha = flash.strength * (flash.holdLeft > 0.0f ? 1.0f : glm::clamp(flash.fadeLeft / flash.fadeSeconds, 0.0f, 1.0f));

	UIRenderer::Begin(screenSize);
	UIRenderer::DrawRectangle(glm::vec2(0.0f), screenSize, glm::vec4(flash.color, alpha));
	UIRenderer::End();
}

void HUDSystem::ShowPrompt(EntityManager& entityManager, const std::string& text)
{
	if (PlayerHUDComponent* hud = TryGetPlayerHUD(entityManager))
		hud->promptText = text;
}

void HUDSystem::ShowVictory(EntityManager& entityManager, const std::string& text, float afterSeconds)
{
	if (PlayerHUDComponent* hud = TryGetPlayerHUD(entityManager))
	{
		hud->victoryText     = text;
		hud->victoryWaitLeft = afterSeconds;
		hud->victorySeconds  = -1.0f;
	}
}

void HUDSystem::ShowReading(EntityManager& entityManager, const std::string& title, const std::string& text, float seconds)
{
	if (PlayerHUDComponent* hud = TryGetPlayerHUD(entityManager))
	{
		hud->readingTitle       = title;
		hud->readingText        = text;
		hud->readingSecondsLeft = seconds;
	}
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
