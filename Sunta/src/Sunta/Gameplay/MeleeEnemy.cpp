#include "Core/SuntaPreCompiled.h"
#include "MeleeEnemy.h"
#include "CharacterSoundsSystem.h"
#include "CharacterSoundsComponent.h"

#include "BaseEnemyStates.h"
#include "EnemyContext.h"
#include "EnemyStateMachine.h"
#include "EnemySystem.h"
#include "GameplayMath.h"
#include "SpellSystem.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"

namespace Sunta
{

namespace
{

using namespace MeleeEnemyStateNames;

// ========== Numbers used below ==========

// Closer to home than this counts as home
constexpr float ArrivedHomeDistance = 0.5f;

// The next light attack of a combo is asked for only while the player is still about this close (times the attack distance)
constexpr float ComboReachFactor = 1.3f;

// An attack that didn't start within this time (no stamina, no animation) is given up
constexpr float AttackStartGraceSeconds = 0.2f;

// Walking home, the enemy turns back only when the player is this much closer than the give-up distance (no flickering at the border)
constexpr float SeeAgainFactor = 0.8f;

// Chasing: a running enemy slows to a walk only this much (meters) inside its run distance, and an enemy that stopped
// to attack starts walking again only this much outside its attack distance. Without the margin a player standing
// at the border makes it switch walk/run (or walk/stand) every frame, and the weapon jumps between the poses of the two
constexpr float RunMargin  = 1.5f;
constexpr float StopMargin = 0.6f;

// A spell this close to landing (seconds) is dodged now: the roll is already untouchable when it arrives
constexpr float DodgeSpellSeconds = 0.3f;

// A spell is dodged with a roll to the side this often (the rest are backsteps), a blow this often
constexpr float SideRollChanceFromSpell = 0.75f;
constexpr float SideRollChanceFromBlow  = 0.35f;

// ========== Small helpers ==========

// The states are added only to enemies with the component, but a script can switch any enemy to them
const MeleeEnemyComponent& GetSettings(const EnemyContext& context)
{
	static const MeleeEnemyComponent defaultSettings;

	const MeleeEnemyComponent* settings = context.TryGet<MeleeEnemyComponent>();
	return settings ? *settings : defaultSettings;
}

float RandomSide()
{
	return EnemyContext::GetRandomFraction() < 0.5f ? -1.0f : 1.0f;
}

int RandomLightCombo(int maxLightCombo)
{
	const int longest = std::max(maxLightCombo, 1);
	return 1 + static_cast<int>(EnemyContext::GetRandomFraction() * longest) % longest;
}

bool HasLostTarget(const EnemyContext& context)
{
	return !context.HasTarget() || context.GetDistanceToTarget() > GetSettings(context).giveUpDistance;
}

// True when the enemy decides to get out of the way now: of a blow the player just started close by, or of a spell
// about to land. It makes up its mind once about each of them (see MeleeEnemyComponent::dodgeChance)
bool WantsToDodge(EnemyContext& context, bool& outIsFromSpell)
{
	MeleeEnemyComponent* settings = context.TryGet<MeleeEnemyComponent>();
	if (!settings || settings->dodgeChance <= 0.0f || !context.HasTarget() || (!context.HasRoll() && !context.HasBackstep()))
		return false;

	bool isThreatened = false;

	const unsigned int attackNumber = context.GetTargetAttackNumber();
	if (context.IsTargetAttacking() && attackNumber != settings->judgedTargetAttackNumber
		&& context.GetDistanceToTarget() <= settings->dodgeDistance)
	{
		settings->judgedTargetAttackNumber = attackNumber;
		outIsFromSpell = false;
		isThreatened   = true;
	}

	unsigned int projectileNumber = 0;
	if (SpellSystem::TryFindIncomingProjectile(context.GetEntityManager(), context.GetEntityID(), DodgeSpellSeconds, projectileNumber)
		&& projectileNumber != settings->judgedProjectileNumber)
	{
		settings->judgedProjectileNumber = projectileNumber;
		outIsFromSpell = true;
		isThreatened   = true;
	}

	return isThreatened && context.IsDodgeReady() && EnemyContext::GetRandomFraction() < settings->dodgeChance;
}

// ========== The states ==========

class WatchState : public EnemyState
{
public:
	void Enter(EnemyContext& context) override
	{
		context.Stop();
		context.LookAtTarget(false);
	}

