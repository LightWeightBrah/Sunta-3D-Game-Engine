#pragma once

#include <limits>

namespace Sunta
{

using CollisionMask = unsigned int;

// A layer is ONE bit of a CollisionMask: the layer in slot 0 is bit 0, the layer in slot 1 is bit 1 and so on
// (scene files store this value). Which slots are used and what the layers are called is decided by the project
// (CollisionLayerSet, edited in the editor), so the engine code never refers to a layer by name
enum class CollisionLayer : CollisionMask
{
	None = 0
};

constexpr unsigned int MaxCollisionLayers = std::numeric_limits<CollisionMask>::digits;

constexpr CollisionLayer LayerInSlot(unsigned int slot)
{
	return static_cast<CollisionLayer>(CollisionMask(1) << slot);
}

// Every new collider starts on it, a scene file without a layer gets it, and it can't be removed from the project
constexpr CollisionLayer DefaultCollisionLayer = LayerInSlot(0);

constexpr CollisionMask ToMask(CollisionLayer layer)
{
	return static_cast<CollisionMask>(layer);
}

// False for None and for a value with more than one bit, because that is not a single layer
constexpr bool TryGetLayerSlot(CollisionLayer layer, unsigned int& outSlot)
{
	for (unsigned int slot = 0; slot < MaxCollisionLayers; slot++)
	{
		if (layer == LayerInSlot(slot))
		{
			outSlot = slot;
			return true;
		}
	}

	return false;
}

// Sums a lot of layers in 1 mask, e.g. for a raycast: MakeMask(playerLayer, enemyLayer)
template<typename... Layers>
constexpr CollisionMask MakeMask(Layers... layers)
{
	return (ToMask(layers) | ...);
}

// True if layer is inside mask
constexpr bool MaskContainsLayer(CollisionMask mask, CollisionLayer layer)
{
	return (mask & ToMask(layer)) != 0;
}

// The names of the layers are in CollisionLayerSet.h, which layers collide with which is decided by CollisionSettings.h

}
