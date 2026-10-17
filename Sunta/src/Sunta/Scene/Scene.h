#pragma once
#include <memory>

#include "Camera.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

class Renderer;
class RendererDevice;

struct TriggerEnterEvent;
struct TriggerStayEvent;
struct TriggerExitEvent;

struct CollisionEnterEvent;
struct CollisionStayEvent;
struct CollisionExitEvent;

struct OBB;

struct CameraComponent;
struct TransformComponent;

// Whether the game simulation (physics, animations, scripts) is running
// Separate from EngineMode (Editor/Game), which only controls camera and cursor
enum class PlayState 
{
	Stopped, 
	Playing 
};

class Scene
{
public:
	Scene();
	~Scene();

	void Init(RendererDevice& rendererDevice, float windowWidth, float windowHeight);
	void ProcessInput();
	void Update();
	void Render(Renderer& renderer);
	void Clear();

	// Starts the game simulation from the current state of the scene
	// Used by the game build on start and by the editor's Play button
	void BeginPlay();

	int       GetEntityUnderMouse       (const glm::vec2& mousePosition, float viewportWidth, float viewportHeight);
	glm::vec3 GetWorldPositionUnderMouse(const glm::vec2& mousePosition, float viewportWidth, float viewportHeight);

	bool      IsPlaying()					 const { return playState == PlayState::Playing; }
	
	EntityManager& GetEntityManager()		       { return entityManager;    }
	PlayState      GetPlayState()			 const { return playState;        }

	// The free camera: the one the editor flies around the scene with (the game uses it only when it has no CameraComponent)
	const Camera&      GetCamera()           const { return camera;           }
	const std::string& GetName()             const { return name;             }
	const std::string& GetFilePath()         const { return filePath;         }

	void SetName(const std::string& name)	       { this->name      = name;  }
	void SetFilePath(const std::string& path)      { this->filePath  = path;  }
	void SetPlayState(PlayState state)			   { this->playState = state; }

	// What the scene is drawn from: the active CameraComponent while the game plays, the free camera otherwise
	// The editor's picking and transform gizmo use the same view, so they line up with what is on the screen
	const glm::mat4& GetViewMatrix()       const { return activeCameraData.viewMatrix;       }
	const glm::mat4& GetProjectionMatrix() const { return activeCameraData.projectionMatrix; }

	// Speed of the free camera (units per second), the mouse wheel changes it too
	float GetEditorCameraSpeed()           const { return camera.GetMovementSpeed(); }
	void  SetEditorCameraSpeed(float speed)      { camera.SetMovementSpeed(speed);   }

	// Puts the entity where the free camera is and turns it the same way (for placing a CameraComponent)
	void AlignEntityWithEditorCamera(unsigned int entityID);


private:
	EntityManager		entityManager;
	Camera				camera;
	PlayState			playState			= PlayState::Stopped;

	// The view of the current frame (matrices and position only), see GetViewMatrix()
	SceneData			activeCameraData;
	float				aspectRatio			= 16.0f / 9.0f; // of the window, a CameraComponent has no size of its own

	unsigned int		resizeEventID;

	unsigned int		triggerEnterEventID = 0;
	unsigned int		triggerStayEventID  = 0;
	unsigned int		triggerExitEventID  = 0;

	unsigned int		collisionEnterEventID = 0;
	unsigned int		collisionStayEventID  = 0;
	unsigned int		collisionExitEventID  = 0;

	std::string			name = "Untitled_Scene";
	std::string			filePath;

	void OnWindowResize(float windowWidth, float windowHeight);

	// The first active CameraComponent together with the Transform of its entity, false when the scene has none
	bool TryFindActiveSceneCamera(CameraComponent*& outCamera, TransformComponent*& outTransform);
	bool ShouldUseSceneCamera();
	void UpdateActiveCameraData();

	void OnTriggerEnter(const TriggerEnterEvent& event);
	void OnTriggerStay (const TriggerStayEvent& event);
	void OnTriggerExit (const TriggerExitEvent&  event);

	void OnCollisionEnter(const CollisionEnterEvent& event);
	void OnCollisionStay (const CollisionStayEvent&  event);
	void OnCollisionExit (const CollisionExitEvent&  event);

	bool TryGetEntityWorldPickingBox(unsigned int entity, OBB& outBox);
	bool TryFindClosestEntityHitByRay(const Ray& ray, unsigned int& outEntityID, float& outDistance);

};

}