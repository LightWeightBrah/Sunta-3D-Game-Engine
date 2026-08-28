#include "Core/SuntaPreCompiled.h"
#include "SceneSerializer.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "Scene/Scene.h"
#include "ECS/EntityManager.h"
#include "ECS/Component.h"
#include "Core/Log.h"

namespace Sunta
{

using json = nlohmann::json;

namespace SceneKeys
{
	constexpr const char* SceneName				    = "scene_name";
	constexpr const char* Entities				    = "entities";

	constexpr const char* TagComponent              = "tag";
	constexpr const char* TransformComponent        = "transform";
	constexpr const char* ScriptComponent           = "script";
	constexpr const char* MeshComponent             = "mesh";
	constexpr const char* ModelComponent            = "model";
	constexpr const char* AnimatorComponent         = "animator";
	constexpr const char* DirectionalLightComponent = "directional_light";
	constexpr const char* PointLightComponent	    = "point_light";
	constexpr const char* SpotLightComponent        = "spot_light";

	constexpr const char* Name                      = "name";
	constexpr const char* Position                  = "position";
	constexpr const char* Rotation                  = "rotation";
	constexpr const char* Scale                     = "scale";
	constexpr const char* ScriptPath                = "script_path";
	constexpr const char* IsVisible                 = "is_visible";
	constexpr const char* MeshName                  = "mesh_name";
	constexpr const char* MaterialName              = "material_name";
	constexpr const char* ModelName                 = "model_name";
	constexpr const char* CurrentAnimationName      = "current_animation_name";
												    
	constexpr const char* Color						= "color";
	constexpr const char* Attenuation               = "attenuation";
	constexpr const char* Ambient                   = "ambient";
	constexpr const char* Diffuse                   = "diffuse";
	constexpr const char* Specular                  = "specular";
	constexpr const char* AttenuationConstant       = "attenuation_constant";
	constexpr const char* AttenuationLinear         = "attenuation_linear";
	constexpr const char* AttenuationQuadratic      = "attenuation_quadratic";
	constexpr const char* InnerCutOff               = "inner_cut_off";
	constexpr const char* OuterCutOff               = "outer_cut_off";

}

static json SerializeVec3(const glm::vec3& vec)
{
	return { vec.x, vec.y, vec.z };
}

static glm::vec3 DeserializeVec3(const json& j, const glm::vec3& defaultValue = glm::vec3(0.0f))
{
	if (j.is_array() && j.size() == 3)
		return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());

	return defaultValue;
}

static json SerializeLightColor(const LightColor& color)
{
	return {
		{ SceneKeys::Ambient,  SerializeVec3(color.ambientIntensity)  },
		{ SceneKeys::Diffuse,  SerializeVec3(color.diffuseIntensity)  },
		{ SceneKeys::Specular, SerializeVec3(color.specularIntensity) }
	};
}

static void DeserializeLightColor(const json& j, LightColor& color)
{
	if (j.contains(SceneKeys::Ambient))  color.ambientIntensity  = DeserializeVec3(j[SceneKeys::Ambient]);
	if (j.contains(SceneKeys::Diffuse))  color.diffuseIntensity  = DeserializeVec3(j[SceneKeys::Diffuse]);
	if (j.contains(SceneKeys::Specular)) color.specularIntensity = DeserializeVec3(j[SceneKeys::Specular]);
}

static json SerializeAttenuation(const Attenuation& attenuation)
{
	return {
		{ SceneKeys::AttenuationConstant,  attenuation.constant  },
		{ SceneKeys::AttenuationLinear,    attenuation.linear    },
		{ SceneKeys::AttenuationQuadratic, attenuation.quadratic }
	};
}

static void DeserializeAttenuation(const json& j, Attenuation& attenuation)
{
	if (j.contains(SceneKeys::AttenuationConstant))  attenuation.constant  = j[SceneKeys::AttenuationConstant];
	if (j.contains(SceneKeys::AttenuationLinear))    attenuation.linear    = j[SceneKeys::AttenuationLinear];
	if (j.contains(SceneKeys::AttenuationQuadratic)) attenuation.quadratic = j[SceneKeys::AttenuationQuadratic];
}

