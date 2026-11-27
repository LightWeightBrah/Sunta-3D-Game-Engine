#include "Core/SuntaPreCompiled.h"
#include "SceneSerializer.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Scene/Scene.h"
#include "ECS/EntityManager.h"
#include "Scene/EntityHierarchy.h"
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
	constexpr const char* StrikeAimComponent        = "strike_aim";
	constexpr const char* StrikeAimSpineBones       = "spine_bones";
	constexpr const char* StrikeAimClipHeight       = "clip_strike_height";
	constexpr const char* StrikeAimTargetHeight     = "target_height";
	constexpr const char* StrikeAimMaxPitch         = "max_pitch_degrees";
	constexpr const char* StrikeAimMaxYaw           = "max_yaw_degrees";
	constexpr const char* StrikeAimFadeSeconds      = "fade_seconds";
	constexpr const char* StrikeAimSearchDistance   = "search_distance";
	constexpr const char* LockOnTargetComponent     = "lock_on_target";
	constexpr const char* CharacterActionsComponent = "character_actions";
	constexpr const char* HealthComponent           = "health";
	constexpr const char* StaminaComponent          = "stamina";
	constexpr const char* HitboxComponent           = "hitbox";
	constexpr const char* ShieldComponent           = "shield";
	constexpr const char* PoiseComponent            = "poise";
	constexpr const char* PostureComponent          = "posture";
	constexpr const char* WrathComponent            = "wrath";
	constexpr const char* SummonComponent           = "summon";
	constexpr const char* SummonAllies              = "allies";
	constexpr const char* SummonMaxCharges          = "max_charges";
	constexpr const char* SummonLifetime            = "lifetime_seconds";
	constexpr const char* SummonEffect              = "summon_effect";
	constexpr const char* SummonCalledText          = "called_text";
	constexpr const char* MaxWrath                  = "max_wrath";
	constexpr const char* WrathFromDamageDealt      = "from_damage_dealt";
	constexpr const char* WrathFromDamageTaken      = "from_damage_taken";
	constexpr const char* WrathFromDeflects         = "from_deflects";
	constexpr const char* WrathDrainPerSecond       = "drain_per_second";
	constexpr const char* WrathDrainDelay           = "drain_delay_seconds";
	constexpr const char* WrathDuration             = "duration_seconds";
	constexpr const char* WrathDamageMultiplier     = "damage_multiplier";
	constexpr const char* WrathPostureMultiplier    = "posture_multiplier";
	constexpr const char* WrathGlowColor            = "glow_color";
	constexpr const char* WrathReleaseEffect        = "release_effect";
	constexpr const char* MountComponent            = "mount";
	constexpr const char* PickupComponent           = "pickup";
	constexpr const char* TerrainComponent          = "terrain";
	constexpr const char* Heightmap                 = "heightmap";
	constexpr const char* TerrainSizeX              = "size_x";
	constexpr const char* TerrainSizeZ              = "size_z";
	constexpr const char* HeightScale               = "height_scale";
	constexpr const char* InventoryComponent        = "inventory";
	constexpr const char* LootDropComponent         = "loot_drop";
	constexpr const char* ItemId                    = "item_id";
	constexpr const char* PickupPrompt              = "prompt";
	constexpr const char* PickupObtainedText        = "obtained_text";
	constexpr const char* InteractDistancePickup    = "interact_distance";
	constexpr const char* ReadText                  = "read_text";
	constexpr const char* VoiceSound                = "voice_sound";
	constexpr const char* ReadSeconds               = "read_seconds";
	constexpr const char* PickupSound               = "pickup_sound";
	constexpr const char* PickupTemplate            = "pickup_template";
	constexpr const char* LootChance                = "chance";
	constexpr const char* OnlyOnce                  = "only_once";
	constexpr const char* SeatOffset                = "seat_offset";
	constexpr const char* MountDistance             = "mount_distance";
	constexpr const char* MountPrompt               = "mount_prompt";
	constexpr const char* DismountPrompt            = "dismount_prompt";
	constexpr const char* RiderIdleAnimation        = "rider_idle_animation";
	constexpr const char* RiderWalkAnimation        = "rider_walk_animation";
	constexpr const char* RiderRunAnimation         = "rider_run_animation";
	constexpr const char* RunAnimationFromSpeed     = "run_animation_from_speed";
	constexpr const char* CameraRise                = "camera_rise";
	constexpr const char* CameraPullBack            = "camera_pull_back";
	constexpr const char* ParticleEmitterComponent  = "particle_emitter";
	constexpr const char* GlowSpriteComponent       = "glow_sprite";
	constexpr const char* LiquidOrbComponent        = "liquid_orb";
	constexpr const char* SwarmComponent            = "swarm";
	constexpr const char* FoliageComponent          = "foliage";
	constexpr const char* FoliageModel              = "model_name";
	constexpr const char* FoliageCount              = "count";
	constexpr const char* FoliageAreaX              = "area_size_x";
	constexpr const char* FoliageAreaZ              = "area_size_z";
	constexpr const char* FoliageMask               = "density_mask";
	constexpr const char* FoliageMinScale           = "min_scale";
	constexpr const char* FoliageMaxScale           = "max_scale";
	constexpr const char* FoliageSway               = "sway";
	constexpr const char* FoliageSwaySpeed          = "sway_speed";
	constexpr const char* FoliageDrawDistance       = "draw_distance";
	constexpr const char* FoliageTint               = "tint";
	constexpr const char* FoliageTintVariation      = "tint_variation";
	constexpr const char* FoliageSeed               = "seed";
	constexpr const char* SwarmCount                = "count";
	constexpr const char* SwarmSeed                 = "seed";
	constexpr const char* SwarmPieceSize            = "piece_size";
	constexpr const char* SwarmSizeVariation        = "size_variation";
	constexpr const char* SwarmRadius               = "radius";
	constexpr const char* SwarmShellThickness       = "shell_thickness";
	constexpr const char* SwarmSpin                 = "spin_degrees_per_second";
	constexpr const char* SwarmTumble               = "tumble_degrees_per_second";
	constexpr const char* SwarmSkyCenter            = "sky_center";
	constexpr const char* SwarmSkyHalfExtents       = "sky_half_extents";
	constexpr const char* SwarmFallSpeed            = "fall_speed";
	constexpr const char* SwarmFallSpread           = "fall_spread_seconds";
	constexpr const char* SwarmGroundHeight         = "ground_height";
	constexpr const char* SwarmStuckSeconds         = "stuck_seconds";
	constexpr const char* SwarmGlowColor            = "glow_color";
	constexpr const char* SwarmTrailLength          = "trail_length";
	constexpr const char* SwarmHitEffect            = "hit_effect";
	constexpr const char* SwarmHitBurst             = "hit_burst";
	constexpr const char* SwarmModelName            = "model_name";
	constexpr const char* IsDark                    = "is_dark";
	constexpr const char* ParticleStyle             = "style";
	constexpr const char* SpawnShape                = "spawn_shape";
	constexpr const char* SpawnRadius               = "spawn_radius";
	constexpr const char* OutwardSpeed              = "outward_speed";
	constexpr const char* SizeRandomness            = "size_randomness";
	constexpr const char* ParticleSpin              = "spin_degrees_per_second";
	constexpr const char* SpriteColor               = "color";
	constexpr const char* SpriteSize                = "size";
	constexpr const char* OrbTint                   = "tint";
	constexpr const char* OrbBrightness             = "brightness";
	constexpr const char* OrbWobble                 = "wobble";
	constexpr const char* OrbWobbleSpeed            = "wobble_speed";
	constexpr const char* OrbClearness              = "clearness";
	constexpr const char* LaunchDelaySeconds        = "launch_delay_seconds";
	constexpr const char* BodyFacesFlight           = "body_faces_flight";
	constexpr const char* SpinsAroundFlight         = "spins_around_flight";
	constexpr const char* FormSeconds               = "form_seconds";
	constexpr const char* AimsAfterForming          = "aims_after_forming";
	constexpr const char* WeaponTrailComponent      = "weapon_trail";
	constexpr const char* EnvironmentComponent      = "environment";
	constexpr const char* CutsceneComponent         = "cutscene";
	constexpr const char* AudioSourceComponent      = "audio_source";
	constexpr const char* CharacterSoundsComponent  = "character_sounds";
	constexpr const char* HealingFlaskComponent     = "healing_flask";
	constexpr const char* CurrencyComponent         = "currency";
	constexpr const char* CurrencyRewardComponent   = "currency_reward";
	constexpr const char* LevelUpComponent          = "level_up";
	constexpr const char* CastsShadows              = "casts_shadows";
	constexpr const char* IsPostProcessingEnabled   = "is_post_processing_enabled";
	constexpr const char* Exposure                  = "exposure";
	constexpr const char* IsBloomEnabled            = "is_bloom_enabled";
	constexpr const char* BloomThreshold            = "bloom_threshold";
	constexpr const char* BloomIntensity            = "bloom_intensity";
	constexpr const char* BloomBlurPasses           = "bloom_blur_passes";
	constexpr const char* HitImpactFrame            = "hit_impact_frame";
	constexpr const char* PixelSize                 = "pixel_size";
	constexpr const char* PaletteLevels             = "palette_levels";
	constexpr const char* DitherStrength            = "dither_strength";
	constexpr const char* OutlineStrength           = "outline_strength";
	constexpr const char* OutlineColor              = "outline_color";
	constexpr const char* HueShift                  = "hue_shift";
	constexpr const char* ShadowHue                 = "shadow_hue";
	constexpr const char* LightHue                  = "light_hue";
	constexpr const char* CleanLines                = "clean_lines";
	constexpr const char* RemoveOrphans             = "remove_orphans";
	constexpr const char* PaintRadius               = "paint_radius";
	constexpr const char* Saturation                = "saturation";
	constexpr const char* Contrast                  = "contrast";
	constexpr const char* ColorTint                 = "color_tint";
	constexpr const char* Vignette                  = "vignette";
	constexpr const char* ParticleDrag              = "drag";
	constexpr const char* ParticleStretch           = "stretch";
	constexpr const char* HasMiddleColor            = "has_middle_color";
	constexpr const char* MiddleColor               = "middle_color";
	constexpr const char* BurstAmount               = "burst_amount";
	constexpr const char* FlipbookTexture           = "flipbook_texture";
	constexpr const char* FlipbookColumns           = "flipbook_columns";
	constexpr const char* FlipbookRows              = "flipbook_rows";
	constexpr const char* FlipbookLoops             = "flipbook_loops";
	constexpr const char* FlipbookRandomStart       = "flipbook_random_start";
	constexpr const char* EffectShapeComponent      = "effect_shape";
	constexpr const char* ShapeKind                 = "kind";
	constexpr const char* ShapeLifetime             = "lifetime_seconds";
	constexpr const char* ShapeStartRadius          = "start_radius";
	constexpr const char* ShapeEndRadius            = "end_radius";
	constexpr const char* ShapeThickness            = "thickness";
	constexpr const char* ShapeHeight               = "height";
	constexpr const char* ShapeArcDegrees           = "arc_degrees";
	constexpr const char* ShapeRotation             = "rotation_degrees";
	constexpr const char* ShapeRotationRandomness   = "rotation_randomness";
	constexpr const char* ShapeOffset               = "offset";
	constexpr const char* ShapeSpin                 = "spin_degrees_per_second";
	constexpr const char* ShapeTwist                = "twist_degrees";
	constexpr const char* ShapeCoreColor            = "core_color";
	constexpr const char* ShapeEdgeColor            = "edge_color";
	constexpr const char* ShapeIntensity            = "intensity";
	constexpr const char* ShapeNoiseScale           = "noise_scale";
	constexpr const char* ShapeScrollSpeed          = "scroll_speed";
	constexpr const char* ShapeDissolve             = "dissolve";
	constexpr const char* ShapeLightIntensity       = "light_intensity";
	constexpr const char* ShapeLightRadius          = "light_radius";
	constexpr const char* ShapeLightColor           = "light_color";
	constexpr const char* ShadowDistance            = "shadow_distance";
	constexpr const char* ShadowStrength            = "shadow_strength";
	constexpr const char* ShadowSoftness            = "shadow_softness";
	constexpr const char* ShadowResolution          = "shadow_resolution";
	constexpr const char* BossComponent             = "boss";
	constexpr const char* FogGateComponent          = "fog_gate";
	constexpr const char* EnemyComponent            = "enemy";
	constexpr const char* MeleeEnemyComponent       = "melee_enemy";
	constexpr const char* SpellCasterComponent      = "spell_caster";
	constexpr const char* ProjectileComponent       = "projectile";

	constexpr const char* CastAnimation             = "cast_animation";
	constexpr const char* ProjectileName            = "projectile";
	constexpr const char* ProjectileCount           = "projectile_count";
	constexpr const char* SpreadDegrees             = "spread_degrees";
	constexpr const char* ReleaseAt                 = "release_at";
	constexpr const char* AtTargetsFeet             = "at_targets_feet";
	constexpr const char* DropHeight                = "drop_height";
	constexpr const char* SpellCooldownSeconds      = "cooldown_seconds";
	constexpr const char* ChargeEffect              = "charge_effect";
	constexpr const char* ChargeParticlesPerSecond  = "charge_particles_per_second";
	constexpr const char* CastSound                 = "cast_sound";
	constexpr const char* CastKey                   = "cast_key";
	constexpr const char* IsCastOnRequestOnly       = "cast_on_request_only";
	constexpr const char* IsUninterruptible         = "is_uninterruptible";
	constexpr const char* StartsDormant             = "starts_dormant";
	constexpr const char* IsAlly                    = "is_ally";
	constexpr const char* ProjectileSpeed           = "speed";
	constexpr const char* ProjectileRadius          = "radius";
	constexpr const char* HomingDegreesPerSecond    = "homing_degrees_per_second";
	constexpr const char* FuseSeconds               = "fuse_seconds";
	constexpr const char* TrailEffect               = "trail_effect";
	constexpr const char* TrailParticlesPerSecond   = "trail_particles_per_second";
	constexpr const char* SpinDegreesPerSecond      = "spin_degrees_per_second";
	constexpr const char* HitBurst                  = "hit_burst";
	constexpr const char* HitSound                  = "hit_sound";
	constexpr const char* PlayerHUDComponent        = "player_hud";
	constexpr const char* BonfireComponent          = "bonfire";
	constexpr const char* RespawnComponent          = "respawn";
	constexpr const char* EnemyHealthBarComponent   = "enemy_health_bar";
	constexpr const char* ParentComponent           = "parent";

	constexpr const char* Name                      = "name";
	constexpr const char* Position                  = "position";
	constexpr const char* Rotation                  = "rotation";
	constexpr const char* Scale                     = "scale";
	constexpr const char* RotationQuaternion        = "rotation_quaternion";
	constexpr const char* ScriptPath                = "script_path";
	constexpr const char* IsVisible                 = "is_visible";
	constexpr const char* EmissiveStrength          = "emissive_strength";
	constexpr const char* FogAmount                 = "fog_amount";
	constexpr const char* MeshName                  = "mesh_name";
	constexpr const char* MaterialName              = "material_name";
	constexpr const char* ModelName                 = "model_name";
	constexpr const char* CurrentAnimationName      = "current_animation_name";

	constexpr const char* ShowGizmos				= "show_gizmos";
	constexpr const char* LocalOffset               = "local_offset";
	constexpr const char* HalfExtents               = "half_extents";
	constexpr const char* IsTrigger                 = "is_trigger";
	constexpr const char* BlocksCamera              = "blocks_camera";
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
	constexpr const char* LockOnSideDegrees         = "lock_on_side_degrees";
	constexpr const char* LockOnSharpness           = "lock_on_sharpness";
	constexpr const char* ShakeDegrees              = "shake_degrees";
	constexpr const char* ShakeFrequency            = "shake_frequency";
	constexpr const char* ShakeFadePerSecond        = "shake_fade_per_second";

	constexpr const char* MaxDistance               = "max_distance";
	constexpr const char* SearchAngleDegrees        = "search_angle_degrees";
	constexpr const char* EyeHeight                 = "eye_height";
	constexpr const char* SwitchMouseDistance       = "switch_mouse_distance";
	constexpr const char* RequireLineOfSight        = "require_line_of_sight";
	constexpr const char* ShowSightRays             = "show_sight_rays";

	constexpr const char* IdleAnimation             = "idle_animation";
	constexpr const char* WalkAnimation             = "walk_animation";
	constexpr const char* RunAnimation              = "run_animation";
	constexpr const char* WalkBackAnimation         = "walk_back_animation";
	constexpr const char* StrafeLeftAnimation       = "strafe_left_animation";
	constexpr const char* StrafeRightAnimation      = "strafe_right_animation";
	constexpr const char* BlendSeconds              = "blend_seconds";
	constexpr const char* WalkAnimationSpeed        = "walk_animation_speed";
	constexpr const char* RunAnimationSpeed         = "run_animation_speed";

	constexpr const char* IsLockable                = "is_lockable";
	constexpr const char* AimHeight                 = "aim_height";
	constexpr const char* AimBone                   = "aim_bone";

	constexpr const char* RollAnimation             = "roll_animation";
	constexpr const char* BackstepAnimation         = "backstep_animation";
	constexpr const char* LightAttack1Animation     = "light_attack_1_animation";
	constexpr const char* LightAttack2Animation     = "light_attack_2_animation";
	constexpr const char* LightAttack3Animation     = "light_attack_3_animation";
	constexpr const char* HeavyAttackAnimation      = "heavy_attack_animation";
	constexpr const char* LeapAttackAnimation       = "leap_attack_animation";
	constexpr const char* LeapSpeed                 = "leap_speed";
	constexpr const char* LeapHitFrom               = "leap_hit_from";
	constexpr const char* LeapHitTo                 = "leap_hit_to";
	constexpr const char* LeapAttackDistance        = "leap_attack_distance";
	constexpr const char* RollSpeed                 = "roll_speed";
	constexpr const char* BackstepSpeed             = "backstep_speed";
	constexpr const char* AttackStepSpeed           = "attack_step_speed";
	constexpr const char* InvulnerableFrom          = "invulnerable_from";
	constexpr const char* InvulnerableTo            = "invulnerable_to";
	constexpr const char* InputBufferFrom           = "input_buffer_from";
	constexpr const char* CancelFrom                = "cancel_from";
	constexpr const char* HitReactionAnimation      = "hit_reaction_animation";
	constexpr const char* DeathAnimation            = "death_animation";
	constexpr const char* RestAnimation             = "rest_animation";
	constexpr const char* StandUpAnimation          = "stand_up_animation";
	constexpr const char* BlockAnimation            = "block_animation";
	constexpr const char* BlockImpactAnimation      = "block_impact_animation";
	constexpr const char* ParryAnimation            = "parry_animation";
	constexpr const char* ParriedAnimation          = "parried_animation";
	constexpr const char* JumpAnimation             = "jump_animation";
	constexpr const char* JumpAttackAnimation       = "jump_attack_animation";
	constexpr const char* JumpHeavyAttackAnimation  = "jump_heavy_attack_animation";
	constexpr const char* JumpSpeed                 = "jump_speed";
	constexpr const char* JumpMoveSpeed             = "jump_move_speed";
	constexpr const char* JumpPlungeSpeed           = "jump_plunge_speed";
	constexpr const char* JumpPlungeFrom            = "jump_plunge_from";
	constexpr const char* JumpAttackDamageMultiplier = "jump_attack_damage_multiplier";
	constexpr const char* JumpPlungeShockwaveRadius = "jump_plunge_shockwave_radius";
	constexpr const char* JumpPlungeShockwaveDamage = "jump_plunge_shockwave_damage";
	constexpr const char* JumpPlungeEffect          = "jump_plunge_effect";
	constexpr const char* UpperBodyBone             = "upper_body_bone";
	constexpr const char* ShockwaveRadius           = "shockwave_radius";
	constexpr const char* ShockwaveDamage           = "shockwave_damage";
	constexpr const char* ShockwaveDistance         = "shockwave_distance";
	constexpr const char* ShockwaveEffect           = "shockwave_effect";
	constexpr const char* SwingEffect               = "swing_effect";
	constexpr const char* HyperArmor                = "hyper_armor";
	constexpr const char* HeavySwingEffect          = "heavy_swing_effect";
	constexpr const char* SwingEffectHeight         = "swing_effect_height";
	constexpr const char* SwingEffectDistance       = "swing_effect_distance";
	constexpr const char* SwingEffectAt             = "swing_effect_at";
	constexpr const char* ShockwaveCameraShake      = "shockwave_camera_shake";
	constexpr const char* AttackTurnDegreesPerSecond = "attack_turn_degrees_per_second";
	constexpr const char* AttackLungeSpeed          = "attack_lunge_speed";
	constexpr const char* AttackReach               = "attack_reach";
	constexpr const char* FinisherKnockback         = "finisher_knockback";
	constexpr const char* HeavyKnockback            = "heavy_knockback";
	constexpr const char* KnockdownAnimation        = "knockdown_animation";
	constexpr const char* HeavyKnockbackChance      = "heavy_knockback_chance";
	constexpr const char* GetUpAnimation            = "get_up_animation";
	constexpr const char* KnockdownChance           = "knockdown_chance";
	constexpr const char* KnockdownSeconds          = "knockdown_seconds";
	constexpr const char* AttackDelayChance         = "attack_delay_chance";
	constexpr const char* AttackDelaySeconds        = "attack_delay_seconds";
	constexpr const char* AttackDelayAt             = "attack_delay_at";
	constexpr const char* FinisherChance            = "finisher_chance";
	constexpr const char* DodgeChance               = "dodge_chance";
	constexpr const char* DodgeDistance             = "dodge_distance";
	constexpr const char* DodgeCooldownSeconds      = "dodge_cooldown_seconds";
	constexpr const char* GuardChance               = "guard_chance";
	constexpr const char* GuardSeconds              = "guard_seconds";
	constexpr const char* WeaponDamageTaken         = "weapon_damage_taken";
	constexpr const char* SpellDamageTaken          = "spell_damage_taken";
	constexpr const char* SpellImmunitySeconds      = "spell_immunity_seconds";
	constexpr const char* RallyWindowSeconds        = "rally_window_seconds";
	constexpr const char* RallyPerDamage            = "rally_per_damage";
	constexpr const char* HitboxIsEnabled           = "is_enabled";
	constexpr const char* HitReactionDodgeFrom      = "hit_reaction_dodge_from";
	constexpr const char* FootstepCameraShake       = "footstep_camera_shake";
	constexpr const char* FootstepShakeRadius       = "footstep_shake_radius";
	constexpr const char* PhaseTwoLight1Animation   = "phase_two_light_attack_1_animation";
	constexpr const char* PhaseTwoLight2Animation   = "phase_two_light_attack_2_animation";
	constexpr const char* PhaseTwoLight3Animation   = "phase_two_light_attack_3_animation";
	constexpr const char* PhaseTwoHeavyAnimation    = "phase_two_heavy_attack_animation";
	constexpr const char* PhaseTwoLightHitFrom      = "phase_two_light_hit_from";
	constexpr const char* PhaseTwoLightHitTo        = "phase_two_light_hit_to";
	constexpr const char* PhaseTwoHeavyHitFrom      = "phase_two_heavy_hit_from";
	constexpr const char* PhaseTwoHeavyHitTo        = "phase_two_heavy_hit_to";
	constexpr const char* PhaseTwoShockwaveRadius   = "phase_two_shockwave_radius";
	constexpr const char* PhaseTwoMaxLightCombo     = "phase_two_max_light_combo";
	constexpr const char* ParryFrom                 = "parry_from";
	constexpr const char* ParryTo                   = "parry_to";
	constexpr const char* DeflectSeconds            = "deflect_seconds";
	constexpr const char* RiposteDamageMultiplier   = "riposte_damage_multiplier";
	constexpr const char* ExtraLightAttackAnimations = "extra_light_attack_animations";
	constexpr const char* HeavyAttack2Animation     = "heavy_attack_2_animation";
	constexpr const char* RunningAttackAnimation    = "running_attack_animation";
	constexpr const char* RunningAttackSpeed        = "running_attack_speed";
	constexpr const char* DodgeAttackAnimation      = "dodge_attack_animation";
	constexpr const char* RiposteAnimation          = "riposte_animation";
	constexpr const char* RiposteReach              = "riposte_reach";
	constexpr const char* AttackLungeMaxDistance    = "attack_lunge_max_distance";
	constexpr const char* DodgeCancelFrom           = "dodge_cancel_from";
	constexpr const char* LockedOnIdleAnimation     = "locked_on_idle_animation";
	constexpr const char* AttackMoves               = "attack_moves";
	constexpr const char* MoveAnimation             = "animation";
	constexpr const char* MoveHitFrom               = "hit_from";
	constexpr const char* MoveHitTo                 = "hit_to";
	constexpr const char* MoveDamageMultiplier      = "damage_multiplier";
	constexpr const char* MoveCancelFrom            = "cancel_from";
	constexpr const char* MoveStrikeHeight          = "strike_height";
	constexpr const char* ExtraHitReactionAnimations = "extra_hit_reaction_animations";
	constexpr const char* HeavyHitReactionAnimation = "heavy_hit_reaction_animation";
	constexpr const char* KnockbackAnimation        = "knockback_animation";
	constexpr const char* KnockbackSeconds          = "knockback_seconds";
	constexpr const char* KnockbackDust             = "knockback_dust";
	constexpr const char* Knockback                 = "knockback";
	constexpr const char* LightHitFrom              = "light_hit_from";
	constexpr const char* LightHitTo                = "light_hit_to";
	constexpr const char* HeavyHitFrom              = "heavy_hit_from";
	constexpr const char* HeavyHitTo                = "heavy_hit_to";
	constexpr const char* HeavyDamageMultiplier     = "heavy_damage_multiplier";
	constexpr const char* AttackSpeed               = "attack_speed";

	constexpr const char* MaxHealth                 = "max_health";
	constexpr const char* HitEffect                 = "hit_effect";
	constexpr const char* DeflectEffect             = "deflect_effect";
	constexpr const char* DeathSlowMotionSeconds    = "death_slow_motion_seconds";
	constexpr const char* DeathSlowMotionScale      = "death_slow_motion_scale";

	constexpr const char* MaxStamina                = "max_stamina";
	constexpr const char* RegenerationPerSecond     = "regeneration_per_second";
	constexpr const char* RegenerationDelaySeconds  = "regeneration_delay_seconds";
	constexpr const char* RollCost                  = "roll_cost";
	constexpr const char* BackstepCost              = "backstep_cost";
	constexpr const char* LightAttackCost           = "light_attack_cost";
	constexpr const char* HeavyAttackCost           = "heavy_attack_cost";
	constexpr const char* ParryCost                 = "parry_cost";
	constexpr const char* RunCostPerSecond          = "run_cost_per_second";

	constexpr const char* Damage                    = "damage";
	constexpr const char* PoiseDamage               = "poise_damage";
	constexpr const char* HitStopSeconds            = "hit_stop_seconds";
	constexpr const char* CameraShake               = "camera_shake";

	constexpr const char* GuardAngleDegrees         = "guard_angle_degrees";
	constexpr const char* DamageThrough             = "damage_through";
	constexpr const char* StaminaPerDamage          = "stamina_per_damage";
	constexpr const char* BlockEffect               = "block_effect";

	constexpr const char* PhaseTwoHealthFraction    = "phase_two_health_fraction";
	constexpr const char* PhaseTwoDamageMultiplier  = "phase_two_damage_multiplier";
	constexpr const char* PhaseTwoAttackSpeedup     = "phase_two_attack_speedup";
	constexpr const char* PhaseTwoAnimation         = "phase_two_animation";
	constexpr const char* PhaseTwoGlow              = "phase_two_glow";
	constexpr const char* PhaseTwoGrowth            = "phase_two_growth";
	constexpr const char* PhaseTwoSound             = "phase_two_sound";
	constexpr const char* PhaseTwoMusic             = "phase_two_music";
	constexpr const char* DefeatCutscene            = "defeat_cutscene";
	constexpr const char* DefeatCutsceneDelay       = "defeat_cutscene_delay";

	constexpr const char* Sound                     = "sound";
	constexpr const char* SoundVolume               = "volume";
	constexpr const char* IsLooping                 = "is_looping";
	constexpr const char* PlayOnStart               = "play_on_start";
	constexpr const char* IsSpatial                 = "is_spatial";
	constexpr const char* MinDistance               = "min_distance";

	constexpr const char* AttackSound               = "attack_sound";
	constexpr const char* HurtSound                 = "hurt_sound";
	constexpr const char* BlockSound                = "block_sound";
	constexpr const char* ParrySound                = "parry_sound";
	constexpr const char* DeathSound                = "death_sound";
	constexpr const char* RollSound                 = "roll_sound";
	constexpr const char* FootstepSound             = "footstep_sound";
	constexpr const char* ShockwaveSound            = "shockwave_sound";
	constexpr const char* PainVoice                 = "pain_voice";
	constexpr const char* AttackVoice               = "attack_voice";
	constexpr const char* DeathVoice                = "death_voice";
	constexpr const char* AlertVoice                = "alert_voice";
	constexpr const char* PainVoiceChance           = "pain_voice_chance";
	constexpr const char* AttackVoiceChance         = "attack_voice_chance";
	constexpr const char* FootstepMeters            = "footstep_meters";

	constexpr const char* MaxCharges                = "max_charges";
	constexpr const char* HealAmount                = "heal_amount";
	constexpr const char* DrinkAnimation            = "drink_animation";
	constexpr const char* HealAtProgress            = "heal_at_progress";
	constexpr const char* HealEffect                = "heal_effect";
	constexpr const char* HiddenWhileDrinking       = "hidden_while_drinking";
	constexpr const char* ShownWhileDrinking        = "shown_while_drinking";
	constexpr const char* DrinkSound                = "drink_sound";
	constexpr const char* HealSound                 = "heal_sound";

	constexpr const char* CurrencyAmount            = "amount";
	constexpr const char* IsDroppedOnDeath          = "is_dropped_on_death";
	constexpr const char* DropMarker                = "drop_marker";
	constexpr const char* RetrieveDistance          = "retrieve_distance";
	constexpr const char* IsRetrievedWithKey        = "is_retrieved_with_key";
	constexpr const char* RetrievePrompt            = "retrieve_prompt";
	constexpr const char* GainSound                 = "gain_sound";
	constexpr const char* RetrieveSound             = "retrieve_sound";
	constexpr const char* Level                     = "level";
	constexpr const char* FirstLevelCost            = "first_level_cost";
	constexpr const char* CostGrowth                = "cost_growth";
	constexpr const char* StartingAttributeValue    = "starting_attribute_value";
	constexpr const char* HealthAttribute           = "health_attribute";
	constexpr const char* HealthPerPoint            = "health_per_point";
	constexpr const char* HealthPoints              = "health_points";
	constexpr const char* StaminaAttribute          = "stamina_attribute";
	constexpr const char* StaminaPerPoint           = "stamina_per_point";
	constexpr const char* StaminaPoints             = "stamina_points";
	constexpr const char* DamageAttribute           = "damage_attribute";
	constexpr const char* DamagePerPoint            = "damage_per_point";
	constexpr const char* DamagePoints              = "damage_points";
	constexpr const char* LevelUpSound              = "level_up_sound";
	constexpr const char* LevelUpEffect             = "level_up_effect";
	constexpr const char* AttackSoundLeadSeconds    = "attack_sound_lead_seconds";
	constexpr const char* LeftFootBone              = "left_foot_bone";
	constexpr const char* RightFootBone             = "right_foot_bone";

	constexpr const char* IsEmitting                = "is_emitting";
	constexpr const char* ParticlesPerSecond        = "particles_per_second";
	constexpr const char* LifetimeSeconds           = "lifetime_seconds";
	constexpr const char* SpawnHalfExtents          = "spawn_half_extents";
	constexpr const char* ParticleVelocity          = "velocity";
	constexpr const char* VelocityRandomness        = "velocity_randomness";
	constexpr const char* ParticleGravity           = "gravity";
	constexpr const char* StartSize                 = "start_size";
	constexpr const char* EndSize                   = "end_size";
	constexpr const char* StartColor                = "start_color";
	constexpr const char* EndColor                  = "end_color";
	constexpr const char* ParticleIntensity         = "intensity";
	constexpr const char* BaseOffset                = "base_offset";
	constexpr const char* TipOffset                 = "tip_offset";
	constexpr const char* OnlyWhileStriking         = "only_while_striking";
	constexpr const char* MaxParticles              = "max_particles";

	constexpr const char* SkyboxFolder              = "skybox_folder";
	constexpr const char* SkyBrightness             = "sky_brightness";
	constexpr const char* IsFogEnabled              = "is_fog_enabled";
	constexpr const char* FogColor                  = "fog_color";
	constexpr const char* FogDensity                = "fog_density";

	constexpr const char* ArenaCenter               = "arena_center";
	constexpr const char* ArenaHalfExtents          = "arena_half_extents";
	constexpr const char* VictoryMessage            = "victory_message";
	constexpr const char* IsCrossedWithKey          = "is_crossed_with_key";
	constexpr const char* FogCutscene               = "cutscene";
	constexpr const char* BattleMusic               = "battle_music";
	constexpr const char* BattleMusicVolume         = "battle_music_volume";
	constexpr const char* VictorySound              = "victory_sound";
	constexpr const char* DurationSeconds           = "duration_seconds";
	constexpr const char* LookAtEntity              = "look_at_entity";
	constexpr const char* LookAtHeight              = "look_at_height";
	constexpr const char* LookAlsoAtEntity          = "look_also_at_entity";
	constexpr const char* AnimationEntity           = "animation_entity";
	constexpr const char* CutsceneAnimationName     = "animation_name";
	constexpr const char* AnimationAtSeconds        = "animation_at_seconds";
	constexpr const char* CutsceneTitle             = "title";
	constexpr const char* TitleSeconds              = "title_seconds";
	constexpr const char* CanBeSkipped              = "can_be_skipped";
	constexpr const char* VideoFile                 = "video_file";
	constexpr const char* WaitForKey                = "wait_for_key";
	constexpr const char* WaitPrompt                = "wait_prompt";
	constexpr const char* WaitPresses               = "wait_presses";
	constexpr const char* PressSound                = "press_sound";
	constexpr const char* PressWindowSeconds        = "press_window_seconds";
	constexpr const char* BlendInSeconds            = "blend_in_seconds";
	constexpr const char* FadeSeconds               = "fade_seconds";
	constexpr const char* CutsceneSound             = "cutscene_sound";

	constexpr const char* MaxPoise                  = "max_poise";
	constexpr const char* MaxPosture                = "max_posture";
	constexpr const char* PostureFromHits           = "from_hits";
	constexpr const char* PostureFromBlocks         = "from_blocks";
	constexpr const char* PostureFromBeingParried   = "from_being_parried";
	constexpr const char* PostureRecoverPerSecond   = "recover_per_second";
	constexpr const char* PostureRecoverDelay       = "recover_delay_seconds";
	constexpr const char* PostureLowHealthRecover   = "low_health_recover_factor";
	constexpr const char* PostureBrokenSeconds      = "broken_seconds";
	constexpr const char* FinisherDamage            = "finisher_damage";
	constexpr const char* FinisherSlowMotion        = "finisher_slow_motion";
	constexpr const char* FinisherSlowSeconds       = "finisher_slow_seconds";
	constexpr const char* PostureBreakEffect        = "break_effect";
	constexpr const char* FinisherEffect            = "finisher_effect";
	constexpr const char* MoveIsPerilous            = "is_perilous";
	constexpr const char* HeavyAttacksArePerilous   = "heavy_attacks_are_perilous";
	constexpr const char* PerilousWarningEffect     = "perilous_warning_effect";
	constexpr const char* RecoverAfterSeconds       = "recover_after_seconds";
	constexpr const char* StaggerImmunitySeconds    = "stagger_immunity_seconds";

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

	constexpr const char* InteractDistance          = "interact_distance";
	constexpr const char* RestPrompt                = "rest_prompt";
	constexpr const char* LeavePrompt               = "leave_prompt";
	constexpr const char* LitMessage                = "lit_message";
	constexpr const char* RespawnOffset             = "respawn_offset";
	constexpr const char* RespawnDelaySeconds       = "respawn_delay_seconds";

	constexpr const char* PixelsPerHealthPoint      = "pixels_per_health_point";
	constexpr const char* PixelsPerStaminaPoint     = "pixels_per_stamina_point";
	constexpr const char* ShowLockOnMarker          = "show_lock_on_marker";
	constexpr const char* DeathMessage              = "death_message";
	constexpr const char* HUDStyleFile              = "style";

	constexpr const char* IsBoss                    = "is_boss";
	constexpr const char* DisplayName               = "display_name";
	constexpr const char* ShowSecondsAfterHit       = "show_seconds_after_hit";
	constexpr const char* HeightAboveFeet           = "height_above_feet";

	// The parent is saved as its place in the list of entities (entity IDs change every time a scene is loaded)
	constexpr const char* ParentIndex               = "index";
	constexpr const char* BoneName                  = "bone_name";
	constexpr const char* BonePositionOnly          = "bone_position_only";

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
			{ SceneKeys::BoneName,    parent.boneName     },
			{ SceneKeys::BonePositionOnly, parent.followsBonePositionOnly }
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
		parent.followsBonePositionOnly = parentData.value(SceneKeys::BonePositionOnly, false);
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

