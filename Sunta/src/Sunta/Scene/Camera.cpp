#include "Core/SuntaPreCompiled.h"

#include "Camera.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"
#include "Physics/CollisionShapes.h"

namespace Sunta
{

Camera::Camera(glm::vec3 position, float pitch, float yaw) 
	: position(position)
	, pitch(pitch)
	, yaw(yaw)
{
	subscriptionID = EventBus::Subscribe<EngineModeChangedEvent>([this](const auto& event) { OnEngineModeChanged(event); });
	UpdateCamera();
}

Camera::~Camera()
{
	EventBus::Unsubscribe(subscriptionID);
}

	
void Camera::HandleKeyboardMove(MOVEMENT direction, float deltaTime)
{
	if (isLocked)
		return;

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
	if (isLocked)
		return;

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
	if (isLocked)
		return;

	fov -= (float)yOffset;
	
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 45.0f)
		fov = 45.0f;
	
	UpdateProjectionMatrix();
}
	
void Camera::UpdateProjectionMatrix()
{
	if (aspectRatio <= 0.0f)
		return;

	projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
}
	
void Camera::SetViewportSize(float windowWidth, float windowHeight)
{
	// Don't calculate aspect ratio if window is minimzed (thus width and height = 0 when minimized)
	if (windowWidth <= 0.0f || windowHeight <= 0.0f)
		return;

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
	
Ray Camera::GetMouseScreenPositionToPointRay(const glm::vec2& mousePosition, float viewportWidth, float viewportHeight) const
{
	if (viewportWidth <= 0.0f || viewportHeight <= 0.0f)
		return Ray{ position, front };

	// Screen space: top-left is (0, 0), bottom-right is (viewportWidth, viewportHeight), Y pointing DOWN
	// NDC (normalized device coordinates): (X,Y) = [ (-1.0f, 1.0f), (-1.0f, 1.0f) ]
	
	// Map X pixel position [0, width] to [-1, 1] range:
	// mousePosition.x / viewportWidth -> gets a 0 to 1 ratio
	// Multiply by 2 -> scales it to [0, 2]
	// Subtract 1 -> shifts it to [-1, 1] (Left: -1, Center: 0, Right: 1)
	float normalizedDeviceX =        (2.0f * mousePosition.x) / viewportWidth - 1.0f;

	// Map Y pixel position [0, height] to [-1, 1] AND flip the Y-axis:
	// (2.0f * mousePosition.y) / height -> scales to [0, 2] with Y growing down
	// Subtract from 1.0f -> flips the axis so Top becomes 1, Center is 0, and Bottom becomes -1
	float normalizedDeviceY = 1.0f - (2.0f * mousePosition.y) / viewportHeight;

	// Clip space: same as NDC, but with a w component 
	// w = 1 marks this as a POINT (as opposed to a direction) 
	// z = -1 just means "toward the camera's near plane"
	glm::vec4 rayClip(normalizedDeviceX, normalizedDeviceY, -1.0f, 1.0f);

	// Undo the projection matrix to go from clip space to view/eye space
	// (the space where the camera sits at the origin, looking down -Z)
	glm::vec4 rayViewSpace = glm::inverse(GetProjectionMatrix()) * rayClip;

	// From here on we only care about a DIRECTION, not a specific point, so:
	// - z is reset to -1 (forward, in view space)
	// - w is set to 0, which tells the next matrix multiply "this is a
	//   direction, ignore any position/translation, only apply rotation"
	//   (a point should move with the camera, a direction shouldn't)
	rayViewSpace = glm::vec4(rayViewSpace.x, rayViewSpace.y, -1.0f, 0.0f);

	// Undo the view matrix to go from view space to world space
	// Because w = 0, only the camera's rotation gets applied here (not its position)
	glm::vec3 rayWorldDirection = glm::normalize(glm::vec3(glm::inverse(GetViewMatrix()) * rayViewSpace));

	// The ray starts at the camera and points in the direction we just found
	Ray ray;
	ray.origin = position;
	ray.direction = rayWorldDirection;

	return ray;

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

void Camera::OnEngineModeChanged(const EngineModeChangedEvent& event)
{
	isLocked = (event.mode == EngineMode::Editor);
}

}