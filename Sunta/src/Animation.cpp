#include "Animation.h"
#include <iostream>
#include "AssimpUtilities.h"

Animation::Animation(const std::string& animationPath, Model* model, unsigned int index)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);

    if (!scene || !scene->mAnimations) 
    {
        std::cout << "ANIMATION ERROR: No animations in " << animationPath << std::endl;
        return;
    }

    auto anim       =         scene->mAnimations[index];
    duration        = (float) anim->mDuration;
    ticksPerSecond  = (float) anim->mTicksPerSecond;

    CopyHierarchyToCustomNodeData(rootNode, scene->mRootNode);
    SetupBones(anim);
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