#pragma once

namespace Sunta
{

class EntityManager;
enum class CharacterAction;

// Spends and refills the stamina of every entity with a StaminaComponent
class StaminaSystem
{
public:
	// Call when the game starts: every bar is full
	static void PrepareForPlay(EntityManager& entityManager);

	// Call once per frame AFTER the Character Controller and the Character Actions (running costs stamina, actions stop the refill)
	static void Update(EntityManager& entityManager, float deltaTime);

	// Takes the cost of the action. False (nothing taken) when the bar is empty. A character without stamina can always act
	static bool TrySpendForAction(EntityManager& entityManager, unsigned int entityID, CharacterAction action);

	// Takes the amount (a hit blocked with a shield). False when there was not enough: what was left is gone anyway
	// A character without stamina always has enough
	static bool TrySpend(EntityManager& entityManager, unsigned int entityID, float amount);

	// False only for a character whose stamina ran out (it can't run until some comes back)
	static bool HasStaminaLeft(EntityManager& entityManager, unsigned int entityID);
};

}
