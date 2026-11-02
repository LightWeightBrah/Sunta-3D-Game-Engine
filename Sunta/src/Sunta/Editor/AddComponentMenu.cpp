#include "Core/SuntaPreCompiled.h"
#include "AddComponentMenu.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <typeinfo>
#include <vector>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "Core/EngineAssets.h"
#include "Core/ResourceManager.h"
#include "ECS/Component.h"
#include "ECS/ComponentLayout.h"
#include "ECS/EntityManager.h"
#include "Renderer/Texture.h"
#include "Gameplay/BonfireSetup.h"
#include "Gameplay/CharacterActionsSystem.h"
#include "Gameplay/CharacterAnimationSystem.h"
#include "Gameplay/CharacterControllerSystem.h"
#include "Gameplay/EnemySystem.h"
#include "Gameplay/FollowCameraSystem.h"
#include "Gameplay/MeleeEnemy.h"
#include "Gameplay/RustedKnightSetup.h"
#include "Gameplay/SoulsPlayerSetup.h"

namespace Sunta
{

namespace
{

// ========== Look of the menu ==========

constexpr float MenuWidth  = 340.0f;
constexpr float MenuHeight = 440.0f;
constexpr float IconSize   = 18.0f;
constexpr float IconToTextSpacing = 6.0f;

// ========== The groups, in the order they are shown ==========

constexpr const char* Presets     = "Presets";
constexpr const char* Rendering   = "Rendering";
constexpr const char* Physics     = "Physics";
constexpr const char* Animation   = "Animation";
constexpr const char* Character   = "Character";
constexpr const char* Combat      = "Combat";
constexpr const char* Enemies     = "Enemies";
constexpr const char* Progression = "Progression";
constexpr const char* World       = "World";
constexpr const char* Audio       = "Audio";
constexpr const char* Scripting   = "Scripting";

const std::vector<const char*> Groups =
{
	Presets, Rendering, Physics, Animation, Character, Combat, Enemies, Progression, World, Audio, Scripting
};

// ========== The rows of the menu ==========

using EntityCheck  = std::function<bool(EntityManager&, unsigned int)>;
using EntityAction = std::function<void(EntityManager&, unsigned int)>;

struct MenuEntry
{
	const char*  name;
	const char*  group;
	const char*  description;
	size_t       typeHash;  // the icon is the one the component has in the Inspector (0 = 'iconKey')
	const char*  iconKey;
	EntityCheck  has;
	EntityAction add;
};

// A component added on its own
template<typename TComponent>
MenuEntry Plain(const char* name, const char* group, const char* description)
{
	return { name, group, description, typeid(TComponent).hash_code(), EngineAssets::Icons::DefaultFile,
		[](EntityManager& entityManager, unsigned int entityID) { return entityManager.GetComponent<TComponent>(entityID) != nullptr; },
		[](EntityManager& entityManager, unsigned int entityID) { entityManager.AddComponent<TComponent>(entityID); } };
}

// A component that brings the components it needs with it
template<typename TComponent>
MenuEntry WithRequired(const char* name, const char* group, const char* description, void (*addWithRequired)(EntityManager&, unsigned int))
{
	return { name, group, description, typeid(TComponent).hash_code(), EngineAssets::Icons::DefaultFile,
		[](EntityManager& entityManager, unsigned int entityID) { return entityManager.GetComponent<TComponent>(entityID) != nullptr; },
		addWithRequired };
}

// A ready set of many components (a whole player, an enemy, a bonfire)
MenuEntry Preset(const char* name, const char* description, const char* iconKey,
	bool (*isSetUp)(EntityManager&, unsigned int), void (*setUp)(EntityManager&, unsigned int))
{
	return { name, Presets, description, 0, iconKey, isSetUp, setUp };
}

const std::vector<MenuEntry>& GetEntries()
{
	static const std::vector<MenuEntry> entries =
	{
		Preset("Souls Player", "Everything a player needs: controller, rolls and attacks, health, stamina, HUD, camera, sword and shield",
			EngineAssets::Icons::SoulsPlayer, SoulsPlayerSetup::IsSetUp, SoulsPlayerSetup::SetUp),
		Preset("Rusted Knight", "A ready enemy: sword, shield, senses and a melee fighter's behaviour",
			EngineAssets::Icons::RustedKnight, RustedKnightSetup::IsSetUp, RustedKnightSetup::SetUp),
		Preset("Bonfire", "A place to rest: the model, a warm light and resting",
			EngineAssets::Icons::SoulsBonfire, BonfireSetup::IsSetUp, BonfireSetup::SetUp),

		Plain<MeshComponent>            ("Mesh",              Rendering, "A simple shape (cube, sphere, ...) with a material"),
		Plain<ModelComponent>           ("Model",             Rendering, "A 3D model loaded from a file"),
		Plain<DirectionalLightComponent>("Directional Light", Rendering, "Light from far away in one direction (sun, moon)"),
		Plain<PointLightComponent>      ("Point Light",       Rendering, "Light shining in every direction from a point (a torch)"),
		Plain<SpotLightComponent>       ("Spot Light",        Rendering, "A cone of light"),
		Plain<CameraComponent>          ("Camera",            Rendering, "What the game is seen from"),
		Plain<EnvironmentComponent>     ("Environment",       Rendering, "The sky and the fog of the whole scene"),
		Plain<ParticleEmitterComponent> ("Particle Emitter",  Rendering, "Specks of fire, sparks, mist, embers"),
		Plain<WeaponTrailComponent>     ("Weapon Trail",      Rendering, "A glowing streak behind a swung weapon"),

		Plain<BoxColliderComponent>     ("Box Collider",      Physics, "A box others collide with (or a trigger)"),
		Plain<PhysicsBodyComponent>     ("Physics Body",      Physics, "Gravity, speed and being pushed around"),

		Plain<AnimatorComponent>        ("Animator",          Animation, "Plays the animations of the Model"),
		WithRequired<CharacterAnimationComponent>("Character Animation", Animation, "Chooses idle, walk and run by how fast the character moves",
			CharacterAnimationSystem::AddWithRequiredComponents),

		WithRequired<CharacterControllerComponent>("Character Controller", Character, "Walking and running, by the keyboard or a script",
			CharacterControllerSystem::AddWithRequiredComponents),
		WithRequired<CharacterActionsComponent>("Character Actions", Character, "Rolls, attacks, block, parry",
			CharacterActionsSystem::AddWithRequiredComponents),
		Plain<HealthComponent>          ("Health",            Character, "How much damage it takes to die"),
		Plain<StaminaComponent>         ("Stamina",           Character, "Spent by rolls, attacks and blocking"),
		Plain<PoiseComponent>           ("Poise",             Character, "How many hits it takes before staggering"),
		Plain<LockOnComponent>          ("Lock On",           Character, "Locks the view on an enemy"),
		Plain<LockOnTargetComponent>    ("Lock On Target",    Character, "Can be locked on"),
		WithRequired<FollowCameraComponent>("Follow Camera",  Character, "A camera that follows the player",
			FollowCameraSystem::AddWithRequiredComponents),
		Plain<RespawnComponent>         ("Respawn",           Character, "Comes back at the last bonfire after dying"),
		Plain<PlayerHUDComponent>       ("Player HUD",        Character, "Health and stamina bars, messages, the death screen"),

		Plain<HitboxComponent>          ("Hitbox",            Combat, "The part of a weapon that hurts"),
		Plain<ShieldComponent>          ("Shield",            Combat, "Blocks hits from the front"),

		WithRequired<EnemyComponent>    ("Enemy",             Enemies, "Senses and a state machine of an enemy",
			EnemySystem::AddWithRequiredComponents),
		WithRequired<MeleeEnemyComponent>("Melee Enemy",      Enemies, "Chases and attacks with its weapon",
			MeleeEnemy::AddWithRequiredComponents),
		Plain<EnemyHealthBarComponent>  ("Enemy Health Bar",  Enemies, "A bar over the enemy, or the boss bar at the bottom"),
		Plain<BossComponent>            ("Boss",              Enemies, "Waits behind a fog gate, fights harder in phase two"),

		Plain<HealingFlaskComponent>    ("Healing Flask",     Progression, "Heals with a few drinks, filled again at every rest"),
		Plain<CurrencyComponent>        ("Currency",          Progression, "What the player collects (souls, runes), lost on death"),
		Plain<CurrencyRewardComponent>  ("Currency Reward",   Progression, "How much Currency defeating it gives"),
		Plain<LevelUpComponent>         ("Level Up",          Progression, "Spends the Currency on attributes while resting at a bonfire"),

		Plain<FogGateComponent>         ("Fog Gate",          World, "The fog in the doorway of a boss arena"),
		Plain<CutsceneComponent>        ("Cutscene",          World, "A camera flight through points, or a video"),

		Plain<AudioSourceComponent>     ("Audio Source",      Audio, "A sound of the entity: a fire crackling, wind, music"),
		Plain<CharacterSoundsComponent> ("Character Sounds",  Audio, "Swings, hits, blocks and footsteps of a character"),

		Plain<ScriptComponent>          ("Script",            Scripting, "Gameplay written in Lua"),
	};

	return entries;
}

// ========== Searching ==========

std::string searchText;

std::string ToLower(std::string text)
{
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return text;
}

bool Matches(const MenuEntry& entry, const std::string& lowerSearch)
{
	return ToLower(entry.name).find(lowerSearch) != std::string::npos
		|| ToLower(entry.group).find(lowerSearch) != std::string::npos;
}

// ========== Drawing ==========

const char* GetIconKey(const MenuEntry& entry)
{
	const ComponentType* type = entry.typeHash ? InspectorComponentRegistry::GetComponentTypeByHash(entry.typeHash) : nullptr;
	return type ? type->iconKey.c_str() : entry.iconKey;
}

void DrawIcon(const MenuEntry& entry)
{
	if (auto icon = ResourceManager::GetEditorIcon(GetIconKey(entry)))
		ImGui::Image((ImTextureID)(uintptr_t)icon->GetID(), ImVec2(IconSize, IconSize));
	else
		ImGui::Dummy(ImVec2(IconSize, IconSize));

	ImGui::SameLine(0.0f, IconToTextSpacing);
}

// One row: the icon and the name (with its group while searching). True when it was clicked and the component added
bool DrawEntry(EntityManager& entityManager, unsigned int entityID, const MenuEntry& entry, bool showsGroup)
{
	const bool isAdded = entry.has(entityManager, entityID);

	ImGui::PushID(entry.name);
	ImGui::BeginDisabled(isAdded);

	DrawIcon(entry);
	const bool isClicked = ImGui::Selectable(entry.name);

	if (showsGroup)
	{
		const float groupWidth = ImGui::CalcTextSize(entry.group).x;
		ImGui::SameLine(ImGui::GetContentRegionMax().x - groupWidth);
		ImGui::TextDisabled("%s", entry.group);
	}

	ImGui::EndDisabled();

	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_DelayShort))
		ImGui::SetTooltip("%s%s", entry.description, isAdded ? "\n(already added)" : "");

