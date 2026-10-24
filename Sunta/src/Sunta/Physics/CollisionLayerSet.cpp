#include "Core/SuntaPreCompiled.h"
#include "CollisionLayerSet.h"

namespace Sunta
{

bool CollisionLayerSet::Contains(CollisionLayer layer) const
{
	unsigned int slot = 0;

	return TryGetUsedSlot(layer, slot);
}

const std::string& CollisionLayerSet::GetName(CollisionLayer layer) const
{
	static const std::string MissingLayerName = "(missing layer)";

	unsigned int slot = 0;

	return TryGetUsedSlot(layer, slot) ? namesPerSlot[slot] : MissingLayerName;
}

bool CollisionLayerSet::TryFindByName(const std::string& name, CollisionLayer& outLayer) const
{
	for (unsigned int slot = 0; slot < MaxCollisionLayers; slot++)
	{
		if (!namesPerSlot[slot].empty() && namesPerSlot[slot] == name)
		{
			outLayer = LayerInSlot(slot);
			return true;
		}
	}

	return false;
}

std::vector<CollisionLayer> CollisionLayerSet::GetLayers() const
{
	std::vector<CollisionLayer> layers;

	for (unsigned int slot = 0; slot < MaxCollisionLayers; slot++)
	{
		if (!namesPerSlot[slot].empty())
			layers.push_back(LayerInSlot(slot));
	}

	return layers;
}

bool CollisionLayerSet::IsFull() const
{
	unsigned int freeSlot = 0;

	return !TryFindFreeSlot(freeSlot);
}

CollisionLayerSet::NameProblem CollisionLayerSet::CheckName(const std::string& name, CollisionLayer ignoredLayer) const
{
	if (name.empty())
		return NameProblem::Empty;

	if (name.front() == ' ' || name.back() == ' ')
		return NameProblem::SpaceAtTheEdge;

	if (name.find(CollisionPairSeparator) != std::string::npos)
		return NameProblem::ContainsPairSeparator;

	CollisionLayer layerWithThisName = CollisionLayer::None;

	if (TryFindByName(name, layerWithThisName) && layerWithThisName != ignoredLayer)
		return NameProblem::AlreadyUsed;

	return NameProblem::None;
}

bool CollisionLayerSet::TryAdd(const std::string& name)
{
	unsigned int freeSlot = 0;

	return TryFindFreeSlot(freeSlot) && TryAddAt(LayerInSlot(freeSlot), name);
}

bool CollisionLayerSet::TryAddAt(CollisionLayer layer, const std::string& name)
{
	unsigned int slot = 0;

	if (!TryGetLayerSlot(layer, slot) || !namesPerSlot[slot].empty())
		return false;

	if (CheckName(name) != NameProblem::None)
		return false;

	namesPerSlot[slot] = name;
	return true;
}

bool CollisionLayerSet::TryRename(CollisionLayer layer, const std::string& newName)
{
	unsigned int slot = 0;

	if (!TryGetUsedSlot(layer, slot))
		return false;

	if (CheckName(newName, layer) != NameProblem::None)
		return false;

	namesPerSlot[slot] = newName;
	return true;
}

bool CollisionLayerSet::CanRemove(CollisionLayer layer) const
{
	return Contains(layer) && layer != DefaultCollisionLayer;
}

void CollisionLayerSet::Remove(CollisionLayer layer)
{
	unsigned int slot = 0;

	if (CanRemove(layer) && TryGetUsedSlot(layer, slot))
		namesPerSlot[slot].clear();
}

bool CollisionLayerSet::operator==(const CollisionLayerSet& other) const
{
	return namesPerSlot == other.namesPerSlot;
}

bool CollisionLayerSet::operator!=(const CollisionLayerSet& other) const
{
	return !(*this == other);
}

bool CollisionLayerSet::TryGetUsedSlot(CollisionLayer layer, unsigned int& outSlot) const
{
	return TryGetLayerSlot(layer, outSlot) && !namesPerSlot[outSlot].empty();
}

bool CollisionLayerSet::TryFindFreeSlot(unsigned int& outSlot) const
{
	for (unsigned int slot = 0; slot < MaxCollisionLayers; slot++)
	{
		if (namesPerSlot[slot].empty())
		{
			outSlot = slot;
			return true;
		}
	}

	return false;
}

}
