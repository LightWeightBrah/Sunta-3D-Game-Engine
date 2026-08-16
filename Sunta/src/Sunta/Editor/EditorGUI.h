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

struct FileDroppedEvent;

class EditorGUI
{
public:
	static void Begin(const std::string& name);
	static void End();

	static bool BeginGroup(const std::string& label);
	static void EndGroup();

	static bool PropertyFloat3(const std::string& label, glm::vec3& value);
	static bool PropertyColor(const std::string& label, glm::vec4& color);
	static bool PropertyBool(const std::string& label, bool& value);

	static void Text(const std::string& text);
	static bool Button(const std::string& label);

	static void DrawMainMenuBar(Scene& scene);
	static void DrawToolbar(Scene& scene, RendererDevice& rendererDevice, float toolbarHeight);
	static void DrawHierarchy(EntityManager& entityManager);
	static void DrawInspector(EntityManager& entityManager);
	static void DrawFileBrowser();
	static void DrawSceneDropTarget(Scene& scene);

	static void OnFileDropped(const FileDroppedEvent& event);

	static bool DrawTextureSlot(const char* label, std::shared_ptr<Texture>& texture);
	
	static void HandleSelectionInteraction();
	static void ClearSelection();
	static void ClearFocus();

	static void SetDarkTheme();
	static void DrawWindowBackground(const std::string& textureKey, glm::vec4 tintColor = glm::vec4(1.0f));
private:
	struct PendingCreationState
	{
		bool active = false;
		char nameBuffer[256] = "";
		std::string extension = "";
		std::string iconKey = "";
		std::function<void(const std::filesystem::path& fullPath)> onCreate;
	};

	static inline PendingCreationState pendingCreation;

	static inline int selectedEntity = -1;

	static inline std::filesystem::path currentDirectory = "";
	static inline std::filesystem::path selectedFile	 = "";

	static inline int entityToRename = -1;
	static inline char entityNameBuffer[256] = "";

	static inline std::filesystem::path fileToRename = "";
	static inline char fileRenameBuffer[256] = "";

	static inline std::filesystem::path pendingRenamePath = "";
	static inline std::string pendingRenameNewName = "";

	static inline std::filesystem::path lastSelectedFile;
	static inline std::shared_ptr<Material> currentMaterial = nullptr;
	
	static void DrawEntityComponentList(unsigned int entityID, EntityManager& entityManager);
	static void DrawSingleComponent(unsigned int entityID, const ComponentType* componentType, IInspectableStorage* pool);
	static bool DrawPropertyWidget(const PropertyDefinition& property, void* propertyData);
	static std::string GetIconKeyForPath(const std::filesystem::path& path, bool isDirectory);

	static void DeletePathAndUnloadResources(const std::filesystem::path& path);

	static bool IsClickingEmptySpace();

};

}