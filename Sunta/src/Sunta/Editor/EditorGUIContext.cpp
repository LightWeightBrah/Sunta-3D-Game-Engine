#include "Core/SuntaPreCompiled.h"
#include "EditorGUIContext.h"

#include <imgui/imgui.h>
#include <imgui_internal.h>

#include "Core/Window.h"
#include "EditorGUIBackend.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"
#include "EditorGUI.h"

namespace Sunta
{

std::unique_ptr<EditorGUIBackend> EditorGUIContext::backend;
unsigned int EditorGUIContext::engineModeChangeID;

EditorGUIContext::~EditorGUIContext() = default;

void EditorGUIContext::Init(Window* window)
{
	if (!window)
		return;
	
	backend = window->CreateGUIBackend();

	if (!backend)
		return;

	backend->Init(window->GetNativeWindow());
	window->SetAsGraphicsTarget();

	engineModeChangeID = EventBus::Subscribe<EngineModeChangedEvent>(EditorGUIContext::OnEngineModeChanged);
}

void EditorGUIContext::Shutdown(Window* window)
{
	if (!backend || !window)
		return;

	EventBus::Unsubsribe(engineModeChangeID);
	backend->Shutdown(window->GetNativeWindow());
	backend.reset();
}

void EditorGUIContext::NewFrame(Window* window)
{
	if (!backend || !window)
		return;

	backend->NewFrame(window->GetNativeWindow());
}

void EditorGUIContext::EndFrame(Window* window)
{
	if (!backend || !window)
		return;

	backend->EndFrame(window->GetNativeWindow());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		window->SetAsGraphicsTarget();
}

//TODO: Change to framebuffers
void EditorGUIContext::BeginDockingSpace(Window* window)
{
	if (!backend || !window)
		return;

	MatchWindowSizeToViewport();
	ApplyInvisibleWindowStyle();

	ImGui::Begin(rootWindowID, nullptr, GetRootWindowFlags());

	RestoreNormalWindowStyle();

	unsigned int dockspaceID = ImGui::GetID(mainDockingSpaceID);
	// creates docking space						 passthru flag so  we can interact with scene
	ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	static bool firstInit = true;
	if (firstInit)
	{
		SetupInitialLayout(dockspaceID);
		firstInit = false;
	}

	ImGui::End();
}

void EditorGUIContext::RenderUI(Window* window, EntityManager& entityManager)
{
	EditorGUIContext::NewFrame(window);
	EditorGUIContext::BeginDockingSpace(window);
		
	EditorGUI::Begin(inspectorName);
	EditorGUI::DrawInspector(entityManager);
	EditorGUI::End();

	EditorGUI::Begin(hierarchyName);
	EditorGUI::DrawHierarchy(entityManager);
	EditorGUI::End();

	EditorGUI::Begin(fileBrowserName);
	EditorGUI::DrawFileBrowser();
	EditorGUI::End();

	EditorGUIContext::EndFrame(window);
}

void EditorGUIContext::MatchWindowSizeToViewport()
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
}

void EditorGUIContext::ApplyInvisibleWindowStyle()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
}

int EditorGUIContext::GetRootWindowFlags()
{
	return ImGuiWindowFlags_NoDocking
		 | ImGuiWindowFlags_NoTitleBar
		 | ImGuiWindowFlags_NoCollapse
		 | ImGuiWindowFlags_NoResize
		 | ImGuiWindowFlags_NoMove
		 | ImGuiWindowFlags_NoBringToFrontOnFocus
		 | ImGuiWindowFlags_NoNavFocus
		 | ImGuiWindowFlags_MenuBar
		 | ImGuiWindowFlags_NoBackground;
}

void EditorGUIContext::RestoreNormalWindowStyle()
{
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(3);
}

void EditorGUIContext::SetupInitialLayout(unsigned int dockspaceID)
{
	ImGui::DockBuilderRemoveNode(dockspaceID); // clears panel layout from .ini file
	ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace); // creates main grid panel
	ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetMainViewport()->Size); // sets main grid panel size to whole viewport

	// splits main panel to 4 panels: hierarchy, inspector, viewport, fileBrowser
	unsigned int hierarchyPanelID   = ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Left,  defaultHierarchyRatio,   nullptr, &dockspaceID);
	unsigned int inspectorPanelID   = ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Right, defaultInspectorRatio,   nullptr, &dockspaceID);
	unsigned int fileBrowserPanelID = ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Down, defaultFileBrowserRatio, nullptr, &dockspaceID);

	ImGui::DockBuilderDockWindow(hierarchyName,   hierarchyPanelID);
	ImGui::DockBuilderDockWindow(inspectorName,   inspectorPanelID);
	ImGui::DockBuilderDockWindow(fileBrowserName, fileBrowserPanelID);
	ImGui::DockBuilderDockWindow(viewportName,    dockspaceID);
	
	ImGui::DockBuilderFinish(dockspaceID);
}

void EditorGUIContext::SetInputCapture(bool enabled)
{
	ImGuiIO& io = ImGui::GetIO();

	if (enabled)
	{
		// enable mouse interaction, remove noMouse flag
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
	}
	else
	{
		// disable interactions, add noMouse flag and set mouse pos outside window
		io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
		io.MousePos = ImVec2(-1.0f, -1.0f);
	}
}

void EditorGUIContext::OnEngineModeChanged(const EngineModeChangedEvent& event)
{
	bool enabled = (event.mode == EngineMode::Editor);
	EditorGUIContext::SetInputCapture(enabled);
}



}