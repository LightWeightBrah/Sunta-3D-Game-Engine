#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>

namespace Sunta
{

class EntityManager;
class Shader;
class VertexArray;

// Moves and draws the specks of every Particle Emitter (see ParticleEmitterComponent)
//
// Each speck is a square that always faces the camera (a billboard), drawn with a round soft spot in the shader,
// its color added to the picture (additive blending) and without writing depth, so specks never hide each other
class ParticleSystem
{
public:
	// Call once per frame while the game plays, after the transforms (an emitter on a bone throws from where the bone is)
	static void Update(EntityManager& entityManager, float deltaTime);

	// Call after the solid models: the specks are hidden behind walls but never hide what is behind them
	static void Draw(EntityManager& entityManager, const glm::mat4& view, const glm::mat4& projection,
		const glm::vec3& cameraPosition, float fogDensity);

	// When the game starts and stops: no specks left over from the last time
	static void Clear(EntityManager& entityManager);

	// Throws 'count' specks of the emitter named 'emitterName' around 'position' (in the next Update), e.g. sparks where
	// a sword hit a shield. One emitter in the scene serves every hit. False when there is no such emitter
	static bool BurstAt(EntityManager& entityManager, const std::string& emitterName, const glm::vec3& position, int count);

private:
	static inline std::shared_ptr<VertexArray> quad;
	static inline std::shared_ptr<Shader>      shader;

	static void EnsureInitialized();
};

}
