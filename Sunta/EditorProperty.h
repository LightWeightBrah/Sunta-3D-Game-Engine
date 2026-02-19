#pragma once

namespace Sunta
{

enum class PropertyType
{
	None = 0,
	Float,
	Float3,
	Color,
	Bool
};

struct EditorProperty
{
	const char* name;
	void* data;
	PropertyType type;
};

}