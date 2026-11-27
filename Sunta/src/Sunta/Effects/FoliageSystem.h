#pragma once

namespace Sunta
{

class EntityManager;
struct SceneData;

// Scatters and draws the Foliage of a scene (see FoliageComponent)
class FoliageSystem
{
public:
	// Call when the game starts (after the terrain is ready): every Foliage scatters its copies once
	static void PrepareForPlay(EntityManager& entityManager);

	// Call after the solid things: the copies near the camera, swaying, drawn with the scene's light and fog
	static void Draw(EntityManager& entityManager, const SceneData& sceneData, float seconds);
};

}
