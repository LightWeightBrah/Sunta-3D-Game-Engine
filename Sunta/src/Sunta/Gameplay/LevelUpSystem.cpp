#include "Core/SuntaPreCompiled.h"
#include "LevelUpSystem.h"

#include <glm/glm.hpp>

#include "CharacterControllerSystem.h"
#include "CurrencySystem.h"
#include "EntityName.h"
#include "HUDSystem.h"
#include "Audio/Audio.h"
#include "Audio/AudioSystem.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "Core/Log.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Renderer/UIRenderer.h"
#include "Scene/EntityHierarchy.h"

namespace Sunta
{

namespace
{

// ========== Keys ==========

constexpr int UpKeys[]      = { SUNTA_KEY_W, SUNTA_KEY_UP };
constexpr int DownKeys[]    = { SUNTA_KEY_S, SUNTA_KEY_DOWN };
constexpr int AddKeys[]     = { SUNTA_KEY_D, SUNTA_KEY_RIGHT };
constexpr int RemoveKeys[]  = { SUNTA_KEY_A, SUNTA_KEY_LEFT };
constexpr int ConfirmKeys[] = { SUNTA_KEY_ENTER, SUNTA_KEY_SPACE };

constexpr const char* ChooseHint  = "[W/S] choose     [A/D] take back / add a point";
constexpr const char* ConfirmHint = "[Enter] level up     [E] leave";
constexpr const char* StatsTitle  = "Stats";

// How many specks the level up's effect throws at once, and how long the menu glows after it
constexpr int   LevelUpBurst      = 70;
constexpr float RaisedGlowSeconds = 0.8f;

// ========== Look of the menu (pixels and colors) ==========

constexpr float AttributesWidth  = 470.0f;
constexpr float StatsWidth       = 380.0f;
constexpr float PanelGap         = 16.0f;
constexpr float MenuSideMargin   = 70.0f;
constexpr float MenuPadding      = 22.0f;
constexpr float TitleHeight      = 30.0f;
constexpr float TextHeight       = 24.0f;
constexpr float RowHeight        = 36.0f;
constexpr float HintHeight       = 17.0f;
constexpr float HintLineHeight   = 24.0f;
constexpr float SectionGap       = 14.0f;
constexpr float LineThickness    = 1.0f;

// The two value columns ("10  ->  11") from the right edge of a panel: the value now ends here, the arrow is centered here
constexpr float NowColumnFromRight   = 96.0f;
constexpr float ArrowColumnFromRight = 66.0f;

// The arrow between the values: a shaft and a head made of thin bars
constexpr float ArrowShaftLength = 12.0f;
constexpr float ArrowThickness   = 2.0f;
constexpr int   ArrowHeadColumns = 6;

const glm::vec4 MenuBackground    = glm::vec4(0.02f, 0.02f, 0.02f, 0.8f);
const glm::vec4 MenuBorder        = glm::vec4(0.45f, 0.39f, 0.28f, 0.9f);
const glm::vec4 TitleColor        = glm::vec4(0.95f, 0.78f, 0.38f, 1.0f);
const glm::vec4 TextColor         = glm::vec4(0.92f, 0.88f, 0.8f, 1.0f);
const glm::vec4 LabelColor        = glm::vec4(0.7f, 0.65f, 0.56f, 1.0f);
const glm::vec4 ArrowColor        = glm::vec4(0.6f, 0.55f, 0.47f, 1.0f);
const glm::vec4 RaisedValueColor  = glm::vec4(0.45f, 0.68f, 1.0f, 1.0f);  // blue, as in Dark Souls
const glm::vec4 LoweredValueColor = glm::vec4(0.85f, 0.3f, 0.25f, 1.0f);
const glm::vec4 TooExpensiveColor = glm::vec4(0.85f, 0.3f, 0.25f, 1.0f);
const glm::vec4 SelectedRowColor  = glm::vec4(0.45f, 0.39f, 0.28f, 0.35f);
const glm::vec4 GlowColor         = glm::vec4(0.95f, 0.78f, 0.38f, 0.35f);

// ========== The attributes ==========

// One row of the menu: the attribute, its name in the game, its points and the points added but not confirmed
struct AttributeRow
{
	LevelUpAttribute   attribute;
	const std::string* name;
	int*               points;
	int*               pendingPoints;
};

std::vector<AttributeRow> GetRows(LevelUpComponent& levelUp)
{
	const AttributeRow all[] =
	{
		{ LevelUpAttribute::Health,  &levelUp.healthAttribute,  &levelUp.healthPoints,  &levelUp.pendingHealthPoints  },
		{ LevelUpAttribute::Stamina, &levelUp.staminaAttribute, &levelUp.staminaPoints, &levelUp.pendingStaminaPoints },
		{ LevelUpAttribute::Damage,  &levelUp.damageAttribute,  &levelUp.damagePoints,  &levelUp.pendingDamagePoints  }
	};

	std::vector<AttributeRow> rows;
	for (const AttributeRow& row : all)
	{
		if (!row.name->empty())
			rows.push_back(row);
	}
	return rows;
}

int* GetPoints(LevelUpComponent& levelUp, LevelUpAttribute attribute)
{
	switch (attribute)
	{
	case LevelUpAttribute::Health:  return &levelUp.healthPoints;
	case LevelUpAttribute::Stamina: return &levelUp.staminaPoints;
	default:                        return &levelUp.damagePoints;
	}
}

void ForgetPendingPoints(LevelUpComponent& levelUp)
{
	levelUp.pendingHealthPoints  = 0;
	levelUp.pendingStaminaPoints = 0;
	levelUp.pendingDamagePoints  = 0;
}

// The maximum health and stamina are the values from the scene plus what the points give
void ApplyPoints(EntityManager& entityManager, unsigned int entityID, const LevelUpComponent& levelUp)
{
	if (HealthComponent* health = entityManager.GetComponent<HealthComponent>(entityID))
		health->maxHealth = levelUp.baseMaxHealth + levelUp.healthPoints * levelUp.healthPerPoint;

	if (StaminaComponent* stamina = entityManager.GetComponent<StaminaComponent>(entityID))
		stamina->maxStamina = levelUp.baseMaxStamina + levelUp.staminaPoints * levelUp.staminaPerPoint;
}

// The new part of the bar comes filled
void AddGainToCurrent(EntityManager& entityManager, unsigned int entityID, const LevelUpComponent& levelUp, LevelUpAttribute attribute)
{
	HealthComponent*  health  = entityManager.GetComponent<HealthComponent>(entityID);
	StaminaComponent* stamina = entityManager.GetComponent<StaminaComponent>(entityID);

	if (attribute == LevelUpAttribute::Health && health && !health->isDead)
		health->currentHealth = glm::min(health->currentHealth + levelUp.healthPerPoint, health->maxHealth);

	if (attribute == LevelUpAttribute::Stamina && stamina)
		stamina->currentStamina = glm::min(stamina->currentStamina + levelUp.staminaPerPoint, stamina->maxStamina);
}

// Pays for the next level and gives the attribute one point. False when the Currency is too low
bool RaiseOnePoint(EntityManager& entityManager, unsigned int entityID, LevelUpComponent& levelUp, LevelUpAttribute attribute)
{
	CurrencyComponent* currency = entityManager.GetComponent<CurrencyComponent>(entityID);
	const int cost = levelUp.GetCost();
	if (!currency || currency->amount < cost)
		return false;

	CurrencySystem::Add(entityManager, entityID, -cost);

	*GetPoints(levelUp, attribute) += 1;
	levelUp.level += 1;

	ApplyPoints(entityManager, entityID, levelUp);
	AddGainToCurrent(entityManager, entityID, levelUp, attribute);
	return true;
}

void BurstEffect(EntityManager& entityManager, unsigned int entityID, const std::string& effectName)
{
	if (effectName.empty())
		return;

	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, entityID))
	{
		const TagComponent*       tag     = entityManager.GetComponent<TagComponent>(childID);
		ParticleEmitterComponent* emitter = entityManager.GetComponent<ParticleEmitterComponent>(childID);
		if (tag && emitter && tag->name == effectName)
			emitter->burstCount += LevelUpBurst;
	}
}

