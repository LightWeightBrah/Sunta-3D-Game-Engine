#pragma once
#include <iostream>
#include <glm/gtx/quaternion.hpp>

#include "Model.h"
#include "Texture.h"
#include "Mesh.h"
#include "AssimpUtilities.h"
#include "VertexLayouts.h"

Model::Model(const std::string& path, bool flipUV)
{
	LoadModel(path, flipUV);
}

void Model::LoadModel(std::string path, bool flipUV) 
{
	std::cout << "Loading model: " << path << std::endl;

	unsigned int flags = aiProcess_Triangulate |
		aiProcess_LimitBoneWeights |
		aiProcess_PopulateArmatureData;

	if (flipUV)
		flags |= aiProcess_FlipUVs;

	Assimp::Importer importer;
	scene = importer.ReadFile(path, flags);
	
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
	{
		std::cout << "ASSIMP ERROR: " << importer.GetErrorString() << std::endl;
		return;
	}
	directory = path.substr(0, path.find_last_of("\\/"));

	hasAnimations = scene->HasAnimations();
	globalInverseTransform = glm::inverse(AssimpUtilities::ConvertAssimpMatrixToGLM(scene->mRootNode->mTransformation));

	ProcessNode(scene->mRootNode);

	std::cout << "Model loaded successfully!" << std::endl;

}

void Model::ProcessNode(aiNode* node) 
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++) 
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.emplace_back(ProcessMesh(mesh));
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++) 
	{
		ProcessNode(node->mChildren[i]);
	}
}

Mesh Model::ProcessMesh(aiMesh* mesh)
{
	std::vector<unsigned int> indices;
	std::vector<TextureItem>  meshTextures;

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
			indices.push_back(mesh->mFaces[i].mIndices[j]);
	}

	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		std::vector<TextureItem> diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		meshTextures.insert(meshTextures.end(), diffuseMaps.begin(), diffuseMaps.end());

		std::vector<TextureItem> specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		meshTextures.insert(meshTextures.end(), specularMaps.begin(), specularMaps.end());
	}
	
	if (mesh->HasBones())
	{
		std::vector<SkinnedVertex> vertices(mesh->mNumVertices);
		SetVertexData(mesh, vertices);
		ProcessMeshBones(mesh, vertices);

		return Mesh(vertices.data(), vertices.size() * sizeof(SkinnedVertex),
			indices, meshTextures, VertexLayouts::GetSkinnedLayout());
	}

	std::vector<StaticVertex> vertices(mesh->mNumVertices);
	SetVertexData(mesh, vertices);

	return Mesh(vertices.data(), vertices.size() * sizeof(StaticVertex),
		indices, meshTextures, VertexLayouts::GetStaticLayout());
}

void Model::ProcessMeshBones(aiMesh* mesh, std::vector<SkinnedVertex>& vertices)
{
	for (int i = 0; i < mesh->mNumBones; i++)
		ProcessMeshSingleBone(mesh, vertices, i);
}

void Model::ProcessMeshSingleBone(aiMesh* mesh, std::vector<SkinnedVertex>& vertices, int boneIndex)
{
	aiBone* bone		= mesh->mBones[boneIndex];
	unsigned int id = GetBoneId(bone);

	/*std::cout << "\nBone: " << boneIndex << " " << bone->mName.C_Str()
		<< "\nNumber of vertices affected by this bone: " << bone->mNumWeights << std::endl;*/

	
	for (int i = 0; i < bone->mNumWeights; i++)
	{
		aiVertexWeight& vertexWeight = bone->mWeights[i];

		unsigned int vertexId = vertexWeight.mVertexId;
		float        weight   = vertexWeight.mWeight;
		
		SetBonesForVertex(vertices, vertexId, id, weight);

		/*std::cout << "\nVertex id: " << vertexWeight.mVertexId << "\nWeight: " 
			<< vertexWeight.mWeight << std::endl;*/
	}
}

void Model::SetBonesForVertex(std::vector<SkinnedVertex>& vertices, unsigned int vertexId, unsigned int id, float weight)
{
	for (unsigned int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++)
	{
		SkinnedVertex& vertex = vertices[vertexId];

		if (vertex.weights[i] == 0.0)
		{
			vertex.boneIDs[i] = id;
			vertex.weights[i] = weight;
			break;
		}
	}
}

unsigned int Model::GetBoneId(aiBone* bone)
{
	unsigned int id = 0;
	std::string boneName(bone->mName.C_Str());

	if (boneNameToInfo.find(boneName) == boneNameToInfo.end())
	{
		id							= boneNameToInfo.size();
		boneNameToInfo[boneName].id = id;
		boneNameToInfo[boneName].offset = AssimpUtilities::ConvertAssimpMatrixToGLM(bone->mOffsetMatrix);
	}
	else
	{
		id = boneNameToInfo[boneName].id;
	}

	return id;
}

std::vector<TextureItem> Model::LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
	std::vector<TextureItem> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) 
	{
		aiString str;
		mat->GetTexture(type, i, &str);

		std::string path = std::string(str.C_Str());
		std::string filename = path.substr(path.find_last_of("\\/") + 1);

		std::string fullPath = directory + "/" + filename;


		bool skip = false;
		for (unsigned int j = 0; j < texturesLoaded.size(); j++) 
		{
			if (texturesLoaded[j].texture->GetFilepath() == fullPath) 
			{
				textures.push_back(texturesLoaded[j]);
				skip = true;
				break;
			}
		}

		if (!skip) 
		{
			auto tex = std::make_shared<Texture>(fullPath);
			TextureItem texture;

			texture.texture = tex;
			texture.type	= typeName;

			textures.push_back(texture);
			texturesLoaded.push_back(texture);
		}
	}
	return textures;
}