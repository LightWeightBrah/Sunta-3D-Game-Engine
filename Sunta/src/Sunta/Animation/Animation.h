#pragma once
#include <map>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Bone.h"
#include "Renderer/Model.h"

namespace Sunta
{

struct AssimpNodeData
{
	std::string                 name;
	glm::mat4                   transformation;
	std::vector<AssimpNodeData> children;
};
	
class Animation
{
private:
	float   duration       = 0.0f;
	float   ticksPerSecond = 0.0f;
	bool    isValid        = false;
	    
	std::map<std::string, Bone> bones;
	AssimpNodeData              rootNode;
	
	void CopyHierarchyToCustomNodeData(AssimpNodeData& dest, const aiNode* src);
	void SetupBones                   (const aiAnimation* animation);
	
public:
	Animation() = default;
	Animation(const std::string& path, Model* model, unsigned int index = 0);
	
	Bone* FindBone(const std::string& name);
	
	bool  IsValid()						const { return isValid; }
	float GetTicksPerSecond()           const { return ticksPerSecond; }
	float GetDuration()                 const { return duration; }
	const AssimpNodeData& GetRootNode() const { return rootNode; }
};

}