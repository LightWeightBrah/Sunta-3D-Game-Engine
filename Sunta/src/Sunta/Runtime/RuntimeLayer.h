#pragma once
#include "Sunta/Core/Layer.h"

namespace Sunta
{

class Application;

// What the shipped game does: loads the start scene from game.config and plays it. No editor
class RuntimeLayer : public Layer
{
public:
	explicit RuntimeLayer(Application& application);

	void OnAttach() override;
	void OnProcessInput() override;

private:
	Application& application;

	void LoadStartScene();
};

}
