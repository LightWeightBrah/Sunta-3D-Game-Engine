#pragma once
#include <string>

namespace Sunta
{

class Scene;

// The scene open in the editor seen as a document: whether it has changes that are not saved yet, and saving / opening it
// Changes are found by comparing the scene with a snapshot taken at the last save or load, so HasUnsavedChanges()
// is right even when some code forgets to call NotifyChanged(): that only makes the window title late
class SceneDocument
{
public:
	// Remembers the scene the editor works on and takes the first snapshot. Call once, after the scene is loaded
	static void Init(Scene& scene);

	// Not meant to be asked while the game plays (physics and scripts change the scene), see StopPlayIfRunning()
	static bool HasUnsavedChanges();

	static std::string GetSceneName();

	// Code that edits the scene calls this, so the editor knows when to look for unsaved changes again
	// It only sends a SceneChangedEvent (the scene is not compared here), so it is cheap to call every frame of a drag
	static void NotifyChanged();

	// Saves to the file of the scene, asks for a file when the scene has none
	// False when the user cancelled the file dialog or the file couldn't be written
	static bool Save();
	static bool SaveAs();

	static void New();
	static void Open(const std::string& path);
	static void OpenViaDialog();

	// Puts the scene back as it was before Play, so the question about changes is about the editor's scene
	static void StopPlayIfRunning();

	// The scene was just loaded or saved by other code, so it counts as unchanged from now on
	static void MarkSaved();
};

}