// Once per level up, however many points it gave
void Celebrate(EntityManager& entityManager, unsigned int entityID, LevelUpComponent& levelUp, int fromLevel, int paid)
{
	levelUp.raisedGlowSecondsLeft = RaisedGlowSeconds;

	BurstEffect(entityManager, entityID, levelUp.levelUpEffect);
	AudioSystem::PlayAtEntity(entityManager, entityID, levelUp.levelUpSound);

	const CurrencyComponent* currency = entityManager.GetComponent<CurrencyComponent>(entityID);
	SUNTA_ENGINE_LOG_INFO("Level Up: '{0}' went from {1} {2} to {3} for {4} {5}", GetEntityName(entityManager, entityID),
		levelUp.displayName, fromLevel, levelUp.level, paid, currency ? currency->displayName : "");
}

// ========== The menu ==========

template<size_t Count>
bool IsAnyKeyDown(const int (&keys)[Count])
{
	for (int key : keys)
	{
		if (InputManager::IsKeyDown(key))
			return true;
	}
	return false;
}

// The player rests at a bonfire: the menu is open
bool TryFindPlayerInMenu(EntityManager& entityManager, unsigned int& outPlayerID)
{
	if (!CharacterControllerSystem::TryFindPlayer(entityManager, outPlayerID))
		return false;

	const CharacterActionsComponent*    actions    = entityManager.GetComponent<CharacterActionsComponent>(outPlayerID);
	const CharacterControllerComponent* controller = entityManager.GetComponent<CharacterControllerComponent>(outPlayerID);

	return actions && actions->IsResting() && !(controller && controller->isHeldByCutscene)
		&& entityManager.GetComponent<LevelUpComponent>(outPlayerID);
}

