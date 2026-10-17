#pragma once

namespace Sunta
{

class EntityManager;

class IComponentDrawer
{
public:
	virtual ~IComponentDrawer() = default;

	// True when the user edited the component in this frame
	virtual bool Draw(void* componentData, unsigned int entityID, EntityManager& entityManager) = 0;
};

}