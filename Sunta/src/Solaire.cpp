#include "Solaire.h"

Solaire::Solaire(glm::vec3 startPosition, const std::string& filepath) :
	position(startPosition), startPoisiton(startPosition)
{
	resources = ResourceManager::GetModelData(filepath);
	animator  = Animator  (&resources->model);
	animator.PlayAnimation(&resources->animations.at(AnimationType::IDLE));
}

void Solaire::SetPath(std::vector<GridPoint> gridPoints)
{
	for (auto& point : gridPoints)
		movingPoints.push({ point.x, FOOT_Y_HEIGHT, point.z });
}

void Solaire::Update(float deltaTime)
{
	UpdateAnimations(deltaTime);

	if (movingPoints.empty())
		return;

	glm::vec3 target = movingPoints.front();
	glm::vec3 direction = target - position;

	float distanceToTarget = glm::length(direction);
	
	rotation = glm::degrees(atan2(direction.x, direction.z));

	if (distanceToTarget < 0.1f)
	{
		position = target;
		movingPoints.pop();

		if (movingPoints.empty())
		{
			rotation = 0.0f;
			animator.PlayAnimation(&resources->animations.at(AnimationType::GESTURE));
		}
	}
	else
	{
		glm::vec3 moveDirection = glm::normalize(direction);
		position += moveDirection * MOVE_SPEED * deltaTime;
	}
}

void Solaire::Reset()
{
	position	 = startPoisiton;
	movingPoints = std::queue<glm::vec3>();
	rotation	 = 0.0f;
	animator.PlayAnimation(&resources->animations.at(AnimationType::IDLE));
}

glm::mat4 Solaire::GetModelMatrix()
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);
	model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.00035f, 0.00035f, 0.00035f));

	return model;
}

void Solaire::UpdateAnimations(float deltaTime)
{
	bool isMoving = !movingPoints.empty();

	Animation* gestureAnim = &resources->animations.at(AnimationType::GESTURE);
	if (animator.GetCurrentAnimation() == gestureAnim && !isMoving)
	{
		animator.UpdateAnimation(deltaTime);
		return;
	}

	AnimationType newAnimationType = isMoving ? AnimationType::RUNNING : AnimationType::IDLE;
	Animation* animationtoPlay = &resources->animations.at(newAnimationType);
	if(animator.GetCurrentAnimation() != animationtoPlay)
		animator.PlayAnimation(animationtoPlay);

	animator.UpdateAnimation(deltaTime);
}