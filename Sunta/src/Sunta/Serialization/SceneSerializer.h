#pragma once

#include <string>

namespace Sunta
{

class Scene;

class SceneSerializer
{
public:
	static bool Serialize(const std::string& filepath, Scene& scene);

	// The text that Serialize() writes to a file (the editor compares it to find unsaved changes)
	static std::string SerializeToString(Scene& scene);

	static bool Deserialize(const std::string& filepath, Scene& scene);
};

}
