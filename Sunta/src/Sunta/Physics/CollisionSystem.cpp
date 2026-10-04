#include "Core/SuntaPreCompiled.h"
#include "CollisionSystem.h"
#include "PhysicsSystem.h"

#include "ECS/EntityManager.h"
#include "ECS/Component.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"

#include <limits>
#include <unordered_set>

namespace Sunta
{

void CollisionSystem::Update(EntityManager& entityManager, float deltaTime)
{
	UpdateColliders(entityManager);
	ResolveSolidCollisions(entityManager, deltaTime);
	DetectTriggerEvents(entityManager);
}

void CollisionSystem::UpdateColliders(EntityManager& entityManager)
{
	auto& colliders = entityManager.GetAllComponents<BoxColliderComponent>();

	for (unsigned int i = 0; i < colliders.size(); i++)
	{
		unsigned int entityID = entityManager.GetEntityIDForComponent(colliders[i]);

		auto* worldMatrixComponent = entityManager.GetComponent<WorldMatrixComponent>(entityID);
		if (!worldMatrixComponent)
			continue;

		colliders[i].worldOBB = MakeWorldOBB(worldMatrixComponent->matrix, colliders[i].localOffset, colliders[i].halfExtents);

	}
}

namespace
{

// Friction happens when two surfaces slide against each other. Sliding can
// go ANY direction along the surface, so we describe it with two directions
// that both lie flat on it (the same way a map needs "east" and "north"
// to describe any spot)
//
// These are NOT world X and Z. They depend on how the surface is tilted:
// on a floor they end up being X and Z, on a wall they end up being Y and Z
struct SurfaceSlideDirections
{
	glm::vec3 firstSlideDirection  = glm::vec3(0.0f);
	glm::vec3 secondSlideDirection = glm::vec3(0.0f);
};

// ONE spot where two boxes touch each other
// A box landing flat on the floor touches at 4 spots (its 4 bottom corners)
// A box landing on its corner touches at 1
struct CollisionPoint
{
	glm::vec3 worldPosition = glm::vec3(0.0f);

	// How deep this particular point is pushed into the other box
	// A tilted box has one corner buried deep and the opposite one barely touching
	float penetrationDepth = 0.0f;

	ContactFeatureID featureID; // which real corner/crossing this is (see SAT.h)

	// Distance from each body's center out to this spot
	// Pushing far from the center makes a body spin (like a door handle)
	// Pushing through the center only makes it slide
	glm::vec3 leverArmFromCenterA = glm::vec3(0.0f);
	glm::vec3 leverArmFromCenterB = glm::vec3(0.0f);

	// "One unit of push here changes the speed by this much"
	// Heavy, hard-to-spin bodies give a small number: they need a big push
	// Worked out once per step so the solver loop stays cheap
	float speedGainPerUnitOfNormalPush      = 0.0f;
	float speedGainPerUnitOfFirstSlidePush  = 0.0f;
	float speedGainPerUnitOfSecondSlidePush = 0.0f;

	// How fast this spot should end up moving APART (0 = no bounce)
	float targetBounceSpeed = 0.0f;

	// We remember how much we have pushed in total, not just in this pass,
	// so a later pass can take back a push that an earlier one overdid
	float totalNormalPush      = 0.0f;
	float totalFirstSlidePush  = 0.0f;
	float totalSecondSlidePush = 0.0f;
};

// 2 boxes touching, plus everything that is the same for all their
// touching spots (material, which way is "out of the surface" etc.)
struct CollisionPair
{
	unsigned int entityA = 0;
	unsigned int entityB = 0;

	PhysicsBodyComponent* bodyA = nullptr;
	PhysicsBodyComponent* bodyB = nullptr;

	glm::vec3 centerOfA = glm::vec3(0.0f);
	glm::vec3 centerOfB = glm::vec3(0.0f);

	float inverseMassOfA = 0.0f;
	float inverseMassOfB = 0.0f;

	glm::mat3 inverseInertiaOfA = glm::mat3(0.0f);
	glm::mat3 inverseInertiaOfB = glm::mat3(0.0f);

	glm::vec3 pushDirectionFromAToB = glm::vec3(0.0f);

	// Which one of candidate axis (15 axis for box) was used 
	// to build contact of this pair
	// We remember this for next frame, so SAT can still use same axis 
	int usedAxisIndex = -1;

	// Flat along the surface (this is where sliding and friction happen)
	SurfaceSlideDirections slideDirections;

	float bounciness = 0.0f;
	float friction   = 0.0f;

	std::vector<CollisionPoint> points;
};

// One remembered push from last frame at 1 specific contact point
struct RememberedContactImpulse
{
	ContactFeatureID featureID; // WHICH real corner/crossing this was

	float normalPush = 0.0f; // how hard we pushed the bodies apart
	float firstSlidePush = 0.0f; // how hard we pushed sideways, direction 1
	float secondSlidePush = 0.0f; // how hard we pushed sideways, direction 2

};

// Everything we remember about ONE pair of touching entities,
// so solver in next frame can continue where he ended:
// used SAT axis & impulses in every contact point
struct RememberedPairState
{
	int usedAxisIndex = -1;
	std::vector<RememberedContactImpulse> contactImpulses;
};

struct EntityPairKey
{
	unsigned int entityA;
	unsigned int entityB;

