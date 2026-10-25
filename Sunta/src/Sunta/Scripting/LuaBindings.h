#pragma once
#include <sol/sol.hpp>

namespace Sunta
{

// Exposes engine types and functions to Lua scripts:
// vec3, Entity (this / other), components (Transform, PhysicsBody, CharacterController, LockOn, LockOnTarget), Input and Key
class LuaBindings
{
public:
	static void Register(sol::state& lua);

private:
	static void RegisterVector3(sol::state& lua);
	static void RegisterTransformComponent(sol::state& lua);
	static void RegisterPhysicsBodyComponent(sol::state& lua);
	static void RegisterCharacterControllerComponent(sol::state& lua);
	static void RegisterLockOnComponent(sol::state& lua);
	static void RegisterLockOnTargetComponent(sol::state& lua);
	static void RegisterEntity(sol::state& lua);
	static void RegisterInput(sol::state& lua);
};

}