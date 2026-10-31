#include "Core/SuntaPreCompiled.h"
#include "SceneSerializer.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Scene/Scene.h"
#include "ECS/EntityManager.h"
#include "ECS/Component.h"
#include "Core/Log.h"
#include "Physics/CollisionLayers.h"
#include "Core/VirtualFileSystem.h"

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
	constexpr const char* BoxColliderComponent      = "box_collider";
	constexpr const char* PhysicsBodyComponent      = "physics_body";
	constexpr const char* DirectionalLightComponent = "directional_light";
	constexpr const char* PointLightComponent	    = "point_light";
	constexpr const char* SpotLightComponent        = "spot_light";
	constexpr const char* CameraComponent           = "camera";
	constexpr const char* CharacterControllerComponent = "character_controller";
	constexpr const char* CharacterAnimationComponent = "character_animation";
	constexpr const char* FollowCameraComponent     = "follow_camera";
	constexpr const char* LockOnComponent           = "lock_on";
	constexpr const char* LockOnTargetComponent     = "lock_on_target";
	constexpr const char* CharacterActionsComponent = "character_actions";
	constexpr const char* HealthComponent           = "health";
	constexpr const char* StaminaComponent          = "stamina";
	constexpr const char* HitboxComponent           = "hitbox";
	constexpr const char* EnemyComponent            = "enemy";
	constexpr const char* MeleeEnemyComponent       = "melee_enemy";
	constexpr const char* ParentComponent           = "parent";

	constexpr const char* Name                      = "name";
	constexpr const char* Position                  = "position";
	constexpr const char* Rotation                  = "rotation";
	constexpr const char* Scale                     = "scale";
	constexpr const char* RotationQuaternion        = "rotation_quaternion";
	constexpr const char* ScriptPath                = "script_path";
	constexpr const char* IsVisible                 = "is_visible";
	constexpr const char* MeshName                  = "mesh_name";
	constexpr const char* MaterialName              = "material_name";
	constexpr const char* ModelName                 = "model_name";
	constexpr const char* CurrentAnimationName      = "current_animation_name";

	constexpr const char* ShowGizmos				= "show_gizmos";
	constexpr const char* LocalOffset               = "local_offset";
	constexpr const char* HalfExtents               = "half_extents";
	constexpr const char* IsTrigger                 = "is_trigger";
	constexpr const char* Layer                     = "layer";

	constexpr const char* IsKinematic               = "is_kinematic";
	constexpr const char* UseGravity                = "use_gravity";
	constexpr const char* Velocity                  = "velocity";
	constexpr const char* AngularVelocity			= "angular_velocity";
	constexpr const char* FreezeRotationX			= "freeze_rotation_x";
	constexpr const char* FreezeRotationY			= "freeze_rotation_y";
	constexpr const char* FreezeRotationZ			= "freeze_rotation_z";
	constexpr const char* Mass						= "mass";
	constexpr const char* Restitution				= "restitution";
	constexpr const char* Friction					= "friction";
	constexpr const char* LinearDamping				= "linear_damping";
	constexpr const char* AngularDamping			= "angular_damping";
												    
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

	constexpr const char* FieldOfView               = "field_of_view";
	constexpr const char* NearPlane                 = "near_plane";
	constexpr const char* FarPlane                  = "far_plane";
	constexpr const char* IsActive                  = "is_active";

	constexpr const char* WalkSpeed                 = "walk_speed";
	constexpr const char* RunSpeed                  = "run_speed";
	constexpr const char* Acceleration              = "acceleration";
	constexpr const char* TurnSpeed                 = "turn_speed";
	constexpr const char* FacingOffsetDegrees       = "facing_offset_degrees";
	constexpr const char* IsPlayerControlled        = "is_player_controlled";

	constexpr const char* Distance                  = "distance";
	constexpr const char* PivotHeight               = "pivot_height";
	constexpr const char* ShoulderOffset            = "shoulder_offset";
	constexpr const char* ViewYawOffsetDegrees      = "view_yaw_offset_degrees";
	constexpr const char* ViewPitchOffsetDegrees    = "view_pitch_offset_degrees";
	constexpr const char* MouseSensitivity          = "mouse_sensitivity";
	constexpr const char* InvertY                   = "invert_y";
	constexpr const char* MinPitchDegrees           = "min_pitch_degrees";
	constexpr const char* MaxPitchDegrees           = "max_pitch_degrees";
	constexpr const char* FollowSharpness           = "follow_sharpness";
	constexpr const char* LockOnPitchDegrees        = "lock_on_pitch_degrees";
	constexpr const char* LockOnSharpness           = "lock_on_sharpness";

	constexpr const char* MaxDistance               = "max_distance";
	constexpr const char* SearchAngleDegrees        = "search_angle_degrees";
	constexpr const char* EyeHeight                 = "eye_height";
	constexpr const char* SwitchMouseDistance       = "switch_mouse_distance";
	constexpr const char* RequireLineOfSight        = "require_line_of_sight";
	constexpr const char* ShowSightRays             = "show_sight_rays";

	constexpr const char* IdleAnimation             = "idle_animation";
	constexpr const char* WalkAnimation             = "walk_animation";
	constexpr const char* RunAnimation              = "run_animation";
	constexpr const char* BlendSeconds              = "blend_seconds";

	constexpr const char* IsLockable                = "is_lockable";
	constexpr const char* AimHeight                 = "aim_height";

	constexpr const char* RollAnimation             = "roll_animation";
	constexpr const char* BackstepAnimation         = "backstep_animation";
	constexpr const char* LightAttack1Animation     = "light_attack_1_animation";
	constexpr const char* LightAttack2Animation     = "light_attack_2_animation";
	constexpr const char* LightAttack3Animation     = "light_attack_3_animation";
	constexpr const char* HeavyAttackAnimation      = "heavy_attack_animation";
	constexpr const char* RollSpeed                 = "roll_speed";
	constexpr const char* BackstepSpeed             = "backstep_speed";
	constexpr const char* AttackStepSpeed           = "attack_step_speed";
	constexpr const char* InvulnerableFrom          = "invulnerable_from";
	constexpr const char* InvulnerableTo            = "invulnerable_to";
	constexpr const char* InputBufferFrom           = "input_buffer_from";
	constexpr const char* CancelFrom                = "cancel_from";
	constexpr const char* HitReactionAnimation      = "hit_reaction_animation";
	constexpr const char* DeathAnimation            = "death_animation";
	constexpr const char* LightHitFrom              = "light_hit_from";
	constexpr const char* LightHitTo                = "light_hit_to";
	constexpr const char* HeavyHitFrom              = "heavy_hit_from";
	constexpr const char* HeavyHitTo                = "heavy_hit_to";
	constexpr const char* HeavyDamageMultiplier     = "heavy_damage_multiplier";

	constexpr const char* MaxHealth                 = "max_health";

	constexpr const char* MaxStamina                = "max_stamina";
	constexpr const char* RegenerationPerSecond     = "regeneration_per_second";
	constexpr const char* RegenerationDelaySeconds  = "regeneration_delay_seconds";
	constexpr const char* RollCost                  = "roll_cost";
	constexpr const char* BackstepCost              = "backstep_cost";
	constexpr const char* LightAttackCost           = "light_attack_cost";
	constexpr const char* HeavyAttackCost           = "heavy_attack_cost";
	constexpr const char* RunCostPerSecond          = "run_cost_per_second";

	constexpr const char* Damage                    = "damage";

	constexpr const char* SightDistance             = "sight_distance";
	constexpr const char* SightAngleDegrees         = "sight_angle_degrees";
	constexpr const char* HearingDistance           = "hearing_distance";
	constexpr const char* AttackDistance            = "attack_distance";
	constexpr const char* RunDistance               = "run_distance";
	constexpr const char* GiveUpDistance            = "give_up_distance";
	constexpr const char* AttackCooldownSeconds     = "attack_cooldown_seconds";
	constexpr const char* HeavyAttackChance         = "heavy_attack_chance";
	constexpr const char* MaxLightCombo             = "max_light_combo";
	constexpr const char* RecoverSeconds            = "recover_seconds";

	// The parent is saved as its place in the list of entities (entity IDs change every time a scene is loaded)
	constexpr const char* ParentIndex               = "index";
	constexpr const char* BoneName                  = "bone_name";

}

