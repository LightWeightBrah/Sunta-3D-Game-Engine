#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;

// Plays the Audio Sources of the scene (see AudioSourceComponent) and puts the ears of the game into its camera
class AudioSystem
{
public:
	// When the game starts: decodes every sound of the scene in advance and starts the sources with 'playOnStart'
	static void PrepareForPlay(EntityManager& entityManager);

	// Once per frame while the game plays, after the camera: the ears are where the picture is seen from,
	// spatial sources follow their entities
	static void Update(EntityManager& entityManager, const glm::mat4& viewMatrix);

	// When the game stops: silence, every voice is freed
	static void StopAll(EntityManager& entityManager);

	static void Play(EntityManager& entityManager, unsigned int entityID);
	static void Stop(EntityManager& entityManager, unsigned int entityID, float fadeOutSeconds = 0.0f);
	static bool IsPlaying(EntityManager& entityManager, unsigned int entityID);

	// A sound played once where the entity is (a hit on a character). 'volume' 1 = as recorded
	static void PlayAtEntity(EntityManager& entityManager, unsigned int entityID, const std::string& sound, float volume = 1.0f);
};

}
