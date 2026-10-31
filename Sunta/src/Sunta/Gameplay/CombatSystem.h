#pragma once

namespace Sunta
{

class EntityManager;
struct HitboxComponent;

// Weapons hitting characters: Hitbox against Health
class CombatSystem
{
public:
	// Call when the game starts: everyone is alive with full health
	static void PrepareForPlay(EntityManager& entityManager);

	// Call once per frame AFTER the transforms (the weapon must be where the hand holds it in this frame)
	static void Update(EntityManager& entityManager, float deltaTime);

	// Hurts the entity (also usable from scripts, e.g. a trap). Nothing happens to a dead or invulnerable (rolling) target
	// Returns true when the damage landed. 'attackerEntityID' is only for the log
	static bool ApplyDamage(EntityManager& entityManager, unsigned int targetEntityID, float damage, unsigned int attackerEntityID);

	// True for a moment after the weapon hurt someone (for drawing the box in another color)
	static bool HasJustHit(const HitboxComponent& hitbox);

	// Call when the game starts: explains in the log why a weapon would never hurt anyone
	static void ReportProblemsBeforePlay(EntityManager& entityManager);
};

}