int GetCurrencyAmount(EntityManager& entityManager, unsigned int entityID)
{
	const CurrencyComponent* currency = entityManager.GetComponent<CurrencyComponent>(entityID);
	return currency ? currency->amount : 0;
}

// Enough Currency for the points already added and one more
bool CanAddPoint(EntityManager& entityManager, unsigned int playerID, const LevelUpComponent& levelUp)
{
	const int nextCost = levelUp.GetCostAt(levelUp.level + levelUp.GetPendingPoints());
	return GetCurrencyAmount(entityManager, playerID) >= levelUp.GetPendingCost() + nextCost;
}

// Pays for all the points added in the menu
void Confirm(EntityManager& entityManager, unsigned int playerID, LevelUpComponent& levelUp)
{
	const int fromLevel  = levelUp.level;
	const int fromAmount = GetCurrencyAmount(entityManager, playerID);

	for (const AttributeRow& row : GetRows(levelUp))
	{
		for (int i = 0; i < *row.pendingPoints; ++i)
			RaiseOnePoint(entityManager, playerID, levelUp, row.attribute);
	}

	ForgetPendingPoints(levelUp);

	if (levelUp.level > fromLevel)
		Celebrate(entityManager, playerID, levelUp, fromLevel, fromAmount - GetCurrencyAmount(entityManager, playerID));
}

void HandleMenuKeys(EntityManager& entityManager, unsigned int playerID, LevelUpComponent& levelUp)
{
	const std::vector<AttributeRow> rows = GetRows(levelUp);
	if (rows.empty())
		return;

	const int rowCount = (int)rows.size();

	if (IsAnyKeyDown(UpKeys))
		levelUp.selectedAttribute = (levelUp.selectedAttribute + rowCount - 1) % rowCount;
	if (IsAnyKeyDown(DownKeys))
		levelUp.selectedAttribute = (levelUp.selectedAttribute + 1) % rowCount;

	levelUp.selectedAttribute = glm::clamp(levelUp.selectedAttribute, 0, rowCount - 1);
	int& pending = *rows[levelUp.selectedAttribute].pendingPoints;

	if (IsAnyKeyDown(AddKeys) && CanAddPoint(entityManager, playerID, levelUp))
		pending += 1;
	if (IsAnyKeyDown(RemoveKeys) && pending > 0)
		pending -= 1;

	if (IsAnyKeyDown(ConfirmKeys) && levelUp.GetPendingPoints() > 0)
		Confirm(entityManager, playerID, levelUp);
}

// ========== Drawing ==========

// "->" drawn with bars (the font has no arrow)
void DrawArrow(const glm::vec2& center, const glm::vec4& color)
{
	const float length = ArrowShaftLength + ArrowHeadColumns;
	const float left   = center.x - length * 0.5f;

	UIRenderer::DrawRectangle(glm::vec2(left, center.y - ArrowThickness * 0.5f), glm::vec2(ArrowShaftLength, ArrowThickness), color);

	for (int i = 0; i < ArrowHeadColumns; ++i)
	{
		const float height = (ArrowHeadColumns - i) * 2.0f;
		UIRenderer::DrawRectangle(glm::vec2(left + ArrowShaftLength + i, center.y - height * 0.5f), glm::vec2(1.0f, height), color);
	}
}

