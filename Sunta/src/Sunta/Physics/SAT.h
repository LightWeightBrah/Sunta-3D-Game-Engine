#pragma once

#include "CollisionShapes.h"

namespace Sunta
{
// SAT (Separating Axis Theorem)
// 
// DEFINITION:
// 2 convex shapes do NOT overlap if there exists at least one axis onto which,
// after projecting both shapes, their projected intervals ("shadows") don't touch.
// That axis is called a "separating axis".
// 
// IMPLEMENTATION:
// Project both shapes onto a candidate axis -> each becomes a 1D [min, max] range.
// If the two ranges DON'T overlap, that axis proves the shapes aren't touching
// ("separating axis"). If NO candidate axis separates them, the shapes overlap.
//
// HOW IT WORKS FOR OBB VS OBB:
// For two OBBs, only 15 axes can ever possibly be a separating axis:
//   - 3 face-normal axes of box A (local X, Y, Z)
//   - 3 face-normal axes of box B (local X, Y, Z)
//   - 9 cross products of (edge of A) x (edge of B)
// This is a known geometric fact for boxes specifically

bool Overlaps(const OBB& a, const OBB& b);

}