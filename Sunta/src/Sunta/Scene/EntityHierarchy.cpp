#include "Core/SuntaPreCompiled.h"
#include "EntityHierarchy.h"

#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Renderer/Model.h"
#include "Utilities/MathUtilities.h"

namespace Sunta
{

namespace
{

// How far UpdateWorldMatrices got with each entity
enum class WorldMatrixState : unsigned char
{
	NotDone,
	InProgress, // only reachable again through a parent loop, which CanSetParent never lets happen
	Done
};

// The bone in the space of the model: from the pose the Animator plays, or the rest pose when there is no Animator
bool TryGetBoneModelTransform(EntityManager& entityManager, unsigned int entityID, const std::string& boneName, glm::mat4& outTransform)
{
	const ModelComponent* modelComponent = entityManager.GetComponent<ModelComponent>(entityID);
	if (!modelComponent || !modelComponent->modelData || !modelComponent->modelData->model)
		return false;

	const Model& model = *modelComponent->modelData->model;

	unsigned int nodeIndex = 0;
	if (!model.TryGetNodeIndex(boneName, nodeIndex))
		return false;

	const AnimatorComponent* animatorComponent = entityManager.GetComponent<AnimatorComponent>(entityID);

	if (animatorComponent && animatorComponent->animator.TryGetNodeModelTransform(nodeIndex, outTransform))
		return true;

	outTransform = model.GetRestNodeModelTransform(nodeIndex);
	return true;
}

// Where the entity is in the world, worked out from the Transforms (the world matrix may still be from the last frame)
glm::mat4 GetCurrentWorldMatrix(EntityManager& entityManager, unsigned int entityID)
{
	const TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID);
	const glm::mat4 localMatrix = transform ? transform->GetLocalMatrix() : glm::mat4(1.0f);

	return EntityHierarchy::GetParentSpace(entityManager, entityID) * localMatrix;
}

void PlaceInWorld(EntityManager& entityManager, unsigned int entityID, const glm::mat4& worldMatrix)
{
	if (TransformComponent* transform = entityManager.GetComponent<TransformComponent>(entityID))
		transform->SetFromLocalMatrix(glm::inverse(EntityHierarchy::GetParentSpace(entityManager, entityID)) * worldMatrix);
}

void UpdateWorldMatrix(EntityManager& entityManager, unsigned int entityID, std::vector<WorldMatrixState>& states)
{
	if (states[entityID] != WorldMatrixState::NotDone)
		return;

	states[entityID] = WorldMatrixState::InProgress;

	unsigned int parentID = 0;
	if (EntityHierarchy::TryGetParent(entityManager, entityID, parentID))
		UpdateWorldMatrix(entityManager, parentID, states);

	TransformComponent*   transform   = entityManager.GetComponent<TransformComponent>(entityID);
	WorldMatrixComponent* worldMatrix = entityManager.GetComponent<WorldMatrixComponent>(entityID);

	if (transform && worldMatrix)
	{
		worldMatrix->matrix = EntityHierarchy::GetParentSpace(entityManager, entityID) * transform->GetLocalMatrix();
		transform->isDirty  = false;
	}

	states[entityID] = WorldMatrixState::Done;
}

}

bool EntityHierarchy::TryGetParent(EntityManager& entityManager, unsigned int entityID, unsigned int& outParentID)
{
	const ParentComponent* parent = entityManager.GetComponent<ParentComponent>(entityID);
	if (!parent || !entityManager.IsAlive(parent->parentID))
		return false;

	outParentID = parent->parentID;
	return true;
}

std::vector<unsigned int> EntityHierarchy::GetChildren(EntityManager& entityManager, unsigned int parentID)
{
	std::vector<unsigned int> children;

	for (ParentComponent& parent : entityManager.GetAllComponents<ParentComponent>())
	{
		const unsigned int childID = entityManager.GetEntityIDForComponent(parent);

		if (parent.parentID == parentID && entityManager.IsAlive(childID))
			children.push_back(childID);
	}

	// The same order as the entities were created in, so the Hierarchy doesn't shuffle them
	std::sort(children.begin(), children.end());
	return children;
}

