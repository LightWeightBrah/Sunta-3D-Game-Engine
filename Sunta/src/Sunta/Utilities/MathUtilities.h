#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Sunta
{

// A placement made of a position, a turn and a size (what a Transform holds)
struct TransformParts
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 scale    = glm::vec3(1.0f);
};

class Math
{
public:
	static glm::vec3 DegreesToDirection(const glm::vec3& rotation);

	// position * rotation * scale, the matrix every entity and every bone is placed with
	static glm::mat4 ComposeTransform(const TransformParts& parts);

	// The opposite: splits such a matrix back into its parts (shear, which such a matrix never has, is lost)
	static TransformParts DecomposeTransform(const glm::mat4& matrix);

	// The same matrix with the size taken out of its axes: only the position and the turn stay
	static glm::mat4 RemoveScale(const glm::mat4& matrix);
};

}