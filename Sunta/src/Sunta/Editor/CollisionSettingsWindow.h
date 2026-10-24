#pragma once

namespace Sunta
{

// The "Collision Layers & Matrix" window of the editor (Project > Collision Layers & Matrix...): the list of collision layers
// (add, rename, remove) and a table of checkboxes that says which layers collide with which
// Every change is applied at once and saved in the game.config of the open project
class CollisionSettingsWindow
{
public:
	// Reads the collision settings of the open project, call once when the editor starts
	static void LoadProjectSettings();

	// The menu item that opens the window, call between ImGui::BeginMenu("Project") and ImGui::EndMenu()
	static void DrawMenuItem();

	// Draws the window when it is open. Call once per frame, outside of any menu
	static void Draw();
};

}
