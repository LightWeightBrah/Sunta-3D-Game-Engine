#pragma once
#include <string>
#include <glm/glm.hpp>

namespace Sunta
{

class Scene;
class RendererDevice;


class EntityFactory
{
public:
	static unsigned int CreateCube(Scene& scene, RendererDevice& rendererDevice, const std::string& name = "Cube", const glm::vec3& position = glm::vec3(0.0f));
	static unsigned int CreatePyramid(Scene& scene, RendererDevice& rendererDevice, const std::string& name = "Pyramid", const glm::vec3& position = glm::vec3(0.0f));
	
	static unsigned int CreateDirectionalLight(Scene& scene, RendererDevice& rendererDevice, const std::string& name = "Directional Light", const glm::vec3& position = glm::vec3(0.0f));
	static unsigned int CreatePointLight(Scene& scene, RendererDevice& rendererDevice, const std::string& name = "Point Light", const glm::vec3& position = glm::vec3(0.0f));
	static unsigned int CreateSpotLight(Scene& scene, RendererDevice& rendererDevice, const std::string& name = "Spot Light", const glm::vec3& position = glm::vec3(0.0f));

};

}