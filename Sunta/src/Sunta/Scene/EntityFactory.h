#pragma once
#include <string>
#include <glm/glm.hpp>

#include "Core/EngineAssets.h"

namespace Sunta
{

class Scene;
class Material;

class EntityFactory
{
public:
	static unsigned int CreateEmpty           (Scene& scene, const glm::vec3& position = glm::vec3(0.0f), const std::string& name = "Empty Entity");
									          
	static unsigned int CreateModel           (Scene& scene, const glm::vec3& position = glm::vec3(0.0f), const std::string& name = "Model",			 const std::string&		   modelName	  = EngineAssets::Models::Solaire);
	static unsigned int CreateCube            (Scene& scene, const glm::vec3& position = glm::vec3(0.0f), const std::string& name = "Cube",              std::shared_ptr<Material> customMaterial = nullptr);
	static unsigned int CreatePyramid         (Scene& scene, const glm::vec3& position = glm::vec3(0.0f), const std::string& name = "Pyramid",           std::shared_ptr<Material> customMaterial = nullptr);
	static unsigned int CreateCone            (Scene& scene, const glm::vec3& position = glm::vec3(0.0f), const std::string& name = "Cone",              std::shared_ptr<Material> customMaterial = nullptr);
	static unsigned int CreateSphere          (Scene& scene, const glm::vec3& position = glm::vec3(0.0f), const std::string& name = "Sphere",            std::shared_ptr<Material> customMaterial = nullptr);
	static unsigned int CreateCapsule         (Scene& scene, const glm::vec3& position = glm::vec3(0.0f), const std::string& name = "Capsule",           std::shared_ptr<Material> customMaterial = nullptr);
	
	static unsigned int CreateDirectionalLight(Scene& scene, const glm::vec3& position = glm::vec3(0.0f), const std::string& name = "Directional Light", std::shared_ptr<Material> customMaterial = nullptr);
	static unsigned int CreatePointLight      (Scene& scene, const glm::vec3& position = glm::vec3(0.0f), const std::string& name = "Point Light",       std::shared_ptr<Material> customMaterial = nullptr);
	static unsigned int CreateSpotLight       (Scene& scene, const glm::vec3& position = glm::vec3(0.0f), const std::string& name = "Spot Light",        std::shared_ptr<Material> customMaterial = nullptr);

};

}