#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;
struct SceneData;
struct PointLightData;

// Plays effects by name and draws their shapes (see EffectShapeComponent)
//
// An effect is an entity with an Effect Shape and/or a Particle Emitter, and all its children with them: playing it
// at a point adds one shape of each and bursts every emitter there. Everything that names a "hit effect" (weapons,
// spells, swarms, scripts) plays effects this way, so a hit can be a whole composition: a slash, a ring of shock,
// sparks, smoke and a flash of light, set up in the scene, not in code
class EffectSystem
{
public:
	// The effect named 'name' at 'position'. 'direction' (along the ground) is where it faces: e.g. from the attacker
	// to the one hit; zero = the way the effect's entity faces. 'count' specks for the emitters that don't say their
	// own 'burstAmount'. False when nothing of that name exists
	static bool Play(EntityManager& entityManager, const std::string& name, const glm::vec3& position,
		const glm::vec3& direction = glm::vec3(0.0f), int count = 20);

	// Call once per frame while the game plays: the shapes age and die
	static void Update(EntityManager& entityManager, float deltaTime);

	// Call after the solid things (the shapes are light added to the picture, hidden behind walls)
	static void Draw(EntityManager& entityManager, const SceneData& sceneData, float seconds);

	// The flashes of light of the living shapes, added to the scene's point lights (up to 'maxLights' in all)
	static void AddLights(EntityManager& entityManager, std::vector<PointLightData>& pointLights, size_t maxLights);

	// When the game starts and stops: no shapes left
	static void Clear(EntityManager& entityManager);
};

}
