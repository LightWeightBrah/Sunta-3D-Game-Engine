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
#include <limits>

namespace
{

template<typename TVertex>
void ComputeVertexBounds(const std::vector<TVertex>& vertices, glm::vec3& outMin, glm::vec3& outMax)
{
	outMin = glm::vec3(std::numeric_limits<float>::max());
	outMax = glm::vec3(-std::numeric_limits<float>::max());

	for (const auto& vertex : vertices)
	{
		outMin = glm::min(outMin, vertex.Position);
		outMax = glm::max(outMax, vertex.Position);
	}
}

}

namespace Sunta
{

Model::Model(RendererDevice& rendererDevice, const std::string& path, bool flipUV, float importScale)
	: rendererDevice(&rendererDevice)
	, importScale(importScale)
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
	globalInverseTransform = glm::scale(globalInverseTransform, glm::vec3(importScale));
	ProcessNode(scene->mRootNode);

	localBoundsMin = glm::vec3(std::numeric_limits<float>::max());
	localBoundsMax = glm::vec3(-std::numeric_limits<float>::max());

	for (const auto& subMesh : subMeshes)
	{
		if (!subMesh.mesh)
			continue;

		localBoundsMin = glm::min(localBoundsMin, subMesh.mesh->GetLocalBoundsMin());
		localBoundsMax = glm::max(localBoundsMax, subMesh.mesh->GetLocalBoundsMax());
	}

	if (HasBones())
	{
		defaultBoneMatrices.assign(200, glm::mat4(1.0f));
		CalculateDefaultBoneTransform(scene->mRootNode, glm::mat4(1.0f));

		// Now that bind-pose bone matrices are known, skin each collected vertex
	    // the same way the GPU would and measure the result, so we 
	    // get a bounding box that actually matches the rendered model
		localBoundsMin = glm::vec3(std::numeric_limits<float>::max());
		localBoundsMax = glm::vec3(-std::numeric_limits<float>::max());

		for (const auto& vertex : skinnedVerticesForBounds)
		{
			glm::vec3 skinnedPosition = SkinVertexPositionForBounds(vertex, defaultBoneMatrices);

			localBoundsMin = glm::min(localBoundsMin, skinnedPosition);
			localBoundsMax = glm::max(localBoundsMax, skinnedPosition);
		}

		skinnedVerticesForBounds.clear();
		skinnedVerticesForBounds.shrink_to_fit();
	}
	else
	{
		// Static model: submesh vertices are already scaled correctly,
		// so just check each submesh's own bounds
		localBoundsMin = glm::vec3(std::numeric_limits<float>::max());
		localBoundsMax = glm::vec3(-std::numeric_limits<float>::max());

		for (const auto& subMesh : subMeshes)
		{
			if (!subMesh.mesh)
				continue;

			localBoundsMin = glm::min(localBoundsMin, subMesh.mesh->GetLocalBoundsMin());
			localBoundsMax = glm::max(localBoundsMax, subMesh.mesh->GetLocalBoundsMax());
		}
	}

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

		// Save these vertices so LoadModel can skin them later (once bone matrices
		// are ready) and measure their real positions to build the bounding box
		skinnedVerticesForBounds.insert(skinnedVerticesForBounds.end(), vertices.begin(), vertices.end());

		glm::vec3 boundsMin, boundsMax;
		ComputeVertexBounds(vertices, boundsMin, boundsMax);

		auto skinnedMesh = std::make_shared<Mesh>(*rendererDevice, vertices.data(), vertices.size() * sizeof(SkinnedVertex),
			indices, VertexLayouts::GetSkinnedLayout(), boundsMin, boundsMax);

		return { skinnedMesh, meshMaterial };
	}

	std::vector<StaticVertex> vertices(mesh->mNumVertices);
	SetVertexData(mesh, vertices);

	glm::vec3 boundsMin, boundsMax;
	ComputeVertexBounds(vertices, boundsMin, boundsMax);

	auto staticMesh = std::make_shared<Mesh>(*rendererDevice, vertices.data(), vertices.size() * sizeof(StaticVertex),
		indices, VertexLayouts::GetStaticLayout(), boundsMin, boundsMax);

	return { staticMesh, meshMaterial };
}

glm::vec3 Model::SkinVertexPositionForBounds(const SkinnedVertex& vertex, const std::vector<glm::mat4>& boneMatrices)
{
	glm::vec4 skinnedPosition(0.0f);
	float totalWeight = 0.0f;

	for (unsigned int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++)
	{
		float weight = vertex.weights[i];
		if (weight <= 0.0f)
			continue;

		skinnedPosition += weight * (boneMatrices[vertex.boneIDs[i]] * glm::vec4(vertex.Position, 1.0f));
		totalWeight += weight;
	}

	// No bone weights assigned to this vertex (just use its raw position)
	if (totalWeight <= 0.0f)
		return vertex.Position;

	return glm::vec3(skinnedPosition);
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

		if (!std::filesystem::exists(fullPath))
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
	aiBone* bone = mesh->mBones[boneIndex];
	unsigned int id = GetBoneId(bone);

	/*std::cout << "\nBone: " << boneIndex << " " << bone->mName.C_Str()
		<< "\nNumber of vertices affected by this bone: " << bone->mNumWeights << std::endl;*/


	for (int i = 0; i < bone->mNumWeights; i++)
	{
		aiVertexWeight& vertexWeight = bone->mWeights[i];

		unsigned int vertexId = vertexWeight.mVertexId;
		float        weight = vertexWeight.mWeight;

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
		id = boneNameToInfo.size();
		boneNameToInfo[boneName].id = id;
		boneNameToInfo[boneName].offset = AssimpUtilities::ConvertAssimpMatrixToGLM(bone->mOffsetMatrix);
	}
	else
	{
		id = boneNameToInfo[boneName].id;
	}

	return id;
}

void Model::CalculateDefaultBoneTransform(aiNode* node, glm::mat4 parentTransform)
{
	glm::mat4 nodeTransform = AssimpUtilities::ConvertAssimpMatrixToGLM(node->mTransformation);
	glm::mat4 globalTransform = parentTransform * nodeTransform;

	std::string nodeName = node->mName.C_Str();
	auto it = boneNameToInfo.find(nodeName);
	if (it != boneNameToInfo.end())
		defaultBoneMatrices[it->second.id] = globalInverseTransform * globalTransform * it->second.offset;

	for (unsigned int i = 0; i < node->mNumChildren; i++)
		CalculateDefaultBoneTransform(node->mChildren[i], globalTransform);

}


}