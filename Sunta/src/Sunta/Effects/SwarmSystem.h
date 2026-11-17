#pragma once

#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;
struct SceneData;
struct SwarmComponent;

// Moves and draws every Swarm (see SwarmComponent): thousands of copies of a model, all drawn at once
//
// Each copy is a point with a few numbers of its own (where on the sphere, where in the sky, its size, its turn, when
// it leaves); its place is worked out every frame from the stage the swarm is at, so no copy is an entity. The falling
// ones leave a streak of light behind them (a second instanced draw, added to the picture)
class SwarmSystem
{
public:
	// Call once per frame while the game plays, after the transforms (the swarm is born where its entity is)
	static void Update(EntityManager& entityManager, float deltaTime);

	// Call with the solid models (the copies are solid), before the particles; the streaks are added to the picture
	static void Draw(EntityManager& entityManager, const SceneData& sceneData);

	// When the game starts and stops: every swarm back to its start
	static void Clear(EntityManager& entityManager);

	// The copies come down, one after another (see SwarmComponent)
	static void Fall(SwarmComponent& swarm);
	static void Reset(SwarmComponent& swarm);
};

}
