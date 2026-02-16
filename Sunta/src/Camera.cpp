#include <iostream>
#include "Camera.h"

Camera::Camera(glm::vec3 position, float pitch, float yaw) 
	: position(position)
	, pitch(pitch)
	, yaw(yaw)
{
	UpdateCamera();
}

void Camera::HandleKeyboardMove(MOVEMENT direction, float deltaTime)
{
	float speed = this->movementSpeed * deltaTime;

	glm::vec3 moveFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
	glm::vec3 moveRight = glm::normalize(glm::vec3(right.x, 0.0f, right.z));

	glm::vec3 targetFront = stayOnHeight ? moveFront : front;
	glm::vec3 targetRight = stayOnHeight ? moveRight : right;

	//std::cout << "pos " << position.x << " " << position.y << " " << position.z << "\n";

	switch (direction)
	{
	case MOVEMENT::FORWARD:		position += targetFront * speed;	break;
	case MOVEMENT::BACKWARD:	position -= targetFront * speed;	break;
	case MOVEMENT::LEFT:		position -= targetRight * speed;	break;
	case MOVEMENT::RIGHT:		position += targetRight * speed;	break;
	case MOVEMENT::UP:			position += worldUp		* speed;	break;
	case MOVEMENT::DOWN: 		position -= worldUp		* speed;	break;
	}
}

void Camera::HandleMouseMovement(float xOffset, float yOffset)
{
	xOffset *= mouseSensitivity;
	yOffset *= mouseSensitivity;

	yaw += xOffset;
	pitch += yOffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	UpdateCamera();
}

void Camera::HandleScrolling(float yOffset)
{
	fov -= (float)yOffset;

	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 45.0f)
		fov = 45.0f;

	UpdateProjectionMatrix();
}

void Camera::UpdateProjectionMatrix()
{
	projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
}

void Camera::SetViewportSize(float windowWidth, float windowHeight)
{
	aspectRatio = windowWidth / windowHeight;
	UpdateProjectionMatrix();
}

SceneData Camera::GetSceneData() const
{
	SceneData data;

	data.viewMatrix			= GetViewMatrix();
	data.projectionMatrix	= GetProjectionMatrix();
	data.cameraPosition		= GetPosition();
	
	return data;
}

void Camera::UpdateCamera()
{
	glm::vec3 direction;

	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front		= glm::normalize(direction);


	// normalize the vectors, cause their length gets closer to 
	// 0 the more you look up or down which results in slower movement
	right		= glm::normalize(glm::cross(front, worldUp));
	up			= glm::normalize(glm::cross(right, front));
}