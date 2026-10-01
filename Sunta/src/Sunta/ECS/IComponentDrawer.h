#pragma once

namespace Sunta
{

class EntityManager;

class IComponentDrawer
{
public:
	virtual ~IComponentDrawer() = default;
	virtual void Draw(void* componentData, unsigned int entityID, EntityManager& entityManager) = 0;
};

}