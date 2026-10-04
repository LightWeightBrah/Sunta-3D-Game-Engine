#include "Core/SuntaPreCompiled.h"
#include "LuaBindings.h"

#include <string>
#include <type_traits>
#include <glm/glm.hpp>

#include "ECS/Entity.h"
#include "ECS/Component.h"
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
		"physicsBody", ComponentHandleProperty<PhysicsBodyComponent>()
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