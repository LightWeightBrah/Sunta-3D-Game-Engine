#pragma once

namespace Sunta::Scripting
{
	namespace Fields
	{
		constexpr const char* EntityID = "entityID";
	}

	namespace Functions
	{
		constexpr const char* OnStart        = "OnStart";
		constexpr const char* OnUpdate       = "OnUpdate";

		constexpr const char* OnTriggerEnter = "OnTriggerEnter";
		constexpr const char* OnTriggerStay  = "OnTriggerStay";
		constexpr const char* OnTriggerExit  = "OnTriggerExit";

		constexpr const char* OnCollisionEnter = "OnCollisionEnter";
		constexpr const char* OnCollisionStay  = "OnCollisionStay";
		constexpr const char* OnCollisionExit  = "OnCollisionExit";
	}
}

namespace Sunta::EngineAssets
{
	namespace App
	{
		constexpr const char* EngineLogoPath = "@engine/App/engine_logo.png";
	}

	namespace Shaders
	{
		constexpr const char* Error = "Error";
		constexpr const char* Lit   = "Lit";
		constexpr const char* Unlit = "Unlit";
	}

	namespace Meshes
	{
		constexpr const char* Cube     = "cube";
		constexpr const char* Pyramid  = "pyramid";
		constexpr const char* Sphere   = "sphere";
		constexpr const char* Cone     = "cone";
		constexpr const char* Capsule  = "capsule";
	}

	namespace Models
	{
		constexpr const char* Solaire = "solaire";
		constexpr const char* Backpack = "backpack";
	}

	namespace Materials
	{
		constexpr const char* Error     = "error_material";
		constexpr const char* Default   = "default_material";
		constexpr const char* Unlit     = "unlit_material";
		constexpr const char* Textured  = "textured_material";
	}

	namespace Textures
	{
		constexpr const char* Container2Diffuse  = "container2Diffuse";
		constexpr const char* Container2Specular = "container2Specular";
		constexpr const char* WhiteTexture		 = "whiteTexture";
		constexpr const char* ErrorTexture       = "errorTexture";
		constexpr const char* CubeContainer      = "cube_container";
		constexpr const char* CubeChad           = "cube_chad";
	}

	namespace Icons
	{
		constexpr const char* DefaultFolder       = "default_folder";
		constexpr const char* CppFolder		      = "cpp_folder";
		constexpr const char* LuaFolder		      = "lua_folder";
		constexpr const char* ModelFolder         = "3d_model_folder";
		constexpr const char* ShaderFolder        = "shader_folder";
		constexpr const char* ImageFolder         = "image_folder";
		constexpr const char* AudioFolder         = "audio_folder";
		constexpr const char* FontsFolder         = "fonts_folder";
		constexpr const char* MaterialFolder	  = "material_folder";
		constexpr const char* SceneFolder		  = "scene_folder";
											      
		constexpr const char* DefaultFile         = "default_file";
		constexpr const char* CppFile             = "cpp_file";
		constexpr const char* LuaFile             = "lua_file";
		constexpr const char* ModelFile           = "3d_model_file";
		constexpr const char* ShaderFile          = "shader_file";
		constexpr const char* ImageFile           = "image_file";
		constexpr const char* AudioFile           = "audio_file";
		constexpr const char* FontFile            = "font_file";
		constexpr const char* MaterialFile		  = "material_file";
		constexpr const char* SceneFile			  = "scene_file";

		constexpr const char* Cube                = "cube";
		constexpr const char* Capsule             = "capsule";
		constexpr const char* Sphere              = "sphere";
		constexpr const char* Cone                = "cone";
		constexpr const char* Pyramid             = "pyramid";

		constexpr const char* BoxCollider         = "box_collider";
		constexpr const char* CapsuleCollider     = "capsule_collider";
		constexpr const char* SphereCollider      = "sphere_collider";

		constexpr const char* DirectionalLight    = "directional_light";
		constexpr const char* PointLight          = "point_light";
		constexpr const char* Spotlight           = "spotlight";

		constexpr const char* Locked              = "locked";
		constexpr const char* Unlocked            = "unlocked";
		constexpr const char* MoveTool            = "move_tool";
		constexpr const char* RotateTool          = "rotate_tool";
		constexpr const char* ScaleTool           = "scale_tool";
		constexpr const char* SnapOption          = "snap_option";
											      
		constexpr const char* PlayButton          = "play_button";
		constexpr const char* PauseButton         = "pause_button";
		constexpr const char* StopButton          = "stop_button";
		constexpr const char* RewindButton        = "rewind_button";
		constexpr const char* LoopButton          = "loop_button";

		constexpr const char* AnimationClip       = "animation_clip";
		constexpr const char* AnimationController = "animation_controller";
		constexpr const char* Pose                = "pose";
		constexpr const char* Skeleton            = "skeleton";

		constexpr const char* BlendTree			  = "blend_tree";
		constexpr const char* Particle			  = "particle";
		constexpr const char* Physics			  = "physics";
		constexpr const char* PostProcessing	  = "post_processing";
		constexpr const char* Skybox			  = "skybox";
		constexpr const char* Transform			  = "transform";
		constexpr const char* Console			  = "console";

		constexpr const char* Bonfire			  = "bonfire";
		constexpr const char* CharacterTarget     = "character_target";
		constexpr const char* Fog			      = "fog";
		constexpr const char* StateMachine        = "state_machine";
	}

	namespace UIAssets
	{
		constexpr const char* EditorWindowBG      = "editor_window_bg";
		constexpr const char* FileBrowserBG       = "file_browser_bg";
	}

}