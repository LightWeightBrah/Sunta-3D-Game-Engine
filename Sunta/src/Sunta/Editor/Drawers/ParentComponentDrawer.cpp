#include "Core/SuntaPreCompiled.h"
#include "ParentComponentDrawer.h"

#include <imgui/imgui.h>

#include "NameCombo.h"
#include "ECS/Component.h"
#include "ECS/EntityManager.h"
#include "Gameplay/EntityName.h"
#include "Renderer/Model.h"

namespace Sunta
{

namespace
{

const Model* FindModel(EntityManager& entityManager, unsigned int entityID)
{
	const ModelComponent* model = entityManager.GetComponent<ModelComponent>(entityID);
	return (model && model->modelData) ? model->modelData->model.get() : nullptr;
}

}

bool ParentComponentDrawer::Draw(void* componentData, unsigned int entityID, EntityManager& entityManager)
{
	auto* parent = static_cast<ParentComponent*>(componentData);

	if (!entityManager.IsAlive(parent->parentID))
	{
		ImGui::TextDisabled("The parent no longer exists");
		return false;
	}

	ImGui::Text("Parent: %s", GetEntityName(entityManager, parent->parentID).c_str());

	bool edited = false;

	if (const Model* parentModel = FindModel(entityManager, parent->parentID))
	{
		edited = DrawNameCombo("Bone", parent->boneName, parentModel->GetNodeNames());

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Follow a bone of the parent (e.g. Weapon_R of the Knight: a sword in the hand). The Transform is then relative to that bone");
	}

	ImGui::TextDisabled("Unparent: right click in the Hierarchy, or drag it below the list");

	return edited;
}

}
