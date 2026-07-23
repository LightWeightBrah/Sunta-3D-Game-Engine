#pragma once
#include <memory>

namespace Sunta
{

class Window;
class EditorGUIBackend;
class Scene;
class RendererDevice;
struct EngineModeChangedEvent;

class EditorGUIContext
{
public:
	//DESTRUCTOR required for unique_ptr to work
	~EditorGUIContext();

	static void Init(Window* window);
	static void Shutdown(Window* window);

	static void NewFrame(Window* window);
	static void EndFrame(Window* window);

	static void BeginDockingSpace(Window* window);

	inline static const char* GetInspectorName() { return inspectorName; }

	static void RenderUI(Window* window, Scene& scene, RendererDevice& rendererDevice);
private:
	static std::unique_ptr<EditorGUIBackend> backend;
	static unsigned int engineModeChangeID;

	static constexpr float defaultFileBrowserRatio	= 0.25f; // 25% screen height

	static constexpr float defaultHierarchyRatio	= 0.2f;	 // 20% screen width
	static constexpr float defaultInspectorRatio	= 0.25f; // 20% screen width
	//								from 80% take 25% so we have 60% viewport from initial screen

	static constexpr const char* viewportName		= "Viewport";
	static constexpr const char* fileBrowserName    = "File Browser";
	static constexpr const char* inspectorName		= "Inspector";
	static constexpr const char* hierarchyName		= "Hierarchy";
	static constexpr const char* rootWindowID		= "Main Viewport Docking Window";
	static constexpr const char* mainDockingSpaceID = "Editor Docking Space";
	
	static void MatchWindowSizeToViewport();
	static int  GetRootWindowFlags();
	static void ApplyInvisibleWindowStyle();
	static void RestoreNormalWindowStyle();
	static void SetupInitialLayout(unsigned int dockspaceID);

	static void SetInputCapture(bool enabled);
	static void OnEngineModeChanged(const EngineModeChangedEvent& event);



};

}