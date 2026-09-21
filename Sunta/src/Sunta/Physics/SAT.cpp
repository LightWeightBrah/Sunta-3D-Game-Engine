#include "Core/SuntaPreCompiled.h"
#include "SAT.h"

#include <array>
#include <cmath>
#include <glm/glm.hpp>

namespace Sunta
{

// Anonymous namespace = INTERNAL LINKAGE, the code is visible only in SAT.cpp file (static keyword would work similar)
namespace
{

struct ProjectionInterval
{
	float min;
	float max;
};

// Every point on the clipped polygon's boundary sits at the crossing of
// exactly two straight lines
struct PolygonVertex
{
	glm::vec3    position;
	BoundaryLine definingLineA;
	BoundaryLine definingLineB;
};

inline bool IntervalsOverlap(const ProjectionInterval& a, const ProjectionInterval& b)
{
	return a.min <= b.max && b.min <= a.max;
}

// How much 2 Projected Intervals Overlap on Test Axis
// Negative value means they DON'T OVERLAP
// Postive  value means they OVERLAP and shows how deep
inline float GetOverlapAmount(const ProjectionInterval& a, const ProjectionInterval& b)
{
	return std::min(a.max, b.max) - std::max(a.min, b.min);
}

// Two consecutive polygon vertices always share exactly one of their two
// defining lines, this finds it
inline BoundaryLine FindSharedBoundaryLine(const PolygonVertex& first, const PolygonVertex& second)
{
	if (first.definingLineA == second.definingLineA || first.definingLineA == second.definingLineB)
		return first.definingLineA;

	// Geometry guarantees it has to be line B if it wasn't line A
	return first.definingLineB;
}

// Projects box onto axis and returns the [min, max] range it covers there
inline ProjectionInterval ProjectOBBOntoAxis(const OBB& box, const glm::vec3& axis)
{
	// box's center projected on axis is the midpoint of the range
	float centerPositionOnAxis = glm::dot(box.center, axis);

	// box is symmetric around its center, so we only need HOW FAR it spreads to each size
	glm::vec3 spreadFromAxisX = box.orientation[0] * box.halfExtents.x;
	glm::vec3 spreadFromAxisY = box.orientation[1] * box.halfExtents.y;
	glm::vec3 spreadFromAxisZ = box.orientation[2] * box.halfExtents.z;

	float halfRangeWidth = std::abs(glm::dot(spreadFromAxisX, axis))
		+ std::abs(glm::dot(spreadFromAxisY, axis))
		+ std::abs(glm::dot(spreadFromAxisZ, axis));

	return { centerPositionOnAxis - halfRangeWidth, centerPositionOnAxis + halfRangeWidth };
}

inline bool IsAxisTooShortToCheck(const glm::vec3& axis)
{
	// Edge x edge cross product can come out near-zero for near-parallel edges -
	// that carries no direction info, so treat it as "doesn't separate" and move on
	constexpr float SHORTEST_AXIS_TO_CHECK_LENGTH_SQUARED = 1e-8f;
	return glm::dot(axis, axis) < SHORTEST_AXIS_TO_CHECK_LENGTH_SQUARED;
}

inline bool IsSeparatingAxis(const OBB& a, const OBB& b, const glm::vec3& axis)
{
	if (IsAxisTooShortToCheck(axis))
		return false;

	glm::vec3 normalizedAxis = glm::normalize(axis);
	ProjectionInterval rangeA = ProjectOBBOntoAxis(a, normalizedAxis);
	ProjectionInterval rangeB = ProjectOBBOntoAxis(b, normalizedAxis);

	return !IntervalsOverlap(rangeA, rangeB);
}

constexpr int OBB_AXES_PER_BOX = 3;															   // a box has 3 local axes: X, Y, Z
constexpr int OBB_VS_OBB_FACE_AXIS_COUNT = OBB_AXES_PER_BOX * 2;							   // A 3 axes  +  B 3 axes = 6
constexpr int OBB_VS_OBB_EDGE_AXIS_COUNT = OBB_AXES_PER_BOX * OBB_AXES_PER_BOX;				   // every A axis x every B axis = 9
constexpr int OBB_VS_OBB_AXIS_COUNT = OBB_VS_OBB_FACE_AXIS_COUNT + OBB_VS_OBB_EDGE_AXIS_COUNT; // 6 + 9 = 15

inline std::array<glm::vec3, OBB_VS_OBB_AXIS_COUNT> BuildBoxVsBoxAxes(const OBB& a, const OBB& b)
{
	std::array<glm::vec3, OBB_VS_OBB_AXIS_COUNT> testAxes{};

	// FIRST 3: Box A own face-normal axes
	for (int axis = 0; axis < OBB_AXES_PER_BOX; axis++)
		testAxes[axis] = a.orientation[axis];

	// NEXT 3: Box B own face-normal axes
	for (int axis = 0; axis < OBB_AXES_PER_BOX; axis++)
		testAxes[OBB_AXES_PER_BOX + axis] = b.orientation[axis];

	// LAST 9: Every cross-product combination of (A axis) x (B axis) 
	for (int axisOfA = 0; axisOfA < OBB_AXES_PER_BOX; axisOfA++)
	{
		for (int axisOfB = 0; axisOfB < OBB_AXES_PER_BOX; axisOfB++)
		{
			int slot = OBB_VS_OBB_FACE_AXIS_COUNT + (axisOfA * OBB_AXES_PER_BOX + axisOfB);
			testAxes[slot] = glm::cross(a.orientation[axisOfA], b.orientation[axisOfB]);
		}
	}

	return testAxes;
}

}

namespace
{

// FACE CONTACT POINTS (finding where 2 flat faces touch)
// 
// ANALOGY:
// Imagine pressing a cookie cutter down onto rolled-out dough.
//   - the COOKIE CUTTER's shape = the "reference face" 
//     (face on one box, the other box's surface being pushed into, like a table)
// 
//   - the DOUGH                 = the "incident face" 
//     (the other box's face poking into it, like a crate resting on the table)
// 
//   - whatever's left after cutting = the real touching area (our contact points)
//
// WHOLE FACE CONTACT POINTS PROCESS:
//   STEP 1: FindFaceThatPointsTowards()               - pick which of the 6 faces to use
//   STEP 2: GetCornersOfFace()                        - get that face's 4 corners
//   STEP 3: TrimIncidentFaceToReferenceFaceBoundary() - the "cookie cutter" step
//   STEP 4: KeepOnlyThePenetratingPoints()            - throw away anything not
//                                                       poking INTO the reference face
// 

constexpr int BOX_FACE_COUNT        = 6;
constexpr int BOX_FACE_CORNER_COUNT = 4;

// Local Corner indicesfor each FACE (matching CollisionShapes.h BOX_CORNER_SIGNS)
constexpr int BOX_FACE_CORNER_INDICES[BOX_FACE_COUNT][BOX_FACE_CORNER_COUNT] =
{
	// Order matters: walk around the square's edges, NEVER across
	// (that makes a crossed/broken shape instead of a square)

	{1, 2, 6, 5}, // +X face
	{0, 4, 7, 3}, // -X face
	{4, 5, 6, 7}, // +Y face
	{0, 3, 2, 1}, // -Y face
	{3, 2, 6, 7}, // +Z face
	{0, 1, 5, 4}  // -Z face
};

// Box has 6 faces. This returns the direction one of them faces
// (pointing straight out and away from the box)
// Numbering: 0=+X, 1=-X, 
//            2=+Y, 3=-Y, 
//            4=+Z, 5=-Z
inline glm::vec3 GetBoxFaceOutwardNormal(const OBB& box, int faceIndex)
{
	int axis = faceIndex / 2; // 0=X, 1=Y, 2=Z
	float sign = (faceIndex % 2 == 0) ? 1.0f : -1.0f; // (+) (-)
	return box.orientation[axis] * sign;
}

// STEP 1: 
// Out of a box's 6 faces, which one points most closely towards `direction`? 
// E.g.: 
// If `direction` points straight up, this returns box's TOP face.
inline int FindBoxFaceThatPointsTowards(const OBB& box, const glm::vec3& direction)
{
	int bestFaceIndex = 0;
	float bestAlignment = -std::numeric_limits<float>::max();

	for (int faceIndex = 0; faceIndex < BOX_FACE_COUNT; faceIndex++)
	{
		// dot product measures "how well-aligned" 2 directions are:
		// the bigger the number - the more they point in the same way
		float alignment = glm::dot(GetBoxFaceOutwardNormal(box, faceIndex), direction);

		if (alignment > bestAlignment)
		{
			bestAlignment = alignment;
			bestFaceIndex = faceIndex;
		}
	}

	return bestFaceIndex;
}

// STEP 2: Get the 4 corners positions (in world space) of Box's one face
inline std::array<glm::vec3, BOX_FACE_CORNER_COUNT> GetBoxCornersOfFace(const OBB& box, int faceIndex)
{
	std::array<glm::vec3, BOX_CORNER_COUNT> allEightCorners = GetOBBCorners(box);
	std::array<glm::vec3, BOX_FACE_CORNER_COUNT> corners{};

	for (int i = 0; i < BOX_FACE_CORNER_COUNT; i++)
		corners[i] = allEightCorners[BOX_FACE_CORNER_INDICES[faceIndex][i]];

	return corners;
}

inline glm::vec3 GetBoxPolygonCenter(const std::array<glm::vec3, BOX_FACE_CORNER_COUNT>& corners)
{
	glm::vec3 sum(0.0f);
	for (const glm::vec3& corner : corners)
		sum += corner;

	return sum / static_cast<float>(BOX_FACE_CORNER_COUNT);
}

// Plane is an infinite, flat wall in 3D space
// A point sits EXACTLY on the wall when:
// dot(outwardNormal, point) == offsetFromWorldOrigin
struct Plane
{
	glm::vec3 outwardNormal;
	float offsetFromWorldOrigin;
};

// Which side of the wall is a point on?
//   negative = "inside"  (behind the wall - the side outwardNormal points AWAY from)
//   positive = "outside" (in front of the wall - the side outwardNormal points TOWARDS)
//   zero     = exactly touching the wall
inline float GetSignedDistanceToPlane(const Plane& plane, const glm::vec3& point)
{
	return glm::dot(plane.outwardNormal, point) - plane.offsetFromWorldOrigin;
}

// Algorithm: "Sutherland-Hodgman polygon clipping"
// 
// HOW IT WORKS : Go through the polygon's edges one at a time
//   - If the edge's START point is inside -> keep it
//   - If the edge CROSSES the wall (one end inside, one outside) -> find the
//     exact crossing point and add THAT

inline std::vector<PolygonVertex> TrimPolygonToInsideOfPlane(const std::vector<PolygonVertex>& polygon, const Plane& plane, int wallIndex)
{
	std::vector<PolygonVertex> trimmedPolygon;
	if (polygon.empty())
		return trimmedPolygon;

	for (unsigned int i = 0; i < polygon.size(); i++)
	{
		const PolygonVertex& edgeStart = polygon[i];
		const PolygonVertex& edgeEnd   = polygon[(i + 1) % polygon.size()];

		float edgeStartDistance = GetSignedDistanceToPlane(plane, edgeStart.position);
		float edgeEndDistance   = GetSignedDistanceToPlane(plane, edgeEnd.position);

		bool edgeStartIsInside = edgeStartDistance <= 0.0f;
		bool edgeEndIsInside   = edgeEndDistance   <= 0.0f;

		if (edgeStartIsInside)
			trimmedPolygon.push_back(edgeStart);

		bool edgeCrossesWall = (edgeStartIsInside != edgeEndIsInside);
		if (edgeCrossesWall)
		{
			// How far along the edge (0 = start, 1 = end) does it cross the wall
			float crossingFraction = edgeStartDistance / (edgeStartDistance - edgeEndDistance);
			glm::vec3 crossingPoint = edgeStart.position + crossingFraction * (edgeEnd.position - edgeStart.position);

			BoundaryLine sharedLine = FindSharedBoundaryLine(edgeStart, edgeEnd);
			BoundaryLine clippingWall{ BoundaryLineType::ReferenceFaceWall, wallIndex };

			trimmedPolygon.push_back({ crossingPoint, sharedLine, clippingWall });
		}
	}

	return trimmedPolygon;
}

// Build the wall that stand up straight up from one edge of the reference face
inline Plane BuildSidePlaneForReferenceEdge(
	const glm::vec3& edgeStart, const glm::vec3& edgeEnd,
	const glm::vec3& referenceFaceNormal, const glm::vec3& referenceFaceCenter)
{
	glm::vec3 edgeDirection = edgeEnd - edgeStart;
	glm::vec3 wallNormal = glm::cross(referenceFaceNormal, edgeDirection);

	// The face's 4 corners might be listed clockwise or counter-clockwise
	// This makes sure the wall always faces AWAY from the face's own center
	glm::vec3 edgeMidpoint = (edgeStart + edgeEnd) * 0.5f;
	if (glm::dot(wallNormal, edgeMidpoint - referenceFaceCenter) < 0.0f)
		wallNormal = -wallNormal;

	return Plane{ wallNormal, glm::dot(wallNormal, edgeStart) };
}

// STEP 3 (the "cookie cutter" step): 
// trims the incident face down to only the part that falls within the reference face's 4 edges
inline std::vector<PolygonVertex> TrimBoxIncidentFaceToReferenceFaceBoundary(
	const std::array<glm::vec3, BOX_FACE_CORNER_COUNT>& referenceFaceCorners,
	const glm::vec3& referenceFaceNormal,
	const std::array<glm::vec3, BOX_FACE_CORNER_COUNT>& incidentFaceCorners)
{
	std::vector<PolygonVertex> remainingPolygon;
	remainingPolygon.reserve(BOX_FACE_CORNER_COUNT);

	for (int i = 0; i < BOX_FACE_CORNER_COUNT; i++)
	{
		int previousEdgeIndex = (i + BOX_FACE_CORNER_COUNT - 1) % BOX_FACE_CORNER_COUNT;

		remainingPolygon.push_back({
			incidentFaceCorners[i],
			BoundaryLine{ BoundaryLineType::IncidentFaceEdge, previousEdgeIndex },
			BoundaryLine{ BoundaryLineType::IncidentFaceEdge, i }
		});
	}

	glm::vec3 referenceFaceCenter = GetBoxPolygonCenter(referenceFaceCorners);

	for (int wallIndex = 0; wallIndex < BOX_FACE_CORNER_COUNT && !remainingPolygon.empty(); wallIndex++)
	{
		glm::vec3 edgeStart = referenceFaceCorners[wallIndex];
		glm::vec3 edgeEnd   = referenceFaceCorners[(wallIndex + 1) % BOX_FACE_CORNER_COUNT];

		Plane sideWall = BuildSidePlaneForReferenceEdge(edgeStart, edgeEnd, referenceFaceNormal, referenceFaceCenter);
		remainingPolygon = TrimPolygonToInsideOfPlane(remainingPolygon, sideWall, wallIndex);
	}

	return remainingPolygon;
}

// STEP 4:
// STEP 3 only clips the polygon to the right SHAPE (as seen from above)
// but it DOESN'T CHECK DEPTH. A tilted face can have corners that are inside
// the boundary but actually lifted away from the reference face, not
// touching it. This keeps only the points that are behind the face 
// (along its normal) and removes the rest
inline std::vector<ContactPoint> KeepOnlyThePenetratingPoints(
	const std::vector<PolygonVertex>& candidatePoints,
	const glm::vec3& referenceFaceNormal,
	const glm::vec3& referenceFaceCenter
)
{
	std::vector<ContactPoint> contacts;

	for (const PolygonVertex& vertex : candidatePoints)
	{
		float howFarBehindTheFace = -glm::dot(referenceFaceNormal, vertex.position - referenceFaceCenter);

		bool isPenetratingPoint = howFarBehindTheFace >= 0.0f;
		if (isPenetratingPoint)
		{
			ContactPoint contactPoint;
			contactPoint.worldPosition    = vertex.position;
			contactPoint.penetrationDepth = howFarBehindTheFace;
			contactPoint.featureID.lineA  = vertex.definingLineA;
			contactPoint.featureID.lineB  = vertex.definingLineB;

			// referenceFaceIndex/incidentFaceIndex filled in by the caller,
			// which is the only place that actually knows those
			contacts.push_back(contactPoint);

		}
	}

	return contacts;
}

// All 4 steps added up
inline std::vector<ContactPoint> FindFaceContactPoints(const OBB& referenceBox, const OBB& incidentBox, const glm::vec3& referenceFaceNormal)
{
	// STEP 1 + 2 for the reference box
	int referenceFaceIndex = FindBoxFaceThatPointsTowards(referenceBox, referenceFaceNormal);
	std::array<glm::vec3, BOX_FACE_CORNER_COUNT> referenceFaceCorners = GetBoxCornersOfFace(referenceBox, referenceFaceIndex);

	// STEP 1 + 2 for the incident box (FACE POINTS OPPOOSITE TO THE REFERENCE NORMAL)
	// E.g. bottom of a crate resting on a table points straight DOWN, opposite to the table UP normal
	int incidentFaceIndex = FindBoxFaceThatPointsTowards(incidentBox, -referenceFaceNormal);
	std::array<glm::vec3, BOX_FACE_CORNER_COUNT> incidentFaceCorners = GetBoxCornersOfFace(incidentBox, incidentFaceIndex);

	// STEP 3
	std::vector<PolygonVertex> clippedPolygon = TrimBoxIncidentFaceToReferenceFaceBoundary(referenceFaceCorners, referenceFaceNormal, incidentFaceCorners);

	// STEP 4
	glm::vec3 referenceFaceCenter = GetBoxPolygonCenter(referenceFaceCorners);
	std::vector<ContactPoint> contactPoints = KeepOnlyThePenetratingPoints(clippedPolygon, referenceFaceNormal, referenceFaceCenter);

	for (ContactPoint& contact : contactPoints)
	{
		contact.featureID.type				 = CollisionContactType::FaceToFace;
		contact.featureID.referenceFaceIndex = referenceFaceIndex;
		contact.featureID.incidentFaceIndex  = incidentFaceIndex;
	}

	return contactPoints;
}

// EDGE-EDGE contact
struct Segment
{
	glm::vec3 start;
	glm::vec3 end;
};

// Box has 4 parallel edges on each axis, this picks the one CLOSEST to the other box
inline Segment GetBoxEdgeSegment(const OBB& box, int edgeAxisIndex, const glm::vec3& directionTowardsOtherBox)
{
	glm::vec3 offsetFromCenter = glm::vec3(0.0f);

	for (unsigned int axis = 0; axis < OBB_AXES_PER_BOX; axis++)
	{
		if(axis == edgeAxisIndex)
			continue;

		float sign = (glm::dot(directionTowardsOtherBox, box.orientation[axis]) >= 0.0f) ? 1.0f : -1.0f;
		offsetFromCenter += box.orientation[axis] * (box.halfExtents[axis] * sign);
	}

	glm::vec3 edgeDirection = box.orientation[edgeAxisIndex];
	glm::vec3 edgeCenter = box.center + offsetFromCenter;
	float halfLength = box.halfExtents[edgeAxisIndex];

	return { edgeCenter - edgeDirection * halfLength, edgeCenter + edgeDirection * halfLength };
}

// Finds 2 points (1 on each segment) that are closest to each other
//
// This follows the method from video:
// Shortest distance between two skew lines in 3D space. 
// Made by channel: 'DLBmaths' 
// Link: https://www.youtube.com/watch?v=HC5YikQxwZA
//
// I used same naming as in the video:
// 
//    L1(segmentA): P(t) = P0 + t * u1
//    L2(segmentB): Q(s) = Q0 + s * u2
inline glm::vec3 ClosestPointBetweenSegments(const Segment& segmentA, const Segment& segmentB)
{
	glm::vec3 P0 = segmentA.start;
	glm::vec3 Q0 = segmentB.start;
	glm::vec3 u1 = segmentA.end - segmentA.start;
	glm::vec3 u2 = segmentB.end - segmentB.start;

	// Write out PQ in terms of the unknown t and s:
	//
	//		PQ =  Q(s) - P(t)  =  (Q0-P0) + s*u2 - t*u1  =   P0toQ0 + s*u2 - t*u1
	//
	// So now PQ = P0toQ0 + s*u2 - t*u1

	glm::vec3 P0toQ0 = Q0 - P0;

	// Now Vector PQ must be perpendicular to Line1 (segmentA) u1 
	//                   and perpendicular to Line2 (segmentB) u2
	// 
	// We can check it via Dot product:
	// 
	// PQ . u1 = 0 
	// PQ . u2 = 0
	// 
	// Now we expand each dot product using the fact that:
	// 
	// (a + b) . c = a.c + b.c
	// 
	// EQUATION 1: 
	// (PQ . u1 = 0): 
	// (P0toQ0 + s*u2 - t*u1) . u1 = 0
	// (P0toQ0 . u1) + s(u2 . u1) - t(u1 . u1) = 0
	// 
	// EQUATION 2: 
	// (PQ . u2 = 0): 
	// (P0toQ0 + s*u2 - t*u1) . u2 = 0
	// (P0toQ0 . u2) + s(u2 . u2) - t(u1 . u2) = 0
	//
	// Note: 
	// 1) Dot product of the vector with itself = always the square of the length:
	// (u1 . u1) = ALWAYS squared length of u1
	// (u2 . u2) = ALWAYS squared length of u2
	// 
	// 2) Dot product measures how aligned are 2 vectors, so it doesn't care about the order:
	// (u1 . u2) = (u2 . u1)
	// 
	//

	float u1DotU1 = glm::dot(u1, u1); // this could be called u1LengthSquared
	float u2DotU2 = glm::dot(u2, u2); // this could be called u2LengthSquared
	float u1DotU2 = glm::dot(u1, u2);
	float P0toQ0DotU1 = glm::dot(P0toQ0, u1);
	float P0toQ0DotU2 = glm::dot(P0toQ0, u2);

	// Segment (line) with near 0 length isn't a real line, 
	// but just a single point (start == end) 
	// If we have 0 length u1 or u2 we'll get divide by 0 error
	// so we handle those cases

	constexpr float EPSILON = 1e-8f;
	bool segmentAIsPoint = u1DotU1 <= EPSILON;
	bool segmentBIsPoint = u2DotU2 <= EPSILON;

	float t = 0.0f;
	float s = 0.0f;

	if (segmentAIsPoint && segmentBIsPoint)
	{
		// Both lines are just single points so nothing to solve
		t = 0.0f;
		s = 0.0f;
	}
	else if (segmentAIsPoint)
	{
		// L1 = single point (P0), so just find the closest point to it on L2
		// 
		// Q(s) - P0 = (Q0 + s*u2) - P0 = P0toQ0 + s*u2
		// (P0toQ0 + s*u2) . u2 = 0
		// (P0toQ0 . u2)  +  s*(u2 . u2)  =  0
		// P0toQ0DotU2 + s * u2DotU2 = 0
		// s = -P0toQ0DotU2 / u2DotU2
		t = 0.0f;
		s = -P0toQ0DotU2 / u2DotU2;
	}
	else if (segmentBIsPoint)
	{
		// L2 = single point (Q0), so just find the closest point to it on L1
		// 
		// Q0 - P(t) = Q0 - (P0 + t*u1) = P0toQ0 - t*u1
		// (P0toQ0 - t*u1) . u1 = 0
		// (P0toQ0 . u1)  -  t*(u1 . u1) = 0
		// P0toQ0DotU1 - t * u1DotU1 = 0
		// t = P0toQ0DotU1 / u1DotU1
		t = P0toQ0DotU1 / u1DotU1;
		s = 0.0f;
	}
	else
	{
		// Now we can change
		//
		// EQUATION 1: (P0toQ0 . u1) + s(u2 . u1) - t(u1 . u1) = 0
		// EQUATION 2: (P0toQ0 . u2) + s(u2 . u2) - t(u1 . u2) = 0
		//
		// To:
		// EQUATION 1:  u1DotU1 * t  -  u1DotU2 * s  =  P0toQ0DotU1
		// EQUATION 2:  u1DotU2 * t  -  u2DotU2 * s  =  P0toQ0DotU2
		//
		// ===============================================================
		// SOLVING FOR 's' (get 't' from EQUATION 1, plug into EQUATION 2
		// ===============================================================
		// 
		// For:
		// EQUATION 1:  u1DotU1 * t  -  u1DotU2 * s  =  P0toQ0DotU1
		// 
		// We get t:
		// EQUATION 1:  t = (P0toQ0DotU1 + u1DotU2 * s) / u1DotU1 
		// 
		// Now use that t in EQUATION2:
		// 
		// EQUATION2: u1DotU2 * [(P0toQ0DotU1 + u1DotU2*s) / u1DotU1]  -  u2DotU2*s  =  P0toQ0DotU2
		// 
		// Now we multiply both sides by u1DotU1 to remove the fraction
		// 
		// EQUATION2: u1DotU2*P0toQ0DotU1 + (u1DotU2*u1DotU2)*s - u1DotU1*u2DotU2*s = P0toQ0DotU2 * u1DotU1
		// 
		// Now get all 's' on the left side:
		// 
		// EQUATION2: s * (u1DotU2*u1DotU2 - u1DotU1*u2DotU2) = P0toQ0DotU2*u1DotU1 - u1DotU2*P0toQ0DotU1
		//
		// Now we multiply both sides by -1 (just to flip the sign so it
		// matches the form we'll get for 't' below, doesn't change the answer)
		// 
		// EQUATION2: s * (u1DotU1*u2DotU2 - u1DotU2*u1DotU2) = u1DotU2*P0toQ0DotU1 - u1DotU1*P0toQ0DotU2
		//
		// And we get 
		// 
		// s = [u1DotU2*P0toQ0DotU1 - u1DotU1*P0toQ0DotU2] / (u1DotU1*u2DotU2 - u1DotU2*u1DotU2)
		// 
		// ===============================================================
		// SOLVING FOR 't' (get 's' from EQUATION 2, plug into EQUATION 1
		// ===============================================================
		// 
		// EQUATION 1:  u1DotU1 * t  -  u1DotU2 * s  =  P0toQ0DotU1
		// EQUATION 2:  u1DotU2 * t  -  u2DotU2 * s  =  P0toQ0DotU2
		// 
		// For: 
		// EQUATION 2: u1DotU2 * t  -  u2DotU2 * s  =  P0toQ0DotU2
		// 
		// We get s:
		// EQUATION 2: s = (u1DotU2*t - P0toQ0DotU2) / u2DotU2
		//
		// Now use that s in EQUATION 1:
		//
		// EQUATION 1: u1DotU1*t  -  u1DotU2 * [(u1DotU2*t - P0toQ0DotU2) / u2DotU2]  =  P0toQ0DotU1
		//
		// Now we multiply both sides by u2DotU2 to remove the fraction
		//
		// EQUATION 1: u1DotU1*u2DotU2*t - u1DotU2*(u1DotU2*t - P0toQ0DotU2) = P0toQ0DotU1 * u2DotU2
		//
		// Expand the bracket (- times - becomes +):
		//
		// EQUATION 1: u1DotU1*u2DotU2*t - u1DotU2*u1DotU2*t + u1DotU2*P0toQ0DotU2 = P0toQ0DotU1*u2DotU2
		//
		// Now get all 't' on the left side:
		//
		// EQUATION 1: t * (u1DotU1*u2DotU2 - u1DotU2*u1DotU2) = P0toQ0DotU1*u2DotU2 - u1DotU2*P0toQ0DotU2
		//
		// (no sign-flip needed this time as it already came out matching)
		//
		// And we get:
		//
		// t = [P0toQ0DotU1*u2DotU2 - u1DotU2*P0toQ0DotU2] / (u1DotU1*u2DotU2 - u1DotU2*u1DotU2)
		// 
		// ======================================================================================
		// So 't' and 's' are:
		// t = [P0toQ0DotU1*u2DotU2 - u1DotU2*P0toQ0DotU2] / (u1DotU1*u2DotU2 - u1DotU2*u1DotU2)
		// s = [u1DotU2*P0toQ0DotU1 - u1DotU1*P0toQ0DotU2] / (u1DotU1*u2DotU2 - u1DotU2*u1DotU2)
		// ======================================================================================
		// 
		// ====================================================================== 
		// VERY IMPORTANT:
		// 
		// The divider becomes our determinant because 
		// it's the shared piece for equations, so:
		// 
		//     (u1DotU1*u2DotU2 - u1DotU2*u1DotU2) is our DETERMINANT!!!
		// 
		// ======================================================================

		float determinant = u1DotU1 * u2DotU2 - u1DotU2 * u1DotU2;

		// id determinant == 0, there's no single unqiue(t, s) answer
		// so that means that u1 and u2 are PARALLEL
		// and video's method assumes 2 lines cross at an angle, 
		// so when no single result for (t, s) use t = 0, s = 0

		if (determinant > EPSILON)
		{
			t = (P0toQ0DotU1 * u2DotU2 - u1DotU2 * P0toQ0DotU2) / determinant;
			s = (u1DotU2 * P0toQ0DotU1 - u1DotU1 * P0toQ0DotU2) / determinant;
		}

	}

	// Because this we need this algorithm for short EDGES of a box
	// and not 2 infinetely long lines like on the video 
	// Clamp 't' and 's' to stay BETWEEN: 
	// 0 (the edge's start corner)
	// 1 (the edge's end corner)
	// So closeset point can't be off the end of real physical EDGE

	t = glm::clamp(t, 0.0f, 1.0f);
	s = glm::clamp(s, 0.0f, 1.0f);

	glm::vec3 P = P0 + u1 * t;
	glm::vec3 Q = Q0 + u2 * s;

	// 2 box edges are slightly overlapping, so P and Q 
	// are right next to each other, so we take the point 
	// between them as the contact point

	return (P + Q) * 0.5f;
}

}

bool Overlaps(const OBB& a, const OBB& b)
{
	std::array<glm::vec3, OBB_VS_OBB_AXIS_COUNT> testAxes = BuildBoxVsBoxAxes(a, b);

	for (const glm::vec3& axis : testAxes)
	{
		// if at least ONE separating axis is found, we're CERTAIN boxes DON'T OVERLAP
		if (IsSeparatingAxis(a, b, axis))
			return false;
	}

	// If none of the 15 candidates axes separated the boxes => THEY OVERLAP
	return true;
}

SeparationInfo GetSeparationInfo(const OBB& a, const OBB& b)
{
	SeparationInfo result;

	std::array<glm::vec3, OBB_VS_OBB_AXIS_COUNT> testAxes = BuildBoxVsBoxAxes(a, b);

	float shallowestOverlapFound = std::numeric_limits<float>::max();
	glm::vec3 axisOfShallowestOverlap = glm::vec3(0.0f);

	for (const glm::vec3& axis : testAxes)
	{
		if(IsAxisTooShortToCheck(axis))
			continue;

		glm::vec3 normalizedAxis = glm::normalize(axis);

		ProjectionInterval rangeA = ProjectOBBOntoAxis(a, normalizedAxis);
		ProjectionInterval rangeB = ProjectOBBOntoAxis(b, normalizedAxis);

		float overlapAmount = GetOverlapAmount(rangeA, rangeB);

		if (overlapAmount < 0.0f)
		{
			// this axis separates the boxes, so colliders don't overlap
			// if at least 1 axis separates, then there's no overlap
			result.areOverlapping = false;
			return result;
		}

		if (overlapAmount < shallowestOverlapFound)
		{
			shallowestOverlapFound = overlapAmount;
			axisOfShallowestOverlap = normalizedAxis;
		}
	}

	result.areOverlapping = true;
	result.overlapDepth = shallowestOverlapFound;

	// Axis came from a face normal or cross product (it doesn't have direction we need)
	// so it could point towards A or towards B (WE DON'T KNOW)
	// We want push direction to ALWAYS go from A to B, so flip the axis if it's pointing the wrong way
	glm::vec3 directionFromCenterAToCenterB = b.center - a.center;
	bool axisAlreadyPointTowardsB = glm::dot(directionFromCenterAToCenterB, axisOfShallowestOverlap) >= 0.0f;

	result.pushDirectionFromAToB = axisAlreadyPointTowardsB ? axisOfShallowestOverlap : -axisOfShallowestOverlap;

	return result;
}

ContactManifold GetContactManifoldBoxVsBox(const OBB& a, const OBB& b)
{
	ContactManifold manifold;

	std::array<glm::vec3, OBB_VS_OBB_AXIS_COUNT> testAxes = BuildBoxVsBoxAxes(a, b);

	float shallowestOverlapFound = std::numeric_limits<float>::max();
	int shallowestAxisIndex = -1;
	glm::vec3 shallowestOverlapAxis = glm::vec3(0.0f);

	for (unsigned int i = 0; i < OBB_VS_OBB_AXIS_COUNT; i++)
	{
		const glm::vec3& axis = testAxes[i];

		if (IsAxisTooShortToCheck(axis))
			continue;

		glm::vec3 normalizedAxis = glm::normalize(axis);

		ProjectionInterval rangeA = ProjectOBBOntoAxis(a, normalizedAxis);
		ProjectionInterval rangeB = ProjectOBBOntoAxis(b, normalizedAxis);

		float overlapAmount = GetOverlapAmount(rangeA, rangeB);

		if (overlapAmount < 0.0f)
		{
			// this axis separates the boxes, so colliders don't overlap
			// if at least 1 axis separates, then there's no overlap
			manifold.areOverlapping = false;
			return manifold;
		}

		if (overlapAmount < shallowestOverlapFound)
		{
			shallowestOverlapFound = overlapAmount;
			shallowestAxisIndex = i;
			shallowestOverlapAxis = normalizedAxis;
		}
	}

	manifold.areOverlapping = true;
	manifold.overlapDepth = shallowestOverlapFound;

	// Axis came from a face normal or cross product (it doesn't have direction we need)
	// so it could point towards A or towards B (WE DON'T KNOW)
	// We want push direction to ALWAYS go from A to B, so flip the axis if it's pointing the wrong way
	glm::vec3 directionFromCenterAToCenterB = b.center - a.center;
	bool axisAlreadyPointTowardsB = glm::dot(directionFromCenterAToCenterB, shallowestOverlapAxis) >= 0.0f;

	manifold.pushDirectionFromAToB = axisAlreadyPointTowardsB ? shallowestOverlapAxis : -shallowestOverlapAxis;

	// We can do this cause first we got 6 face axis, then we got 9 edge axis
	bool shallowestWasFaceAxis = shallowestAxisIndex < OBB_VS_OBB_FACE_AXIS_COUNT;

	if (shallowestWasFaceAxis)
	{
		bool referenceIsA = shallowestAxisIndex < OBB_AXES_PER_BOX;
		const OBB& referenceBox = referenceIsA ? a : b;
		const OBB& incidentBox  = referenceIsA ? b : a;

		glm::vec3 referenceFaceNormal = referenceIsA ? manifold.pushDirectionFromAToB : -manifold.pushDirectionFromAToB;

		manifold.contacts = FindFaceContactPoints(referenceBox, incidentBox, referenceFaceNormal);
	}
	else
	{
		// Edge box A, box B axis placement (from BuildBoxVsBoxAxes)
		// 
		//                 axisOfB=0   axisOfB=1   axisOfB=2
		// axisOfA = 0         0           1           2
		// axisOfA = 1         3           4           5
		// axisOfA = 2         6           7           8
		// 
		int edgeSlot = shallowestAxisIndex - OBB_VS_OBB_FACE_AXIS_COUNT;
		int axisOfA = edgeSlot / OBB_AXES_PER_BOX; // which row    (which of box A's 3 axes)
		int axisOfB = edgeSlot % OBB_AXES_PER_BOX; // which column (which of box B's 3 axes)

		Segment edgeOnA = GetBoxEdgeSegment(a, axisOfA,  directionFromCenterAToCenterB);
		Segment edgeOnB = GetBoxEdgeSegment(b, axisOfB, -directionFromCenterAToCenterB);

		glm::vec3 contactPoint = ClosestPointBetweenSegments(edgeOnA, edgeOnB);

		ContactPoint contact;
		contact.worldPosition    = contactPoint;
		contact.penetrationDepth = manifold.overlapDepth;

		contact.featureID.type           = CollisionContactType::EdgeToEdge;
		contact.featureID.edgeAxisOnBoxA = axisOfA;
		contact.featureID.edgeAxisOnBoxB = axisOfB;

		manifold.contacts.push_back(contact);
	}

	return manifold;
}

}