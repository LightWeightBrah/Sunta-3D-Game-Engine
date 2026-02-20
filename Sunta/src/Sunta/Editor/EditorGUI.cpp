#include "EditorGUI.h"
#include <imgui/imgui.h>
#include "../Inspectable.h"


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

void EditorGUI::DrawInspector(Inspectable* obj)
{
	if (!obj)
		return;

	for (auto& property : obj->editorProperties)
	{
		if (DrawProperty(&property))
		{
			if (property.onUpdate)
				property.onUpdate();
		}
	}
}


bool EditorGUI::DrawProperty(EditorProperty* property)
{
	bool changed = false;
	const char* label = property->labelName.c_str();
	void* data = property->data;

	switch (property->type)
	{
	case PropertyType::Folder:
		DrawFolder(label, (Inspectable*)(data));
		break;

	case PropertyType::Int:
		changed = ImGui::DragInt(label, (int*)data, 0.1f);
		break;
	case PropertyType::Float:
		changed = ImGui::DragFloat(label, (float*)data, 0.1f);
		break;
	case PropertyType::Bool:
		changed = ImGui::Checkbox(label, (bool*)data);
		break;
	case PropertyType::Float3:
		changed = ImGui::DragFloat3(label, (float*)data, 0.1f);
		break;
	case PropertyType::Color:
		changed = ImGui::ColorEdit3(label, (float*)data);
		break;

	case PropertyType::None:
		break;

	default:
		break;
	}

	return changed;
}

void EditorGUI::DrawFolder(const char* label, Inspectable* subObject)
{
	if (!subObject)
		return;

	//This adds a bit of indent margin to the right
	ImGui::Indent();

	if (BeginGroup(label))
	{
		DrawInspector(subObject);
		EndGroup();
	}

	ImGui::Unindent();
}

}