#include "Core/SuntaPreCompiled.h"

#include "EditorGUI.h"
#include <imgui/imgui.h>
#include <imgui_internal.h>
#include "ECS/EntityManager.h"
#include "ECS/ComponentLayout.h"


namespace Sunta
{

void EditorGUI::Begin(const std::string& name)
{
	ImGui::Begin(name.c_str());
}

void EditorGUI::End()
{
	ImGui::End();
}

bool EditorGUI::BeginGroup(const std::string& label)
{
	return ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
}

void EditorGUI::EndGroup()
{

}

bool EditorGUI::PropertyFloat3(const std::string& label, glm::vec3& value)
{
	return ImGui::DragFloat3(label.c_str(), &value.x, 0.1f);
}

bool EditorGUI::PropertyColor(const std::string& label, glm::vec4& color)
{
	return ImGui::ColorEdit4(label.c_str(), &color.r);
}

bool EditorGUI::PropertyBool(const std::string& label, bool& value)
{
	return ImGui::Checkbox(label.c_str(), &value);
}

void EditorGUI::Text(const std::string& text)
{
	ImGui::Text(text.c_str());
}

bool EditorGUI::Button(const std::string& label)
{
	return ImGui::Button(label.c_str());
}

void EditorGUI::DrawInspector(EntityManager& entityManager)
{
	unsigned int totalEntities = entityManager.GetEntityCount();

	for (unsigned int entityID = 0; entityID < totalEntities; entityID++)
	{
		std::string label = ("Entity " + std::to_string(entityID));

		if (!ImGui::TreeNode(label.c_str()))
			continue;

		DrawEntityComponentList(entityID, entityManager);

		ImGui::TreePop();

	}
}

void EditorGUI::DrawEntityComponentList(unsigned int entityID, EntityManager& entityManager)
{
	for (auto const& [typeHash, storage] : entityManager.GetInspectableMap())
	{
		if (!storage->Contains(entityID))
			continue;

		const auto* componentType =
			InspectorComponentRegistry::GetComponentTypeByHash(typeHash);

		if (!componentType)
			continue;

		DrawSingleComponent(entityID, componentType, storage);

	}
}

void EditorGUI::DrawSingleComponent(unsigned int entityID, const ComponentType* componentType, IInspectableStorage* storage)
{
	if (!ImGui::CollapsingHeader(componentType->name.c_str()))
		return;

	void* componentData = storage->GetEntityComponentData(entityID);
	bool anyPropertyChanged = false;

	for (const auto& property : componentType->properties)
	{
		void* propertyData = (char*)componentData + property.byteOffset;

		if (DrawPropertyWidget(property, propertyData))
			anyPropertyChanged = true;
	}

	if (anyPropertyChanged && componentType->onChanged)
		componentType->onChanged(componentData);
}

bool EditorGUI::DrawPropertyWidget(const PropertyDefinition& property, void* propertyData)
{
	bool changed = false;

	switch (property.dataType)
	{
	case PropertyDataType::Int:	
		changed = ImGui::DragInt(property.label.c_str(), (int*)propertyData); 
		break;
	case PropertyDataType::Bool:
		changed = ImGui::Checkbox(property.label.c_str(), (bool*)propertyData);
		break;
	case PropertyDataType::Float:
		changed = ImGui::DragFloat(property.label.c_str(), (float*)propertyData, 0.1f);
		break;
	case PropertyDataType::Float3:
		if (property.HasRange())
			changed = ImGui::SliderFloat3(property.label.c_str(), (float*)propertyData, property.minValue, property.maxValue);
		else
			changed = ImGui::DragFloat3(property.label.c_str(), (float*)propertyData, 0.1f);

		break;
	case PropertyDataType::Color:
		changed = ImGui::ColorEdit3(property.label.c_str(), (float*)propertyData);
		break;

	}
	
	return changed;
}

void EditorGUI::ClearFocus()
{
	if (!ImGui::GetCurrentContext())
		return;
	
	//Removes foucs from the active widget (slider, button, color etc.)
	ImGui::ClearActiveID();

	//Deselects the window
	ImGui::SetWindowFocus(nullptr);
}


}