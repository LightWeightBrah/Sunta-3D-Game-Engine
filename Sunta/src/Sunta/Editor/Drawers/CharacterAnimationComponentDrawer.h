#pragma once
#include "ECS/IComponentDrawer.h"

namespace Sunta
{

// Shows a dropdown of the animations of the Model for each state of the character
class CharacterAnimationComponentDrawer : public IComponentDrawer
{
public:
	bool Draw(void* componentData, unsigned int entityID, EntityManager& entityManager) override;
};

}
