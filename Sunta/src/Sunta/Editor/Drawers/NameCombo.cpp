#include "Core/SuntaPreCompiled.h"
#include "NameCombo.h"

#include <imgui/imgui.h>

#include "Animation/Animation.h"

namespace Sunta
{

namespace
{

constexpr const char* NoNameLabel = "(none)";

bool DrawSelectable(const char* name, bool isSelected)
{
	if (isSelected)
		ImGui::SetItemDefaultFocus();

	return ImGui::Selectable(name, isSelected) && !isSelected;
}

}

bool DrawNameCombo(const char* label, std::string& chosenName, const std::vector<std::string>& names)
{
	bool changed = false;

	const std::string preview = chosenName.empty() ? NoNameLabel : chosenName;

	if (ImGui::BeginCombo(label, preview.c_str()))
	{
		if (DrawSelectable(NoNameLabel, chosenName.empty()))
		{
			chosenName.clear();
			changed = true;
		}

		for (const std::string& name : names)
		{
			if (DrawSelectable(name.c_str(), chosenName == name))
			{
				chosenName = name;
				changed    = true;
			}
		}

		ImGui::EndCombo();
	}

	return changed;
}

bool DrawAnimationCombo(const char* label, std::string& chosenName, const std::map<std::string, Animation>& animations)
{
	std::vector<std::string> names;
	names.reserve(animations.size());

	for (const auto& [animationName, animation] : animations)
		names.push_back(animationName);

	return DrawNameCombo(label, chosenName, names);
}

}
