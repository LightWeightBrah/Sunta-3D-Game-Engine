#pragma once
#include "EditorProperty.h"
#include <string>
#include <vector>

namespace Sunta
{

class Inspectable
{
public:
	virtual ~Inspectable() = default;

	std::vector<EditorProperty> editorProperties;

protected:
	void AddProperty(const std::string& name, void* data, PropertyType type)
	{
		editorProperties.push_back({ name, data, type });
	}

};


}