#pragma once

namespace Sunta
{

// "Project" menu and the "Project Settings" window of the editor:
// name of the game, start scene, window size, icon, and building the game (Debug or Release)
// Everything except the build configuration is stored in the game.config of the open project
class ProjectSettingsWindow
{
public:
	// Items of the "Project" menu, call between ImGui::BeginMenu("Project") and ImGui::EndMenu()
	static void DrawProjectMenuItems();

	// Draws the window when it is open. Call once per frame, outside of any menu
	static void Draw();
};

}