glm::vec4 GetChangeColor(float now, float next)
{
	if (next > now) return RaisedValueColor;
	if (next < now) return LoweredValueColor;
	return TextColor;
}

void DrawTextRightAligned(const std::string& text, float right, float top, float height, const glm::vec4& color)
{
	UIRenderer::DrawTextAt(text, glm::vec2(right - UIRenderer::MeasureText(text, height), top), height, color);
}

// A row as in Dark Souls: the name on the left, then the value now, an arrow and the value after the level up
void DrawChangeRow(const std::string& label, const std::string& now, const std::string& next, const glm::vec4& nextColor,
	float top, float height, float left, float right, const glm::vec4& labelColor)
{
	UIRenderer::DrawTextAt(label, glm::vec2(left, top), height, labelColor);
	DrawTextRightAligned(now, right - NowColumnFromRight, top, height, TextColor);
	DrawArrow(glm::vec2(right - ArrowColumnFromRight, top + height * 0.55f), ArrowColor);
	DrawTextRightAligned(next, right, top, height, nextColor);
}

void DrawPanel(const glm::vec2& topLeft, const glm::vec2& size)
{
	UIRenderer::DrawRectangle(topLeft, size, MenuBackground);
	UIRenderer::DrawRectangleOutline(topLeft, size, LineThickness * 2.0f, MenuBorder);
}

void DrawSeparator(float left, float right, float top)
{
	UIRenderer::DrawRectangle(glm::vec2(left, top), glm::vec2(right - left, LineThickness), MenuBorder);
}

std::string ToText(float value)
{
	return std::to_string((int)glm::round(value));
}

// The damage of the character's weapon (a child with a Hitbox, e.g. the sword in the hand). False without one
bool TryGetWeaponDamage(EntityManager& entityManager, unsigned int entityID, float& outDamage)
{
	for (unsigned int childID : EntityHierarchy::GetChildren(entityManager, entityID))
	{
		if (const HitboxComponent* hitbox = entityManager.GetComponent<HitboxComponent>(childID))
		{
			outDamage = hitbox->damage;
			return true;
		}
	}
	return false;
}

// Both panels are as tall as the attributes need
float GetMenuHeight(size_t rowCount)
{
	return MenuPadding * 2.0f + TitleHeight + SectionGap + TextHeight * 2.0f + SectionGap * 2.0f
		+ rowCount * RowHeight + SectionGap + HintLineHeight * 2.0f;
}

