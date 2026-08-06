#pragma once

#include <memory>
#include <string>

namespace Sunta
{

class Material;

class MaterialSerializer
{
public:
	static bool Serialize(const std::string& filepath, const std::shared_ptr<Material>& material);
	static std::shared_ptr<Material> Deserialize(const std::string& filepath);
};

}