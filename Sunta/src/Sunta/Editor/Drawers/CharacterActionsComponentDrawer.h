#pragma once
#include "ECS/IComponentDrawer.h"

namespace Sunta
{

// Shows a dropdown of the animations of the Model for every action and sliders for its timing
class CharacterActionsComponentDrawer : public IComponentDrawer
{
public:
	bool Draw(void* componentData, unsigned int entityID, EntityManager& entityManager) override;
};

}
