#include "Core/SuntaPreCompiled.h"
#include "MathUtilities.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Sunta
{

glm::vec3 Math::DegreesToDirection(const glm::vec3& rotation)
{
	glm::vec3 direction;

	float yaw   = glm::radians(rotation.y);
	float pitch = glm::radians(rotation.x);

	direction.x = cos(yaw) * cos(pitch);
	direction.y = sin(pitch);
	direction.z = sin(yaw) * cos(pitch);

	return glm::normalize(direction);
}

}