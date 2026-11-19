#pragma once

namespace Sunta
{
	class EntityManager;

	// Commands of an input script (see Core/InputPlayback.h) that need to know the game: a test can then reach and
	// fight an enemy that moves, instead of pressing keys blindly.
	//   lockon                  keep the player locked on to the nearest enemy (locks again when the lock is lost)
	//   unlock                  stop that and drop the lock
	//   approach DIST SECONDS   walk to the locked enemy until DIST meters from it, for SECONDS
	//   teleport X Y Z          put the player there (a test of a boss starts at its fog, not at the level's start)
	//   freeze / unfreeze       enemies stop thinking (a fixed target to compare blows) / think again
	//   fight SECONDS [DIST] [free]  lock on, stay DIST (default 1.1) meters from the enemy and attack whenever it is
	//                           in reach ("free": no lock, no walking - only the attacks)
	class TestBot
	{
	public:
		// Every frame of play while an input script plays, before the character controller
		static void Update(EntityManager& entityManager, float deltaTime);
	};
}