	std::string Update(EnemyContext& context) override
	{
		context.Stop();
		if (!context.CanSeeTarget())
			return "";

		// It has seen the player: a shout
		CharacterSoundsSystem::PlayVoice(context.GetEntityManager(), context.GetEntityID(), &CharacterSoundsComponent::alertVoice);
		return Chase;
	}
};

// One dodge: a roll to a side (it passes through what was coming) or a backstep, then straight back to the fight
class EvadeState : public EnemyState
{
public:
	// What the dodge that is about to start gets away from (set by the state that decided to dodge)
	inline static bool isNextFromSpell = false;

	void Enter(EnemyContext& context) override
	{
		const float sideRollChance = isNextFromSpell ? SideRollChanceFromSpell : SideRollChanceFromBlow;
		const bool  rollsAside     = context.HasRoll() && (!context.HasBackstep() || EnemyContext::GetRandomFraction() < sideRollChance);

		glm::vec3 direction(0.0f);
		if (rollsAside)
		{
			const glm::vec3 toTarget = GameplayMath::GetHorizontalDirectionOrDefault(context.GetTargetPosition() - context.GetPosition());
			direction = glm::cross(toTarget, GameplayMath::WorldUp) * RandomSide();
		}

		context.Dodge(direction);
		hasAsked = false;
	}

	std::string Update(EnemyContext& context) override
	{
		// The frame the dodge was asked for its direction stays "held", then the enemy lets go of it
		if (!hasAsked)
		{
			hasAsked = true;
			return "";
		}

		context.Stop();

		if (context.IsBusy() || context.GetSecondsInState() < AttackStartGraceSeconds)
			return "";

		context.StartDodgeCooldown(GetSettings(context).dodgeCooldownSeconds);
		return Chase;
	}

private:
	bool hasAsked = false;
};

class ChaseState : public EnemyState
{
public:
	void Enter(EnemyContext& context) override
	{
		context.LookAtTarget(true);
	}

	std::string Update(EnemyContext& context) override
	{
		if (HasLostTarget(context))
			return ReturnHome;

		if (WantsToDodge(context, EvadeState::isNextFromSpell))
			return Evade;

		const MeleeEnemyComponent& settings = GetSettings(context);
		const float distance = context.GetDistanceToTarget();

		if (distance > settings.attackDistance && distance <= settings.leapAttackDistance && context.IsAttackReady())
			return Leap;

		// A caster keeps a player at a distance under fire instead of only walking up to it
		if (context.IsAttackReady() && context.CanSeeTarget()
			&& SpellSystem::HasSpellReady(context.GetEntityManager(), context.GetEntityID(), distance))
			return Cast;

		const float walkUntil = context.IsMoving() ? settings.attackDistance : settings.attackDistance + StopMargin;
		if (distance > walkUntil)
		{
			const float runFrom = context.IsRunning() ? settings.runDistance - RunMargin : settings.runDistance;
			context.MoveTowards(context.GetTargetPosition(), distance > runFrom);
			return "";
		}

		context.Stop();
		return context.IsAttackReady() ? Attack : "";
	}
};

// One heavy attack, or a few light ones in a row (each next one is asked for while the previous plays, like a player's combo)
class AttackState : public EnemyState
{
public:
	void Enter(EnemyContext& context) override
	{
		const MeleeEnemyComponent& settings = GetSettings(context);
		const bool isHeavy = EnemyContext::GetRandomFraction() < settings.heavyAttackChance;

		lightAttacksLeft      = isHeavy ? 0 : RandomLightCombo(settings.maxLightCombo) - 1;
		endsWithFinisher      = !isHeavy && EnemyContext::GetRandomFraction() < settings.finisherChance;
		lastAttackNumber      = context.GetAttackNumber();
		hasStartedFirstAttack = false;

		context.Stop();
		context.RequestAction(isHeavy ? CharacterAction::HeavyAttack : CharacterAction::LightAttack);
	}

	std::string Update(EnemyContext& context) override
	{
		CountStartedAttacks(context);

		if (context.IsBusy())
		{
			AskForNextLightAttack(context);
			return "";
		}

		if (context.GetSecondsInState() < AttackStartGraceSeconds)
			return "";

		context.StartAttackCooldown(GetSettings(context).attackCooldownSeconds);
		return Recover;
	}

private:
	int          lightAttacksLeft      = 0;
	bool         endsWithFinisher      = false;
	unsigned int lastAttackNumber      = 0;
	bool         hasStartedFirstAttack = false;

