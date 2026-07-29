#pragma once
#include <string>
#include <glm/glm.hpp>
#include <filesystem>

namespace Sunta
{
class PropertyDefinition;
class EntityManager;
class ComponentType;
class IInspectableStorage;
class Scene;
class RendererDevice;

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

	static void DrawToolbar(Scene& scene, RendererDevice& rendererDevice, float toolbarHeight);
	static void DrawHierarchy(EntityManager& entityManager);
	static void DrawInspector(EntityManager& entityManager);
	static void DrawFileBrowser();
	
	static void ClearFocus();

	static void SetDarkTheme();
	static void DrawWindowBackground(const std::string& textureKey, glm::vec4 tintColor = glm::vec4(1.0f));
private:
	static inline int selectedEntity = -1;

	static inline bool isCreatingFolder = false;
	static inline char newFolderName[64] = "";

	static inline std::filesystem::path currentDirectory = "";
	static inline std::filesystem::path selectedFile	 = "";
	
	static void DrawEntityComponentList(unsigned int entityID, EntityManager& entityManager);
	static void DrawSingleComponent(unsigned int entityID, const ComponentType* componentType, IInspectableStorage* pool);
	static bool DrawPropertyWidget(const PropertyDefinition& property, void* propertyData);
	static std::string GetIconKeyForPath(const std::filesystem::path& path, bool isDirectory);

};

}