#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;

// Spells: the casts of every Spell Caster (see SpellCasterComponent) and the flight of their projectiles
// (see ProjectileComponent). The projectiles are not entities: each is a point that moves, leaves the specks of its
// trail and hurts what it touches
class SpellSystem
{
public:
	// Call when the game starts and stops: no projectile in the air, every spell ready
	static void Clear(EntityManager& entityManager);

	// Call once per frame while the game plays, AFTER the transforms (a spell leaves from where the hand is in this frame)
	static void Update(EntityManager& entityManager, float deltaTime);

	// Starts a spell of the character (one of its Spell Caster children) that is ready and fits the distance to its
	// target. False when there is none, or the character can't act now
	static bool TryCastSpell(EntityManager& entityManager, unsigned int casterEntityID, float distanceToTarget);

	// Starts this very spell (the entity with the Spell Caster), whatever the distance to the target: for scripts and
	// for the player's key. False when it is not ready, or its caster can't act now
	static bool TryCast(EntityManager& entityManager, unsigned int spellEntityID);

	// Throws one projectile without any cast (a script's own patterns: a ring of fire around a boss, a rain over the
	// arena): the Projectile template named 'projectileName' leaves 'from' towards 'direction' on the side of
	// 'casterEntityID', homing (if it does) on that character's target. False when there is no such template
	static bool Launch(EntityManager& entityManager, const std::string& projectileName, unsigned int casterEntityID,
		const glm::vec3& from, const glm::vec3& direction);

	// True when a flying projectile of the other side would reach the character within 'withinSeconds' (an enemy
	// dodging a spell). 'outProjectileNumber' tells the projectiles apart: each has its own
	static bool TryFindIncomingProjectile(EntityManager& entityManager, unsigned int entityID, float withinSeconds,
		unsigned int& outProjectileNumber);

	// True when the character has a spell ready that fits the distance (an enemy decides to cast instead of walking up)
	static bool HasSpellReady(EntityManager& entityManager, unsigned int casterEntityID, float distanceToTarget);

	// Call when the game starts: explains in the log why a spell would never be cast
	static void ReportProblemsBeforePlay(EntityManager& entityManager);
};

}
