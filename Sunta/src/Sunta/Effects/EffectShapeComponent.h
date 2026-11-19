#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// The shape an effect draws in the air (all of them are made of light, eaten away by noise as they fade):
//  Ring   - a flat ring lying on the ground, swelling out (the shock of a blow into the ground, a wave of a spell)
//  Arc    - a part of a ring sweeping around in a blink, its leading edge brightest (the slash of a blade or a fist,
//           tilted by 'rotationDegrees' z: 0 a level swing, 60 a diagonal one, 90 from above)
//  Pillar - an open tube rising from the ground, twisting if wanted (a column of fire, a tornado, a beam from the sky)
//  Sphere - a ball's shell, brightest at its rim (a bubble of force, the flash of an explosion)
//  Box    - a cube of light, its edges burning, its faces faint and flowing (a block of something solid made of
//           a technique, a cage, a crystal); 'endRadius' is half its edge
enum class EffectShapeKind : int
{
	Ring   = 0,
	Arc    = 1,
	Pillar = 2,
	Sphere = 3,
	Box    = 4
};

// One shape of light played where something happens: a slash, a shockwave, a column of fire. The way the effects of
// fighting games and spells are made (meshes with scrolling noise, a white-hot core and a colored edge, eaten away
// as they die), together with Particle Emitters as children of one entity
//
// How to use it: an entity (anywhere, e.g. under the arena) with an Effect Shape, and maybe Particle Emitters and
// more Effect Shapes as its children: the whole thing is one effect. It is played by name where it should happen:
// a script calls this.world:effect("Heavy Impact", position, direction), a weapon's or a spell's 'hit effect' names
// it. Each play adds one shape at that place, facing 'direction' (level, along the ground), and bursts the children
// The shape grows from 'startRadius' to 'endRadius' over 'lifetimeSeconds' (fast at first, like a blast); its look:
// 'coreColor' (white-hot) inside, 'edgeColor' outside, 'intensity' how strongly it shines (above 1 it blooms),
// 'noiseScale' and 'scrollSpeed' the ragged pattern flowing through it, 'dissolve' how much of it noise eats away
// at its end. 'lightIntensity' above 0 also lights the world around it for its short life
struct EffectShapeComponent
{
	static constexpr float MaxLifetime  = 10.0f;
	static constexpr float MaxRadius    = 100.0f;
	static constexpr float MaxIntensity = 50.0f;
	static constexpr float MaxSpin      = 1440.0f;
	static constexpr int   MaxInstances = 256;

	int   kind            = (int)EffectShapeKind::Ring;
	float lifetimeSeconds = 0.45f;

	float startRadius = 0.3f;
	float endRadius   = 3.0f;
	// Ring and Arc: how wide the band is (a part of the radius); Pillar: how tall it is (meters)
	float thickness   = 0.35f;
	float height      = 3.0f;
	// Ring and Arc: how much of the circle (360 = a whole ring)
	float arcDegrees  = 360.0f;

	// Turned this much from the way it is played (x pitch, y yaw, z roll), each play a random part more of 'rotationRandomness'
	glm::vec3 rotationDegrees    = glm::vec3(0.0f);
	glm::vec3 rotationRandomness = glm::vec3(0.0f);
	// Moved this far from the played point (in its own turned space: z forward, y up)
	glm::vec3 offset             = glm::vec3(0.0f);

	// Turning around its own up while it lives (a spinning ring, a whirling pillar) and a twist up the pillar
	float spinDegreesPerSecond = 0.0f;
	float twistDegrees         = 0.0f;

	glm::vec3 coreColor = glm::vec3(1.0f, 0.92f, 0.75f);
	glm::vec3 edgeColor = glm::vec3(1.0f, 0.35f, 0.05f);
	float     intensity = 3.0f;

	float noiseScale  = 3.0f;
	float scrollSpeed = 1.5f;
	float dissolve    = 0.7f;

	// A flash of light on everything around it (fading with the shape), meters it reaches
	float     lightIntensity = 0.0f;
	float     lightRadius    = 8.0f;
	glm::vec3 lightColor     = glm::vec3(1.0f, 0.6f, 0.3f);

	// ========== Filled while the game plays, not saved with the scene ==========

	struct Instance
	{
		glm::mat4 matrix = glm::mat4(1.0f);   // where and how it is turned (no size: the radius is worked out from its age)
		float     age    = 0.0f;
		float     seed   = 0.0f;
	};
	std::vector<Instance> instances;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<EffectShapeComponent>("Effect Shape", EngineAssets::Icons::Bonfire,
			{
				ADD_PROPERTY_RANGED(EffectShapeComponent, kind,            PropertyDataType::Int,   0, 4),
				ADD_PROPERTY_RANGED(EffectShapeComponent, lifetimeSeconds, PropertyDataType::Float, 0.05f, MaxLifetime),
				ADD_PROPERTY_RANGED(EffectShapeComponent, startRadius,     PropertyDataType::Float, 0.0f, MaxRadius),
				ADD_PROPERTY_RANGED(EffectShapeComponent, endRadius,       PropertyDataType::Float, 0.0f, MaxRadius),
				ADD_PROPERTY_RANGED(EffectShapeComponent, thickness,       PropertyDataType::Float, 0.01f, 1.0f),
				ADD_PROPERTY_RANGED(EffectShapeComponent, height,          PropertyDataType::Float, 0.0f, MaxRadius),
				ADD_PROPERTY_RANGED(EffectShapeComponent, arcDegrees,      PropertyDataType::Float, 1.0f, 360.0f),
				ADD_PROPERTY(EffectShapeComponent, rotationDegrees,    PropertyDataType::Float3),
				ADD_PROPERTY(EffectShapeComponent, rotationRandomness, PropertyDataType::Float3),
				ADD_PROPERTY(EffectShapeComponent, offset,             PropertyDataType::Float3),
				ADD_PROPERTY_RANGED(EffectShapeComponent, spinDegreesPerSecond, PropertyDataType::Float, -MaxSpin, MaxSpin),
				ADD_PROPERTY_RANGED(EffectShapeComponent, twistDegrees,         PropertyDataType::Float, -MaxSpin, MaxSpin),
				ADD_PROPERTY(EffectShapeComponent, coreColor, PropertyDataType::Color),
				ADD_PROPERTY(EffectShapeComponent, edgeColor, PropertyDataType::Color),
				ADD_PROPERTY_RANGED(EffectShapeComponent, intensity,   PropertyDataType::Float, 0.0f, MaxIntensity),
				ADD_PROPERTY_RANGED(EffectShapeComponent, noiseScale,  PropertyDataType::Float, 0.1f, 20.0f),
				ADD_PROPERTY_RANGED(EffectShapeComponent, scrollSpeed, PropertyDataType::Float, -10.0f, 10.0f),
				ADD_PROPERTY_RANGED(EffectShapeComponent, dissolve,    PropertyDataType::Float, 0.0f, 1.0f),
				ADD_PROPERTY_RANGED(EffectShapeComponent, lightIntensity, PropertyDataType::Float, 0.0f, MaxIntensity),
				ADD_PROPERTY_RANGED(EffectShapeComponent, lightRadius,    PropertyDataType::Float, 0.5f, MaxRadius),
				ADD_PROPERTY(EffectShapeComponent, lightColor, PropertyDataType::Color)
			});
	}
};

}
