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

	    // A looping animation starts over at the end, one played once stops on its last frame
	    bool                   isLooping        = true;

	    // While one animation changes into another, the old one is still played and fades out (see CrossFadeTo)
	    Animation*             previousAnimation = nullptr;
	    float                  previousTime      = 0.0f;
	    float                  blendTime         = 0.0f;
	    float                  blendDuration     = 0.0f;
	    bool                   isPreviousLooping = true;

	    // Every node of the model (bones and sockets) in the space of the model, in the order of Model::GetNodeNames
	    std::vector<glm::mat4> nodeModelTransforms;
	    unsigned int           nodeWalkIndex = 0;
	
		std::vector<glm::mat4> finalBoneMatrices = std::vector<glm::mat4>(200, glm::mat4(1.0f));
	
	public:
	    Animator() = default;
	    Animator(Model* model);
	
	    void UpdateAnimation       (float deltaTime);
	    void PlayAnimation         (Animation* animation, bool shouldLoop = true);

	    // Changes to another animation smoothly: the old one fades out while the new one fades in
	    // Without a current animation (or with a zero duration) it is the same as PlayAnimation
	    // The same animation played once can be started again (two rolls in a row), a looping one just keeps playing
	    void CrossFadeTo           (Animation* animation, float durationSeconds, bool shouldLoop = true);
	    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform);

		// For Raycasting Boundding Box fixes
		void SampleAtTime(float time);

	    const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return finalBoneMatrices; }
	    inline const Animation*       GetCurrentAnimation () const { return currentAnimation;  }

	    // How far the current animation is: 0 at the start, 1 at the end
	    float GetProgress() const;

	    // True when an animation played once reached its last frame (never for a looping one)
	    bool  IsFinished () const;

	    // Where a node of the model is right now, in the space of the model (index from Model::TryGetNodeIndex)
	    bool  TryGetNodeModelTransform(unsigned int nodeIndex, glm::mat4& outTransform) const;

	private:
	    void      CalculatePose            ();
	    void      AdvanceBlend             (float deltaTime);
	    float     GetBlendFactor           () const;
	    glm::mat4 GetPreviousLocalTransform(const AssimpNodeData* node);
	};
}