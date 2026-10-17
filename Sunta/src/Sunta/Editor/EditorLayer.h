#pragma once
#include "Sunta/Core/Layer.h"
#include "Sunta/Events/EventTypes.h"

namespace Sunta
{

class Application;

// The whole editor: ImGui UI, inspector registration and the Editor/Game mode switch (TAB)
class EditorLayer : public Layer
{
public:
	explicit EditorLayer(Application& application);

	void OnAttach() override;
	void OnDetach() override;
	void OnProcessInput() override;
	void OnRender() override;

private:
	Application& application;

	EngineMode currentEngineMode = EngineMode::Game;

	void RegisterComponentsToInspector();
	void ToggleEngineMode();
};

}
