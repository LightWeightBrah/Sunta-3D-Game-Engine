#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Transform
{
public:
	

	void SetPosition(const glm::vec3& position) { this->position = position; isDirty = true; }
	void SetRotation(const glm::vec3& rotation) { this->rotation = rotation; isDirty = true; }
	void SetScale   (const glm::vec3& scale)	{ this->scale	 = scale;	 isDirty = true; }

	const glm::vec3& GetPosition() const { return position; }
	const glm::vec3& GetRotation() const { return rotation; }
	const glm::vec3& GetScale()	   const { return scale;	}

	glm::mat4 GetModelMatrix()
	{
		if (isDirty)
		{
			cachedMatrix = GetMatrix();
			isDirty = false;
		}

		return cachedMatrix;
	}

private:
	bool isDirty = true;

	glm::vec3 position		= glm::vec3(0.0f);
	glm::vec3 rotation		= glm::vec3(0.0f);
	glm::vec3 scale			= glm::vec3(1.0f);

	glm::mat4 cachedMatrix	= glm::mat4(1.0f);

	glm::mat4 GetMatrix()
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, position);

		model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		model = glm::scale(model, scale);

		return model;
	}
};
