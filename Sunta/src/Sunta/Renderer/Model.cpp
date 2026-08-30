#include "Core/SuntaPreCompiled.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
#include "Core/EngineAssets.h"
#include <optional>
#include "Utilities/TextureNamingConventionsUtilities.h"

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
	
	unsigned int flags = aiProcess_Triangulate 
		| aiProcess_LimitBoneWeights 
		| aiProcess_PopulateArmatureData
		| aiProcess_GlobalScale;
	
	if (flipUV)
		flags |= aiProcess_FlipUVs;
	
	scene = importer.ReadFile(path, flags);
		
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
	{
		SUNTA_ENGINE_LOG_ERROR("ASSIMP ERROR: {}", importer.GetErrorString());
		return;
	}
	directory = path.substr(0, path.find_last_of("\\/"));
	
	hasAnimations = scene->HasAnimations();
	globalInverseTransform = glm::inverse(AssimpUtilities::ConvertAssimpMatrixToGLM(scene->mRootNode->mTransformation));
	// Make suere model isn't super big on scene
	globalInverseTransform = glm::scale(globalInverseTransform, glm::vec3(0.05f));	
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

		auto modelShader = ResourceManager::GetShaderData(Sunta::EngineAssets::Shaders::Lit);

		meshMaterial = ResourceManager::LoadOrGetMaterial(materialKey, modelShader);

		if (meshMaterial->NeedsLoading())
		{
			LoadMaterialTextures(material, aiTextureType_DIFFUSE, meshMaterial);
			LoadMaterialTextures(material, aiTextureType_SPECULAR, meshMaterial);

			meshMaterial->SetAlphaCutout(true);

			float shininess = 0.0f;
			bool hasShininess = (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) && shininess > 1.0f;

			if (hasShininess)
			{
				//SUNTA_ENGINE_LOG_WARNING("Material '{}' shininess from FBX = {}", materialName.C_Str(), shininess);
				meshMaterial->SetShininess(shininess);
			}
			else
			{
				meshMaterial->SetShininess(64.0f);
			}

			aiColor3D specColor(0.0f, 0.0f, 0.0f);
			bool hasSpecColor = (material->Get(AI_MATKEY_COLOR_SPECULAR, specColor) == AI_SUCCESS)
				&& (specColor.r + specColor.g + specColor.b) > 0.01f;

			constexpr float defaultSpecularIntensity = 0.15f;

			if (hasSpecColor)
			{
				meshMaterial->SetSpecular(glm::vec3(specColor.r, specColor.g, specColor.b));
			}
			else if (meshMaterial->GetSpecularMaps().empty())
			{
				// No texture & No Specular color in material => Don't add full white
				meshMaterial->SetSpecular(glm::vec3(defaultSpecularIntensity));
			}
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

		auto texture = ResourceManager::LoadOrGetTexture(textureFullPath);

		if (type == aiTextureType_DIFFUSE)
		{
			material->AddDiffuseMap(texture);
			TryFillMissingTextureByNamingConvention(filename, "specular", material);
		}
		else if (type == aiTextureType_SPECULAR)
		{
			material->AddSpecularMap(texture);
		}

	}
}
	
void Model::TryFillMissingTextureByNamingConvention(const std::string& diffuseFilename, const std::string& textureTypeName, std::shared_ptr<Material>& material)
{
	if (textureTypeName == "specular" && !material->GetSpecularMaps().empty())
		return;

	std::vector<std::string> candidates =
		TextureNamingConventionsUtilities::TryResolveCandidates(diffuseFilename, textureTypeName);

	for (const auto& candidateFilename : candidates)
	{
		std::string fullPath = directory + "/" + candidateFilename;

		if(!std::filesystem::exists(fullPath))
			continue;

		auto texture = ResourceManager::LoadOrGetTexture(fullPath);

		if (textureTypeName == "specular")
			material->AddSpecularMap(texture);

		SUNTA_ENGINE_LOG_INFO("Model '{0}': added '{1}' texture by naming convention -> '{2}'",
			directory, textureTypeName, fullPath);

		return;
	}

	if (!candidates.empty())
	{
		SUNTA_ENGINE_LOG_WARNING("Model '{0}': Tried {1} Texture naming-convention guess(es) for '{2}' (e.g. '{3}') but none of the files exist",
			directory, candidates.size(), textureTypeName, directory + "/" + candidates.front());
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