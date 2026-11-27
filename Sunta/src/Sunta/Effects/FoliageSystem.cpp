#include "Core/SuntaPreCompiled.h"
#include "FoliageSystem.h"

#include <map>
#include <random>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>

#include "FoliageComponent.h"
#include "Core/Log.h"
#include "Core/ResourceManager.h"
#include "Core/VirtualFileSystem.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Gameplay/TerrainSystem.h"
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

constexpr const char* ShaderPath  = "@engine/Shaders/Foliage.shader";
constexpr unsigned int DiffuseSlot = 0;
// The last part of the draw distance over which the copies shrink away
constexpr float FadePart = 0.25f;
// Tries per copy before a sparse mask gives up on it
constexpr int MaskTries = 8;
// Copies farther than this and outside a cone of the view (the cosine of its half angle, wide for a wide lens) are skipped
constexpr float NearAlways = 4.0f;
constexpr float ViewCone   = 0.35f;

struct FoliageDrawer
{
	std::unique_ptr<InstancedDrawer> drawer;
	std::shared_ptr<Texture>         diffuse;
};

// What each drawn copy hands the shader: its place and size, its turn, tint and sway phase
struct Instance
{
	glm::vec4 placeAndSize;
	glm::vec4 turnTintPhase;
};

std::map<std::string, FoliageDrawer>& GetDrawers()
{
	static std::map<std::string, FoliageDrawer> drawers;
	return drawers;
}

std::shared_ptr<Shader>& GetShader() { static std::shared_ptr<Shader> shader; return shader; }

FoliageDrawer* FindDrawer(const std::string& modelName)
{
	auto& drawers = GetDrawers();
	auto found = drawers.find(modelName);
	if (found != drawers.end())
		return found->second.drawer ? &found->second : nullptr;

	FoliageDrawer& entry = drawers[modelName];
	std::shared_ptr<ModelData> modelData = ResourceManager::GetModelData(modelName);
	if (!modelData || !modelData->model || modelData->model->GetStaticIndices().empty())
	{
		SUNTA_ENGINE_LOG_ERROR("Foliage: the model '{0}' can't be used (missing, or it has bones)", modelName);
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
		{ ShaderDataType::Float4, "iPlaceSize", false, true },
		{ ShaderDataType::Float4, "iTurnTintPhase", false, true }
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

// The density mask: 0..1 per pixel, rows from -Z to +Z. Empty when there is none or it can't be read
std::vector<float> LoadMask(const std::string& path, int& outWidth, int& outHeight)
{
	std::vector<float> mask;
	outWidth = outHeight = 0;
	if (path.empty())
		return mask;
	int channels = 0;
	stbi_set_flip_vertically_on_load(false);
	stbi_uc* pixels = stbi_load(VirtualFileSystem::Resolve(path).c_str(), &outWidth, &outHeight, &channels, 1);
	if (!pixels)
	{
		SUNTA_ENGINE_LOG_WARNING("Foliage: the density mask '{0}' can't be read, the whole area is filled", path);
		return mask;
	}
	mask.assign(pixels, pixels + outWidth * outHeight);
	for (float& value : mask)
		value /= 255.0f;
	stbi_image_free(pixels);
	return mask;
}

void Scatter(EntityManager& entityManager, FoliageComponent& foliage)
{
	foliage.places.clear();
	foliage.extras.clear();
	foliage.isScattered = true;

	const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityManager.GetEntityIDForComponent(foliage));
	const glm::vec3 center = transform ? transform->position : glm::vec3(0.0f);

	int maskWidth = 0, maskHeight = 0;
	const std::vector<float> mask = LoadMask(foliage.densityMask, maskWidth, maskHeight);

	std::mt19937 random((unsigned int)foliage.seed);
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);
	const float lowScale  = glm::min(foliage.minScale, foliage.maxScale);
	const float highScale = glm::max(foliage.minScale, foliage.maxScale);

	foliage.places.reserve(foliage.count);
	foliage.extras.reserve(foliage.count);
	for (int i = 0; i < foliage.count; ++i)
	{
		for (int attempt = 0; attempt < MaskTries; ++attempt)
		{
			const float u = unit(random), v = unit(random);
			if (!mask.empty())
			{
				const int px = glm::clamp((int)(u * maskWidth), 0, maskWidth - 1);
				const int py = glm::clamp((int)(v * maskHeight), 0, maskHeight - 1);
				if (unit(random) > mask[py * maskWidth + px])
					continue;
			}
			glm::vec3 place = center + glm::vec3((u - 0.5f) * foliage.areaSizeX, 0.0f, (v - 0.5f) * foliage.areaSizeZ);
			float ground = 0.0f;
			if (TerrainSystem::TryGetHeight(entityManager, place.x, place.z, ground))
				place.y = ground;
			const float size = glm::mix(lowScale, highScale, unit(random));
			const float tint = 1.0f + (unit(random) * 2.0f - 1.0f) * foliage.tintVariation;
			foliage.places.push_back(glm::vec4(place, size));
			foliage.extras.push_back(glm::vec4(unit(random) * 6.2831853f, tint, unit(random) * 6.2831853f, 0.0f));
			break;
		}
	}
	SUNTA_ENGINE_LOG_INFO("Foliage: {0} copies of '{1}' scattered", foliage.places.size(), foliage.modelName);
}

}

void FoliageSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (FoliageComponent& foliage : entityManager.GetAllComponents<FoliageComponent>())
		Scatter(entityManager, foliage);
}

void FoliageSystem::Draw(EntityManager& entityManager, const SceneData& sceneData, float seconds)
{
	auto& all = entityManager.GetAllComponents<FoliageComponent>();
	if (all.empty())
		return;

	RendererDevice& rendererDevice = Renderer::GetDevice();
	if (!GetShader())
		GetShader() = rendererDevice.CreateShader(VirtualFileSystem::Resolve(ShaderPath));
	Shader& shader = *GetShader();

	glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));
	glm::vec3 ambient = glm::vec3(0.3f);
	glm::vec3 diffuse = glm::vec3(0.7f);
	if (!sceneData.directionalLights.empty())
	{
		lightDirection = sceneData.directionalLights.front().direction;
		ambient        = sceneData.directionalLights.front().color.ambientIntensity;
		diffuse        = sceneData.directionalLights.front().color.diffuseIntensity;
	}

	shader.Bind();
	shader.SetUniformMatrix4fv("viewProjection", sceneData.projectionMatrix * sceneData.viewMatrix);
	shader.SetUniform3f("lightDirection", lightDirection);
	shader.SetUniform3f("lightAmbient", ambient);
	shader.SetUniform3f("lightDiffuse", diffuse);
	shader.SetUniform3f("cameraPosition", sceneData.cameraPosition);
	shader.SetUniform3f("fogColor", sceneData.fogColor);
	shader.SetUniform1f("fogDensity", sceneData.fogDensity);
	shader.SetUniform1f("time", seconds);
	shader.SetUniform1i("diffuseMap", DiffuseSlot);

	static std::vector<Instance> visible;
	for (FoliageComponent& foliage : all)
	{
		if (!foliage.isScattered)
			Scatter(entityManager, foliage);
		if (foliage.places.empty() || foliage.modelName.empty())
			continue;
		FoliageDrawer* drawer = FindDrawer(foliage.modelName);
		if (!drawer)
			continue;

		// Only the copies near the camera; the farthest part of the distance shrinks them away
		visible.clear();
		const float reach = foliage.drawDistance;
		const float fadeFrom = reach * (1.0f - FadePart);
		const glm::vec2 eye = glm::vec2(sceneData.cameraPosition.x, sceneData.cameraPosition.z);
		// Where the camera looks, along the ground (the view matrix's third row is its backward axis)
		glm::vec2 viewForward = -glm::vec2(sceneData.viewMatrix[0][2], sceneData.viewMatrix[2][2]);
		viewForward = glm::length(viewForward) > 0.001f ? glm::normalize(viewForward) : glm::vec2(0.0f, -1.0f);
		for (size_t i = 0; i < foliage.places.size(); ++i)
		{
			const glm::vec4& place = foliage.places[i];
			const glm::vec2 toCopy = glm::vec2(place.x, place.z) - eye;
			const float distance = glm::length(toCopy);
			if (distance > reach)
				continue;
			// Outside the camera's view (behind it or far to a side): not drawn. Near ones always are (a tuft at
			// the edge of the screen must not vanish while its top still shows)
			if (distance > NearAlways && glm::dot(toCopy / distance, viewForward) < ViewCone)
				continue;
			const float shrink = distance > fadeFrom ? 1.0f - (distance - fadeFrom) / (reach - fadeFrom) : 1.0f;
			visible.push_back({ glm::vec4(glm::vec3(place), place.w * shrink), foliage.extras[i] });
		}
		if (visible.empty())
			continue;

		shader.SetUniform3f("tint", foliage.tint);
		shader.SetUniform1f("sway", foliage.sway);
		shader.SetUniform1f("swaySpeed", foliage.swaySpeed);
		shader.SetUniformBool("hasDiffuseMap", drawer->diffuse != nullptr);
		if (drawer->diffuse)
			drawer->diffuse->Bind(DiffuseSlot);
		drawer->drawer->Draw(visible.data(), (unsigned int)visible.size());
	}
}

}
