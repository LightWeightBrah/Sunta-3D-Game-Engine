#include "Bone.h"
#include "AssimpUtilities.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

Bone::Bone(const std::string& name, const aiNodeAnim* channel)
    : name(name), localTransform(1.0f)
{
    for (unsigned int i = 0; i < channel->mNumPositionKeys; i++) 
    {
        KeyPosition data;
        data.position  = { channel->mPositionKeys[i].mValue.x, channel->mPositionKeys[i].mValue.y, channel->mPositionKeys[i].mValue.z };
        data.timeStamp = (float)channel->mPositionKeys[i].mTime;
        positions.push_back(data);
    }

    for (unsigned int i = 0; i < channel->mNumRotationKeys; i++) 
    {
        KeyRotation data;
        data.orientation = { (float)channel->mRotationKeys[i].mValue.w, (float)channel->mRotationKeys[i].mValue.x,
                             (float)channel->mRotationKeys[i].mValue.y, (float)channel->mRotationKeys[i].mValue.z };
        data.timeStamp   =   (float)channel->mRotationKeys[i].mTime;
        rotations.push_back(data);
    }

    for (unsigned int i = 0; i < channel->mNumScalingKeys; i++) 
    {
        KeyScale data;
        data.scale     = { channel->mScalingKeys[i].mValue.x, channel->mScalingKeys[i].mValue.y, channel->mScalingKeys[i].mValue.z };
        data.timeStamp = (float)channel->mScalingKeys[i].mTime;
        scales.push_back(data);
    }
}

void Bone::Update(float animationTime)
{
    glm::mat4 translation = InterpolatePosition(animationTime);
    glm::mat4 rotation    = InterpolateRotation(animationTime);
    glm::mat4 scale       = InterpolateScale   (animationTime);
    localTransform = translation * rotation * scale;
}

template<typename T>
unsigned int GetKeyIndex(float animTime, const std::vector<T>& keys)
{
    for (unsigned int i = 0; i < keys.size() - 1; i++) 
    {
        if (animTime < keys[i + 1].timeStamp)
            return i;
    }
    return 0;
}

float GetFactor(float animTime, float frameTime, float nextFrameTime)
{
    float deltaTime = nextFrameTime - frameTime;
    return (animTime - frameTime) / deltaTime;
}

glm::mat4 Bone::InterpolatePosition(float animationTime)
{
    if (positions.size() == 1)
        return glm::translate(glm::mat4(1.0f), positions[0].position);

    unsigned int index     = GetKeyIndex(animationTime, positions);
    unsigned int nextIndex = index + 1;
    float factor = GetFactor(animationTime, positions[index].timeStamp, positions[nextIndex].timeStamp);

    glm::vec3 finalPos = glm::mix(positions[index].position, positions[nextIndex].position, factor);
    return glm::translate(glm::mat4(1.0f), finalPos);
}

glm::mat4 Bone::InterpolateRotation(float animationTime)
{
    if (rotations.size() == 1)
        return glm::toMat4(glm::normalize(rotations[0].orientation));

    unsigned int index     = GetKeyIndex(animationTime, rotations);
    unsigned int nextIndex = index + 1;
    float factor = GetFactor(animationTime, rotations[index].timeStamp, rotations[nextIndex].timeStamp);

    glm::quat finalRot = glm::slerp(rotations[index].orientation, rotations[nextIndex].orientation, factor);
    return glm::toMat4(glm::normalize(finalRot));
}

glm::mat4 Bone::InterpolateScale(float animationTime)
{
    if (scales.size() == 1)
        return glm::scale(glm::mat4(1.0f), scales[0].scale);

    unsigned int index     = GetKeyIndex(animationTime, scales);
    unsigned int nextIndex = index + 1;
    float factor = GetFactor(animationTime, scales[index].timeStamp, scales[nextIndex].timeStamp);

    glm::vec3 finalScale = glm::mix(scales[index].scale, scales[nextIndex].scale, factor);
    return glm::scale(glm::mat4(1.0f), finalScale);
}