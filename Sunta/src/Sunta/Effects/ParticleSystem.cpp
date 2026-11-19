#include "Core/SuntaPreCompiled.h"
#include "ParticleSystem.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <glm/gtc/constants.hpp>

#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "EffectSystem.h"
#include "Gameplay/EntityName.h"
#include "Core/VirtualFileSystem.h"
#include "Renderer/ElementBuffer.h"
#include "Renderer/InstancedDrawer.h"
#include "Renderer/Renderer.h"
#include "Renderer/RendererDevice.h"
#include "Renderer/ScopedVertexArrayBind.h"
#include "Renderer/Shader.h"
#include "Renderer/VertexArray.h"
#include "Renderer/VertexBuffer.h"
#include "Renderer/VertexLayouts.h"

namespace Sunta
{

namespace
{

constexpr const char* ShaderPath = "@engine/Shaders/Particle.shader";

// A square around its middle; the vertex shader turns it to face the camera
const glm::vec3 QuadCorners[] =
{
	{ -0.5f, -0.5f, 0.0f },
	{  0.5f, -0.5f, 0.0f },
	{  0.5f,  0.5f, 0.0f },
	{ -0.5f,  0.5f, 0.0f }
};

const unsigned int QuadIndices[] = { 0, 1, 2, 2, 3, 0 };

std::mt19937& GetRandom()
{
	static std::mt19937 random(std::random_device{}());
	return random;
}

// -1..1
float RandomSigned()
{
	static std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	return distribution(GetRandom());
}

glm::vec3 RandomInBox(const glm::vec3& halfExtents)
{
	return glm::vec3(RandomSigned() * halfExtents.x, RandomSigned() * halfExtents.y, RandomSigned() * halfExtents.z);
}

// 0..1
float RandomUnit()
{
	return (RandomSigned() + 1.0f) * 0.5f;
}

glm::vec3 RandomDirection()
{
	for (;;)
	{
		const glm::vec3 point(RandomSigned(), RandomSigned(), RandomSigned());
		const float length = glm::length(point);
		if (length > 0.05f && length <= 1.0f)
			return point / length;
	}
}

// Where it is born around the emitter (in the emitter's own space) and which way is "out" from there
void PickBirthPlace(const ParticleEmitterComponent& emitter, glm::vec3& outPlace, glm::vec3& outOutward)
{
	switch ((ParticleSpawnShape)emitter.spawnShape)
	{
	case ParticleSpawnShape::Sphere:
		outOutward = RandomDirection();
		// Evenly through the ball (more of them far from the middle, where there is more room)
		outPlace = outOutward * emitter.spawnRadius * std::cbrt(RandomUnit());
		return;

	case ParticleSpawnShape::Dome:
		outOutward = RandomDirection();
		outOutward.y = std::abs(outOutward.y);
		// On the shell, a little into it (the swelling surface of a hill of fire)
		outPlace = outOutward * emitter.spawnRadius * (0.92f + 0.08f * RandomUnit());
		return;

	case ParticleSpawnShape::Box:
	default:
		outPlace   = RandomInBox(emitter.spawnHalfExtents);
		outOutward = glm::length(outPlace) > 0.0001f ? glm::normalize(outPlace) : glm::vec3(0.0f, 1.0f, 0.0f);
		return;
	}
}

// Born around the emitter (turned and scaled with it), flying in world space
void SpawnParticle(ParticleEmitterComponent& emitter, const glm::mat4& emitterMatrix)
{
	if (emitter.maxParticles <= 0)
		return;

	glm::vec3 place, outward;
	PickBirthPlace(emitter, place, outward);

	Particle particle;
	particle.position = glm::vec3(emitterMatrix * glm::vec4(place, 1.0f));
	particle.velocity = emitter.velocity + RandomInBox(glm::vec3(emitter.velocityRandomness))
		+ glm::vec3(emitterMatrix * glm::vec4(outward, 0.0f)) * emitter.outwardSpeed;
	particle.lifetime = emitter.lifetimeSeconds * (0.75f + 0.25f * (RandomSigned() + 1.0f));
	particle.size     = 1.0f + RandomSigned() * emitter.sizeRandomness;
	particle.rotation = RandomUnit() * glm::two_pi<float>();
	particle.spin     = RandomSigned() * glm::radians(emitter.spinDegreesPerSecond);
	particle.seed     = RandomUnit();

	// Full: over the oldest ones in turn (not by moving all the others: there may be tens of thousands)
	if ((int)emitter.particles.size() >= emitter.maxParticles)
	{
		emitter.particles[emitter.nextOverwrite % emitter.particles.size()] = particle;
		emitter.nextOverwrite++;
		return;
	}
	emitter.particles.push_back(particle);
}

void MoveParticles(ParticleEmitterComponent& emitter, float deltaTime)
{
	// The air takes the same part of the speed every second, however long the frame
	const float keptSpeed = emitter.drag > 0.0f ? std::exp(-emitter.drag * deltaTime) : 1.0f;

	for (Particle& particle : emitter.particles)
	{
		particle.velocity *= keptSpeed;
		particle.velocity += emitter.gravity * deltaTime;
		particle.position += particle.velocity * deltaTime;
		particle.rotation += particle.spin * deltaTime;
		particle.age      += deltaTime;
	}

	// The dead ones swapped with the last (the order of specks doesn't matter)
	for (size_t i = 0; i < emitter.particles.size();)
	{
		if (emitter.particles[i].age >= emitter.particles[i].lifetime)
		{
			emitter.particles[i] = emitter.particles.back();
			emitter.particles.pop_back();
		}
		else
		{
			i++;
		}
	}
}

// What each speck gives the shader (one block per speck, see Particle.shader)
struct SpeckInstance
{
	glm::vec4 centerAndSize;
	glm::vec4 colorAndOpacity;
	glm::vec4 rotationSeedStyle;   // rotation (radians), seed (0..1), style, part of its life
	glm::vec4 velocityStretch;     // where it flies (a spark is stretched along it) and the seconds of flight it is long
};

BufferLayout GetSpeckLayout()
{
	return BufferLayout
	{
		{ ShaderDataType::Float4, "iCenterSize",        false, true },
		{ ShaderDataType::Float4, "iColorOpacity",      false, true },
		{ ShaderDataType::Float4, "iRotationSeedStyle", false, true },
		{ ShaderDataType::Float4, "iVelocityStretch",   false, true }
	};
}

std::unique_ptr<InstancedDrawer>& GetDrawer()
{
	static std::unique_ptr<InstancedDrawer> drawer;
	return drawer;
}

std::vector<SpeckInstance>& GetSpecks()
{
	static std::vector<SpeckInstance> specks;
	return specks;
}

// Its color at this part of its life: start -> end, or start -> middle -> end
glm::vec3 ColorAt(const ParticleEmitterComponent& emitter, float lifePart)
{
	if (!emitter.hasMiddleColor)
		return glm::mix(emitter.startColor, emitter.endColor, lifePart);

	return lifePart < 0.5f ? glm::mix(emitter.startColor, emitter.middleColor, lifePart * 2.0f)
		: glm::mix(emitter.middleColor, emitter.endColor, (lifePart - 0.5f) * 2.0f);
}

bool IsDrawnDark(const ParticleEmitterComponent& emitter)
{
	return emitter.isDark || emitter.style == (int)ParticleStyle::Smoke;
}

}

void ParticleSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (ParticleEmitterComponent& emitter : entityManager.GetAllComponents<ParticleEmitterComponent>())
	{
		const unsigned int entityID = entityManager.GetEntityIDForComponent(emitter);
		const WorldMatrixComponent* world = entityManager.GetComponent<WorldMatrixComponent>(entityID);
		const glm::mat4 emitterMatrix = world ? world->matrix : glm::mat4(1.0f);

		MoveParticles(emitter, deltaTime);

		if (emitter.isEmitting)
			emitter.unbornParticles += emitter.particlesPerSecond * deltaTime;

		int newParticles = (int)emitter.unbornParticles + emitter.burstCount;
		emitter.unbornParticles -= (float)(int)emitter.unbornParticles;
		emitter.burstCount = 0;

		for (int i = 0; i < newParticles; i++)
			SpawnParticle(emitter, emitterMatrix);

		// Around another point: the emitter's turn and size stay, only its place changes
		for (const ParticleEmitterComponent::PointBurst& burst : emitter.pointBursts)
		{
			glm::mat4 burstMatrix = emitterMatrix;
			burstMatrix[3] = glm::vec4(burst.position, 1.0f);

			for (int i = 0; i < burst.count; i++)
				SpawnParticle(emitter, burstMatrix);
		}
		emitter.pointBursts.clear();
	}
}

