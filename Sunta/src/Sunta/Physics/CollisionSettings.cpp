#include "Core/SuntaPreCompiled.h"
#include "CollisionSettings.h"

namespace Sunta
{

namespace
{

namespace DefaultLayerName
{
	constexpr const char* Environment  = "Environment";  // static geometry: walls, floors, ceilings etc.
	constexpr const char* Player       = "Player";       // physical player body that blocks movement
	constexpr const char* Enemy        = "Enemy";        // physical enemy body that blocks movement
	constexpr const char* PlayerHitbox = "PlayerHitbox"; // this DEALS damage e.g. the Player's sword
	constexpr const char* EnemyHitbox  = "EnemyHitbox";  // this DEALS damage e.g. the Enemy's sword
	constexpr const char* Trigger      = "Trigger";      // general triggers (checkpoints, traps etc.)
}

struct DefaultRule
{
	const char* firstLayerName;
	const char* secondLayerName;
};

// The first layer is the default one (slot 0), so a new collider starts as part of the environment
constexpr std::array DefaultLayerNames =
{
	DefaultLayerName::Environment,
	DefaultLayerName::Player,
	DefaultLayerName::Enemy,
	DefaultLayerName::PlayerHitbox,
	DefaultLayerName::EnemyHitbox,
	DefaultLayerName::Trigger
};

// Physical bodies (Environment, Player, Enemy) collide with each other
// Hitboxes and triggers only see the bodies they are meant for
constexpr std::array DefaultRules =
{
	DefaultRule{ DefaultLayerName::Environment,  DefaultLayerName::Environment },
	DefaultRule{ DefaultLayerName::Environment,  DefaultLayerName::Player      },
	DefaultRule{ DefaultLayerName::Environment,  DefaultLayerName::Enemy       },
	DefaultRule{ DefaultLayerName::Player,       DefaultLayerName::Player      },
	DefaultRule{ DefaultLayerName::Player,       DefaultLayerName::Enemy       },
	DefaultRule{ DefaultLayerName::Enemy,        DefaultLayerName::Enemy       },

	DefaultRule{ DefaultLayerName::PlayerHitbox, DefaultLayerName::Enemy       },
	DefaultRule{ DefaultLayerName::EnemyHitbox,  DefaultLayerName::Player      },

	DefaultRule{ DefaultLayerName::Trigger,      DefaultLayerName::Player      },
	DefaultRule{ DefaultLayerName::Trigger,      DefaultLayerName::Enemy       }
};

CollisionLayerSet CreateDefaultLayers()
{
	CollisionLayerSet layers;

	for (const char* layerName : DefaultLayerNames)
		layers.TryAdd(layerName);

	return layers;
}

void AddDefaultRules(CollisionSettings& settings)
{
	for (const DefaultRule& rule : DefaultRules)
	{
		CollisionLayer firstLayer  = CollisionLayer::None;
		CollisionLayer secondLayer = CollisionLayer::None;

		settings.GetLayers().TryFindByName(rule.firstLayerName,  firstLayer);
		settings.GetLayers().TryFindByName(rule.secondLayerName, secondLayer);

		settings.SetCanCollide(firstLayer, secondLayer, true);
	}
}

CollisionSettings& GetActiveSettingsStorage()
{
	static CollisionSettings activeSettings;
	return activeSettings;
}

}

CollisionSettings::CollisionSettings()
{
	ResetToDefaults();
}

CollisionSettings::CollisionSettings(const CollisionLayerSet& initialLayers)
	: layers(initialLayers)
{
}

const CollisionLayerSet& CollisionSettings::GetLayers() const
{
	return layers;
}

bool CollisionSettings::CanCollide(CollisionLayer layerA, CollisionLayer layerB) const
{
	return matrix.CanCollide(layerA, layerB);
}

void CollisionSettings::SetCanCollide(CollisionLayer layerA, CollisionLayer layerB, bool canCollide)
{
	if (layers.Contains(layerA) && layers.Contains(layerB))
		matrix.SetCanCollide(layerA, layerB, canCollide);
}

bool CollisionSettings::TryAddLayer(const std::string& name)
{
	return layers.TryAdd(name);
}

bool CollisionSettings::TryRenameLayer(CollisionLayer layer, const std::string& newName)
{
	return layers.TryRename(layer, newName);
}

void CollisionSettings::RemoveLayer(CollisionLayer layer)
{
	if (!layers.CanRemove(layer))
		return;

	layers.Remove(layer);
	matrix.ClearLayer(layer);
}

void CollisionSettings::ResetToDefaults()
{
	layers = CreateDefaultLayers();
	matrix = CollisionMatrix();

	AddDefaultRules(*this);
}

bool CollisionSettings::operator==(const CollisionSettings& other) const
{
	return layers == other.layers && matrix == other.matrix;
}

bool CollisionSettings::operator!=(const CollisionSettings& other) const
{
	return !(*this == other);
}

const CollisionSettings& CollisionSettings::GetActive()
{
	return GetActiveSettingsStorage();
}

void CollisionSettings::SetActive(const CollisionSettings& settings)
{
	GetActiveSettingsStorage() = settings;
}

}
