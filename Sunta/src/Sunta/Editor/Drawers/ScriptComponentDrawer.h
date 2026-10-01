#pragma once
#include "ECS/IComponentDrawer.h"

namespace Sunta
{

class ScriptComponentDrawer : public IComponentDrawer
{
public:
	void Draw(void* componentData, unsigned int entityID, EntityManager& entityManager) override;
};

}