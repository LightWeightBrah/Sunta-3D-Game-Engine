#pragma once
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <filesystem>
#include <functional>

namespace Sunta
{
class PropertyDefinition;
class EntityManager;
class ComponentType;
class IInspectableStorage;
class Scene;
class RendererDevice;
class Material;
class Texture;
class ScriptComponent;

struct FileDroppedEvent;

enum class GizmoOperation 
{ 
	Translate, 
	Rotate, 
	Scale 
};

enum class GizmoSpace 
{ 
	World, 
	Local 
};

struct PendingCreationState
{
	bool active           = false;
	char nameBuffer[256]  = "";
	std::string extension = "";
	std::string iconKey   = "";
	std::function<void(const std::filesystem::path& fullPath)> onCreate;
};

class EditorGUI
{
public:
	static void Init();
	static void Shutdown();

	static void Begin(const std::string& name);
	static void End();

	static bool BeginGroup(const std::string& label);
	static void EndGroup();

	static bool PropertyFloat3(const std::string& label, glm::vec3& value);
	static bool PropertyColor(const std::string& label, glm::vec4& color);
	static bool PropertyBool(const std::string& label, bool& value);

	static void Text(const std::string& text);
	static bool Button(const std::string& label);

	static void StartPlayMode(Scene& scene);
	static void StopPlayMode(Scene& scene);

	static void DrawMainMenuBarAndToolbar(Scene& scene, float toolbarHeight);

	static void DrawHierarchy(EntityManager& entityManager);
	static void DrawInspector(EntityManager& entityManager);
	static void DrawFileBrowser(Scene& scene);
	static void DrawTransformGizmo(Scene& scene);
	static void DrawSceneDropTarget(Scene& scene);

	static void OnFileDropped(const FileDroppedEvent& event);

	static bool DrawTextureSlot(const char* label, std::shared_ptr<Texture>& texture);

	static void HandleSelectionInteraction(Scene& scene);
	static void ClearSelection();
	static void ClearFocus();

	static void SetDarkTheme();
	static void DrawWindowBackground(const std::string& textureKey, glm::vec4 tintColor = glm::vec4(1.0f));
private:
	static inline bool           showTransformGizmo = true;
	static inline GizmoOperation gizmoOperation     = GizmoOperation::Translate;
	static inline GizmoSpace     gizmoSpace			= GizmoSpace::World;

	static inline PendingCreationState pendingCreation;

	static inline int selectedEntity = -1;

	// What the Inspector currently displays. Lags `selectedEntity` while locked
	static inline int inspectedEntity = -1;

	static inline std::filesystem::path currentDirectory = "";
	static inline std::filesystem::path selectedFile = "";
	static inline std::filesystem::path preDragSelectedFile = "";

	// What the Inspector currently displays. Lags `selectedFile` briefly whenever a
	// click could still turn into a drag
	static inline std::filesystem::path inspectedFile = "";

	// While true, the Inspector stops following `selectedFile`/`inspectedFile`
	// changes entirely and keeps showing whatever was open when it was locked
	static inline bool inspectorLocked = false;

	static inline int entityToRename = -1;
	static inline char entityNameBuffer[256] = "";

	static inline std::filesystem::path fileToRename = "";
	static inline char fileRenameBuffer[256] = "";

	static inline std::filesystem::path pendingRenamePath = "";
	static inline std::string pendingRenameNewName = "";

	static inline std::filesystem::path lastSelectedFile;
	static inline std::shared_ptr<Material> currentMaterial = nullptr;

	static void DrawTopBarsBackground(const std::string& texture, glm::vec4 tintColor, glm::vec2 backgroundPosition, glm::vec2 backgroundSize);
	static void DrawMainMenuBar(Scene& scene);
	static void DrawToolbar(Scene& scene, float toolbarHeight);

	static void DrawEntityComponentList(unsigned int entityID, EntityManager& entityManager);
	static void DrawSingleComponent(unsigned int entityID, EntityManager& entityManager, const ComponentType* componentType, IInspectableStorage* pool);
	static bool DrawPropertyWidget(const PropertyDefinition& property, void* propertyData);
	static std::string GetIconKeyForPath(const std::filesystem::path& path, bool isDirectory);

	static void DeletePathAndUnloadResources(const std::filesystem::path& path);

	static glm::vec2 GetMousePositionInMainWindow();
	static bool IsClickingEmptySpace();

	static void SetSelectedEntity(int entityID);

};

}