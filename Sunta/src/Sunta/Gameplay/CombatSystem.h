#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "HealthComponent.h"

namespace Sunta
{

class EntityManager;
struct HitboxComponent;

// One loss of health, kept until the scripts have heard of it (the Lua callback OnDamaged)
struct DamageEvent
{
	unsigned int targetEntityID   = 0;
	unsigned int attackerEntityID = 0;
	float        damage           = 0.0f;
	DamageKind   kind             = DamageKind::Other;
};

// Weapons hitting characters: Hitbox against Health, through the defences of the target
// (a parry turns the attack away, a Shield takes it while blocking, Poise keeps the target on its feet)
class CombatSystem
{
public:
	// Fills the posture of a character that has a Posture component (see PostureComponent); true when it broke
	static bool FillPosture(EntityManager& entityManager, unsigned int entityID, float amount);

	// Call when the game starts: everyone is alive with full health
	static void PrepareForPlay(EntityManager& entityManager);

	// Call once per frame AFTER the transforms (the weapon must be where the hand holds it in this frame)
	static void Update(EntityManager& entityManager, float deltaTime);

	// Hurts the entity (also usable from scripts, e.g. a trap): no shield or parry stops it and it always staggers
	// Nothing happens to a dead or invulnerable (rolling) target
	// Returns true when the damage landed. 'attackerEntityID' is only for the log
	static bool ApplyDamage(EntityManager& entityManager, unsigned int targetEntityID, float damage, unsigned int attackerEntityID);

	// A spell's projectile (see ProjectileComponent) touching the target at 'point': through its defences like a weapon,
	// except that a parry can't turn it away. A hit on the player stops the game's time for 'hitStopSeconds' and shakes
	// the camera (0..1). False when it couldn't land (the target is dead or rolling): the projectile flies on
	static bool HitWithSpell(EntityManager& entityManager, unsigned int targetEntityID, float damage, float poiseDamage,
		unsigned int casterEntityID, const glm::vec3& point, float hitStopSeconds, float cameraShake);

	// Everyone hurt since the last call, in the order it happened (the list is emptied): for the scripts' OnDamaged
	static std::vector<DamageEvent> TakeDamageEvents();

	// True for a moment after the weapon hurt someone (for drawing the box in another color)
	static bool HasJustHit(const HitboxComponent& hitbox);

	// Call when the game starts: explains in the log why a weapon would never hurt anyone
	static void ReportProblemsBeforePlay(EntityManager& entityManager);
};

}
