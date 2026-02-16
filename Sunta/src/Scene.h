#pragma once
#include <memory>

#include "Camera.h"

class Shader;
class Renderer;
class Entity;
class LightSource;

class Scene
{
private:
	std::unique_ptr<Entity>			entity;
	std::unique_ptr<LightSource>	lightSource;

	Camera							camera;

	//Solaire	  solaireEntity;
	//Astar       aStar;

public:
	Scene();
	~Scene();

	void Init(float windowWidth, float windowHeight);
	void OnWindowResize(float windowWidth, float windowHeight);
	void ProcessInput();
	void Update();
	void Render(Renderer& renderer);
	void Clear();
	
};