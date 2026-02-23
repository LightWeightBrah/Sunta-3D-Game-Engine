#include "SuntaPreCompiled.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Systems.h"
#include "EntityManager.h"
#include "Component.h"

void Sunta::Systems::UpdateTransform(EntityManager& entityManager)
{
	auto& transforms = entityManager.GetAllComponents<TransformComponent>();
	auto& matrices = entityManager.GetAllComponents<WorldMatrixComponent>();

	for (int i = 0; i < transforms.size(); i++)
	{
		if (transforms[i].isDirty)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, transforms[i].position);

			model = glm::rotate(model, glm::radians(transforms[i].rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, glm::radians(transforms[i].rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(transforms[i].rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

			model = glm::scale(model, transforms[i].scale);

			matrices[i].matrix = model;

			transforms[i].isDirty = false;
		}

	}
}

