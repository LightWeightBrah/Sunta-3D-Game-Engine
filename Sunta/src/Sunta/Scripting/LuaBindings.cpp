#include "Core/SuntaPreCompiled.h"
#include "LuaBindings.h"
#include "Scene/WorldSpawner.h"

#include <string>
#include <type_traits>
#include <glm/glm.hpp>

#include "ECS/Entity.h"
#include "ECS/Component.h"
#include "Gameplay/CharacterActionsSystem.h"
#include "Gameplay/CharacterAnimationComponent.h"
#include "Gameplay/CutsceneSystem.h"
#include "Gameplay/EntityName.h"
#include "Scene/EntityHierarchy.h"
#include "Gameplay/FollowCameraSystem.h"
#include "Gameplay/HUDSystem.h"
#include "Gameplay/SpellSystem.h"
#include "Gameplay/TimeScaleSystem.h"
#include "Effects/ParticleSystem.h"
#include "Audio/Audio.h"
#include "Audio/AudioSystem.h"
#include "Gameplay/CurrencySystem.h"
#include "Gameplay/HealingFlaskSystem.h"
#include "Gameplay/LevelUpSystem.h"
#include "Gameplay/CombatSystem.h"
#include "Gameplay/EnemySystem.h"
#include "Gameplay/EnemyContext.h"
#include "Gameplay/EnemyStateMachine.h"
#include "Gameplay/LockOnSystem.h"
#include "Gameplay/LuaEnemyState.h"
#include "Core/InputManager.h"
#include "Core/KeyCodes.h"
#include "Utilities/MathUtilities.h"
#include "Physics/PhysicsSystem.h"
#include "Effects/SwarmSystem.h"
#include "Effects/EffectSystem.h"
#include "Renderer/PostProcessing.h"

namespace Sunta
{

namespace
{

// A component as seen from Lua: the owning entity, looked up again on every access
// No stored component pointer, because component pools move in memory
template<typename TComponent>
struct ComponentHandle
{
	Entity owner;

