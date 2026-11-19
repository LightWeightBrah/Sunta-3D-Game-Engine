#pragma once

namespace Sunta
{
	class EntityManager;

	// Bends the spine of attacking characters towards their enemy (see StrikeAimComponent)
	class StrikeAimSystem
	{
	public:
		// Every frame of play, before the animators
		static void Update(EntityManager& entityManager, float deltaTime);
	};
}
