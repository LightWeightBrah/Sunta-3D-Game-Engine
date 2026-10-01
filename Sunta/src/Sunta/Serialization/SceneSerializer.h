#pragma once

#include <string>

namespace Sunta
{

class Scene;

class SceneSerializer
{
public:
	static bool Serialize(const std::string& filepath, Scene& scene);
	static bool Deserialize(const std::string& filepath, Scene& scene);
};

}