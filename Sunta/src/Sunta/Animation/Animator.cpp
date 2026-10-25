#include "Core/SuntaPreCompiled.h"

#include "Animator.h"
#include "Utilities/AssimpUtilities.h"
#include "Core/Log.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Sunta
{

namespace
{

// Used when an animation doesn't say how fast it is
constexpr float DefaultTicksPerSecond = 25.0f;

// Anything shorter than this can't be a scale
constexpr float SmallestScale = 0.000001f;

struct TransformParts
{
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
};

float GetTicksPerSecondOrDefault(const Animation& animation)
{
	return animation.GetTicksPerSecond() != 0.0f ? animation.GetTicksPerSecond() : DefaultTicksPerSecond;
}

// Splits a matrix made of a translation, a rotation and a scale (what a bone gives) back into those three
TransformParts SplitIntoParts(const glm::mat4& matrix)
{
	const glm::vec3 axisX = glm::vec3(matrix[0]);
	const glm::vec3 axisY = glm::vec3(matrix[1]);
	const glm::vec3 axisZ = glm::vec3(matrix[2]);

	TransformParts parts;

	parts.position = glm::vec3(matrix[3]);
	parts.scale    = glm::vec3(glm::length(axisX), glm::length(axisY), glm::length(axisZ));

	const glm::vec3 safeScale = glm::max(parts.scale, glm::vec3(SmallestScale));
	const glm::mat3 rotation  = glm::mat3(axisX / safeScale.x, axisY / safeScale.y, axisZ / safeScale.z);

	parts.rotation = glm::normalize(glm::quat_cast(rotation));

	return parts;
}

// 'blend' 0 gives 'from', 1 gives 'to'. Rotations take the short way round
glm::mat4 BlendTransforms(const glm::mat4& from, const glm::mat4& to, float blend)
{
	const TransformParts fromParts = SplitIntoParts(from);
	const TransformParts toParts   = SplitIntoParts(to);

	const glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::mix(fromParts.position, toParts.position, blend));
	const glm::mat4 rotation    = glm::mat4_cast(glm::normalize(glm::slerp(fromParts.rotation, toParts.rotation, blend)));
	const glm::mat4 scale       = glm::scale(glm::mat4(1.0f), glm::mix(fromParts.scale, toParts.scale, blend));

	return translation * rotation * scale;
}

}

Animator::Animator(Model* model)
	: currentModel(model), currentTime(0.0f), currentAnimation(nullptr)
{
	finalBoneMatrices.assign(200, glm::mat4(1.0f));
}
	
void Animator::PlayAnimation(Animation* animation)
{
	currentAnimation  = animation;
	currentTime       = 0.0f;
	previousAnimation = nullptr;
}

void Animator::CrossFadeTo(Animation* animation, float durationSeconds)
{
	if (animation == currentAnimation)
		return;

	if (!currentAnimation || durationSeconds <= 0.0f)
	{
		PlayAnimation(animation);
		return;
	}

	previousAnimation = currentAnimation;
	previousTime      = currentTime;
	blendTime         = 0.0f;
	blendDuration     = durationSeconds;

	currentAnimation  = animation;
	currentTime       = 0.0f;
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

	// The old animation keeps playing (and looping) while it fades out
	previousTime += GetTicksPerSecondOrDefault(*previousAnimation) * deltaTime;
	previousTime  = fmod(previousTime, previousAnimation->GetDuration());
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
	
	float ticksPerSecond = currentAnimation->GetTicksPerSecond();
	currentTime         += (ticksPerSecond != 0 ? ticksPerSecond : 25.0f) * deltaTime;
	currentTime          = fmod(currentTime, currentAnimation->GetDuration());

	AdvanceBlend(deltaTime);
	
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