	// Returns nullptr when the owner has no such component (or was destroyed)
	TComponent* TryGetComponent() const { return owner.GetComponent<TComponent>(); }
};

using TransformHandle   = ComponentHandle<TransformComponent>;
using PhysicsBodyHandle = ComponentHandle<PhysicsBodyComponent>;
using CharacterControllerHandle = ComponentHandle<CharacterControllerComponent>;
using LockOnHandle        = ComponentHandle<LockOnComponent>;
using LockOnTargetHandle  = ComponentHandle<LockOnTargetComponent>;
using HealthHandle        = ComponentHandle<HealthComponent>;
using CharacterActionsHandle = ComponentHandle<CharacterActionsComponent>;
using CharacterAnimationHandle = ComponentHandle<CharacterAnimationComponent>;
using EnemyHandle         = ComponentHandle<EnemyComponent>;
using MeleeEnemyHandle    = ComponentHandle<MeleeEnemyComponent>;
using BossHandle          = ComponentHandle<BossComponent>;
using FogGateHandle       = ComponentHandle<FogGateComponent>;
using ModelHandle         = ComponentHandle<ModelComponent>;
using HitboxHandle        = ComponentHandle<HitboxComponent>;
using EnemyHealthBarHandle = ComponentHandle<EnemyHealthBarComponent>;
using ParticleEmitterHandle = ComponentHandle<ParticleEmitterComponent>;
using EnvironmentHandle   = ComponentHandle<EnvironmentComponent>;
using CutsceneHandle      = ComponentHandle<CutsceneComponent>;
using ProjectileHandle    = ComponentHandle<ProjectileComponent>;
using AudioSourceHandle   = ComponentHandle<AudioSourceComponent>;
using HealingFlaskHandle  = ComponentHandle<HealingFlaskComponent>;
using CurrencyHandle      = ComponentHandle<CurrencyComponent>;
using CurrencyRewardHandle = ComponentHandle<CurrencyRewardComponent>;
using LevelUpHandle       = ComponentHandle<LevelUpComponent>;
using SpellCasterHandle   = ComponentHandle<SpellCasterComponent>;
using GlowSpriteHandle    = ComponentHandle<GlowSpriteComponent>;
using LiquidOrbHandle     = ComponentHandle<LiquidOrbComponent>;
using SwarmHandle         = ComponentHandle<SwarmComponent>;
using DirectionalLightHandle = ComponentHandle<DirectionalLightComponent>;
using FollowCameraHandle  = ComponentHandle<FollowCameraComponent>;

// The scene the entity lives in, as seen from Lua (this.world): what a script does to the game as a whole
struct WorldHandle
{
	EntityManager* entityManager = nullptr;
};

// Returns nil in Lua when the entity doesn't have the component
template<typename TComponent>
sol::optional<ComponentHandle<TComponent>> GetComponentHandleOrNil(const Entity& entity)
{
	if (!entity.HasComponent<TComponent>())
		return sol::nullopt;

	return ComponentHandle<TComponent>{ entity };
}

// ========== Property helpers ==========

// Creates a Lua property (like transform.position) from a POINTER TO MEMBER
// Example: MemberProperty(&TransformComponent::position, glm::vec3(0.0f), OnTransformPositionChanged)
//
// 'member'                      = which field of the component to read/write (pointer to member)
// 'valueWhenComponentIsMissing' = what Lua gets if the entity no longer has this component
// 'onValueWritten'              = function called after every write (e.g. mark transform dirty)
//
// Lua side:  local p = this.transform.position   -> runs the GETTER (first lambda)
//            this.transform.position = vec3(...) -> runs the SETTER (second lambda)
// 
// std::common_type_t<TValue> is just TValue, but it stops the compiler from deducing TValue from this argument
template<typename TComponent, typename TValue, typename TOnValueWritten>
auto MemberProperty(TValue TComponent::* member,
	std::common_type_t<TValue> valueWhenComponentIsMissing,
	TOnValueWritten onValueWritten)
{
	return sol::property(
		[member, valueWhenComponentIsMissing](const ComponentHandle<TComponent>& handle)
		{
			TComponent* component = handle.TryGetComponent();
			return component ? component->*member : valueWhenComponentIsMissing;
		},
		[member, onValueWritten](const ComponentHandle<TComponent>& handle, const TValue& newValue)
		{
			TComponent* component = handle.TryGetComponent();
			if (!component)
				return;

			component->*member = newValue;
			onValueWritten(*component);
		});
}

// Same, for members that need no reaction after being written
template<typename TComponent, typename TValue>
auto MemberProperty(TValue TComponent::* member, std::common_type_t<TValue> valueWhenComponentIsMissing)
{
	return MemberProperty(member, valueWhenComponentIsMissing, [](TComponent&) {});
}

// Creates a read-only Lua property on Entity for access to entity's components,
// e.g. this.transform or this.physicsBody
// 
// (nil in Lua when the entity doesn't have that component)
template<typename TComponent>
auto ComponentHandleProperty()
{
	return sol::readonly_property([](const Entity& entity) { return GetComponentHandleOrNil<TComponent>(entity); });
}

// ========== Reactions to changes made from scripts ==========

// ========== Enemy AI helpers ==========

// What the enemy knows and can do, made for one call from Lua (the same thing the ready C++ states use)
EnemyContext MakeEnemyContext(const EnemyHandle& handle)
{
	return EnemyContext(*handle.owner.GetEntityManager(), handle.owner.GetID(), 0.0f);
}

// Runs a command (move, stop, attack) on the enemy, nothing when it lacks the parts an enemy needs
template<typename TCommand>
void CommandEnemy(const EnemyHandle& handle, TCommand command)
{
	EnemyContext context = MakeEnemyContext(handle);
	if (context.IsValid())
		command(context);
}

// Asks the enemy something (a distance, can it see the player), 'answerWhenInvalid' when it lacks the parts an enemy needs
template<typename TQuestion, typename TAnswer>
TAnswer AskEnemy(const EnemyHandle& handle, TQuestion question, TAnswer answerWhenInvalid)
{
	const EnemyContext context = MakeEnemyContext(handle);
	return context.IsValid() ? question(context) : answerWhenInvalid;
}

// "light", "heavy", "roll", "backstep" as used by enemy:attack(...) in Lua
CharacterAction ParseActionName(const std::string& name)
{
	if (name == "heavy")    return CharacterAction::HeavyAttack;
	if (name == "roll")     return CharacterAction::Roll;
	if (name == "backstep") return CharacterAction::Backstep;
	if (name == "parry")    return CharacterAction::Parry;
	if (name == "leap")     return CharacterAction::LeapAttack;
	if (name == "block")    return CharacterAction::Block;
	if (name == "jump")     return CharacterAction::Jump;

	return CharacterAction::LightAttack;
}

EnemyStateMachine* TryGetStateMachine(const EnemyHandle& handle)
{
	return EnemySystem::GetOrCreateStateMachine(*handle.owner.GetEntityManager(), handle.owner.GetID());
}

void OnTransformPositionChanged(TransformComponent& transformComponent)
{
	transformComponent.isDirty = true;
}

void OnTransformRotationChanged(TransformComponent& transformComponent)
{
	transformComponent.SyncQuaternionFromEuler();
	transformComponent.isDirty = true;
}

void OnTransformScaleChanged(TransformComponent& transformComponent)
{
	transformComponent.isDirty = true;
}

// Changing velocity from a script should always wake the body up,
// otherwise a sleeping body would ignore the new value
void OnPhysicsBodyVelocityChanged(PhysicsBodyComponent& physicsBodyComponent)
{
	PhysicsSystem::WakeUp(physicsBodyComponent);
}

}

void LuaBindings::Register(sol::state& lua)
{
	RegisterVector3(lua);
	RegisterTransformComponent(lua);
	RegisterPhysicsBodyComponent(lua);
	RegisterCharacterControllerComponent(lua);
	RegisterLockOnComponent(lua);
	RegisterLockOnTargetComponent(lua);
	RegisterHealthComponent(lua);
	RegisterCharacterActionsComponent(lua);
	RegisterEnemyComponent(lua);
	RegisterMeleeEnemyComponent(lua);
	RegisterBossComponents(lua);
	RegisterLookComponents(lua);
	RegisterAudio(lua);
	RegisterProgressComponents(lua);
	RegisterSpellCasterComponent(lua);
	RegisterLightComponent(lua);
	RegisterWorld(lua);
	RegisterEntity(lua);
	RegisterInput(lua);
}

void LuaBindings::RegisterVector3(sol::state& lua)
{
	// Usage in Lua: vec3(1, 2, 3), a + b, a * 2, v:length(), v:normalized()
	lua.new_usertype<glm::vec3>("vec3",
		sol::call_constructor, sol::factories(
			[]() { return glm::vec3(0.0f);    },
			[](float x, float y, float z) { return glm::vec3(x, y, z); }),

		// Plain data members: in Lua they behave like fields (v.x, v.x = 5)
		"x", &glm::vec3::x,
		"y", &glm::vec3::y,
		"z", &glm::vec3::z,

		sol::meta_function::addition,    [](const glm::vec3& left, const glm::vec3& right) { return left + right;  },
		sol::meta_function::subtraction, [](const glm::vec3& left, const glm::vec3& right) { return left - right;  },
		sol::meta_function::unary_minus, [](const glm::vec3& vector) { return -vector; },
		sol::meta_function::equal_to,    [](const glm::vec3& left, const glm::vec3& right) { return left == right; },
		sol::meta_function::multiplication, sol::overload(
			[](const glm::vec3& vector, float scalar) { return vector * scalar; },
			[](float scalar, const glm::vec3& vector) { return scalar * vector; }),
		sol::meta_function::to_string, [](const glm::vec3& vector)
		{
			return "vec3(" + std::to_string(vector.x) + ", " + std::to_string(vector.y) + ", " + std::to_string(vector.z) + ")";
		},

		"length",     [](const glm::vec3& vector) { return glm::length(vector); },
		// A zero vector can't be normalized (it would give NaN), so we return it unchanged
		"normalized", [](const glm::vec3& vector) { return glm::length(vector) > 0.0f ? glm::normalize(vector) : vector; },
		"dot",        [](const glm::vec3& left, const glm::vec3& right) { return glm::dot(left, right);      },
		"cross",      [](const glm::vec3& left, const glm::vec3& right) { return glm::cross(left, right);    },
		"distance",   [](const glm::vec3& left, const glm::vec3& right) { return glm::distance(left, right); }
	);
}

void LuaBindings::RegisterTransformComponent(sol::state& lua)
{
	lua.new_usertype<TransformHandle>("Transform", sol::no_constructor,

		"position", MemberProperty(&TransformComponent::position, glm::vec3(0.0f), OnTransformPositionChanged),

		// Euler angles in degrees (same as in the Inspector)
		"rotation", MemberProperty(&TransformComponent::rotation, glm::vec3(0.0f), OnTransformRotationChanged),

		"scale",    MemberProperty(&TransformComponent::scale,    glm::vec3(1.0f), OnTransformScaleChanged),

		// Direction the entity is facing (same convention lights use)
		"forward", sol::readonly_property([](const TransformHandle& handle)
			{
				TransformComponent* transformComponent = handle.TryGetComponent();
				return transformComponent ? Math::DegreesToDirection(transformComponent->rotation) : glm::vec3(0.0f, 0.0f, -1.0f);
			}),

		// Usage in Lua: this.transform:translate(vec3(0, 1, 0))
		// This is a TELEPORT by the offset (it changes the position directly), so it pushes nothing
		// To move a kinematic body so that it pushes other bodies, use physicsBody:movePosition
		"translate", [](const TransformHandle& handle, const glm::vec3& offset)
			{
				TransformComponent* transformComponent = handle.TryGetComponent();
				if (!transformComponent)
					return;

				transformComponent->position += offset;
				OnTransformPositionChanged(*transformComponent);
			}
	);
}

void LuaBindings::RegisterPhysicsBodyComponent(sol::state& lua)
{
	lua.new_usertype<PhysicsBodyHandle>("PhysicsBody", sol::no_constructor,

		"velocity",        MemberProperty(&PhysicsBodyComponent::velocity,        glm::vec3(0.0f), OnPhysicsBodyVelocityChanged),
		"angularVelocity", MemberProperty(&PhysicsBodyComponent::angularVelocity, glm::vec3(0.0f), OnPhysicsBodyVelocityChanged),

		"mass",        MemberProperty(&PhysicsBodyComponent::mass,         0.0f),
		"useGravity",  MemberProperty(&PhysicsBodyComponent::useGravity,  false),
		"isKinematic", MemberProperty(&PhysicsBodyComponent::isKinematic, false),

		// Body can't spin around these of its own axes (e.g. freeze X and Z for a player that must not fall over)
		"freezeRotationX", MemberProperty(&PhysicsBodyComponent::freezeRotationX, false),
		"freezeRotationY", MemberProperty(&PhysicsBodyComponent::freezeRotationY, false),
		"freezeRotationZ", MemberProperty(&PhysicsBodyComponent::freezeRotationZ, false),

		// Instant change of velocity (heavier bodies change less)
		"addImpulse", [](const PhysicsBodyHandle& handle, const glm::vec3& impulse)
			{
				if (PhysicsBodyComponent* physicsBodyComponent = handle.TryGetComponent())
					PhysicsSystem::AddImpulse(*physicsBodyComponent, impulse);

			},

		// Moves a KINEMATIC body to 'targetPosition' as real movement: it pushes other bodies on the way
		// Use it to move a player or a platform. Usage in Lua: this.physicsBody:movePosition(newPosition)
		// (Setting this.transform.position directly is a teleport: it pushes nothing)
		"movePosition", [](const PhysicsBodyHandle& handle, const glm::vec3& targetPosition)
			{
				if (PhysicsBodyComponent* physicsBodyComponent = handle.TryGetComponent())
					PhysicsSystem::MovePosition(*physicsBodyComponent, targetPosition);
			},

		// Instant change of velocity, the same for every mass
		"addVelocityChange", [](const PhysicsBodyHandle& handle, const glm::vec3& velocityChange)
			{
				if (PhysicsBodyComponent* physicsBodyComponent = handle.TryGetComponent())
					PhysicsSystem::AddVelocityChange(*physicsBodyComponent, velocityChange);
			}
	);
}

void LuaBindings::RegisterCharacterControllerComponent(sol::state& lua)
{
	// Usage in Lua: this.characterController.walkSpeed = 3
	// To drive a character from a script (e.g. an enemy): this.characterController.isPlayerControlled = false,
	// then every frame set moveDirection (a vec3 in the world, its length 0..1 is how much of the speed is used) and wantsToRun
	lua.new_usertype<CharacterControllerHandle>("CharacterController", sol::no_constructor,

		"walkSpeed",     MemberProperty(&CharacterControllerComponent::walkSpeed,     0.0f),
		"runSpeed",      MemberProperty(&CharacterControllerComponent::runSpeed,      0.0f),
		"acceleration",  MemberProperty(&CharacterControllerComponent::acceleration,  0.0f),
		"turnSpeed",     MemberProperty(&CharacterControllerComponent::turnSpeed,     0.0f),

		"isPlayerControlled", MemberProperty(&CharacterControllerComponent::isPlayerControlled, false),
		"moveDirection",       MemberProperty(&CharacterControllerComponent::moveDirection,       glm::vec3(0.0f)),
		"wantsToRun",          MemberProperty(&CharacterControllerComponent::wantsToRun,          false)
	);
}

void LuaBindings::RegisterLockOnComponent(sol::state& lua)
{
	// Usage in Lua: if this.lockOn.hasTarget then print(this.lockOn.target.name) end
	// To decide from a script what is locked on to (e.g. for an enemy): this.lockOn.isPlayerControlled = false,
	// then this.lockOn:setTarget(otherEntity) and this.lockOn:clear()
	lua.new_usertype<LockOnHandle>("LockOn", sol::no_constructor,

		"maxDistance",        MemberProperty(&LockOnComponent::maxDistance,        0.0f),
		"isPlayerControlled", MemberProperty(&LockOnComponent::isPlayerControlled, false),

		"hasTarget", sol::readonly_property([](const LockOnHandle& handle)
			{
				const LockOnComponent* lockOnComponent = handle.TryGetComponent();
				return lockOnComponent && lockOnComponent->hasTarget;
			}),

		// nil when nothing is locked on to
		"target", sol::readonly_property([](const LockOnHandle& handle) -> sol::optional<Entity>
			{
				const LockOnComponent* lockOnComponent = handle.TryGetComponent();
				if (!lockOnComponent || !lockOnComponent->hasTarget)
					return sol::nullopt;

				const Entity target(lockOnComponent->targetEntityID, handle.owner.GetEntityManager());
				if (!target.IsValid())
					return sol::nullopt;

				return target;
			}),

		// Locks on to an entity that has a Lock On Target component (a warning in the log when it has none)
		"setTarget", [](const LockOnHandle& handle, const Entity& target)
			{
				if (LockOnComponent* lockOnComponent = handle.TryGetComponent())
					LockOnSystem::TrySetTarget(*handle.owner.GetEntityManager(), *lockOnComponent, target.GetID());
			},

		"clear", [](const LockOnHandle& handle)
			{
				if (LockOnComponent* lockOnComponent = handle.TryGetComponent())
					LockOnSystem::ClearTarget(*lockOnComponent);
			}
	);
}

void LuaBindings::RegisterLockOnTargetComponent(sol::state& lua)
{
	// Usage in Lua: this.lockOnTarget.isLockable = false (e.g. when the enemy dies, the lock on it is dropped)
	lua.new_usertype<LockOnTargetHandle>("LockOnTarget", sol::no_constructor,

		"isLockable", MemberProperty(&LockOnTargetComponent::isLockable, false),
		"aimHeight",  MemberProperty(&LockOnTargetComponent::aimHeight,  0.0f)
	);
}

void LuaBindings::RegisterCharacterActionsComponent(sol::state& lua)
{
	// Usage in Lua: this.characterActions:request("block") (every frame while it should last), :request("parry"), :request("light"),
	// if this.characterActions.isGuarding then ... end, this.characterActions:play("Roar") (any animation of the model, once)
	lua.new_usertype<CharacterActionsHandle>("CharacterActions", sol::no_constructor,

		"play", [](const CharacterActionsHandle& handle, const std::string& animationName)
			{
				EntityManager* entityManager = handle.owner.GetEntityManager();
				return entityManager && CharacterActionsSystem::PlaySpecial(*entityManager, handle.owner.GetID(), animationName);
			},

		"request", [](const CharacterActionsHandle& handle, const std::string& actionName)
			{
				if (CharacterActionsComponent* actions = handle.TryGetComponent())
					actions->requestedAction = ParseActionName(actionName);
			},

		"isBusy", sol::readonly_property([](const CharacterActionsHandle& handle)
			{
				const CharacterActionsComponent* actions = handle.TryGetComponent();
				return actions && actions->IsBusy();
			}),

		// Thrown back along the ground: this.characterActions:knockBack(direction, meters) (see knockbackAnimation)
		"knockBack", [](const CharacterActionsHandle& handle, const glm::vec3& direction, float meters)
			{
				EntityManager* entityManager = handle.owner.GetEntityManager();
				return entityManager && CharacterActionsSystem::PlayKnockback(*entityManager, handle.owner.GetID(), direction, meters);
			},

		"isGuarding", sol::readonly_property([](const CharacterActionsHandle& handle)
			{
				const CharacterActionsComponent* actions = handle.TryGetComponent();
				return actions && actions->IsGuarding();
			}),

		// True during the part of a parry that turns blows away (a script can show it: a flash, a shield of light)
		"isParrying", sol::readonly_property([](const CharacterActionsHandle& handle)
			{
				const CharacterActionsComponent* actions = handle.TryGetComponent();
				return actions && actions->IsParrying();
			}),

		"isOpenToRiposte", sol::readonly_property([](const CharacterActionsHandle& handle)
			{
				const CharacterActionsComponent* actions = handle.TryGetComponent();
				return actions && actions->IsOpenToRiposte();
			}),

		"isAttacking", sol::readonly_property([](const CharacterActionsHandle& handle)
			{
				const CharacterActionsComponent* actions = handle.TryGetComponent();
				return actions && actions->IsAttacking();
			}),

		// What the character does now: "light", "heavy", "leap", "roll", "hit", "cast", ... ("" = nothing), and the
		// animation it plays for it (a summon that adapts to each kind of blow tells a jab from a kick)
		"action", sol::readonly_property([](const CharacterActionsHandle& handle) -> std::string
			{
				const CharacterActionsComponent* actions = handle.TryGetComponent();
				if (!actions)
					return "";
				switch (actions->currentAction)
				{
				case CharacterAction::LightAttack: return "light";
				case CharacterAction::HeavyAttack: return "heavy";
				case CharacterAction::LeapAttack:  return "leap";
				case CharacterAction::Roll:        return "roll";
				case CharacterAction::Backstep:    return "backstep";
				case CharacterAction::HitReaction: return "hit";
				case CharacterAction::Block:       return "block";
				case CharacterAction::Parry:       return "parry";
				case CharacterAction::Parried:     return "parried";
				case CharacterAction::Special:     return "special";
				case CharacterAction::Cast:        return "cast";
				case CharacterAction::UseItem:     return "item";
				case CharacterAction::Jump:        return "jump";
				case CharacterAction::JumpAttack:  return "jump_attack";
				case CharacterAction::Death:       return "death";
				case CharacterAction::None:        return "";
				default:                           return "other";
				}
			}),
		"animation", sol::readonly_property([](const CharacterActionsHandle& handle) -> std::string
			{
				const CharacterActionsComponent* actions = handle.TryGetComponent();
				return actions ? actions->currentAnimation : std::string();
			}),

		// The timing of one attack animation (see AttackMove): this.characterActions:setMove("Attack_Light_1", 0.3, 0.5, 1.2)
		// - a boss who took up a sword strikes at other moments of the same names' new clips
		"setMove", [](const CharacterActionsHandle& handle, const std::string& animation, float hitFrom, float hitTo,
		              sol::optional<float> damageMultiplier)
			{
				CharacterActionsComponent* actions = handle.TryGetComponent();
				if (!actions)
					return;
				for (AttackMove& move : actions->attackMoves)
					if (move.animation == animation)
					{
						move.hitFrom = hitFrom;
						move.hitTo   = hitTo;
						move.damageMultiplier = damageMultiplier.value_or(move.damageMultiplier);
						return;
					}
				AttackMove move;
				move.animation = animation;
				move.hitFrom   = hitFrom;
				move.hitTo     = hitTo;
				move.damageMultiplier = damageMultiplier.value_or(1.0f);
				actions->attackMoves.push_back(move);
			},

		// How fast the attacks play (1 = as the animations were made), and how much more a heavy one hurts
		"attackSpeed",           MemberProperty(&CharacterActionsComponent::attackSpeed,           1.0f),
		"heavyDamageMultiplier", MemberProperty(&CharacterActionsComponent::heavyDamageMultiplier, 1.0f),
		// A delayed blow now and then (0..1), see CharacterActionsComponent
		"attackDelayChance",     MemberProperty(&CharacterActionsComponent::attackDelayChance,     0.0f),
		// What a fighter who takes up another weapon changes: the effects of its swings, its hyper armor (0 none,
		// 1 heavy blows, 2 every blow), how close its lunge ends
		"swingEffect",           MemberProperty(&CharacterActionsComponent::swingEffect,           std::string()),
		"heavySwingEffect",      MemberProperty(&CharacterActionsComponent::heavySwingEffect,      std::string()),
		"swingEffectHeight",     MemberProperty(&CharacterActionsComponent::swingEffectHeight,     0.0f),
		"hyperArmor",            MemberProperty(&CharacterActionsComponent::hyperArmor,            0),
		"attackReach",           MemberProperty(&CharacterActionsComponent::attackReach,           0.0f),
		// this.characterActions.finisherKnockback = 3: the last blow of the light combo throws the target 3 m back;
		// .heavyKnockback the same for heavy attacks
		"finisherKnockback",     MemberProperty(&CharacterActionsComponent::finisherKnockback,     0.0f),
		"heavyKnockback",        MemberProperty(&CharacterActionsComponent::heavyKnockback,        0.0f),
		// .knockdownChance = 0.3: now and then a heavy throw knocks it off its feet (0 = never: a model without the clips)
		"knockdownChance",       MemberProperty(&CharacterActionsComponent::knockdownChance,       0.0f)
	);

	// Usage in Lua: this.characterAnimation.idleAnimation = "Float" (hanging in the air: standing is floating now),
	// this.characterAnimation.lockedOnIdleAnimation = "" (the idle while locked on too); set them back afterwards
	lua.new_usertype<CharacterAnimationHandle>("CharacterAnimation", sol::no_constructor,
		"idleAnimation",         MemberProperty(&CharacterAnimationComponent::idleAnimation,         std::string()),
		"walkAnimation",         MemberProperty(&CharacterAnimationComponent::walkAnimation,         std::string()),
		"runAnimation",          MemberProperty(&CharacterAnimationComponent::runAnimation,          std::string()),
		"lockedOnIdleAnimation", MemberProperty(&CharacterAnimationComponent::lockedOnIdleAnimation, std::string()),
		"blendSeconds",          MemberProperty(&CharacterAnimationComponent::blendSeconds,          0.2f)
	);
}

void LuaBindings::RegisterHealthComponent(sol::state& lua)
{
	// Usage in Lua: if this.health.current < 30 then ... end, other.health:damage(10) (a trap), this.health.isDead
	lua.new_usertype<HealthHandle>("Health", sol::no_constructor,

		"max", MemberProperty(&HealthComponent::maxHealth, 0.0f),

		// For bosses with phases: this.health.minimum = 1 (no hit takes it below: it can't fall yet),
		// this.health.isInvulnerable = true (nothing hurts it while it changes its phase)
		"minimum",        MemberProperty(&HealthComponent::minimumHealth,  0.0f),
		"isInvulnerable", MemberProperty(&HealthComponent::isInvulnerable, false),

		// How much of a weapon's or a spell's damage gets through (1 = all of it): a boss that adapts to what hits it
		"weaponDamageTaken", MemberProperty(&HealthComponent::weaponDamageTaken, 1.0f),
		"spellDamageTaken",  MemberProperty(&HealthComponent::spellDamageTaken,  1.0f),

		// 0..1 of the full health
		"fraction", sol::readonly_property([](const HealthHandle& handle)
			{
				const HealthComponent* health = handle.TryGetComponent();
				return (health && health->maxHealth > 0.0f) ? health->currentHealth / health->maxHealth : 0.0f;
			}),

		// Gives health back (never above the full health), nothing for the dead
		"heal", [](const HealthHandle& handle, float amount)
			{
				HealthComponent* health = handle.TryGetComponent();
				if (health && !health->isDead)
					health->currentHealth = glm::clamp(health->currentHealth + amount, 0.0f, health->maxHealth);
			},

		"current", sol::readonly_property([](const HealthHandle& handle)
			{
				const HealthComponent* health = handle.TryGetComponent();
				return health ? health->currentHealth : 0.0f;
			}),

		"isDead", sol::readonly_property([](const HealthHandle& handle)
			{
				const HealthComponent* health = handle.TryGetComponent();
				return health && health->isDead;
			}),

		// Hurts like a weapon does: plays the hit (or death) animation, does nothing while rolling
		"damage", [](const HealthHandle& handle, float amount)
			{
				return CombatSystem::ApplyDamage(*handle.owner.GetEntityManager(), handle.owner.GetID(), amount, handle.owner.GetID());
			}
	);
}

void LuaBindings::RegisterEnemyComponent(sol::state& lua)
{
	// The enemy's state machine from Lua. A script on the enemy adds its own states or replaces the ready ones by name
	// (Idle, Chase, Attack, Recover, ReturnHome, Dead):
	//
	//     function OnStart()
	//         this.enemy:addState("Chase", {
	//             enter  = function(entity) entity.enemy:lookAtTarget(true) end,
	//             update = function(entity, deltaTime)
	//                 local ai = entity.enemy
	//                 if ai.distanceToTarget < 2 then return "Attack" end
	//                 ai:moveTo(ai.target.transform.position, true)
	//             end
	//         })
	//     end
	lua.new_usertype<EnemyHandle>("Enemy", sol::no_constructor,

		// The senses from the Inspector
		"sightDistance",   MemberProperty(&EnemyComponent::sightDistance,   0.0f),
		"hearingDistance", MemberProperty(&EnemyComponent::hearingDistance, 0.0f),

		// An enemy that waits to be called (Enemy 'startsDormant'): move it where it should appear, then ally.enemy:wake().
		// ally.enemy:sleep() puts it away again
		"isDormant", sol::readonly_property([](const EnemyHandle& handle)
			{
				const EnemyComponent* enemy = handle.TryGetComponent();
				return enemy && enemy->isDormant;
			}),

		"wake", [](const EnemyHandle& handle)
			{
				EnemySystem::SetDormant(*handle.owner.GetEntityManager(), handle.owner.GetID(), false);
			},

		"sleep", [](const EnemyHandle& handle)
			{
				EnemySystem::SetDormant(*handle.owner.GetEntityManager(), handle.owner.GetID(), true);
			},

		// ===== The state machine =====

		"state", sol::readonly_property([](const EnemyHandle& handle)
			{
				const EnemyStateMachine* stateMachine = TryGetStateMachine(handle);
				return stateMachine ? stateMachine->GetCurrentStateName() : std::string();
			}),

		"secondsInState", sol::readonly_property([](const EnemyHandle& handle)
			{
				const EnemyStateMachine* stateMachine = TryGetStateMachine(handle);
				return stateMachine ? stateMachine->GetSecondsInState() : 0.0f;
			}),

		"addState", [](const EnemyHandle& handle, const std::string& name, const sol::table& functions)
			{
				if (EnemyStateMachine* stateMachine = TryGetStateMachine(handle))
					stateMachine->SetState(name, std::make_unique<LuaEnemyState>(functions));
			},

		"changeState", [](const EnemyHandle& handle, const std::string& name)
			{
				if (EnemyStateMachine* stateMachine = TryGetStateMachine(handle))
					stateMachine->RequestState(name);
			},

		// ===== What the enemy knows =====

		"hasTarget", sol::readonly_property([](const EnemyHandle& handle)
			{
				return AskEnemy(handle, [](const EnemyContext& context) { return context.HasTarget(); }, false);
			}),

		"target", sol::readonly_property([](const EnemyHandle& handle) -> sol::optional<Entity>
			{
				const EnemyContext context = MakeEnemyContext(handle);
				if (!context.IsValid() || !context.HasTarget())
					return sol::nullopt;

				return Entity(context.GetTargetID(), handle.owner.GetEntityManager());
			}),

		"distanceToTarget", sol::readonly_property([](const EnemyHandle& handle)
			{
				return AskEnemy(handle, [](const EnemyContext& context) { return context.GetDistanceToTarget(); }, 0.0f);
			}),

		"distanceToHome", sol::readonly_property([](const EnemyHandle& handle)
			{
				return AskEnemy(handle, [](const EnemyContext& context) { return context.GetDistanceToHome(); }, 0.0f);
			}),

		"isBusy", sol::readonly_property([](const EnemyHandle& handle)
			{
				return AskEnemy(handle, [](const EnemyContext& context) { return context.IsBusy(); }, false);
			}),

		"canSeeTarget", [](const EnemyHandle& handle)
			{
				return AskEnemy(handle, [](const EnemyContext& context) { return context.CanSeeTarget(); }, false);
			},

		// ===== What the enemy can do =====

		"moveTo", [](const EnemyHandle& handle, const glm::vec3& point, bool run)
			{
				CommandEnemy(handle, [&](EnemyContext& context) { context.MoveTowards(point, run); });
			},

		"goHome", [](const EnemyHandle& handle)
			{
				CommandEnemy(handle, [](EnemyContext& context) { context.MoveTowards(context.GetEnemy().homePosition, false); });
			},

		"circleTarget", [](const EnemyHandle& handle, float side)
			{
				CommandEnemy(handle, [&](EnemyContext& context) { context.CircleTarget(side); });
			},

		"stop", [](const EnemyHandle& handle)
			{
				CommandEnemy(handle, [](EnemyContext& context) { context.Stop(); });
			},

		"lookAtTarget", [](const EnemyHandle& handle, bool shouldLook)
			{
				CommandEnemy(handle, [&](EnemyContext& context) { context.LookAtTarget(shouldLook); });
			},

		// A roll in a direction of the world (a backstep for vec3(0, 0, 0)), e.g. out of the way of something a script knows of
		"dodge", [](const EnemyHandle& handle, const glm::vec3& direction)
			{
				CommandEnemy(handle, [&](EnemyContext& context) { context.Dodge(direction); });
			},

		// No attack of its own choosing for that long (a script that wants the enemy to hold back for a moment)
		"startAttackCooldown", [](const EnemyHandle& handle, float seconds)
			{
				CommandEnemy(handle, [&](EnemyContext& context) { context.StartAttackCooldown(seconds); });
			},

		// "light", "heavy", "leap", "roll", "backstep", "parry" or "block" (a block lasts only while it is asked for, every frame)
		"attack", [](const EnemyHandle& handle, const std::string& actionName)
			{
				CommandEnemy(handle, [&](EnemyContext& context) { context.RequestAction(ParseActionName(actionName)); });
			}
	);
}

void LuaBindings::RegisterBossComponents(sol::state& lua)
{
	// Usage in Lua (on the boss): if this.boss.phase == 2 then ... end, this.boss.phaseTwoHealthFraction = 0.3
	lua.new_usertype<BossHandle>("Boss", sol::no_constructor,
		"phaseTwoHealthFraction",   MemberProperty(&BossComponent::phaseTwoHealthFraction,   0.0f),
		"phaseTwoDamageMultiplier", MemberProperty(&BossComponent::phaseTwoDamageMultiplier, 0.0f),
		"phaseTwoAttackSpeedup",    MemberProperty(&BossComponent::phaseTwoAttackSpeedup,    0.0f),

		"phase", sol::readonly_property([](const BossHandle& handle)
			{
				const BossComponent* boss = handle.TryGetComponent();
				return (boss && boss->isInPhaseTwo) ? 2 : 1;
			}),

		"isAwake", sol::readonly_property([](const BossHandle& handle)
			{
				const BossComponent* boss = handle.TryGetComponent();
				return boss && boss->isAwake;
			}),

		"isDefeated", sol::readonly_property([](const BossHandle& handle)
			{
				const BossComponent* boss = handle.TryGetComponent();
				return boss && boss->isDefeated;
			})
	);

	// Usage in Lua (on the fog): if this.fogGate.isClosed then ... end (e.g. start the boss music)
	lua.new_usertype<FogGateHandle>("FogGate", sol::no_constructor,
		"victoryMessage", MemberProperty(&FogGateComponent::victoryMessage, std::string()),

		"isClosed", sol::readonly_property([](const FogGateHandle& handle)
			{
				const FogGateComponent* fog = handle.TryGetComponent();
				return fog && fog->isClosed;
			}),

		"isCleared", sol::readonly_property([](const FogGateHandle& handle)
			{
				const FogGateComponent* fog = handle.TryGetComponent();
				return fog && fog->isCleared;
			})
	);
}

void LuaBindings::RegisterMeleeEnemyComponent(sol::state& lua)
{
	// The tuning of a Melee Enemy, e.g. a script makes it more aggressive when hurt: this.meleeEnemy.attackCooldownSeconds = 0.5
	lua.new_usertype<MeleeEnemyHandle>("MeleeEnemy", sol::no_constructor,
		"attackDistance",        MemberProperty(&MeleeEnemyComponent::attackDistance,        0.0f),
		"runDistance",           MemberProperty(&MeleeEnemyComponent::runDistance,           0.0f),
		"giveUpDistance",        MemberProperty(&MeleeEnemyComponent::giveUpDistance,        0.0f),
		"attackCooldownSeconds", MemberProperty(&MeleeEnemyComponent::attackCooldownSeconds, 0.0f),
		"heavyAttackChance",     MemberProperty(&MeleeEnemyComponent::heavyAttackChance,     0.0f),
		"recoverSeconds",        MemberProperty(&MeleeEnemyComponent::recoverSeconds,        0.0f),
		"maxLightCombo",         MemberProperty(&MeleeEnemyComponent::maxLightCombo,         0),
		"finisherChance",        MemberProperty(&MeleeEnemyComponent::finisherChance,        0.0f),
		"dodgeChance",           MemberProperty(&MeleeEnemyComponent::dodgeChance,           0.0f),
		"dodgeCooldownSeconds",  MemberProperty(&MeleeEnemyComponent::dodgeCooldownSeconds,  0.0f),
		"leapAttackDistance",    MemberProperty(&MeleeEnemyComponent::leapAttackDistance,    0.0f)
	);
}

void LuaBindings::RegisterEntity(sol::state& lua)
{
	lua.new_usertype<Entity>("Entity", sol::no_constructor,

		"id",      sol::readonly_property(&Entity::GetID),
		"isValid", sol::readonly_property(&Entity::IsValid), // false after the entity was destroyed

		// A child (or a child's child...) with this name: this:findChild("Right Fist") - not another character's
		"findChild", [](const Entity& entity, const std::string& name) -> sol::optional<Entity>
			{
				EntityManager* entityManager = entity.GetEntityManager();
				if (!entityManager)
					return sol::nullopt;
				std::vector<unsigned int> open = EntityHierarchy::GetChildren(*entityManager, entity.GetID());
				while (!open.empty())
				{
					const unsigned int id = open.back();
					open.pop_back();
					if (GetEntityName(*entityManager, id) == name)
						return Entity(id, entityManager);
					for (unsigned int child : EntityHierarchy::GetChildren(*entityManager, id))
						open.push_back(child);
				}
				return sol::nullopt;
			},

		"name", sol::property(
			[](const Entity& entity)
			{
				TagComponent* tagComponent = entity.GetComponent<TagComponent>();
				return tagComponent ? tagComponent->name : "";
			},
			[](const Entity& entity, const std::string& newName)
			{
				if (TagComponent* tagComponent = entity.GetComponent<TagComponent>())
					tagComponent->name = newName;
			}),

		// nil when the entity doesn't have the component, so scripts can check: if body then ... end
		"transform",   ComponentHandleProperty<TransformComponent>(),
		"physicsBody", ComponentHandleProperty<PhysicsBodyComponent>(),
		"characterController", ComponentHandleProperty<CharacterControllerComponent>(),
		"lockOn",              ComponentHandleProperty<LockOnComponent>(),
		"lockOnTarget",        ComponentHandleProperty<LockOnTargetComponent>(),
		"health",              ComponentHandleProperty<HealthComponent>(),
		"characterActions",    ComponentHandleProperty<CharacterActionsComponent>(),
		"characterAnimation",  ComponentHandleProperty<CharacterAnimationComponent>(),
		"enemy",               ComponentHandleProperty<EnemyComponent>(),
		"meleeEnemy",          ComponentHandleProperty<MeleeEnemyComponent>(),
		"boss",                ComponentHandleProperty<BossComponent>(),
		"fogGate",             ComponentHandleProperty<FogGateComponent>(),
		"model",               ComponentHandleProperty<ModelComponent>(),
		"hitbox",              ComponentHandleProperty<HitboxComponent>(),
		"enemyHealthBar",      ComponentHandleProperty<EnemyHealthBarComponent>(),
		"projectile",          ComponentHandleProperty<ProjectileComponent>(),
		"particles",           ComponentHandleProperty<ParticleEmitterComponent>(),
		"environment",         ComponentHandleProperty<EnvironmentComponent>(),
		"cutscene",            ComponentHandleProperty<CutsceneComponent>(),
		"audioSource",         ComponentHandleProperty<AudioSourceComponent>(),
		"healingFlask",        ComponentHandleProperty<HealingFlaskComponent>(),
		"currency",            ComponentHandleProperty<CurrencyComponent>(),
		"currencyReward",      ComponentHandleProperty<CurrencyRewardComponent>(),
		"levelUp",             ComponentHandleProperty<LevelUpComponent>(),
		"spellCaster",         ComponentHandleProperty<SpellCasterComponent>(),
		"glowSprite",          ComponentHandleProperty<GlowSpriteComponent>(),
		"followCamera",        ComponentHandleProperty<FollowCameraComponent>(),
		"liquidOrb",           ComponentHandleProperty<LiquidOrbComponent>(),
		"swarm",               ComponentHandleProperty<SwarmComponent>(),
		"light",               ComponentHandleProperty<DirectionalLightComponent>(),

		// The scene this entity lives in: this.world:find("Boss"), this.world:notify("..."), see RegisterWorld
		"world", sol::readonly_property([](const Entity& entity) { return WorldHandle{ entity.GetEntityManager() }; })
	);
}

void LuaBindings::RegisterSpellCasterComponent(sol::state& lua)
{
	// Usage in Lua (on the entity with the Spell Caster, a child of the character):
	//     local spell = this.world:find("Fire Volley Spell")
	//     if spell.spellCaster.isReady then spell.spellCaster:cast() end
	// 'timesReleased' grows each time its projectiles leave: a script notices a finished cast by its change
	lua.new_usertype<SpellCasterHandle>("SpellCaster", sol::no_constructor,
		"cooldownSeconds", MemberProperty(&SpellCasterComponent::cooldownSeconds, 0.0f),
		"cooldownLeft",    MemberProperty(&SpellCasterComponent::cooldownLeft,    0.0f),
		"projectileCount", MemberProperty(&SpellCasterComponent::projectileCount, 0),
		"spreadDegrees",   MemberProperty(&SpellCasterComponent::spreadDegrees,   0.0f),
		"isCastOnRequestOnly", MemberProperty(&SpellCasterComponent::isCastOnRequestOnly, false),
		"isUninterruptible",   MemberProperty(&SpellCasterComponent::isUninterruptible,   false),

		"isReady", sol::readonly_property([](const SpellCasterHandle& handle)
			{
				const SpellCasterComponent* spell = handle.TryGetComponent();
				return spell && spell->IsReady();
			}),

		"isCasting", sol::readonly_property([](const SpellCasterHandle& handle)
			{
				const SpellCasterComponent* spell = handle.TryGetComponent();
				return spell && spell->isCasting;
			}),

		"timesReleased", sol::readonly_property([](const SpellCasterHandle& handle)
			{
				const SpellCasterComponent* spell = handle.TryGetComponent();
				return spell ? spell->timesReleased : 0;
			}),

		// False when the spell is not ready or its caster is busy (attacking, staggering).
		// spell.spellCaster:cast(true): forced (a staged moment): cooling down or busy, it is cast anyway
		"cast", [](const SpellCasterHandle& handle, sol::optional<bool> isForced)
			{
				EntityManager* entityManager = handle.owner.GetEntityManager();
				return entityManager && SpellSystem::TryCast(*entityManager, handle.owner.GetID(), isForced.value_or(false));
			}
	);
}

void LuaBindings::RegisterLightComponent(sol::state& lua)
{
	// The sun or the moon of the scene (a Directional Light). Usage in Lua: sun.light.color = vec3(1.0, 0.2, 0.1)
	// (its light, red as blood), sun.light.ambient = vec3(0.1, 0.0, 0.0) (what the shadows keep)
	lua.new_usertype<DirectionalLightHandle>("Light", sol::no_constructor,
		"color", sol::property(
			[](const DirectionalLightHandle& handle)
			{
				const DirectionalLightComponent* light = handle.TryGetComponent();
				return light ? light->color.diffuseIntensity : glm::vec3(0.0f);
			},
			[](const DirectionalLightHandle& handle, const glm::vec3& color)
			{
				if (DirectionalLightComponent* light = handle.TryGetComponent())
					light->color.diffuseIntensity = color;
			}),

		"ambient", sol::property(
			[](const DirectionalLightHandle& handle)
			{
				const DirectionalLightComponent* light = handle.TryGetComponent();
				return light ? light->color.ambientIntensity : glm::vec3(0.0f);
			},
			[](const DirectionalLightHandle& handle, const glm::vec3& color)
			{
				if (DirectionalLightComponent* light = handle.TryGetComponent())
					light->color.ambientIntensity = color;
			})
	);
}

void LuaBindings::RegisterWorld(sol::state& lua)
{
	// What a script does to the game as a whole. Usage in Lua:
	//     local boss = this.world:find("Boss")                 (nil when there is no entity with that name)
	//     this.world:notify("THE GREAT RITE")                   (a big message in the middle of the screen, fading away)
	//     this.world:prompt("R: answer with your own spell")   (at the bottom, only in the frames it is asked for)
	//     this.world:shakeCamera(0.6)                          (0..1)
	//     this.world:burst("Fireball Burst", position, 80)     (specks of the Particle Emitter with that name)
	//     this.world:slowMotion(1.5, 0.3)                      (real seconds, the speed of the game meanwhile)
	//     if this.world.isCutscenePlaying then return end
	lua.new_usertype<WorldHandle>("World", sol::no_constructor,

		"find", [](const WorldHandle& world, const std::string& name) -> sol::optional<Entity>
			{
				unsigned int entityID = 0;
				if (!world.entityManager || !TryFindEntityByName(*world.entityManager, name, entityID))
					return sol::nullopt;

				return Entity(entityID, world.entityManager);
			},

		// A copy of an entity of the scene used as a template (with its children) where it is wanted, turned 'yaw'
		// degrees: local guard = this.world:spawn("Bodyguard Template", position, 180) (nil without the template).
		// Keep the template itself out of the fight (far away, an enemy asleep). Its scripts start next frame
		"spawn", [](const WorldHandle& world, const std::string& templateName, const glm::vec3& position, sol::optional<float> yawDegrees) -> sol::optional<Entity>
			{
				unsigned int copyID = 0;
				if (!world.entityManager || !WorldSpawner::Spawn(*world.entityManager, templateName, position, yawDegrees.value_or(0.0f), copyID))
					return sol::nullopt;
				return Entity(copyID, world.entityManager);
			},

		// Removes an entity and everything under it at the end of the frame: this.world:destroy(guard)
		"destroy", [](const WorldHandle&, const Entity& entity)
			{
				WorldSpawner::RequestDestroy(entity.GetID());
			},

		"notify", [](const WorldHandle& world, const std::string& text)
			{
				if (world.entityManager)
					HUDSystem::ShowNotification(*world.entityManager, text);
			},

		"prompt", [](const WorldHandle& world, const std::string& text)
			{
				if (world.entityManager)
					HUDSystem::ShowPrompt(*world.entityManager, text);
			},

		"shakeCamera", [](const WorldHandle& world, float strength)
			{
				if (world.entityManager)
					FollowCameraSystem::Shake(*world.entityManager, strength);
			},

		"burst", [](const WorldHandle& world, const std::string& emitterName, const glm::vec3& position, int count)
			{
				return world.entityManager && ParticleSystem::BurstAt(*world.entityManager, emitterName, position, count);
			},

		// Plays an effect of the scene (an entity with Effect Shapes and Particle Emitters, its children too) at a
		// point, facing a direction along the ground (optional), its emitters throwing 'count' specks (optional):
		//     this.world:effect("Heavy Impact", target.transform.position, this.transform.forward)
		"effect", [](const WorldHandle& world, const std::string& effectName, const glm::vec3& position,
			sol::optional<glm::vec3> direction, sol::optional<int> count)
			{
				return world.entityManager && EffectSystem::Play(*world.entityManager, effectName, position,
					direction.value_or(glm::vec3(0.0f)), count.value_or(20));
			},

		// An impact frame: the picture blurred out from its middle, colors split, contrast punched for a blink
		//     this.world:impactFrame(0.8)   (strength 0..1, optional seconds of real time, 0.3 by default)
		"impactFrame", [](const WorldHandle&, float strength, sol::optional<float> seconds)
			{
				PostProcessing::Kick(strength, seconds.value_or(0.3f));
			},

		// Keeps the black bars of the cutscenes on through a sequence of shots (true), lets them go (false)
		"holdLetterbox", [](const WorldHandle&, bool isHeld)
			{
				CutsceneSystem::HoldLetterbox(isHeld);
			},

		"slowMotion", [](const WorldHandle&, float seconds, float scale)
			{
				TimeScaleSystem::SlowMotion(seconds, scale);
			},

		// The whole screen turns to a color and fades back: this.world:flash(vec3(1, 1, 1), 1.5)
		// (a third number keeps it full for that long first; a fourth is how much it covers at its fullest:
		// this.world:flash(vec3(1, 0.9, 0.7), 0.3, 0, 0.35) is a blink of light, not a white screen)
		"flash", [](const WorldHandle&, const glm::vec3& color, float seconds, sol::optional<float> holdSeconds, sol::optional<float> strength)
			{
				HUDSystem::FlashScreen(color, seconds, holdSeconds.value_or(0.0f), strength.value_or(1.0f));
			},

		// Throws one projectile without a cast: the Projectile template with that name, on the side of 'caster',
		// from a point of the world in a direction (a script's own patterns: rings, rains, walls of fire).
		//     this.world:launch("Boss Orb", this, this.transform.position + vec3(0, 1.4, 0), this.transform.forward)
		"launch", [](const WorldHandle& world, const std::string& projectileName, const Entity& caster, const glm::vec3& from, const glm::vec3& direction)
			{
				return world.entityManager && SpellSystem::Launch(*world.entityManager, projectileName, caster.GetID(), from, direction);
			},

		// True when a flying spell of the other side reaches the entity within that many seconds
		"isSpellIncoming", [](const WorldHandle& world, const Entity& entity, float withinSeconds)
			{
				unsigned int projectileNumber = 0;
				return world.entityManager && SpellSystem::TryFindIncomingProjectile(*world.entityManager, entity.GetID(), withinSeconds, projectileNumber);
			},

		"isCutscenePlaying", sol::readonly_property([](const WorldHandle& world)
			{
				return world.entityManager && CutsceneSystem::IsPlaying(*world.entityManager);
			})
	);
}

void LuaBindings::RegisterLookComponents(sol::state& lua)
{
	// Usage in Lua: this.model.isVisible = false, this.model.emissiveStrength = 2 (its glowing parts shine brighter)
	lua.new_usertype<ModelHandle>("Model", sol::no_constructor,
		"isVisible",        MemberProperty(&ModelComponent::isVisible,        false),
		"emissiveStrength", MemberProperty(&ModelComponent::emissiveStrength, 0.0f),
		// this.model.darkness = 1 (a black silhouette), this.model.glow = vec3(1.2, 0.6, 0.1) (an aura of light all over)
		"darkness",         MemberProperty(&ModelComponent::darkness,         0.0f),
		"glow",             MemberProperty(&ModelComponent::glowColor,        glm::vec3(0.0f)),
		// this.model.rimGlow = vec3(1.0, 0.7, 0.2): light along its outline; this.model.expression = 1: its smile on
		"rimGlow",          MemberProperty(&ModelComponent::rimGlow,          glm::vec3(0.0f)),
		// this.model.aura = vec3(1.4, 0.8, 0.2): a soft glow around the figure, beyond its outline; .auraSize = 0.12 (m)
		"aura",             MemberProperty(&ModelComponent::aura,             glm::vec3(0.0f)),
		"auraSize",         MemberProperty(&ModelComponent::auraSize,         0.0f),
		"expression",       MemberProperty(&ModelComponent::expression,       0.0f),
		// this.model.modelName = "sih_boss_armored": another model put on in its place (a boss changing its body).
		// Its animations go on by name, so the two models should share their skeleton and the names of their clips
		"modelName",        MemberProperty(&ModelComponent::modelName,        std::string(),
			[](ModelComponent& model) { model.isDirty = true; })
	);

	// Usage in Lua: sword.hitbox.isEnabled = true (a weapon taken up), boss's fist: fist.hitbox.isEnabled = false;
	// sword.hitbox.damage = 40
	// Usage in Lua (on a projectile template): if bottle.projectile.timesBurst > seen then ... bottle.projectile.lastBurstPosition
	lua.new_usertype<ProjectileHandle>("Projectile", sol::no_constructor,
		"timesBurst", sol::readonly_property([](const ProjectileHandle& handle)
			{
				const ProjectileComponent* projectile = handle.TryGetComponent();
				return projectile ? projectile->timesBurst : 0;
			}),
		"lastBurstPosition", sol::readonly_property([](const ProjectileHandle& handle)
			{
				const ProjectileComponent* projectile = handle.TryGetComponent();
				return projectile ? projectile->lastBurstPosition : glm::vec3(0.0f);
			})
	);

	// Usage in Lua: this.enemyHealthBar.displayName = "The Hollow King, Unbound" (a boss renamed by its phase)
	lua.new_usertype<EnemyHealthBarHandle>("EnemyHealthBar", sol::no_constructor,
		"displayName", MemberProperty(&EnemyHealthBarComponent::displayName, std::string())
	);

	lua.new_usertype<HitboxHandle>("Hitbox", sol::no_constructor,
		"isEnabled", MemberProperty(&HitboxComponent::isEnabled, false),
		"damage",    MemberProperty(&HitboxComponent::damage,    0.0f),
		"knockback", MemberProperty(&HitboxComponent::knockback, 0.0f)
	);

	// Usage in Lua (on the camera's entity): camera.followCamera.distance = 4 (closer in a fist fight, farther for a giant),
	// .shoulderOffset = 0.7 (over the right shoulder: a tall enemy isn't hidden behind the player),
	// .lockOnPitchDegrees = 10 (how far down it looks while locked on),
	// .lockOnSideDegrees = 35 (swung off the line to the enemy: a fist fight seen side by side)
	lua.new_usertype<FollowCameraHandle>("FollowCamera", sol::no_constructor,
		"distance",           MemberProperty(&FollowCameraComponent::distance,           0.0f),
		"pivotHeight",        MemberProperty(&FollowCameraComponent::pivotHeight,        0.0f),
		"shoulderOffset",     MemberProperty(&FollowCameraComponent::shoulderOffset,     0.0f),
		"lockOnPitchDegrees", MemberProperty(&FollowCameraComponent::lockOnPitchDegrees, 0.0f),
		"lockOnSideDegrees",  MemberProperty(&FollowCameraComponent::lockOnSideDegrees,  0.0f)
	);

	// Usage in Lua: eye.glowSprite.isVisible = true, eye.glowSprite.intensity = 8 (glowing eyes turned on by a script)
	lua.new_usertype<GlowSpriteHandle>("GlowSprite", sol::no_constructor,
		"isVisible", MemberProperty(&GlowSpriteComponent::isVisible, false),
		"color",     MemberProperty(&GlowSpriteComponent::color,     glm::vec3(1.0f)),
		"size",      MemberProperty(&GlowSpriteComponent::size,      0.0f),
		"intensity", MemberProperty(&GlowSpriteComponent::intensity, 0.0f)
	);

	// Usage in Lua: orb.liquidOrb.isVisible = true, orb.liquidOrb.brightness = 2
	lua.new_usertype<LiquidOrbHandle>("LiquidOrb", sol::no_constructor,
		"isVisible",  MemberProperty(&LiquidOrbComponent::isVisible,  false),
		"tint",       MemberProperty(&LiquidOrbComponent::tint,       glm::vec3(1.0f)),
		"brightness", MemberProperty(&LiquidOrbComponent::brightness, 1.0f),
		"clearness",  MemberProperty(&LiquidOrbComponent::clearness,  0.0f),
		"wobble",     MemberProperty(&LiquidOrbComponent::wobble,     0.0f)
	);

	// Usage in Lua (see SwarmComponent): s = find("Ember Swarm").swarm; s.isVisible = true; s.gathered = 1;
	// s.radius = 40; s.spread = 1; s:fall(); s:reset(); if s.isFalling then ... end
	lua.new_usertype<SwarmHandle>("Swarm", sol::no_constructor,
		"isVisible",            MemberProperty(&SwarmComponent::isVisible,            false),
		"gathered",             MemberProperty(&SwarmComponent::gathered,             0.0f),
		"spread",               MemberProperty(&SwarmComponent::spread,               0.0f),
		"radius",               MemberProperty(&SwarmComponent::radius,               0.0f),
		"pieceSize",            MemberProperty(&SwarmComponent::pieceSize,            0.0f),
		"spinDegreesPerSecond", MemberProperty(&SwarmComponent::spinDegreesPerSecond, 0.0f),
		"skyCenter",            MemberProperty(&SwarmComponent::skyCenter,            glm::vec3(0.0f)),
		"skyHalfExtents",       MemberProperty(&SwarmComponent::skyHalfExtents,       glm::vec3(0.0f)),
		"groundHeight",         MemberProperty(&SwarmComponent::groundHeight,         0.0f),
		"glowColor",            MemberProperty(&SwarmComponent::glowColor,            glm::vec3(0.0f)),
		"isFalling", sol::readonly_property([](const SwarmHandle& handle)
			{
				const SwarmComponent* swarm = handle.TryGetComponent();
				return swarm && swarm->fallSeconds >= 0.0f;
			}),
		"fall", [](const SwarmHandle& handle)
			{
				if (SwarmComponent* swarm = handle.TryGetComponent())
					SwarmSystem::Fall(*swarm);
			},
		"reset", [](const SwarmHandle& handle)
			{
				if (SwarmComponent* swarm = handle.TryGetComponent())
					SwarmSystem::Reset(*swarm);
			},
		// for _, at in ipairs(s:breakWithin(center, 12)) do ... end: the stuck copies that close break (a list of
		// where they were, for an effect on each)
		"breakWithin", [](const SwarmHandle& handle, const glm::vec3& center, float radius, sol::this_state state)
			{
				std::vector<glm::vec3> broken;
				if (SwarmComponent* swarm = handle.TryGetComponent())
					SwarmSystem::BreakWithin(*swarm, center, radius, broken);
				sol::state_view lua(state);
				sol::table list = lua.create_table();
				for (std::size_t i = 0; i < broken.size(); ++i)
					list[i + 1] = broken[i];
				return list;
			}
	);

	// Usage in Lua (on the cutscene's entity): this.cutscene:play(), this.cutscene:replay() (again, even if it was played), if this.cutscene.isPlaying then ... end
	lua.new_usertype<CutsceneHandle>("Cutscene", sol::no_constructor,
		"play", [](const CutsceneHandle& handle)
			{
				if (EntityManager* entityManager = handle.owner.GetEntityManager())
					CutsceneSystem::Play(*entityManager, handle.owner.GetID());
			},

		// Plays it even if it was already played (a shot cut back to again and again: angles taken in turn)
		"replay", [](const CutsceneHandle& handle)
			{
				EntityManager* entityManager = handle.owner.GetEntityManager();
				if (CutsceneComponent* cutscene = handle.TryGetComponent(); entityManager && cutscene)
				{
					cutscene->hasPlayed = false;
					CutsceneSystem::Play(*entityManager, handle.owner.GetID());
				}
			},

		// The player pressed its 'waitForKey' (a moment the player answers)
		"wasAnswered", sol::readonly_property([](const CutsceneHandle& handle)
			{
				const CutsceneComponent* cutscene = handle.TryGetComponent();
				return cutscene && cutscene->wasAnswered;
			}),

		"isPlaying", sol::readonly_property([](const CutsceneHandle& handle)
			{
				const CutsceneComponent* cutscene = handle.TryGetComponent();
				return cutscene && cutscene->isPlaying;
			})
	);

	// Usage in Lua (on the entity with the Environment): this.environment.fogDensity = 0.1, this.environment.skyBrightness = 0.5,
	// this.environment.exposure = 1.2, this.environment.bloomIntensity = 1.5 (the boss fight glows more)
	lua.new_usertype<EnvironmentHandle>("Environment", sol::no_constructor,
		"skyBrightness", MemberProperty(&EnvironmentComponent::skyBrightness, 0.0f),
		"isFogEnabled",  MemberProperty(&EnvironmentComponent::isFogEnabled,  false),
		"fogDensity",    MemberProperty(&EnvironmentComponent::fogDensity,    0.0f),
		"fogColor",      MemberProperty(&EnvironmentComponent::fogColor,      glm::vec3(0.0f)),
		// Another sky (a folder with its six pictures), e.g. when a boss changes the world around the fight
		"skyboxFolder",  MemberProperty(&EnvironmentComponent::skyboxFolder,  std::string()),
		"exposure",       MemberProperty(&EnvironmentComponent::exposure,       0.0f),
		"bloomIntensity", MemberProperty(&EnvironmentComponent::bloomIntensity, 0.0f),
		"bloomThreshold", MemberProperty(&EnvironmentComponent::bloomThreshold, 0.0f)
	);

	// Usage in Lua: this.particles.isEmitting = true, this.particles:burst(40) (40 specks at once), this.particles.particlesPerSecond = 80
	lua.new_usertype<ParticleEmitterHandle>("ParticleEmitter", sol::no_constructor,
		"isEmitting",         MemberProperty(&ParticleEmitterComponent::isEmitting,         false),
		"particlesPerSecond", MemberProperty(&ParticleEmitterComponent::particlesPerSecond, 0.0f),
		"intensity",          MemberProperty(&ParticleEmitterComponent::intensity,          0.0f),
		"spawnRadius",        MemberProperty(&ParticleEmitterComponent::spawnRadius,        0.0f),
		"outwardSpeed",       MemberProperty(&ParticleEmitterComponent::outwardSpeed,       0.0f),
		"startSize",          MemberProperty(&ParticleEmitterComponent::startSize,          0.0f),

		"burst", [](const ParticleEmitterHandle& handle, int count)
			{
				if (ParticleEmitterComponent* emitter = handle.TryGetComponent())
					emitter->burstCount += glm::max(count, 0);
			}
	);
}

void LuaBindings::RegisterAudio(sol::state& lua)
{
	// Usage in Lua (on an entity with an Audio Source): this.audioSource:play(), this.audioSource:stop(2) (fading out
	// for 2 seconds), if this.audioSource.isPlaying then ... end, this.audioSource.volume = 0.5
	lua.new_usertype<AudioSourceHandle>("AudioSource", sol::no_constructor,
		"volume", MemberProperty(&AudioSourceComponent::volume, 0.0f),

		"play", [](const AudioSourceHandle& handle)
			{
				if (EntityManager* entityManager = handle.owner.GetEntityManager())
					AudioSystem::Play(*entityManager, handle.owner.GetID());
			},

		"stop", [](const AudioSourceHandle& handle, sol::optional<float> fadeOutSeconds)
			{
				if (EntityManager* entityManager = handle.owner.GetEntityManager())
					AudioSystem::Stop(*entityManager, handle.owner.GetID(), fadeOutSeconds.value_or(0.0f));
			},

		"isPlaying", sol::readonly_property([](const AudioSourceHandle& handle)
			{
				EntityManager* entityManager = handle.owner.GetEntityManager();
				return entityManager && AudioSystem::IsPlaying(*entityManager, handle.owner.GetID());
			})
	);

	// Usage in Lua: Audio.play("@game/Sounds/UI/bell.ogg"), Audio.playAt("@game/Sounds/Combat/Hit", this.transform.position),
	// Audio.playMusic("@game/Music/boss.ogg", 0.6), Audio.stopMusic(3) (fading out for 3 seconds)
	// A sound may be a folder: one of its files plays at random. Volume 1 = as recorded
	lua["Audio"] = lua.create_table_with(
		"play", [](const std::string& sound, sol::optional<float> volume)
			{
				Audio::PlayOptions options;
				options.volume = volume.value_or(1.0f);
				Audio::Play(sound, options);
			},

		// Audio.playVoice("@game/Sounds/Voice/line.ogg", 1.5): a line spoken, the music quieter under it
		// (Audio.setMusicDucking(0.35): how loud the music stays meanwhile)
		"playVoice", [](const std::string& sound, sol::optional<float> volume)
			{
				Audio::PlayOptions options;
				options.volume     = volume.value_or(1.0f);
				options.ducksMusic = true;
				Audio::Play(sound, options);
			},

		"setMusicDucking", [](float volume)
			{
				Audio::SetMusicDucking(volume);
			},

		"playAt", [](const std::string& sound, const glm::vec3& position, sol::optional<float> volume)
			{
				Audio::PlayOptions options;
				options.volume    = volume.value_or(1.0f);
				options.isSpatial = true;
				options.position  = position;
				Audio::Play(sound, options);
			},

		"playMusic", [](const std::string& sound, sol::optional<float> volume, sol::optional<float> fadeSeconds)
			{
				Audio::PlayMusic(sound, volume.value_or(1.0f), fadeSeconds.value_or(2.0f));
			},

		"stopMusic", [](sol::optional<float> fadeSeconds)
			{
				Audio::StopMusic(fadeSeconds.value_or(2.0f));
			},

		// Audio.musicSeconds(): how far the music has played (-1 without music), to keep a staged moment on its beat
		"musicSeconds", []()
			{
				return Audio::GetMusicSeconds();
			},

		// Audio.seekMusic(38.8): the music jumps there (a moment of the fight meets its place in the soundtrack)
		"seekMusic", [](float seconds)
			{
				Audio::SeekMusic(seconds);
			}
	);
}

void LuaBindings::RegisterProgressComponents(sol::state& lua)
{
	// Usage in Lua: this.healingFlask:drink() (false when empty or busy), this.healingFlask.charges, this.healingFlask.maxCharges = 7
	lua.new_usertype<HealingFlaskHandle>("HealingFlask", sol::no_constructor,
		"charges",    MemberProperty(&HealingFlaskComponent::charges,    0),
		"maxCharges", MemberProperty(&HealingFlaskComponent::maxCharges, 0),
		"healAmount", MemberProperty(&HealingFlaskComponent::healAmount, 0.0f),

		"drink", [](const HealingFlaskHandle& handle)
			{
				EntityManager* entityManager = handle.owner.GetEntityManager();
				return entityManager && HealingFlaskSystem::Drink(*entityManager, handle.owner.GetID());
			}
	);

	// Usage in Lua: this.currency.amount, this.currency:add(500) (shown as a gain), this.currency:add(-100) (spent)
	lua.new_usertype<CurrencyHandle>("Currency", sol::no_constructor,
		"amount",        sol::readonly_property([](const CurrencyHandle& handle)
			{
				const CurrencyComponent* currency = handle.TryGetComponent();
				return currency ? currency->amount : 0;
			}),
		"droppedAmount", sol::readonly_property([](const CurrencyHandle& handle)
			{
				const CurrencyComponent* currency = handle.TryGetComponent();
				return currency ? currency->droppedAmount : 0;
			}),

		"add", [](const CurrencyHandle& handle, int amount)
			{
				if (EntityManager* entityManager = handle.owner.GetEntityManager())
					CurrencySystem::Add(*entityManager, handle.owner.GetID(), amount);
			}
	);

	// Usage in Lua (on an enemy): this.currencyReward.amount = 1000
	lua.new_usertype<CurrencyRewardHandle>("CurrencyReward", sol::no_constructor,
		"amount", MemberProperty(&CurrencyRewardComponent::amount, 0)
	);

	// Usage in Lua: this.levelUp.level, this.levelUp.cost (of the next level), this.levelUp:raise("health") (false when the
	// Currency is too low), this.levelUp.healthPoints / staminaPoints / damagePoints
	lua.new_usertype<LevelUpHandle>("LevelUp", sol::no_constructor,
		"level",         MemberProperty(&LevelUpComponent::level,         0),
		"healthPoints",  MemberProperty(&LevelUpComponent::healthPoints,  0),
		"staminaPoints", MemberProperty(&LevelUpComponent::staminaPoints, 0),
		"damagePoints",  MemberProperty(&LevelUpComponent::damagePoints,  0),
		"cost",          sol::readonly_property([](const LevelUpHandle& handle)
			{
				const LevelUpComponent* levelUp = handle.TryGetComponent();
				return levelUp ? levelUp->GetCost() : 0;
			}),

		"raise", [](const LevelUpHandle& handle, const std::string& attributeWord)
			{
				EntityManager* entityManager = handle.owner.GetEntityManager();
				LevelUpAttribute attribute;
				return entityManager && LevelUpSystem::TryParseAttribute(attributeWord, attribute)
					&& LevelUpSystem::Raise(*entityManager, handle.owner.GetID(), attribute);
			}
	);
}

void LuaBindings::RegisterInput(sol::state& lua)
{
	// Usage in Lua: Input.isKeyPressed(Key.W)  (held)  /  Input.isKeyDown(Key.Space)  (pressed this frame),
	// Input.isMouseButtonDown(Mouse.Left)
	lua["Input"] = lua.create_table_with(
		"isKeyPressed", [](int keyCode) { return InputManager::IsKeyPressed(keyCode); },
		"isKeyDown",    [](int keyCode) { return InputManager::IsKeyDown(keyCode);    },
		"isMouseButtonPressed", [](int button) { return InputManager::IsMouseButtonPressed(button); },
		"isMouseButtonDown",    [](int button) { return InputManager::IsMouseButtonDown(button);    }
	);

	lua["Mouse"] = lua.create_table_with(
		"Left",   SUNTA_MOUSE_BUTTON_LEFT,
		"Right",  SUNTA_MOUSE_BUTTON_RIGHT,
		"Middle", SUNTA_MOUSE_BUTTON_MIDDLE
	);

	// Add more keys here the same way, when you need them
	lua["Key"] = lua.create_table_with(
		"W",         SUNTA_KEY_W,
		"A",         SUNTA_KEY_A,
		"S",         SUNTA_KEY_S,
		"D",         SUNTA_KEY_D,
		"Space",     SUNTA_KEY_SPACE,
		"LeftShift", SUNTA_KEY_LEFT_SHIFT,
		"LeftAlt",   SUNTA_KEY_LEFT_ALT,
		"Enter",     SUNTA_KEY_ENTER,
		"Tab",       SUNTA_KEY_TAB
	);

	// Every letter (Key.R) and digit (Key.Num1): their codes are the codes of their characters
	sol::table keys = lua["Key"];
	for (char letter = 'A'; letter <= 'Z'; ++letter)
		keys[std::string(1, letter)] = static_cast<int>(letter);

	for (char digit = '0'; digit <= '9'; ++digit)
		keys[std::string("Num") + digit] = static_cast<int>(digit);

	// The function keys (Key.F2)
	for (int number = 1; number <= 12; ++number)
		keys["F" + std::to_string(number)] = SUNTA_KEY_F1 + number - 1;
}

}