	// The first attack that starts is the one asked for in Enter, every next one is a link of the combo
	void CountStartedAttacks(EnemyContext& context)
	{
		const unsigned int attackNumber = context.GetAttackNumber();
		if (attackNumber == lastAttackNumber)
			return;

		// The finisher is the last link: once a heavy attack started after the light ones nothing more is asked for
		if (hasStartedFirstAttack && lightAttacksLeft == 0)
			endsWithFinisher = false;
		if (hasStartedFirstAttack)
			lightAttacksLeft = std::max(lightAttacksLeft - 1, 0);

		hasStartedFirstAttack = true;
		lastAttackNumber      = attackNumber;
	}

	void AskForNextLightAttack(EnemyContext& context)
	{
		const bool isTargetClose = context.GetDistanceToTarget() <= GetSettings(context).attackDistance * ComboReachFactor;
		if (!isTargetClose)
			return;

		if (lightAttacksLeft > 0)
			context.RequestAction(CharacterAction::LightAttack);
		else if (endsWithFinisher && hasStartedFirstAttack)
			context.RequestAction(CharacterAction::HeavyAttack);
	}
};

// One jump at the player, then the usual pause after an attack
class LeapState : public EnemyState
{
public:
	void Enter(EnemyContext& context) override
	{
		context.Stop();
		context.RequestAction(CharacterAction::LeapAttack);
	}

	std::string Update(EnemyContext& context) override
	{
		if (context.IsBusy() || context.GetSecondsInState() < AttackStartGraceSeconds)
			return "";

		context.StartAttackCooldown(GetSettings(context).attackCooldownSeconds);
		return Recover;
	}
};

// One spell at the player (see SpellCasterComponent), then the usual pause after an attack
class CastState : public EnemyState
{
public:
	void Enter(EnemyContext& context) override
	{
		context.Stop();
		hasCast = SpellSystem::TryCastSpell(context.GetEntityManager(), context.GetEntityID(), context.GetDistanceToTarget());
	}

	std::string Update(EnemyContext& context) override
	{
		if (hasCast && (context.IsBusy() || context.GetSecondsInState() < AttackStartGraceSeconds))
			return "";

		context.StartAttackCooldown(GetSettings(context).attackCooldownSeconds);
		return hasCast ? Recover : Chase;
	}

private:
	bool hasCast = false;
};

class RecoverState : public EnemyState
{
public:
	void Enter(EnemyContext& context) override
	{
		side = RandomSide();
	}

	std::string Update(EnemyContext& context) override
	{
		if (HasLostTarget(context))
			return ReturnHome;

		if (WantsToDodge(context, EvadeState::isNextFromSpell))
			return Evade;

		context.CircleTarget(side);

		return context.GetSecondsInState() >= GetSettings(context).recoverSeconds ? Chase : "";
	}

private:
	float side = 1.0f;
};

class ReturnHomeState : public EnemyState
{
public:
	void Enter(EnemyContext& context) override
	{
		context.LookAtTarget(false);
	}

	std::string Update(EnemyContext& context) override
	{
		const bool isTargetCloseAgain = context.GetDistanceToTarget() < GetSettings(context).giveUpDistance * SeeAgainFactor;

		if (context.CanSeeTarget() && isTargetCloseAgain)
			return Chase;

		if (context.GetDistanceToHome() <= ArrivedHomeDistance)
			return EnemyStateNames::Idle;

		context.MoveTowards(context.GetEnemy().homePosition, false);
		return "";
	}
};

}

void MeleeEnemy::AddStates(EnemyStateMachine& stateMachine)
{
	stateMachine.SetState(EnemyStateNames::Idle, std::make_unique<WatchState>());
	stateMachine.SetState(Chase,                 std::make_unique<ChaseState>());
	stateMachine.SetState(Attack,                std::make_unique<AttackState>());
	stateMachine.SetState(Leap,                  std::make_unique<LeapState>());
	stateMachine.SetState(Cast,                  std::make_unique<CastState>());
	stateMachine.SetState(Evade,                 std::make_unique<EvadeState>());
	stateMachine.SetState(Recover,               std::make_unique<RecoverState>());
	stateMachine.SetState(ReturnHome,            std::make_unique<ReturnHomeState>());
}

void MeleeEnemy::AddWithRequiredComponents(EntityManager& entityManager, unsigned int entityID)
{
	if (!entityManager.GetComponent<EnemyComponent>(entityID))
		EnemySystem::AddWithRequiredComponents(entityManager, entityID);

	entityManager.AddComponent<MeleeEnemyComponent>(entityID);
}

}
