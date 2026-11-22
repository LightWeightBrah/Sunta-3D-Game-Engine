#pragma once

#include <string>
#include <vector>
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
	UseItem,     // drinking from a flask or using any item: an animation played once on the upper body (as the block),
	             // the character walks slowly meanwhile. A roll or an attack can't cut it short, a hit can
	Cast,        // casting a spell (see SpellCasterComponent): an animation played once by name, turning after the target
	             // until the spell leaves. A roll or an attack can't cut it short, a hit can (the spell is lost)
	Jump,        // off the ground (Space): up, carried along the held direction, over when the feet touch the ground again
	JumpAttack   // an attack pressed in the air: a blow on the way down, or (heavy) a plunge into the ground that ends in
	             // the shockwave of the character, when it has one
};

// One attack animation with timing of its own: blows of one combo land at different moments (a jab early, a hook late),
// so each says when it hurts and how hard. An attack whose animation has no move uses the light/heavy windows
struct AttackMove
{
	std::string animation;
	float hitFrom          = 0.3f;
	float hitTo            = 0.6f;
	float damageMultiplier = 1.0f;
	// After this part the attack may be cut short by the next one (below 0: the character's 'cancelFrom'). A heavy
	// blow ends later, so two of them pressed fast don't blend into each other
	float cancelFrom       = -1.0f;
	// Where this animation strikes by itself (meters above the feet), for the Strike Aim; 0 = the Strike Aim's own height
	float strikeHeight     = 0.0f;
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
	static constexpr float MinAttackSpeed      = 0.25f;
	static constexpr float MaxAttackSpeed      = 3.0f;

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

	// ========== Jumping (all optional: without 'jumpAnimation' the character doesn't jump) ==========

	std::string jumpAnimation;
	// An attack pressed in the air (light), and the heavy one: a plunge straight down onto the ground
	std::string jumpAttackAnimation;
	std::string jumpHeavyAttackAnimation;
	// How fast the jump leaves the ground (meters per second up: 5 is about a meter and a quarter high), how fast it
	// carries the character along the held direction, and how fast the heavy jump attack drives it down
	float jumpSpeed      = 5.0f;
	float jumpMoveSpeed  = 4.0f;
	float jumpPlungeSpeed = 14.0f;
	// When the plunge of the heavy jump attack starts going down (0..1 of its animation; before it the character
	// hangs in the air for its wind-up)
	float jumpPlungeFrom = 0.25f;
	// How much more the attacks from a jump hurt than a light attack (the heavy one: also the heavy multiplier)
	float jumpAttackDamageMultiplier = 1.3f;
	// Where the plunge lands: a shockwave of its own (0 radius = none) and an effect of the scene (see EffectSystem)
	float       jumpPlungeShockwaveRadius = 0.0f;
	float       jumpPlungeShockwaveDamage = 30.0f;
	std::string jumpPlungeEffect;

	// ========== More ways to attack (all optional: an empty name turns that one off) ==========

	// The combo goes on past the third light attack with these, in order, then starts over
	std::vector<std::string> extraLightAttackAnimations;
	// A heavy attack pressed again during a heavy one: the follow-up blow (then back to the first)
	std::string heavyAttack2Animation;
	// An attack while running: a lunge that carries the character 'runningAttackSpeed' meters per second into it
	std::string runningAttackAnimation;
	float       runningAttackSpeed = 7.0f;
	// An attack pressed during a roll or a backstep: it comes out of the dodge
	std::string dodgeAttackAnimation;
	// An attack at an enemy staggering after a parry (within 'riposteReach' meters, in front): the finishing blow
	// (it hurts 'riposteDamageMultiplier' times more either way)
	std::string riposteAnimation;
	float       riposteReach = 2.5f;
	// Timing and strength of single attack animations (see AttackMove)
	std::vector<AttackMove> attackMoves;

	// The stagger after a hit, chosen at random among the hit reaction and these; a heavy blow (a heavy attack, a leap)
	// plays 'heavyHitReactionAnimation' when there is one
	std::vector<std::string> extraHitReactionAnimations;
	std::string heavyHitReactionAnimation;

	// Thrown back by a blow (a hitbox's 'knockback', or a script): this animation (sliding back on the feet, catching
	// itself, standing up - or the heavy hit reaction when empty), moving back the blow's meters over 'knockbackSeconds',
	// fast at first and slowing down, the Particle Emitter of the scene named 'knockbackDust' bursting at the feet
	std::string knockbackAnimation;
	float       knockbackSeconds = 0.7f;
	std::string knockbackDust;

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

	// Hits don't stagger the character while it attacks (hyper armor, as a souls boss's blows): 0 never, 1 during
	// heavy attacks and leaps, 2 during every attack. Its health still drops: trading blows with it costs the attacker
	int hyperArmor = 0;

