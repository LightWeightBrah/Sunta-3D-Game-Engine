#pragma once

#include <array>
#include <string>
#include <vector>

#include "CollisionLayers.h"

namespace Sunta
{

// game.config writes a collision rule as "Player + Enemy", so a layer name can't contain this character
constexpr char CollisionPairSeparator = '+';

// The layers a project has: MaxCollisionLayers slots, each one either free or holding the name of a layer
// A layer keeps its slot for its whole life, so renaming or removing other layers never changes it
// (that is what scene files rely on)
class CollisionLayerSet
{
public:
	enum class NameProblem
	{
		None,
		Empty,
		SpaceAtTheEdge,
		ContainsPairSeparator,
		AlreadyUsed
	};

	// A set without any layers
	CollisionLayerSet() = default;

	bool Contains(CollisionLayer layer) const;

	// "(missing layer)" when the layer is not in the set, e.g. a collider still points at a removed layer
	const std::string& GetName(CollisionLayer layer) const;

	bool TryFindByName(const std::string& name, CollisionLayer& outLayer) const;

	// In slot order, which is the order the editor shows them
	std::vector<CollisionLayer> GetLayers() const;

	bool IsFull() const;

	// Says whether the name can be used. ignoredLayer is the layer being renamed, it may keep its own name
	NameProblem CheckName(const std::string& name, CollisionLayer ignoredLayer = CollisionLayer::None) const;

	// The new layer takes the first free slot. Returns false when the set is full or the name has a problem
	bool TryAdd(const std::string& name);

	// Puts the layer in a given slot, used when the set is read from a file. Returns false when the slot is taken or the name has a problem
	bool TryAddAt(CollisionLayer layer, const std::string& name);

	bool TryRename(CollisionLayer layer, const std::string& newName);

	// Everything except the default layer
	bool CanRemove(CollisionLayer layer) const;
	void Remove(CollisionLayer layer);

	bool operator==(const CollisionLayerSet& other) const;
	bool operator!=(const CollisionLayerSet& other) const;

private:
	bool TryGetUsedSlot(CollisionLayer layer, unsigned int& outSlot) const;
	bool TryFindFreeSlot(unsigned int& outSlot) const;

	// Entry i = name of the layer in slot i, an empty name = a free slot
	std::array<std::string, MaxCollisionLayers> namesPerSlot;
};

}
