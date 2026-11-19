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
	float     size     = 1.0f;    // times the emitter's size
	float     rotation = 0.0f;    // radians, in the picture
	float     spin     = 0.0f;    // radians per second
	float     seed     = 0.0f;    // 0..1: the shape of its flame or puff
};

// How a speck looks: a soft spot of light; a tongue of fire (ragged, a hot white-yellow core, its own shape for each);
// a puff of smoke (ragged, it covers what is behind it); a spark (a streak of light stretched along its flight, longer
// the faster it flies: the sparks of a sword on armor, of an explosion); a ring (a thin circle of light swelling out,
// facing the camera: the shock of a blow)
enum class ParticleStyle : int
{
	Glow  = 0,
	Flame = 1,
	Smoke = 2,
	Spark = 3,
	Ring  = 4
};

// Where the specks are born around the emitter: in a box; in a ball; on a dome (the upper half of a sphere's shell,
// e.g. a hill of fire swelling out of the ground)
enum class ParticleSpawnShape : int
{
	Box    = 0,
	Sphere = 1,
	Dome   = 2
};

// Throws out small glowing specks: embers, sparks, ash, magic. They shine (their color is added to the picture,
// so many of them together get brighter) and face the camera
//
// How to use it: Add Component > Particle Emitter on any entity (a child of a character on a bone, e.g. 'Chest',
// makes the specks rise from it). Each speck is born somewhere in the box 'spawnHalfExtents' around the entity,
// flies with 'velocity' (plus a random part up to 'velocityRandomness' in every direction), is pulled by 'gravity'
// (e.g. -9.8 down for sparks, a little up for embers), and over its 'lifetimeSeconds' turns from 'startColor'
// and 'startSize' to 'endColor' and 'endSize' (a black end color fades it out)
// 'style' gives them the look of fire or smoke instead of a spot of light; 'spawnShape' a ball or a dome of 'spawnRadius'
// instead of the box, from which they fly out at 'outwardSpeed'. Tens of thousands at once are fine (drawn together)
// 'drag' makes a fast burst hang in the air; 'hasMiddleColor' a three-color fade (hot, burning, ash); 'style' Spark
// streaks along the flight ('stretch'), Ring a swelling circle of shock
// 'isEmitting' off stops new specks (the ones in the air live on). A script: this.particles.isEmitting = true,
// this.particles:burst(50) throws 50 at once (a hit, an explosion)
struct ParticleEmitterComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxRate      = 40000.0f;
	static constexpr float MaxLifetime  = 20.0f;
	static constexpr float MaxSize      = 20.0f;
	static constexpr float MaxSpeed     = 50.0f;
	static constexpr float MaxIntensity = 20.0f;
	static constexpr int   MaxCount     = 80000;
	static constexpr float MaxRadius    = 200.0f;
	static constexpr float MaxSpin      = 720.0f;
	static constexpr float MaxDrag      = 20.0f;
	static constexpr float MaxStretch   = 1.0f;

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

	// Dark specks cover what is behind them instead of adding light to it: smoke, ash, a swarm of black shards
	// (their colors are then the colors themselves, not light)
	bool isDark = false;

	int   style        = (int)ParticleStyle::Glow;
	int   spawnShape   = (int)ParticleSpawnShape::Box;
	float spawnRadius  = 1.0f;
	float outwardSpeed = 0.0f;

	// Some bigger, some smaller (a part of the size), and turning in the picture (degrees per second, each its own way)
	float sizeRandomness       = 0.0f;
	float spinDegreesPerSecond = 0.0f;

	// How quickly the air slows the specks (a part of their speed lost each second): a burst that shoots out fast
	// and hangs in the air instead of flying on evenly (what makes an explosion look like one)
	float drag = 0.0f;

	// A spark is this many seconds of its flight long (its streak), at least its size
	float stretch = 0.04f;

	// A third color in the middle of a speck's life: white-hot -> orange -> dark red, instead of only two
	bool      hasMiddleColor = false;
	glm::vec3 middleColor    = glm::vec3(1.0f, 0.3f, 0.05f);

	// How many specks it throws when it is a part of a bigger effect that is played (see ParticleSystem::BurstAt:
	// the children of the played entity burst too). 0: as many as asked of the whole effect
	int burstAmount = 0;

	// A flipbook: a picture of 'flipbookColumns' x 'flipbookRows' frames (a baked simulation of smoke, fire or
	// water, left to right, top to bottom) drawn on every speck instead of the style's own shape, played over its
	// life 'flipbookLoops' times; each speck starts at its own frame when 'flipbookRandomStart' is on. Its colors
	// are multiplied by the speck's color (a white flipbook takes the emitter's colors). Empty: the style's shape
	std::string flipbookTexture;
	int   flipbookColumns     = 8;
	int   flipbookRows        = 8;
	float flipbookLoops       = 1.0f;
	bool  flipbookRandomStart = false;

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

	// Where the next speck goes when there are already 'maxParticles' (over the oldest ones, in turn)
	size_t nextOverwrite = 0;

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
				ADD_PROPERTY(ParticleEmitterComponent, maxParticles, PropertyDataType::Int),
				ADD_PROPERTY(ParticleEmitterComponent, isDark, PropertyDataType::Bool),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, style,      PropertyDataType::Int, 0, 4),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, spawnShape, PropertyDataType::Int, 0, 2),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, spawnRadius,  PropertyDataType::Float, 0.0f, MaxRadius),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, outwardSpeed, PropertyDataType::Float, -MaxSpeed, MaxSpeed),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, sizeRandomness,       PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, spinDegreesPerSecond, PropertyDataType::Float, 0.0f, MaxSpin),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, drag,    PropertyDataType::Float, 0.0f, MaxDrag),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, stretch, PropertyDataType::Float, 0.0f, MaxStretch),
				ADD_PROPERTY(ParticleEmitterComponent, hasMiddleColor, PropertyDataType::Bool),
				ADD_PROPERTY(ParticleEmitterComponent, middleColor,    PropertyDataType::Color),
				ADD_PROPERTY(ParticleEmitterComponent, burstAmount,    PropertyDataType::Int),
				ADD_PROPERTY(ParticleEmitterComponent, flipbookTexture, PropertyDataType::String),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, flipbookColumns, PropertyDataType::Int, 1, 16),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, flipbookRows,    PropertyDataType::Int, 1, 16),
				ADD_PROPERTY_RANGED(ParticleEmitterComponent, flipbookLoops,   PropertyDataType::Float, 0.0f, 10.0f),
				ADD_PROPERTY(ParticleEmitterComponent, flipbookRandomStart, PropertyDataType::Bool)
			});
	}
};

}
