#include "Core/SuntaPreCompiled.h"
#include "TerrainSystem.h"

#include <stb/stb_image.h>

#include "TerrainComponent.h"
#include "Core/Log.h"
#include "Core/VirtualFileSystem.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

void LoadHeightmap(TerrainComponent& terrain)
{
	if (terrain.loadedPath == terrain.heightmap && !terrain.heights.empty())
		return;

	terrain.heights.clear();
	terrain.columns = terrain.rows = 0;
	terrain.loadedPath = terrain.heightmap;
	if (terrain.heightmap.empty())
		return;

	const std::string path = VirtualFileSystem::Resolve(terrain.heightmap);
	int width = 0, height = 0, channels = 0;
	stbi_us* pixels = stbi_load_16(path.c_str(), &width, &height, &channels, 1);
	if (!pixels)
	{
		SUNTA_ENGINE_LOG_WARNING("Terrain: can't read the heightmap '{0}'", terrain.heightmap);
		return;
	}

	terrain.columns = width;
	terrain.rows    = height;
	terrain.heights.resize(static_cast<size_t>(width) * height);
	for (size_t i = 0; i < terrain.heights.size(); i++)
		terrain.heights[i] = pixels[i] / 65535.0f;
	stbi_image_free(pixels);
	SUNTA_ENGINE_LOG_INFO("Terrain: heightmap '{0}' {1}x{2}", terrain.heightmap, width, height);
}

// Bilinear: smooth between the pixels. The picture's top row is the far (-Z) edge, its left column -X
bool SampleHeight(const TerrainComponent& terrain, const glm::vec3& origin, float x, float z, float& outHeight)
{
	if (terrain.heights.empty())
		return false;

	const float u = (x - origin.x) / terrain.sizeX + 0.5f;
	const float v = (z - origin.z) / terrain.sizeZ + 0.5f;
	if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
		return false;

	const float column = u * static_cast<float>(terrain.columns - 1);
	const float row    = v * static_cast<float>(terrain.rows - 1);
	const int c0 = static_cast<int>(column), r0 = static_cast<int>(row);
	const int c1 = std::min(c0 + 1, terrain.columns - 1), r1 = std::min(r0 + 1, terrain.rows - 1);
	const float fc = column - static_cast<float>(c0), fr = row - static_cast<float>(r0);

	auto at = [&](int c, int r) { return terrain.heights[static_cast<size_t>(r) * terrain.columns + c]; };
	const float top    = glm::mix(at(c0, r0), at(c1, r0), fc);
	const float bottom = glm::mix(at(c0, r1), at(c1, r1), fc);
	outHeight = origin.y + glm::mix(top, bottom, fr) * terrain.heightScale;
	return true;
}

}

void TerrainSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (TerrainComponent& terrain : entityManager.GetAllComponents<TerrainComponent>())
		LoadHeightmap(terrain);
}

bool TerrainSystem::TryGetHeight(EntityManager& entityManager, float x, float z, float& outHeight)
{
	bool isFound = false;
	for (TerrainComponent& terrain : entityManager.GetAllComponents<TerrainComponent>())
	{
		const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityManager.GetEntityIDForComponent(terrain));
		float height = 0.0f;
		if (transform && SampleHeight(terrain, transform->position, x, z, height) && (!isFound || height > outHeight))
		{
			outHeight = height;
			isFound   = true;
		}
	}
	return isFound;
}

void TerrainSystem::KeepBodiesOnGround(EntityManager& entityManager)
{
	if (entityManager.GetAllComponents<TerrainComponent>().empty())
		return;

	for (PhysicsBodyComponent& body : entityManager.GetAllComponents<PhysicsBodyComponent>())
	{
		if (body.isKinematic)
			continue;
		const unsigned int entityID = entityManager.GetEntityIDForComponent(body);
		TransformComponent*         transform = entityManager.GetComponent<TransformComponent>(entityID);
		const BoxColliderComponent* collider  = entityManager.GetComponent<BoxColliderComponent>(entityID);
		if (!transform || !collider || collider->isTrigger)
			continue;

		float ground = 0.0f;
		if (!TryGetHeight(entityManager, transform->position.x, transform->position.z, ground))
			continue;

		// The bottom of the body's box (bodies stand upright)
		const float bottom = collider->worldOBB.center.y - collider->worldOBB.halfExtents.y;
		if (bottom >= ground)
			continue;

		transform->position.y += ground - bottom;
		transform->isDirty = true;
		if (body.velocity.y < 0.0f)
			body.velocity.y = 0.0f;
	}
}

}
