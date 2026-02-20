#pragma once
#include <string>
#include <glm/glm.hpp>


namespace Sunta
{
class Inspectable;
struct EditorProperty;

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

	static void DrawInspector(Inspectable* obj);
	static void ClearFocus();

private:
	static bool DrawProperty(EditorProperty* property);
	static void DrawFolder(const char* name, Inspectable* subObject);

};

}