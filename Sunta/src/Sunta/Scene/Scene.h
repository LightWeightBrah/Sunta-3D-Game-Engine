#pragma once
#include <memory>

#include "Camera.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

class Renderer;
class RendererDevice;

class Scene
{
public:
	Scene();
	~Scene();

	void Init(RendererDevice& rendererDevice, float windowWidth, float windowHeight);
	void OnWindowResize(float windowWidth, float windowHeight);
	void ProcessInput();
	void Update();
	void Render(Renderer& renderer);
	void Clear();

	EntityManager& GetEntityManager() { return entityManager; }

private:
	EntityManager							entityManager;
	Camera									camera;

	unsigned int resizeEventID;
	
	//Solaire	  solaireEntity;
	//Astar       aStar;
};

}