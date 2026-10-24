#pragma once

#include <string>

#include "CollisionLayerSet.h"
#include "CollisionMatrix.h"

namespace Sunta
{

// The collision rules of one project: which layers exist and which of them collide with which
// ONE set for the whole game (edited in the editor: Project > Collision Layers & Matrix..., stored in game.config),
// a collider only chooses its own layer
// The layers and the matrix are changed only through this class, so a removed layer never keeps its rules
class CollisionSettings
{
public:
	// Starts with the default layers and rules (see ResetToDefaults)
	CollisionSettings();

	// Starts with the given layers and nothing colliding, used when the settings are read from a file
	explicit CollisionSettings(const CollisionLayerSet& initialLayers);

	const CollisionLayerSet& GetLayers() const;

	bool CanCollide(CollisionLayer layerA, CollisionLayer layerB) const;

	// Ignored when one of the layers is not in the project
	void SetCanCollide(CollisionLayer layerA, CollisionLayer layerB, bool canCollide);

	bool TryAddLayer(const std::string& name);
	bool TryRenameLayer(CollisionLayer layer, const std::string& newName);

	// A removed layer loses all its rules. Colliders that still use it collide with nothing until they get another layer
	void RemoveLayer(CollisionLayer layer);

	// Custom layers are removed, the default ones and their rules come back
	void ResetToDefaults();

	bool operator==(const CollisionSettings& other) const;
	bool operator!=(const CollisionSettings& other) const;

	// The rules the running game (and Play mode of the editor) uses
	static const CollisionSettings& GetActive();
	static void SetActive(const CollisionSettings& settings);

private:
	CollisionLayerSet layers;
	CollisionMatrix   matrix;
};

}
