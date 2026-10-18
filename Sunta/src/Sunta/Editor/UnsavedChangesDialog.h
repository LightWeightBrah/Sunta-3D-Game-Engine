#pragma once
#include <functional>

namespace Sunta
{

// The question "Save changes?" before something that would lose or ignore the unsaved changes of the scene
// (closing the editor, New / Open Scene, Switch Project, Build Game)
class UnsavedChangesDialog
{
public:
	// Runs the action at once when the scene has no unsaved changes
	// Otherwise asks Save / Don't Save / Cancel first and runs the action after Save or Don't Save (Cancel drops it)
	// While the game plays it is stopped first, so the question is about the scene of the editor
	static void RunAfterAsking(std::function<void()> action);

	// Draws the question when there is one. Call once per frame, outside of any menu
	static void Draw();
};

}
