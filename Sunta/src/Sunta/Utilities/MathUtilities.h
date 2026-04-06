#pragma once

#include <glm/glm.hpp>

namespace Sunta
{

class Math
{
public:
	static glm::vec3 DegreesToDirection(const glm::vec3& rotation);
};

}