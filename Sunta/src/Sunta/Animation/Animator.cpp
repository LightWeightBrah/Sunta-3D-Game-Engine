#include "Core/SuntaPreCompiled.h"

#include "Animator.h"
#include "Utilities/AssimpUtilities.h"
#include "Core/Log.h"
#include "Utilities/MathUtilities.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Sunta
{

namespace
{

// Used when an animation doesn't say how fast it is
constexpr float DefaultTicksPerSecond = 25.0f;

float GetTicksPerSecondOrDefault(const Animation& animation)
{
	return animation.GetTicksPerSecond() != 0.0f ? animation.GetTicksPerSecond() : DefaultTicksPerSecond;
}

// The time of an animation 'deltaTime' seconds later: wrapped around when it loops, held at the end when it doesn't
float AdvanceTime(const Animation& animation, float time, float deltaTime, bool isLooping)
{
	const float duration = animation.GetDuration();
	if (duration <= 0.0f)
		return 0.0f;

	const float advancedTime = time + GetTicksPerSecondOrDefault(animation) * deltaTime;

	return isLooping ? fmod(advancedTime, duration) : glm::min(advancedTime, duration);
}

// 'blend' 0 gives 'from', 1 gives 'to'. Rotations take the short way round
glm::mat4 BlendTransforms(const glm::mat4& from, const glm::mat4& to, float blend)
{
	const TransformParts fromParts = Math::DecomposeTransform(from);
	const TransformParts toParts   = Math::DecomposeTransform(to);

	TransformParts blended;
	blended.position = glm::mix(fromParts.position, toParts.position, blend);
	blended.rotation = glm::normalize(glm::slerp(fromParts.rotation, toParts.rotation, blend));
	blended.scale    = glm::mix(fromParts.scale, toParts.scale, blend);

	return Math::ComposeTransform(blended);
}

}

Animator::Animator(Model* model)
	: currentModel(model), currentTime(0.0f), currentAnimation(nullptr)
{
	finalBoneMatrices.assign(200, glm::mat4(1.0f));

	if (model)
		nodeModelTransforms.assign(model->GetNodeNames().size(), glm::mat4(1.0f));
}
	
void Animator::PlayAnimation(Animation* animation, bool shouldLoop)
{
	currentAnimation  = animation;
	currentTime       = 0.0f;
	isLooping         = shouldLoop;
	previousAnimation = nullptr;
}

void Animator::CrossFadeTo(Animation* animation, float durationSeconds, bool shouldLoop)
{
	if (animation == currentAnimation && isLooping && shouldLoop)
		return;

	if (!currentAnimation || durationSeconds <= 0.0f)
	{
		PlayAnimation(animation, shouldLoop);
		return;
	}

	previousAnimation = currentAnimation;
	previousTime      = currentTime;
	isPreviousLooping = isLooping;
	blendTime         = 0.0f;
	blendDuration     = durationSeconds;

	currentAnimation  = animation;
	currentTime       = 0.0f;
	isLooping         = shouldLoop;
}

float Animator::GetProgress() const
{
	if (!currentAnimation || currentAnimation->GetDuration() <= 0.0f)
		return 0.0f;

	return glm::clamp(currentTime / currentAnimation->GetDuration(), 0.0f, 1.0f);
}

bool Animator::IsFinished() const
{
	return currentAnimation && !isLooping && GetProgress() >= 1.0f;
}

bool Animator::TryGetNodeModelTransform(unsigned int nodeIndex, glm::mat4& outTransform) const
{
	if (nodeIndex >= nodeModelTransforms.size())
		return false;

	outTransform = nodeModelTransforms[nodeIndex];
	return true;
}

void Animator::AdvanceBlend(float deltaTime)
{
	if (!previousAnimation)
		return;

	blendTime += deltaTime;

	if (blendTime >= blendDuration || previousAnimation->GetDuration() <= 0.0f)
	{
		previousAnimation = nullptr;
		return;
	}

	// The old animation keeps playing while it fades out (one played once stays on its last frame)
	previousTime = AdvanceTime(*previousAnimation, previousTime, deltaTime, isPreviousLooping);
}

// 0 at the start of the fade, 1 at the end, and eased so it neither starts nor stops abruptly
float Animator::GetBlendFactor() const
{
	const float linear = glm::clamp(blendTime / blendDuration, 0.0f, 1.0f);

	return linear * linear * (3.0f - 2.0f * linear);
}

// The bone of the old animation as it is right now (or the resting pose of the node when that animation doesn't move it)
glm::mat4 Animator::GetPreviousLocalTransform(const AssimpNodeData* node)
{
	Bone* bone = previousAnimation->FindBone(node->name);

	if (!bone)
		return node->transformation;

	bone->Update(previousTime);
	return bone->GetLocalTransform();
}
	
void Animator::UpdateAnimation(float deltaTime)
{
	if (!currentAnimation) 
	    return;
	
	currentTime = AdvanceTime(*currentAnimation, currentTime, deltaTime, isLooping);

	AdvanceBlend(deltaTime);
	
	CalculatePose();
}

void Animator::CalculatePose()
{
	nodeWalkIndex = 0;
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

	if (previousAnimation)
		nodeTransform = BlendTransforms(GetPreviousLocalTransform(node), nodeTransform, GetBlendFactor());
	
	glm::mat4 globalTransform = parentTransform * nodeTransform;

	if (nodeWalkIndex < nodeModelTransforms.size())
		nodeModelTransforms[nodeWalkIndex] = currentModel->GetGlobalInverseTransform() * globalTransform;
	nodeWalkIndex++;
	
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
	CalculatePose();
}

}