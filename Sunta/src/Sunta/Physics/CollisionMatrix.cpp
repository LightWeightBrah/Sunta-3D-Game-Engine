#include "Core/SuntaPreCompiled.h"
#include "CollisionMatrix.h"

namespace Sunta
{

bool CollisionMatrix::CanCollide(CollisionLayer layerA, CollisionLayer layerB) const
{
	unsigned int slotA = 0;

	return TryGetLayerSlot(layerA, slotA) && MaskContainsLayer(collidingLayersPerSlot[slotA], layerB);
}

void CollisionMatrix::SetCanCollide(CollisionLayer layerA, CollisionLayer layerB, bool canCollide)
{
	unsigned int slotA = 0;
	unsigned int slotB = 0;

	if (!TryGetLayerSlot(layerA, slotA) || !TryGetLayerSlot(layerB, slotB))
		return;

	if (canCollide)
	{
		collidingLayersPerSlot[slotA] |= ToMask(layerB);
		collidingLayersPerSlot[slotB] |= ToMask(layerA);
	}
	else
	{
		collidingLayersPerSlot[slotA] &= ~ToMask(layerB);
		collidingLayersPerSlot[slotB] &= ~ToMask(layerA);
	}
}

void CollisionMatrix::ClearLayer(CollisionLayer layer)
{
	unsigned int slot = 0;

	if (!TryGetLayerSlot(layer, slot))
		return;

	for (CollisionMask& collidingLayers : collidingLayersPerSlot)
		collidingLayers &= ~ToMask(layer);

	collidingLayersPerSlot[slot] = 0;
}

bool CollisionMatrix::operator==(const CollisionMatrix& other) const
{
	return collidingLayersPerSlot == other.collidingLayersPerSlot;
}

bool CollisionMatrix::operator!=(const CollisionMatrix& other) const
{
	return !(*this == other);
}

}