// Where each entity ended up in the saved list of entities (entity ID -> place in the list)
using SavedIndices = std::unordered_map<unsigned int, size_t>;

// Written after all entities, because a parent can come later in the list than its child
static void SerializeParents(EntityManager& entityManager, const SavedIndices& savedIndices, json& entitiesArray)
{
	for (ParentComponent& parent : entityManager.GetAllComponents<ParentComponent>())
	{
		const auto child      = savedIndices.find(entityManager.GetEntityIDForComponent(parent));
		const auto savedParent = savedIndices.find(parent.parentID);

		if (child == savedIndices.end() || savedParent == savedIndices.end())
			continue;

		entitiesArray[child->second][SceneKeys::ParentComponent] =
		{
			{ SceneKeys::ParentIndex, savedParent->second },
			{ SceneKeys::BoneName,    parent.boneName     }
		};
	}
}

// 'entityAtSavedIndex' says which entity was created for each place in the saved list (-1: the place was empty)
static void DeserializeParents(EntityManager& entityManager, const json& entitiesJson, const std::vector<int>& entityAtSavedIndex)
{
	for (size_t savedIndex = 0; savedIndex < entityAtSavedIndex.size(); savedIndex++)
	{
		const json& entityJson = entitiesJson[savedIndex];
		const int   childID    = entityAtSavedIndex[savedIndex];

		if (childID < 0 || !entityJson.contains(SceneKeys::ParentComponent))
			continue;

		const json&  parentData  = entityJson[SceneKeys::ParentComponent];
		const size_t parentIndex = parentData.value(SceneKeys::ParentIndex, entityAtSavedIndex.size());

		if (parentIndex >= entityAtSavedIndex.size() || entityAtSavedIndex[parentIndex] < 0)
			continue;

		ParentComponent& parent = entityManager.AddComponent<ParentComponent>(static_cast<unsigned int>(childID));
		parent.parentID = static_cast<unsigned int>(entityAtSavedIndex[parentIndex]);
		parent.boneName = parentData.value(SceneKeys::BoneName, std::string());
	}
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

static json SerializeQuaternion(const glm::quat& quaternion)
{
	return { quaternion.w, quaternion.x, quaternion.y, quaternion.z };
}

static glm::quat DeserializeQuaternion(const json& j, const glm::quat& defaultValue = glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
{
	if (j.is_array() && j.size() == 4)
		return glm::quat(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>());

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

std::string SceneSerializer::SerializeToString(Scene& scene)
{
	auto& entityManager = scene.GetEntityManager();
	unsigned int totalEntities = entityManager.GetEntityCount();

	json sceneJson;
	sceneJson[SceneKeys::SceneName] = scene.GetName();

	json entitiesArray = json::array();
	SavedIndices savedIndices;

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
				{ SceneKeys::Scale,    SerializeVec3(transform->scale)    },
				{ SceneKeys::RotationQuaternion, SerializeQuaternion(transform->rotationQuaternion) }
			};
		}

		if (auto* script = entityManager.GetComponent<ScriptComponent>(i))
		{
			if (!script->scripts.empty())
			{
				json scriptsArray = json::array();
				for (const auto& container : script->scripts)
				{
					// Saved as a virtual path ("@game/Scripts/a.lua"), because the physical path differs on every machine and in the packaged game
					scriptsArray.push_back({ { SceneKeys::ScriptPath, VirtualFileSystem::ToVirtualPath(container.scriptPath) } });
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

		if (auto* boxCollider = entityManager.GetComponent<BoxColliderComponent>(i))
		{
			entityJson[SceneKeys::BoxColliderComponent] =
			{
				{ SceneKeys::ShowGizmos,                      boxCollider->showGizmos   },
				{ SceneKeys::LocalOffset,       SerializeVec3(boxCollider->localOffset) },
				{ SceneKeys::HalfExtents,       SerializeVec3(boxCollider->halfExtents) },
				{ SceneKeys::IsTrigger,                       boxCollider->isTrigger    },
				{ SceneKeys::Layer, static_cast<unsigned int>(boxCollider->layer)       }
			};
		}

		if (auto* physicsBody = entityManager.GetComponent<PhysicsBodyComponent>(i))
		{
			entityJson[SceneKeys::PhysicsBodyComponent] =
			{
				{ SceneKeys::IsKinematic,                    physicsBody->isKinematic     },
				{ SceneKeys::UseGravity,                     physicsBody->useGravity      },
				{ SceneKeys::Velocity,         SerializeVec3(physicsBody->velocity)       },
				{ SceneKeys::AngularVelocity,  SerializeVec3(physicsBody->angularVelocity)},

				{ SceneKeys::FreezeRotationX,                physicsBody->freezeRotationX },
				{ SceneKeys::FreezeRotationY,                physicsBody->freezeRotationY },
				{ SceneKeys::FreezeRotationZ,                physicsBody->freezeRotationZ },

				{ SceneKeys::Mass,                           physicsBody->mass            },
				{ SceneKeys::Restitution,                    physicsBody->restitution     },
				{ SceneKeys::Friction,                       physicsBody->friction        },
				{ SceneKeys::LinearDamping,                  physicsBody->linearDamping   },
				{ SceneKeys::AngularDamping,                 physicsBody->angularDamping  }
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

		if (auto* camera = entityManager.GetComponent<CameraComponent>(i))
		{
			entityJson[SceneKeys::CameraComponent] =
			{
				{ SceneKeys::FieldOfView, camera->fieldOfView },
				{ SceneKeys::NearPlane,   camera->nearPlane   },
				{ SceneKeys::FarPlane,    camera->farPlane    },
				{ SceneKeys::IsActive,    camera->isActive    }
			};
		}

		if (auto* characterController = entityManager.GetComponent<CharacterControllerComponent>(i))
		{
			entityJson[SceneKeys::CharacterControllerComponent] =
			{
				{ SceneKeys::WalkSpeed,           characterController->walkSpeed           },
				{ SceneKeys::RunSpeed,            characterController->runSpeed            },
				{ SceneKeys::Acceleration,        characterController->acceleration        },
				{ SceneKeys::TurnSpeed,           characterController->turnSpeed           },
				{ SceneKeys::FacingOffsetDegrees, characterController->facingOffsetDegrees },
				{ SceneKeys::IsPlayerControlled,  characterController->isPlayerControlled  }
			};
		}

		if (auto* characterAnimation = entityManager.GetComponent<CharacterAnimationComponent>(i))
		{
			entityJson[SceneKeys::CharacterAnimationComponent] =
			{
				{ SceneKeys::IdleAnimation, characterAnimation->idleAnimation },
				{ SceneKeys::WalkAnimation, characterAnimation->walkAnimation },
				{ SceneKeys::RunAnimation,  characterAnimation->runAnimation  },
				{ SceneKeys::BlendSeconds,  characterAnimation->blendSeconds  }
			};
		}

		if (auto* followCamera = entityManager.GetComponent<FollowCameraComponent>(i))
		{
			entityJson[SceneKeys::FollowCameraComponent] =
			{
				{ SceneKeys::Distance,         followCamera->distance           },
				{ SceneKeys::PivotHeight,      followCamera->pivotHeight        },
				{ SceneKeys::ShoulderOffset,   followCamera->shoulderOffset     },
				{ SceneKeys::ViewYawOffsetDegrees,   followCamera->viewYawOffsetDegrees   },
				{ SceneKeys::ViewPitchOffsetDegrees, followCamera->viewPitchOffsetDegrees },
				{ SceneKeys::MouseSensitivity, followCamera->mouseSensitivity   },
				{ SceneKeys::InvertY,          followCamera->invertY            },
				{ SceneKeys::MinPitchDegrees,  followCamera->minPitchDegrees    },
				{ SceneKeys::MaxPitchDegrees,  followCamera->maxPitchDegrees    },
				{ SceneKeys::FollowSharpness,  followCamera->followSharpness    },
				{ SceneKeys::LockOnPitchDegrees, followCamera->lockOnPitchDegrees },
				{ SceneKeys::LockOnSharpness,  followCamera->lockOnSharpness    }
			};
		}

		if (auto* lockOn = entityManager.GetComponent<LockOnComponent>(i))
		{
			entityJson[SceneKeys::LockOnComponent] =
			{
				{ SceneKeys::MaxDistance,         lockOn->maxDistance         },
				{ SceneKeys::SearchAngleDegrees,  lockOn->searchAngleDegrees  },
				{ SceneKeys::EyeHeight,           lockOn->eyeHeight           },
				{ SceneKeys::SwitchMouseDistance, lockOn->switchMouseDistance },
				{ SceneKeys::RequireLineOfSight,  lockOn->requireLineOfSight  },
				{ SceneKeys::IsPlayerControlled,  lockOn->isPlayerControlled  },
				{ SceneKeys::ShowSightRays,       lockOn->showSightRays       }
			};
		}

		if (auto* lockOnTarget = entityManager.GetComponent<LockOnTargetComponent>(i))
		{
			entityJson[SceneKeys::LockOnTargetComponent] =
			{
				{ SceneKeys::IsLockable, lockOnTarget->isLockable },
				{ SceneKeys::AimHeight,  lockOnTarget->aimHeight  }
			};
		}

		if (auto* actions = entityManager.GetComponent<CharacterActionsComponent>(i))
		{
			entityJson[SceneKeys::CharacterActionsComponent] =
			{
				{ SceneKeys::RollAnimation,         actions->rollAnimation         },
				{ SceneKeys::BackstepAnimation,     actions->backstepAnimation     },
				{ SceneKeys::LightAttack1Animation, actions->lightAttack1Animation },
				{ SceneKeys::LightAttack2Animation, actions->lightAttack2Animation },
				{ SceneKeys::LightAttack3Animation, actions->lightAttack3Animation },
				{ SceneKeys::HeavyAttackAnimation,  actions->heavyAttackAnimation  },
				{ SceneKeys::RollSpeed,             actions->rollSpeed             },
				{ SceneKeys::BackstepSpeed,         actions->backstepSpeed         },
				{ SceneKeys::AttackStepSpeed,       actions->attackStepSpeed       },
				{ SceneKeys::InvulnerableFrom,      actions->invulnerableFrom      },
				{ SceneKeys::InvulnerableTo,        actions->invulnerableTo        },
				{ SceneKeys::InputBufferFrom,       actions->inputBufferFrom       },
				{ SceneKeys::CancelFrom,            actions->cancelFrom            },
				{ SceneKeys::BlendSeconds,          actions->blendSeconds          },
				{ SceneKeys::HitReactionAnimation,  actions->hitReactionAnimation  },
				{ SceneKeys::DeathAnimation,        actions->deathAnimation        },
				{ SceneKeys::LightHitFrom,          actions->lightHitFrom          },
				{ SceneKeys::LightHitTo,            actions->lightHitTo            },
				{ SceneKeys::HeavyHitFrom,          actions->heavyHitFrom          },
				{ SceneKeys::HeavyHitTo,            actions->heavyHitTo            },
				{ SceneKeys::HeavyDamageMultiplier, actions->heavyDamageMultiplier }
			};
		}

		if (auto* health = entityManager.GetComponent<HealthComponent>(i))
		{
			entityJson[SceneKeys::HealthComponent] = { { SceneKeys::MaxHealth, health->maxHealth } };
		}

		if (auto* stamina = entityManager.GetComponent<StaminaComponent>(i))
		{
			entityJson[SceneKeys::StaminaComponent] =
			{
				{ SceneKeys::MaxStamina,               stamina->maxStamina               },
				{ SceneKeys::RegenerationPerSecond,    stamina->regenerationPerSecond    },
				{ SceneKeys::RegenerationDelaySeconds, stamina->regenerationDelaySeconds },
				{ SceneKeys::RollCost,                 stamina->rollCost                 },
				{ SceneKeys::BackstepCost,             stamina->backstepCost             },
				{ SceneKeys::LightAttackCost,          stamina->lightAttackCost          },
				{ SceneKeys::HeavyAttackCost,          stamina->heavyAttackCost          },
				{ SceneKeys::RunCostPerSecond,         stamina->runCostPerSecond         }
			};
		}

		if (auto* hitbox = entityManager.GetComponent<HitboxComponent>(i))
		{
			entityJson[SceneKeys::HitboxComponent] =
			{
				{ SceneKeys::Damage,                     hitbox->damage       },
				{ SceneKeys::LocalOffset,  SerializeVec3(hitbox->localOffset) },
				{ SceneKeys::HalfExtents,  SerializeVec3(hitbox->halfExtents) },
				{ SceneKeys::Layer, static_cast<unsigned int>(hitbox->layer)  },
				{ SceneKeys::ShowGizmos,                 hitbox->showGizmos   }
			};
		}

		if (auto* enemy = entityManager.GetComponent<EnemyComponent>(i))
		{
			entityJson[SceneKeys::EnemyComponent] =
			{
				{ SceneKeys::SightDistance,     enemy->sightDistance     },
				{ SceneKeys::SightAngleDegrees, enemy->sightAngleDegrees },
				{ SceneKeys::HearingDistance,   enemy->hearingDistance   },
				{ SceneKeys::EyeHeight,         enemy->eyeHeight         }
			};
		}

		if (auto* meleeEnemy = entityManager.GetComponent<MeleeEnemyComponent>(i))
		{
			entityJson[SceneKeys::MeleeEnemyComponent] =
			{
				{ SceneKeys::AttackDistance,        meleeEnemy->attackDistance        },
				{ SceneKeys::RunDistance,           meleeEnemy->runDistance           },
				{ SceneKeys::GiveUpDistance,        meleeEnemy->giveUpDistance        },
				{ SceneKeys::AttackCooldownSeconds, meleeEnemy->attackCooldownSeconds },
				{ SceneKeys::HeavyAttackChance,     meleeEnemy->heavyAttackChance     },
				{ SceneKeys::MaxLightCombo,         meleeEnemy->maxLightCombo         },
				{ SceneKeys::RecoverSeconds,        meleeEnemy->recoverSeconds        }
			};
		}

		if (!entityJson.empty())
		{
			savedIndices[i] = entitiesArray.size();
			entitiesArray.push_back(entityJson);
		}
	}

	SerializeParents(entityManager, savedIndices, entitiesArray);

	sceneJson[SceneKeys::Entities] = entitiesArray;

	return sceneJson.dump(4);
}

bool SceneSerializer::Serialize(const std::string& filepath, Scene& scene)
{
	std::ofstream outFile(filepath);
	if (!outFile.is_open())
	{
		SUNTA_ENGINE_LOG_ERROR("SceneSerializer::Serialize: Couldn't open file for writing: '{0}'", filepath);
		return false;
	}

	outFile << SerializeToString(scene);
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

	const json& entitiesJson = sceneJson[SceneKeys::Entities];
	std::vector<int> entityAtSavedIndex;

	for (const auto& entityJson : entitiesJson)
	{
		entityAtSavedIndex.push_back(-1);

		if(entityJson.empty())
			continue;

		unsigned int entity = entityManager.CreateEntity();
		entityAtSavedIndex.back() = static_cast<int>(entity);

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

			if (transformData.contains(SceneKeys::RotationQuaternion))
				transform.rotationQuaternion = DeserializeQuaternion(transformData[SceneKeys::RotationQuaternion]);
			else
				transform.SyncQuaternionFromEuler();


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
						scriptComponent.AddScript(VirtualFileSystem::Resolve(path));
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

		if (entityJson.contains(SceneKeys::BoxColliderComponent))
		{
			const auto& boxColliderData = entityJson[SceneKeys::BoxColliderComponent];
			auto& boxColliderComponent = entityManager.AddComponent<BoxColliderComponent>(entity);

			if (boxColliderData.contains(SceneKeys::LocalOffset))
				boxColliderComponent.localOffset = DeserializeVec3(boxColliderData[SceneKeys::LocalOffset]);

			if (boxColliderData.contains(SceneKeys::HalfExtents))
				boxColliderComponent.halfExtents = DeserializeVec3(boxColliderData[SceneKeys::HalfExtents], glm::vec3(0.5f));

			boxColliderComponent.showGizmos   = boxColliderData.value(SceneKeys::ShowGizmos, false);
			boxColliderComponent.isTrigger    = boxColliderData.value(SceneKeys::IsTrigger, false);
			boxColliderComponent.layer        = static_cast<CollisionLayer>(boxColliderData.value(SceneKeys::Layer, static_cast<unsigned int>(DefaultCollisionLayer)));
		}

		if (entityJson.contains(SceneKeys::PhysicsBodyComponent))
		{
			const auto& physicsBodyData = entityJson[SceneKeys::PhysicsBodyComponent];
			auto& physicsBodyComponent = entityManager.AddComponent<PhysicsBodyComponent>(entity);

			physicsBodyComponent.isKinematic = physicsBodyData.value(SceneKeys::IsKinematic, false);
			physicsBodyComponent.useGravity  = physicsBodyData.value(SceneKeys::UseGravity,  true);

			if (physicsBodyData.contains(SceneKeys::Velocity))
				physicsBodyComponent.velocity = DeserializeVec3(physicsBodyData[SceneKeys::Velocity]);

			if (physicsBodyData.contains(SceneKeys::AngularVelocity))
				physicsBodyComponent.angularVelocity = DeserializeVec3(physicsBodyData[SceneKeys::AngularVelocity]);

			physicsBodyComponent.freezeRotationX = physicsBodyData.value(SceneKeys::FreezeRotationX, false);
			physicsBodyComponent.freezeRotationY = physicsBodyData.value(SceneKeys::FreezeRotationY, false);
			physicsBodyComponent.freezeRotationZ = physicsBodyData.value(SceneKeys::FreezeRotationZ, false);

			// The fallback is the component's own default, so the default values live in ONE place (Component.h)
			physicsBodyComponent.mass           = physicsBodyData.value(SceneKeys::Mass,		   physicsBodyComponent.mass);
			physicsBodyComponent.restitution    = physicsBodyData.value(SceneKeys::Restitution,	   physicsBodyComponent.restitution);
			physicsBodyComponent.friction       = physicsBodyData.value(SceneKeys::Friction,	   physicsBodyComponent.friction);
			physicsBodyComponent.linearDamping  = physicsBodyData.value(SceneKeys::LinearDamping,  physicsBodyComponent.linearDamping);
			physicsBodyComponent.angularDamping = physicsBodyData.value(SceneKeys::AngularDamping, physicsBodyComponent.angularDamping);

			if (physicsBodyData.contains(SceneKeys::Velocity))
				physicsBodyComponent.velocity = DeserializeVec3(physicsBodyData[SceneKeys::Velocity]);
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

		if (entityJson.contains(SceneKeys::CameraComponent))
		{
			const auto& cameraData = entityJson[SceneKeys::CameraComponent];
			auto& camera = entityManager.AddComponent<CameraComponent>(entity);

			camera.fieldOfView = cameraData.value(SceneKeys::FieldOfView, camera.fieldOfView);
			camera.nearPlane   = cameraData.value(SceneKeys::NearPlane,   camera.nearPlane);
			camera.farPlane    = cameraData.value(SceneKeys::FarPlane,    camera.farPlane);
			camera.isActive    = cameraData.value(SceneKeys::IsActive,    camera.isActive);
		}

		if (entityJson.contains(SceneKeys::CharacterControllerComponent))
		{
			const auto& controllerData = entityJson[SceneKeys::CharacterControllerComponent];
			auto& controller = entityManager.AddComponent<CharacterControllerComponent>(entity);

			// A missing key keeps the default written in CharacterControllerComponent.h
			controller.walkSpeed            = controllerData.value(SceneKeys::WalkSpeed,           controller.walkSpeed);
			controller.runSpeed             = controllerData.value(SceneKeys::RunSpeed,            controller.runSpeed);
			controller.acceleration         = controllerData.value(SceneKeys::Acceleration,        controller.acceleration);
			controller.turnSpeed            = controllerData.value(SceneKeys::TurnSpeed,           controller.turnSpeed);
			controller.facingOffsetDegrees  = controllerData.value(SceneKeys::FacingOffsetDegrees, controller.facingOffsetDegrees);
			controller.isPlayerControlled   = controllerData.value(SceneKeys::IsPlayerControlled,  controller.isPlayerControlled);
		}

		if (entityJson.contains(SceneKeys::CharacterAnimationComponent))
		{
			const auto& animationData = entityJson[SceneKeys::CharacterAnimationComponent];
			auto& characterAnimation = entityManager.AddComponent<CharacterAnimationComponent>(entity);

			// A missing key keeps the default written in CharacterAnimationComponent.h
			characterAnimation.idleAnimation = animationData.value(SceneKeys::IdleAnimation, characterAnimation.idleAnimation);
			characterAnimation.walkAnimation = animationData.value(SceneKeys::WalkAnimation, characterAnimation.walkAnimation);
			characterAnimation.runAnimation  = animationData.value(SceneKeys::RunAnimation,  characterAnimation.runAnimation);
			characterAnimation.blendSeconds  = animationData.value(SceneKeys::BlendSeconds,  characterAnimation.blendSeconds);
		}

		if (entityJson.contains(SceneKeys::FollowCameraComponent))
		{
			const auto& followData = entityJson[SceneKeys::FollowCameraComponent];
			auto& followCamera = entityManager.AddComponent<FollowCameraComponent>(entity);

			// A missing key keeps the default written in FollowCameraComponent.h
			followCamera.distance            = followData.value(SceneKeys::Distance,           followCamera.distance);
			followCamera.pivotHeight         = followData.value(SceneKeys::PivotHeight,        followCamera.pivotHeight);
			followCamera.shoulderOffset      = followData.value(SceneKeys::ShoulderOffset,     followCamera.shoulderOffset);
			followCamera.viewYawOffsetDegrees   = followData.value(SceneKeys::ViewYawOffsetDegrees,   followCamera.viewYawOffsetDegrees);
			followCamera.viewPitchOffsetDegrees = followData.value(SceneKeys::ViewPitchOffsetDegrees, followCamera.viewPitchOffsetDegrees);
			followCamera.mouseSensitivity    = followData.value(SceneKeys::MouseSensitivity,   followCamera.mouseSensitivity);
			followCamera.invertY             = followData.value(SceneKeys::InvertY,            followCamera.invertY);
			followCamera.minPitchDegrees     = followData.value(SceneKeys::MinPitchDegrees,    followCamera.minPitchDegrees);
			followCamera.maxPitchDegrees     = followData.value(SceneKeys::MaxPitchDegrees,    followCamera.maxPitchDegrees);
			followCamera.followSharpness     = followData.value(SceneKeys::FollowSharpness,    followCamera.followSharpness);
			followCamera.lockOnPitchDegrees  = followData.value(SceneKeys::LockOnPitchDegrees, followCamera.lockOnPitchDegrees);
			followCamera.lockOnSharpness     = followData.value(SceneKeys::LockOnSharpness,    followCamera.lockOnSharpness);
		}

		if (entityJson.contains(SceneKeys::LockOnComponent))
		{
			const auto& lockOnData = entityJson[SceneKeys::LockOnComponent];
			auto& lockOn = entityManager.AddComponent<LockOnComponent>(entity);

			// A missing key keeps the default written in LockOnComponent.h
			lockOn.maxDistance         = lockOnData.value(SceneKeys::MaxDistance,         lockOn.maxDistance);
			lockOn.searchAngleDegrees  = lockOnData.value(SceneKeys::SearchAngleDegrees,  lockOn.searchAngleDegrees);
			lockOn.eyeHeight           = lockOnData.value(SceneKeys::EyeHeight,           lockOn.eyeHeight);
			lockOn.switchMouseDistance = lockOnData.value(SceneKeys::SwitchMouseDistance, lockOn.switchMouseDistance);
			lockOn.requireLineOfSight  = lockOnData.value(SceneKeys::RequireLineOfSight,  lockOn.requireLineOfSight);
			lockOn.isPlayerControlled  = lockOnData.value(SceneKeys::IsPlayerControlled,  lockOn.isPlayerControlled);
			lockOn.showSightRays       = lockOnData.value(SceneKeys::ShowSightRays,       lockOn.showSightRays);
		}

		if (entityJson.contains(SceneKeys::LockOnTargetComponent))
		{
			const auto& targetData = entityJson[SceneKeys::LockOnTargetComponent];
			auto& lockOnTarget = entityManager.AddComponent<LockOnTargetComponent>(entity);

			lockOnTarget.isLockable = targetData.value(SceneKeys::IsLockable, lockOnTarget.isLockable);
			lockOnTarget.aimHeight  = targetData.value(SceneKeys::AimHeight,  lockOnTarget.aimHeight);
		}

		if (entityJson.contains(SceneKeys::CharacterActionsComponent))
		{
			const auto& actionsData = entityJson[SceneKeys::CharacterActionsComponent];
			auto& actions = entityManager.AddComponent<CharacterActionsComponent>(entity);

			// A missing key keeps the default written in CharacterActionsComponent.h
			actions.rollAnimation         = actionsData.value(SceneKeys::RollAnimation,         actions.rollAnimation);
			actions.backstepAnimation     = actionsData.value(SceneKeys::BackstepAnimation,     actions.backstepAnimation);
			actions.lightAttack1Animation = actionsData.value(SceneKeys::LightAttack1Animation, actions.lightAttack1Animation);
			actions.lightAttack2Animation = actionsData.value(SceneKeys::LightAttack2Animation, actions.lightAttack2Animation);
			actions.lightAttack3Animation = actionsData.value(SceneKeys::LightAttack3Animation, actions.lightAttack3Animation);
			actions.heavyAttackAnimation  = actionsData.value(SceneKeys::HeavyAttackAnimation,  actions.heavyAttackAnimation);
			actions.rollSpeed             = actionsData.value(SceneKeys::RollSpeed,             actions.rollSpeed);
			actions.backstepSpeed         = actionsData.value(SceneKeys::BackstepSpeed,         actions.backstepSpeed);
			actions.attackStepSpeed       = actionsData.value(SceneKeys::AttackStepSpeed,       actions.attackStepSpeed);
			actions.invulnerableFrom      = actionsData.value(SceneKeys::InvulnerableFrom,      actions.invulnerableFrom);
			actions.invulnerableTo        = actionsData.value(SceneKeys::InvulnerableTo,        actions.invulnerableTo);
			actions.inputBufferFrom       = actionsData.value(SceneKeys::InputBufferFrom,       actions.inputBufferFrom);
			actions.cancelFrom            = actionsData.value(SceneKeys::CancelFrom,            actions.cancelFrom);
			actions.blendSeconds          = actionsData.value(SceneKeys::BlendSeconds,          actions.blendSeconds);
			actions.hitReactionAnimation  = actionsData.value(SceneKeys::HitReactionAnimation,  actions.hitReactionAnimation);
			actions.deathAnimation        = actionsData.value(SceneKeys::DeathAnimation,        actions.deathAnimation);
			actions.lightHitFrom          = actionsData.value(SceneKeys::LightHitFrom,          actions.lightHitFrom);
			actions.lightHitTo            = actionsData.value(SceneKeys::LightHitTo,            actions.lightHitTo);
			actions.heavyHitFrom          = actionsData.value(SceneKeys::HeavyHitFrom,          actions.heavyHitFrom);
			actions.heavyHitTo            = actionsData.value(SceneKeys::HeavyHitTo,            actions.heavyHitTo);
			actions.heavyDamageMultiplier = actionsData.value(SceneKeys::HeavyDamageMultiplier, actions.heavyDamageMultiplier);
		}

		if (entityJson.contains(SceneKeys::HealthComponent))
		{
			auto& health = entityManager.AddComponent<HealthComponent>(entity);
			health.maxHealth     = entityJson[SceneKeys::HealthComponent].value(SceneKeys::MaxHealth, health.maxHealth);
			health.currentHealth = health.maxHealth;
		}

		if (entityJson.contains(SceneKeys::StaminaComponent))
		{
			const auto& staminaData = entityJson[SceneKeys::StaminaComponent];
			auto& stamina = entityManager.AddComponent<StaminaComponent>(entity);

			// A missing key keeps the default written in StaminaComponent.h
			stamina.maxStamina               = staminaData.value(SceneKeys::MaxStamina,               stamina.maxStamina);
			stamina.regenerationPerSecond    = staminaData.value(SceneKeys::RegenerationPerSecond,    stamina.regenerationPerSecond);
			stamina.regenerationDelaySeconds = staminaData.value(SceneKeys::RegenerationDelaySeconds, stamina.regenerationDelaySeconds);
			stamina.rollCost                 = staminaData.value(SceneKeys::RollCost,                 stamina.rollCost);
			stamina.backstepCost             = staminaData.value(SceneKeys::BackstepCost,             stamina.backstepCost);
			stamina.lightAttackCost          = staminaData.value(SceneKeys::LightAttackCost,          stamina.lightAttackCost);
			stamina.heavyAttackCost          = staminaData.value(SceneKeys::HeavyAttackCost,          stamina.heavyAttackCost);
			stamina.runCostPerSecond         = staminaData.value(SceneKeys::RunCostPerSecond,         stamina.runCostPerSecond);
			stamina.currentStamina           = stamina.maxStamina;
		}

		if (entityJson.contains(SceneKeys::HitboxComponent))
		{
			const auto& hitboxData = entityJson[SceneKeys::HitboxComponent];
			auto& hitbox = entityManager.AddComponent<HitboxComponent>(entity);

			hitbox.damage     = hitboxData.value(SceneKeys::Damage,     hitbox.damage);
			hitbox.showGizmos = hitboxData.value(SceneKeys::ShowGizmos, hitbox.showGizmos);
			hitbox.layer      = static_cast<CollisionLayer>(hitboxData.value(SceneKeys::Layer, static_cast<unsigned int>(hitbox.layer)));

			if (hitboxData.contains(SceneKeys::LocalOffset))
				hitbox.localOffset = DeserializeVec3(hitboxData[SceneKeys::LocalOffset]);

			if (hitboxData.contains(SceneKeys::HalfExtents))
				hitbox.halfExtents = DeserializeVec3(hitboxData[SceneKeys::HalfExtents]);
		}

		if (entityJson.contains(SceneKeys::EnemyComponent))
		{
			const auto& enemyData = entityJson[SceneKeys::EnemyComponent];
			auto& enemy = entityManager.AddComponent<EnemyComponent>(entity);

			// A missing key keeps the default written in EnemyComponent.h
			enemy.sightDistance     = enemyData.value(SceneKeys::SightDistance,     enemy.sightDistance);
			enemy.sightAngleDegrees = enemyData.value(SceneKeys::SightAngleDegrees, enemy.sightAngleDegrees);
			enemy.hearingDistance   = enemyData.value(SceneKeys::HearingDistance,   enemy.hearingDistance);
			enemy.eyeHeight         = enemyData.value(SceneKeys::EyeHeight,         enemy.eyeHeight);
		}

		if (entityJson.contains(SceneKeys::MeleeEnemyComponent))
		{
			const auto& meleeData = entityJson[SceneKeys::MeleeEnemyComponent];
			auto& meleeEnemy = entityManager.AddComponent<MeleeEnemyComponent>(entity);

			// A missing key keeps the default written in MeleeEnemyComponent.h
			meleeEnemy.attackDistance        = meleeData.value(SceneKeys::AttackDistance,        meleeEnemy.attackDistance);
			meleeEnemy.runDistance           = meleeData.value(SceneKeys::RunDistance,           meleeEnemy.runDistance);
			meleeEnemy.giveUpDistance        = meleeData.value(SceneKeys::GiveUpDistance,        meleeEnemy.giveUpDistance);
			meleeEnemy.attackCooldownSeconds = meleeData.value(SceneKeys::AttackCooldownSeconds, meleeEnemy.attackCooldownSeconds);
			meleeEnemy.heavyAttackChance     = meleeData.value(SceneKeys::HeavyAttackChance,     meleeEnemy.heavyAttackChance);
			meleeEnemy.maxLightCombo         = meleeData.value(SceneKeys::MaxLightCombo,         meleeEnemy.maxLightCombo);
			meleeEnemy.recoverSeconds        = meleeData.value(SceneKeys::RecoverSeconds,        meleeEnemy.recoverSeconds);
		}

	}

	DeserializeParents(entityManager, entitiesJson, entityAtSavedIndex);
	
	SUNTA_ENGINE_LOG_INFO("SceneSerializer::Deserialize: Successfully loaded Scene '{0}' from '{1}'", scene.GetName(), filepath);
	return true;

}

}