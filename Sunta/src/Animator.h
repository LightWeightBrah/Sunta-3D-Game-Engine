#pragma once
#include <assimp/anim.h>
#include "Model.h"
#include "Bone.h"
#include "Animation.h"

class Animator 
{
private:
    float                  currentTime;
    float                  deltaTime;

    Animation*             currentAnimation;
    Model*                 currentModel;

    std::vector<glm::mat4> finalBoneMatrices;

public:
    Animator() = default;
    Animator(Model* model);

    void UpdateAnimation       (float deltaTime);
    void PlayAnimation         (Animation* animation);
    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform);

    const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return finalBoneMatrices; }
    inline const Animation*       GetCurrentAnimation () const { return currentAnimation;  }
};