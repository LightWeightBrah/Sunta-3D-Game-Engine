#pragma once

namespace Sunta
{

class Scene;

// Keyboard shortcuts of the editor menus (Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S)
// They do what the menu items do and are ignored in the same situations: while the game plays,
// while typing in a text field and while a menu or a dialog is open
class EditorShortcuts
{
public:
	// Call once per frame, after the ImGui frame was built
	static void Process(const Scene& scene);
};

}
