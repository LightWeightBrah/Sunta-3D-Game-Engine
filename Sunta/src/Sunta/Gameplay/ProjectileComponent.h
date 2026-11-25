#pragma once

#include <string>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// What a spell throws: a ball of fire, a bolt, or a pillar of flame bursting from the ground (see SpellCasterComponent)
//
// How to use it: Add Component > Projectile on an entity of the scene that stands for every projectile of its kind
// (a template, like the hit effects: it is never moved itself). A Spell Caster names it. A projectile is a moving point
// that leaves the specks of the emitter named 'trailEffect' behind it: give the scene an entity with a Particle Emitter
// (not emitting) for its look. It hurts the first character of the other side it touches (a roll goes through it,
// a shield blocks it, a parry can't turn it away) and bursts on walls
//
// A projectile can have a body: give the template child entities with a Model (a thrown bottle, an arrow, a skull),
// a Liquid Orb (a ball of water) or a Glow Sprite (a spot of light).
// Each projectile in the air borrows one of them, carries it along its way (the front of the model first, tumbling
// with 'spinDegreesPerSecond') and gives it back when it is gone, so as many bodies can be seen at once as the
// template has children; a projectile that finds none free flies with its trail only. Don't rotate or scale the
// template itself (scale the children)
//
// With 'fuseSeconds' set it doesn't burst on contact: it waits (its trail marks the place, the player has time to run),
// then bursts once and hurts everyone of the other side within 'radius' (a pillar of fire under the player)
struct ProjectileComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxSpeed   = 60.0f;
	static constexpr float MaxDamage  = 1000.0f;
	static constexpr float MaxRadius  = 10.0f;
	static constexpr float MaxSeconds = 20.0f;
	static constexpr float MaxTurn    = 720.0f;
	static constexpr float MaxTrail   = 1000.0f;
	static constexpr float MaxSpin    = 2000.0f;

	// How fast it flies (meters per second), 0 = stays where it appeared
	float speed = 10.0f;

	float damage      = 25.0f;
	float poiseDamage = 20.0f;

	// How close to a character it hurts it (meters), the reach of the burst for a projectile with a fuse
	float radius = 0.35f;

	// How fast it turns after its target (degrees per second): a little homing makes a side step necessary,
	// too much makes it impossible to dodge. 0 = flies straight
	float homingDegreesPerSecond = 0.0f;

	// Gone after this long, if it hit nothing (seconds)
	float lifetimeSeconds = 4.0f;

	// Above 0: it bursts after this long (seconds) instead of on contact, see above
	float fuseSeconds = 0.0f;

	// The look: specks of the emitter named 'trailEffect' thrown along its way (per second)
	std::string trailEffect;
	float       trailParticlesPerSecond = 120.0f;

	// How fast its body (a child with a Model, see above) tumbles end over end as it flies (degrees per second)
	float spinDegreesPerSecond = 0.0f;

	// Above 0: it first hangs where it appeared for this long (its body growing from nothing, its trail gathering
	// around it), hurting nobody, and only then flies, at where its target is by then (a sky filling with missiles
	// before they all come down)
	float launchDelaySeconds = 0.0f;

	// How long its body takes to grow to its size while it hangs (seconds, 0 = all of launchDelaySeconds): a sky that
	// is full at once and stays full
	float formSeconds = 0.0f;

	// After hanging (launchDelaySeconds): on, it turns to where its target is by then; off, it flies the way it was
	// launched (a rain falling straight down around the player, not all of it at them)
	bool aimsAfterForming = true;

	// The body turns to face its flight (an arrow, a thrown bottle). Off: it keeps the rotation the scene gives it
	// (something enormous lowered upright onto the enemy)
	bool bodyFacesFlight = true;

	// The body spins around its flight (a thrown star, its face towards where it flies) instead of tumbling end
	// over end. Only with bodyFacesFlight
	bool spinsAroundFlight = false;

	// The burst where it lands: specks of the emitter named 'hitEffect', a sound
	std::string hitEffect;
	int         hitBurst = 60;
	std::string hitSound;

	// How a hit on the player feels: the game's time stands still for a moment (seconds) and the camera shakes (0..1)
	float hitStopSeconds = 0.05f;
	float cameraShake    = 0.3f;

	// Filled while the game plays (not saved): how many of its projectiles burst so far, and where the last one did
	// (a script answers a landing with its own show, e.g. a falling flask bursting into a pool)
	mutable int       timesBurst        = 0;
	mutable glm::vec3 lastBurstPosition = glm::vec3(0.0f);

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<ProjectileComponent>("Projectile", EngineAssets::Icons::Particle,
			{
				ADD_PROPERTY_RANGED(ProjectileComponent, speed,                   PropertyDataType::Float, 0.0f, MaxSpeed),
				ADD_PROPERTY_RANGED(ProjectileComponent, damage,                  PropertyDataType::Float, 0.0f, MaxDamage),
				ADD_PROPERTY_RANGED(ProjectileComponent, poiseDamage,             PropertyDataType::Float, 0.0f, MaxDamage),
				ADD_PROPERTY_RANGED(ProjectileComponent, radius,                  PropertyDataType::Float, 0.0f, MaxRadius),
				ADD_PROPERTY_RANGED(ProjectileComponent, homingDegreesPerSecond,  PropertyDataType::Float, 0.0f, MaxTurn),
				ADD_PROPERTY_RANGED(ProjectileComponent, lifetimeSeconds,         PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY_RANGED(ProjectileComponent, fuseSeconds,             PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY(ProjectileComponent, trailEffect, PropertyDataType::String),
				ADD_PROPERTY_RANGED(ProjectileComponent, trailParticlesPerSecond, PropertyDataType::Float, 0.0f, MaxTrail),
				ADD_PROPERTY_RANGED(ProjectileComponent, spinDegreesPerSecond,    PropertyDataType::Float, 0.0f, MaxSpin),
				ADD_PROPERTY_RANGED(ProjectileComponent, launchDelaySeconds,      PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY_RANGED(ProjectileComponent, formSeconds,             PropertyDataType::Float, 0.0f, MaxSeconds),
				ADD_PROPERTY(ProjectileComponent, aimsAfterForming, PropertyDataType::Bool),
				ADD_PROPERTY(ProjectileComponent, bodyFacesFlight, PropertyDataType::Bool),
				ADD_PROPERTY(ProjectileComponent, spinsAroundFlight, PropertyDataType::Bool),
				ADD_PROPERTY(ProjectileComponent, hitEffect, PropertyDataType::String),
				ADD_PROPERTY(ProjectileComponent, hitBurst,  PropertyDataType::Int),
				ADD_PROPERTY(ProjectileComponent, hitSound,  PropertyDataType::String),
				ADD_PROPERTY_RANGED(ProjectileComponent, hitStopSeconds,          PropertyDataType::Float, 0.0f, 0.3f),
				ADD_PROPERTY_RANGED(ProjectileComponent, cameraShake,             PropertyDataType::Float, 0.0f, 1.0f)
			});
	}
};

}
