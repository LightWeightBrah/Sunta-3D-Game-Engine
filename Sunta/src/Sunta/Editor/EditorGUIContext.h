#pragma once
#include <memory>

namespace Sunta
{

class Window;
class EditorGUIBackend;
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

private:
	static std::unique_ptr<EditorGUIBackend> backend;
	static unsigned int engineModeChangeID;

	static constexpr float defaultSidebarRatio		= 0.3f; //30% screen width

	static constexpr const char* inspectorName		= "Sunta Engine Editor";
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