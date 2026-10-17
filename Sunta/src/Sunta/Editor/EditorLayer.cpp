#include "Core/SuntaPreCompiled.h"
#include "EditorLayer.h"

#include "EditorGUI.h"
#include "EditorGUIContext.h"
#include "EditorSettings.h"
#include "ProcessLauncher.h"
#include "ProjectManager.h"
#include "Core/Application.h"
#include "Core/InputManager.h"
#include "Core/ResourceLoader.h"
#include "ECS/Component.h"
#include "ECS/ComponentLayout.h"
#include "Events/EventBus.h"
#include "Renderer/Renderer.h"
#include "Scene/Scene.h"

namespace Sunta
{

EditorLayer::EditorLayer(Application& application)
	: application(application)
{

}

void EditorLayer::OnAttach()
{
	ResourceLoader::InitEditorAssets();

	EditorGUIContext::Init(&application.GetWindow());
	EditorGUI::SetDarkTheme();

	RegisterComponentsToInspector();

	// Puts the project on top of the list in the project launcher
	EditorSettings::AddRecentProject(ProjectManager::GetCurrentProjectDirectory());
}

void EditorLayer::OnDetach()
{
	EditorGUIContext::Shutdown(&application.GetWindow());
}

void EditorLayer::OnProcessInput()
{
	if (InputManager::IsKeyDown(GLFW_KEY_ESCAPE))
		application.Close();

	if (InputManager::IsKeyDown(GLFW_KEY_TAB))
		ToggleEngineMode();
}

void EditorLayer::OnUpdate(float deltaTime)
{
	// "Project > Switch Project" in the menu: the launcher is a new process, so this editor can close
	if (ProjectManager::ConsumeSwitchProjectRequest() && ProcessLauncher::StartEditorLauncher())
		application.Close();
}

void EditorLayer::OnRender()
{
	Scene& scene = application.GetScene();

	EditorGUIContext::RenderUI(&application.GetWindow(), scene, Renderer::GetDevice());
	EditorGUI::HandleSelectionInteraction(scene);
}

void EditorLayer::ToggleEngineMode()
{
	currentEngineMode = (currentEngineMode == EngineMode::Game) ? EngineMode::Editor : EngineMode::Game;

	if (currentEngineMode == EngineMode::Game)
	{
		EditorGUI::ClearFocus();
		EditorGUI::ClearSelection();
		InputManager::SetFirstMouse(true);
	}

	EventBus::Publish<EngineModeChangedEvent>({ currentEngineMode });
}

void EditorLayer::RegisterComponentsToInspector()
{
	TagComponent::RegisterToInspector();
	TransformComponent::RegisterToInspector();
	ScriptComponent::RegisterToInspector();
	MeshComponent::RegisterToInspector();
	ModelComponent::RegisterToInspector();
	AnimatorComponent::RegisterToInspector();
	DirectionalLightComponent::RegisterToInspector();
	PointLightComponent::RegisterToInspector();
	SpotLightComponent::RegisterToInspector();
	BoxColliderComponent::RegisterToInspector();
	PhysicsBodyComponent::RegisterToInspector();
}

}