	bool operator==(const EntityPairKey& other) const
	{
		return (entityA == other.entityA && entityB == other.entityB)
			|| (entityA == other.entityB && entityB == other.entityA);
	}
};

// Turns an EntityPairKey into 1 number, so it can be used as a map key
struct EntityPairKeyHash
{
	size_t operator()(const EntityPairKey& key) const
	{
		unsigned int smallerID = std::min(key.entityA, key.entityB);
		unsigned int biggerID  = std::max(key.entityA, key.entityB);

		// GOAL: squash two 32-bit numbers into one 64-bit number, with each
		// one living in its own separate "half", so they can never collide
		//
		// STEP 1: "<<" (left shift) slides smallerID's bits 32 places to
		// the LEFT. This moves smallerID into the UPPER half of a 64-bit
		// number, and fills the newly-empty LOWER half with zeros:
		//
		//   smallerID (say, 5):        ...00000101
		//   after << 32:  00000101 00000000000000000000000000000000
		//                 (smallerID)  (32 empty zero bits)
		//
		// STEP 2: "|" (bitwise OR) combines that with biggerID. Since the
		// lower 32 bits are all zero, OR-ing just drops biggerID straight
		// into that empty space, without touching smallerID's half at all:
		//
		//   biggerID (say, 7):                            ...00000111
		//   combined:     00000101 00000000000000000000000000000111
		//                 (smallerID lives here) (biggerID lives here)
		//
		// RESULT: one 64-bit number with both IDs sitting side by side,
		// never overlapping - which we can now hash like any other number
		unsigned long long combinedID = (static_cast<unsigned long long>(smallerID) << 32) | biggerID;

		return std::hash<unsigned long long>()(combinedID);
	}
};

// "for this pair of entities, here's how hard each of their touching points
// was pushing last frame". Rebuilt fresh every frame, a pair that stops
// touching is simply not written again, so it silently falls out of here
std::unordered_map<EntityPairKey, RememberedPairState, EntityPairKeyHash> previousFrameContacts;

// Resolve as solid only collisions without any trigger in pair, which layers see each other
bool ShouldResolveAsSolid(const BoxColliderComponent& a, const BoxColliderComponent& b)
{
	if (a.isTrigger || b.isTrigger)
		return false;

	return LayersCanInteract(a.layer, a.collidesWith, b.layer, b.collidesWith);
}

// If no physics body or isKinematic = unmovable object (wall etc.)
bool CanBePushed(const PhysicsBodyComponent* physicsBody)
{
	return physicsBody != nullptr && !physicsBody->isKinematic;
}

// Inverse mass in physcics code, as it lets us represent "infinitely heavy, 
// can never be moved" as simple 0 value
float GetInverseMass(const PhysicsBodyComponent* physicsBody)
{
	if (!CanBePushed(physicsBody))
		return 0.0f;

	return 1.0f / physicsBody->mass;
}
// Same idea as GetInverseMass but for rotation: an immovable object never
// needs to know its own resistance to spinning, since it's never going to spin
glm::mat3 GetInverseInertiaTensorWorld(const PhysicsBodyComponent* physicsBody, const BoxColliderComponent& collider)
{
	if (!CanBePushed(physicsBody))
		return glm::mat3(0.0f);

	glm::vec3 inertiaLocal = ComputeBoxInertiaTensorLocal(collider.worldOBB.halfExtents, physicsBody->mass);

	// prevent divide by zero exception, just in case
	constexpr float SMALLEST_USABLE_INERTIA = 1e-8f;
	glm::vec3 inverseInertiaLocal = glm::vec3(
		inertiaLocal.x > SMALLEST_USABLE_INERTIA ? 1.0f / inertiaLocal.x : 0.0f,
		inertiaLocal.y > SMALLEST_USABLE_INERTIA ? 1.0f / inertiaLocal.y : 0.0f,
		inertiaLocal.z > SMALLEST_USABLE_INERTIA ? 1.0f / inertiaLocal.z : 0.0f
	);

	// Freeze Rotation: an axis the body can't rotate around is an axis with infinite resistance to spinning
	// (inverse inertia 0), so no impact can spin the body around it
	if (physicsBody->freezeRotationX) inverseInertiaLocal.x = 0.0f;
	if (physicsBody->freezeRotationY) inverseInertiaLocal.y = 0.0f;
	if (physicsBody->freezeRotationZ) inverseInertiaLocal.z = 0.0f;

	return ComputeInverseInertiaTensorWorld(inverseInertiaLocal, collider.worldOBB.orientation);
}

// A material property doesn't belong to one body alone
// It's a combination of both surfaces touching
//
// Restitution ("bounciness"): the LEAST bouncy of the two wins
// A rubber ball dropped on a bed of sand does not bounce much either,
// even though the ball itself is very bouncy
float CombineRestitution(const PhysicsBodyComponent* bodyA, const PhysicsBodyComponent* bodyB)
{
	if (bodyA && bodyB)
		return std::min(bodyA->restitution, bodyB->restitution);

	return bodyA ? bodyA->restitution : (bodyB ? bodyB->restitution : 0.0f);
}

// A material property doesn't belong to one body alone
// It's a combination of both surfaces touching
//
// Friction: combined with a square root so that "ice touching anything"
// stays close to frictionless, instead of averaging away the ice's slipperiness
float CombineFriction(const PhysicsBodyComponent* bodyA, const PhysicsBodyComponent* bodyB)
{
	if (bodyA && bodyB)
		return std::sqrt(bodyA->friction * bodyB->friction);

	return bodyA ? bodyA->friction : (bodyB ? bodyB->friction : 0.0f);
}

bool IsBodyAlmostStill(const PhysicsBodyComponent& body)
{
	// Speed below these is treated as body NOT MOVING
	constexpr float STILL_LINEAR_SPEED = 0.05f; // meters  / second
	constexpr float STILL_ANGULAR_SPEED = 0.10f; // radians / second

	return glm::length(body.velocity) < STILL_LINEAR_SPEED
		&& glm::length(body.angularVelocity) < STILL_ANGULAR_SPEED;
}

// Returns whichever world axis (X, Y or Z) points the LEAST like the given
// direction. Used to pick a safe helper axis: cross product of two directions that
// point the same way gives a zero vector, which would break everything
glm::vec3 GetWorldAxisPointingLeastLike(const glm::vec3& direction)
{
	float howMuchItPointsAlongX = std::abs(direction.x);
	float howMuchItPointsAlongY = std::abs(direction.y);
	float howMuchItPointsAlongZ = std::abs(direction.z);

	bool xIsTheLeastSimilar = howMuchItPointsAlongX <= howMuchItPointsAlongY
		&& howMuchItPointsAlongX <= howMuchItPointsAlongZ;

	if (xIsTheLeastSimilar)
		return glm::vec3(1.0f, 0.0f, 0.0f);

	bool yIsTheLeastSimilar = howMuchItPointsAlongY <= howMuchItPointsAlongZ;

	if (yIsTheLeastSimilar)
		return glm::vec3(0.0f, 1.0f, 0.0f);

	return glm::vec3(0.0f, 0.0f, 1.0f);
}

SurfaceSlideDirections BuildSlideDirections(const glm::vec3& surfaceNormal)
{
	SurfaceSlideDirections directions;

	glm::vec3 helperAxis = GetWorldAxisPointingLeastLike(surfaceNormal);

	directions.firstSlideDirection = glm::normalize(glm::cross(surfaceNormal, helperAxis));
	directions.secondSlideDirection = glm::normalize(glm::cross(surfaceNormal, directions.firstSlideDirection));

	return directions;
}

// How much 1 UNIT of push along direction would change the separating velocity
// Sum of 2 "how easily this changes" terms:
// 
//  inverseMassA + inverseMassB -> bigger for LIGHT objects (a small push
//     speeds up something light a lot)
// 
//   dot(angularResponseA + angularResponseB, direction) -> bigger for objects
//     that are EASY to spin (same idea, for spinning instead of sliding)
//
// So a BIGGER denominator = "this pair's velocity changes a lot per unit of impulse" 
// (light, easy to spin) -> only a SMALL impulse is needed
// 
// A heavy, hard-to-spin pair gives a SMALL denominator -> needs a BIGGER
// impulse for the same speed change
// 
// Same as real life: it takes more force to change a heavy object's speed than a light one
float GetImpulseResponse(const glm::vec3& direction,
	const glm::vec3& leverArmA, float inverseMassA, const glm::mat3& inverseInertiaA,
	const glm::vec3& leverArmB, float inverseMassB, const glm::mat3& inverseInertiaB)
{
	glm::vec3 angularResponseA = glm::cross(inverseInertiaA * glm::cross(leverArmA, direction), leverArmA);
	glm::vec3 angularResponseB = glm::cross(inverseInertiaB * glm::cross(leverArmB, direction), leverArmB);

	float impulseDenominator = inverseMassA + inverseMassB + glm::dot(angularResponseA + angularResponseB, direction);

	return impulseDenominator;
}

glm::vec3 GetVelocityOfBodyAtPoint(const PhysicsBodyComponent* body, const glm::vec3& leverArm)
{
	if (!body)
		return glm::vec3(0.0f);

	// Linear velocity from rotation (v = w x r). (E.g. a spinning wheel's edge moves faster than its center)
	// Cross product calculates the correct direction (sideways along the circle "direction tangent to the circle")
	glm::vec3 extraSpeedFromSpinning = glm::cross(body->angularVelocity, leverArm);

	return body->velocity + extraSpeedFromSpinning;
}

// How fast the two touching spots are moving relative to each other
// We only ever care about the DIFFERENCE, never the absolute speeds:
// two boxes flying side by side at 100 km/h are not colliding at all
glm::vec3 GetSpeedDifferenceAtPoint(const CollisionPair& pair, const CollisionPoint& point)
{
	glm::vec3 speedOfA = GetVelocityOfBodyAtPoint(pair.bodyA, point.leverArmFromCenterA);
	glm::vec3 speedOfB = GetVelocityOfBodyAtPoint(pair.bodyB, point.leverArmFromCenterB);

	return speedOfB - speedOfA;
}

// An "impulse" is an INSTANT push: it changes speed right now, instead of
// pushing over time the way a force does. Collisions happen too fast to be
// worth simulating as forces, so everything here works in impulses
void PushBothBodiesApart(const CollisionPair& pair, const CollisionPoint& point, const glm::vec3& impulse)
{
	if (pair.bodyA && pair.inverseMassOfA > 0.0f)
	{
		pair.bodyA->velocity -= impulse * pair.inverseMassOfA;

		// Angular velocity apply explanation for both physicsBodyA && physicsBodyB:
		//
		// Turns a straight-line push (impulse) applied at an offset (leverArm) into a SPIN
		// 
		// Same idea as pushing a door: 
		// Push near the hinge (leverArm short, or push points straight at the hinge) 
		// and it barely turns
		//
		// Push far from the hinge, at an angle, and it swings hard 
		// 
		// cross() naturally captures this: 
		// it's biggest when impulse is perpendicular to leverArm (pure "sideways"
		// push, like a door handle), and drops to ZERO when impulse points straight
		// along leverArm (pushing straight through the center never causes a spin)
		pair.bodyA->angularVelocity -= pair.inverseInertiaOfA * glm::cross(point.leverArmFromCenterA, impulse);
	}

	if (pair.bodyB && pair.inverseMassOfB > 0.0f)
	{
		pair.bodyB->velocity += impulse * pair.inverseMassOfB;
		pair.bodyB->angularVelocity += pair.inverseInertiaOfB * glm::cross(point.leverArmFromCenterB, impulse);
	}
}

// Turns "how easily does this pair react" into "how much push do I need",
// by flipping it upside down once instead of dividing inside every loop pass
float GetSpeedGainPerUnitOfPush(const CollisionPair& pair, const CollisionPoint& point, const glm::vec3& pushDirection)
{
	// Calculate how easily the objects react to a push at this specific contact point 
	// (If you push a light cardboard box,  it flies away instantly)
	// (If you push a heavy concrete block, it barely moves)
	// This takes into account both their weight (mass) and where you hit them (leverage/rotation)
	float howEasilyItReacts = GetImpulseResponse(pushDirection,
		point.leverArmFromCenterA, pair.inverseMassOfA, pair.inverseInertiaOfA,
		point.leverArmFromCenterB, pair.inverseMassOfB, pair.inverseInertiaOfB);

	constexpr float MIN_REACTION_THRESHOLD = 1e-8f;

	if (howEasilyItReacts < MIN_REACTION_THRESHOLD)
		return 0.0f;

	// Flip it upside down (1 / reaction) so the solver can use fast (for CPU) multiplication 
	// instead of doing slow (for CPU) divisions in every single loop pass
	return 1.0f / howEasilyItReacts;
}

float CalculateTargetBounceSpeed(const CollisionPair& pair, const CollisionPoint& point)
{
	glm::vec3 speedDifference = GetSpeedDifferenceAtPoint(pair, point);

	// Negative means the two spots are still moving INTO each other
	float approachSpeed = glm::dot(speedDifference, pair.pushDirectionFromAToB);

	// Below this impact speed nothing bounces. Without this rule a box resting
	// on the floor keeps making tiny hops forever, because gravity gives it a
	// small downward speed on every single step, and every one of those would
	// be treated as a fresh little impact.
	constexpr float SLOWEST_IMPACT_THAT_STILL_BOUNCES = 1.0f; // meters / second

	bool hitHardEnoughToBounce = approachSpeed < -SLOWEST_IMPACT_THAT_STILL_BOUNCES;

	if (!hitHardEnoughToBounce)
		return 0.0f;

	// Bounciness 0.2 means "leave with 20% of the speed you arrived with"
	// The minus flips "coming in" into "going out"
	return -pair.bounciness * approachSpeed;
}

void PrepareCollisionPoint(const CollisionPair& pair, CollisionPoint& point)
{
	point.leverArmFromCenterA = point.worldPosition - pair.centerOfA;
	point.leverArmFromCenterB = point.worldPosition - pair.centerOfB;

	point.speedGainPerUnitOfNormalPush = GetSpeedGainPerUnitOfPush(pair, point, pair.pushDirectionFromAToB);
	point.speedGainPerUnitOfFirstSlidePush = GetSpeedGainPerUnitOfPush(pair, point, pair.slideDirections.firstSlideDirection);
	point.speedGainPerUnitOfSecondSlidePush = GetSpeedGainPerUnitOfPush(pair, point, pair.slideDirections.secondSlideDirection);

	point.targetBounceSpeed = CalculateTargetBounceSpeed(pair, point);
}

// =====================================================================
// OVERLAP FIXING (Direct Position Correction)
// =====================================================================
// THE PROBLEM WITH VELOCITY:
// Pushing objects apart via velocity makes them bounce and jitter forever, because 
// the solver mistakes the "push" for real speed that leaks into the next frame
//
// THE SOLUTION:
// We modify TransformComponent::position directly and leave velocity completely alone
// Nudges are queued in 'pendingPositionCorrections' during solver passes and applied 
// all at once at the end of the step

// Total position displacement per entity for this step, accumulated across all passes
// Applied to Transforms at the end of the step and then cleared
std::unordered_map<unsigned int, glm::vec3> pendingPositionCorrections;

void AddPositionCorrection(unsigned int entityID, const glm::vec3& nudge)
{
	pendingPositionCorrections[entityID] += nudge;
}

glm::vec3 GetQueuedCorrectionSoFar(unsigned int entityID)
{
	auto found = pendingPositionCorrections.find(entityID);
	return (found != pendingPositionCorrections.end()) ? found->second : glm::vec3(0.0f);
}

// Calculates how much these two objects are STILL overlapping right now
//
// WHAT IT DOES:
// Takes the starting overlap and subtracts any pushes we ALREADY 
// applied to these objects in this step.
//
// WHY WE NEED IT:
// In a stack (tower) of boxes, pushing Box 1 also moves Box 2
// If Box 3 doesn't know Box 2 moved, it uses OLD positions to fix its collision
// This makes towers wobble and shake
float GetRemainingOverlap(const CollisionPair& pair, const CollisionPoint& point)
{
	glm::vec3 correctionOfA = GetQueuedCorrectionSoFar(pair.entityA);
	glm::vec3 correctionOfB = GetQueuedCorrectionSoFar(pair.entityB);

	float alreadyFixedAlongPushAxis = glm::dot(correctionOfB - correctionOfA, pair.pushDirectionFromAToB);

	return std::max(point.penetrationDepth - alreadyFixedAlongPushAxis, 0.0f);
}

// How many meters (combined, for both bodies) this one contact point wants
// to push apart in THIS pass (NOT a velocity)
float CalculatePositionCorrectionAmount(float effectivePenetration)
{
	// We let bodies sink into each other by this much and simply ignore it.
	// Chasing the last fraction of a millimeter is exactly what makes
	// resting boxes vibrate instead of sitting still
	constexpr float ACCEPTABLE_OVERLAP = 0.005f;

	// What share of the leftover overlap we undo per pass
	// 1.0 would fix it instantly, but that makes objects visibly pop
	// 0.2 spreads the fix over several passes/steps and looks calm
	constexpr float OVERLAP_FIX_PER_PASS = 0.2f;

	float overlapWorthFixing = std::max(effectivePenetration - ACCEPTABLE_OVERLAP, 0.0f);

	return OVERLAP_FIX_PER_PASS * overlapWorthFixing;
}

bool BothSidesAreSleepingOrStatic(const CollisionPair& pair)
{
	bool aIsSettled = !pair.bodyA || pair.bodyA->isSleeping;
	bool bIsSettled = !pair.bodyB || pair.bodyB->isSleeping;

	return aIsSettled && bIsSettled;
}

// Calculates position correction for a single contact point during the CURRENT solver pass
// Running this iteratively across multiple passes (see POSITION_SOLVER_PASSES) allows stacked
// objects (tower from cubes etc.) to find their stable position in a single frame
// rather than propagating corrections over multiple frames
void QueuePositionCorrectionsForPair(const CollisionPair& pair, const CollisionPoint& point)
{
	if (BothSidesAreSleepingOrStatic(pair))
		return;

	float remainingOverlap = GetRemainingOverlap(pair, point);
	float correctionMeters = CalculatePositionCorrectionAmount(remainingOverlap);
	if (correctionMeters <= 0.0f)
		return; // no overlap worth fixing here

	float totalInverseMass = pair.inverseMassOfA + pair.inverseMassOfB;
	if (totalInverseMass <= 0.0f)
		return; // both sides immovable

	// Split the fix between the two bodies according to how heavy they are
	float shareForA = pair.inverseMassOfA / totalInverseMass;
	float shareForB = pair.inverseMassOfB / totalInverseMass;

	// Stops one huge overlap (e.g. something teleported/spawned
	// inside a wall) from popping out an object instantly across the level
	// in a single pass
	constexpr float MAX_CORRECTION_PER_PASS = 0.2f; // meters
	correctionMeters = std::min(correctionMeters, MAX_CORRECTION_PER_PASS);

	glm::vec3 positionImpulse = pair.pushDirectionFromAToB * correctionMeters;

	if (pair.inverseMassOfA > 0.0f)
	{
		glm::vec3 pushOnA = -positionImpulse * shareForA;
		AddPositionCorrection(pair.entityA, pushOnA);
	}

	if (pair.inverseMassOfB > 0.0f)
	{
		glm::vec3 pushOnB = positionImpulse * shareForB;
		AddPositionCorrection(pair.entityB, pushOnB);
	}
}

// Applies every queued nudge straight onto the real transform and clears
// the queue for next step. This is the only place overlap-fixing ever
// touches the entity (never through PhysicsBodyComponent::velocity)
void ApplyQueuedPositionCorrections(EntityManager& entityManager)
{
	for (auto& [entityID, nudge] : pendingPositionCorrections)
	{
		auto* transform = entityManager.GetComponent<TransformComponent>(entityID);
		if (!transform)
			continue;

		transform->position += nudge;
		transform->isDirty = true;
	}

	pendingPositionCorrections.clear();
}

void SolvePushApartDirection(const CollisionPair& pair, CollisionPoint& point)
{
	// STEP 1: where are we, and where do we want to be?
	glm::vec3 speedDifference = GetSpeedDifferenceAtPoint(pair, point);
	float currentSeparatingSpeed = glm::dot(speedDifference, pair.pushDirectionFromAToB);

	// Target is JUST the bounce speed (0 whenever restitution is 0)
	// Overlap is fixed entirely separately (QueuePositionCorrectionForPoint) 
	// So it never gets baked into
	// real velocity and can't leak into next frame
	float wantedSeparatingSpeed = point.targetBounceSpeed;

	// STEP 2: how much push covers that gap?
	float missingSpeed = wantedSeparatingSpeed - currentSeparatingSpeed;
	float pushToAddNow = missingSpeed * point.speedGainPerUnitOfNormalPush;

	// STEP 3: clamp the TOTAL, not this one pass
	//
	// A surface can only PUSH bodies apart  
	// A floor never sucks a box downwards
	// so the running total must stay at 0 or above
	//
	// But this single pass is allowed to come out NEGATIVE, as long as the
	// total stays positive. That is what lets a later corner take back push
	// that an earlier corner overdid. Without it, whichever corner happens
	// to be solved first always wins, and a box dropped perfectly flat
	// still flips over
	float pushBefore = point.totalNormalPush;
	point.totalNormalPush = std::max(pushBefore + pushToAddNow, 0.0f);

	float pushActuallyAdded = point.totalNormalPush - pushBefore;

	PushBothBodiesApart(pair, point, pair.pushDirectionFromAToB * pushActuallyAdded);
}

void SolveOneSlideDirection(const CollisionPair& pair, CollisionPoint& point,
	const glm::vec3& slideDirection, float speedGainPerUnitOfPush, float& totalSlidePush)
{
	glm::vec3 speedDifference = GetSpeedDifferenceAtPoint(pair, point);
	float currentSlideSpeed = glm::dot(speedDifference, slideDirection);

	// Friction always wants to stop the sliding completely
	// So unlike the push-apart direction, the target here is plain 0
	float pushToAddNow = -currentSlideSpeed * speedGainPerUnitOfPush;

	// Friction can never be stronger than how hard the surfaces are pressed together
	// Rest a book on a table and it slides easily
	// Press down hard and it will not move
	// "How hard pressed" is exactly totalNormalPush
	float strongestFrictionAllowed = pair.friction * point.totalNormalPush;

	// Sliding can go either way along this direction, so the limit is symmetric
	float pushBefore = totalSlidePush;
	totalSlidePush = glm::clamp(pushBefore + pushToAddNow, -strongestFrictionAllowed, strongestFrictionAllowed);

	float pushActuallyAdded = totalSlidePush - pushBefore;

	PushBothBodiesApart(pair, point, slideDirection * pushActuallyAdded);
}

void SolveFriction(const CollisionPair& pair, CollisionPoint& point)
{
	SolveOneSlideDirection(pair, point,
		pair.slideDirections.firstSlideDirection,
		point.speedGainPerUnitOfFirstSlidePush,
		point.totalFirstSlidePush);

	SolveOneSlideDirection(pair, point,
		pair.slideDirections.secondSlideDirection,
		point.speedGainPerUnitOfSecondSlidePush,
		point.totalSecondSlidePush);
}

CollisionPair BuildCollisionPair(
	unsigned int entityA, PhysicsBodyComponent* bodyA, const BoxColliderComponent& colliderA, float inverseMassA,
	unsigned int entityB, PhysicsBodyComponent* bodyB, const BoxColliderComponent& colliderB, float inverseMassB,
	const ContactManifold& manifold)
{
	CollisionPair pair;

	pair.entityA = entityA;
	pair.entityB = entityB;

	pair.bodyA = bodyA;
	pair.bodyB = bodyB;

	pair.centerOfA = colliderA.worldOBB.center;
	pair.centerOfB = colliderB.worldOBB.center;

	pair.inverseMassOfA = inverseMassA;
	pair.inverseMassOfB = inverseMassB;

	pair.inverseInertiaOfA = GetInverseInertiaTensorWorld(bodyA, colliderA);
	pair.inverseInertiaOfB = GetInverseInertiaTensorWorld(bodyB, colliderB);

	pair.bounciness = CombineRestitution(bodyA, bodyB);
	pair.friction = CombineFriction(bodyA, bodyB);

	pair.pushDirectionFromAToB = manifold.pushDirectionFromAToB;
	pair.slideDirections = BuildSlideDirections(pair.pushDirectionFromAToB);
	pair.usedAxisIndex = manifold.usedAxisIndex;

	for (const ContactPoint& geometricContact : manifold.contacts)
	{
		CollisionPoint point;

		point.worldPosition    = geometricContact.worldPosition;
		point.penetrationDepth = geometricContact.penetrationDepth;
		point.featureID        = geometricContact.featureID;

		pair.points.push_back(point);
	}

	return pair;
}

// Checks which of the 15 candidate SAT axes was used for this pair 
// in the previous frame so we can ask the SAT to stick with it
// Returns -1 if this pair is completely new (they have never touched 
// or stopped touching), meaning "no preference, choose whatever is actually
// best in this frame"
int GetPreviousUsedAxisIndex(unsigned int entityA, unsigned int entityB)
{
	auto cachedPairIt = previousFrameContacts.find({ entityA, entityB });
	if (cachedPairIt == previousFrameContacts.end())
		return -1;

	return cachedPairIt->second.usedAxisIndex;
}

// Breadth First Search (BFS)
// Learned it from:
// https://www.youtube.com/watch?v=xlVX7dXLS64
// Very nice
// 
// This allow us to create stack/tower of cubes (CollisionPairs) one on another more stable
// cause we know which depth each cube (CollisionPair) is
std::unordered_map<unsigned int, int> ComputeStackSupportDepth(const std::vector<CollisionPair>& pairs)
{
	std::unordered_map<unsigned int, int> entityToDepth;
	std::queue<unsigned int> toVisit;

	// START: every entity touching with something inmovable (inverseMass <= 0) has depth 0
	for (const CollisionPair& pair : pairs)
	{
		// A is inmovable in this case (we can move B)
		if (pair.inverseMassOfA <= 0.0f && entityToDepth.find(pair.entityB) == entityToDepth.end())
		{
			entityToDepth[pair.entityB] = 0; // depth 0
			toVisit.push(pair.entityB);
		}

		// B is inmovable in this case (we can move A)
		if (pair.inverseMassOfB <= 0.0f && entityToDepth.find(pair.entityA) == entityToDepth.end())
		{
			entityToDepth[pair.entityA] = 0; // depth 0
			toVisit.push(pair.entityA);
		}
	}

	while (!toVisit.empty())
	{
		unsigned int currentEntity = toVisit.front();
		toVisit.pop();
		int currentDepth = entityToDepth[currentEntity];

		for (const CollisionPair& pair : pairs)
		{
			unsigned int otherEntity = (pair.entityA == currentEntity) ? pair.entityB
				: (pair.entityB == currentEntity) ? pair.entityA
				: std::numeric_limits<unsigned int>::max(); // max (No currentEntity found)

			if(otherEntity == std::numeric_limits<unsigned int>::max())
				continue;

			if (entityToDepth.find(otherEntity) == entityToDepth.end())
			{
				entityToDepth[otherEntity] = currentDepth + 1;
				toVisit.push(otherEntity);
			}
		}
	}

	return entityToDepth;
}

std::vector<CollisionPair> CollectCollisionPairs(EntityManager& entityManager)
{
	std::vector<CollisionPair> pairs;

	auto& colliders = entityManager.GetAllComponents<BoxColliderComponent>();

	for (unsigned int i = 0; i < colliders.size(); i++)
	{
		for (unsigned int j = i + 1; j < colliders.size(); j++)
		{
			if(!ShouldResolveAsSolid(colliders[i], colliders[j]))
				continue;

			unsigned int entityA = entityManager.GetEntityIDForComponent(colliders[i]);
			unsigned int entityB = entityManager.GetEntityIDForComponent(colliders[j]);

			int preferredAxisIndex = GetPreviousUsedAxisIndex(entityA, entityB);

			ContactManifold manifold = GetContactManifoldBoxVsBox(colliders[i].worldOBB,
				colliders[j].worldOBB, preferredAxisIndex);

			if (!manifold.areOverlapping || manifold.contacts.empty())
				continue;

			auto* bodyA = entityManager.GetComponent<PhysicsBodyComponent>(entityA);
			auto* bodyB = entityManager.GetComponent<PhysicsBodyComponent>(entityB);

			float inverseMassA = GetInverseMass(bodyA);
			float inverseMassB = GetInverseMass(bodyB);

			bool bothAreImmovable = inverseMassA <= 0.0f && inverseMassB <= 0.0f;
			if (bothAreImmovable)
				continue;

			pairs.push_back(BuildCollisionPair(
				entityA, bodyA, colliders[i], inverseMassA,
				entityB, bodyB, colliders[j], inverseMassB,
				manifold));
		}
	}

	// For tower/stack of cubes (Collisions)
	// Solver processes pairs sequentially. If a contact HIGHER in a tower/stack 
	// is resolved before the contact BELOW has provided support,
	// the top box briefly "floats" in mid-air and receives an impulse 
	// calculated as if it were freely falling
	// Sorting from bottom to top ensures every contact is resolved with a solid foundation 
	// already underneath
	std::unordered_map<unsigned int, int> supportDepth = ComputeStackSupportDepth(pairs);

	auto depthOf = [&](unsigned int entityID)
		{
			auto found = supportDepth.find(entityID);
			return found != supportDepth.end() ? found->second : 0;
		};

	std::sort(pairs.begin(), pairs.end(), [&](const CollisionPair& a, const CollisionPair& b)
		{
			int minDepthA = std::min(depthOf(a.entityA), depthOf(a.entityB));
			int minDepthB = std::min(depthOf(b.entityA), depthOf(b.entityB));

			return minDepthA < minDepthB;
		});

	return pairs;
}

// Looks through last frame's remembered points for this SAME pair and finds
// the one that is the exact same real corner (matched by featureID, not by
// position). Returns nullptr if this corner is brand new this frame
const RememberedContactImpulse* FindMatchingRememberedImpulse(
	const std::vector<RememberedContactImpulse>& rememberedPoints,
	const ContactFeatureID& featureID)
{
	for (const RememberedContactImpulse& remembered : rememberedPoints)
	{
		if (remembered.featureID == featureID)
			return &remembered;
	}

	return nullptr;
}

// For every touching point in this pair, checks whether it existed last
// frame too, and if so, starts it from last frame's push amounts instead
// of zero. This is the "remembering" half of warm starting
void LoadRememberedImpulses(CollisionPair& pair)
{
	auto cachedPairIt = previousFrameContacts.find({ pair.entityA, pair.entityB });
	if (cachedPairIt == previousFrameContacts.end())
		return; // this pair did not exist last frame (new collision, everything starts at zero, as normal)

	const std::vector<RememberedContactImpulse>& rememberedPoints = cachedPairIt->second.contactImpulses;

	for (CollisionPoint& point : pair.points)
	{
		const RememberedContactImpulse* match = FindMatchingRememberedImpulse(rememberedPoints, point.featureID);
		if(!match)
			continue; // new corner this frame (starts at zero)

		point.totalNormalPush      = match->normalPush;
		point.totalFirstSlidePush  = match->firstSlidePush;
		point.totalSecondSlidePush = match->secondSlidePush;
	}
}

// Turns one point's three remembered numbers (push apart + 2 slide directions) 
// into one real push, and applies it to both bodies right now
void ApplyRememberedImpulse(const CollisionPair& pair, const CollisionPoint& point)
{
	glm::vec3 combinedPush =
		  pair.pushDirectionFromAToB                * point.totalNormalPush
		+ pair.slideDirections.firstSlideDirection  * point.totalFirstSlidePush
		+ pair.slideDirections.secondSlideDirection * point.totalSecondSlidePush;

	PushBothBodiesApart(pair, point, combinedPush);
}

// Saves this frame's final push amounts so NEXT frame can start from them
// Replacing the whole cache (rather than editing it) means a pair that
// stopped touching is automatically forgotten (nothing to clean up by hand)
void SaveImpulsesForNextFrame(const std::vector<CollisionPair>& pairs)
{
	std::unordered_map<EntityPairKey, RememberedPairState, EntityPairKeyHash> newCache;

	for (const CollisionPair& pair : pairs)
	{
		RememberedPairState state;
		state.usedAxisIndex = pair.usedAxisIndex;
		state.contactImpulses.reserve(pair.points.size());

		for (const CollisionPoint& point : pair.points)
		{
			state.contactImpulses.push_back({
				point.featureID,
				point.totalNormalPush,
				point.totalFirstSlidePush,
				point.totalSecondSlidePush
				});
		}

		newCache[{ pair.entityA, pair.entityB }] = std::move(state);
	}

	previousFrameContacts = std::move(newCache);
}

void SolveCollisionPairs(std::vector<CollisionPair>& pairs)
{
	// For every point, work out how bouncy it should be, how easily it
	// reacts to a push, etc. based on THIS frame's real impact speed
	for (CollisionPair& pair : pairs)
		for (CollisionPoint& point : pair.points)
			PrepareCollisionPoint(pair, point);

	// WARM START: recall last frame's push for each point, and apply it now 
	// So the solver below starts close to the right answer
	// instead of from a blank slate
	for (CollisionPair& pair : pairs)
	{
		LoadRememberedImpulses(pair);

		for (CollisionPoint& point : pair.points)
			ApplyRememberedImpulse(pair, point);
	}

	// Fixing one touching point always slightly disturbs the others 
	// Push one corner of a box down and the opposite corner lifts a little
	// So we go over every point again and again
	// Each pass the leftover error gets smaller, until everything settles
	// More passes = steadier stacks of boxes, but more CPU time
	// 10 is a solid default
	constexpr int SOLVER_PASSES = 10;

	// Refine that warm-starting guess over several passes, until it settles
	for (unsigned int pass = 0; pass < SOLVER_PASSES; pass++)
	{
		for (CollisionPair& pair : pairs)
		{
			for (CollisionPoint& point : pair.points)
			{
				SolvePushApartDirection(pair, point);
				SolveFriction(pair, point);
			}
		}
	}

	// Remember this frame's result, so NEXT frame can warm-start from it
	SaveImpulsesForNextFrame(pairs);

	// Fixes positions AFTER velocities are done, so we don't affect movement
	// Runs multiple passes so stacked objects (like boxes) can pass position 
	// fixes up to each other and stop shaking in one frame
	//
	// Fewer passes = faster, but towers wobble longer
	// More  passes = towers stay still immediately, but uses more CPU
	constexpr int POSITION_SOLVER_PASSES = 4;

	for (int pass = 0; pass < POSITION_SOLVER_PASSES; pass++)
		for (CollisionPair& pair : pairs)
			for (CollisionPoint& point : pair.points)
				QueuePositionCorrectionsForPair(pair, point);
}

// If a sleeping box gets bumped by something awake and moving, 
// it SHOULD NOT stay still/frozen
// So before solving, we check every pair and wake up both
// sides whenever at least one of them is awake and moving
void WakeTouchingPairIfNeeded(CollisionPair& pair)
{
	bool eitherSideIsAwakeAndMoving =
		(pair.bodyA && !pair.bodyA->isSleeping && !IsBodyAlmostStill(*pair.bodyA))
		|| (pair.bodyB && !pair.bodyB->isSleeping && !IsBodyAlmostStill(*pair.bodyB));

	if (!eitherSideIsAwakeAndMoving)
		return;

	if (pair.bodyA)
		PhysicsSystem::WakeUp(*pair.bodyA);
	if (pair.bodyB)
		PhysicsSystem::WakeUp(*pair.bodyB);
}

// Puts bodies to sleep if they stay almost still/frozen while touching something 
// for long enough. Requires contact with a pair so objects flying freely in 
// mid-air never freeze
void UpdateSleepStates(EntityManager& entityManager, const std::vector<CollisionPair>& pairs, float deltaTime)
{
	std::unordered_set<PhysicsBodyComponent*> bodiesTouchingSomething;

	for (const CollisionPair& pair : pairs)
	{
		if (pair.bodyA)
			bodiesTouchingSomething.insert(pair.bodyA);
		if (pair.bodyB)
			bodiesTouchingSomething.insert(pair.bodyB);
	}

	auto& physicsBodies = entityManager.GetAllComponents<PhysicsBodyComponent>();

	for (auto& body : physicsBodies)
	{
		// A kinematic body (e.g. player) is moved by something else
		// (input, script, etc.), "sleeping" has no meaning for it
		if (body.isKinematic)
			continue;

		bool isTouchingSomething = bodiesTouchingSomething.count(&body) > 0;

		if (!isTouchingSomething || !IsBodyAlmostStill(body))
		{
			body.timeSpentAlmostStill = 0.0f;
			continue;
		}

		body.timeSpentAlmostStill += deltaTime;

		constexpr float TIME_NEEDED_TO_FALL_ASLEEP = 0.5f;
		if (body.timeSpentAlmostStill >= TIME_NEEDED_TO_FALL_ASLEEP)
		{
			body.isSleeping = true;

			body.velocity        = glm::vec3(0.0f);
			body.angularVelocity = glm::vec3(0.0f);
		}

	}
}

}

void CollisionSystem::ResolveSolidCollisions(EntityManager& entityManager, float deltaTime)
{
	std::vector<CollisionPair> pairs = CollectCollisionPairs(entityManager);

	// Wakes up entire stacks (tower) of touching objects in a single frame
	// 
	// If Box A wakes up Box B, Box B needs to wake up Box C in the same step
	// We repeat this loop until no new objects wake up, ensuring the wake-up signal 
	// passes through whole towers immediately, regardless of pair order
	//
	// Without this, wake-up moves only 1 stack level per frame, and a sleeping 
	// body receives impulses before it actually wakes up

	bool somethingWokeUp = true;
	while (somethingWokeUp)
	{
		somethingWokeUp = false;

		for (CollisionPair& pair : pairs)
		{
			bool aWasAsleep = pair.bodyA && pair.bodyA->isSleeping;
			bool bWasAsleep = pair.bodyB && pair.bodyB->isSleeping;

			WakeTouchingPairIfNeeded(pair);

			if ((aWasAsleep && pair.bodyA && !pair.bodyA->isSleeping) ||
				(bWasAsleep && pair.bodyB && !pair.bodyB->isSleeping))
				somethingWokeUp = true;
		}
	}

	SolveCollisionPairs(pairs);

	// Apply the overlap fixes queued during the solve above, directly to
	// position. Must happen AFTER solving (so it uses this step's final
	// penetration depths) and BEFORE sleeping (so a box that just got
	// nudged doesn't get judged on its pre-nudge position)
	ApplyQueuedPositionCorrections(entityManager);

	UpdateSleepStates(entityManager, pairs, deltaTime);
}

namespace
{

bool ContainsPair(const std::vector<OverlapPair>& pairs, const OverlapPair& target)
{
	for (const auto& pair : pairs)
	{
		if (pair == target)
			return true;
	}

	return false;
}

void PublishTriggerEvent(EntityManager& entityManager, const OverlapPair& pair, OverlapEventType eventType)
{
	auto* colliderA        = entityManager.GetComponent<BoxColliderComponent>(pair.entityA);
	bool  isEntityATrigger = colliderA && colliderA->isTrigger;

	unsigned int triggerEntityID = isEntityATrigger ? pair.entityA : pair.entityB;
	unsigned int otherEntityID   = isEntityATrigger ? pair.entityB : pair.entityA;

	switch (eventType)
	{
	case OverlapEventType::Enter: EventBus::Publish(TriggerEnterEvent{ triggerEntityID, otherEntityID }); break;
	case OverlapEventType::Stay:  EventBus::Publish(TriggerStayEvent { triggerEntityID, otherEntityID }); break;
	case OverlapEventType::Exit:  EventBus::Publish(TriggerExitEvent { triggerEntityID, otherEntityID }); break;
	}
}

void PublishCollisionEvent(const OverlapPair& pair, OverlapEventType eventType)
{
	switch (eventType)
	{
	case OverlapEventType::Enter: EventBus::Publish(CollisionEnterEvent{ pair.entityA, pair.entityB }); break;
	case OverlapEventType::Stay:  EventBus::Publish(CollisionStayEvent { pair.entityA, pair.entityB }); break;
	case OverlapEventType::Exit:  EventBus::Publish(CollisionExitEvent { pair.entityA, pair.entityB }); break;
	}
}

}

void CollisionSystem::DetectTriggerEvents(EntityManager& entityManager)
{
	auto& colliders = entityManager.GetAllComponents<BoxColliderComponent>();

	std::vector<OverlapPair> currentTriggerOverlapPairs;
	std::vector<OverlapPair> currentSolidOverlapPairs;
	entitiesWithAnyOverlap.clear();

	for (unsigned int i = 0; i < colliders.size(); i++)
	{
		for (unsigned int j = i + 1; j < colliders.size(); j++)
		{
			bool layersCanInteract = LayersCanInteract(colliders[i].layer, colliders[i].collidesWith,
				colliders[j].layer, colliders[j].collidesWith);

			if (!layersCanInteract)
				continue;

			if (!Overlaps(colliders[i].worldOBB, colliders[j].worldOBB))
				continue;

			unsigned int entityI = entityManager.GetEntityIDForComponent(colliders[i]);
			unsigned int entityJ = entityManager.GetEntityIDForComponent(colliders[j]);

			entitiesWithAnyOverlap.insert(entityI);
			entitiesWithAnyOverlap.insert(entityJ);

			bool pairContainsTrigger = colliders[i].isTrigger || colliders[j].isTrigger;
			if (pairContainsTrigger)
				currentTriggerOverlapPairs.push_back({ entityI, entityJ });
			else
				currentSolidOverlapPairs.push_back({ entityI, entityJ });
		}
	}

	// Triggers: Enter / Stay / Exit
	for (const auto& pair : currentTriggerOverlapPairs)
	{
		bool wasOverlappingLastFrame = ContainsPair(previousTriggerOverlapPairs, pair);
		PublishTriggerEvent(entityManager, pair, wasOverlappingLastFrame ? OverlapEventType::Stay : OverlapEventType::Enter);;
	}

	for (const auto& pair : previousTriggerOverlapPairs)
	{
		bool stoppedOverlappingThisFrame = !ContainsPair(currentTriggerOverlapPairs, pair);
		if (stoppedOverlappingThisFrame)
			PublishTriggerEvent(entityManager, pair, OverlapEventType::Exit);
	}

	previousTriggerOverlapPairs = std::move(currentTriggerOverlapPairs);

	// Solids: Enter / Stay / Exit
	for (const auto& pair : currentSolidOverlapPairs)
	{
		bool wasOverlappingLastFrame = ContainsPair(previousSolidOverlapPairs, pair);
		PublishCollisionEvent(pair, wasOverlappingLastFrame ? OverlapEventType::Stay : OverlapEventType::Enter);;
	}

	for (const auto& pair : previousSolidOverlapPairs)
	{
		bool stoppedOverlappingThisFrame = !ContainsPair(currentSolidOverlapPairs, pair);
		if (stoppedOverlappingThisFrame)
			PublishCollisionEvent(pair, OverlapEventType::Exit);
	}

	previousSolidOverlapPairs = std::move(currentSolidOverlapPairs);
}

void CollisionSystem::Reset()
{
	previousTriggerOverlapPairs.clear();
	previousSolidOverlapPairs.clear();
	entitiesWithAnyOverlap.clear();
	previousFrameContacts.clear();
	pendingPositionCorrections.clear();
}

bool CollisionSystem::IsEntityOverlapping(unsigned int entityID)
{
	return entitiesWithAnyOverlap.count(entityID) > 0;
}

void CollisionSystem::ForgetEntity(unsigned int entityID)
{
	for (auto it = previousFrameContacts.begin(); it != previousFrameContacts.end();)
	{
		if (it->first.entityA == entityID || it->first.entityB == entityID)
			it = previousFrameContacts.erase(it);
		else
			++it; // ++it is better practice than it++ for iterators in c++
	}

	// In case this frame's solve is currently in progress, clear any unapplied
	// nudges as well. Otherwise the entity will still receive a "late" correction
	// calculated from the old geometry
	pendingPositionCorrections.erase(entityID);
}

}