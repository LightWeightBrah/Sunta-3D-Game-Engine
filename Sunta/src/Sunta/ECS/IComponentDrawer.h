#pragma once

namespace Sunta
{

class IComponentDrawer
{
public:
	virtual ~IComponentDrawer() = default;
	virtual void Draw(void* componentData, unsigned int entityID) = 0;
};

}