bool ParticleSystem::BurstAt(EntityManager& entityManager, const std::string& effectName, const glm::vec3& position, int count,
	const glm::vec3& direction)
{
	return EffectSystem::Play(entityManager, effectName, position, direction, count);
}

void ParticleSystem::BurstEmitter(ParticleEmitterComponent& emitter, const glm::vec3& position, int count)
{
	if (count > 0)
		emitter.pointBursts.push_back({ position, count });
}

void ParticleSystem::Clear(EntityManager& entityManager)
{
	for (ParticleEmitterComponent& emitter : entityManager.GetAllComponents<ParticleEmitterComponent>())
	{
		emitter.particles.clear();
		emitter.unbornParticles = 0.0f;
		emitter.burstCount      = 0;
		emitter.nextOverwrite   = 0;
		emitter.pointBursts.clear();
	}
}

void ParticleSystem::EnsureInitialized()
{
	if (shader)
		return;

	RendererDevice& rendererDevice = Renderer::GetDevice();

	quad = rendererDevice.CreateVertexArray();
	ScopedVertexArrayBind vertexArrayGuard(*quad);

	BufferDescriptor vertexBufferDescriptor;
	vertexBufferDescriptor.data = QuadCorners;
	vertexBufferDescriptor.size = sizeof(QuadCorners);

	auto vertexBuffer = rendererDevice.CreateVertexBuffer(vertexBufferDescriptor);
	vertexBuffer->SetLayout(VertexLayouts::GetPositionOnlyLayout());

	BufferDescriptor elementBufferDescriptor;
	elementBufferDescriptor.data = QuadIndices;
	elementBufferDescriptor.size = sizeof(QuadIndices);

	auto elementBuffer = rendererDevice.CreateElementBuffer(elementBufferDescriptor);
	quad->AddVertexBuffer(vertexBuffer);
	quad->SetElementBuffer(elementBuffer);

	GetDrawer() = std::make_unique<InstancedDrawer>(vertexBuffer, elementBuffer, GetSpeckLayout());

	shader = rendererDevice.CreateShader(VirtualFileSystem::Resolve(ShaderPath));
}

