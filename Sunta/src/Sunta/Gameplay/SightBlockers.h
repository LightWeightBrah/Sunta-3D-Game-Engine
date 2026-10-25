#pragma once

#include "Physics/CollisionLayers.h"

namespace Sunta
{

// What the lock-on (can the target be seen?) and the follow camera (is a wall in the way?) treat as an obstacle:
// the colliders on the default collision layer (slot 0, "Environment" in a new project)
// So walls and floors should stay on that layer, and characters and enemies belong on their own layers
constexpr CollisionMask SightBlockingLayers = ToMask(DefaultCollisionLayer);

}
