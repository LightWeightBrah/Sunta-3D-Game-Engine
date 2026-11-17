#pragma once

#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// A ball of clear liquid (water, oil, a crystal ball): it bends the sky seen through it, mirrors it on its skin,
// shines along its rim and wobbles slowly like a drop held in the air
//
// How to use it: Add Component > Liquid Orb on an entity. Its Transform's scale is its size (1 = one meter across).
// It reads the sky of the scene's Environment, so give the scene a skybox. As a child of a Projectile template
// it is what the spell throws (see ProjectileComponent)
struct LiquidOrbComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxBrightness = 10.0f;
	static constexpr float MaxWobble     = 0.5f;
	static constexpr float MaxSpeed      = 10.0f;

	bool isVisible = true;

	// The color of the liquid (white = clear water; baby oil is almost clear, a little warm)
	glm::vec3 tint = glm::vec3(1.0f);

	// How bright the sky inside it is (1 = as the sky itself)
	float brightness = 1.3f;

	// How far its skin moves in and out (a part of its radius) and how fast
	float wobble      = 0.05f;
	float wobbleSpeed = 2.0f;

	// How much of what is behind it shows through (0 = like a mirror ball, 1 = barely there)
	float clearness = 0.25f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<LiquidOrbComponent>("Liquid Orb", EngineAssets::Icons::Particle,
			{
				ADD_PROPERTY(LiquidOrbComponent, isVisible, PropertyDataType::Bool),
				ADD_PROPERTY(LiquidOrbComponent, tint,      PropertyDataType::Color),
				ADD_PROPERTY_RANGED(LiquidOrbComponent, brightness,  PropertyDataType::Float, 0.0f, MaxBrightness),
				ADD_PROPERTY_RANGED(LiquidOrbComponent, wobble,      PropertyDataType::Float, 0.0f, MaxWobble),
				ADD_PROPERTY_RANGED(LiquidOrbComponent, wobbleSpeed, PropertyDataType::Float, 0.0f, MaxSpeed),
				ADD_PROPERTY_RANGED(LiquidOrbComponent, clearness,   PropertyDataType::Float, 0.0f, 1.0f)
			});
	}
};

}
