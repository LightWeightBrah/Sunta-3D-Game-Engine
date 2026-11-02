#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// One glowing speck of an emitter, in world space (it stays where it flew, the emitter may move on)
struct Particle
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 velocity = glm::vec3(0.0f);
	float     age      = 0.0f;
	float     lifetime = 1.0f;
};

// Throws out small glowing specks: embers, sparks, ash, magic. They shine (their color is added to the picture,
// so many of them together get brighter) and face the camera
//
// How to use it: Add Component > Particle Emitter on any entity (a child of a character on a bone, e.g. 'Chest',
// makes the specks rise from it). Each speck is born somewhere in the box 'spawnHalfExtents' around the entity,
// flies with 'velocity' (plus a random part up to 'velocityRandomness' in every direction), is pulled by 'gravity'
// (e.g. -9.8 down for sparks, a little up for embers), and over its 'lifetimeSeconds' turns from 'startColor'
// and 'startSize' to 'endColor' and 'endSize' (a black end color fades it out)
// 'isEmitting' off stops new specks (the ones in the air live on). A script: this.particles.isEmitting = true,
// this.particles:burst(50) throws 50 at once (a hit, an explosion)
struct ParticleEmitterComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxRate      = 500.0f;
	static constexpr float MaxLifetime  = 20.0f;
	static constexpr float MaxSize      = 5.0f;
	static constexpr float MaxSpeed     = 50.0f;
	static constexpr float MaxIntensity = 20.0f;
	static constexpr int   MaxCount     = 5000;

	bool  isEmitting        = true;
	float particlesPerSecond = 30.0f;
	float lifetimeSeconds    = 1.5f;

	glm::vec3 spawnHalfExtents   = glm::vec3(0.2f);
	glm::vec3 velocity           = glm::vec3(0.0f, 1.0f, 0.0f);
	float     velocityRandomness = 0.5f;
	glm::vec3 gravity            = glm::vec3(0.0f);

	float     startSize  = 0.12f;
	float     endSize    = 0.02f;
	glm::vec3 startColor = glm::vec3(1.0f, 0.55f, 0.15f);
	glm::vec3 endColor   = glm::vec3(0.3f, 0.02f, 0.0f);

	// How bright the colors shine (above 1 the specks glow strongly)
	float intensity = 1.5f;

	// No more specks than this at once (the oldest ones go first)
	int maxParticles = 400;

	// ========== Filled while the game plays, not saved with the scene ==========

	std::vector<Particle> particles;

	// Part of a speck not born yet (a rate of 30 per second at 60 frames per second is half a speck per frame)
	float unbornParticles = 0.0f;

	// Specks asked for by burst(), born in the next frame
	int burstCount = 0;

	// Bursts somewhere else than the emitter (sparks where a sword hit): born around that point in the next frame
	struct PointBurst
	{
		glm::vec3 position;
		int       count;
	};
	std::vector<PointBurst> pointBursts;

	// 'isEmitting' as set in the scene (the game, e.g. a boss in its second phase, can switch it on and back)
	bool wasEmittingAtStart = true;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<ParticleEmitterComponent>("Particle Emitter", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY(ParticleEmitterComponent, isEmitting, PropertyDataType::Bool),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, particlesPerSecond, PropertyDataType::Float, 0.0f, MaxRate),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, lifetimeSeconds,    PropertyDataType::Float, 0.05f, MaxLifetime),
				ADD_PROPERTY(ParticleEmitterComponent, spawnHalfExtents, PropertyDataType::Float3),
				ADD_PROPERTY(ParticleEmitterComponent, velocity,         PropertyDataType::Float3),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, velocityRandomness, PropertyDataType::Float, 0.0f, MaxSpeed),
				ADD_PROPERTY(ParticleEmitterComponent, gravity,          PropertyDataType::Float3),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, startSize, PropertyDataType::Float, 0.0f, MaxSize),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, endSize,   PropertyDataType::Float, 0.0f, MaxSize),
				ADD_PROPERTY(ParticleEmitterComponent, startColor, PropertyDataType::Color),
				ADD_PROPERTY(ParticleEmitterComponent, endColor,   PropertyDataType::Color),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, intensity, PropertyDataType::Float, 0.0f, MaxIntensity),
				ADD_PROPERTY(ParticleEmitterComponent, maxParticles, PropertyDataType::Int)
			});
	}
};

}
