#include "Core/SuntaPreCompiled.h"
#include "EffectSystem.h"

#include <algorithm>
#include <random>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "EffectShapeComponent.h"
#include "ParticleEmitterComponent.h"
#include "ParticleSystem.h"
#include "Core/VirtualFileSystem.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Gameplay/EntityName.h"
#include "Renderer/ElementBuffer.h"
#include "Renderer/InstancedDrawer.h"
#include "Renderer/Renderer.h"
#include "Renderer/RendererDevice.h"
#include "Renderer/Shader.h"
#include "Renderer/VertexBuffer.h"
#include "Renderer/VertexLayouts.h"
#include "Scene/SceneData.h"

namespace Sunta
{

namespace
{

constexpr const char* ShaderPath = "@engine/Shaders/EffectShape.shader";

// The one mesh every shape is made of: a grid of (around, across) from 0 to 1; the vertex shader bends it into
// a ring, an arc, a tube or a ball
constexpr int Around = 96;
constexpr int Across = 16;

// What each shape gives the shader: its matrix (four columns) and (age 0..1, seed, -, -)
struct ShapeInstance
{
	glm::vec4 column0;
	glm::vec4 column1;
	glm::vec4 column2;
	glm::vec4 column3;
	glm::vec4 ageSeed;
};

std::mt19937& GetRandom()
{
	static std::mt19937 random(std::random_device{}());
	return random;
}

float RandomSigned()
{
	static std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	return distribution(GetRandom());
}

std::unique_ptr<InstancedDrawer>& GetDrawer()
{
	static std::unique_ptr<InstancedDrawer> drawer;
	return drawer;
}

std::shared_ptr<Shader>& GetShader()
{
	static std::shared_ptr<Shader> shader;
	return shader;
}

std::vector<ShapeInstance>& GetInstances()
{
	static std::vector<ShapeInstance> instances;
	return instances;
}

void EnsureInitialized()
{
	if (GetDrawer())
		return;

	RendererDevice& rendererDevice = Renderer::GetDevice();

	static std::vector<glm::vec3> corners;
	static std::vector<unsigned int> indices;
	corners.clear();
	indices.clear();
	for (int j = 0; j <= Across; j++)
		for (int i = 0; i <= Around; i++)
			corners.emplace_back((float)i / Around, (float)j / Across, 0.0f);

	for (int j = 0; j < Across; j++)
	{
		for (int i = 0; i < Around; i++)
		{
			const unsigned int a = j * (Around + 1) + i;
			const unsigned int b = a + 1;
			const unsigned int c = a + (Around + 1);
			const unsigned int d = c + 1;
			indices.insert(indices.end(), { a, b, d, d, c, a });
		}
	}

	BufferDescriptor vertexDescriptor;
	vertexDescriptor.data = corners.data();
	vertexDescriptor.size = (unsigned int)(corners.size() * sizeof(glm::vec3));
	auto vertices = rendererDevice.CreateVertexBuffer(vertexDescriptor);
	vertices->SetLayout(VertexLayouts::GetPositionOnlyLayout());

	BufferDescriptor triangleDescriptor;
	triangleDescriptor.data = indices.data();
	triangleDescriptor.size = (unsigned int)(indices.size() * sizeof(unsigned int));
	auto triangles = rendererDevice.CreateElementBuffer(triangleDescriptor);

	const BufferLayout instanceLayout
	{
		{ ShaderDataType::Float4, "iColumn0", false, true },
		{ ShaderDataType::Float4, "iColumn1", false, true },
		{ ShaderDataType::Float4, "iColumn2", false, true },
		{ ShaderDataType::Float4, "iColumn3", false, true },
		{ ShaderDataType::Float4, "iAgeSeed", false, true }
	};
	GetDrawer() = std::make_unique<InstancedDrawer>(vertices, triangles, instanceLayout);
	GetShader() = rendererDevice.CreateShader(VirtualFileSystem::Resolve(ShaderPath));
}

// Which way the effect faces: along 'direction' on the ground, or the way its entity faces
glm::mat4 FacingMatrix(const glm::vec3& position, const glm::vec3& direction, const glm::mat4& entityMatrix)
{
	glm::vec3 forward(direction.x, 0.0f, direction.z);
	if (glm::length(forward) < 0.0001f)
	{
		forward = glm::vec3(entityMatrix[2]);
		forward.y = 0.0f;
	}
	forward = glm::length(forward) > 0.0001f ? glm::normalize(forward) : glm::vec3(0.0f, 0.0f, 1.0f);

	const glm::vec3 up(0.0f, 1.0f, 0.0f);
	const glm::vec3 right = glm::normalize(glm::cross(up, forward));

	glm::mat4 matrix(1.0f);
	matrix[0] = glm::vec4(right, 0.0f);
	matrix[1] = glm::vec4(up, 0.0f);
	matrix[2] = glm::vec4(forward, 0.0f);
	matrix[3] = glm::vec4(position, 1.0f);
	return matrix;
}

void AddShape(EffectShapeComponent& shape, const glm::mat4& facing)
{
	const glm::vec3 degrees = shape.rotationDegrees + shape.rotationRandomness * glm::vec3(RandomSigned(), RandomSigned(), RandomSigned());

	// Yaw, then pitch, then roll (a slash is tilted around the way it faces)
	glm::mat4 matrix = glm::translate(facing, shape.offset);
	matrix = glm::rotate(matrix, glm::radians(degrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
	matrix = glm::rotate(matrix, glm::radians(degrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
	matrix = glm::rotate(matrix, glm::radians(degrees.z), glm::vec3(0.0f, 0.0f, 1.0f));

	EffectShapeComponent::Instance instance;
	instance.matrix = matrix;
	instance.seed   = (RandomSigned() + 1.0f) * 0.5f;

	// Too many at once: the oldest goes
	if ((int)shape.instances.size() >= EffectShapeComponent::MaxInstances)
		shape.instances.erase(shape.instances.begin());
	shape.instances.push_back(instance);
}

void PlayOn(EntityManager& entityManager, unsigned int entityID, const glm::vec3& position, const glm::vec3& direction, int count)
{
	const WorldMatrixComponent* world = entityManager.GetComponent<WorldMatrixComponent>(entityID);
	const glm::mat4 entityMatrix = world ? world->matrix : glm::mat4(1.0f);

	if (ParticleEmitterComponent* emitter = entityManager.GetComponent<ParticleEmitterComponent>(entityID))
		ParticleSystem::BurstEmitter(*emitter, position, emitter->burstAmount > 0 ? emitter->burstAmount : count);

	if (EffectShapeComponent* shape = entityManager.GetComponent<EffectShapeComponent>(entityID))
		AddShape(*shape, FacingMatrix(position, direction, entityMatrix));
}

// The shape's light: brightest at its birth, gone at its end
float LightFade(float agePart)
{
	return (1.0f - agePart) * (1.0f - agePart);
}

}

bool EffectSystem::Play(EntityManager& entityManager, const std::string& name, const glm::vec3& position,
	const glm::vec3& direction, int count)
{
	unsigned int rootID = 0;
	if (name.empty() || !TryFindEntityByName(entityManager, name, rootID))
		return false;

	// The entity and everything under it (children of children too), each once
	std::vector<unsigned int> members = { rootID };
	for (size_t next = 0; next < members.size(); next++)
	{
		for (const ParentComponent& parent : entityManager.GetAllComponents<ParentComponent>())
		{
			if (parent.parentID == members[next])
				members.push_back(entityManager.GetEntityIDForComponent(parent));
		}
	}

	for (const unsigned int member : members)
		PlayOn(entityManager, member, position, direction, count);
	return true;
}

void EffectSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (EffectShapeComponent& shape : entityManager.GetAllComponents<EffectShapeComponent>())
	{
		for (EffectShapeComponent::Instance& instance : shape.instances)
			instance.age += deltaTime;

		const float lifetime = shape.lifetimeSeconds;
		shape.instances.erase(std::remove_if(shape.instances.begin(), shape.instances.end(),
			[lifetime](const EffectShapeComponent::Instance& instance) { return instance.age >= lifetime; }), shape.instances.end());
	}
}

void EffectSystem::AddLights(EntityManager& entityManager, std::vector<PointLightData>& pointLights, size_t maxLights)
{
	for (const EffectShapeComponent& shape : entityManager.GetAllComponents<EffectShapeComponent>())
	{
		if (shape.lightIntensity <= 0.0f)
			continue;

		for (const EffectShapeComponent::Instance& instance : shape.instances)
		{
			if (pointLights.size() >= maxLights)
				return;

			const float strength = shape.lightIntensity * LightFade(instance.age / shape.lifetimeSeconds);

			PointLightData light;
			light.position = glm::vec3(instance.matrix[3]) + glm::vec3(0.0f, 0.5f, 0.0f);
			light.color.ambientIntensity  = glm::vec3(0.0f);
			light.color.diffuseIntensity  = shape.lightColor * strength;
			light.color.specularIntensity = shape.lightColor * strength * 0.5f;
			// About a tenth of its light left at 'lightRadius'
			const float radius = glm::max(shape.lightRadius, 0.5f);
			light.attenuation.constant  = 1.0f;
			light.attenuation.linear    = 2.0f / radius;
			light.attenuation.quadratic = 6.0f / (radius * radius);
			pointLights.push_back(light);
		}
	}
}

void EffectSystem::Draw(EntityManager& entityManager, const SceneData& sceneData, float seconds)
{
	auto& shapes = entityManager.GetAllComponents<EffectShapeComponent>();
	bool hasAnything = false;
	for (const EffectShapeComponent& shape : shapes)
		hasAnything = hasAnything || !shape.instances.empty();
	if (!hasAnything)
		return;

	EnsureInitialized();

	RendererDevice& rendererDevice = Renderer::GetDevice();
	Shader& shader = *GetShader();
	shader.Bind();
	shader.SetUniformMatrix4fv("viewProjection", sceneData.projectionMatrix * sceneData.viewMatrix);
	shader.SetUniform3f("cameraPosition", sceneData.cameraPosition);
	shader.SetUniform1f("fogDensity", sceneData.fogDensity);
	shader.SetUniform1f("seconds", seconds);

	rendererDevice.SetDepthWrite(false);
	rendererDevice.SetAdditiveBlending(true);

	std::vector<ShapeInstance>& instances = GetInstances();
	for (const EffectShapeComponent& shape : shapes)
	{
		if (shape.instances.empty())
			continue;

		instances.clear();
		for (const EffectShapeComponent::Instance& instance : shape.instances)
		{
			instances.push_back({ instance.matrix[0], instance.matrix[1], instance.matrix[2], instance.matrix[3],
				glm::vec4(glm::clamp(instance.age / shape.lifetimeSeconds, 0.0f, 1.0f), instance.seed, 0.0f, 0.0f) });
		}

		shader.SetUniform1i("kind", shape.kind);
		shader.SetUniform1f("startRadius", shape.startRadius);
		shader.SetUniform1f("endRadius", shape.endRadius);
		shader.SetUniform1f("thickness", shape.thickness);
		shader.SetUniform1f("height", shape.height);
		shader.SetUniform1f("arcRadians", glm::radians(shape.arcDegrees));
		shader.SetUniform1f("spinRadians", glm::radians(shape.spinDegreesPerSecond) * shape.lifetimeSeconds);
		shader.SetUniform1f("twistRadians", glm::radians(shape.twistDegrees));
		shader.SetUniform3f("coreColor", shape.coreColor * shape.intensity);
		shader.SetUniform3f("edgeColor", shape.edgeColor * shape.intensity);
		shader.SetUniform1f("noiseScale", shape.noiseScale);
		shader.SetUniform1f("scrollSpeed", shape.scrollSpeed);
		shader.SetUniform1f("dissolve", shape.dissolve);
		shader.SetUniform1f("lifetimeSeconds", shape.lifetimeSeconds);

		GetDrawer()->Draw(instances.data(), (unsigned int)instances.size());
	}

	rendererDevice.SetAdditiveBlending(false);
	rendererDevice.SetDepthWrite(true);
}

void EffectSystem::Clear(EntityManager& entityManager)
{
	for (EffectShapeComponent& shape : entityManager.GetAllComponents<EffectShapeComponent>())
		shape.instances.clear();
}

}
