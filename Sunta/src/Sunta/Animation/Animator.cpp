#include "Core/SuntaPreCompiled.h"

#include "Animator.h"
#include "Utilities/AssimpUtilities.h"
#include "Core/Log.h"

namespace Sunta
{

Animator::Animator(Model* model)
	: currentModel(model), currentTime(0.0f), currentAnimation(nullptr)
{
	finalBoneMatrices.assign(200, glm::mat4(1.0f));
}
	
void Animator::PlayAnimation(Animation* animation)
{
	currentAnimation = animation;
	currentTime      = 0.0f;
}
	
void Animator::UpdateAnimation(float deltaTime)
{
	if (!currentAnimation) 
	    return;
	
	float ticksPerSecond = currentAnimation->GetTicksPerSecond();
	currentTime         += (ticksPerSecond != 0 ? ticksPerSecond : 25.0f) * deltaTime;
	currentTime          = fmod(currentTime, currentAnimation->GetDuration());
	
	CalculateBoneTransform(&currentAnimation->GetRootNode(), glm::mat4(1.0f));
}
	
void Animator::CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
{
	std::string nodeName = node->name;
	glm::mat4   nodeTransform = node->transformation;
	
	Bone* bone = currentAnimation->FindBone(nodeName);
	if (bone)
	{
	    bone->Update(currentTime);
	    nodeTransform = bone->GetLocalTransform();
	}
	
	glm::mat4 globalTransform = parentTransform * nodeTransform;
	
	auto& boneInfoMap = currentModel->GetBoneNameToInfo();
	if (boneInfoMap.count(nodeName))
	{
		const auto& info = boneInfoMap.at(nodeName);
	    finalBoneMatrices[info.id] = currentModel->GetGlobalInverseTransform() * globalTransform * info.offset;
	}
	
	for (const auto& child : node->children)
	    CalculateBoneTransform(&child, globalTransform);
}

void Animator::SampleAtTime(float time)
{
	if (!currentAnimation)
		return;

	currentTime = time;
	CalculateBoneTransform(&currentAnimation->GetRootNode(), glm::mat4(1.0f));
}

}