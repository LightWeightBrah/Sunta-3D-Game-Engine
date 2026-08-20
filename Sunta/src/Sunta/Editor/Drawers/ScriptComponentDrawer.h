#pragma once

namespace Sunta
{

class IComponentDrawer;

class ScriptComponentDrawer : public IComponentDrawer
{
public:
	void Draw(void* componentData, unsigned int entityID) override;
};

}