void ParticleSystem::Draw(EntityManager& entityManager, const glm::mat4& view, const glm::mat4& projection,
	const glm::vec3& cameraPosition, float fogDensity)
{
	auto& emitters = entityManager.GetAllComponents<ParticleEmitterComponent>();

	auto& sprites = entityManager.GetAllComponents<GlowSpriteComponent>();

	bool hasAnything = std::any_of(sprites.begin(), sprites.end(), [](const GlowSpriteComponent& sprite) { return sprite.isVisible; });
	for (const ParticleEmitterComponent& emitter : emitters)
		hasAnything = hasAnything || !emitter.particles.empty();

	if (!hasAnything)
		return;

	EnsureInitialized();

	RendererDevice& rendererDevice = Renderer::GetDevice();
	rendererDevice.SetDepthWrite(false);

	// The camera's right and up directions in the world: the first two rows of the view matrix
	const glm::vec3 cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
	const glm::vec3 cameraUp    = glm::vec3(view[0][1], view[1][1], view[2][1]);

	shader->Bind();
	shader->SetUniformMatrix4fv("viewProjection", projection * view);
	shader->SetUniform3f("cameraRight", cameraRight.x, cameraRight.y, cameraRight.z);
	shader->SetUniform3f("cameraUp", cameraUp.x, cameraUp.y, cameraUp.z);
	shader->SetUniform3f("cameraPosition", cameraPosition.x, cameraPosition.y, cameraPosition.z);
	shader->SetUniform1f("fogDensity", fogDensity);

	std::vector<SpeckInstance>& specks = GetSpecks();

	// The dark specks first (they cover what is behind them), then everything that is light (added on top).
	// All the specks of a pass are drawn at once
	for (const bool isDarkPass : { true, false })
	{
		specks.clear();

		for (const ParticleEmitterComponent& emitter : emitters)
		{
			if (IsDrawnDark(emitter) != isDarkPass)
				continue;

			// Fire and smoke swell in and fade; glows, sparks and rings fade in the shader by their age
			const bool isGlow = emitter.style == (int)ParticleStyle::Glow || emitter.style == (int)ParticleStyle::Spark
				|| emitter.style == (int)ParticleStyle::Ring;

			for (const Particle& particle : emitter.particles)
			{
				const float     lifePart = glm::clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
				const float     size     = glm::mix(emitter.startSize, emitter.endSize, lifePart) * particle.size;
				// A dark speck keeps its color and fades by its 'intensity' (how solid it is); a light one shines by it
				const glm::vec3 color    = ColorAt(emitter, lifePart) * (isDarkPass ? 1.0f : emitter.intensity);
				float           opacity  = isDarkPass ? glm::clamp(emitter.intensity, 0.0f, 1.0f) * (1.0f - lifePart * lifePart) : 1.0f;
				// Fire and smoke swell in at their birth and fade at their end, so a dense fire doesn't flicker
				const float     fade     = isGlow ? 1.0f : glm::smoothstep(0.0f, 0.12f, lifePart) * (1.0f - glm::smoothstep(0.7f, 1.0f, lifePart));
				if (isDarkPass)
					opacity *= fade;

				specks.push_back({ glm::vec4(particle.position, size),
					glm::vec4(isDarkPass ? color : color * fade, opacity),
					glm::vec4(particle.rotation, particle.seed, (float)emitter.style, lifePart),
					glm::vec4(particle.velocity, emitter.stretch) });
			}
		}

		// The sprites of light (see GlowSpriteComponent): one soft spot each, where their entity is
		if (!isDarkPass)
		{
			for (const GlowSpriteComponent& sprite : sprites)
			{
				const WorldMatrixComponent* worldMatrix = entityManager.GetComponent<WorldMatrixComponent>(entityManager.GetEntityIDForComponent(sprite));
				if (!sprite.isVisible || !worldMatrix || sprite.intensity <= 0.0f)
					continue;

				specks.push_back({ glm::vec4(glm::vec3(worldMatrix->matrix[3]), sprite.size),
					glm::vec4(sprite.color * sprite.intensity, 1.0f), glm::vec4(0.0f), glm::vec4(0.0f) });
			}
		}

		if (specks.empty())
			continue;

		if (isDarkPass)
			rendererDevice.SetAlphaBlending(true);
		else
			rendererDevice.SetAdditiveBlending(true);

		shader->SetUniformBool("isDark", isDarkPass);
		GetDrawer()->Draw(specks.data(), (unsigned int)specks.size());
	}

	rendererDevice.SetDepthWrite(true);
	rendererDevice.SetAdditiveBlending(false);
}

}
