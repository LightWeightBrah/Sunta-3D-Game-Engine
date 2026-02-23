#pragma once
#include <memory>

#include "Camera.h"
#include "EntityManager.h"

namespace Sunta
{

class Shader;
class Renderer;
class SceneData;

class Scene
{
public:
	Scene();
	~Scene();

	void Init(float windowWidth, float windowHeight);
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