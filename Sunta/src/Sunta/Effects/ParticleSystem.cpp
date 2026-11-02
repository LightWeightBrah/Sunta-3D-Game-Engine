#include "Core/SuntaPreCompiled.h"
#include "ParticleSystem.h"

#include <algorithm>
#include <random>

#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Gameplay/EntityName.h"
#include "Core/VirtualFileSystem.h"
#include "Renderer/ElementBuffer.h"
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

// Born in the box around the emitter (turned and scaled with it), flying in world space
void SpawnParticle(ParticleEmitterComponent& emitter, const glm::mat4& emitterMatrix)
{
	if (emitter.maxParticles <= 0)
		return;

	if ((int)emitter.particles.size() >= emitter.maxParticles)
		emitter.particles.erase(emitter.particles.begin());

	Particle particle;
	particle.position = glm::vec3(emitterMatrix * glm::vec4(RandomInBox(emitter.spawnHalfExtents), 1.0f));
	particle.velocity = emitter.velocity + RandomInBox(glm::vec3(emitter.velocityRandomness));
	particle.lifetime = emitter.lifetimeSeconds * (0.75f + 0.25f * (RandomSigned() + 1.0f));
	emitter.particles.push_back(particle);
}

void MoveParticles(ParticleEmitterComponent& emitter, float deltaTime)
{
	for (Particle& particle : emitter.particles)
	{
		particle.velocity += emitter.gravity * deltaTime;
		particle.position += particle.velocity * deltaTime;
		particle.age      += deltaTime;
	}

	auto isDead = [](const Particle& particle) { return particle.age >= particle.lifetime; };
	emitter.particles.erase(std::remove_if(emitter.particles.begin(), emitter.particles.end(), isDead), emitter.particles.end());
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

bool ParticleSystem::BurstAt(EntityManager& entityManager, const std::string& emitterName, const glm::vec3& position, int count)
{
	unsigned int emitterID = 0;
	if (!TryFindEntityByName(entityManager, emitterName, emitterID))
		return false;

	ParticleEmitterComponent* emitter = entityManager.GetComponent<ParticleEmitterComponent>(emitterID);
	if (!emitter)
		return false;

	emitter->pointBursts.push_back({ position, count });
	return true;
}

void ParticleSystem::Clear(EntityManager& entityManager)
{
	for (ParticleEmitterComponent& emitter : entityManager.GetAllComponents<ParticleEmitterComponent>())
	{
		emitter.particles.clear();
		emitter.unbornParticles = 0.0f;
		emitter.burstCount      = 0;
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

	quad->AddVertexBuffer(vertexBuffer);
	quad->SetElementBuffer(rendererDevice.CreateElementBuffer(elementBufferDescriptor));

	shader = rendererDevice.CreateShader(VirtualFileSystem::Resolve(ShaderPath));
}

void ParticleSystem::Draw(EntityManager& entityManager, const glm::mat4& view, const glm::mat4& projection,
	const glm::vec3& cameraPosition, float fogDensity)
{
	auto& emitters = entityManager.GetAllComponents<ParticleEmitterComponent>();

	bool hasParticles = false;
	for (const ParticleEmitterComponent& emitter : emitters)
		hasParticles = hasParticles || !emitter.particles.empty();

	if (!hasParticles)
		return;

	EnsureInitialized();

	RendererDevice& rendererDevice = Renderer::GetDevice();
	rendererDevice.SetAdditiveBlending(true);
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

	for (const ParticleEmitterComponent& emitter : emitters)
	{
		for (const Particle& particle : emitter.particles)
		{
			const float     lifePart = glm::clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
			const float     size     = glm::mix(emitter.startSize, emitter.endSize, lifePart);
			const glm::vec3 color    = glm::mix(emitter.startColor, emitter.endColor, lifePart) * emitter.intensity;

			shader->SetUniform3f("center", particle.position.x, particle.position.y, particle.position.z);
			shader->SetUniform1f("size", size);
			shader->SetUniform3f("color", color.r, color.g, color.b);

			rendererDevice.DrawElements(*quad);
		}
	}

	rendererDevice.SetDepthWrite(true);
	rendererDevice.SetAdditiveBlending(false);
}

}
