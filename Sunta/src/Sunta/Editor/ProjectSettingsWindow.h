#pragma once

namespace Sunta
{

// "Project" menu and the "Project Settings" window of the editor:
// name of the game, start scene, window size, fullscreen, vsync, icon, and building the game (with or without the development console)
// Everything except the "Development build" choice is stored in the game.config of the open project
class ProjectSettingsWindow
{
public:
	// Items of the "Project" menu, call between ImGui::BeginMenu("Project") and ImGui::EndMenu()
	static void DrawProjectMenuItems();

	// Draws the window when it is open. Call once per frame, outside of any menu
	static void Draw();

	// A build runs on its own thread: the editor calls this before it closes, so no file is left half copied
	static void WaitForBuildToFinish();
};

}
