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

	int       GetEntityUnderMouse       (const glm::vec2& mousePosition, float viewportWidth, float viewportHeight);
	glm::vec3 GetWorldPositionUnderMouse(const glm::vec2& mousePosition, float viewportWidth, float viewportHeight);

	EntityManager& GetEntityManager()		       { return entityManager; }

	const Camera&  GetCamera()               const { return camera; }
	const std::string& GetName()             const { return name; }
	const std::string& GetFilePath()         const { return filePath; }
	void SetName(const std::string& name)	       { this->name = name; }
	void SetFilePath(const std::string& path)      { this->filePath = path; }

private:
	EntityManager		entityManager;
	Camera				camera;

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