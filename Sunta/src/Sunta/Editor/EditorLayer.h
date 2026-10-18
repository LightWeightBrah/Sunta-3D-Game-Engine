#pragma once
#include <string>
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
	void OnUpdate(float deltaTime) override;
	void OnRender() override;
	bool OnCloseRequested() override;

private:
	Application& application;

	EngineMode currentEngineMode = EngineMode::Game;

	// What the window title was built from, to find out when it needs to be built again
	std::string projectName;
	std::string windowTitle;
	std::string titleSceneName;
	bool        titleShowsPlaying = false;

	// Set by SceneChangedEvent, cleared when the title is built again
	unsigned int sceneChangedSubscriptionID = 0;
	bool         windowTitleOutdated        = false;

	void RegisterComponentsToInspector();
	void ToggleEngineMode();

	// "Scene* - Project - Sunta Editor": the star means unsaved changes, "(Playing)" replaces it while the game plays
	// Looking for unsaved changes compares the whole scene, so the title is built again only after a SceneChangedEvent
	// (see SceneDocument::NotifyChanged), and not while a mouse button is held: a drag (slider, gizmo) sends the event
	// every frame, and the comparison can wait until the drag ends
	std::string BuildWindowTitle();
	void UpdateWindowTitle();

	// Closes the editor, after asking about unsaved changes of the scene
	void RequestClose();
};

}