// One entity as it is saved (every component it has)
static json SerializeEntity(EntityManager& entityManager, unsigned int i)
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
			{ SceneKeys::IsVisible,        model->isVisible        },
			{ SceneKeys::ModelName,        model->modelName        },
			{ SceneKeys::EmissiveStrength, model->emissiveStrength },
			{ SceneKeys::FogAmount, model->fogAmount }
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
			{ SceneKeys::BlocksCamera,                    boxCollider->blocksCamera },
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
		json lightJson = SerializeLightColor(directionalLight->color);
		lightJson[SceneKeys::CastsShadows]     = directionalLight->castsShadows;
		lightJson[SceneKeys::ShadowDistance]   = directionalLight->shadowDistance;
		lightJson[SceneKeys::ShadowStrength]   = directionalLight->shadowStrength;
		lightJson[SceneKeys::ShadowSoftness]   = directionalLight->shadowSoftness;
		lightJson[SceneKeys::ShadowResolution] = directionalLight->shadowResolution;

		entityJson[SceneKeys::DirectionalLightComponent] = lightJson;
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
			{ SceneKeys::WalkBackAnimation,    characterAnimation->walkBackAnimation    },
			{ SceneKeys::StrafeLeftAnimation,  characterAnimation->strafeLeftAnimation  },
			{ SceneKeys::StrafeRightAnimation, characterAnimation->strafeRightAnimation },
			{ SceneKeys::LockedOnIdleAnimation, characterAnimation->lockedOnIdleAnimation },
			{ SceneKeys::BlendSeconds,  characterAnimation->blendSeconds  },
			{ SceneKeys::WalkAnimationSpeed, characterAnimation->walkAnimationSpeed },
			{ SceneKeys::RunAnimationSpeed,  characterAnimation->runAnimationSpeed  }
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
			{ SceneKeys::LockOnSideDegrees, followCamera->lockOnSideDegrees },
			{ SceneKeys::LockOnSharpness,  followCamera->lockOnSharpness    },
			{ SceneKeys::ShakeDegrees,       followCamera->shakeDegrees       },
			{ SceneKeys::ShakeFrequency,     followCamera->shakeFrequency     },
			{ SceneKeys::ShakeFadePerSecond, followCamera->shakeFadePerSecond }
		};
	}

	if (auto* aim = entityManager.GetComponent<StrikeAimComponent>(i))
	{
		entityJson[SceneKeys::StrikeAimComponent] =
		{
			{ SceneKeys::StrikeAimSpineBones,     aim->spineBones       },
			{ SceneKeys::StrikeAimClipHeight,     aim->clipStrikeHeight },
			{ SceneKeys::StrikeAimTargetHeight,   aim->targetHeight     },
			{ SceneKeys::StrikeAimMaxPitch,       aim->maxPitchDegrees  },
			{ SceneKeys::StrikeAimMaxYaw,         aim->maxYawDegrees    },
			{ SceneKeys::StrikeAimFadeSeconds,    aim->fadeSeconds      },
			{ SceneKeys::StrikeAimSearchDistance, aim->searchDistance   }
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
			{ SceneKeys::AimHeight,  lockOnTarget->aimHeight  },
			{ SceneKeys::AimBone,    lockOnTarget->aimBone    }
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
			{ SceneKeys::LeapAttackAnimation,   actions->leapAttackAnimation   },
			{ SceneKeys::LeapSpeed,             actions->leapSpeed             },
			{ SceneKeys::LeapHitFrom,           actions->leapHitFrom           },
			{ SceneKeys::LeapHitTo,             actions->leapHitTo             },
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
			{ SceneKeys::RestAnimation,         actions->restAnimation         },
			{ SceneKeys::StandUpAnimation,      actions->standUpAnimation      },
			{ SceneKeys::BlockAnimation,        actions->blockAnimation        },
			{ SceneKeys::BlockImpactAnimation,  actions->blockImpactAnimation  },
			{ SceneKeys::ParryAnimation,        actions->parryAnimation        },
			{ SceneKeys::ParriedAnimation,      actions->parriedAnimation      },
			{ SceneKeys::JumpAnimation,         actions->jumpAnimation         },
			{ SceneKeys::JumpAttackAnimation,   actions->jumpAttackAnimation   },
			{ SceneKeys::JumpHeavyAttackAnimation, actions->jumpHeavyAttackAnimation },
			{ SceneKeys::JumpSpeed,             actions->jumpSpeed             },
			{ SceneKeys::JumpMoveSpeed,         actions->jumpMoveSpeed         },
			{ SceneKeys::JumpPlungeSpeed,       actions->jumpPlungeSpeed       },
			{ SceneKeys::JumpPlungeFrom,        actions->jumpPlungeFrom        },
			{ SceneKeys::JumpAttackDamageMultiplier, actions->jumpAttackDamageMultiplier },
			{ SceneKeys::JumpPlungeShockwaveRadius, actions->jumpPlungeShockwaveRadius },
			{ SceneKeys::JumpPlungeShockwaveDamage, actions->jumpPlungeShockwaveDamage },
			{ SceneKeys::JumpPlungeEffect,      actions->jumpPlungeEffect      },
			{ SceneKeys::UpperBodyBone,         actions->upperBodyBone         },
			{ SceneKeys::ShockwaveRadius,       actions->shockwaveRadius       },
			{ SceneKeys::ShockwaveDamage,       actions->shockwaveDamage       },
			{ SceneKeys::ShockwaveDistance,     actions->shockwaveDistance     },
			{ SceneKeys::ShockwaveEffect,       actions->shockwaveEffect       },
			{ SceneKeys::SwingEffect,           actions->swingEffect           },
			{ SceneKeys::HyperArmor,            actions->hyperArmor            },
			{ SceneKeys::HeavySwingEffect,      actions->heavySwingEffect      },
			{ SceneKeys::SwingEffectHeight,     actions->swingEffectHeight     },
			{ SceneKeys::SwingEffectDistance,   actions->swingEffectDistance   },
			{ SceneKeys::SwingEffectAt,         actions->swingEffectAt         },
			{ SceneKeys::ShockwaveCameraShake,  actions->shockwaveCameraShake  },
			{ SceneKeys::AttackTurnDegreesPerSecond, actions->attackTurnDegreesPerSecond },
			{ SceneKeys::AttackLungeSpeed,      actions->attackLungeSpeed      },
			{ SceneKeys::AttackReach,           actions->attackReach           },
			{ SceneKeys::FinisherKnockback,     actions->finisherKnockback     },
			{ SceneKeys::HeavyKnockback,        actions->heavyKnockback        },
			{ SceneKeys::KnockdownAnimation,    actions->knockdownAnimation    },
			{ SceneKeys::HeavyKnockbackChance,  actions->heavyKnockbackChance  },
			{ SceneKeys::GetUpAnimation,        actions->getUpAnimation        },
			{ SceneKeys::KnockdownChance,       actions->knockdownChance       },
			{ SceneKeys::KnockdownSeconds,      actions->knockdownSeconds      },
			{ SceneKeys::AttackDelayChance,     actions->attackDelayChance     },
			{ SceneKeys::AttackDelaySeconds,    actions->attackDelaySeconds    },
			{ SceneKeys::AttackDelayAt,         actions->attackDelayAt         },
			{ SceneKeys::ParryFrom,             actions->parryFrom             },
			{ SceneKeys::ParryTo,               actions->parryTo               },
			{ SceneKeys::DeflectSeconds,        actions->deflectSeconds        },
			{ SceneKeys::RiposteDamageMultiplier, actions->riposteDamageMultiplier },
			{ SceneKeys::LightHitFrom,          actions->lightHitFrom          },
			{ SceneKeys::LightHitTo,            actions->lightHitTo            },
			{ SceneKeys::HeavyHitFrom,          actions->heavyHitFrom          },
			{ SceneKeys::HeavyHitTo,            actions->heavyHitTo            },
			{ SceneKeys::HeavyDamageMultiplier, actions->heavyDamageMultiplier },
			{ SceneKeys::AttackSpeed,           actions->attackSpeed           },
			{ SceneKeys::ExtraLightAttackAnimations, actions->extraLightAttackAnimations },
			{ SceneKeys::HeavyAttack2Animation,  actions->heavyAttack2Animation  },
			{ SceneKeys::RunningAttackAnimation, actions->runningAttackAnimation },
			{ SceneKeys::RunningAttackSpeed,     actions->runningAttackSpeed     },
			{ SceneKeys::DodgeAttackAnimation,   actions->dodgeAttackAnimation   },
			{ SceneKeys::RiposteAnimation,       actions->riposteAnimation       },
			{ SceneKeys::RiposteReach,           actions->riposteReach           },
			{ SceneKeys::AttackLungeMaxDistance, actions->attackLungeMaxDistance },
			{ SceneKeys::DodgeCancelFrom,        actions->dodgeCancelFrom        },
			{ SceneKeys::HitReactionDodgeFrom,   actions->hitReactionDodgeFrom   },
			{ SceneKeys::ExtraHitReactionAnimations, actions->extraHitReactionAnimations },
			{ SceneKeys::HeavyHitReactionAnimation, actions->heavyHitReactionAnimation },
			{ SceneKeys::KnockbackAnimation,     actions->knockbackAnimation     },
			{ SceneKeys::KnockbackSeconds,       actions->knockbackSeconds       },
			{ SceneKeys::KnockbackDust,          actions->knockbackDust          },
			{ SceneKeys::HeavyAttacksArePerilous, actions->areHeavyAttacksPerilous },
			{ SceneKeys::PerilousWarningEffect,   actions->perilousWarningEffect   }
		};

		nlohmann::json moves = nlohmann::json::array();
		for (const AttackMove& move : actions->attackMoves)
		{
			moves.push_back({
				{ SceneKeys::MoveAnimation,        move.animation        },
				{ SceneKeys::MoveHitFrom,          move.hitFrom          },
				{ SceneKeys::MoveHitTo,            move.hitTo            },
				{ SceneKeys::MoveDamageMultiplier, move.damageMultiplier },
				{ SceneKeys::MoveCancelFrom,       move.cancelFrom       },
				{ SceneKeys::MoveStrikeHeight,     move.strikeHeight     },
				{ SceneKeys::MoveIsPerilous,       move.isPerilous       }
			});
		}
		entityJson[SceneKeys::CharacterActionsComponent][SceneKeys::AttackMoves] = moves;
	}

	if (auto* health = entityManager.GetComponent<HealthComponent>(i))
	{
		entityJson[SceneKeys::HealthComponent] =
		{
			{ SceneKeys::MaxHealth,              health->maxHealth              },
			{ SceneKeys::HitEffect,              health->hitEffect              },
			{ SceneKeys::DeflectEffect,          health->deflectEffect          },
			{ SceneKeys::DeathSlowMotionSeconds, health->deathSlowMotionSeconds },
			{ SceneKeys::DeathSlowMotionScale,   health->deathSlowMotionScale   },
			{ SceneKeys::WeaponDamageTaken,      health->weaponDamageTaken      },
			{ SceneKeys::SpellDamageTaken,       health->spellDamageTaken       },
			{ SceneKeys::SpellImmunitySeconds,   health->spellImmunitySeconds   },
			{ SceneKeys::RallyWindowSeconds,     health->rallyWindowSeconds     },
			{ SceneKeys::RallyPerDamage,         health->rallyPerDamage         }
		};
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
			{ SceneKeys::ParryCost,                stamina->parryCost                },
			{ SceneKeys::RunCostPerSecond,         stamina->runCostPerSecond         }
		};
	}

	if (auto* hitbox = entityManager.GetComponent<HitboxComponent>(i))
	{
		entityJson[SceneKeys::HitboxComponent] =
		{
			{ SceneKeys::Damage,                     hitbox->damage       },
			{ SceneKeys::PoiseDamage,                hitbox->poiseDamage  },
			{ SceneKeys::HitStopSeconds,             hitbox->hitStopSeconds },
			{ SceneKeys::Knockback,                  hitbox->knockback    },
			{ SceneKeys::CameraShake,                hitbox->cameraShake  },
			{ SceneKeys::LocalOffset,  SerializeVec3(hitbox->localOffset) },
			{ SceneKeys::HalfExtents,  SerializeVec3(hitbox->halfExtents) },
			{ SceneKeys::Layer, static_cast<unsigned int>(hitbox->layer)  },
			{ SceneKeys::ShowGizmos,                 hitbox->showGizmos   },
			{ SceneKeys::HitboxIsEnabled,            hitbox->isEnabled    }
		};
	}

	if (auto* shield = entityManager.GetComponent<ShieldComponent>(i))
	{
		entityJson[SceneKeys::ShieldComponent] =
		{
			{ SceneKeys::GuardAngleDegrees, shield->guardAngleDegrees },
			{ SceneKeys::DamageThrough,     shield->damageThrough     },
			{ SceneKeys::StaminaPerDamage,  shield->staminaPerDamage  },
			{ SceneKeys::BlockEffect,       shield->blockEffect       }
		};
	}

	if (auto* boss = entityManager.GetComponent<BossComponent>(i))
	{
		entityJson[SceneKeys::BossComponent] =
		{
			{ SceneKeys::PhaseTwoHealthFraction,   boss->phaseTwoHealthFraction   },
			{ SceneKeys::PhaseTwoDamageMultiplier, boss->phaseTwoDamageMultiplier },
			{ SceneKeys::PhaseTwoAttackSpeedup,    boss->phaseTwoAttackSpeedup    },
			{ SceneKeys::PhaseTwoAnimation,        boss->phaseTwoAnimation        },
			{ SceneKeys::PhaseTwoSound,            boss->phaseTwoSound            },
			{ SceneKeys::PhaseTwoMusic,            boss->phaseTwoMusic            },
			{ SceneKeys::DefeatCutscene,           boss->defeatCutscene           },
			{ SceneKeys::DefeatCutsceneDelay,      boss->defeatCutsceneDelay      },
			{ SceneKeys::PhaseTwoGlow,             boss->phaseTwoGlow             },
			{ SceneKeys::PhaseTwoGrowth,           boss->phaseTwoGrowth           },
			{ SceneKeys::PhaseTwoLight1Animation,  boss->phaseTwoLight1Animation  },
			{ SceneKeys::PhaseTwoLight2Animation,  boss->phaseTwoLight2Animation  },
			{ SceneKeys::PhaseTwoLight3Animation,  boss->phaseTwoLight3Animation  },
			{ SceneKeys::PhaseTwoHeavyAnimation,   boss->phaseTwoHeavyAnimation   },
			{ SceneKeys::PhaseTwoLightHitFrom,     boss->phaseTwoLightHitFrom     },
			{ SceneKeys::PhaseTwoLightHitTo,       boss->phaseTwoLightHitTo       },
			{ SceneKeys::PhaseTwoHeavyHitFrom,     boss->phaseTwoHeavyHitFrom     },
			{ SceneKeys::PhaseTwoHeavyHitTo,       boss->phaseTwoHeavyHitTo       },
			{ SceneKeys::PhaseTwoShockwaveRadius,  boss->phaseTwoShockwaveRadius  },
			{ SceneKeys::PhaseTwoMaxLightCombo,    boss->phaseTwoMaxLightCombo    }
		};
	}

	if (auto* cutscene = entityManager.GetComponent<CutsceneComponent>(i))
	{
		entityJson[SceneKeys::CutsceneComponent] =
		{
			{ SceneKeys::DurationSeconds,       cutscene->durationSeconds    },
			{ SceneKeys::LookAtEntity,          cutscene->lookAtEntity       },
			{ SceneKeys::LookAtHeight,          cutscene->lookAtHeight       },
			{ SceneKeys::LookAlsoAtEntity,      cutscene->lookAlsoAtEntity   },
			{ SceneKeys::AnimationEntity,       cutscene->animationEntity    },
			{ SceneKeys::CutsceneAnimationName, cutscene->animationName      },
			{ SceneKeys::AnimationAtSeconds,    cutscene->animationAtSeconds },
			{ SceneKeys::CutsceneTitle,         cutscene->title              },
			{ SceneKeys::TitleSeconds,          cutscene->titleSeconds       },
			{ SceneKeys::CanBeSkipped,          cutscene->canBeSkipped       },
			{ SceneKeys::VideoFile,             cutscene->videoFile          },
			{ SceneKeys::WaitForKey,            cutscene->waitForKey         },
			{ SceneKeys::WaitPrompt,            cutscene->waitPrompt         },
			{ SceneKeys::WaitPresses,           cutscene->waitPresses        },
			{ SceneKeys::PressWindowSeconds,    cutscene->pressWindowSeconds },
			{ SceneKeys::PressSound,            cutscene->pressSound         },
			{ SceneKeys::BlendInSeconds,        cutscene->blendInSeconds     },
			{ SceneKeys::FadeSeconds,           cutscene->fadeSeconds        },
			{ SceneKeys::CutsceneSound,         cutscene->sound              }
		};
	}

	if (auto* environment = entityManager.GetComponent<EnvironmentComponent>(i))
	{
		entityJson[SceneKeys::EnvironmentComponent] =
		{
			{ SceneKeys::SkyboxFolder,  environment->skyboxFolder           },
			{ SceneKeys::SkyBrightness, environment->skyBrightness          },
			{ SceneKeys::IsFogEnabled,  environment->isFogEnabled           },
			{ SceneKeys::FogColor,      SerializeVec3(environment->fogColor) },
			{ SceneKeys::FogDensity,    environment->fogDensity             },
			{ SceneKeys::IsPostProcessingEnabled, environment->isPostProcessingEnabled },
			{ SceneKeys::Exposure,                environment->exposure                },
			{ SceneKeys::IsBloomEnabled,          environment->isBloomEnabled          },
			{ SceneKeys::BloomThreshold,          environment->bloomThreshold          },
			{ SceneKeys::BloomIntensity,          environment->bloomIntensity          },
			{ SceneKeys::BloomBlurPasses,         environment->bloomBlurPasses         },
			{ SceneKeys::HitImpactFrame,          environment->hitImpactFrame          },
			{ SceneKeys::PixelSize,               environment->pixelSize               },
			{ SceneKeys::PaletteLevels,           environment->paletteLevels           },
			{ SceneKeys::DitherStrength,          environment->ditherStrength          },
			{ SceneKeys::OutlineStrength,         environment->outlineStrength         },
			{ SceneKeys::OutlineColor,            SerializeVec3(environment->outlineColor) },
			{ SceneKeys::HueShift,                environment->hueShift                },
			{ SceneKeys::ShadowHue,               environment->shadowHue               },
			{ SceneKeys::LightHue,                environment->lightHue                },
			{ SceneKeys::CleanLines,              environment->cleanLines              },
			{ SceneKeys::RemoveOrphans,           environment->removeOrphans           },
			{ SceneKeys::PaintRadius,             environment->paintRadius             },
			{ SceneKeys::Saturation,              environment->saturation              },
			{ SceneKeys::Contrast,                environment->contrast                },
			{ SceneKeys::ColorTint,               SerializeVec3(environment->colorTint) },
			{ SceneKeys::Vignette,                environment->vignette                }
		};
	}

	if (auto* emitter = entityManager.GetComponent<ParticleEmitterComponent>(i))
	{
		entityJson[SceneKeys::ParticleEmitterComponent] =
		{
			{ SceneKeys::IsEmitting,         emitter->isEmitting                     },
			{ SceneKeys::ParticlesPerSecond, emitter->particlesPerSecond             },
			{ SceneKeys::LifetimeSeconds,    emitter->lifetimeSeconds                },
			{ SceneKeys::SpawnHalfExtents,   SerializeVec3(emitter->spawnHalfExtents) },
			{ SceneKeys::ParticleVelocity,   SerializeVec3(emitter->velocity)        },
			{ SceneKeys::VelocityRandomness, emitter->velocityRandomness             },
			{ SceneKeys::ParticleGravity,    SerializeVec3(emitter->gravity)         },
			{ SceneKeys::StartSize,          emitter->startSize                      },
			{ SceneKeys::EndSize,            emitter->endSize                        },
			{ SceneKeys::StartColor,         SerializeVec3(emitter->startColor)      },
			{ SceneKeys::EndColor,           SerializeVec3(emitter->endColor)        },
			{ SceneKeys::ParticleIntensity,  emitter->intensity                      },
			{ SceneKeys::MaxParticles,       emitter->maxParticles                   },
			{ SceneKeys::IsDark,             emitter->isDark                         },
			{ SceneKeys::ParticleStyle,      emitter->style                          },
			{ SceneKeys::SpawnShape,         emitter->spawnShape                     },
			{ SceneKeys::SpawnRadius,        emitter->spawnRadius                    },
			{ SceneKeys::OutwardSpeed,       emitter->outwardSpeed                   },
			{ SceneKeys::SizeRandomness,     emitter->sizeRandomness                 },
			{ SceneKeys::ParticleSpin,       emitter->spinDegreesPerSecond           },
			{ SceneKeys::ParticleDrag,       emitter->drag                           },
			{ SceneKeys::ParticleStretch,    emitter->stretch                        },
			{ SceneKeys::HasMiddleColor,     emitter->hasMiddleColor                 },
			{ SceneKeys::MiddleColor,        SerializeVec3(emitter->middleColor)     },
			{ SceneKeys::BurstAmount,        emitter->burstAmount                    },
			{ SceneKeys::FlipbookTexture,    emitter->flipbookTexture                },
			{ SceneKeys::FlipbookColumns,    emitter->flipbookColumns                },
			{ SceneKeys::FlipbookRows,       emitter->flipbookRows                   },
			{ SceneKeys::FlipbookLoops,      emitter->flipbookLoops                  },
			{ SceneKeys::FlipbookRandomStart, emitter->flipbookRandomStart           }
		};
	}

	if (auto* shape = entityManager.GetComponent<EffectShapeComponent>(i))
	{
		entityJson[SceneKeys::EffectShapeComponent] =
		{
			{ SceneKeys::ShapeKind,               shape->kind                              },
			{ SceneKeys::ShapeLifetime,           shape->lifetimeSeconds                   },
			{ SceneKeys::ShapeStartRadius,        shape->startRadius                       },
			{ SceneKeys::ShapeEndRadius,          shape->endRadius                         },
			{ SceneKeys::ShapeThickness,          shape->thickness                         },
			{ SceneKeys::ShapeHeight,             shape->height                            },
			{ SceneKeys::ShapeArcDegrees,         shape->arcDegrees                        },
			{ SceneKeys::ShapeRotation,           SerializeVec3(shape->rotationDegrees)    },
			{ SceneKeys::ShapeRotationRandomness, SerializeVec3(shape->rotationRandomness) },
			{ SceneKeys::ShapeOffset,             SerializeVec3(shape->offset)             },
			{ SceneKeys::ShapeSpin,               shape->spinDegreesPerSecond              },
			{ SceneKeys::ShapeTwist,              shape->twistDegrees                      },
			{ SceneKeys::ShapeCoreColor,          SerializeVec3(shape->coreColor)          },
			{ SceneKeys::ShapeEdgeColor,          SerializeVec3(shape->edgeColor)          },
			{ SceneKeys::ShapeIntensity,          shape->intensity                         },
			{ SceneKeys::ShapeNoiseScale,         shape->noiseScale                        },
			{ SceneKeys::ShapeScrollSpeed,        shape->scrollSpeed                       },
			{ SceneKeys::ShapeDissolve,           shape->dissolve                          },
			{ SceneKeys::ShapeLightIntensity,     shape->lightIntensity                    },
			{ SceneKeys::ShapeLightRadius,        shape->lightRadius                       },
			{ SceneKeys::ShapeLightColor,         SerializeVec3(shape->lightColor)         }
		};
	}

	if (auto* sprite = entityManager.GetComponent<GlowSpriteComponent>(i))
	{
		entityJson[SceneKeys::GlowSpriteComponent] =
		{
			{ SceneKeys::IsVisible,         sprite->isVisible            },
			{ SceneKeys::SpriteColor,       SerializeVec3(sprite->color) },
			{ SceneKeys::SpriteSize,        sprite->size                 },
			{ SceneKeys::ParticleIntensity, sprite->intensity            }
		};
	}

	if (auto* orb = entityManager.GetComponent<LiquidOrbComponent>(i))
	{
		entityJson[SceneKeys::LiquidOrbComponent] =
		{
			{ SceneKeys::IsVisible,      orb->isVisible           },
			{ SceneKeys::OrbTint,        SerializeVec3(orb->tint) },
			{ SceneKeys::OrbBrightness,  orb->brightness          },
			{ SceneKeys::OrbWobble,      orb->wobble              },
			{ SceneKeys::OrbWobbleSpeed, orb->wobbleSpeed         },
			{ SceneKeys::OrbClearness,   orb->clearness           }
		};
	}

	if (auto* foliage = entityManager.GetComponent<FoliageComponent>(i))
	{
		entityJson[SceneKeys::FoliageComponent] =
		{
			{ SceneKeys::FoliageModel,         foliage->modelName     },
			{ SceneKeys::FoliageCount,         foliage->count         },
			{ SceneKeys::FoliageAreaX,         foliage->areaSizeX     },
			{ SceneKeys::FoliageAreaZ,         foliage->areaSizeZ     },
			{ SceneKeys::FoliageMask,          foliage->densityMask   },
			{ SceneKeys::FoliageMinScale,      foliage->minScale      },
			{ SceneKeys::FoliageMaxScale,      foliage->maxScale      },
			{ SceneKeys::FoliageSway,          foliage->sway          },
			{ SceneKeys::FoliageSwaySpeed,     foliage->swaySpeed     },
			{ SceneKeys::FoliageDrawDistance,  foliage->drawDistance  },
			{ SceneKeys::FoliageTint,          { foliage->tint.x, foliage->tint.y, foliage->tint.z } },
			{ SceneKeys::FoliageTintVariation, foliage->tintVariation },
			{ SceneKeys::FoliageSeed,          foliage->seed          }
		};
	}

	if (auto* swarm = entityManager.GetComponent<SwarmComponent>(i))
	{
		entityJson[SceneKeys::SwarmComponent] =
		{
			{ SceneKeys::IsVisible,           swarm->isVisible                      },
			{ SceneKeys::SwarmModelName,      swarm->modelName                      },
			{ SceneKeys::SwarmCount,          swarm->count                          },
			{ SceneKeys::SwarmSeed,           swarm->seed                           },
			{ SceneKeys::SwarmPieceSize,      swarm->pieceSize                      },
			{ SceneKeys::SwarmSizeVariation,  swarm->sizeVariation                  },
			{ SceneKeys::SwarmRadius,         swarm->radius                         },
			{ SceneKeys::SwarmShellThickness, swarm->shellThickness                 },
			{ SceneKeys::SwarmSpin,           swarm->spinDegreesPerSecond           },
			{ SceneKeys::SwarmTumble,         swarm->tumbleDegreesPerSecond         },
			{ SceneKeys::SwarmSkyCenter,      SerializeVec3(swarm->skyCenter)       },
			{ SceneKeys::SwarmSkyHalfExtents, SerializeVec3(swarm->skyHalfExtents)  },
			{ SceneKeys::SwarmFallSpeed,      swarm->fallSpeed                      },
			{ SceneKeys::SwarmFallSpread,     swarm->fallSpreadSeconds              },
			{ SceneKeys::SwarmGroundHeight,   swarm->groundHeight                   },
			{ SceneKeys::SwarmStuckSeconds,   swarm->stuckSeconds                   },
			{ SceneKeys::SwarmGlowColor,      SerializeVec3(swarm->glowColor)       },
			{ SceneKeys::SwarmTrailLength,    swarm->trailLength                    },
			{ SceneKeys::SwarmHitEffect,      swarm->hitEffect                      },
			{ SceneKeys::SwarmHitBurst,       swarm->hitBurst                       }
		};
	}

	if (auto* trail = entityManager.GetComponent<WeaponTrailComponent>(i))
	{
		entityJson[SceneKeys::WeaponTrailComponent] =
		{
			{ SceneKeys::BaseOffset,        SerializeVec3(trail->baseOffset) },
			{ SceneKeys::TipOffset,         SerializeVec3(trail->tipOffset)  },
			{ SceneKeys::Color,        SerializeVec3(trail->color)      },
			{ SceneKeys::ParticleIntensity, trail->intensity                 },
			{ SceneKeys::LifetimeSeconds,   trail->lifetimeSeconds           },
			{ SceneKeys::OnlyWhileStriking, trail->onlyWhileStriking         }
		};
	}

	if (auto* fogGate = entityManager.GetComponent<FogGateComponent>(i))
	{
		entityJson[SceneKeys::FogGateComponent] =
		{
			{ SceneKeys::ArenaCenter,      SerializeVec3(fogGate->arenaCenter)      },
			{ SceneKeys::ArenaHalfExtents, SerializeVec3(fogGate->arenaHalfExtents) },
			{ SceneKeys::VictoryMessage,                fogGate->victoryMessage     },
			{ SceneKeys::IsCrossedWithKey,              fogGate->isCrossedWithKey   },
			{ SceneKeys::FogCutscene,                   fogGate->cutscene           },
			{ SceneKeys::BattleMusic,                   fogGate->battleMusic        },
			{ SceneKeys::BattleMusicVolume,             fogGate->battleMusicVolume  },
			{ SceneKeys::VictorySound,                  fogGate->victorySound       }
		};
	}

	if (auto* flask = entityManager.GetComponent<HealingFlaskComponent>(i))
	{
		entityJson[SceneKeys::HealingFlaskComponent] =
		{
			{ SceneKeys::DisplayName,    flask->displayName    },
			{ SceneKeys::MaxCharges,     flask->maxCharges     },
			{ SceneKeys::HealAmount,     flask->healAmount     },
			{ SceneKeys::DrinkAnimation, flask->drinkAnimation },
			{ SceneKeys::HealAtProgress, flask->healAtProgress },
			{ SceneKeys::HiddenWhileDrinking, flask->hiddenWhileDrinking },
			{ SceneKeys::ShownWhileDrinking,  flask->shownWhileDrinking  },
			{ SceneKeys::HealEffect,     flask->healEffect     },
			{ SceneKeys::DrinkSound,     flask->drinkSound     },
			{ SceneKeys::HealSound,      flask->healSound      }
		};
	}

	if (auto* currency = entityManager.GetComponent<CurrencyComponent>(i))
	{
		entityJson[SceneKeys::CurrencyComponent] =
		{
			{ SceneKeys::DisplayName,        currency->displayName        },
			{ SceneKeys::CurrencyAmount,     currency->amount             },
			{ SceneKeys::IsDroppedOnDeath,   currency->isDroppedOnDeath   },
			{ SceneKeys::DropMarker,         currency->dropMarker         },
			{ SceneKeys::RetrieveDistance,   currency->retrieveDistance   },
			{ SceneKeys::IsRetrievedWithKey, currency->isRetrievedWithKey },
			{ SceneKeys::RetrievePrompt,     currency->retrievePrompt     },
			{ SceneKeys::GainSound,          currency->gainSound          },
			{ SceneKeys::RetrieveSound,      currency->retrieveSound      }
		};
	}

	if (auto* reward = entityManager.GetComponent<CurrencyRewardComponent>(i))
	{
		entityJson[SceneKeys::CurrencyRewardComponent] =
		{
			{ SceneKeys::CurrencyAmount, reward->amount }
		};
	}

	if (auto* levelUp = entityManager.GetComponent<LevelUpComponent>(i))
	{
		entityJson[SceneKeys::LevelUpComponent] =
		{
			{ SceneKeys::DisplayName,            levelUp->displayName            },
			{ SceneKeys::Level,                  levelUp->level                  },
			{ SceneKeys::FirstLevelCost,         levelUp->firstLevelCost         },
			{ SceneKeys::CostGrowth,             levelUp->costGrowth             },
			{ SceneKeys::StartingAttributeValue, levelUp->startingAttributeValue },
			{ SceneKeys::HealthAttribute,        levelUp->healthAttribute        },
			{ SceneKeys::HealthPerPoint,         levelUp->healthPerPoint         },
			{ SceneKeys::HealthPoints,           levelUp->healthPoints           },
			{ SceneKeys::StaminaAttribute,       levelUp->staminaAttribute       },
			{ SceneKeys::StaminaPerPoint,        levelUp->staminaPerPoint        },
			{ SceneKeys::StaminaPoints,          levelUp->staminaPoints          },
			{ SceneKeys::DamageAttribute,        levelUp->damageAttribute        },
			{ SceneKeys::DamagePerPoint,         levelUp->damagePerPoint         },
			{ SceneKeys::DamagePoints,           levelUp->damagePoints           },
			{ SceneKeys::LevelUpSound,           levelUp->levelUpSound           },
			{ SceneKeys::LevelUpEffect,          levelUp->levelUpEffect          }
		};
	}

	if (auto* audioSource = entityManager.GetComponent<AudioSourceComponent>(i))
	{
		entityJson[SceneKeys::AudioSourceComponent] =
		{
			{ SceneKeys::Sound,       audioSource->sound       },
			{ SceneKeys::SoundVolume, audioSource->volume      },
			{ SceneKeys::IsLooping,   audioSource->isLooping   },
			{ SceneKeys::PlayOnStart, audioSource->playOnStart },
			{ SceneKeys::IsSpatial,   audioSource->isSpatial   },
			{ SceneKeys::MinDistance, audioSource->minDistance },
			{ SceneKeys::MaxDistance, audioSource->maxDistance }
		};
	}

	if (auto* sounds = entityManager.GetComponent<CharacterSoundsComponent>(i))
	{
		entityJson[SceneKeys::CharacterSoundsComponent] =
		{
			{ SceneKeys::AttackSound,    sounds->attackSound    },
			{ SceneKeys::HurtSound,      sounds->hurtSound      },
			{ SceneKeys::BlockSound,     sounds->blockSound     },
			{ SceneKeys::ParrySound,     sounds->parrySound     },
			{ SceneKeys::DeathSound,     sounds->deathSound     },
			{ SceneKeys::RollSound,      sounds->rollSound      },
			{ SceneKeys::FootstepSound,  sounds->footstepSound  },
			{ SceneKeys::ShockwaveSound, sounds->shockwaveSound },
			{ SceneKeys::AttackSoundLeadSeconds, sounds->attackSoundLeadSeconds },
			{ SceneKeys::LeftFootBone,   sounds->leftFootBone   },
			{ SceneKeys::RightFootBone,  sounds->rightFootBone  },
			{ SceneKeys::FootstepMeters, sounds->footstepMeters },
			{ SceneKeys::FootstepCameraShake, sounds->footstepCameraShake },
			{ SceneKeys::FootstepShakeRadius, sounds->footstepShakeRadius },
			{ SceneKeys::SoundVolume,    sounds->volume         },
			{ SceneKeys::PainVoice,      sounds->painVoice      },
			{ SceneKeys::AttackVoice,    sounds->attackVoice    },
			{ SceneKeys::DeathVoice,     sounds->deathVoice     },
			{ SceneKeys::AlertVoice,     sounds->alertVoice     },
			{ SceneKeys::PainVoiceChance,   sounds->painVoiceChance   },
			{ SceneKeys::AttackVoiceChance, sounds->attackVoiceChance }
		};
	}

	if (auto* poise = entityManager.GetComponent<PoiseComponent>(i))
	{
		entityJson[SceneKeys::PoiseComponent] =
		{
			{ SceneKeys::MaxPoise,            poise->maxPoise            },
			{ SceneKeys::RecoverAfterSeconds, poise->recoverAfterSeconds },
			{ SceneKeys::StaggerImmunitySeconds, poise->staggerImmunitySeconds }
		};
	}

	if (auto* terrain = entityManager.GetComponent<TerrainComponent>(i))
	{
		entityJson[SceneKeys::TerrainComponent] =
		{
			{ SceneKeys::Heightmap,    terrain->heightmap   },
			{ SceneKeys::TerrainSizeX, terrain->sizeX       },
			{ SceneKeys::TerrainSizeZ, terrain->sizeZ       },
			{ SceneKeys::HeightScale,  terrain->heightScale }
		};
	}

	if (auto* pickup = entityManager.GetComponent<PickupComponent>(i))
	{
		entityJson[SceneKeys::PickupComponent] =
		{
			{ SceneKeys::ItemId,                 pickup->itemId           },
			{ SceneKeys::DisplayName,            pickup->displayName      },
			{ SceneKeys::PickupPrompt,           pickup->prompt           },
			{ SceneKeys::PickupObtainedText,     pickup->obtainedText     },
			{ SceneKeys::InteractDistancePickup, pickup->interactDistance },
			{ SceneKeys::ReadText,               pickup->readText         },
			{ SceneKeys::VoiceSound,             pickup->voiceSound       },
			{ SceneKeys::ReadSeconds,            pickup->readSeconds      },
			{ SceneKeys::PickupSound,            pickup->pickupSound      }
		};
	}

	if (entityManager.GetComponent<InventoryComponent>(i))
		entityJson[SceneKeys::InventoryComponent] = nlohmann::json::object();

	if (auto* loot = entityManager.GetComponent<LootDropComponent>(i))
	{
		entityJson[SceneKeys::LootDropComponent] =
		{
			{ SceneKeys::PickupTemplate, loot->pickupTemplate },
			{ SceneKeys::LootChance,     loot->chance         },
			{ SceneKeys::OnlyOnce,       loot->onlyOnce       }
		};
	}

	if (auto* mount = entityManager.GetComponent<MountComponent>(i))
	{
		entityJson[SceneKeys::MountComponent] =
		{
			{ SceneKeys::SeatOffset,            { mount->seatOffset.x, mount->seatOffset.y, mount->seatOffset.z } },
			{ SceneKeys::MountDistance,         mount->mountDistance         },
			{ SceneKeys::MountPrompt,           mount->mountPrompt           },
			{ SceneKeys::DismountPrompt,        mount->dismountPrompt        },
			{ SceneKeys::RiderIdleAnimation,    mount->riderIdleAnimation    },
			{ SceneKeys::RiderWalkAnimation,    mount->riderWalkAnimation    },
			{ SceneKeys::RiderRunAnimation,     mount->riderRunAnimation     },
			{ SceneKeys::RunAnimationFromSpeed, mount->runAnimationFromSpeed },
			{ SceneKeys::CameraRise,            mount->cameraRise            },
			{ SceneKeys::CameraPullBack,        mount->cameraPullBack        }
		};
	}

	if (auto* summon = entityManager.GetComponent<SummonComponent>(i))
	{
		entityJson[SceneKeys::SummonComponent] =
		{
			{ SceneKeys::SummonAllies,     summon->allies          },
			{ SceneKeys::SummonMaxCharges, summon->maxCharges      },
			{ SceneKeys::SummonLifetime,   summon->lifetimeSeconds },
			{ SceneKeys::SummonEffect,     summon->summonEffect    },
			{ SceneKeys::SummonCalledText, summon->calledText      }
		};
	}

	if (auto* wrath = entityManager.GetComponent<WrathComponent>(i))
	{
		entityJson[SceneKeys::WrathComponent] =
		{
			{ SceneKeys::MaxWrath,               wrath->maxWrath          },
			{ SceneKeys::WrathFromDamageDealt,   wrath->fromDamageDealt   },
			{ SceneKeys::WrathFromDamageTaken,   wrath->fromDamageTaken   },
			{ SceneKeys::WrathFromDeflects,      wrath->fromDeflects      },
			{ SceneKeys::WrathDrainPerSecond,    wrath->drainPerSecond    },
			{ SceneKeys::WrathDrainDelay,        wrath->drainDelaySeconds },
			{ SceneKeys::WrathDuration,          wrath->durationSeconds   },
			{ SceneKeys::WrathDamageMultiplier,  wrath->damageMultiplier  },
			{ SceneKeys::WrathPostureMultiplier, wrath->postureMultiplier },
			{ SceneKeys::WrathGlowColor,         { wrath->glowColor.x, wrath->glowColor.y, wrath->glowColor.z } },
			{ SceneKeys::WrathReleaseEffect,     wrath->releaseEffect     }
		};
	}

	if (auto* posture = entityManager.GetComponent<PostureComponent>(i))
	{
		entityJson[SceneKeys::PostureComponent] =
		{
			{ SceneKeys::MaxPosture,              posture->maxPosture             },
			{ SceneKeys::PostureFromHits,         posture->fromHits               },
			{ SceneKeys::PostureFromBlocks,       posture->fromBlocks             },
			{ SceneKeys::PostureFromBeingParried, posture->fromBeingParried       },
			{ SceneKeys::PostureRecoverPerSecond, posture->recoverPerSecond       },
			{ SceneKeys::PostureRecoverDelay,     posture->recoverDelaySeconds    },
			{ SceneKeys::PostureLowHealthRecover, posture->lowHealthRecoverFactor },
			{ SceneKeys::PostureBrokenSeconds,    posture->brokenSeconds          },
			{ SceneKeys::FinisherDamage,          posture->finisherDamage         },
			{ SceneKeys::FinisherSlowMotion,      posture->finisherSlowMotion     },
			{ SceneKeys::FinisherSlowSeconds,     posture->finisherSlowSeconds    },
			{ SceneKeys::PostureBreakEffect,      posture->breakEffect            },
			{ SceneKeys::FinisherEffect,          posture->finisherEffect         }
		};
	}

	if (auto* enemy = entityManager.GetComponent<EnemyComponent>(i))
	{
		entityJson[SceneKeys::EnemyComponent] =
		{
			{ SceneKeys::SightDistance,     enemy->sightDistance     },
			{ SceneKeys::SightAngleDegrees, enemy->sightAngleDegrees },
			{ SceneKeys::HearingDistance,   enemy->hearingDistance   },
			{ SceneKeys::EyeHeight,         enemy->eyeHeight         },
			{ SceneKeys::StartsDormant,     enemy->startsDormant     },
			{ SceneKeys::IsAlly,            enemy->isAlly            }
		};
	}

	if (auto* bonfire = entityManager.GetComponent<BonfireComponent>(i))
	{
		entityJson[SceneKeys::BonfireComponent] =
		{
			{ SceneKeys::InteractDistance,               bonfire->interactDistance },
			{ SceneKeys::RespawnOffset,    SerializeVec3(bonfire->respawnOffset)    },
			{ SceneKeys::RestPrompt,       bonfire->restPrompt                      },
			{ SceneKeys::LeavePrompt,      bonfire->leavePrompt                     },
			{ SceneKeys::LitMessage,       bonfire->litMessage                      }
		};
	}

	if (auto* respawn = entityManager.GetComponent<RespawnComponent>(i))
	{
		entityJson[SceneKeys::RespawnComponent] = { { SceneKeys::RespawnDelaySeconds, respawn->respawnDelaySeconds } };
	}

	if (auto* hud = entityManager.GetComponent<PlayerHUDComponent>(i))
	{
		entityJson[SceneKeys::PlayerHUDComponent] =
		{
			{ SceneKeys::PixelsPerHealthPoint,  hud->pixelsPerHealthPoint  },
			{ SceneKeys::PixelsPerStaminaPoint, hud->pixelsPerStaminaPoint },
			{ SceneKeys::ShowLockOnMarker,      hud->showLockOnMarker      },
			{ SceneKeys::DeathMessage,          hud->deathMessage          },
			{ SceneKeys::HUDStyleFile,          hud->style                 }
		};
	}

	if (auto* healthBar = entityManager.GetComponent<EnemyHealthBarComponent>(i))
	{
		entityJson[SceneKeys::EnemyHealthBarComponent] =
		{
			{ SceneKeys::IsBoss,              healthBar->isBoss              },
			{ SceneKeys::DisplayName,         healthBar->displayName         },
			{ SceneKeys::ShowSecondsAfterHit, healthBar->showSecondsAfterHit },
			{ SceneKeys::HeightAboveFeet,     healthBar->heightAboveFeet     }
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
			{ SceneKeys::FinisherChance,        meleeEnemy->finisherChance        },
			{ SceneKeys::RecoverSeconds,        meleeEnemy->recoverSeconds        },
			{ SceneKeys::DodgeChance,           meleeEnemy->dodgeChance           },
			{ SceneKeys::DodgeDistance,         meleeEnemy->dodgeDistance         },
			{ SceneKeys::DodgeCooldownSeconds,  meleeEnemy->dodgeCooldownSeconds  },
			{ SceneKeys::GuardChance,           meleeEnemy->guardChance           },
			{ SceneKeys::GuardSeconds,          meleeEnemy->guardSeconds          },
			{ SceneKeys::LeapAttackDistance,    meleeEnemy->leapAttackDistance    }
		};
	}

	if (auto* spell = entityManager.GetComponent<SpellCasterComponent>(i))
	{
		entityJson[SceneKeys::SpellCasterComponent] =
		{
			{ SceneKeys::CastAnimation,            spell->castAnimation            },
			{ SceneKeys::ProjectileName,           spell->projectile               },
			{ SceneKeys::ProjectileCount,          spell->projectileCount          },
			{ SceneKeys::SpreadDegrees,            spell->spreadDegrees            },
			{ SceneKeys::ReleaseAt,                spell->releaseAt                },
			{ SceneKeys::AtTargetsFeet,            spell->atTargetsFeet            },
			{ SceneKeys::DropHeight,               spell->dropHeight               },
			{ SceneKeys::MinDistance,              spell->minDistance              },
			{ SceneKeys::MaxDistance,              spell->maxDistance              },
			{ SceneKeys::SpellCooldownSeconds,     spell->cooldownSeconds          },
			{ SceneKeys::ChargeEffect,             spell->chargeEffect             },
			{ SceneKeys::ChargeParticlesPerSecond, spell->chargeParticlesPerSecond },
			{ SceneKeys::CastSound,                spell->castSound                },
			{ SceneKeys::CastKey,                  spell->castKey                  },
			{ SceneKeys::DisplayName,              spell->displayName              },
			{ SceneKeys::IsCastOnRequestOnly,      spell->isCastOnRequestOnly      },
			{ SceneKeys::IsUninterruptible,        spell->isUninterruptible        }
		};
	}

	if (auto* projectile = entityManager.GetComponent<ProjectileComponent>(i))
	{
		entityJson[SceneKeys::ProjectileComponent] =
		{
			{ SceneKeys::ProjectileSpeed,         projectile->speed                   },
			{ SceneKeys::Damage,                  projectile->damage                  },
			{ SceneKeys::PoiseDamage,             projectile->poiseDamage             },
			{ SceneKeys::ProjectileRadius,        projectile->radius                  },
			{ SceneKeys::HomingDegreesPerSecond,  projectile->homingDegreesPerSecond  },
			{ SceneKeys::LifetimeSeconds,         projectile->lifetimeSeconds         },
			{ SceneKeys::FuseSeconds,             projectile->fuseSeconds             },
			{ SceneKeys::TrailEffect,             projectile->trailEffect             },
			{ SceneKeys::TrailParticlesPerSecond, projectile->trailParticlesPerSecond },
			{ SceneKeys::SpinDegreesPerSecond,    projectile->spinDegreesPerSecond    },
			{ SceneKeys::LaunchDelaySeconds,      projectile->launchDelaySeconds      },
			{ SceneKeys::BodyFacesFlight,         projectile->bodyFacesFlight         },
			{ SceneKeys::SpinsAroundFlight,       projectile->spinsAroundFlight       },
			{ SceneKeys::FormSeconds,             projectile->formSeconds             },
			{ SceneKeys::AimsAfterForming,        projectile->aimsAfterForming        },
			{ SceneKeys::HitEffect,               projectile->hitEffect               },
			{ SceneKeys::HitBurst,                projectile->hitBurst                },
			{ SceneKeys::HitSound,                projectile->hitSound                },
			{ SceneKeys::HitStopSeconds,          projectile->hitStopSeconds          },
			{ SceneKeys::CameraShake,             projectile->cameraShake             }
		};
	}

	return entityJson;
}

