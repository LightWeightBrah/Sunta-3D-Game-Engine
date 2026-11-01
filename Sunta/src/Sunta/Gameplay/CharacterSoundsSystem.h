#pragma once

#include <string>

namespace Sunta
{

class EntityManager;
struct CharacterSoundsComponent;

// Plays the sounds of characters (see CharacterSoundsComponent): the fight calls Play at its moments,
// Update follows the animation: the swoosh of an attack just before it hits, a step when a foot touches the ground
class CharacterSoundsSystem
{
public:
	using Sound = std::string CharacterSoundsComponent::*;

	// When the game starts: every sound decoded in advance, nobody is in the middle of a step
	static void PrepareForPlay(EntityManager& entityManager);

	// Once per frame while the game plays, after the transforms (the feet are where this frame's pose put them)
	static void Update(EntityManager& entityManager, float deltaTime);

	// E.g. Play(entityManager, targetID, &CharacterSoundsComponent::hurtSound). Nothing without Character Sounds
	static void Play(EntityManager& entityManager, unsigned int entityID, Sound sound);
};

}
