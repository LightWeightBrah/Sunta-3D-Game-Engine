#pragma once

namespace Sunta
{

enum class CollisionLayer : unsigned int
{
	None = 0,

	Environment  = 1u << 0, // static geometry: walls, floors, ceilings etc.
	Player       = 1u << 1, // physical player body that blocks movement
	Enemy        = 1u << 2, // physical enemy  body that blocks movement

	PlayerHitbox = 1u << 3, // this DEALS damage e.g Player's sword 
	EnemyHitbox  = 1u << 4, // this DEALS damage e.g Enemy's  sword 

	Trigger      = 1u << 5  // general triggers (checkpoints, traps etc.)
};

using CollisionMask = unsigned int;

constexpr CollisionMask ToMask(CollisionLayer layer)
{
	return static_cast<CollisionMask>(layer);
}

// sums a lot of Layers in 1 MASK e.g MakeMask(CollisionLayer::Player, CollisionLayer::Enemy)
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

// True if Mask of ColliderA is inside Mask of ColliderB and vice versa
// Collision/trigger between 2 objects works ONLY IF AT LEAST 1 collider Layer 
// wants to react on another collider Layer
constexpr bool LayersCanInteract(CollisionLayer layerA, CollisionMask maskA, 
	CollisionLayer layerB, CollisionMask maskB)
{
	return MaskContainsLayer(maskB, layerA) || MaskContainsLayer(maskA, layerB);
}

}