#pragma once

#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A soft spot of light that always faces the camera: glowing eyes, a lantern's halo, the heart of a spell
//
// How to use it: Add Component > Glow Sprite on an entity (make it a child of a character's bone for eyes that glow:
// one entity per eye, a few centimeters in front of the head bone). Its light is added to the picture, so with Bloom
// on (Environment) a bright one shines. A script turns it on and off: entity.glowSprite.isVisible = true,
// or fades it with entity.glowSprite.intensity
struct GlowSpriteComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxSize      = 20.0f;
	static constexpr float MaxIntensity = 50.0f;

	bool isVisible = true;

	glm::vec3 color = glm::vec3(1.0f);

	// The width of the spot (meters), and how bright it is (above 1 it glows with Bloom)
	float size      = 0.1f;
	float intensity = 4.0f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<GlowSpriteComponent>("Glow Sprite", EngineAssets::Icons::Particle,
			{
				ADD_PROPERTY(GlowSpriteComponent, isVisible, PropertyDataType::Bool),
				ADD_PROPERTY(GlowSpriteComponent, color,     PropertyDataType::Color),
				ADD_PROPERTY_RANGED(GlowSpriteComponent, size,      PropertyDataType::Float, 0.0f, MaxSize),
				ADD_PROPERTY_RANGED(GlowSpriteComponent, intensity, PropertyDataType::Float, 0.0f, MaxIntensity)
			});
	}
};

}
