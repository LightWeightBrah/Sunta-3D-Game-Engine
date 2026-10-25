#pragma once

#include <cstddef>

#include "ECS/ComponentLayout.h"
#include "Core/EngineAssets.h"

namespace Sunta
{

// Marks an entity (an enemy, a boss) as something the player can lock on to
//
// How to use it: in the editor choose Add Component > Lock On Target on the enemy
// The player needs the Lock On component too (Add Component > Lock On), then Q locks on to the enemy in front of the camera
//
// A script turns 'isLockable' off when the enemy dies (or should not be targeted for another reason),
// the lock on it is dropped at once
struct LockOnTargetComponent
{
	// Limit of the slider in the Inspector
	static constexpr float MaxAimHeight = 5.0f;

	// Off: the enemy can't be locked on to, and a lock that already holds it is dropped
	bool isLockable = true;

	// Where the lock-on aims, as a height above the position of the entity (the chest of a person, the middle of a big boss)
	float aimHeight = 1.2f;

	static void RegisterToInspector()
	{
		InspectorComponentRegistry::RegisterComponent<LockOnTargetComponent>("Lock On Target", EngineAssets::Icons::CharacterTarget,
			{
				ADD_PROPERTY(LockOnTargetComponent, isLockable, PropertyDataType::Bool),
				ADD_PROPERTY_RANGED(LockOnTargetComponent, aimHeight, PropertyDataType::Float, 0.0f, MaxAimHeight)
			});
	}
};

}
