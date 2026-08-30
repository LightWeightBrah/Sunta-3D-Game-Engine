#pragma once
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "VertexTypes.h"

namespace Sunta
{

class Material;
class RendererDevice;

struct BoneInfo
{
	unsigned int id = 0;
	glm::mat4	 offset;
};
	
struct SubMesh
{
	std::shared_ptr<Mesh>		mesh;
	std::shared_ptr<Material>	material;
};

class Model 
{
public:
	Model() = default;
	Model(RendererDevice& rendererDevice, const std::string& path, bool flipUV, float importScale = 1.0f);
	
	inline const bool HasAnimations()				  const { return hasAnimations; }
	inline const bool HasBones()				      const { return !boneNameToInfo.empty(); }
	inline const std::vector<SubMesh>& GetSubMeshes() const { return subMeshes;		}
	inline const aiScene* GetScene()				  const { return scene;         }
	
	inline const std::map<std::string, BoneInfo> GetBoneNameToInfo() const { return boneNameToInfo;			}
	inline const glm::mat4 GetGlobalInverseTransform()				 const { return globalInverseTransform; }
	inline const std::vector<glm::mat4> GetDefaultBoneMatrices()	 const { return defaultBoneMatrices; }
	
private:
	RendererDevice*					rendererDevice;

	Assimp::Importer				importer;
	const aiScene*					scene;

	std::string						directory;
	
	std::vector<SubMesh>			subMeshes;
	
	std::map<std::string, BoneInfo> boneNameToInfo;
	glm::mat4						globalInverseTransform;
	std::vector<glm::mat4>			defaultBoneMatrices;
	
	bool							hasAnimations;
	float							importScale = 1.0f;
	
	void LoadModel(std::string path, bool flipUV);
		
	void ProcessNode(aiNode* node);
	SubMesh ProcessSubMesh(aiMesh* mesh);
	
	void LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::shared_ptr<Material>& material);
	void TryFillMissingTextureByNamingConvention(
		const std::string& diffuseFilename,
		const std::string& textureTypeName,
		std::shared_ptr<Material>& material);

	void ProcessMeshBones		(aiMesh* mesh, std::vector<SkinnedVertex>& vertices);
	void ProcessMeshSingleBone	(aiMesh* mesh, std::vector<SkinnedVertex>& vertices, int boneIndex);
	
	void SetBonesForVertex		(std::vector<SkinnedVertex>& vertices, unsigned int vertexId, unsigned int id, float weight);
		
	template<typename T>
	void SetVertexData(aiMesh* mesh, std::vector<T>& vertices)
	{
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			vertices[i].Position	= { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
			vertices[i].Normal		= { mesh->mNormals[i].x , mesh->mNormals[i].y , mesh->mNormals[i].z	 };
			vertices[i].TexCoords	= mesh->mTextureCoords[0] ? 
				glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);
				
				
			if constexpr (std::is_same_v< T, SkinnedVertex>)
			{
				for (unsigned int j = 0; j < MAX_NUM_BONES_PER_VERTEX; j++)
				{
					vertices[i].boneIDs[j] = 0;
					vertices[i].weights[j] = 0.0f;
				}
			}
			else
			{
				vertices[i].Position *= importScale;
			}
		}
	}
	
	unsigned int GetBoneId		(aiBone* bone);
	void CalculateDefaultBoneTransform(aiNode* node, glm::mat4 parentTransform);

};

}