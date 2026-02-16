#pragma once

#include <vector>
#include <queue>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "AStar.h"
#include "ResourceManager.h"
#include "Animator.h"

class Solaire
{
private:
	glm::vec3 position;
	std::queue<glm::vec3> movingPoints;

	std::shared_ptr<ModelData>	resources;
	Animator					animator;

	glm::vec3 startPoisiton;
	float	  rotation		  = 0.0f;

	const float MOVE_SPEED	  = 2.5f;
	const float FOOT_Y_HEIGHT = 0.55;

public:
	Solaire(glm::vec3 startPosition, const std::string& filepath);

	void SetPath(std::vector<GridPoint> gridPoints);
	void Update (float deltaTime);
	void Reset();

	inline const Model& GetCharacterModel() const { return  resources->model; }
	inline const Animator* GetAnimator  ()  const { return &animator; }
	
	glm::mat4 GetModelMatrix();

private:
	void UpdateAnimations(float deltaTime);
};