#include "Core/SuntaPreCompiled.h"
#include "LuaBindings.h"

#include <string>
#include <type_traits>
#include <glm/glm.hpp>

#include "ECS/Entity.h"
#include "ECS/Component.h"
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
using EnemyHandle         = ComponentHandle<EnemyComponent>;
using MeleeEnemyHandle    = ComponentHandle<MeleeEnemyComponent>;

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
	RegisterEnemyComponent(lua);
	RegisterMeleeEnemyComponent(lua);
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

void LuaBindings::RegisterHealthComponent(sol::state& lua)
{
	// Usage in Lua: if this.health.current < 30 then ... end, other.health:damage(10) (a trap), this.health.isDead
	lua.new_usertype<HealthHandle>("Health", sol::no_constructor,

		"max", MemberProperty(&HealthComponent::maxHealth, 0.0f),

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

		// "light", "heavy", "roll" or "backstep"
		"attack", [](const EnemyHandle& handle, const std::string& actionName)
			{
				CommandEnemy(handle, [&](EnemyContext& context) { context.RequestAction(ParseActionName(actionName)); });
			}
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
		"recoverSeconds",        MemberProperty(&MeleeEnemyComponent::recoverSeconds,        0.0f)
	);
}

void LuaBindings::RegisterEntity(sol::state& lua)
{
	lua.new_usertype<Entity>("Entity", sol::no_constructor,

		"id",      sol::readonly_property(&Entity::GetID),
		"isValid", sol::readonly_property(&Entity::IsValid), // false after the entity was destroyed

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
		"enemy",               ComponentHandleProperty<EnemyComponent>(),
		"meleeEnemy",          ComponentHandleProperty<MeleeEnemyComponent>()
	);
}

void LuaBindings::RegisterInput(sol::state& lua)
{
	// Usage in Lua: Input.isKeyPressed(Key.W)  (held)  /  Input.isKeyDown(Key.Space)  (pressed this frame)
	lua["Input"] = lua.create_table_with(
		"isKeyPressed", [](int keyCode) { return InputManager::IsKeyPressed(keyCode); },
		"isKeyDown",    [](int keyCode) { return InputManager::IsKeyDown(keyCode);    }
	);

	// Add more keys here the same way, when you need them
	lua["Key"] = lua.create_table_with(
		"W",         SUNTA_KEY_W,
		"A",         SUNTA_KEY_A,
		"S",         SUNTA_KEY_S,
		"D",         SUNTA_KEY_D,
		"Space",     SUNTA_KEY_SPACE,
		"LeftShift", SUNTA_KEY_LEFT_SHIFT,
		"LeftAlt",   SUNTA_KEY_LEFT_ALT
	);
}

}