bool SceneSerializer::Serialize(const std::string& filepath, Scene& scene)
{
	auto& entityManager = scene.GetEntityManager();
	unsigned int totalEntities = entityManager.GetEntityCount();

	json sceneJson;
	sceneJson[SceneKeys::SceneName] = scene.GetName();

	json entitiesArray = json::array();

	for (unsigned int i = 0; i < totalEntities; i++)
	{
		json entityJson;

		if (auto* tag = entityManager.GetComponent<TagComponent>(i))
		{
			entityJson[SceneKeys::TagComponent] = { { SceneKeys::Name, tag->name } };
		}

		if (auto* transform = entityManager.GetComponent<TransformComponent>(i))
		{
			entityJson[SceneKeys::TransformComponent] =
			{
				{ SceneKeys::Position, SerializeVec3(transform->position) },
				{ SceneKeys::Rotation, SerializeVec3(transform->rotation) },
				{ SceneKeys::Scale,    SerializeVec3(transform->scale)    }
			};
		}

		if (auto* script = entityManager.GetComponent<ScriptComponent>(i))
		{
			if (!script->scripts.empty())
			{
				json scriptsArray = json::array();
				for (const auto& container : script->scripts)
				{
					scriptsArray.push_back({ { SceneKeys::ScriptPath, container.scriptPath} });
				}

				entityJson[SceneKeys::ScriptComponent] = scriptsArray;
			}
		}

		if (auto* mesh = entityManager.GetComponent<MeshComponent>(i))
		{
			entityJson[SceneKeys::MeshComponent] =
			{
				{ SceneKeys::IsVisible,    mesh->isVisible     },
				{ SceneKeys::MeshName,     mesh->meshName      },
				{ SceneKeys::MaterialName, mesh->materialName  }
			};
		}

		if (auto* model = entityManager.GetComponent<ModelComponent>(i))
		{
			entityJson[SceneKeys::ModelComponent] =
			{
				{ SceneKeys::IsVisible,    model->isVisible     },
				{ SceneKeys::ModelName,    model->modelName     }
			};
		}

		if (auto* animator = entityManager.GetComponent<AnimatorComponent>(i))
		{
			entityJson[SceneKeys::AnimatorComponent] =
			{
				{ SceneKeys::CurrentAnimationName, animator->currentAnimationName }
			};
		}

		if (auto* directionalLight = entityManager.GetComponent<DirectionalLightComponent>(i))
		{
			entityJson[SceneKeys::DirectionalLightComponent] = SerializeLightColor(directionalLight->color);
		}

		if (auto* pointLight = entityManager.GetComponent<PointLightComponent>(i))
		{
			entityJson[SceneKeys::PointLightComponent] =
			{
				{ SceneKeys::Color,       SerializeLightColor(pointLight->color)        },
				{ SceneKeys::Attenuation, SerializeAttenuation(pointLight->attenuation) }
			};
		}

		if (auto* spotLight = entityManager.GetComponent<SpotLightComponent>(i))
		{
			entityJson[SceneKeys::SpotLightComponent] =
			{
				{ SceneKeys::Color,       SerializeLightColor(spotLight->color)        },
				{ SceneKeys::Attenuation, SerializeAttenuation(spotLight->attenuation) },
				{ SceneKeys::InnerCutOff, spotLight->innerCutOffAngle                  },
				{ SceneKeys::OuterCutOff, spotLight->outerCutOffAngle                  }
			};
		}

		if(!entityJson.empty())
			entitiesArray.push_back(entityJson);
	}

	sceneJson[SceneKeys::Entities] = entitiesArray;

	std::ofstream outFile(filepath);
	if (!outFile.is_open())
	{
		SUNTA_ENGINE_LOG_ERROR("SceneSerializer::Serialize: Couldn't open file for writing: '{0}'", filepath);
		return false;
	}

	outFile << sceneJson.dump(4);
	outFile.close();

	SUNTA_ENGINE_LOG_INFO("SceneSerializer::Serialize: Successfully Saved Scene to: '{0}'", filepath);
	return true;
}

