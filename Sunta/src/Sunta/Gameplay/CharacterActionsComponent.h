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
	LeapAttack,  // a jump at a target further away that ends in a blow as strong as a heavy attack (closing the gap)
	HitReaction, // staggering after being hit (started by the Health, not by a button)
	Death,       // the last animation, the character never acts again (until it is brought back at a bonfire)
	Rest,        // kneeling at a bonfire, ends when the bonfire stands the character up
	StandUp,     // getting up after a rest
	Block,       // holding the shield up (needs a Shield), lasts while the block key is held
	BlockImpact, // a hit taken on the shield
	Parry,       // a quick turn of the shield: an attack that lands early in it is turned away (see 'parryFrom')
	Parried,     // the long stagger of an attacker whose attack was parried: its next hit taken is a riposte
	Special,     // any animation played once by name (a roar, a taunt), started by a script or a boss;
	             // hits still hurt but don't stagger the character (it can't be interrupted)
	UseItem      // drinking from a flask or using any item: an animation played once on the upper body (as the block),
	             // the character walks slowly meanwhile. A roll or an attack can't cut it short, a hit can
};

// Rolls and attacks of a character, no script needed (the Dark Souls style of combat)
//
// How to use it: in the editor choose Add Component > Character Actions on a character that has a Character Controller,
// a Model with animations and an Animator. Animations whose names contain roll, backstep, light_1/2/3 and heavy
// are chosen for you, the others are picked from the dropdowns in the Inspector
// After pressing Play: a tap of Left Shift rolls (a backstep when no direction is held; holding it runs), left mouse button = light attack
// (press again during the attack to chain the combo), Left Ctrl + left mouse button = heavy attack,
// holding the right mouse button blocks and a tap of F parries (both need a Shield on the character, see ShieldComponent)
// The block plays on the upper body only (from 'upperBodyBone' up): the legs keep walking, so the character can move
// behind its shield (walking, not running)
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
	std::string leapAttackAnimation;
	std::string hitReactionAnimation;
	std::string deathAnimation;
	std::string restAnimation;
	std::string standUpAnimation;
	std::string blockAnimation;
	std::string blockImpactAnimation;
	std::string parryAnimation;
	std::string parriedAnimation;

	// A shockwave where a heavy attack or a leap lands (0 radius = none): everyone of the other side within
	// 'shockwaveRadius' of the point 'shockwaveDistance' in front of the character takes 'shockwaveDamage' (a roll dodges it)
	// when the attack starts to hit. The Particle Emitter of the child named 'shockwaveEffect' bursts there
	float       shockwaveRadius   = 0.0f;
	float       shockwaveDamage   = 30.0f;
	float       shockwaveDistance = 2.5f;
	std::string shockwaveEffect;
	// How hard the shockwave shakes the camera (0..1) when the player stands where it lands, fading to nothing
	// at three times its radius
	float       shockwaveCameraShake = 0.6f;

	// How fast an attack keeps turning towards its target (degrees per second) until it starts to hit: a boss tracks
	// the player during its wind-up and commits only for the blow itself, so walking around it doesn't dodge it.
	// 0 = the attack commits to its direction when it starts (the player's attacks)
	float attackTurnDegreesPerSecond = 0.0f;

	// An attack steps in to reach its target: up to this fast (meters per second) during its step, just enough to end
	// 'attackReach' meters from the target (a boss never swings at the air in front of a player who backed off).
	// 0 = every attack steps at 'attackStepSpeed'
	float attackLungeSpeed = 0.0f;
	float attackReach      = 1.8f;

	// A delayed blow (the cruel timing of the Souls bosses): with this chance (0..1) an attack holds still in its wind-up
	// for 'attackDelaySeconds', at 'attackDelayAt' of the way to its hit (keeps tracking its target meanwhile),
	// then strikes. A player who rolls on the usual rhythm is caught
	float attackDelayChance  = 0.0f;
	float attackDelaySeconds = 0.5f;
	float attackDelayAt      = 0.7f;

	// The bone where the upper body starts: an animation of the upper body only (the block) moves it and the bones above it
	std::string upperBodyBone = "Spine";

	// How fast the actions move the character (meters per second)
	float rollSpeed       = 5.5f;
	float backstepSpeed   = 4.0f;
	float attackStepSpeed = 1.5f;
	// The fastest a leap may fly: it flies just far enough to land in front of its target (the locked-on one)
	float leapSpeed       = 8.0f;

	// Parts of an action are given as fractions of its animation: 0 is the start, 1 is the end
	// The part of the roll and the backstep during which nothing can hurt the character
	float invulnerableFrom = 0.1f;
	float invulnerableTo   = 0.55f;

	// The striking part of the attacks: the weapon's Hitbox hurts only then
	float lightHitFrom = 0.28f;
	float lightHitTo   = 0.56f;
	float heavyHitFrom = 0.45f;
	float heavyHitTo   = 0.66f;
	float leapHitFrom  = 0.4f;
	float leapHitTo    = 0.65f;

	// How much more a heavy attack hurts than a light one
	float heavyDamageMultiplier = 1.8f;

	// The part of the parry during which an attack that lands is turned away
	float parryFrom = 0.0f;
	float parryTo   = 0.4f;

	// How much more an attack hurts a character that was just parried (a riposte)
	float riposteDamageMultiplier = 3.0f;

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

	// How fast the current leap flies (chosen when it starts, from the distance to the target)
	float currentLeapSpeed = 0.0f;

	// How fast the current attack steps in (chosen when it starts, see attackLungeSpeed)
	float currentAttackStepSpeed = 0.0f;

	// The hold of the current attack's wind-up still to come (seconds), and whether it is going on now
	float attackDelayLeft = 0.0f;
	bool  isHoldingAttack = false;

	// The animation of the current Special action
	std::string specialAnimation;

	// The animation of the current UseItem action
	std::string itemAnimation;

	// True during the invulnerable part of a roll or a backstep
	bool isInvulnerable = false;

	// How far the current action is (0 = start, 1 = end of its animation)
	float actionProgress = 0.0f;

	// Grows with every attack, so a weapon knows when a new swing started (and may hit the same enemy again)
	unsigned int attackNumber = 0;

	// The attack whose shockwave already happened (one per attack)
	unsigned int shockwaveAttackNumber = 0;

	bool IsBusy() const { return currentAction != CharacterAction::None; }

	bool IsResting() const { return currentAction == CharacterAction::Rest; }

	bool IsUsingItem() const { return currentAction == CharacterAction::UseItem; }

	bool IsAttacking() const
	{
		return currentAction == CharacterAction::LightAttack || currentAction == CharacterAction::HeavyAttack || currentAction == CharacterAction::LeapAttack;
	}

	// The shield is up: blocking, or taking a hit on it
	bool IsGuarding() const { return currentAction == CharacterAction::Block || currentAction == CharacterAction::BlockImpact; }

	// True during the part of a parry that turns attacks away
	bool IsParrying() const { return currentAction == CharacterAction::Parry && actionProgress >= parryFrom && actionProgress <= parryTo; }

	// Staggering after its attack was parried: the next hit it takes is a riposte
	bool IsOpenToRiposte() const { return currentAction == CharacterAction::Parried; }

	// Where the part of the current attack that hurts starts (0..1 of the attack), below 0 when no attack is going on
	float GetHitFrom() const
	{
		switch (currentAction)
		{
		case CharacterAction::LightAttack: return lightHitFrom;
		case CharacterAction::HeavyAttack: return heavyHitFrom;
		case CharacterAction::LeapAttack:  return leapHitFrom;
		default:                           return -1.0f;
		}
	}

	// True during the part of the current attack that hurts
	bool IsInHitWindow() const
	{
		if (currentAction == CharacterAction::LightAttack)
			return actionProgress >= lightHitFrom && actionProgress <= lightHitTo;

		if (currentAction == CharacterAction::HeavyAttack)
			return actionProgress >= heavyHitFrom && actionProgress <= heavyHitTo;

		if (currentAction == CharacterAction::LeapAttack)
			return actionProgress >= leapHitFrom && actionProgress <= leapHitTo;

		return false;
	}

	// A leap ends in a heavy blow
	float GetDamageMultiplier() const
	{
		return (currentAction == CharacterAction::HeavyAttack || currentAction == CharacterAction::LeapAttack) ? heavyDamageMultiplier : 1.0f;
	}

	static void RegisterToInspector()
	{
		// The Inspector draws it with a drawer (dropdowns of the animations and sliders), so there is no list of properties
		InspectorComponentRegistry::RegisterComponent<CharacterActionsComponent>("Character Actions", EngineAssets::Icons::AnimationController, { });
	}
};

}
