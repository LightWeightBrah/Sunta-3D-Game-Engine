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

namespace
{

// Anything shorter than this can't be a scale
constexpr float SmallestScale = 0.000001f;

}

glm::mat4 Math::ComposeTransform(const TransformParts& parts)
{
	return glm::translate(glm::mat4(1.0f), parts.position)
		* glm::mat4_cast(parts.rotation)
		* glm::scale(glm::mat4(1.0f), parts.scale);
}

TransformParts Math::DecomposeTransform(const glm::mat4& matrix)
{
	const glm::vec3 axisX = glm::vec3(matrix[0]);
	const glm::vec3 axisY = glm::vec3(matrix[1]);
	const glm::vec3 axisZ = glm::vec3(matrix[2]);

	TransformParts parts;

	parts.position = glm::vec3(matrix[3]);
	parts.scale    = glm::vec3(glm::length(axisX), glm::length(axisY), glm::length(axisZ));

	const glm::vec3 safeScale = glm::max(parts.scale, glm::vec3(SmallestScale));
	const glm::mat3 rotation  = glm::mat3(axisX / safeScale.x, axisY / safeScale.y, axisZ / safeScale.z);

	parts.rotation = glm::normalize(glm::quat_cast(rotation));

	return parts;
}

glm::mat4 Math::RemoveScale(const glm::mat4& matrix)
{
	TransformParts parts = DecomposeTransform(matrix);
	parts.scale = glm::vec3(1.0f);

	return ComposeTransform(parts);
}

}