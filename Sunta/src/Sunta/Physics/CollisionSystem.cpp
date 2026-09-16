#include "Core/SuntaPreCompiled.h"
#include "CollisionSystem.h"

#include "ECS/EntityManager.h"
#include "ECS/Component.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"

namespace Sunta
{

void CollisionSystem::Update(EntityManager& entityManager)
{
	UpdateColliders(entityManager);
	ResolveSolidCollisions(entityManager);
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

	glm::vec3 inertiaLocal = ComputeBoxInertiaTensorLocal(collider.halfExtents, physicsBody->mass);
	glm::vec3 inverseInertiaLocal = 1.0f / inertiaLocal;

	return ComputeInverseInertiaTensorWorld(inverseInertiaLocal, collider.worldOBB.orientation);
}

constexpr float EQUAL_PUSH_SHARE_BETWEEN_TWO_MOVABLE_BODIES = 0.5f;

void MoveEntityAndKeepColliderInSync(EntityManager& entityManager, unsigned int entityID,
	BoxColliderComponent& collider, const glm::vec3& movement)
{
	auto* transform = entityManager.GetComponent<TransformComponent>(entityID);
	if (!transform)
		return;

	transform->position += movement;
	transform->isDirty = true;

	collider.worldOBB.center += movement;
}

// Push 2 boxes aparat so they stop overlapping on collision, 
// NO VELOCITY OR ROTATION APPLIED HERE
void SeparateOverlappingBodies(EntityManager& entityManager,
	BoxColliderComponent& colliderA, unsigned int entityA, float inverseMassA,
	BoxColliderComponent& colliderB, unsigned int entityB, float inverseMassB,
	const ContactManifold& manifold
)
{
	float totalInverseMass = inverseMassA + inverseMassB;
	if (totalInverseMass <= 0.0f)
		return;

	// The lighter object (bigger inverse mass) gets pushed further
	// E.g. bowling ball barely moves when a ping-pong ball bounces off it


	// "For every action (force) in nature there is an equal and opposite reaction" ~Sir Isaac Newton~
	float shareA = inverseMassA / totalInverseMass;
	float shareB = inverseMassB / totalInverseMass;

	if (inverseMassA > 0.0f)
		MoveEntityAndKeepColliderInSync(entityManager, entityA, colliderA, -manifold.pushDirectionFromAToB * manifold.overlapDepth * shareA);

	if (inverseMassB > 0.0f)
		MoveEntityAndKeepColliderInSync(entityManager, entityB, colliderB,  manifold.pushDirectionFromAToB * manifold.overlapDepth * shareB);

}

// Velocity of the contact point ITSELF, not the whole object's center
// If something spins, points further from the center move faster than
// points near it (think of a ceiling fan blade tip vs its middle) 
// so even if the object's center isn't moving, a spinning point CAN BE MOVING
// cross(angularVelocity, leverArm) calculates that extra motion
// added by the spin
glm::vec3 GetVelocityAtPoint(const glm::vec3& velocity, const glm::vec3& angularVelocity, const glm::vec3& leverArm)
{
	return velocity + glm::cross(angularVelocity, leverArm);
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

void ApplyImpulseToBothBodies(
	PhysicsBodyComponent* physicsBodyA, float inverseMassA, const glm::mat3& inverseInertiaA, const glm::vec3& leverArmA,
	PhysicsBodyComponent* physicsBodyB, float inverseMassB, const glm::mat3& inverseInertiaB, const glm::vec3& leverArmB,
	const glm::vec3& impulse)
{
	if (physicsBodyA && inverseMassA > 0.0f)
	{
		physicsBodyA->velocity -= impulse * inverseMassA;

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
		physicsBodyA->angularVelocity -= inverseInertiaA * glm::cross(leverArmA, impulse);
	}

	if (physicsBodyB && inverseMassB > 0.0f)
	{
		physicsBodyB->velocity += impulse * inverseMassB;
		physicsBodyB->angularVelocity += inverseInertiaB * glm::cross(leverArmB, impulse);
	}
}

glm::vec3 GetSlidingDirection(const glm::vec3& relativeVelocity, const glm::vec3& normal)
{
	glm::vec3 approachingVelocity = normal * glm::dot(relativeVelocity, normal);
	glm::vec3 slidingVelocity = relativeVelocity - approachingVelocity;

	constexpr float MIN_SLIDING_SPEED = 1e-5f;
	float slidingSpeed = glm::length(slidingVelocity);

	if (slidingSpeed <= MIN_SLIDING_SPEED)
		return glm::vec3(0.0f);

	return slidingVelocity / slidingSpeed;
}

float GetFrictionImpulseMagnitude(
	const glm::vec3& relativeVelocity, const glm::vec3& slidingDirection,
	const glm::vec3& leverArmA, float inverseMassA, const glm::mat3& inverseInertiaA,
	const glm::vec3& leverArmB, float inverseMassB, const glm::mat3& inverseInertiaB,
	float bounceImpulseMagnitude, float friction
)
{
	float response = GetImpulseResponse(slidingDirection,
		leverArmA, inverseMassA, inverseInertiaA,
		leverArmB, inverseMassB, inverseInertiaB);

	if (response <= 0.0f)
		return 0.0f;

	float impulseToFullyStopSliding = -glm::dot(relativeVelocity, slidingDirection) / response;
	float maxAllowedImpulseByFriction = friction * bounceImpulseMagnitude;

	return glm::clamp(impulseToFullyStopSliding, -maxAllowedImpulseByFriction, maxAllowedImpulseByFriction);
}

float ApplyNormalImpulse(
	PhysicsBodyComponent* physicsBodyA, float inverseMassA, const glm::mat3& inverseInertiaA, const glm::vec3& leverArmA,
	PhysicsBodyComponent* physicsBodyB, float inverseMassB, const glm::mat3& inverseInertiaB, const glm::vec3& leverArmB,
	const glm::vec3& relativeVelocity, const glm::vec3& normal, float restitution)
{
	// How fast the two contact points are moving apart (along the normal)
	// Negative = still approaching (there's still collision to resolve)
	float separatingVelocity = glm::dot(relativeVelocity, normal);

	if (separatingVelocity > 0.0f)
		return 0.0f; // colliders are already separating, so nothing to do

	// Restitution: after the hit, separating speed should be restitution
	// times what it was before (0 = stops, 1 = bounces back as fast as it hit).
	float desiredSeparatingVelocity = -restitution * separatingVelocity;
	float deltaVelocity = desiredSeparatingVelocity - separatingVelocity;

	float normalResponse = GetImpulseResponse(normal,
		leverArmA, inverseMassA, inverseInertiaA,
		leverArmB, inverseMassB, inverseInertiaB);

	if (normalResponse <= 0.0f)
		return 0.0f;

	float normalImpulseMagnitude = deltaVelocity / normalResponse;
	glm::vec3 normalImpulse = normalImpulseMagnitude * normal;

	ApplyImpulseToBothBodies(
		physicsBodyA, inverseMassA, inverseInertiaA, leverArmA,
		physicsBodyB, inverseMassB, inverseInertiaB, leverArmB,
		normalImpulse);

	return normalImpulseMagnitude;
}

void ApplyFrictionImpulse(
	PhysicsBodyComponent* physicsBodyA, float inverseMassA, const glm::mat3& inverseInertiaA, const glm::vec3& leverArmA,
	PhysicsBodyComponent* physicsBodyB, float inverseMassB, const glm::mat3& inverseInertiaB, const glm::vec3& leverArmB,
	const glm::vec3& relativeVelocity, const glm::vec3& normal, float normalImpulseMagnitude, float friction)
{
	glm::vec3 slidingDirection = GetSlidingDirection(relativeVelocity, normal);
	if (slidingDirection == glm::vec3(0.0f)) // No sliding, so nothing for friction to do
		return;

	float frictionImpulseMagnitude = GetFrictionImpulseMagnitude(relativeVelocity, slidingDirection,
		leverArmA, inverseMassA, inverseInertiaA,
		leverArmB, inverseMassB, inverseInertiaB,
		normalImpulseMagnitude, friction);

	glm::vec3 frictionImpulse = frictionImpulseMagnitude * slidingDirection;

	ApplyImpulseToBothBodies(
		physicsBodyA, inverseMassA, inverseInertiaA, leverArmA,
		physicsBodyB, inverseMassB, inverseInertiaB, leverArmB,
		frictionImpulse);
}

void ApplyCollisionImpulse(
	PhysicsBodyComponent* physicsBodyA, const glm::vec3& centerA, 
	float inverseMassA,                 const glm::mat3& inverseInertiaA,

	PhysicsBodyComponent* physicsBodyB, const glm::vec3& centerB,
	float inverseMassB, const glm::mat3& inverseInertiaB,

	const glm::vec3& normal, const glm::vec3& contactPoint, float restitution, float friction)
{
	if (inverseMassA <= 0.0f && inverseMassB <= 0.0f)
		return;

	glm::vec3 velocityA = physicsBodyA ? physicsBodyA->velocity : glm::vec3(0.0f);
	glm::vec3 velocityB = physicsBodyB ? physicsBodyB->velocity : glm::vec3(0.0f);

	glm::vec3 angularVelocityA = physicsBodyA ? physicsBodyA->angularVelocity : glm::vec3(0.0f);
	glm::vec3 angularVelocityB = physicsBodyB ? physicsBodyB->angularVelocity : glm::vec3(0.0f);

	// "Lever Arm" - the offset from each object's center to the actual
	// contact point. The further the contact point is from the center, the
	// more torque (spin)
	glm::vec3 leverArmA = contactPoint - centerA;
	glm::vec3 leverArmB = contactPoint - centerB;

	glm::vec3 contactVelocityA = GetVelocityAtPoint(velocityA, angularVelocityA, leverArmA);
	glm::vec3 contactVelocityB = GetVelocityAtPoint(velocityB, angularVelocityB, leverArmB);

	// How fast the two contact points are moving apart (along the normal)
	// Negative = still approaching (there's still collision to resolve)
	glm::vec3 relativeVelocity = contactVelocityB - contactVelocityA;

	float bounceImpulseMagnitude = ApplyNormalImpulse(
		physicsBodyA, inverseMassA, inverseInertiaA, leverArmA,
		physicsBodyB, inverseMassB, inverseInertiaB, leverArmB,
		relativeVelocity, normal, restitution);
	
	if (bounceImpulseMagnitude <= 0.0f)
		return; //already separating or nothing to resolve (skip friction)

	ApplyFrictionImpulse(
		physicsBodyA, inverseMassA, inverseInertiaA, leverArmA,
		physicsBodyB, inverseMassB, inverseInertiaB, leverArmB,
		relativeVelocity, normal, bounceImpulseMagnitude, friction);
}


// Stops velocity from pushing object into a surface it's touching (e.g. floor)
// while keeping the part of velocity that slides along the surface
// Without this gravity would keep pushing the object into the floor every frame
// and PushEntitiesApart would keep pushing it back out, causing jittering
glm::vec3 StopVelocityGoingIntoSurface(const glm::vec3& velocity, const glm::vec3& surfaceNormal)
{
	// Dot product tells us how much velocity points toward the surface
	// Positive (+) = moving away from surface
	// Negative (-) = moving into the surface
	float velocityTowardsSurface = glm::dot(velocity, surfaceNormal);

	// postive dot product = moving away from surface (so don't do anything)
	if (velocityTowardsSurface >= 0.0f)
		return velocity;

	// Subtract only the "into surface" part of velocity , keep the rest untouched (slinding along surface)
	return velocity - surfaceNormal * velocityTowardsSurface;
}

// Applies StopVelocityGoingIntoSurface to whichever of the two bodies can actually move,
// using the manifold's push direction as each body's local "surface normal"
// (pushDirectionFromAToB points from A to B, so B's surface normal faces towards it while A's faces away)
void StopBodiesFromRegainingTheirOverlap(
	PhysicsBodyComponent* physicsBodyA, float inverseMassA,
	PhysicsBodyComponent* physicsBodyB, float inverseMassB,
	const glm::vec3& pushDirectionFromAToB)
{
	if (inverseMassA > 0.0f)
		physicsBodyA->velocity = StopVelocityGoingIntoSurface(physicsBodyA->velocity, -pushDirectionFromAToB);

	if (inverseMassB > 0.0f)
		physicsBodyB->velocity = StopVelocityGoingIntoSurface(physicsBodyB->velocity, pushDirectionFromAToB);
}

}

void SnapMinimalMotionToZero(EntityManager& entityManager)
{
	constexpr float SLEEP_LINEAR_SPEED_THRESHOLD = 0.15f;  // meters/second
	constexpr float SLEEP_ANGULAR_SPEED_THRESHOLD = 0.25f; // radians/second

	auto& physicsBodies = entityManager.GetAllComponents<PhysicsBodyComponent>();

	for (auto& physicsBody : physicsBodies)
	{
		if (glm::length(physicsBody.velocity) < SLEEP_LINEAR_SPEED_THRESHOLD)
			physicsBody.velocity = glm::vec3(0.0f);

		if (glm::length(physicsBody.angularVelocity) < SLEEP_ANGULAR_SPEED_THRESHOLD)
			physicsBody.angularVelocity = glm::vec3(0.0f);
	}
}

void CollisionSystem::ResolveSolidCollisions(EntityManager& entityManager)
{
	auto& colliders = entityManager.GetAllComponents<BoxColliderComponent>();

	for (unsigned int i = 0; i < colliders.size(); i++)
	{
		for (unsigned int j = i + 1; j < colliders.size(); j++)
		{
			if (!ShouldResolveAsSolid(colliders[i], colliders[j]))
				continue;

			ContactManifold manifold = GetContactManifoldBoxVsBox(colliders[i].worldOBB, colliders[j].worldOBB);

			if (!manifold.areOverlapping)
				continue;

			unsigned int entityI = entityManager.GetEntityIDForComponent(colliders[i]);
			unsigned int entityJ = entityManager.GetEntityIDForComponent(colliders[j]);

			auto* physicsBodyI = entityManager.GetComponent<PhysicsBodyComponent>(entityI);
			auto* physicsBodyJ = entityManager.GetComponent<PhysicsBodyComponent>(entityJ);

			float inverseMassI = GetInverseMass(physicsBodyI);
			float inverseMassJ = GetInverseMass(physicsBodyJ);

			if(inverseMassI <= 0.0f && inverseMassJ <= 0.0f)
				continue;

			SeparateOverlappingBodies(entityManager, 
				colliders[i], entityI, inverseMassI,
				colliders[j], entityJ, inverseMassJ, manifold);

			glm::mat3 inverseInertiaI = GetInverseInertiaTensorWorld(physicsBodyI, colliders[i]);
			glm::mat3 inverseInertiaJ = GetInverseInertiaTensorWorld(physicsBodyJ, colliders[j]);

			float restitution = physicsBodyI && physicsBodyJ
				? std::min(physicsBodyI->restitution, physicsBodyJ->restitution)
				: (physicsBodyI ? physicsBodyI->restitution : physicsBodyJ->restitution);

			float friction = physicsBodyI && physicsBodyJ
				? std::sqrt(physicsBodyI->friction * physicsBodyJ->friction)
				: (physicsBodyI ? physicsBodyI->friction : physicsBodyJ->friction);


			// Iterative impulse solver:
			// When objects touch in multiple contact points at once, fixing the force at one point
			// messes up the others. Doing this in a loop makes each of the contact point
			// adjustments smoother, step by step, until everything settles down nicely without shaking
			constexpr int SOLVER_ITERATIONS = 8;

			for (unsigned int iteration = 0; iteration < SOLVER_ITERATIONS; iteration++)
			{
				for (const ContactPoint& contact : manifold.contacts)
				{
					ApplyCollisionImpulse(
						physicsBodyI, colliders[i].worldOBB.center, inverseMassI, inverseInertiaI,
						physicsBodyJ, colliders[j].worldOBB.center, inverseMassJ, inverseInertiaJ,
						manifold.pushDirectionFromAToB, contact.worldPosition, restitution, friction);
				}
			}

			StopBodiesFromRegainingTheirOverlap(
				physicsBodyI, inverseMassI,
				physicsBodyJ, inverseMassJ,
				manifold.pushDirectionFromAToB);

		}
	}

	SnapMinimalMotionToZero(entityManager);
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

void PublishTriggerEvent(EntityManager& entityManager, const OverlapPair& pair, TriggerEventType eventType)
{
	auto* colliderA = entityManager.GetComponent<BoxColliderComponent>(pair.entityA);

	bool isEntityATrigger = colliderA && colliderA->isTrigger;

	unsigned int triggerEntityID = isEntityATrigger ? pair.entityA : pair.entityB;
	unsigned int otherEntityID   = isEntityATrigger ? pair.entityB : pair.entityA;

	if (eventType == TriggerEventType::Enter)
		EventBus::Publish(TriggerEnterEvent{ triggerEntityID, otherEntityID });
	else
		EventBus::Publish(TriggerExitEvent{ triggerEntityID, otherEntityID });

}

}

void CollisionSystem::DetectTriggerEvents(EntityManager& entityManager)
{
	auto& colliders = entityManager.GetAllComponents<BoxColliderComponent>();

	std::vector<OverlapPair> currentOverlaps;

	for (unsigned int i = 0; i < colliders.size(); i++)
	{
		for (unsigned int j = i + 1; j < colliders.size(); j++)
		{
			bool pairContainsTrigger = colliders[i].isTrigger || colliders[j].isTrigger;

			if(!pairContainsTrigger)
				continue;

			bool layersCanInteract = LayersCanInteract(colliders[i].layer, colliders[i].collidesWith,
											           colliders[j].layer, colliders[j].collidesWith);

			if(!layersCanInteract)
				continue;

			if (Overlaps(colliders[i].worldOBB, colliders[j].worldOBB))
			{
				unsigned int entityI = entityManager.GetEntityIDForComponent(colliders[i]);
				unsigned int entityJ = entityManager.GetEntityIDForComponent(colliders[j]);

				currentOverlaps.push_back({ entityI, entityJ });
			}
		}
	}

	// Overlapping NOW but NOT LAST FRAME => pair just started touching
	for (const auto& pair : currentOverlaps)
	{
		if (!ContainsPair(previousOverlaps, pair))
			PublishTriggerEvent(entityManager, pair, TriggerEventType::Enter);
	}

	// Overlapping LAST FRAME but NOT NOW => pair just stopped touching
	for (const auto& pair : previousOverlaps)
	{
		if (!ContainsPair(currentOverlaps, pair))
			PublishTriggerEvent(entityManager, pair, TriggerEventType::Exit);
	}

	previousOverlaps = std::move(currentOverlaps);
}

bool CollisionSystem::IsEntityOverlapping(unsigned int enityID)
{
	for (const auto& pair : previousOverlaps)
	{
		if (pair.entityA == enityID || pair.entityB == enityID)
			return true;
	}

	return false;
}

}