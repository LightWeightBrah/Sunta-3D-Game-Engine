#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;
class Mesh;
class Shader;

// Draws every Liquid Orb (see LiquidOrbComponent): a sphere whose skin shows the sky bent through it and mirrored on it
class LiquidOrbRenderer
{
public:
	// Call after everything solid was drawn (the orbs are a little see-through). 'skyboxFolder' is the sky they bend
	// (the Environment's), 'seconds' the time their skin wobbles with
	static void Draw(EntityManager& entityManager, const glm::mat4& view, const glm::mat4& projection,
		const glm::vec3& cameraPosition, const std::string& skyboxFolder, float skyBrightness, float seconds);

private:
	static inline std::unique_ptr<Mesh>   sphere;
	static inline std::shared_ptr<Shader> shader;

	static void EnsureInitialized();
};

}
