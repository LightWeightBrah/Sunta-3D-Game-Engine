#include "Core/SuntaPreCompiled.h"
#include "SwarmSystem.h"

#include <map>
#include <random>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "SwarmComponent.h"
#include "ParticleEmitterComponent.h"
#include "Core/Log.h"
#include "Core/ResourceManager.h"
#include "Core/VirtualFileSystem.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Gameplay/EntityName.h"
#include "Renderer/ElementBuffer.h"
#include "Renderer/InstancedDrawer.h"
#include "Renderer/Material.h"
#include "Renderer/Model.h"
#include "Renderer/Renderer.h"
#include "Renderer/RendererDevice.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"
#include "Renderer/VertexBuffer.h"
#include "Renderer/VertexLayouts.h"
#include "Scene/SceneData.h"

namespace Sunta
{

namespace
{

constexpr const char* PieceShaderPath  = "@engine/Shaders/Swarm.shader";
constexpr const char* StreakShaderPath = "@engine/Shaders/SwarmStreak.shader";

constexpr int   DiffuseSlot      = 0;
// A falling copy reaches its full heat in this long, and a stuck one shrinks into the ground in this long at its end
constexpr float HeatUpSeconds    = 0.6f;
constexpr float SinkSeconds      = 1.0f;
// How far into the ground a stuck copy goes (a part of its size) and how slowly the sky ones bob up and down
constexpr float StuckDepth       = 0.2f;
constexpr float SkyBobMeters     = 0.6f;
// How much of the falling copies' width their streak has
constexpr float StreakWidth      = 0.55f;

glm::vec3 RandomDirection(std::mt19937& random)
{
	std::uniform_real_distribution<float> signedUnit(-1.0f, 1.0f);
	for (;;)
	{
		const glm::vec3 point(signedUnit(random), signedUnit(random), signedUnit(random));
		const float length = glm::length(point);
		if (length > 0.05f && length <= 1.0f)
			return point / length;
	}
}

// Every copy's own numbers, the same each time for the same seed
void BuildPieces(SwarmComponent& swarm)
{
	std::mt19937 random((unsigned int)swarm.seed * 7919u + 17u);
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	std::uniform_real_distribution<float> signedUnit(-1.0f, 1.0f);

	swarm.pieces.assign((size_t)glm::max(swarm.count, 0), SwarmComponent::Piece{});
	for (SwarmComponent::Piece& piece : swarm.pieces)
	{
		piece.direction   = RandomDirection(random);
		piece.depth       = unit(random);
		piece.skyOffset   = glm::vec3(signedUnit(random), signedUnit(random), signedUnit(random));
		piece.size        = 1.0f + signedUnit(random) * swarm.sizeVariation;
		piece.tumbleAxis  = RandomDirection(random);
		piece.tumbleSpeed = 0.5f + unit(random);
		piece.phase       = unit(random) * 360.0f;
		piece.leaveAt     = unit(random);
		piece.fallDelay   = unit(random);
		piece.fallTilt    = glm::vec3(signedUnit(random), 0.0f, signedUnit(random)) * 0.25f;
	}

	swarm.builtCount = swarm.count;
	swarm.builtSeed  = swarm.seed;
}

// 0 until 'leaveAt' of the stage, 1 a little after: the copies go one after another, not all at once
float Staged(float stage, float leaveAt)
{
	const float t = glm::clamp(stage * 1.5f - leaveAt * 0.5f, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

glm::vec3 WorldPositionOf(EntityManager& entityManager, unsigned int entityID)
{
	if (const WorldMatrixComponent* world = entityManager.GetComponent<WorldMatrixComponent>(entityID))
		return glm::vec3(world->matrix[3]);
	const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);
	return transform ? transform->position : glm::vec3(0.0f);
}

void Land(EntityManager& entityManager, SwarmComponent& swarm, SwarmComponent::Piece& piece, ParticleEmitterComponent* hitEmitter)
{
	piece.isFalling  = false;
	piece.isStuck    = true;
	piece.stuckFor   = 0.0f;
	piece.position.y = swarm.groundHeight + swarm.pieceSize * piece.size * StuckDepth;

	if (hitEmitter && swarm.hitBurst > 0)
		hitEmitter->pointBursts.push_back({ piece.position, swarm.hitBurst });
}

// ========== Drawing ==========

struct PieceDrawer
{
	std::unique_ptr<InstancedDrawer> drawer;
	std::shared_ptr<Texture>         diffuse;
};

std::map<std::string, PieceDrawer>& GetPieceDrawers()
{
	static std::map<std::string, PieceDrawer> drawers;
	return drawers;
}

std::shared_ptr<Shader>& GetPieceShader()  { static std::shared_ptr<Shader> shader; return shader; }
std::shared_ptr<Shader>& GetStreakShader() { static std::shared_ptr<Shader> shader; return shader; }
std::unique_ptr<InstancedDrawer>& GetStreakDrawer() { static std::unique_ptr<InstancedDrawer> drawer; return drawer; }

// The model's own vertices and triangles, drawn as many times as there are copies
PieceDrawer* FindPieceDrawer(const std::string& modelName)
{
	auto& drawers = GetPieceDrawers();
	auto found = drawers.find(modelName);
	if (found != drawers.end())
		return found->second.drawer ? &found->second : nullptr;

	PieceDrawer& entry = drawers[modelName];

	std::shared_ptr<ModelData> modelData = ResourceManager::GetModelData(modelName);
	if (!modelData || !modelData->model || modelData->model->GetStaticIndices().empty())
	{
		SUNTA_ENGINE_LOG_ERROR("Swarm: the model '{0}' can't be used (missing, or it has bones)", modelName);
		return nullptr;
	}

	const Model& model = *modelData->model;
	RendererDevice& rendererDevice = Renderer::GetDevice();

	BufferDescriptor vertexDescriptor;
	vertexDescriptor.data = model.GetStaticVertices().data();
	vertexDescriptor.size = (unsigned int)(model.GetStaticVertices().size() * sizeof(StaticVertex));
	auto vertices = rendererDevice.CreateVertexBuffer(vertexDescriptor);
	vertices->SetLayout(VertexLayouts::GetStaticLayout());

	BufferDescriptor triangleDescriptor;
	triangleDescriptor.data = model.GetStaticIndices().data();
	triangleDescriptor.size = (unsigned int)(model.GetStaticIndices().size() * sizeof(unsigned int));
	auto triangles = rendererDevice.CreateElementBuffer(triangleDescriptor);

	const BufferLayout instanceLayout
	{
		{ ShaderDataType::Mat4,   "iModel", false, true },
		{ ShaderDataType::Float4, "iGlow",  false, true }
	};
	entry.drawer = std::make_unique<InstancedDrawer>(vertices, triangles, instanceLayout);

	for (const SubMesh& subMesh : model.GetSubMeshes())
	{
		if (subMesh.material && !subMesh.material->GetDiffuseMaps().empty())
		{
			entry.diffuse = subMesh.material->GetDiffuseMaps().front();
			break;
		}
	}
	return &entry;
}

void EnsureStreaks()
{
	if (GetStreakDrawer())
		return;

	RendererDevice& rendererDevice = Renderer::GetDevice();

	// x: 0 at the copy, 1 at the end of its streak; y: across it
	static const glm::vec3 corners[] = { { 0.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };
	static const unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

	BufferDescriptor vertexDescriptor;
	vertexDescriptor.data = corners;
	vertexDescriptor.size = sizeof(corners);
	auto vertices = rendererDevice.CreateVertexBuffer(vertexDescriptor);
	vertices->SetLayout(VertexLayouts::GetPositionOnlyLayout());

	BufferDescriptor triangleDescriptor;
	triangleDescriptor.data = indices;
	triangleDescriptor.size = sizeof(indices);
	auto triangles = rendererDevice.CreateElementBuffer(triangleDescriptor);

	const BufferLayout instanceLayout
	{
		{ ShaderDataType::Float4, "iHead", false, true },
		{ ShaderDataType::Float4, "iTail", false, true }
	};
	GetStreakDrawer() = std::make_unique<InstancedDrawer>(vertices, triangles, instanceLayout);
}

}

void SwarmSystem::Fall(SwarmComponent& swarm)
{
	if (swarm.fallSeconds < 0.0f)
		swarm.fallSeconds = 0.0f;
}

void SwarmSystem::Reset(SwarmComponent& swarm)
{
	swarm.gathered    = 0.0f;
	swarm.spread      = 0.0f;
	swarm.fallSeconds = -1.0f;
	swarm.seconds     = 0.0f;
	for (SwarmComponent::Piece& piece : swarm.pieces)
	{
		piece.isFalling = false;
		piece.isStuck   = false;
		piece.heat      = 0.0f;
		piece.stuckFor  = 0.0f;
	}
	swarm.instances.clear();
	swarm.streaks.clear();
}

void SwarmSystem::Clear(EntityManager& entityManager)
{
	for (SwarmComponent& swarm : entityManager.GetAllComponents<SwarmComponent>())
		Reset(swarm);
}

void SwarmSystem::Update(EntityManager& entityManager, float deltaTime)
{
	for (SwarmComponent& swarm : entityManager.GetAllComponents<SwarmComponent>())
	{
		swarm.instances.clear();
		swarm.streaks.clear();

		if (swarm.builtCount != swarm.count || swarm.builtSeed != swarm.seed)
			BuildPieces(swarm);

		swarm.seconds += deltaTime;
		if (swarm.fallSeconds >= 0.0f)
			swarm.fallSeconds += deltaTime;

		const unsigned int entityID = entityManager.GetEntityIDForComponent(swarm);
		const glm::vec3 center = WorldPositionOf(entityManager, entityID);
		const glm::quat cloudTurn = glm::angleAxis(glm::radians(swarm.seconds * swarm.spinDegreesPerSecond), glm::vec3(0.0f, 1.0f, 0.0f));

		ParticleEmitterComponent* hitEmitter = nullptr;
		unsigned int emitterID = 0;
		if (!swarm.hitEffect.empty() && TryFindEntityByName(entityManager, swarm.hitEffect, emitterID))
			hitEmitter = entityManager.GetComponent<ParticleEmitterComponent>(emitterID);

		const float fallSpreadSeconds = glm::max(swarm.fallSpreadSeconds, 0.001f);

		for (SwarmComponent::Piece& piece : swarm.pieces)
		{
			float scale = swarm.pieceSize * piece.size;

			if (!piece.isFalling && !piece.isStuck)
			{
				const float gathered = Staged(swarm.gathered, piece.leaveAt);
				const float spread   = Staged(swarm.spread, piece.leaveAt);

				const glm::vec3 onSphere = center + cloudTurn * (piece.direction * swarm.radius * (1.0f - swarm.shellThickness * piece.depth) * gathered);
				const glm::vec3 inSky    = swarm.skyCenter + piece.skyOffset * swarm.skyHalfExtents
					+ glm::vec3(0.0f, std::sin(swarm.seconds * 0.7f + glm::radians(piece.phase)) * SkyBobMeters, 0.0f);

				piece.position = glm::mix(onSphere, inSky, spread);
				piece.rotation = glm::angleAxis(glm::radians(piece.phase + swarm.seconds * swarm.tumbleDegreesPerSecond * piece.tumbleSpeed), piece.tumbleAxis);
				piece.heat     = 0.0f;
				// They grow as they leave the middle
				scale *= glm::min(gathered * 2.0f + spread, 1.0f);

				if (swarm.fallSeconds >= piece.fallDelay * fallSpreadSeconds && scale > 0.0f)
					piece.isFalling = true;
			}

			if (piece.isFalling)
			{
				const glm::vec3 direction = glm::normalize(glm::vec3(piece.fallTilt.x, -1.0f, piece.fallTilt.z));
				piece.position += direction * swarm.fallSpeed * deltaTime;
				piece.rotation  = glm::angleAxis(glm::radians(swarm.tumbleDegreesPerSecond * piece.tumbleSpeed * 2.0f * deltaTime), piece.tumbleAxis) * piece.rotation;
				piece.heat      = glm::min(piece.heat + deltaTime / HeatUpSeconds, 1.0f);

				if (piece.position.y <= swarm.groundHeight)
					Land(entityManager, swarm, piece, hitEmitter);
				else if (swarm.trailLength > 0.0f)
					swarm.streaks.push_back({ glm::vec4(piece.position, scale * StreakWidth),
						glm::vec4(piece.position - direction * swarm.trailLength * piece.heat, piece.heat) });
			}

			if (piece.isStuck)
			{
				// Burning where it struck, cooling down, then sinking out of sight
				piece.stuckFor += deltaTime;
				piece.heat = glm::clamp(1.0f - piece.stuckFor / glm::max(swarm.stuckSeconds, 0.001f), 0.0f, 1.0f);
				scale *= glm::clamp(1.0f - (piece.stuckFor - swarm.stuckSeconds) / SinkSeconds, 0.0f, 1.0f);
			}

			if (scale <= 0.0001f || !swarm.isVisible)
				continue;

			const glm::mat4 matrix = glm::translate(glm::mat4(1.0f), piece.position) * glm::mat4_cast(piece.rotation)
				* glm::scale(glm::mat4(1.0f), glm::vec3(scale));
			swarm.instances.push_back({ matrix, glm::vec4(swarm.glowColor * piece.heat, 0.0f) });
		}
	}
}

void SwarmSystem::Draw(EntityManager& entityManager, const SceneData& sceneData)
{
	auto& swarms = entityManager.GetAllComponents<SwarmComponent>();
	bool hasAnything = false;
	for (const SwarmComponent& swarm : swarms)
		hasAnything = hasAnything || (swarm.isVisible && !swarm.instances.empty());
	if (!hasAnything)
		return;

	RendererDevice& rendererDevice = Renderer::GetDevice();
	if (!GetPieceShader())
	{
		GetPieceShader()  = rendererDevice.CreateShader(VirtualFileSystem::Resolve(PieceShaderPath));
		GetStreakShader() = rendererDevice.CreateShader(VirtualFileSystem::Resolve(StreakShaderPath));
	}

	const glm::mat4 viewProjection = sceneData.projectionMatrix * sceneData.viewMatrix;

	// The light of the first directional light (the sun, the moon), or a plain one
	glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));
	glm::vec3 ambient = glm::vec3(0.3f);
	glm::vec3 diffuse = glm::vec3(0.7f);
	if (!sceneData.directionalLights.empty())
	{
		lightDirection = sceneData.directionalLights.front().direction;
		ambient        = sceneData.directionalLights.front().color.ambientIntensity;
		diffuse        = sceneData.directionalLights.front().color.diffuseIntensity;
	}

