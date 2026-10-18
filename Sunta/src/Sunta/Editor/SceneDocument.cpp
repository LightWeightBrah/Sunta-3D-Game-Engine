#include "Core/SuntaPreCompiled.h"
#include "SceneDocument.h"

#include <filesystem>

#include "EditorGUI.h"
#include "Core/Platform.h"
#include "Core/VirtualFileSystem.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"
#include "Scene/Scene.h"
#include "Serialization/SceneSerializer.h"

namespace Sunta
{

namespace
{

namespace fs = std::filesystem;

constexpr const char* SceneDialogName     = "Scene Files";
constexpr const char* SceneExtension      = "scene";
constexpr const char* ScenesVirtualFolder = "@game/Scenes";
constexpr const char* UntitledSceneName   = "Untitled_Scene";

Scene* openScene = nullptr;

// The scene as text, taken at the last save or load. Any difference from the scene now means unsaved changes
std::string savedSnapshot;

// Folder where the scene dialogs (open / save) start
// make_preferred() gives "C:\Dev\..." instead of a mix of '/' and '\', which Windows dialogs expect
std::string GetSceneDialogStartDirectory()
{
	return fs::path(VirtualFileSystem::Resolve(ScenesVirtualFolder)).make_preferred().string();
}

}

void SceneDocument::Init(Scene& scene)
{
	openScene = &scene;
	MarkSaved();
}

void SceneDocument::MarkSaved()
{
	if (openScene)
		savedSnapshot = SceneSerializer::SerializeToString(*openScene);

	NotifyChanged();
}

void SceneDocument::NotifyChanged()
{
	EventBus::Publish<SceneChangedEvent>({});
}

bool SceneDocument::HasUnsavedChanges()
{
	return openScene && SceneSerializer::SerializeToString(*openScene) != savedSnapshot;
}

std::string SceneDocument::GetSceneName()
{
	return openScene ? openScene->GetName() : std::string();
}

bool SceneDocument::Save()
{
	if (!openScene)
		return false;

	if (openScene->GetFilePath().empty())
		return SaveAs();

	if (!SceneSerializer::Serialize(openScene->GetFilePath(), *openScene))
		return false;

	MarkSaved();
	return true;
}

bool SceneDocument::SaveAs()
{
	if (!openScene)
		return false;

	const std::string chosenPath = Platform::Get().SaveFileDialog(SceneDialogName, { SceneExtension }, GetSceneDialogStartDirectory());
	if (chosenPath.empty())
		return false;

	const fs::path normalizedPath = fs::path(chosenPath).lexically_normal();

	openScene->SetName(normalizedPath.stem().string());
	openScene->SetFilePath(normalizedPath.string());

	if (!SceneSerializer::Serialize(normalizedPath.string(), *openScene))
		return false;

	MarkSaved();
	return true;
}

void SceneDocument::New()
{
	if (!openScene)
		return;

	openScene->Clear();
	openScene->SetName(UntitledSceneName);
	openScene->SetFilePath("");

	EditorGUI::ClearSelection(); // old entity IDs don't exist in the new scene
	MarkSaved();
}

void SceneDocument::Open(const std::string& path)
{
	if (!openScene || !SceneSerializer::Deserialize(path, *openScene))
		return;

	EditorGUI::ClearSelection(); // old entity IDs don't exist in the new scene
	MarkSaved();
}

void SceneDocument::OpenViaDialog()
{
	const std::string path = Platform::Get().OpenFileDialog(SceneDialogName, { SceneExtension }, GetSceneDialogStartDirectory());

	if (!path.empty())
		Open(path);
}

void SceneDocument::StopPlayIfRunning()
{
	if (openScene && openScene->IsPlaying())
		EditorGUI::StopPlayMode(*openScene);
}

}
