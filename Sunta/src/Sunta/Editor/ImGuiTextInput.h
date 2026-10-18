#pragma once
#include <cstdio>
#include <string>

#include <imgui/imgui.h>

namespace Sunta
{

// ImGui::InputText for std::string. Returns true when the user changed the text
inline bool InputString(const char* label, std::string& value)
{
	constexpr size_t BufferSize = 256;

	char buffer[BufferSize];
	std::snprintf(buffer, BufferSize, "%s", value.c_str());

	if (!ImGui::InputText(label, buffer, BufferSize))
		return false;

	value = buffer;
	return true;
}

}