// Gives a new entity the components saved for it
static void DeserializeEntity(EntityManager& entityManager, unsigned int entity, const json& entityJson)
{

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
		modelComponent.emissiveStrength = modelData.value(SceneKeys::EmissiveStrength, modelComponent.emissiveStrength);
		modelComponent.fogAmount        = modelData.value(SceneKeys::FogAmount, modelComponent.fogAmount);
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
		boxColliderComponent.blocksCamera = boxColliderData.value(SceneKeys::BlocksCamera, true);
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
		const auto& lightData = entityJson[SceneKeys::DirectionalLightComponent];
		auto& directionalLight = entityManager.AddComponent<DirectionalLightComponent>(entity);
		DeserializeLightColor(lightData, directionalLight.color);

		directionalLight.castsShadows     = lightData.value(SceneKeys::CastsShadows,     directionalLight.castsShadows);
		directionalLight.shadowDistance   = lightData.value(SceneKeys::ShadowDistance,   directionalLight.shadowDistance);
		directionalLight.shadowStrength   = lightData.value(SceneKeys::ShadowStrength,   directionalLight.shadowStrength);
		directionalLight.shadowSoftness   = lightData.value(SceneKeys::ShadowSoftness,   directionalLight.shadowSoftness);
		directionalLight.shadowResolution = lightData.value(SceneKeys::ShadowResolution, directionalLight.shadowResolution);
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
		characterAnimation.walkBackAnimation    = animationData.value(SceneKeys::WalkBackAnimation,    characterAnimation.walkBackAnimation);
		characterAnimation.strafeLeftAnimation  = animationData.value(SceneKeys::StrafeLeftAnimation,  characterAnimation.strafeLeftAnimation);
		characterAnimation.strafeRightAnimation = animationData.value(SceneKeys::StrafeRightAnimation, characterAnimation.strafeRightAnimation);
		characterAnimation.lockedOnIdleAnimation = animationData.value(SceneKeys::LockedOnIdleAnimation, characterAnimation.lockedOnIdleAnimation);
		characterAnimation.blendSeconds  = animationData.value(SceneKeys::BlendSeconds,  characterAnimation.blendSeconds);
		characterAnimation.walkAnimationSpeed = animationData.value(SceneKeys::WalkAnimationSpeed, characterAnimation.walkAnimationSpeed);
		characterAnimation.runAnimationSpeed  = animationData.value(SceneKeys::RunAnimationSpeed,  characterAnimation.runAnimationSpeed);
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
		followCamera.lockOnSideDegrees   = followData.value(SceneKeys::LockOnSideDegrees, followCamera.lockOnSideDegrees);
		followCamera.lockOnSharpness     = followData.value(SceneKeys::LockOnSharpness,    followCamera.lockOnSharpness);
		followCamera.shakeDegrees        = followData.value(SceneKeys::ShakeDegrees,       followCamera.shakeDegrees);
		followCamera.shakeFrequency      = followData.value(SceneKeys::ShakeFrequency,     followCamera.shakeFrequency);
		followCamera.shakeFadePerSecond  = followData.value(SceneKeys::ShakeFadePerSecond, followCamera.shakeFadePerSecond);
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

	if (entityJson.contains(SceneKeys::StrikeAimComponent))
	{
		const auto& aimData = entityJson[SceneKeys::StrikeAimComponent];
		auto& aim = entityManager.AddComponent<StrikeAimComponent>(entity);

		// A missing key keeps the default written in StrikeAimComponent.h
		aim.spineBones       = aimData.value(SceneKeys::StrikeAimSpineBones,     aim.spineBones);
		aim.clipStrikeHeight = aimData.value(SceneKeys::StrikeAimClipHeight,     aim.clipStrikeHeight);
		aim.targetHeight     = aimData.value(SceneKeys::StrikeAimTargetHeight,   aim.targetHeight);
		aim.maxPitchDegrees  = aimData.value(SceneKeys::StrikeAimMaxPitch,       aim.maxPitchDegrees);
		aim.maxYawDegrees    = aimData.value(SceneKeys::StrikeAimMaxYaw,         aim.maxYawDegrees);
		aim.fadeSeconds      = aimData.value(SceneKeys::StrikeAimFadeSeconds,    aim.fadeSeconds);
		aim.searchDistance   = aimData.value(SceneKeys::StrikeAimSearchDistance, aim.searchDistance);
	}

	if (entityJson.contains(SceneKeys::LockOnTargetComponent))
	{
		const auto& targetData = entityJson[SceneKeys::LockOnTargetComponent];
		auto& lockOnTarget = entityManager.AddComponent<LockOnTargetComponent>(entity);

		lockOnTarget.isLockable = targetData.value(SceneKeys::IsLockable, lockOnTarget.isLockable);
		lockOnTarget.aimHeight  = targetData.value(SceneKeys::AimHeight,  lockOnTarget.aimHeight);
		lockOnTarget.aimBone    = targetData.value(SceneKeys::AimBone,    lockOnTarget.aimBone);
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
		actions.leapAttackAnimation   = actionsData.value(SceneKeys::LeapAttackAnimation,   actions.leapAttackAnimation);
		actions.leapSpeed             = actionsData.value(SceneKeys::LeapSpeed,             actions.leapSpeed);
		actions.leapHitFrom           = actionsData.value(SceneKeys::LeapHitFrom,           actions.leapHitFrom);
		actions.leapHitTo             = actionsData.value(SceneKeys::LeapHitTo,             actions.leapHitTo);
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
		actions.restAnimation         = actionsData.value(SceneKeys::RestAnimation,         actions.restAnimation);
		actions.standUpAnimation      = actionsData.value(SceneKeys::StandUpAnimation,      actions.standUpAnimation);
		actions.blockAnimation        = actionsData.value(SceneKeys::BlockAnimation,        actions.blockAnimation);
		actions.blockImpactAnimation  = actionsData.value(SceneKeys::BlockImpactAnimation,  actions.blockImpactAnimation);
		actions.parryAnimation        = actionsData.value(SceneKeys::ParryAnimation,        actions.parryAnimation);
		actions.parriedAnimation      = actionsData.value(SceneKeys::ParriedAnimation,      actions.parriedAnimation);
		actions.jumpAnimation         = actionsData.value(SceneKeys::JumpAnimation,         actions.jumpAnimation);
		actions.jumpAttackAnimation   = actionsData.value(SceneKeys::JumpAttackAnimation,   actions.jumpAttackAnimation);
		actions.jumpHeavyAttackAnimation = actionsData.value(SceneKeys::JumpHeavyAttackAnimation, actions.jumpHeavyAttackAnimation);
		actions.jumpSpeed             = actionsData.value(SceneKeys::JumpSpeed,             actions.jumpSpeed);
		actions.jumpMoveSpeed         = actionsData.value(SceneKeys::JumpMoveSpeed,         actions.jumpMoveSpeed);
		actions.jumpPlungeSpeed       = actionsData.value(SceneKeys::JumpPlungeSpeed,       actions.jumpPlungeSpeed);
		actions.jumpPlungeFrom        = actionsData.value(SceneKeys::JumpPlungeFrom,        actions.jumpPlungeFrom);
		actions.jumpAttackDamageMultiplier = actionsData.value(SceneKeys::JumpAttackDamageMultiplier, actions.jumpAttackDamageMultiplier);
		actions.jumpPlungeShockwaveRadius = actionsData.value(SceneKeys::JumpPlungeShockwaveRadius, actions.jumpPlungeShockwaveRadius);
		actions.jumpPlungeShockwaveDamage = actionsData.value(SceneKeys::JumpPlungeShockwaveDamage, actions.jumpPlungeShockwaveDamage);
		actions.jumpPlungeEffect      = actionsData.value(SceneKeys::JumpPlungeEffect,      actions.jumpPlungeEffect);
		actions.upperBodyBone         = actionsData.value(SceneKeys::UpperBodyBone,         actions.upperBodyBone);
		actions.shockwaveRadius       = actionsData.value(SceneKeys::ShockwaveRadius,       actions.shockwaveRadius);
		actions.shockwaveDamage       = actionsData.value(SceneKeys::ShockwaveDamage,       actions.shockwaveDamage);
		actions.shockwaveDistance     = actionsData.value(SceneKeys::ShockwaveDistance,     actions.shockwaveDistance);
		actions.shockwaveEffect       = actionsData.value(SceneKeys::ShockwaveEffect,       actions.shockwaveEffect);
		actions.swingEffect           = actionsData.value(SceneKeys::SwingEffect,           actions.swingEffect);
		actions.hyperArmor            = actionsData.value(SceneKeys::HyperArmor,            actions.hyperArmor);
		actions.heavySwingEffect      = actionsData.value(SceneKeys::HeavySwingEffect,      actions.heavySwingEffect);
		actions.swingEffectHeight     = actionsData.value(SceneKeys::SwingEffectHeight,     actions.swingEffectHeight);
		actions.swingEffectDistance   = actionsData.value(SceneKeys::SwingEffectDistance,   actions.swingEffectDistance);
		actions.swingEffectAt         = actionsData.value(SceneKeys::SwingEffectAt,         actions.swingEffectAt);
		actions.shockwaveCameraShake  = actionsData.value(SceneKeys::ShockwaveCameraShake,  actions.shockwaveCameraShake);
		actions.attackTurnDegreesPerSecond = actionsData.value(SceneKeys::AttackTurnDegreesPerSecond, actions.attackTurnDegreesPerSecond);
		actions.attackLungeSpeed      = actionsData.value(SceneKeys::AttackLungeSpeed,      actions.attackLungeSpeed);
		actions.attackReach           = actionsData.value(SceneKeys::AttackReach,           actions.attackReach);
		actions.finisherKnockback     = actionsData.value(SceneKeys::FinisherKnockback,     actions.finisherKnockback);
		actions.heavyKnockback        = actionsData.value(SceneKeys::HeavyKnockback,        actions.heavyKnockback);
		actions.knockdownAnimation    = actionsData.value(SceneKeys::KnockdownAnimation,    actions.knockdownAnimation);
		actions.heavyKnockbackChance  = actionsData.value(SceneKeys::HeavyKnockbackChance,  actions.heavyKnockbackChance);
		actions.getUpAnimation        = actionsData.value(SceneKeys::GetUpAnimation,        actions.getUpAnimation);
		actions.knockdownChance       = actionsData.value(SceneKeys::KnockdownChance,       actions.knockdownChance);
		actions.knockdownSeconds      = actionsData.value(SceneKeys::KnockdownSeconds,      actions.knockdownSeconds);
		actions.attackDelayChance     = actionsData.value(SceneKeys::AttackDelayChance,     actions.attackDelayChance);
		actions.attackDelaySeconds    = actionsData.value(SceneKeys::AttackDelaySeconds,    actions.attackDelaySeconds);
		actions.attackDelayAt         = actionsData.value(SceneKeys::AttackDelayAt,         actions.attackDelayAt);
		actions.parryFrom             = actionsData.value(SceneKeys::ParryFrom,             actions.parryFrom);
		actions.parryTo               = actionsData.value(SceneKeys::ParryTo,               actions.parryTo);
		actions.deflectSeconds        = actionsData.value(SceneKeys::DeflectSeconds,        actions.deflectSeconds);
		actions.riposteDamageMultiplier = actionsData.value(SceneKeys::RiposteDamageMultiplier, actions.riposteDamageMultiplier);
		actions.lightHitFrom          = actionsData.value(SceneKeys::LightHitFrom,          actions.lightHitFrom);
		actions.lightHitTo            = actionsData.value(SceneKeys::LightHitTo,            actions.lightHitTo);
		actions.heavyHitFrom          = actionsData.value(SceneKeys::HeavyHitFrom,          actions.heavyHitFrom);
		actions.heavyHitTo            = actionsData.value(SceneKeys::HeavyHitTo,            actions.heavyHitTo);
		actions.heavyDamageMultiplier = actionsData.value(SceneKeys::HeavyDamageMultiplier, actions.heavyDamageMultiplier);
		actions.attackSpeed           = actionsData.value(SceneKeys::AttackSpeed,           actions.attackSpeed);
		actions.extraLightAttackAnimations = actionsData.value(SceneKeys::ExtraLightAttackAnimations, actions.extraLightAttackAnimations);
		actions.heavyAttack2Animation  = actionsData.value(SceneKeys::HeavyAttack2Animation,  actions.heavyAttack2Animation);
		actions.runningAttackAnimation = actionsData.value(SceneKeys::RunningAttackAnimation, actions.runningAttackAnimation);
		actions.runningAttackSpeed     = actionsData.value(SceneKeys::RunningAttackSpeed,     actions.runningAttackSpeed);
		actions.dodgeAttackAnimation   = actionsData.value(SceneKeys::DodgeAttackAnimation,   actions.dodgeAttackAnimation);
		actions.riposteAnimation       = actionsData.value(SceneKeys::RiposteAnimation,       actions.riposteAnimation);
		actions.riposteReach           = actionsData.value(SceneKeys::RiposteReach,           actions.riposteReach);
		actions.attackLungeMaxDistance = actionsData.value(SceneKeys::AttackLungeMaxDistance, actions.attackLungeMaxDistance);
		actions.dodgeCancelFrom        = actionsData.value(SceneKeys::DodgeCancelFrom,        actions.dodgeCancelFrom);
		actions.hitReactionDodgeFrom   = actionsData.value(SceneKeys::HitReactionDodgeFrom,   actions.hitReactionDodgeFrom);
		actions.extraHitReactionAnimations = actionsData.value(SceneKeys::ExtraHitReactionAnimations, actions.extraHitReactionAnimations);
		actions.heavyHitReactionAnimation = actionsData.value(SceneKeys::HeavyHitReactionAnimation, actions.heavyHitReactionAnimation);
		actions.knockbackAnimation = actionsData.value(SceneKeys::KnockbackAnimation, actions.knockbackAnimation);
		actions.knockbackSeconds   = actionsData.value(SceneKeys::KnockbackSeconds,   actions.knockbackSeconds);
		actions.knockbackDust      = actionsData.value(SceneKeys::KnockbackDust,      actions.knockbackDust);
		actions.areHeavyAttacksPerilous = actionsData.value(SceneKeys::HeavyAttacksArePerilous, actions.areHeavyAttacksPerilous);
		actions.perilousWarningEffect   = actionsData.value(SceneKeys::PerilousWarningEffect,   actions.perilousWarningEffect);

		if (actionsData.contains(SceneKeys::AttackMoves))
		{
			for (const auto& moveData : actionsData[SceneKeys::AttackMoves])
			{
				AttackMove move;
				move.animation        = moveData.value(SceneKeys::MoveAnimation,        move.animation);
				move.hitFrom          = moveData.value(SceneKeys::MoveHitFrom,          move.hitFrom);
				move.hitTo            = moveData.value(SceneKeys::MoveHitTo,            move.hitTo);
				move.damageMultiplier = moveData.value(SceneKeys::MoveDamageMultiplier, move.damageMultiplier);
				move.cancelFrom       = moveData.value(SceneKeys::MoveCancelFrom,       move.cancelFrom);
				move.strikeHeight     = moveData.value(SceneKeys::MoveStrikeHeight,     move.strikeHeight);
				move.isPerilous       = moveData.value(SceneKeys::MoveIsPerilous,       move.isPerilous);
				actions.attackMoves.push_back(move);
			}
		}
	}

	if (entityJson.contains(SceneKeys::HealthComponent))
	{
		const auto& healthData = entityJson[SceneKeys::HealthComponent];
		auto& health = entityManager.AddComponent<HealthComponent>(entity);
		health.maxHealth     = healthData.value(SceneKeys::MaxHealth, health.maxHealth);
		health.hitEffect     = healthData.value(SceneKeys::HitEffect, health.hitEffect);
		health.deflectEffect = healthData.value(SceneKeys::DeflectEffect, health.deflectEffect);
		health.deathSlowMotionSeconds = healthData.value(SceneKeys::DeathSlowMotionSeconds, health.deathSlowMotionSeconds);
		health.deathSlowMotionScale   = healthData.value(SceneKeys::DeathSlowMotionScale,   health.deathSlowMotionScale);
		health.weaponDamageTaken      = healthData.value(SceneKeys::WeaponDamageTaken,      health.weaponDamageTaken);
		health.spellDamageTaken       = healthData.value(SceneKeys::SpellDamageTaken,       health.spellDamageTaken);
		health.spellImmunitySeconds   = healthData.value(SceneKeys::SpellImmunitySeconds,   health.spellImmunitySeconds);
		health.rallyWindowSeconds     = healthData.value(SceneKeys::RallyWindowSeconds,     health.rallyWindowSeconds);
		health.rallyPerDamage         = healthData.value(SceneKeys::RallyPerDamage,         health.rallyPerDamage);
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
		stamina.parryCost                = staminaData.value(SceneKeys::ParryCost,                stamina.parryCost);
		stamina.runCostPerSecond         = staminaData.value(SceneKeys::RunCostPerSecond,         stamina.runCostPerSecond);
		stamina.currentStamina           = stamina.maxStamina;
	}

	if (entityJson.contains(SceneKeys::HitboxComponent))
	{
		const auto& hitboxData = entityJson[SceneKeys::HitboxComponent];
		auto& hitbox = entityManager.AddComponent<HitboxComponent>(entity);

		hitbox.damage      = hitboxData.value(SceneKeys::Damage,      hitbox.damage);
		hitbox.poiseDamage = hitboxData.value(SceneKeys::PoiseDamage, hitbox.poiseDamage);
		hitbox.hitStopSeconds = hitboxData.value(SceneKeys::HitStopSeconds, hitbox.hitStopSeconds);
		hitbox.knockback      = hitboxData.value(SceneKeys::Knockback,      hitbox.knockback);
		hitbox.cameraShake    = hitboxData.value(SceneKeys::CameraShake,    hitbox.cameraShake);
		hitbox.showGizmos = hitboxData.value(SceneKeys::ShowGizmos, hitbox.showGizmos);
		hitbox.isEnabled  = hitboxData.value(SceneKeys::HitboxIsEnabled, hitbox.isEnabled);
		hitbox.layer      = static_cast<CollisionLayer>(hitboxData.value(SceneKeys::Layer, static_cast<unsigned int>(hitbox.layer)));

		if (hitboxData.contains(SceneKeys::LocalOffset))
			hitbox.localOffset = DeserializeVec3(hitboxData[SceneKeys::LocalOffset]);

		if (hitboxData.contains(SceneKeys::HalfExtents))
			hitbox.halfExtents = DeserializeVec3(hitboxData[SceneKeys::HalfExtents]);
	}

	if (entityJson.contains(SceneKeys::ShieldComponent))
	{
		const auto& shieldData = entityJson[SceneKeys::ShieldComponent];
		auto& shield = entityManager.AddComponent<ShieldComponent>(entity);

		shield.guardAngleDegrees = shieldData.value(SceneKeys::GuardAngleDegrees, shield.guardAngleDegrees);
		shield.damageThrough     = shieldData.value(SceneKeys::DamageThrough,     shield.damageThrough);
		shield.staminaPerDamage  = shieldData.value(SceneKeys::StaminaPerDamage,  shield.staminaPerDamage);
		shield.blockEffect       = shieldData.value(SceneKeys::BlockEffect,       shield.blockEffect);
	}

	if (entityJson.contains(SceneKeys::BossComponent))
	{
		const auto& bossData = entityJson[SceneKeys::BossComponent];
		auto& boss = entityManager.AddComponent<BossComponent>(entity);

		boss.phaseTwoHealthFraction   = bossData.value(SceneKeys::PhaseTwoHealthFraction,   boss.phaseTwoHealthFraction);
		boss.phaseTwoDamageMultiplier = bossData.value(SceneKeys::PhaseTwoDamageMultiplier, boss.phaseTwoDamageMultiplier);
		boss.phaseTwoAttackSpeedup    = bossData.value(SceneKeys::PhaseTwoAttackSpeedup,    boss.phaseTwoAttackSpeedup);
		boss.phaseTwoAnimation        = bossData.value(SceneKeys::PhaseTwoAnimation,        boss.phaseTwoAnimation);
		boss.phaseTwoSound            = bossData.value(SceneKeys::PhaseTwoSound,            boss.phaseTwoSound);
		boss.phaseTwoMusic            = bossData.value(SceneKeys::PhaseTwoMusic,            boss.phaseTwoMusic);
		boss.defeatCutscene           = bossData.value(SceneKeys::DefeatCutscene,           boss.defeatCutscene);
		boss.defeatCutsceneDelay      = bossData.value(SceneKeys::DefeatCutsceneDelay,      boss.defeatCutsceneDelay);
		boss.phaseTwoGlow             = bossData.value(SceneKeys::PhaseTwoGlow,             boss.phaseTwoGlow);
		boss.phaseTwoGrowth           = bossData.value(SceneKeys::PhaseTwoGrowth,           boss.phaseTwoGrowth);
		boss.phaseTwoLight1Animation  = bossData.value(SceneKeys::PhaseTwoLight1Animation,  boss.phaseTwoLight1Animation);
		boss.phaseTwoLight2Animation  = bossData.value(SceneKeys::PhaseTwoLight2Animation,  boss.phaseTwoLight2Animation);
		boss.phaseTwoLight3Animation  = bossData.value(SceneKeys::PhaseTwoLight3Animation,  boss.phaseTwoLight3Animation);
		boss.phaseTwoHeavyAnimation   = bossData.value(SceneKeys::PhaseTwoHeavyAnimation,   boss.phaseTwoHeavyAnimation);
		boss.phaseTwoLightHitFrom     = bossData.value(SceneKeys::PhaseTwoLightHitFrom,     boss.phaseTwoLightHitFrom);
		boss.phaseTwoLightHitTo       = bossData.value(SceneKeys::PhaseTwoLightHitTo,       boss.phaseTwoLightHitTo);
		boss.phaseTwoHeavyHitFrom     = bossData.value(SceneKeys::PhaseTwoHeavyHitFrom,     boss.phaseTwoHeavyHitFrom);
		boss.phaseTwoHeavyHitTo       = bossData.value(SceneKeys::PhaseTwoHeavyHitTo,       boss.phaseTwoHeavyHitTo);
		boss.phaseTwoShockwaveRadius  = bossData.value(SceneKeys::PhaseTwoShockwaveRadius,  boss.phaseTwoShockwaveRadius);
		boss.phaseTwoMaxLightCombo    = bossData.value(SceneKeys::PhaseTwoMaxLightCombo,    boss.phaseTwoMaxLightCombo);
	}

	if (entityJson.contains(SceneKeys::CutsceneComponent))
	{
		const auto& cutsceneData = entityJson[SceneKeys::CutsceneComponent];
		auto& cutscene = entityManager.AddComponent<CutsceneComponent>(entity);

		cutscene.durationSeconds    = cutsceneData.value(SceneKeys::DurationSeconds,       cutscene.durationSeconds);
		cutscene.lookAtEntity       = cutsceneData.value(SceneKeys::LookAtEntity,          cutscene.lookAtEntity);
		cutscene.lookAtHeight       = cutsceneData.value(SceneKeys::LookAtHeight,          cutscene.lookAtHeight);
		cutscene.lookAlsoAtEntity   = cutsceneData.value(SceneKeys::LookAlsoAtEntity,      cutscene.lookAlsoAtEntity);
		cutscene.animationEntity    = cutsceneData.value(SceneKeys::AnimationEntity,       cutscene.animationEntity);
		cutscene.animationName      = cutsceneData.value(SceneKeys::CutsceneAnimationName, cutscene.animationName);
		cutscene.animationAtSeconds = cutsceneData.value(SceneKeys::AnimationAtSeconds,    cutscene.animationAtSeconds);
		cutscene.title              = cutsceneData.value(SceneKeys::CutsceneTitle,         cutscene.title);
		cutscene.titleSeconds       = cutsceneData.value(SceneKeys::TitleSeconds,          cutscene.titleSeconds);
		cutscene.canBeSkipped       = cutsceneData.value(SceneKeys::CanBeSkipped,          cutscene.canBeSkipped);
		cutscene.videoFile          = cutsceneData.value(SceneKeys::VideoFile,             cutscene.videoFile);
		cutscene.waitForKey         = cutsceneData.value(SceneKeys::WaitForKey,            cutscene.waitForKey);
		cutscene.waitPrompt         = cutsceneData.value(SceneKeys::WaitPrompt,            cutscene.waitPrompt);
		cutscene.waitPresses        = cutsceneData.value(SceneKeys::WaitPresses,           cutscene.waitPresses);
		cutscene.pressWindowSeconds = cutsceneData.value(SceneKeys::PressWindowSeconds,    cutscene.pressWindowSeconds);
		cutscene.pressSound         = cutsceneData.value(SceneKeys::PressSound,            cutscene.pressSound);
		cutscene.blendInSeconds     = cutsceneData.value(SceneKeys::BlendInSeconds,        cutscene.blendInSeconds);
		cutscene.fadeSeconds        = cutsceneData.value(SceneKeys::FadeSeconds,           cutscene.fadeSeconds);
		cutscene.sound              = cutsceneData.value(SceneKeys::CutsceneSound,         cutscene.sound);
	}

	if (entityJson.contains(SceneKeys::EnvironmentComponent))
	{
		const auto& environmentData = entityJson[SceneKeys::EnvironmentComponent];
		auto& environment = entityManager.AddComponent<EnvironmentComponent>(entity);

		environment.skyboxFolder  = environmentData.value(SceneKeys::SkyboxFolder,  environment.skyboxFolder);
		environment.skyBrightness = environmentData.value(SceneKeys::SkyBrightness, environment.skyBrightness);
		environment.isFogEnabled  = environmentData.value(SceneKeys::IsFogEnabled,  environment.isFogEnabled);
		environment.fogDensity    = environmentData.value(SceneKeys::FogDensity,    environment.fogDensity);

		environment.isPostProcessingEnabled = environmentData.value(SceneKeys::IsPostProcessingEnabled, environment.isPostProcessingEnabled);
		environment.exposure                = environmentData.value(SceneKeys::Exposure,                environment.exposure);
		environment.isBloomEnabled          = environmentData.value(SceneKeys::IsBloomEnabled,          environment.isBloomEnabled);
		environment.bloomThreshold          = environmentData.value(SceneKeys::BloomThreshold,          environment.bloomThreshold);
		environment.bloomIntensity          = environmentData.value(SceneKeys::BloomIntensity,          environment.bloomIntensity);
		environment.bloomBlurPasses         = environmentData.value(SceneKeys::BloomBlurPasses,         environment.bloomBlurPasses);
		environment.hitImpactFrame          = environmentData.value(SceneKeys::HitImpactFrame,          environment.hitImpactFrame);
		environment.pixelSize               = environmentData.value(SceneKeys::PixelSize,               environment.pixelSize);
		environment.paletteLevels           = environmentData.value(SceneKeys::PaletteLevels,           environment.paletteLevels);
		environment.hueShift                = environmentData.value(SceneKeys::HueShift,                environment.hueShift);
		environment.shadowHue               = environmentData.value(SceneKeys::ShadowHue,               environment.shadowHue);
		environment.lightHue                = environmentData.value(SceneKeys::LightHue,                environment.lightHue);
		environment.cleanLines              = environmentData.value(SceneKeys::CleanLines,              environment.cleanLines);
		environment.removeOrphans           = environmentData.value(SceneKeys::RemoveOrphans,           environment.removeOrphans);
		environment.ditherStrength          = environmentData.value(SceneKeys::DitherStrength,          environment.ditherStrength);
		environment.outlineStrength         = environmentData.value(SceneKeys::OutlineStrength,         environment.outlineStrength);
		if (environmentData.contains(SceneKeys::OutlineColor))
			environment.outlineColor = DeserializeVec3(environmentData[SceneKeys::OutlineColor], environment.outlineColor);
		environment.paintRadius             = environmentData.value(SceneKeys::PaintRadius,             environment.paintRadius);
		environment.saturation              = environmentData.value(SceneKeys::Saturation,              environment.saturation);
		environment.contrast                = environmentData.value(SceneKeys::Contrast,                environment.contrast);
		environment.vignette                = environmentData.value(SceneKeys::Vignette,                environment.vignette);
		if (environmentData.contains(SceneKeys::ColorTint))
			environment.colorTint = DeserializeVec3(environmentData[SceneKeys::ColorTint], environment.colorTint);

		if (environmentData.contains(SceneKeys::FogColor))
			environment.fogColor = DeserializeVec3(environmentData[SceneKeys::FogColor]);
	}

	if (entityJson.contains(SceneKeys::WeaponTrailComponent))
	{
		const auto& trailData = entityJson[SceneKeys::WeaponTrailComponent];
		auto& trail = entityManager.AddComponent<WeaponTrailComponent>(entity);

		// A missing key keeps the default written in WeaponTrailComponent.h
		trail.intensity         = trailData.value(SceneKeys::ParticleIntensity, trail.intensity);
		trail.lifetimeSeconds   = trailData.value(SceneKeys::LifetimeSeconds,   trail.lifetimeSeconds);
		trail.onlyWhileStriking = trailData.value(SceneKeys::OnlyWhileStriking, trail.onlyWhileStriking);

		if (trailData.contains(SceneKeys::BaseOffset))
			trail.baseOffset = DeserializeVec3(trailData[SceneKeys::BaseOffset]);
		if (trailData.contains(SceneKeys::TipOffset))
			trail.tipOffset = DeserializeVec3(trailData[SceneKeys::TipOffset]);
		if (trailData.contains(SceneKeys::Color))
			trail.color = DeserializeVec3(trailData[SceneKeys::Color]);
	}

	if (entityJson.contains(SceneKeys::ParticleEmitterComponent))
	{
		const auto& emitterData = entityJson[SceneKeys::ParticleEmitterComponent];
		auto& emitter = entityManager.AddComponent<ParticleEmitterComponent>(entity);

		emitter.isEmitting         = emitterData.value(SceneKeys::IsEmitting,         emitter.isEmitting);
		emitter.particlesPerSecond = emitterData.value(SceneKeys::ParticlesPerSecond, emitter.particlesPerSecond);
		emitter.lifetimeSeconds    = emitterData.value(SceneKeys::LifetimeSeconds,    emitter.lifetimeSeconds);
		emitter.velocityRandomness = emitterData.value(SceneKeys::VelocityRandomness, emitter.velocityRandomness);
		emitter.startSize          = emitterData.value(SceneKeys::StartSize,          emitter.startSize);
		emitter.endSize            = emitterData.value(SceneKeys::EndSize,            emitter.endSize);
		emitter.intensity          = emitterData.value(SceneKeys::ParticleIntensity,  emitter.intensity);
		emitter.maxParticles       = emitterData.value(SceneKeys::MaxParticles,       emitter.maxParticles);
		emitter.isDark             = emitterData.value(SceneKeys::IsDark,             emitter.isDark);
		emitter.style              = emitterData.value(SceneKeys::ParticleStyle,      emitter.style);
		emitter.spawnShape         = emitterData.value(SceneKeys::SpawnShape,         emitter.spawnShape);
		emitter.spawnRadius        = emitterData.value(SceneKeys::SpawnRadius,        emitter.spawnRadius);
		emitter.outwardSpeed       = emitterData.value(SceneKeys::OutwardSpeed,       emitter.outwardSpeed);
		emitter.sizeRandomness     = emitterData.value(SceneKeys::SizeRandomness,     emitter.sizeRandomness);
		emitter.spinDegreesPerSecond = emitterData.value(SceneKeys::ParticleSpin,     emitter.spinDegreesPerSecond);

		if (emitterData.contains(SceneKeys::SpawnHalfExtents))
			emitter.spawnHalfExtents = DeserializeVec3(emitterData[SceneKeys::SpawnHalfExtents]);
		if (emitterData.contains(SceneKeys::ParticleVelocity))
			emitter.velocity = DeserializeVec3(emitterData[SceneKeys::ParticleVelocity]);
		if (emitterData.contains(SceneKeys::ParticleGravity))
			emitter.gravity = DeserializeVec3(emitterData[SceneKeys::ParticleGravity]);
		if (emitterData.contains(SceneKeys::StartColor))
			emitter.startColor = DeserializeVec3(emitterData[SceneKeys::StartColor]);
		if (emitterData.contains(SceneKeys::EndColor))
			emitter.endColor = DeserializeVec3(emitterData[SceneKeys::EndColor]);

		emitter.drag           = emitterData.value(SceneKeys::ParticleDrag,    emitter.drag);
		emitter.stretch        = emitterData.value(SceneKeys::ParticleStretch, emitter.stretch);
		emitter.hasMiddleColor = emitterData.value(SceneKeys::HasMiddleColor,  emitter.hasMiddleColor);
		emitter.burstAmount    = emitterData.value(SceneKeys::BurstAmount,     emitter.burstAmount);
		emitter.flipbookTexture     = emitterData.value(SceneKeys::FlipbookTexture,     emitter.flipbookTexture);
		emitter.flipbookColumns     = emitterData.value(SceneKeys::FlipbookColumns,     emitter.flipbookColumns);
		emitter.flipbookRows        = emitterData.value(SceneKeys::FlipbookRows,        emitter.flipbookRows);
		emitter.flipbookLoops       = emitterData.value(SceneKeys::FlipbookLoops,       emitter.flipbookLoops);
		emitter.flipbookRandomStart = emitterData.value(SceneKeys::FlipbookRandomStart, emitter.flipbookRandomStart);
		if (emitterData.contains(SceneKeys::MiddleColor))
			emitter.middleColor = DeserializeVec3(emitterData[SceneKeys::MiddleColor]);

		emitter.wasEmittingAtStart = emitter.isEmitting;
	}

	if (entityJson.contains(SceneKeys::EffectShapeComponent))
	{
		const auto& shapeData = entityJson[SceneKeys::EffectShapeComponent];
		auto& shape = entityManager.AddComponent<EffectShapeComponent>(entity);

		shape.kind                 = shapeData.value(SceneKeys::ShapeKind,           shape.kind);
		shape.lifetimeSeconds      = shapeData.value(SceneKeys::ShapeLifetime,       shape.lifetimeSeconds);
		shape.startRadius          = shapeData.value(SceneKeys::ShapeStartRadius,    shape.startRadius);
		shape.endRadius            = shapeData.value(SceneKeys::ShapeEndRadius,      shape.endRadius);
		shape.thickness            = shapeData.value(SceneKeys::ShapeThickness,      shape.thickness);
		shape.height               = shapeData.value(SceneKeys::ShapeHeight,         shape.height);
		shape.arcDegrees           = shapeData.value(SceneKeys::ShapeArcDegrees,     shape.arcDegrees);
		shape.spinDegreesPerSecond = shapeData.value(SceneKeys::ShapeSpin,           shape.spinDegreesPerSecond);
		shape.twistDegrees         = shapeData.value(SceneKeys::ShapeTwist,          shape.twistDegrees);
		shape.intensity            = shapeData.value(SceneKeys::ShapeIntensity,      shape.intensity);
		shape.noiseScale           = shapeData.value(SceneKeys::ShapeNoiseScale,     shape.noiseScale);
		shape.scrollSpeed          = shapeData.value(SceneKeys::ShapeScrollSpeed,    shape.scrollSpeed);
		shape.dissolve             = shapeData.value(SceneKeys::ShapeDissolve,       shape.dissolve);
		shape.lightIntensity       = shapeData.value(SceneKeys::ShapeLightIntensity, shape.lightIntensity);
		shape.lightRadius          = shapeData.value(SceneKeys::ShapeLightRadius,    shape.lightRadius);

		if (shapeData.contains(SceneKeys::ShapeRotation))
			shape.rotationDegrees = DeserializeVec3(shapeData[SceneKeys::ShapeRotation]);
		if (shapeData.contains(SceneKeys::ShapeRotationRandomness))
			shape.rotationRandomness = DeserializeVec3(shapeData[SceneKeys::ShapeRotationRandomness]);
		if (shapeData.contains(SceneKeys::ShapeOffset))
			shape.offset = DeserializeVec3(shapeData[SceneKeys::ShapeOffset]);
		if (shapeData.contains(SceneKeys::ShapeCoreColor))
			shape.coreColor = DeserializeVec3(shapeData[SceneKeys::ShapeCoreColor]);
		if (shapeData.contains(SceneKeys::ShapeEdgeColor))
			shape.edgeColor = DeserializeVec3(shapeData[SceneKeys::ShapeEdgeColor]);
		if (shapeData.contains(SceneKeys::ShapeLightColor))
			shape.lightColor = DeserializeVec3(shapeData[SceneKeys::ShapeLightColor]);
	}

	if (entityJson.contains(SceneKeys::GlowSpriteComponent))
	{
		const auto& spriteData = entityJson[SceneKeys::GlowSpriteComponent];
		auto& sprite = entityManager.AddComponent<GlowSpriteComponent>(entity);

		sprite.isVisible = spriteData.value(SceneKeys::IsVisible,         sprite.isVisible);
		sprite.size      = spriteData.value(SceneKeys::SpriteSize,        sprite.size);
		sprite.intensity = spriteData.value(SceneKeys::ParticleIntensity, sprite.intensity);

		if (spriteData.contains(SceneKeys::SpriteColor))
			sprite.color = DeserializeVec3(spriteData[SceneKeys::SpriteColor]);
	}

	if (entityJson.contains(SceneKeys::LiquidOrbComponent))
	{
		const auto& orbData = entityJson[SceneKeys::LiquidOrbComponent];
		auto& orb = entityManager.AddComponent<LiquidOrbComponent>(entity);

		orb.isVisible   = orbData.value(SceneKeys::IsVisible,      orb.isVisible);
		orb.brightness  = orbData.value(SceneKeys::OrbBrightness,  orb.brightness);
		orb.wobble      = orbData.value(SceneKeys::OrbWobble,      orb.wobble);
		orb.wobbleSpeed = orbData.value(SceneKeys::OrbWobbleSpeed, orb.wobbleSpeed);
		orb.clearness   = orbData.value(SceneKeys::OrbClearness,   orb.clearness);

		if (orbData.contains(SceneKeys::OrbTint))
			orb.tint = DeserializeVec3(orbData[SceneKeys::OrbTint]);
	}

	if (entityJson.contains(SceneKeys::FoliageComponent))
	{
		const auto& data = entityJson[SceneKeys::FoliageComponent];
		auto& foliage = entityManager.AddComponent<FoliageComponent>(entity);
		foliage.modelName     = data.value(SceneKeys::FoliageModel,         foliage.modelName);
		foliage.count         = data.value(SceneKeys::FoliageCount,         foliage.count);
		foliage.areaSizeX     = data.value(SceneKeys::FoliageAreaX,         foliage.areaSizeX);
		foliage.areaSizeZ     = data.value(SceneKeys::FoliageAreaZ,         foliage.areaSizeZ);
		foliage.densityMask   = data.value(SceneKeys::FoliageMask,          foliage.densityMask);
		foliage.minScale      = data.value(SceneKeys::FoliageMinScale,      foliage.minScale);
		foliage.maxScale      = data.value(SceneKeys::FoliageMaxScale,      foliage.maxScale);
		foliage.sway          = data.value(SceneKeys::FoliageSway,          foliage.sway);
		foliage.swaySpeed     = data.value(SceneKeys::FoliageSwaySpeed,     foliage.swaySpeed);
		foliage.drawDistance  = data.value(SceneKeys::FoliageDrawDistance,  foliage.drawDistance);
		if (data.contains(SceneKeys::FoliageTint) && data[SceneKeys::FoliageTint].size() == 3)
			foliage.tint = glm::vec3(data[SceneKeys::FoliageTint][0], data[SceneKeys::FoliageTint][1], data[SceneKeys::FoliageTint][2]);
		foliage.tintVariation = data.value(SceneKeys::FoliageTintVariation, foliage.tintVariation);
		foliage.seed          = data.value(SceneKeys::FoliageSeed,          foliage.seed);
	}

	if (entityJson.contains(SceneKeys::SwarmComponent))
	{
		const auto& swarmData = entityJson[SceneKeys::SwarmComponent];
		auto& swarm = entityManager.AddComponent<SwarmComponent>(entity);

		swarm.isVisible              = swarmData.value(SceneKeys::IsVisible,           swarm.isVisible);
		swarm.modelName              = swarmData.value(SceneKeys::SwarmModelName,      swarm.modelName);
		swarm.count                  = swarmData.value(SceneKeys::SwarmCount,          swarm.count);
		swarm.seed                   = swarmData.value(SceneKeys::SwarmSeed,           swarm.seed);
		swarm.pieceSize              = swarmData.value(SceneKeys::SwarmPieceSize,      swarm.pieceSize);
		swarm.sizeVariation          = swarmData.value(SceneKeys::SwarmSizeVariation,  swarm.sizeVariation);
		swarm.radius                 = swarmData.value(SceneKeys::SwarmRadius,         swarm.radius);
		swarm.shellThickness         = swarmData.value(SceneKeys::SwarmShellThickness, swarm.shellThickness);
		swarm.spinDegreesPerSecond   = swarmData.value(SceneKeys::SwarmSpin,           swarm.spinDegreesPerSecond);
		swarm.tumbleDegreesPerSecond = swarmData.value(SceneKeys::SwarmTumble,         swarm.tumbleDegreesPerSecond);
		swarm.fallSpeed              = swarmData.value(SceneKeys::SwarmFallSpeed,      swarm.fallSpeed);
		swarm.fallSpreadSeconds      = swarmData.value(SceneKeys::SwarmFallSpread,     swarm.fallSpreadSeconds);
		swarm.groundHeight           = swarmData.value(SceneKeys::SwarmGroundHeight,   swarm.groundHeight);
		swarm.stuckSeconds           = swarmData.value(SceneKeys::SwarmStuckSeconds,   swarm.stuckSeconds);
		swarm.trailLength            = swarmData.value(SceneKeys::SwarmTrailLength,    swarm.trailLength);
		swarm.hitEffect              = swarmData.value(SceneKeys::SwarmHitEffect,      swarm.hitEffect);
		swarm.hitBurst               = swarmData.value(SceneKeys::SwarmHitBurst,       swarm.hitBurst);

		if (swarmData.contains(SceneKeys::SwarmSkyCenter))
			swarm.skyCenter = DeserializeVec3(swarmData[SceneKeys::SwarmSkyCenter]);
		if (swarmData.contains(SceneKeys::SwarmSkyHalfExtents))
			swarm.skyHalfExtents = DeserializeVec3(swarmData[SceneKeys::SwarmSkyHalfExtents]);
		if (swarmData.contains(SceneKeys::SwarmGlowColor))
			swarm.glowColor = DeserializeVec3(swarmData[SceneKeys::SwarmGlowColor]);
	}

	if (entityJson.contains(SceneKeys::FogGateComponent))
	{
		const auto& fogData = entityJson[SceneKeys::FogGateComponent];
		auto& fogGate = entityManager.AddComponent<FogGateComponent>(entity);

		if (fogData.contains(SceneKeys::ArenaCenter))
			fogGate.arenaCenter = DeserializeVec3(fogData[SceneKeys::ArenaCenter]);

		if (fogData.contains(SceneKeys::ArenaHalfExtents))
			fogGate.arenaHalfExtents = DeserializeVec3(fogData[SceneKeys::ArenaHalfExtents]);

		fogGate.victoryMessage   = fogData.value(SceneKeys::VictoryMessage,   fogGate.victoryMessage);
		fogGate.isCrossedWithKey = fogData.value(SceneKeys::IsCrossedWithKey, fogGate.isCrossedWithKey);
		fogGate.cutscene         = fogData.value(SceneKeys::FogCutscene,      fogGate.cutscene);
		fogGate.battleMusic       = fogData.value(SceneKeys::BattleMusic,       fogGate.battleMusic);
		fogGate.battleMusicVolume = fogData.value(SceneKeys::BattleMusicVolume, fogGate.battleMusicVolume);
		fogGate.victorySound      = fogData.value(SceneKeys::VictorySound,      fogGate.victorySound);
	}

	if (entityJson.contains(SceneKeys::HealingFlaskComponent))
	{
		const auto& flaskData = entityJson[SceneKeys::HealingFlaskComponent];
		auto& flask = entityManager.AddComponent<HealingFlaskComponent>(entity);

		flask.displayName    = flaskData.value(SceneKeys::DisplayName,    flask.displayName);
		flask.maxCharges     = flaskData.value(SceneKeys::MaxCharges,     flask.maxCharges);
		flask.healAmount     = flaskData.value(SceneKeys::HealAmount,     flask.healAmount);
		flask.drinkAnimation = flaskData.value(SceneKeys::DrinkAnimation, flask.drinkAnimation);
		flask.healAtProgress = flaskData.value(SceneKeys::HealAtProgress, flask.healAtProgress);
		flask.hiddenWhileDrinking = flaskData.value(SceneKeys::HiddenWhileDrinking, flask.hiddenWhileDrinking);
		flask.shownWhileDrinking  = flaskData.value(SceneKeys::ShownWhileDrinking,  flask.shownWhileDrinking);
		flask.healEffect     = flaskData.value(SceneKeys::HealEffect,     flask.healEffect);
		flask.drinkSound     = flaskData.value(SceneKeys::DrinkSound,     flask.drinkSound);
		flask.healSound      = flaskData.value(SceneKeys::HealSound,      flask.healSound);
		flask.charges        = flask.maxCharges;
	}

	if (entityJson.contains(SceneKeys::CurrencyComponent))
	{
		const auto& currencyData = entityJson[SceneKeys::CurrencyComponent];
		auto& currency = entityManager.AddComponent<CurrencyComponent>(entity);

		currency.displayName        = currencyData.value(SceneKeys::DisplayName,        currency.displayName);
		currency.amount             = currencyData.value(SceneKeys::CurrencyAmount,     currency.amount);
		currency.isDroppedOnDeath   = currencyData.value(SceneKeys::IsDroppedOnDeath,   currency.isDroppedOnDeath);
		currency.dropMarker         = currencyData.value(SceneKeys::DropMarker,         currency.dropMarker);
		currency.retrieveDistance   = currencyData.value(SceneKeys::RetrieveDistance,   currency.retrieveDistance);
		currency.isRetrievedWithKey = currencyData.value(SceneKeys::IsRetrievedWithKey, currency.isRetrievedWithKey);
		currency.retrievePrompt     = currencyData.value(SceneKeys::RetrievePrompt,     currency.retrievePrompt);
		currency.gainSound          = currencyData.value(SceneKeys::GainSound,          currency.gainSound);
		currency.retrieveSound      = currencyData.value(SceneKeys::RetrieveSound,      currency.retrieveSound);
	}

	if (entityJson.contains(SceneKeys::CurrencyRewardComponent))
	{
		const auto& rewardData = entityJson[SceneKeys::CurrencyRewardComponent];
		auto& reward = entityManager.AddComponent<CurrencyRewardComponent>(entity);

		reward.amount = rewardData.value(SceneKeys::CurrencyAmount, reward.amount);
	}

	if (entityJson.contains(SceneKeys::LevelUpComponent))
	{
		const auto& levelData = entityJson[SceneKeys::LevelUpComponent];
		auto& levelUp = entityManager.AddComponent<LevelUpComponent>(entity);

		levelUp.displayName            = levelData.value(SceneKeys::DisplayName,            levelUp.displayName);
		levelUp.level                  = levelData.value(SceneKeys::Level,                  levelUp.level);
		levelUp.firstLevelCost         = levelData.value(SceneKeys::FirstLevelCost,         levelUp.firstLevelCost);
		levelUp.costGrowth             = levelData.value(SceneKeys::CostGrowth,             levelUp.costGrowth);
		levelUp.startingAttributeValue = levelData.value(SceneKeys::StartingAttributeValue, levelUp.startingAttributeValue);
		levelUp.healthAttribute        = levelData.value(SceneKeys::HealthAttribute,        levelUp.healthAttribute);
		levelUp.healthPerPoint         = levelData.value(SceneKeys::HealthPerPoint,         levelUp.healthPerPoint);
		levelUp.healthPoints           = levelData.value(SceneKeys::HealthPoints,           levelUp.healthPoints);
		levelUp.staminaAttribute       = levelData.value(SceneKeys::StaminaAttribute,       levelUp.staminaAttribute);
		levelUp.staminaPerPoint        = levelData.value(SceneKeys::StaminaPerPoint,        levelUp.staminaPerPoint);
		levelUp.staminaPoints          = levelData.value(SceneKeys::StaminaPoints,          levelUp.staminaPoints);
		levelUp.damageAttribute        = levelData.value(SceneKeys::DamageAttribute,        levelUp.damageAttribute);
		levelUp.damagePerPoint         = levelData.value(SceneKeys::DamagePerPoint,         levelUp.damagePerPoint);
		levelUp.damagePoints           = levelData.value(SceneKeys::DamagePoints,           levelUp.damagePoints);
		levelUp.levelUpSound           = levelData.value(SceneKeys::LevelUpSound,           levelUp.levelUpSound);
		levelUp.levelUpEffect          = levelData.value(SceneKeys::LevelUpEffect,          levelUp.levelUpEffect);
	}

	if (entityJson.contains(SceneKeys::AudioSourceComponent))
	{
		const auto& sourceData = entityJson[SceneKeys::AudioSourceComponent];
		auto& audioSource = entityManager.AddComponent<AudioSourceComponent>(entity);

		audioSource.sound       = sourceData.value(SceneKeys::Sound,       audioSource.sound);
		audioSource.volume      = sourceData.value(SceneKeys::SoundVolume, audioSource.volume);
		audioSource.isLooping   = sourceData.value(SceneKeys::IsLooping,   audioSource.isLooping);
		audioSource.playOnStart = sourceData.value(SceneKeys::PlayOnStart, audioSource.playOnStart);
		audioSource.isSpatial   = sourceData.value(SceneKeys::IsSpatial,   audioSource.isSpatial);
		audioSource.minDistance = sourceData.value(SceneKeys::MinDistance, audioSource.minDistance);
		audioSource.maxDistance = sourceData.value(SceneKeys::MaxDistance, audioSource.maxDistance);
	}

	if (entityJson.contains(SceneKeys::CharacterSoundsComponent))
	{
		const auto& soundsData = entityJson[SceneKeys::CharacterSoundsComponent];
		auto& sounds = entityManager.AddComponent<CharacterSoundsComponent>(entity);

		sounds.attackSound    = soundsData.value(SceneKeys::AttackSound,    sounds.attackSound);
		sounds.hurtSound      = soundsData.value(SceneKeys::HurtSound,      sounds.hurtSound);
		sounds.blockSound     = soundsData.value(SceneKeys::BlockSound,     sounds.blockSound);
		sounds.parrySound     = soundsData.value(SceneKeys::ParrySound,     sounds.parrySound);
		sounds.deathSound     = soundsData.value(SceneKeys::DeathSound,     sounds.deathSound);
		sounds.rollSound      = soundsData.value(SceneKeys::RollSound,      sounds.rollSound);
		sounds.footstepSound  = soundsData.value(SceneKeys::FootstepSound,  sounds.footstepSound);
		sounds.shockwaveSound = soundsData.value(SceneKeys::ShockwaveSound, sounds.shockwaveSound);
		sounds.attackSoundLeadSeconds = soundsData.value(SceneKeys::AttackSoundLeadSeconds, sounds.attackSoundLeadSeconds);
		sounds.leftFootBone   = soundsData.value(SceneKeys::LeftFootBone,   sounds.leftFootBone);
		sounds.rightFootBone  = soundsData.value(SceneKeys::RightFootBone,  sounds.rightFootBone);
		sounds.footstepMeters = soundsData.value(SceneKeys::FootstepMeters, sounds.footstepMeters);
		sounds.footstepCameraShake = soundsData.value(SceneKeys::FootstepCameraShake, sounds.footstepCameraShake);
		sounds.footstepShakeRadius = soundsData.value(SceneKeys::FootstepShakeRadius, sounds.footstepShakeRadius);
		sounds.volume         = soundsData.value(SceneKeys::SoundVolume,    sounds.volume);
		sounds.painVoice      = soundsData.value(SceneKeys::PainVoice,      sounds.painVoice);
		sounds.attackVoice    = soundsData.value(SceneKeys::AttackVoice,    sounds.attackVoice);
		sounds.deathVoice     = soundsData.value(SceneKeys::DeathVoice,     sounds.deathVoice);
		sounds.alertVoice     = soundsData.value(SceneKeys::AlertVoice,     sounds.alertVoice);
		sounds.painVoiceChance   = soundsData.value(SceneKeys::PainVoiceChance,   sounds.painVoiceChance);
		sounds.attackVoiceChance = soundsData.value(SceneKeys::AttackVoiceChance, sounds.attackVoiceChance);
	}

	if (entityJson.contains(SceneKeys::PoiseComponent))
	{
		const auto& poiseData = entityJson[SceneKeys::PoiseComponent];
		auto& poise = entityManager.AddComponent<PoiseComponent>(entity);

		poise.maxPoise            = poiseData.value(SceneKeys::MaxPoise,            poise.maxPoise);
		poise.recoverAfterSeconds = poiseData.value(SceneKeys::RecoverAfterSeconds, poise.recoverAfterSeconds);
		poise.staggerImmunitySeconds = poiseData.value(SceneKeys::StaggerImmunitySeconds, poise.staggerImmunitySeconds);
		poise.currentPoise        = poise.maxPoise;
	}

	if (entityJson.contains(SceneKeys::TerrainComponent))
	{
		const auto& data = entityJson[SceneKeys::TerrainComponent];
		auto& terrain = entityManager.AddComponent<TerrainComponent>(entity);
		terrain.heightmap   = data.value(SceneKeys::Heightmap,    terrain.heightmap);
		terrain.sizeX       = data.value(SceneKeys::TerrainSizeX, terrain.sizeX);
		terrain.sizeZ       = data.value(SceneKeys::TerrainSizeZ, terrain.sizeZ);
		terrain.heightScale = data.value(SceneKeys::HeightScale,  terrain.heightScale);
	}

	if (entityJson.contains(SceneKeys::PickupComponent))
	{
		const auto& data = entityJson[SceneKeys::PickupComponent];
		auto& pickup = entityManager.AddComponent<PickupComponent>(entity);
		pickup.itemId           = data.value(SceneKeys::ItemId,                 pickup.itemId);
		pickup.displayName      = data.value(SceneKeys::DisplayName,            pickup.displayName);
		pickup.prompt           = data.value(SceneKeys::PickupPrompt,           pickup.prompt);
		pickup.obtainedText     = data.value(SceneKeys::PickupObtainedText,     pickup.obtainedText);
		pickup.interactDistance = data.value(SceneKeys::InteractDistancePickup, pickup.interactDistance);
		pickup.readText         = data.value(SceneKeys::ReadText,               pickup.readText);
		pickup.voiceSound       = data.value(SceneKeys::VoiceSound,             pickup.voiceSound);
		pickup.readSeconds      = data.value(SceneKeys::ReadSeconds,            pickup.readSeconds);
		pickup.pickupSound      = data.value(SceneKeys::PickupSound,            pickup.pickupSound);
	}

	if (entityJson.contains(SceneKeys::InventoryComponent))
		entityManager.AddComponent<InventoryComponent>(entity);

	if (entityJson.contains(SceneKeys::LootDropComponent))
	{
		const auto& data = entityJson[SceneKeys::LootDropComponent];
		auto& loot = entityManager.AddComponent<LootDropComponent>(entity);
		loot.pickupTemplate = data.value(SceneKeys::PickupTemplate, loot.pickupTemplate);
		loot.chance         = data.value(SceneKeys::LootChance,     loot.chance);
		loot.onlyOnce       = data.value(SceneKeys::OnlyOnce,       loot.onlyOnce);
	}

	if (entityJson.contains(SceneKeys::MountComponent))
	{
		const auto& data = entityJson[SceneKeys::MountComponent];
		auto& mount = entityManager.AddComponent<MountComponent>(entity);
		if (data.contains(SceneKeys::SeatOffset) && data[SceneKeys::SeatOffset].is_array() && data[SceneKeys::SeatOffset].size() == 3)
			mount.seatOffset = glm::vec3(data[SceneKeys::SeatOffset][0].get<float>(), data[SceneKeys::SeatOffset][1].get<float>(),
			                             data[SceneKeys::SeatOffset][2].get<float>());
		mount.mountDistance         = data.value(SceneKeys::MountDistance,         mount.mountDistance);
		mount.mountPrompt           = data.value(SceneKeys::MountPrompt,           mount.mountPrompt);
		mount.dismountPrompt        = data.value(SceneKeys::DismountPrompt,        mount.dismountPrompt);
		mount.riderIdleAnimation    = data.value(SceneKeys::RiderIdleAnimation,    mount.riderIdleAnimation);
		mount.riderWalkAnimation    = data.value(SceneKeys::RiderWalkAnimation,    mount.riderWalkAnimation);
		mount.riderRunAnimation     = data.value(SceneKeys::RiderRunAnimation,     mount.riderRunAnimation);
		mount.runAnimationFromSpeed = data.value(SceneKeys::RunAnimationFromSpeed, mount.runAnimationFromSpeed);
		mount.cameraRise            = data.value(SceneKeys::CameraRise,            mount.cameraRise);
		mount.cameraPullBack        = data.value(SceneKeys::CameraPullBack,        mount.cameraPullBack);
	}

	if (entityJson.contains(SceneKeys::SummonComponent))
	{
		const auto& data = entityJson[SceneKeys::SummonComponent];
		auto& summon = entityManager.AddComponent<SummonComponent>(entity);
		summon.allies          = data.value(SceneKeys::SummonAllies,     summon.allies);
		summon.maxCharges      = data.value(SceneKeys::SummonMaxCharges, summon.maxCharges);
		summon.lifetimeSeconds = data.value(SceneKeys::SummonLifetime,   summon.lifetimeSeconds);
		summon.summonEffect    = data.value(SceneKeys::SummonEffect,     summon.summonEffect);
		summon.calledText      = data.value(SceneKeys::SummonCalledText, summon.calledText);
	}

	if (entityJson.contains(SceneKeys::WrathComponent))
	{
		const auto& data = entityJson[SceneKeys::WrathComponent];
		auto& wrath = entityManager.AddComponent<WrathComponent>(entity);
		wrath.maxWrath          = data.value(SceneKeys::MaxWrath,               wrath.maxWrath);
		wrath.fromDamageDealt   = data.value(SceneKeys::WrathFromDamageDealt,   wrath.fromDamageDealt);
		wrath.fromDamageTaken   = data.value(SceneKeys::WrathFromDamageTaken,   wrath.fromDamageTaken);
		wrath.fromDeflects      = data.value(SceneKeys::WrathFromDeflects,      wrath.fromDeflects);
		wrath.drainPerSecond    = data.value(SceneKeys::WrathDrainPerSecond,    wrath.drainPerSecond);
		wrath.drainDelaySeconds = data.value(SceneKeys::WrathDrainDelay,        wrath.drainDelaySeconds);
		wrath.durationSeconds   = data.value(SceneKeys::WrathDuration,          wrath.durationSeconds);
		wrath.damageMultiplier  = data.value(SceneKeys::WrathDamageMultiplier,  wrath.damageMultiplier);
		wrath.postureMultiplier = data.value(SceneKeys::WrathPostureMultiplier, wrath.postureMultiplier);
		if (data.contains(SceneKeys::WrathGlowColor) && data[SceneKeys::WrathGlowColor].size() == 3)
			wrath.glowColor = glm::vec3(data[SceneKeys::WrathGlowColor][0], data[SceneKeys::WrathGlowColor][1], data[SceneKeys::WrathGlowColor][2]);
		wrath.releaseEffect     = data.value(SceneKeys::WrathReleaseEffect,     wrath.releaseEffect);
	}

	if (entityJson.contains(SceneKeys::PostureComponent))
	{
		const auto& data = entityJson[SceneKeys::PostureComponent];
		auto& posture = entityManager.AddComponent<PostureComponent>(entity);

		posture.maxPosture             = data.value(SceneKeys::MaxPosture,              posture.maxPosture);
		posture.fromHits               = data.value(SceneKeys::PostureFromHits,         posture.fromHits);
		posture.fromBlocks             = data.value(SceneKeys::PostureFromBlocks,       posture.fromBlocks);
		posture.fromBeingParried       = data.value(SceneKeys::PostureFromBeingParried, posture.fromBeingParried);
		posture.recoverPerSecond       = data.value(SceneKeys::PostureRecoverPerSecond, posture.recoverPerSecond);
		posture.recoverDelaySeconds    = data.value(SceneKeys::PostureRecoverDelay,     posture.recoverDelaySeconds);
		posture.lowHealthRecoverFactor = data.value(SceneKeys::PostureLowHealthRecover, posture.lowHealthRecoverFactor);
		posture.brokenSeconds          = data.value(SceneKeys::PostureBrokenSeconds,    posture.brokenSeconds);
		posture.finisherDamage         = data.value(SceneKeys::FinisherDamage,          posture.finisherDamage);
		posture.finisherSlowMotion     = data.value(SceneKeys::FinisherSlowMotion,      posture.finisherSlowMotion);
		posture.finisherSlowSeconds    = data.value(SceneKeys::FinisherSlowSeconds,     posture.finisherSlowSeconds);
		posture.breakEffect            = data.value(SceneKeys::PostureBreakEffect,      posture.breakEffect);
		posture.finisherEffect         = data.value(SceneKeys::FinisherEffect,          posture.finisherEffect);
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
		enemy.startsDormant     = enemyData.value(SceneKeys::StartsDormant,     enemy.startsDormant);
		enemy.isAlly            = enemyData.value(SceneKeys::IsAlly,            enemy.isAlly);
	}

	if (entityJson.contains(SceneKeys::BonfireComponent))
	{
		const auto& bonfireData = entityJson[SceneKeys::BonfireComponent];
		auto& bonfire = entityManager.AddComponent<BonfireComponent>(entity);

		bonfire.interactDistance = bonfireData.value(SceneKeys::InteractDistance, bonfire.interactDistance);
		bonfire.restPrompt       = bonfireData.value(SceneKeys::RestPrompt,       bonfire.restPrompt);
		bonfire.leavePrompt      = bonfireData.value(SceneKeys::LeavePrompt,      bonfire.leavePrompt);
		bonfire.litMessage       = bonfireData.value(SceneKeys::LitMessage,       bonfire.litMessage);

		if (bonfireData.contains(SceneKeys::RespawnOffset))
			bonfire.respawnOffset = DeserializeVec3(bonfireData[SceneKeys::RespawnOffset]);
	}

	if (entityJson.contains(SceneKeys::RespawnComponent))
	{
		auto& respawn = entityManager.AddComponent<RespawnComponent>(entity);
		respawn.respawnDelaySeconds = entityJson[SceneKeys::RespawnComponent].value(SceneKeys::RespawnDelaySeconds, respawn.respawnDelaySeconds);
	}

	if (entityJson.contains(SceneKeys::PlayerHUDComponent))
	{
		const auto& hudData = entityJson[SceneKeys::PlayerHUDComponent];
		auto& hud = entityManager.AddComponent<PlayerHUDComponent>(entity);

		// A missing key keeps the default written in PlayerHUDComponent.h
		hud.pixelsPerHealthPoint  = hudData.value(SceneKeys::PixelsPerHealthPoint,  hud.pixelsPerHealthPoint);
		hud.pixelsPerStaminaPoint = hudData.value(SceneKeys::PixelsPerStaminaPoint, hud.pixelsPerStaminaPoint);
		hud.showLockOnMarker      = hudData.value(SceneKeys::ShowLockOnMarker,      hud.showLockOnMarker);
		hud.deathMessage          = hudData.value(SceneKeys::DeathMessage,          hud.deathMessage);
		hud.style                 = hudData.value(SceneKeys::HUDStyleFile,          hud.style);
	}

	if (entityJson.contains(SceneKeys::EnemyHealthBarComponent))
	{
		const auto& barData = entityJson[SceneKeys::EnemyHealthBarComponent];
		auto& healthBar = entityManager.AddComponent<EnemyHealthBarComponent>(entity);

		// A missing key keeps the default written in EnemyHealthBarComponent.h
		healthBar.isBoss              = barData.value(SceneKeys::IsBoss,              healthBar.isBoss);
		healthBar.displayName         = barData.value(SceneKeys::DisplayName,         healthBar.displayName);
		healthBar.showSecondsAfterHit = barData.value(SceneKeys::ShowSecondsAfterHit, healthBar.showSecondsAfterHit);
		healthBar.heightAboveFeet     = barData.value(SceneKeys::HeightAboveFeet,     healthBar.heightAboveFeet);
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
		meleeEnemy.finisherChance        = meleeData.value(SceneKeys::FinisherChance,        meleeEnemy.finisherChance);
		meleeEnemy.recoverSeconds        = meleeData.value(SceneKeys::RecoverSeconds,        meleeEnemy.recoverSeconds);
		meleeEnemy.dodgeChance           = meleeData.value(SceneKeys::DodgeChance,           meleeEnemy.dodgeChance);
		meleeEnemy.dodgeDistance         = meleeData.value(SceneKeys::DodgeDistance,         meleeEnemy.dodgeDistance);
		meleeEnemy.dodgeCooldownSeconds  = meleeData.value(SceneKeys::DodgeCooldownSeconds,  meleeEnemy.dodgeCooldownSeconds);
		meleeEnemy.guardChance           = meleeData.value(SceneKeys::GuardChance,           meleeEnemy.guardChance);
		meleeEnemy.guardSeconds          = meleeData.value(SceneKeys::GuardSeconds,          meleeEnemy.guardSeconds);
		meleeEnemy.leapAttackDistance    = meleeData.value(SceneKeys::LeapAttackDistance,    meleeEnemy.leapAttackDistance);
	}

	if (entityJson.contains(SceneKeys::SpellCasterComponent))
	{
		const auto& spellData = entityJson[SceneKeys::SpellCasterComponent];
		auto& spell = entityManager.AddComponent<SpellCasterComponent>(entity);

		// A missing key keeps the default written in SpellCasterComponent.h
		spell.castAnimation            = spellData.value(SceneKeys::CastAnimation,            spell.castAnimation);
		spell.projectile               = spellData.value(SceneKeys::ProjectileName,           spell.projectile);
		spell.projectileCount          = spellData.value(SceneKeys::ProjectileCount,          spell.projectileCount);
		spell.spreadDegrees            = spellData.value(SceneKeys::SpreadDegrees,            spell.spreadDegrees);
		spell.releaseAt                = spellData.value(SceneKeys::ReleaseAt,                spell.releaseAt);
		spell.atTargetsFeet            = spellData.value(SceneKeys::AtTargetsFeet,            spell.atTargetsFeet);
		spell.dropHeight               = spellData.value(SceneKeys::DropHeight,               spell.dropHeight);
		spell.minDistance              = spellData.value(SceneKeys::MinDistance,              spell.minDistance);
		spell.maxDistance              = spellData.value(SceneKeys::MaxDistance,              spell.maxDistance);
		spell.cooldownSeconds          = spellData.value(SceneKeys::SpellCooldownSeconds,     spell.cooldownSeconds);
		spell.chargeEffect             = spellData.value(SceneKeys::ChargeEffect,             spell.chargeEffect);
		spell.chargeParticlesPerSecond = spellData.value(SceneKeys::ChargeParticlesPerSecond, spell.chargeParticlesPerSecond);
		spell.castSound                = spellData.value(SceneKeys::CastSound,                spell.castSound);
		spell.castKey                  = spellData.value(SceneKeys::CastKey,                  spell.castKey);
		spell.displayName              = spellData.value(SceneKeys::DisplayName,              spell.displayName);
		spell.isCastOnRequestOnly      = spellData.value(SceneKeys::IsCastOnRequestOnly,      spell.isCastOnRequestOnly);
		spell.isUninterruptible        = spellData.value(SceneKeys::IsUninterruptible,        spell.isUninterruptible);
	}

	if (entityJson.contains(SceneKeys::ProjectileComponent))
	{
		const auto& projectileData = entityJson[SceneKeys::ProjectileComponent];
		auto& projectile = entityManager.AddComponent<ProjectileComponent>(entity);

		// A missing key keeps the default written in ProjectileComponent.h
		projectile.speed                   = projectileData.value(SceneKeys::ProjectileSpeed,         projectile.speed);
		projectile.damage                  = projectileData.value(SceneKeys::Damage,                  projectile.damage);
		projectile.poiseDamage             = projectileData.value(SceneKeys::PoiseDamage,             projectile.poiseDamage);
		projectile.radius                  = projectileData.value(SceneKeys::ProjectileRadius,        projectile.radius);
		projectile.homingDegreesPerSecond  = projectileData.value(SceneKeys::HomingDegreesPerSecond,  projectile.homingDegreesPerSecond);
		projectile.lifetimeSeconds         = projectileData.value(SceneKeys::LifetimeSeconds,         projectile.lifetimeSeconds);
		projectile.fuseSeconds             = projectileData.value(SceneKeys::FuseSeconds,             projectile.fuseSeconds);
		projectile.trailEffect             = projectileData.value(SceneKeys::TrailEffect,             projectile.trailEffect);
		projectile.trailParticlesPerSecond = projectileData.value(SceneKeys::TrailParticlesPerSecond, projectile.trailParticlesPerSecond);
		projectile.spinDegreesPerSecond    = projectileData.value(SceneKeys::SpinDegreesPerSecond,    projectile.spinDegreesPerSecond);
		projectile.launchDelaySeconds      = projectileData.value(SceneKeys::LaunchDelaySeconds,      projectile.launchDelaySeconds);
		projectile.bodyFacesFlight         = projectileData.value(SceneKeys::BodyFacesFlight,         projectile.bodyFacesFlight);
		projectile.spinsAroundFlight       = projectileData.value(SceneKeys::SpinsAroundFlight,       projectile.spinsAroundFlight);
		projectile.formSeconds             = projectileData.value(SceneKeys::FormSeconds,             projectile.formSeconds);
		projectile.aimsAfterForming        = projectileData.value(SceneKeys::AimsAfterForming,        projectile.aimsAfterForming);
		projectile.hitEffect               = projectileData.value(SceneKeys::HitEffect,               projectile.hitEffect);
		projectile.hitBurst                = projectileData.value(SceneKeys::HitBurst,                projectile.hitBurst);
		projectile.hitSound                = projectileData.value(SceneKeys::HitSound,                projectile.hitSound);
		projectile.hitStopSeconds          = projectileData.value(SceneKeys::HitStopSeconds,          projectile.hitStopSeconds);
		projectile.cameraShake             = projectileData.value(SceneKeys::CameraShake,             projectile.cameraShake);
	}

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
		json entityJson = SerializeEntity(entityManager, i);

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

bool SceneSerializer::CloneWithDescendants(EntityManager& entityManager, unsigned int entityID, unsigned int& outCopyID,
	std::vector<unsigned int>* outAllCopies)
{
	if (!entityManager.IsAlive(entityID))
		return false;

	SavedIndices savedIndices;
	json entitiesArray = json::array();
	for (unsigned int originalID : EntityHierarchy::GetWithDescendants(entityManager, entityID))
	{
		savedIndices[originalID] = entitiesArray.size();
		entitiesArray.push_back(SerializeEntity(entityManager, originalID));
	}
	// (only the parents inside the copied family: the copy of the first one has none)
	SerializeParents(entityManager, savedIndices, entitiesArray);

	std::vector<int> copies;
	for (const json& entityJson : entitiesArray)
	{
		const unsigned int copyID = entityManager.CreateEntity();
		copies.push_back(static_cast<int>(copyID));
		DeserializeEntity(entityManager, copyID, entityJson);
		if (outAllCopies)
			outAllCopies->push_back(copyID);
	}
	DeserializeParents(entityManager, entitiesArray, copies);

	outCopyID = static_cast<unsigned int>(copies.front());
	return true;
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

		DeserializeEntity(entityManager, entity, entityJson);
	}

	DeserializeParents(entityManager, entitiesJson, entityAtSavedIndex);
	
	SUNTA_ENGINE_LOG_INFO("SceneSerializer::Deserialize: Successfully loaded Scene '{0}' from '{1}'", scene.GetName(), filepath);
	return true;

}

}