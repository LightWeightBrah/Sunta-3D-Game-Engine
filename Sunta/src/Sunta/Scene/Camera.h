#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "SceneData.h"

namespace Sunta
{

struct EngineModeChangedEvent;
struct Ray;

enum class MOVEMENT
{
	FORWARD = 0,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN,
};
	
class Camera
{
public:
	// Range of the movement speed (units per second) the user can choose in the editor
	static constexpr float MinMovementSpeed = 0.25f;
	static constexpr float MaxMovementSpeed = 100.0f;

	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), float pitch = 0.0f, float yaw = -90.0f);
	~Camera();

	void HandleKeyboardMove(MOVEMENT direction, float deltaTime);
	void HandleMouseMovement(float xOffset, float yOffset);

	// The mouse wheel changes the movement speed (the camera is locked in the editor mode, so the wheel does nothing there)
	void HandleScrolling(float yOffset);

	void  SetMovementSpeed(float speed);
	inline float GetMovementSpeed() const { return movementSpeed; }
	
	void UpdateProjectionMatrix();
	void SetViewportSize(float windowWidth, float windowHeight);
	
	inline const glm::mat4	GetViewMatrix()			const { return glm::lookAt(position, position + front, up); } ;
	inline const glm::mat4	GetProjectionMatrix()	const { return projectionMatrix; }
	inline const glm::vec3	GetPosition()			const { return position; }
	inline float			GetFov()				const { return fov; }

	// Orientation of the camera, used to copy the view of the editor into an entity
	glm::quat				GetRotation()			const;
		
	SceneData				GetSceneData()			const;
	Ray                     GetMouseScreenPositionToPointRay(const glm::vec2& mousePosition, float viewportWidth, float viewportHeight) const;

	// The same for any view, e.g. the one of a CameraComponent while the game plays
	static Ray              ScreenPointToRay(const glm::vec2& mousePosition, float viewportWidth, float viewportHeight,
											 const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
		
	inline void				HandleStayOnHeight(bool stayOnHeight) { this->stayOnHeight = stayOnHeight; }
private:
	glm::vec3	 position;
	glm::vec3	 up					=  glm::vec3(0.0f, 1.0f,  0.0f);
	glm::vec3	 front				=  glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3	 right				=  glm::vec3(1.0f, 0.0f,  0.0f);
	glm::vec3	 worldUp			=  glm::vec3(0.0f, 1.0f,  0.0f);
				 
	float		 pitch				=  0.0f;
	float		 yaw				= -90.0f;
	float		 fov				=  45.0f;
				 
	float		 movementSpeed		=  3.5f;
	float		 mouseSensitivity	=  0.1f;
	bool		 stayOnHeight		=  false;
				 							   
	glm::mat4	 projectionMatrix	=  glm::mat4(1.0f);
	float		 aspectRatio		=  16.0 / 9.0f; // safety for first frame
				 
	bool		 isLocked			=  false;
	unsigned int subscriptionID;

	void UpdateCamera();

	void OnEngineModeChanged(const EngineModeChangedEvent& event);
};

}