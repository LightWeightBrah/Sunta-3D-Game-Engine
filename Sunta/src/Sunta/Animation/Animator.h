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

	    // While one animation changes into another, the old one is still played and fades out (see CrossFadeTo)
	    Animation*             previousAnimation = nullptr;
	    float                  previousTime      = 0.0f;
	    float                  blendTime         = 0.0f;
	    float                  blendDuration     = 0.0f;
	
		std::vector<glm::mat4> finalBoneMatrices = std::vector<glm::mat4>(200, glm::mat4(1.0f));
	
	public:
	    Animator() = default;
	    Animator(Model* model);
	
	    void UpdateAnimation       (float deltaTime);
	    void PlayAnimation         (Animation* animation);

	    // Changes to another animation smoothly: the old one fades out while the new one fades in
	    // Without a current animation (or with a zero duration) it is the same as PlayAnimation
	    void CrossFadeTo           (Animation* animation, float durationSeconds);
	    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform);

		// For Raycasting Boundding Box fixes
		void SampleAtTime(float time);

	    const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return finalBoneMatrices; }
	    inline const Animation*       GetCurrentAnimation () const { return currentAnimation;  }

	private:
	    void      AdvanceBlend             (float deltaTime);
	    float     GetBlendFactor           () const;
	    glm::mat4 GetPreviousLocalTransform(const AssimpNodeData* node);
	};
}