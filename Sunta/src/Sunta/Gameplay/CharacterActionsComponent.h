#pragma once

#include <string>
#include <glm/glm.hpp>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Something a character does that takes over its body for a moment: it can't walk freely until it is over
enum class CharacterAction
{
	None,
	Roll,        // a dodge in the direction of the stick, invulnerable for a part of it
	Backstep,    // a short hop back, used when rolling without choosing a direction
	LightAttack, // quick attacks that chain into a combo when pressed again in time
	HeavyAttack, // a slow, strong attack
	HitReaction, // staggering after being hit (started by the Health, not by a button)
	Death        // the last animation, the character never acts again
};

// Rolls and attacks of a character, no script needed (the Dark Souls style of combat)
//
// How to use it: in the editor choose Add Component > Character Actions on a character that has a Character Controller,
// a Model with animations and an Animator. Animations whose names contain roll, backstep, light_1/2/3 and heavy
// are chosen for you, the others are picked from the dropdowns in the Inspector
// After pressing Play: a tap of Left Shift rolls (a backstep when no direction is held; holding it runs), left mouse button = light attack
// (press again during the attack to chain the combo), right mouse button = heavy attack
//
// The action plays its animation once. While it plays, the character moves on its own (the roll carries it forward,
// an attack makes a small step) and turns towards the locked-on enemy or the held direction when the action starts
//
// A script (or an enemy) can ask for an action instead of the keyboard: turn 'isPlayerControlled' off
// on the Character Controller and set 'requestedAction'
//
// Attacks hurt through the Hitbox of the weapon during their striking part ('lightHitFrom'..'lightHitTo' and the heavy one).
// With a Stamina component every action costs stamina, and with none left it doesn't start
struct CharacterActionsComponent
{
	// Limits of the sliders in the Inspector
	static constexpr float MaxSpeed        = 15.0f;
	static constexpr float MaxBlendSeconds = 0.5f;
	static constexpr float MaxDamageMultiplier = 5.0f;

	// Names of the animations of the Model, empty means "this action is not available"
	std::string rollAnimation;
	std::string backstepAnimation;
	std::string lightAttack1Animation;
	std::string lightAttack2Animation;
	std::string lightAttack3Animation;
	std::string heavyAttackAnimation;
	std::string hitReactionAnimation;
	std::string deathAnimation;

	// How fast the actions move the character (meters per second)
	float rollSpeed       = 5.5f;
	float backstepSpeed   = 4.0f;
	float attackStepSpeed = 1.5f;

	// Parts of an action are given as fractions of its animation: 0 is the start, 1 is the end
	// The part of the roll and the backstep during which nothing can hurt the character
	float invulnerableFrom = 0.1f;
	float invulnerableTo   = 0.55f;

	// The striking part of the attacks: the weapon's Hitbox hurts only then
	float lightHitFrom = 0.28f;
	float lightHitTo   = 0.56f;
	float heavyHitFrom = 0.45f;
	float heavyHitTo   = 0.66f;

	// How much more a heavy attack hurts than a light one
	float heavyDamageMultiplier = 1.8f;

	// A button pressed after this part of an action is remembered and starts the next action (no need for perfect timing)
	float inputBufferFrom = 0.3f;

	// After this part an action can be interrupted: by the remembered next action or by walking away
	float cancelFrom = 0.75f;

	// How long the animation of an action takes to fade in
	float blendSeconds = 0.08f;

	// ========== Filled while the game plays, not saved with the scene ==========

	// What a script (or the keyboard) asks for this frame. Cleared once it is read
	CharacterAction requestedAction = CharacterAction::None;

	CharacterAction currentAction = CharacterAction::None;
	CharacterAction queuedAction  = CharacterAction::None;

	// Which light attack of the combo plays now (0, 1 or 2)
	int lightAttackStep = 0;

	// The direction the current action moves the character in (in the world, horizontal)
	glm::vec3 actionDirection = glm::vec3(0.0f, 0.0f, -1.0f);

	// True during the invulnerable part of a roll or a backstep
	bool isInvulnerable = false;

	// How far the current action is (0 = start, 1 = end of its animation)
	float actionProgress = 0.0f;

	// Grows with every attack, so a weapon knows when a new swing started (and may hit the same enemy again)
	unsigned int attackNumber = 0;

	bool IsBusy() const { return currentAction != CharacterAction::None; }

	bool IsAttacking() const { return currentAction == CharacterAction::LightAttack || currentAction == CharacterAction::HeavyAttack; }

	// True during the part of the current attack that hurts
	bool IsInHitWindow() const
	{
		if (currentAction == CharacterAction::LightAttack)
			return actionProgress >= lightHitFrom && actionProgress <= lightHitTo;

		if (currentAction == CharacterAction::HeavyAttack)
			return actionProgress >= heavyHitFrom && actionProgress <= heavyHitTo;

		return false;
	}

	float GetDamageMultiplier() const { return currentAction == CharacterAction::HeavyAttack ? heavyDamageMultiplier : 1.0f; }

	static void RegisterToInspector()
	{
		// The Inspector draws it with a drawer (dropdowns of the animations and sliders), so there is no list of properties
		InspectorComponentRegistry::RegisterComponent<CharacterActionsComponent>("Character Actions", EngineAssets::Icons::AnimationController, { });
	}
};

}
