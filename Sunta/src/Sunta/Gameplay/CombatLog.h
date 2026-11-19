#pragma once
#include <string>

#include "Physics/CollisionShapes.h"

namespace Sunta
{
	class EntityManager;

	// A record of every swing for tuning and automated tests: whether it landed, how close the weapon came to the
	// target's body and at which height of it (0 = feet, 1 = top of the body box), the animation, the distance between
	// the characters. Written to <SUNTA_TEST_OUT>/combat_log.csv when SUNTA_COMBAT_LOG=1 or an input script plays.
	class CombatLog
	{
	public:
		static bool IsOn();

		static void Advance(float deltaTime);

		// Every frame of a swing's hit window, for each possible target
		static void TrackSwing(EntityManager& entityManager, unsigned int ownerID, unsigned int attackNumber,
			unsigned int targetID, unsigned int weaponID, const OBB& weapon, const OBB& targetBody);

		// What a swing that touched the target came to (result: "hurt", "blocked", "parried", "riposte", "missed")
		static void RecordHit(EntityManager& entityManager, unsigned int ownerID, unsigned int attackNumber,
			unsigned int targetID, const char* result, float damage, const glm::vec3& contactPoint, const OBB& targetBody);

		// With SUNTA_COMBAT_TRACE=1: every frame of every attack, where each hitbox of the attacker is (height above its
		// feet, reach in front of it) and the attack's progress - to find when and how high a blow really strikes
		static bool IsTracing();
		static void Trace(EntityManager& entityManager, unsigned int ownerID, unsigned int weaponID, const OBB& weapon,
			float progress, bool isInHitWindow);

		// A free-form event line (phase changes, spells, deaths)
		static void Event(const std::string& text);

		static void Flush();
	};
}
