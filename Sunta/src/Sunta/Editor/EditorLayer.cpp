#include "Core/SuntaPreCompiled.h"
#include "EditorLayer.h"

#include <imgui/imgui.h>

#include "CollisionSettingsWindow.h"
#include "EditorGUI.h"
#include "EditorGUIContext.h"
#include "EditorSettings.h"
#include "EditorShortcuts.h"
#include "ProcessLauncher.h"
#include "ProjectManager.h"
#include "ProjectSettingsWindow.h"
#include "SceneDocument.h"
#include "StartScene.h"
#include "UnsavedChangesDialog.h"
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

namespace
{

// The Debug editor is very slow, so the title tells which one is open (SUNTA_DEBUG is defined by CMake for Debug only)
#if defined(SUNTA_DEBUG)
constexpr const char* WindowTitleSuffix = "Sunta Editor (Debug)";
#else
constexpr const char* WindowTitleSuffix = "Sunta Editor (Release)";
#endif

// ImGui reads the input at the start of the frame, so this is the state of the last frame
bool IsAnyMouseButtonDown()
{
	for (int mouseButton = 0; mouseButton < ImGuiMouseButton_COUNT; ++mouseButton)
	{
		if (ImGui::IsMouseDown(mouseButton))
			return true;
	}

	return false;
}

}

EditorLayer::EditorLayer(Application& application)
	: application(application)
{

}

void EditorLayer::OnAttach()
{
	sceneChangedSubscriptionID = EventBus::Subscribe<SceneChangedEvent>([this](const SceneChangedEvent&)
	{
		windowTitleOutdated = true;
	});

	ResourceLoader::InitEditorAssets();

	EditorGUIContext::Init(&application.GetWindow());
	EditorGUI::SetDarkTheme();

	RegisterComponentsToInspector();

	// The free camera flies at the speed the user chose last time
	const float savedCameraSpeed = EditorSettings::GetCameraSpeed();
	if (savedCameraSpeed > 0.0f)
		application.GetScene().SetEditorCameraSpeed(savedCameraSpeed);

	application.GetScene().SetScriptHotReloadEnabled(true);

	// Play mode uses the collision rules of the project, even before the Collision Layers & Matrix window is opened
	CollisionSettingsWindow::LoadProjectSettings();

	StartScene::OpenInEditor(application.GetScene());

	// After the start scene is loaded (or created), so it starts as "no unsaved changes"
	SceneDocument::Init(application.GetScene());

	// Puts the project on top of the list in the project launcher
	EditorSettings::AddRecentProject(ProjectManager::GetCurrentProjectDirectory());

	projectName = ProjectManager::ReadProjectName(ProjectManager::GetCurrentProjectDirectory());
	UpdateWindowTitle();
}

void EditorLayer::OnDetach()
{
	ProjectSettingsWindow::WaitForBuildToFinish();

	EventBus::Unsubscribe(sceneChangedSubscriptionID);

	EditorSettings::SetCameraSpeed(application.GetScene().GetEditorCameraSpeed());

	EditorGUIContext::Shutdown(&application.GetWindow());
}

void EditorLayer::OnProcessInput()
{
	if (InputManager::IsKeyDown(GLFW_KEY_ESCAPE))
		RequestClose();

	if (InputManager::IsKeyDown(GLFW_KEY_TAB))
		ToggleEngineMode();
}

void EditorLayer::OnUpdate(float deltaTime)
{
	if (windowTitleOutdated && !IsAnyMouseButtonDown())
	{
		windowTitleOutdated = false;
		UpdateWindowTitle();
	}

	// "Project > Switch Project" in the menu: the launcher is a new process, so this editor can close
	if (ProjectManager::ConsumeSwitchProjectRequest())
	{
		UnsavedChangesDialog::RunAfterAsking([this]()
		{
			if (ProcessLauncher::StartEditorLauncher())
				application.Close();
		});
	}
}

void EditorLayer::OnRender()
{
	Scene& scene = application.GetScene();

	EditorGUIContext::RenderUI(&application.GetWindow(), scene, Renderer::GetDevice());
	EditorGUI::HandleSelectionInteraction(scene);

	// In Game mode the keys belong to the game (and the cursor is hidden), so the shortcuts are for the Editor mode only
	if (currentEngineMode == EngineMode::Editor)
		EditorShortcuts::Process(scene);
}

bool EditorLayer::OnCloseRequested()
{
	// Not closed here: the question about unsaved changes may be cancelled, so the editor closes itself after the answer
	RequestClose();
	return false;
}

void EditorLayer::RequestClose()
{
	// The question needs the mouse, and the Editor mode is the one with a free cursor
	if (currentEngineMode == EngineMode::Game)
		ToggleEngineMode();

	UnsavedChangesDialog::RunAfterAsking([this]() { application.Close(); });
}

std::string EditorLayer::BuildWindowTitle()
{
	std::string sceneLabel = titleSceneName;

	// While the game plays, physics and scripts change the scene, so there is nothing to compare
	if (titleShowsPlaying)
		sceneLabel += " (Playing)";
	else if (SceneDocument::HasUnsavedChanges())
		sceneLabel += "*";

	return sceneLabel + " - " + projectName + " - " + WindowTitleSuffix;
}

void EditorLayer::UpdateWindowTitle()
{
	titleSceneName    = SceneDocument::GetSceneName();
	titleShowsPlaying = application.GetScene().IsPlaying();

	const std::string newTitle = BuildWindowTitle();
	if (newTitle == windowTitle)
		return;

	windowTitle = newTitle;
	application.GetWindow().SetTitle(windowTitle);
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
	CameraComponent::RegisterToInspector();
	BoxColliderComponent::RegisterToInspector();
	PhysicsBodyComponent::RegisterToInspector();
}

}
