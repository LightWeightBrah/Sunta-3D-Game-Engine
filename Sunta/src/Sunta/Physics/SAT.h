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
//
// CONTACT MANIFOLD (needed to make a hit realistically spin an object):
// Knowing THAT two boxes overlap, and BY HOW MUCH, isn't enough to make an
// impact spin an object correctly, we also need to know WHERE exactly the
// boxes touch (torque depends on how far the contact point is from the
// object's center). Finding that point is called "contact manifold
// generation". For boxes:
// 
//   - IF THE SHALLOWEST-OVERLAP AXIS WAS A FACE AXIS:
// 
//	   This means the boxes collided flat against each other 
//     (e.g. a box resting flat on the floor). 
// 
//	   The algorithm "clips" the other box's closest face against this box's face
//     Whatever survives the clip IS the touching area (up to 4 points) 
//     (e.g. a box resting flat touches at all 4 of its bottom corners)
// 
//     Algorithm: "Sutherland-Hodgman polygon clipping"
// 
//   - IF THE SHALLOWEST-OVERLAP AXIS WAS AN EDGE AXIS:
//     This means the boxes did NOT collide flat, but rather hit "edge-to-edge" 
//     or "corner-to-corner" at an angle.
// 
//	   The algorithm takes one edge from each box and finds the closest point 
//     between those 2 edges (3D line segments) that are touching (ALWAYS 1 point)
//
//	   Algorithm: "Shortest distance between two skew lines in 3D space"
//

enum class CollisionContactType
{
	FaceToFace,
	EdgeToEdge
};

// A contact point on a clipped face always sits at the crossing of two
// straight lines. This says WHICH TYPE of line one of those is
enum class BoundaryLineType
{
	IncidentFaceEdge,  // one of the 4 edges of the box that's poking in
	ReferenceFaceWall  // one of the 4 side walls used to clip it
};

// One specific straight line bounding the touching area
// Example: { IncidentFaceEdge, 2 } means "incident face's edge number 2"
struct BoundaryLine
{
	BoundaryLineType type  = BoundaryLineType::IncidentFaceEdge;
	int	             index = -1; // which one, 0-3

	bool operator==(const BoundaryLine& other) const
	{
		return type == other.type && index == other.index;
	}
};

struct ContactFeatureID
{
	CollisionContactType type = CollisionContactType::FaceToFace;

	// used when type == FaceToFace
	int referenceFaceIndex = -1;
	int incidentFaceIndex  = -1;
	BoundaryLine lineA;
	BoundaryLine lineB;

	// used when type == EdgeToEdge
	int edgeAxisOnBoxA = -1; // which local axis (0=X, 1=Y, 2=Z) box A's edge runs along
	int edgeAxisOnBoxB = -1; // which local axis (0=X, 1=Y, 2=Z) box B's edge runs along

	bool operator==(const ContactFeatureID& other) const
	{
		if (type != other.type)
			return false;

		if (type == CollisionContactType::EdgeToEdge)
			return edgeAxisOnBoxA == other.edgeAxisOnBoxA
			    && edgeAxisOnBoxB == other.edgeAxisOnBoxB;

		bool touchingSameFaces = referenceFaceIndex == other.referenceFaceIndex
			                   && incidentFaceIndex == other.incidentFaceIndex;

		// lineA/lineB can be listed in either order and still mean the same
		// crossing point, so both orders count as a match
		bool linesMatch = (lineA == other.lineA && lineB == other.lineB)
			           || (lineA == other.lineB && lineB == other.lineA);

		return touchingSameFaces && linesMatch;
	}

	bool operator!=(const ContactFeatureID& other) const { return !(*this == other); }

};

struct SeparationInfo
{
	bool      areOverlapping = false;

	glm::vec3 pushDirectionFromAToB = glm::vec3(0.0f);
	float     overlapDepth = 0.0f;
};

struct ContactPoint
{
	glm::vec3 worldPosition = glm::vec3(0.0f);

	// How deep THIS corner is pushed into the other box
	// Each corner can be pushed in by a different amount when a box lands tilted
	float penetrationDepth = 0.0f;

	// Which real corner/crossing this is (not where it is)
	ContactFeatureID featureID;
};

struct ContactManifold
{
	bool      areOverlapping = false;

	glm::vec3 pushDirectionFromAToB = glm::vec3(0.0f);
	float     overlapDepth = 0.0f;
	std::vector<ContactPoint> contacts;


};

bool Overlaps(const OBB& a, const OBB& b);
SeparationInfo GetSeparationInfo(const OBB& a, const OBB& b);
ContactManifold GetContactManifoldBoxVsBox(const OBB& a, const OBB& b);

}