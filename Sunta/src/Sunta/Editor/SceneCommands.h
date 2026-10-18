#pragma once

namespace Sunta
{

// What the File menu does with the open scene, so a menu item and its keyboard shortcut run exactly the same code
// The shortcut labels are what the menu shows next to the item (EditorShortcuts reacts to the same keys)
class SceneCommands
{
public:
	static constexpr const char* NewShortcutLabel    = "Ctrl+N";
	static constexpr const char* OpenShortcutLabel   = "Ctrl+O";
	static constexpr const char* SaveShortcutLabel   = "Ctrl+S";
	static constexpr const char* SaveAsShortcutLabel = "Ctrl+Shift+S";

	// New and Open ask about unsaved changes first
	static void New();
	static void Open();

	static void Save();
	static void SaveAs();
};

}
