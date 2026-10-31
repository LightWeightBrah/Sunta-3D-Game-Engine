#pragma once
#include "ECS/IComponentDrawer.h"

namespace Sunta
{

// Shows which entity is the parent and a dropdown of the bones of its Model to follow one of them
class ParentComponentDrawer : public IComponentDrawer
{
public:
	bool Draw(void* componentData, unsigned int entityID, EntityManager& entityManager) override;
};

}
