#pragma once
#include <string>
#include <vector>
#include <functional>


namespace Sunta
{

enum class PropertyType
{
	None = 0,
	Folder,

	Int,
	Float,
	Bool,
	Float3,
	Color,
};

struct EditorProperty
{
	std::string labelName;
	void* data;
	PropertyType type;
	std::function<void()> onUpdate;
};

class Inspectable
{
public:
	virtual ~Inspectable() = default;

	std::vector<EditorProperty> editorProperties;

protected:
	void AddProperty(const std::string& name, void* data, PropertyType type, std::function<void()> onUpdate = nullptr)
	{
		editorProperties.push_back({ name, data, type, onUpdate });
	}

	void AddFolder(const std::string& name, Inspectable* inspectable)
	{
		editorProperties.push_back({ name, inspectable, PropertyType::Folder, nullptr});
	}

};


}