#include "Core/SuntaPreCompiled.h"
#include "AudioSystem.h"

#include "Audio.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

// Sounds of the moment (a hit, a roar): a little different each time, loud in a fight, still heard across an arena
constexpr float MomentPitchVariation = 0.06f;
constexpr float MomentMinDistance    = 3.0f;
constexpr float MomentMaxDistance    = 60.0f;

// Where the entity stands in the world this frame (a child on a bone too)
glm::vec3 GetWorldPosition(EntityManager& entityManager, unsigned int entityID)
{
	if (const WorldMatrixComponent* worldMatrix = entityManager.GetComponent<WorldMatrixComponent>(entityID))
		return glm::vec3(worldMatrix->matrix[3]);

	const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);
	return transform ? transform->position : glm::vec3(0.0f);
}

void PutListenerIntoCamera(const glm::mat4& viewMatrix)
{
	const glm::mat4 cameraMatrix = glm::inverse(viewMatrix);

	const glm::vec3 position = glm::vec3(cameraMatrix[3]);
	const glm::vec3 forward  = -glm::vec3(cameraMatrix[2]);
	const glm::vec3 up       = glm::vec3(cameraMatrix[1]);

	Audio::SetListener(position, forward, up);
}

void StartSource(EntityManager& entityManager, unsigned int entityID, AudioSourceComponent& source)
{
	Audio::Stop(source.voice);

	Audio::PlayOptions options;
	options.volume      = source.volume;
	options.isLooping   = source.isLooping;
	options.isSpatial   = source.isSpatial;
	options.position    = GetWorldPosition(entityManager, entityID);
	options.minDistance = source.minDistance;
	options.maxDistance = source.maxDistance;

	source.voice = Audio::Play(source.sound, options);
}

}

void AudioSystem::PrepareForPlay(EntityManager& entityManager)
{
	for (AudioSourceComponent& source : entityManager.GetAllComponents<AudioSourceComponent>())
	{
		source.voice = Audio::NoVoice;
		Audio::Preload(source.sound);

		if (source.playOnStart)
			StartSource(entityManager, entityManager.GetEntityIDForComponent(source), source);
	}
}

void AudioSystem::Update(EntityManager& entityManager, const glm::mat4& viewMatrix)
{
	PutListenerIntoCamera(viewMatrix);

	for (AudioSourceComponent& source : entityManager.GetAllComponents<AudioSourceComponent>())
	{
		// A script may have changed the volume, a moving entity takes its sound along
		Audio::SetVolume(source.voice, source.volume);
		if (source.isSpatial)
			Audio::SetPosition(source.voice, GetWorldPosition(entityManager, entityManager.GetEntityIDForComponent(source)));
	}

	Audio::Update();
}

void AudioSystem::StopAll(EntityManager& entityManager)
{
	Audio::StopAll();

	for (AudioSourceComponent& source : entityManager.GetAllComponents<AudioSourceComponent>())
		source.voice = Audio::NoVoice;
}

void AudioSystem::Play(EntityManager& entityManager, unsigned int entityID)
{
	if (AudioSourceComponent* source = entityManager.GetComponent<AudioSourceComponent>(entityID))
		StartSource(entityManager, entityID, *source);
}

void AudioSystem::Stop(EntityManager& entityManager, unsigned int entityID, float fadeOutSeconds)
{
	if (AudioSourceComponent* source = entityManager.GetComponent<AudioSourceComponent>(entityID))
		Audio::Stop(source->voice, fadeOutSeconds);
}

bool AudioSystem::IsPlaying(EntityManager& entityManager, unsigned int entityID)
{
	const AudioSourceComponent* source = entityManager.GetComponent<AudioSourceComponent>(entityID);
	return source && Audio::IsPlaying(source->voice);
}

void AudioSystem::PlayAtEntity(EntityManager& entityManager, unsigned int entityID, const std::string& sound, float volume)
{
	if (sound.empty())
		return;

	Audio::PlayOptions options;
	options.volume         = volume;
	options.pitchVariation = MomentPitchVariation;
	options.isSpatial      = true;
	options.position       = GetWorldPosition(entityManager, entityID);
	options.minDistance    = MomentMinDistance;
	options.maxDistance    = MomentMaxDistance;

	Audio::Play(sound, options);
}

}
