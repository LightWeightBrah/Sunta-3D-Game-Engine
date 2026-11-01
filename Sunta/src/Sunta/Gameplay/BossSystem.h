#pragma once

namespace Sunta
{

class EntityManager;

// Boss fights: the fog gates of the arenas (FogGateComponent) and the bosses in them (BossComponent)
class BossSystem
{
public:
	// Call when the game starts, AFTER the enemies remembered their homes: every fog is open, every boss rests
	static void PrepareForPlay(EntityManager& entityManager);

	// Call once per frame while the game plays, AFTER the Combat (a boss may have just died)
	static void Update(EntityManager& entityManager);

	// False for a boss that was defeated: it is not brought back with the other enemies
	static bool CanBeBroughtBack(EntityManager& entityManager, unsigned int enemyID);
};

}
