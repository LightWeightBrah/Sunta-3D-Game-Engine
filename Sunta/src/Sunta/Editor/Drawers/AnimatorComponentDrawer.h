#pragma once
#include "ECS/IComponentDrawer.h"

namespace Sunta
{

class AnimatorComponentDrawer : public IComponentDrawer
{
public:
	bool Draw(void* componentData, unsigned int entityID, EntityManager& entityManager) override;
};

}