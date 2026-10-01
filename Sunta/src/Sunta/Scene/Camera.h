#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), float pitch = 0.0f, float yaw = -90.0f);
	~Camera();

	void HandleKeyboardMove(MOVEMENT direction, float deltaTime);
	void HandleMouseMovement(float xOffset, float yOffset);
	void HandleScrolling(float yOffset);
	
	void UpdateProjectionMatrix();
	void SetViewportSize(float windowWidth, float windowHeight);
	
	inline const glm::mat4	GetViewMatrix()			const { return glm::lookAt(position, position + front, up); } ;
	inline const glm::mat4	GetProjectionMatrix()	const { return projectionMatrix; }
	inline const glm::vec3	GetPosition()			const { return position; }
	inline float			GetFov()				const { return fov; }
		
	SceneData				GetSceneData()			const;
	Ray                     GetMouseScreenPositionToPointRay(const glm::vec2& mousePosition, float viewportWidth, float viewportHeight) const;
		
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