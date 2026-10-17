#pragma once

namespace Sunta
{

// A separate part of the application with its own hooks in the main loop
// e.g. EditorLayer draws the editor UI, RuntimeLayer starts the game
// Application calls the hooks of every layer in the order the layers were added
class Layer
{
public:
	virtual ~Layer() = default;

	virtual void OnAttach()                {}
	virtual void OnDetach()                {}
	virtual void OnProcessInput()          {}
	virtual void OnUpdate(float deltaTime) {}
	virtual void OnRender()                {}
};

}
