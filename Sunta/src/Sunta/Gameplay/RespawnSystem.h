#pragma once

#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;

// Brings characters back: the player after death (see RespawnComponent), the enemies when the player rests or dies
class RespawnSystem
{
public:
	// Call when the game starts, AFTER the enemies remembered their homes: the player comes back where it starts until it rests
	static void PrepareForPlay(EntityManager& entityManager);

	// Call once per frame while the game plays: counts the time since the player died and brings it back
	static void Update(EntityManager& entityManager, float deltaTime);

	// Where the player comes back after the next death
	static void SetRespawnPoint(EntityManager& entityManager, unsigned int playerID, const glm::vec3& position, float yawDegrees);

	// Full health and stamina (resting at a bonfire)
	static void RestoreCharacter(EntityManager& entityManager, unsigned int entityID);

	// Every enemy alive again, at the place and turned the way it started, with its state machine from the beginning
	// (a defeated boss stays dead)
	static void RespawnEnemies(EntityManager& entityManager);
};

}
