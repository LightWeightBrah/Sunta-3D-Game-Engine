#pragma once

#include <array>

#include "CollisionLayers.h"

namespace Sunta
{

// Says which layers collide with which. The table is symmetric: if the Player collides with the Enemy, the Enemy collides with the Player
// "Collide" means: solid colliders push each other away, and when one of them is a trigger, the overlap is reported
// It only knows layer slots, which layers exist is the business of CollisionLayerSet (they are kept together by CollisionSettings)
class CollisionMatrix
{
public:
	// Nothing collides with anything
	CollisionMatrix() = default;

	bool CanCollide(CollisionLayer layerA, CollisionLayer layerB) const;
	void SetCanCollide(CollisionLayer layerA, CollisionLayer layerB, bool canCollide);

	// The layer collides with nothing any more, used when it is removed
	void ClearLayer(CollisionLayer layer);

	bool operator==(const CollisionMatrix& other) const;
	bool operator!=(const CollisionMatrix& other) const;

private:
	// Entry i = layers that collide with the layer in slot i
	std::array<CollisionMask, MaxCollisionLayers> collidingLayersPerSlot{};
};

}
