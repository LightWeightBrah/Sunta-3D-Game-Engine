#include "Core/SuntaPreCompiled.h"
#include "MeleeEnemy.h"

#include "BaseEnemyStates.h"
#include "EnemyContext.h"
#include "EnemyStateMachine.h"
#include "EnemySystem.h"
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
		return context.CanSeeTarget() ? Chase : "";
	}
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

		const MeleeEnemyComponent& settings = GetSettings(context);
		const float distance = context.GetDistanceToTarget();

		if (distance > settings.attackDistance)
		{
			context.MoveTowards(context.GetTargetPosition(), distance > settings.runDistance);
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
	unsigned int lastAttackNumber      = 0;
	bool         hasStartedFirstAttack = false;

	// The first attack that starts is the one asked for in Enter, every next one is a link of the combo
	void CountStartedAttacks(EnemyContext& context)
	{
		const unsigned int attackNumber = context.GetAttackNumber();
		if (attackNumber == lastAttackNumber)
			return;

		if (hasStartedFirstAttack)
			lightAttacksLeft = std::max(lightAttacksLeft - 1, 0);

		hasStartedFirstAttack = true;
		lastAttackNumber      = attackNumber;
	}

	void AskForNextLightAttack(EnemyContext& context)
	{
		const bool isTargetClose = context.GetDistanceToTarget() <= GetSettings(context).attackDistance * ComboReachFactor;

		if (lightAttacksLeft > 0 && isTargetClose)
			context.RequestAction(CharacterAction::LightAttack);
	}
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
