#pragma once
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "VertexTypes.h"

struct BoneInfo
{
	unsigned int id = 0;
	glm::mat4	 offset;
};

class Model 
{
public:
	Model() = default;
	Model(const std::string& path, bool flipUV);

	inline const bool HasAnimations()			const { return hasAnimations; }
	inline const std::vector<Mesh>& GetMeshes() const { return meshes; }

	inline const std::map<std::string, BoneInfo> GetBoneNameToInfo() const { return boneNameToInfo; }
	inline const glm::mat4				 GetGlobalInverseTransform() const { return globalInverseTransform; }

private:
	const aiScene*					scene;
	std::string						directory;

	std::vector<Mesh>				meshes;
	std::vector<TextureItem>		texturesLoaded;

	std::map<std::string, BoneInfo> boneNameToInfo;
	glm::mat4						globalInverseTransform;

	bool							hasAnimations;

	void LoadModel(std::string path, bool flipUV);
	
	void ProcessNode(aiNode* node);
	Mesh ProcessMesh(aiMesh* mesh);

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
		}
	}

	unsigned int GetBoneId		(aiBone* bone);

	std::vector<TextureItem> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};