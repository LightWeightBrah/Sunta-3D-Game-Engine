#pragma once

#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
#include <assimp/anim.h>

struct KeyPosition { glm::vec3 position;    float timeStamp; };
struct KeyRotation { glm::quat orientation; float timeStamp; };
struct KeyScale    { glm::vec3 scale;       float timeStamp; };

class Bone 
{
private:
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale>    scales;

    glm::mat4 localTransform;
    std::string name;

public:
    Bone(const std::string& name, const aiNodeAnim* channel);

    void Update(float animationTime);

    glm::mat4 GetLocalTransform()       { return localTransform; }
    std::string GetBoneName    () const { return name; }

    int GetPositionIndex(float animationTime);
    int GetRotationIndex(float animationTime);
    int GetScaleIndex   (float animationTime);

private:
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
   
    glm::mat4 InterpolatePosition(float animationTime);
    glm::mat4 InterpolateRotation(float animationTime);
    glm::mat4 InterpolateScale   (float animationTime);
};