std::vector<unsigned int> EntityHierarchy::GetWithDescendants(EntityManager& entityManager, unsigned int entityID)
{
	std::vector<unsigned int> result = { entityID };

	// 'result' grows while it is walked: every entity added here gets its own children added later
	for (size_t index = 0; index < result.size(); index++)
	{
		for (unsigned int childID : GetChildren(entityManager, result[index]))
			result.push_back(childID);
	}

	return result;
}

bool EntityHierarchy::CanSetParent(EntityManager& entityManager, unsigned int childID, unsigned int parentID)
{
	if (childID == parentID || !entityManager.IsAlive(parentID))
		return false;

	const std::vector<unsigned int> carriedByChild = GetWithDescendants(entityManager, childID);
	return std::find(carriedByChild.begin(), carriedByChild.end(), parentID) == carriedByChild.end();
}

bool EntityHierarchy::SetParent(EntityManager& entityManager, unsigned int childID, unsigned int parentID)
{
	if (!CanSetParent(entityManager, childID, parentID))
		return false;

	const glm::mat4 worldMatrix = GetCurrentWorldMatrix(entityManager, childID);

	ParentComponent* parent = entityManager.GetComponent<ParentComponent>(childID);
	if (!parent)
		parent = &entityManager.AddComponent<ParentComponent>(childID);

	parent->parentID = parentID;
	parent->boneName.clear();

	PlaceInWorld(entityManager, childID, worldMatrix);
	return true;
}

void EntityHierarchy::ClearParent(EntityManager& entityManager, unsigned int childID)
{
	if (!entityManager.GetComponent<ParentComponent>(childID))
		return;

	const glm::mat4 worldMatrix = GetCurrentWorldMatrix(entityManager, childID);

	entityManager.RemoveComponent<ParentComponent>(childID);

	PlaceInWorld(entityManager, childID, worldMatrix);
}

glm::mat4 EntityHierarchy::GetParentSpace(EntityManager& entityManager, unsigned int entityID)
{
	unsigned int parentID = 0;
	if (!TryGetParent(entityManager, entityID, parentID))
		return glm::mat4(1.0f);

	const std::string& boneName = entityManager.GetComponent<ParentComponent>(entityID)->boneName;

	glm::mat4 boneWorldTransform;
	if (!boneName.empty() && TryGetBoneWorldTransform(entityManager, parentID, boneName, boneWorldTransform))
		return boneWorldTransform;

	const WorldMatrixComponent* parentWorldMatrix = entityManager.GetComponent<WorldMatrixComponent>(parentID);
	return parentWorldMatrix ? parentWorldMatrix->matrix : glm::mat4(1.0f);
}

bool EntityHierarchy::TryGetBoneWorldTransform(EntityManager& entityManager, unsigned int entityID, const std::string& boneName, glm::mat4& outTransform)
{
	const WorldMatrixComponent* worldMatrix = entityManager.GetComponent<WorldMatrixComponent>(entityID);

	glm::mat4 boneModelTransform;
	if (!worldMatrix || !TryGetBoneModelTransform(entityManager, entityID, boneName, boneModelTransform))
		return false;

	// A bone carries the import scale of its model, the child keeps its own size
	outTransform = worldMatrix->matrix * Math::RemoveScale(boneModelTransform);
	return true;
}

void EntityHierarchy::UpdateWorldMatrices(EntityManager& entityManager)
{
	std::vector<WorldMatrixState> states(entityManager.GetEntityCount(), WorldMatrixState::NotDone);

	for (TransformComponent& transform : entityManager.GetAllComponents<TransformComponent>())
		UpdateWorldMatrix(entityManager, entityManager.GetEntityIDForComponent(transform), states);
}

}
