#include "Core/SuntaPreCompiled.h"

#include <glm/gtx/quaternion.hpp>

#include "Model.h"
#include "Texture.h"
#include "Mesh.h"
#include "Utilities/AssimpUtilities.h"
#include "VertexLayouts.h"
#include "Core/Log.h"
#include "Core/ResourceManager.h"
#include "Material.h"
#include "RendererDevice.h"

namespace Sunta
{

Model::Model(RendererDevice& rendererDevice, const std::string& path, bool flipUV)
	: rendererDevice(&rendererDevice)
{
	LoadModel(path, flipUV);
}
	
void Model::LoadModel(std::string path, bool flipUV) 
{
	SUNTA_ENGINE_LOG_INFO("Loading model: {}", path);
	
	unsigned int flags = aiProcess_Triangulate |
		aiProcess_LimitBoneWeights |
		aiProcess_PopulateArmatureData;
	
	if (flipUV)
		flags |= aiProcess_FlipUVs;
	
	Assimp::Importer importer;
	scene = importer.ReadFile(path, flags);
		
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
	{
		SUNTA_ENGINE_LOG_ERROR("ASSIMP ERROR: {}", importer.GetErrorString());
		return;
	}
	directory = path.substr(0, path.find_last_of("\\/"));
	
	hasAnimations = scene->HasAnimations();
	globalInverseTransform = glm::inverse(AssimpUtilities::ConvertAssimpMatrixToGLM(scene->mRootNode->mTransformation));
	
	ProcessNode(scene->mRootNode);
	
	SUNTA_ENGINE_LOG_INFO("Model loaded successfully!");
	
}
	
void Model::ProcessNode(aiNode* node) 
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++) 
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		subMeshes.emplace_back(ProcessSubMesh(mesh));
	}
	
	for (unsigned int i = 0; i < node->mNumChildren; i++) 
	{
		ProcessNode(node->mChildren[i]);
	}
}
	
SubMesh Model::ProcessSubMesh(aiMesh* mesh)
{
	std::vector<unsigned int> indices;
	std::shared_ptr<Material> meshMaterial;
	
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
			indices.push_back(mesh->mFaces[i].mIndices[j]);
	}
	
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		aiString materialName;
		material->Get(AI_MATKEY_NAME, materialName);
		std::string materialKey = this->directory + ":" + materialName.C_Str();

		auto modelShader = ResourceManager::GetShaderData("Lit");

		meshMaterial = ResourceManager::LoadOrGetModelMaterial(materialKey, modelShader);

		if (meshMaterial->NeedsLoading())
		{
			LoadMaterialTextures(material, aiTextureType_DIFFUSE, meshMaterial);
			LoadMaterialTextures(material, aiTextureType_SPECULAR, meshMaterial);

			float shininess;
			if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
				meshMaterial->SetShininess(shininess);
		}
	}
		
	if (mesh->HasBones())
	{
		std::vector<SkinnedVertex> vertices(mesh->mNumVertices);
		SetVertexData(mesh, vertices);
		ProcessMeshBones(mesh, vertices);
	
		auto skinnedMesh = std::make_shared<Mesh>(*rendererDevice, vertices.data(), vertices.size() * sizeof(SkinnedVertex),
			indices, VertexLayouts::GetSkinnedLayout());

		return { skinnedMesh, meshMaterial };
	}
	
	std::vector<StaticVertex> vertices(mesh->mNumVertices);
	SetVertexData(mesh, vertices);
	
	auto staticMesh = std::make_shared<Mesh>(*rendererDevice, vertices.data(), vertices.size() * sizeof(StaticVertex),
		indices, VertexLayouts::GetStaticLayout());

	return { staticMesh, meshMaterial };
}

void Model::LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::shared_ptr<Material>& material)
{
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);

		std::string path = std::string(str.C_Str());
		std::string filename = path.substr(path.find_last_of("\\/") + 1);
		std::string textureFullPath = directory + "/" + filename;

		auto texture = ResourceManager::LoadOrGetModelTexture(textureFullPath);

		if (type == aiTextureType_DIFFUSE)
			material->AddDiffuseMap(texture);
		else if (type == aiTextureType_SPECULAR)
			material->AddSpecularMap(texture);

	}
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
	
}