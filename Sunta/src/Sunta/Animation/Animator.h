#pragma once
#include <assimp/anim.h>
#include "Renderer/Model.h"
#include "Bone.h"
#include "Animation.h"

namespace Sunta
{
	class Animator 
	{
	private:
	    float                  currentTime = 0.0f;
	    float                  deltaTime   = 0.0f;
	
	    Animation*             currentAnimation = nullptr;
	    Model*                 currentModel     = nullptr;
	
		std::vector<glm::mat4> finalBoneMatrices = std::vector<glm::mat4>(200, glm::mat4(1.0f));
	
	public:
	    Animator() = default;
	    Animator(Model* model);
	
	    void UpdateAnimation       (float deltaTime);
	    void PlayAnimation         (Animation* animation);
	    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform);

		// For Raycasting Boundding Box fixes
		void SampleAtTime(float time);

	    const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return finalBoneMatrices; }
	    inline const Animation*       GetCurrentAnimation () const { return currentAnimation;  }
	};
}