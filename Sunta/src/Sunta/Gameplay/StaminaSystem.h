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

	// False only for a character whose stamina ran out (it can't run until some comes back)
	static bool HasStaminaLeft(EntityManager& entityManager, unsigned int entityID);
};

}
