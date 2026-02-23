#pragma once
#include <string>
#include <glm/glm.hpp>


namespace Sunta
{
class PropertyDefinition;
class EntityManager;
class ComponentType;
class IInspectableStorage;

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

	static void DrawInspector(EntityManager& entityManager);
	static void ClearFocus();

private:
	static void DrawEntityComponentList(unsigned int entityID, EntityManager& entityManager);
	static void DrawSingleComponent(unsigned int entityID, const ComponentType* componentType, IInspectableStorage* pool);
	static bool DrawPropertyWidget(const PropertyDefinition& property, void* propertyData);

};

}