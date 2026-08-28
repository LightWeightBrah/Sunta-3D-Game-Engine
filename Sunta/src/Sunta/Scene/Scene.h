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

	EntityManager& GetEntityManager()		       { return entityManager; }
											  
	const std::string& GetName()             const { return name; }
	const std::string& GetFilePath()         const { return filePath; }
	void SetName(const std::string& name)	       { this->name = name; }
	void SetFilePath(const std::string& path)      { this->filePath = path; }

private:
	EntityManager		entityManager;
	Camera				camera;

	unsigned int		resizeEventID;
	std::string			name = "Untitled_Scene";
	std::string			filePath;
};

}