// The level, the Currency and the attributes, each "now -> after the level up"
void DrawAttributesPanel(EntityManager& entityManager, unsigned int playerID, LevelUpComponent& levelUp, const glm::vec2& topLeft)
{
	const CurrencyComponent* currency = entityManager.GetComponent<CurrencyComponent>(playerID);
	const std::vector<AttributeRow> rows = GetRows(levelUp);

	const int  held         = currency ? currency->amount : 0;
	const int  pendingCost  = levelUp.GetPendingCost();
	const int  nextLevel    = levelUp.level + levelUp.GetPendingPoints();
	const int  required     = levelUp.GetCostAt(nextLevel);
	const bool canAffordOne = CanAddPoint(entityManager, playerID, levelUp);

	const float height = GetMenuHeight(rows.size());
	const float left  = topLeft.x + MenuPadding;
	const float right = topLeft.x + AttributesWidth - MenuPadding;

	DrawPanel(topLeft, glm::vec2(AttributesWidth, height));

	float top = topLeft.y + MenuPadding;

	if (levelUp.raisedGlowSecondsLeft > 0.0f)
	{
		const float glow = levelUp.raisedGlowSecondsLeft / RaisedGlowSeconds;
		UIRenderer::DrawRectangle(glm::vec2(left - MenuPadding * 0.5f, top - 4.0f), glm::vec2(right - left + MenuPadding, TitleHeight + 8.0f),
			glm::vec4(glm::vec3(GlowColor), GlowColor.a * glow));
	}

	DrawChangeRow(levelUp.displayName, std::to_string(levelUp.level), std::to_string(nextLevel),
		nextLevel > levelUp.level ? RaisedValueColor : TitleColor, top, TitleHeight, left, right, TitleColor);
	top += TitleHeight + SectionGap;

	const std::string currencyName = currency ? currency->displayName : "Currency";
	DrawChangeRow(currencyName + " held", std::to_string(held), std::to_string(held - pendingCost),
		GetChangeColor((float)held, (float)(held - pendingCost)), top, TextHeight, left, right, LabelColor);
	top += TextHeight;

	UIRenderer::DrawTextAt(currencyName + " required", glm::vec2(left, top), TextHeight, LabelColor);
	DrawTextRightAligned(std::to_string(required), right, top, TextHeight, canAffordOne ? TextColor : TooExpensiveColor);
	top += TextHeight + SectionGap;

	DrawSeparator(left, right, top);
	top += SectionGap;

	for (int i = 0; i < (int)rows.size(); ++i)
	{
		const AttributeRow& row = rows[i];
		const bool isSelected = i == levelUp.selectedAttribute;

		if (isSelected)
			UIRenderer::DrawRectangle(glm::vec2(left - MenuPadding * 0.5f, top), glm::vec2(right - left + MenuPadding, RowHeight), SelectedRowColor);

		const int now  = levelUp.startingAttributeValue + *row.points;
		const int next = now + *row.pendingPoints;
		DrawChangeRow(*row.name, std::to_string(now), std::to_string(next), GetChangeColor((float)now, (float)next),
			top + (RowHeight - TextHeight) * 0.5f, TextHeight, left, right, isSelected ? TextColor : LabelColor);

		top += RowHeight;
	}
	top += SectionGap;

	UIRenderer::DrawTextAt(ChooseHint, glm::vec2(left, top), HintHeight, LabelColor);
	UIRenderer::DrawTextAt(ConfirmHint, glm::vec2(left, top + HintLineHeight), HintHeight, LabelColor);
}

// What the attributes give, now and after the level up (the Base Power and Attack of Dark Souls)
struct StatRow
{
	std::string label;
	float       now;
	float       next;
};

std::vector<StatRow> GetStats(EntityManager& entityManager, unsigned int playerID, const LevelUpComponent& levelUp)
{
	std::vector<StatRow> stats;

	const HealthComponent* health = entityManager.GetComponent<HealthComponent>(playerID);
	if (health && !levelUp.healthAttribute.empty())
		stats.push_back({ "HP", health->maxHealth, health->maxHealth + levelUp.pendingHealthPoints * levelUp.healthPerPoint });

	const StaminaComponent* stamina = entityManager.GetComponent<StaminaComponent>(playerID);
	if (stamina && !levelUp.staminaAttribute.empty())
		stats.push_back({ "Stamina", stamina->maxStamina, stamina->maxStamina + levelUp.pendingStaminaPoints * levelUp.staminaPerPoint });

	if (!levelUp.damageAttribute.empty())
	{
		const float nowMultiplier  = levelUp.GetDamageMultiplier();
		const float nextMultiplier = nowMultiplier + levelUp.pendingDamagePoints * levelUp.damagePerPoint;

		float weaponDamage = 0.0f;
		if (TryGetWeaponDamage(entityManager, playerID, weaponDamage))
			stats.push_back({ "Attack", weaponDamage * nowMultiplier, weaponDamage * nextMultiplier });
		else
			stats.push_back({ "Damage bonus %", (nowMultiplier - 1.0f) * 100.0f, (nextMultiplier - 1.0f) * 100.0f });
	}

	return stats;
}

void DrawStatsPanel(EntityManager& entityManager, unsigned int playerID, const LevelUpComponent& levelUp, const glm::vec2& topLeft)
{
	const std::vector<StatRow> stats = GetStats(entityManager, playerID, levelUp);

	const float height = MenuPadding * 2.0f + TitleHeight + SectionGap * 2.0f + stats.size() * RowHeight;
	const float left   = topLeft.x + MenuPadding;
	const float right  = topLeft.x + StatsWidth - MenuPadding;

	DrawPanel(topLeft, glm::vec2(StatsWidth, height));

	float top = topLeft.y + MenuPadding;
	UIRenderer::DrawTextAt(StatsTitle, glm::vec2(left, top), TitleHeight, TitleColor);
	top += TitleHeight + SectionGap;

	DrawSeparator(left, right, top);
	top += SectionGap;

	for (const StatRow& stat : stats)
	{
		DrawChangeRow(stat.label, ToText(stat.now), ToText(stat.next), GetChangeColor(stat.now, stat.next),
			top + (RowHeight - TextHeight) * 0.5f, TextHeight, left, right, LabelColor);
		top += RowHeight;
	}
}

}

void LevelUpSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (LevelUpComponent& levelUp : entityManager.GetAllComponents<LevelUpComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(levelUp);

		// Only once: the scene's values are the ones without the points
		if (!levelUp.isBaseRemembered)
		{
			const HealthComponent*  health  = entityManager.GetComponent<HealthComponent>(entityID);
			const StaminaComponent* stamina = entityManager.GetComponent<StaminaComponent>(entityID);

			levelUp.baseMaxHealth    = health  ? health->maxHealth   : 0.0f;
			levelUp.baseMaxStamina   = stamina ? stamina->maxStamina : 0.0f;
			levelUp.isBaseRemembered = true;
		}

		levelUp.selectedAttribute     = 0;
		levelUp.raisedGlowSecondsLeft = 0.0f;
		ForgetPendingPoints(levelUp);

		ApplyPoints(entityManager, entityID, levelUp);
		Audio::Preload(levelUp.levelUpSound);
	}
}

void LevelUpSystem::Update(EntityManager& entityManager, float deltaTime)
{
	unsigned int playerID = 0;
	const bool isMenuOpen = TryFindPlayerInMenu(entityManager, playerID);

	for (LevelUpComponent& levelUp : entityManager.GetAllComponents<LevelUpComponent>())
	{
		levelUp.raisedGlowSecondsLeft = glm::max(levelUp.raisedGlowSecondsLeft - deltaTime, 0.0f);

		// Leaving the bonfire forgets the points that were not confirmed
		if (!isMenuOpen || entityManager.GetEntityIDForComponent(levelUp) != playerID)
			ForgetPendingPoints(levelUp);
	}

	if (isMenuOpen)
		HandleMenuKeys(entityManager, playerID, *entityManager.GetComponent<LevelUpComponent>(playerID));
}

bool LevelUpSystem::Raise(EntityManager& entityManager, unsigned int entityID, LevelUpAttribute attribute)
{
	LevelUpComponent* levelUp = entityManager.GetComponent<LevelUpComponent>(entityID);
	if (!levelUp)
		return false;

	const int fromLevel = levelUp->level;
	const int cost      = levelUp->GetCost();
	if (!RaiseOnePoint(entityManager, entityID, *levelUp, attribute))
		return false;

	Celebrate(entityManager, entityID, *levelUp, fromLevel, cost);
	return true;
}

bool LevelUpSystem::TryParseAttribute(const std::string& word, LevelUpAttribute& outAttribute)
{
	if (word == "health")  { outAttribute = LevelUpAttribute::Health;  return true; }
	if (word == "stamina") { outAttribute = LevelUpAttribute::Stamina; return true; }
	if (word == "damage")  { outAttribute = LevelUpAttribute::Damage;  return true; }
	return false;
}

float LevelUpSystem::GetDamageMultiplier(EntityManager& entityManager, unsigned int entityID)
{
	const LevelUpComponent* levelUp = entityManager.GetComponent<LevelUpComponent>(entityID);
	return levelUp ? levelUp->GetDamageMultiplier() : 1.0f;
}

void LevelUpSystem::DrawMenu(EntityManager& entityManager, const HUDArea& area)
{
	unsigned int playerID = 0;
	if (!TryFindPlayerInMenu(entityManager, playerID))
		return;

	LevelUpComponent& levelUp = *entityManager.GetComponent<LevelUpComponent>(playerID);

	// The attributes on the left, the stats on the right at the same height: the character by the fire stays visible in the middle
	const float top = area.topLeft.y + glm::max((area.size.y - GetMenuHeight(GetRows(levelUp).size())) * 0.5f, 0.0f);
	const float statsLeft = glm::max(area.topLeft.x + area.size.x - MenuSideMargin - StatsWidth,
		area.topLeft.x + MenuSideMargin + AttributesWidth + PanelGap);

	DrawAttributesPanel(entityManager, playerID, levelUp, glm::vec2(area.topLeft.x + MenuSideMargin, top));
	DrawStatsPanel(entityManager, playerID, levelUp, glm::vec2(statsLeft, top));
}

}
