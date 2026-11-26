#pragma once

namespace Sunta
{

class EntityManager;

// The ground of open worlds (see TerrainComponent): reads the heightmaps, answers how high the ground is, keeps
// every body standing on it
class TerrainSystem
{
public:
	// Call when the game starts: reads the heightmaps
	static void PrepareForPlay(EntityManager& entityManager);

	// After the collisions of a physics step: a body below the ground is put on it (and stops falling)
	static void KeepBodiesOnGround(EntityManager& entityManager);

	// The height of the ground at (x, z) in the world; false where no terrain lies
	static bool TryGetHeight(EntityManager& entityManager, float x, float z, float& outHeight);
};

}