bool SceneSerializer::Deserialize(const std::string& filepath, Scene& scene)
{
	std::ifstream inFile(filepath);
	if (!inFile.is_open())
	{
		SUNTA_ENGINE_LOG_ERROR("SceneSerializer::Deserialize: Couldn't open file: '{0}'", filepath);
		return false;
	}

	json sceneJson;
	try
	{
		inFile >> sceneJson;
	}
	catch (const json::parse_error& error)
	{
		SUNTA_ENGINE_LOG_ERROR("SceneSerializer::Deserialize: JSON Parse Error in '{0}': '{1}'", filepath, error.what());
		return false;
	}

	inFile.close();

	scene.Clear();
	scene.SetFilePath(filepath);
	
	if (sceneJson.contains(SceneKeys::SceneName))
	{
		scene.SetName(sceneJson[SceneKeys::SceneName].get<std::string>());
	}
	
	auto& entityManager = scene.GetEntityManager();

	if (!sceneJson.contains(SceneKeys::Entities) || !sceneJson[SceneKeys::Entities].is_array())
	{
		SUNTA_ENGINE_LOG_WARNING("SceneSerializer::Deserialize: File '{0}' contains no entities...", filepath);
		return true;
	}

	for (const auto& entityJson : sceneJson[SceneKeys::Entities])
	{
		if(entityJson.empty())
			continue;

		unsigned int entity = entityManager.CreateEntity();

		if (entityJson.contains(SceneKeys::TagComponent))
		{
			const auto& tagData = entityJson[SceneKeys::TagComponent];
			auto& tag = entityManager.AddComponent<TagComponent>(entity);
			tag.name = tagData.value(SceneKeys::Name, "Entity");
		}

		if (entityJson.contains(SceneKeys::TransformComponent))
		{
			const auto& transformData = entityJson[SceneKeys::TransformComponent];
			auto& transform = entityManager.AddComponent<TransformComponent>(entity);

			if(transformData.contains(SceneKeys::Position))
				transform.position = DeserializeVec3(transformData[SceneKeys::Position]);

			if (transformData.contains(SceneKeys::Rotation))
				transform.rotation = DeserializeVec3(transformData[SceneKeys::Rotation]);
			
			if (transformData.contains(SceneKeys::Scale))
				transform.scale    = DeserializeVec3(transformData[SceneKeys::Scale], glm::vec3(1.0f));


			transform.isDirty  = true;
		}

		entityManager.AddComponent<WorldMatrixComponent>(entity);

		if (entityJson.contains(SceneKeys::ScriptComponent))
		{
			const auto& scriptData = entityJson[SceneKeys::ScriptComponent];

			if (scriptData.is_array())
			{
				auto& scriptComponent = entityManager.AddComponent<ScriptComponent>(entity);
				
				for (const auto& scriptJson : scriptData)
				{
					std::string path = scriptJson.value(SceneKeys::ScriptPath, "");
					if (!path.empty())
					{
						scriptComponent.LoadScript(path, entity);
					}
				}
			}
		}

		if (entityJson.contains(SceneKeys::MeshComponent))
		{
			const auto& meshData = entityJson[SceneKeys::MeshComponent];
			auto& meshComponent = entityManager.AddComponent<MeshComponent>(entity);

			meshComponent.isVisible    = meshData.value(SceneKeys::IsVisible,    true);
			meshComponent.meshName     = meshData.value(SceneKeys::MeshName,     MeshComponent::NULL_ASSET_NAME);
			meshComponent.materialName = meshData.value(SceneKeys::MaterialName, MeshComponent::NULL_ASSET_NAME);
			meshComponent.isDirty      = true;
		}

		if (entityJson.contains(SceneKeys::ModelComponent))
		{
			const auto& modelData = entityJson[SceneKeys::ModelComponent];
			auto& modelComponent = entityManager.AddComponent<ModelComponent>(entity);

			modelComponent.entityID  = entity;
			modelComponent.isVisible = modelData.value(SceneKeys::IsVisible, true);
			modelComponent.modelName = modelData.value(SceneKeys::ModelName, ModelComponent::NULL_ASSET_NAME);
			modelComponent.isDirty = true;
		}

		if (entityJson.contains(SceneKeys::AnimatorComponent))
		{
			const auto& animatorData = entityJson[SceneKeys::AnimatorComponent];
			auto& animatorComponent = entityManager.AddComponent<AnimatorComponent>(entity);

			animatorComponent.currentAnimationName = animatorData.value(SceneKeys::CurrentAnimationName, "");
		}

		if (entityJson.contains(SceneKeys::DirectionalLightComponent))
		{
			auto& directionalLight = entityManager.AddComponent<DirectionalLightComponent>(entity);
			DeserializeLightColor(entityJson[SceneKeys::DirectionalLightComponent], directionalLight.color);
		}

		if (entityJson.contains(SceneKeys::PointLightComponent))
		{
			const auto& lightData = entityJson[SceneKeys::PointLightComponent];
			auto& pointLight = entityManager.AddComponent<PointLightComponent>(entity);

			if (lightData.contains(SceneKeys::Color))
				DeserializeLightColor(lightData[SceneKeys::Color], pointLight.color);

			if (lightData.contains(SceneKeys::Attenuation))
				DeserializeAttenuation(lightData[SceneKeys::Attenuation], pointLight.attenuation);
		}

		if (entityJson.contains(SceneKeys::SpotLightComponent))
		{
			const auto& lightData = entityJson[SceneKeys::SpotLightComponent];
			auto& spotLight = entityManager.AddComponent<SpotLightComponent>(entity);

			if (lightData.contains(SceneKeys::Color))
				DeserializeLightColor(lightData[SceneKeys::Color], spotLight.color);

			if (lightData.contains(SceneKeys::Attenuation))
				DeserializeAttenuation(lightData[SceneKeys::Attenuation], spotLight.attenuation);

			spotLight.innerCutOffAngle = lightData.value(SceneKeys::InnerCutOff, spotLight.innerCutOffAngle);
			spotLight.outerCutOffAngle = lightData.value(SceneKeys::OuterCutOff, spotLight.outerCutOffAngle);
		}

	}
	
	SUNTA_ENGINE_LOG_INFO("SceneSerializer::Deserialize: Successfully loaded Scene '{0}' from '{1}'", scene.GetName(), filepath);
	return true;

}

}