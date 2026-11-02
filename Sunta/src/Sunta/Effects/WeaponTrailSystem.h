#pragma once

#include <memory>
#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;
class Shader;
class VertexArray;

// Records and draws the streaks of every Weapon Trail (see WeaponTrailComponent)
//
// Each frame the blade's base and tip are remembered; two neighbouring records make a four-cornered piece of ribbon,
// drawn like the particles: its light added to the picture, without writing depth, fading with its age
class WeaponTrailSystem
{
public:
	// Call once per frame while the game plays, after the transforms (the blade is where the hand holds it)
	static void Update(EntityManager& entityManager, float deltaTime);

	// Call after the solid models, together with the particles
	static void Draw(EntityManager& entityManager, const glm::mat4& view, const glm::mat4& projection);

	// When the game starts and stops: no streaks left over
	static void Clear(EntityManager& entityManager);

private:
	static inline std::shared_ptr<VertexArray> quad;
	static inline std::shared_ptr<Shader>      shader;

	static void EnsureInitialized();
};

}