	Shader& pieceShader = *GetPieceShader();
	pieceShader.Bind();
	pieceShader.SetUniformMatrix4fv("viewProjection", viewProjection);
	pieceShader.SetUniform3f("lightDirection", lightDirection);
	pieceShader.SetUniform3f("lightAmbient", ambient);
	pieceShader.SetUniform3f("lightDiffuse", diffuse);
	pieceShader.SetUniform3f("cameraPosition", sceneData.cameraPosition);
	pieceShader.SetUniform3f("fogColor", sceneData.fogColor);
	pieceShader.SetUniform1f("fogDensity", sceneData.fogDensity);
	pieceShader.SetUniform1i("diffuseMap", DiffuseSlot);

	for (const SwarmComponent& swarm : swarms)
	{
		if (!swarm.isVisible || swarm.instances.empty())
			continue;

		PieceDrawer* drawer = FindPieceDrawer(swarm.modelName);
		if (!drawer)
			continue;

		pieceShader.SetUniformBool("hasDiffuseMap", drawer->diffuse != nullptr);
		if (drawer->diffuse)
			drawer->diffuse->Bind(DiffuseSlot);

		drawer->drawer->Draw(swarm.instances.data(), (unsigned int)swarm.instances.size());
	}

	// The streaks of the falling ones: light added to the picture, hidden behind walls but hiding nothing
	EnsureStreaks();
	Shader& streakShader = *GetStreakShader();
	streakShader.Bind();
	streakShader.SetUniformMatrix4fv("viewProjection", viewProjection);
	streakShader.SetUniform3f("cameraPosition", sceneData.cameraPosition);
	streakShader.SetUniform1f("fogDensity", sceneData.fogDensity);

	rendererDevice.SetDepthWrite(false);
	rendererDevice.SetAdditiveBlending(true);
	for (const SwarmComponent& swarm : swarms)
	{
		if (!swarm.isVisible || swarm.streaks.empty())
			continue;

		streakShader.SetUniform3f("glowColor", swarm.glowColor);
		GetStreakDrawer()->Draw(swarm.streaks.data(), (unsigned int)swarm.streaks.size());
	}
	rendererDevice.SetAdditiveBlending(false);
	rendererDevice.SetDepthWrite(true);
}

}