	// An effect of the scene (see EffectSystem) played the moment each attack starts to hit, in front of the
	// character at chest height, facing where it strikes: the arc of a slash, the blur of a fist, a gust
	// 'heavySwingEffect' for heavy attacks and leaps (empty: the same). Empty: none
	std::string swingEffect;
	std::string heavySwingEffect;
	float       swingEffectHeight   = 1.2f;
	float       swingEffectDistance = 0.6f;
	// When in the hit window it appears: 0 as the window opens, 1 as it closes (the moment the blow really flies)
	float       swingEffectAt       = 0.25f;

	// How fast an attack keeps turning towards its target (degrees per second) until it starts to hit: a boss tracks
	// the player during its wind-up and commits only for the blow itself, so walking around it doesn't dodge it.
	// 0 = the attack commits to its direction when it starts (the player's attacks)
	float attackTurnDegreesPerSecond = 0.0f;

	// An attack steps in to reach its target: up to this fast (meters per second) during its step, just enough to end
	// 'attackReach' meters from the target (a boss never swings at the air in front of a player who backed off).
	// 0 = every attack steps at 'attackStepSpeed'
	float attackLungeSpeed = 0.0f;
	float attackReach      = 1.8f;
	// The farthest a lunge carries the character (meters): a punch thrown from too far stays a short step forward
	// instead of a slide across the floor
	float attackLungeMaxDistance = 100.0f;

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

	// How fast the attacks play (1 = as the animations were made): quick hands for a boxer, and a script can raise it
	// during a fight (a boss that speeds up in its last phase: this.characterActions.attackSpeed = 1.3)
	float attackSpeed = 1.0f;

	// The part of the parry during which an attack that lands is turned away
	float parryFrom = 0.0f;
	float parryTo   = 0.4f;

	// How much more an attack hurts a character that was just parried (a riposte)
	float riposteDamageMultiplier = 3.0f;

	// A button pressed after this part of an action is remembered and starts the next action (no need for perfect timing)
	float inputBufferFrom = 0.3f;

	// After this part an action can be interrupted: by the remembered next action or by walking away
	float cancelFrom = 0.75f;
	// A roll or a backstep can't be ended by walking away before this part (a held direction would cut the roll
	// short and snap the character back to walking); the next action may still come after 'cancelFrom'
	float dodgeCancelFrom = 0.0f;
	// After this part of a stagger (a hit reaction) a roll or a backstep asked for cuts it short: the player is not
	// pinned by every blow until its whole animation is over (1 = a stagger always plays to its end)
	float hitReactionDodgeFrom = 1.0f;

	// How long the animation of an action takes to fade in
	float blendSeconds = 0.08f;

	// ========== Filled while the game plays, not saved with the scene ==========

	// What a script (or the keyboard) asks for this frame. Cleared once it is read
	CharacterAction requestedAction = CharacterAction::None;

	CharacterAction currentAction = CharacterAction::None;
	CharacterAction queuedAction  = CharacterAction::None;

	// Which light attack of the combo plays now (0, 1, 2, then the extra ones)
	int lightAttackStep = 0;
	// Which heavy attack plays now (0 the first, 1 the follow-up)
	int heavyAttackStep = 0;

	// The animation the current action plays (attacks pick it when they start: the combo's next blow, a riposte, ...)
	std::string currentAnimation;
	// The stagger chosen for the next hit reaction (see extraHitReactionAnimations)
	std::string chosenHitReaction;
	// The current stagger is a knockback: the speed it started with (meters a second), the dust still to come
	float knockbackStartSpeed = 0.0f;
	float knockbackElapsed    = 0.0f;
	float knockbackDustIn     = 0.0f;
	// The current attack started from a run (it lunges at 'runningAttackSpeed')
	bool isRunningAttack = false;

	// The direction the current action moves the character in (in the world, horizontal)
	glm::vec3 actionDirection = glm::vec3(0.0f, 0.0f, -1.0f);

	// How fast the current leap flies (chosen when it starts, from the distance to the target)
	float currentLeapSpeed = 0.0f;

	// The attack whose swing effect has been played (see swingEffect)
	unsigned int swingEffectAttackNumber = 0;

	// How fast the current attack steps in (chosen when it starts, see attackLungeSpeed)
	float currentAttackStepSpeed = 0.0f;

	// The hold of the current attack's wind-up still to come (seconds), and whether it is going on now
	float attackDelayLeft = 0.0f;
	bool  isHoldingAttack = false;

	// The animation of the current Special action
	std::string specialAnimation;

	// In the air (a jump or an attack from it), for how long, and the vertical speed of the last frame (a landing
	// is the fall stopped by the ground). The current jump attack is the heavy plunge
	bool  isAirborne = false;
	float airSeconds = 0.0f;
	float lastVerticalSpeed = 0.0f;
	bool  isPlunging = false;
	// The plunge reached the ground in this attack (its shockwave goes off once)
	bool  hasPlungeLanded = false;
	// How fast the current jump moves along the ground (chosen when it starts: 0 when no direction was held)
	float currentJumpMoveSpeed = 0.0f;