	ImGui::PopID();

	if (!isClicked)
		return false;

	entry.add(entityManager, entityID);
	return true;
}

bool DrawGroups(EntityManager& entityManager, unsigned int entityID)
{
	bool isAdded = false;

	for (const char* group : Groups)
	{
		ImGui::SeparatorText(group);

		for (const MenuEntry& entry : GetEntries())
		{
			if (entry.group == group)
				isAdded = DrawEntry(entityManager, entityID, entry, false) || isAdded;
		}
	}

	return isAdded;
}

bool DrawSearchResults(EntityManager& entityManager, unsigned int entityID, bool addsFirstMatch)
{
	const std::string lowerSearch = ToLower(searchText);
	bool hasMatch = false;
	bool isAdded  = false;

	for (const MenuEntry& entry : GetEntries())
	{
		if (!Matches(entry, lowerSearch))
			continue;

		// Enter adds the first match that the entity doesn't have yet
		if (addsFirstMatch && !isAdded && !entry.has(entityManager, entityID))
		{
			entry.add(entityManager, entityID);
			return true;
		}

		hasMatch = true;
		isAdded = DrawEntry(entityManager, entityID, entry, true) || isAdded;
	}

	if (!hasMatch)
		ImGui::TextDisabled("No component matches \"%s\"", searchText.c_str());

	return isAdded;
}

}

bool AddComponentMenu::Draw(EntityManager& entityManager, unsigned int entityID)
{
	if (ImGui::IsWindowAppearing())
	{
		searchText.clear();
		ImGui::SetKeyboardFocusHere();
	}

	ImGui::SetNextItemWidth(MenuWidth);
	const bool isEnterPressed = ImGui::InputTextWithHint("##ComponentSearch", "Search components...", &searchText, ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::BeginChild("##ComponentList", ImVec2(MenuWidth, MenuHeight));
	const bool isAdded = searchText.empty()
		? DrawGroups(entityManager, entityID)
		: DrawSearchResults(entityManager, entityID, isEnterPressed);
	ImGui::EndChild();

	return isAdded;
}

}
