#include "Core/SuntaPreCompiled.h"

#include "Animation.h"
#include "Utilities/AssimpUtilities.h"
#include "Core/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Sunta
{
	Animation::Animation(const aiScene* scene, unsigned int index)
	{
	    if (!scene || !scene->mAnimations || index >= scene->mNumAnimations) 
	    {
			SUNTA_ENGINE_LOG_ERROR("ANIMATION ERROR: No animation at index '{0}'", index);
	        return;
	    }
	
	    auto anim       =         scene->mAnimations[index];
	    duration        = (float) anim->mDuration;
	    ticksPerSecond  = (float) anim->mTicksPerSecond;
	
	    CopyHierarchyToCustomNodeData(rootNode, scene->mRootNode);
	    SetupBones(anim);

		isValid = true;
	}
	
	void Animation::CopyHierarchyToCustomNodeData(AssimpNodeData& dest, const aiNode* src)
	{
	    dest.name = src->mName.data;
	    dest.transformation = AssimpUtilities::ConvertAssimpMatrixToGLM(src->mTransformation);
	
	    for (unsigned int i = 0; i < src->mNumChildren; i++)
	    {
	        AssimpNodeData newData;
	        CopyHierarchyToCustomNodeData(newData, src->mChildren[i]);
	        dest.children.push_back(newData);
	    }
	}
	
	void Animation::SetupBones(const aiAnimation* animation)
	{
	    for (unsigned int i = 0; i < animation->mNumChannels; i++)
	    {
	        auto channel = animation->mChannels[i];
	        std::string boneName = channel->mNodeName.data;
	
	        bones.emplace(boneName, Bone(boneName, channel));
	    }
	}
	
	Bone* Animation::FindBone(const std::string& name)
	{
	    if (bones.find(name) == bones.end())
	        return nullptr;
	
	    return &bones.at(name);
	}

	std::vector<std::string> Animation::GetAnimationsNames(const aiScene* scene)
	{
		std::vector<std::string> names;

		if (!scene || !scene->HasAnimations())
			return names;

		names.reserve(scene->mNumAnimations);

		for (unsigned int i = 0; i < scene->mNumAnimations; i++)
		{
			std::string name = scene->mAnimations[i]->mName.C_Str();
			if (name.empty())
				name = "Animation_" + std::to_string(i);

			names.push_back(name);
		}

		return names;
	}

}