	// The animation of the current UseItem action
	std::string itemAnimation;

	// The animation of the current Cast action, and when its spell leaves (0..1): the caster turns after its target until then
	std::string castAnimation;
	float       castReleaseAt = 0.5f;
	// The current cast's spell is uninterruptible (SpellCasterComponent::isUninterruptible): hits don't stop it
	bool        isCastUninterruptible = false;

	// True during the invulnerable part of a roll or a backstep
	bool isInvulnerable = false;

	// The block was asked for in this frame (the key is held, or a script keeps asking)
	bool isBlockHeld = false;

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
		return currentAction == CharacterAction::LightAttack || currentAction == CharacterAction::HeavyAttack || currentAction == CharacterAction::LeapAttack
		    || currentAction == CharacterAction::JumpAttack;
	}

	// The shield is up: blocking, or taking a hit on it while the block is still asked for (a character that let go of
	// the block during the impact is open to the next blow: nobody stays behind a shield just by being hit)
	bool IsGuarding() const
	{
		return currentAction == CharacterAction::Block || (currentAction == CharacterAction::BlockImpact && isBlockHeld);
	}

	// True during the part of a parry that turns attacks away
	bool IsParrying() const { return currentAction == CharacterAction::Parry && actionProgress >= parryFrom && actionProgress <= parryTo; }

	// Staggering after its attack was parried: the next hit it takes is a riposte
	bool IsOpenToRiposte() const { return currentAction == CharacterAction::Parried; }

	// The timing of the attack playing now, when its animation has one (see AttackMove)
	const AttackMove* FindCurrentMove() const
	{
		if (!IsAttacking())
			return nullptr;
		for (const AttackMove& move : attackMoves)
			if (move.animation == currentAnimation)
				return &move;
		return nullptr;
	}

	// Where the part of the current attack that hurts starts (0..1 of the attack), below 0 when no attack is going on
	float GetHitFrom() const
	{
		if (const AttackMove* move = FindCurrentMove())
			return move->hitFrom;
		switch (currentAction)
		{
		case CharacterAction::LightAttack: return lightHitFrom;
		case CharacterAction::HeavyAttack: return heavyHitFrom;
		case CharacterAction::LeapAttack:  return leapHitFrom;
		case CharacterAction::JumpAttack:  return leapHitFrom;
		default:                           return -1.0f;
		}
	}

	// Hyper armor now (see hyperArmor)
	bool HasHyperArmor() const
	{
		if (currentAction == CharacterAction::Cast && isCastUninterruptible)
			return true;
		if (hyperArmor >= 2)
			return IsAttacking();
		return hyperArmor == 1 && (currentAction == CharacterAction::HeavyAttack || currentAction == CharacterAction::LeapAttack
		                           || (currentAction == CharacterAction::JumpAttack && isPlunging));
	}

	// Where it ends (0..1 of the attack), below 0 when no attack is going on
	float GetHitTo() const
	{
		if (const AttackMove* move = FindCurrentMove())
			return move->hitTo;
		switch (currentAction)
		{
		case CharacterAction::LightAttack: return lightHitTo;
		case CharacterAction::HeavyAttack: return heavyHitTo;
		case CharacterAction::LeapAttack:  return leapHitTo;
		case CharacterAction::JumpAttack:  return leapHitTo;
		default:                           return -1.0f;
		}
	}

	// True during the part of the current attack that hurts
	bool IsInHitWindow() const
	{
		if (const AttackMove* move = FindCurrentMove())
			return actionProgress >= move->hitFrom && actionProgress <= move->hitTo;

		if (currentAction == CharacterAction::LightAttack)
			return actionProgress >= lightHitFrom && actionProgress <= lightHitTo;

		if (currentAction == CharacterAction::HeavyAttack)
			return actionProgress >= heavyHitFrom && actionProgress <= heavyHitTo;

		if (currentAction == CharacterAction::LeapAttack || currentAction == CharacterAction::JumpAttack)
			return actionProgress >= leapHitFrom && actionProgress <= leapHitTo;

		return false;
	}

	// A leap ends in a heavy blow
	float GetDamageMultiplier() const
	{
		if (const AttackMove* move = FindCurrentMove())
			return move->damageMultiplier;
		if (currentAction == CharacterAction::JumpAttack)
			return jumpAttackDamageMultiplier * (isPlunging ? heavyDamageMultiplier : 1.0f);
		return (currentAction == CharacterAction::HeavyAttack || currentAction == CharacterAction::LeapAttack) ? heavyDamageMultiplier : 1.0f;
	}

	static void RegisterToInspector()
	{
		// The Inspector draws it with a drawer (dropdowns of the animations and sliders), so there is no list of properties
		InspectorComponentRegistry::RegisterComponent<CharacterActionsComponent>("Character Actions", EngineAssets::Icons::AnimationController, { });
	}
};

}
