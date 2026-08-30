#include "SuntaPreCompiled.h"
#include "ResourceLoader.h"

#include "ResourceManager.h"
#include "Renderer/Primitives.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "EngineAssets.h"
#include "Renderer/Shader.h"
#include "VirtualFileSystem.h"
#include "Utilities/TextureNamingConventionsUtilities.h"
#include "Renderer/Renderer.h"

namespace Sunta
{

void ResourceLoader::Init()
{
	LoadShaders();
	LoadTextures();
	LoadMaterials();
	LoadMeshes();
	LoadTexturesNamingConvention();
	LoadModels();
	LoadIcons();
	LoadUIAssets();
}

void ResourceLoader::LoadShaders()
{
	using namespace Sunta::EngineAssets;
	ResourceManager::LoadShader(Shaders::Lit,		"@engine/Shaders/Lit.shader");
	ResourceManager::LoadShader(Shaders::Unlit,		"@engine/Shaders/Unlit.shader");
	ResourceManager::LoadShader(Shaders::Error,		"@engine/Shaders/Error.shader");
}

void ResourceLoader::LoadTextures()
{
	using namespace Sunta::EngineAssets;

	ResourceManager::LoadTexture(Textures::Container2Diffuse,	"@engine/Textures/container2.png");
	ResourceManager::LoadTexture(Textures::Container2Specular,	"@engine/Textures/container2_specular.png");
	ResourceManager::LoadTexture(Textures::WhiteTexture,		"@engine/Textures/whitePixel.png");
	ResourceManager::LoadTexture(Textures::ErrorTexture,		"@engine/Textures/errorTexture.png");

	ResourceManager::LoadTexture(Textures::CubeContainer,		"@engine/Textures/container.jpg");
	ResourceManager::LoadTexture(Textures::CubeChad,			"@engine/Textures/chad.png");
}

void ResourceLoader::LoadMaterials()
{
	using namespace Sunta::EngineAssets;

	// Error Material
	auto errorShader = ResourceManager::GetShaderData(Shaders::Error);
	auto errorMaterial = std::make_shared<Material>(errorShader);
	errorMaterial->SetUniform3f("errorColor", glm::vec3(1.0f, 0.0f, 1.0f));
	ResourceManager::LoadMaterial(Materials::Error, errorMaterial);

	// Default Material (Lit)
	auto litShader = ResourceManager::GetShaderData(Shaders::Lit);
	litShader->AddFeature(ShaderFeature::Lighting);
	litShader->AddFeature(ShaderFeature::Skinning);
	auto defaultMaterial = std::make_shared<Material>(litShader);
	defaultMaterial->SetAmbient(glm::vec3(0.25f, 0.2f, 0.05f))
		.SetDiffuse(glm::vec3(0.75f, 0.6f, 0.24f))
		.SetSpecular(glm::vec3(0.63f, 0.56f, 0.37f))
		.SetShininess(128.0f);
	ResourceManager::LoadMaterial(Materials::Default, defaultMaterial);

	// Unlit Material
	auto unlitShader = ResourceManager::GetShaderData(Shaders::Unlit);
	auto unlitMaterial = std::make_shared<Material>(unlitShader);
	ResourceManager::LoadMaterial(Materials::Unlit, unlitMaterial);

	// Textures Material
	auto texturedMaterial = std::make_shared<Material>(
		litShader,
		ResourceManager::GetTextureData(Textures::Container2Diffuse),
		ResourceManager::GetTextureData(Textures::Container2Specular)
	);
	texturedMaterial->SetAmbient(glm::vec3(1.0f))
		.SetDiffuse(glm::vec3(1.0f))
		.SetSpecular(glm::vec3(1.0f))
		.SetShininess(32.0f);
	ResourceManager::LoadMaterial(Materials::Textured, texturedMaterial);

	LoadMaterialsFromDirectory(VirtualFileSystem::Resolve("@engine"));
	LoadMaterialsFromDirectory(VirtualFileSystem::Resolve("@game"));
}

void ResourceLoader::LoadMeshes()
{
	using namespace Sunta::EngineAssets;

	auto& rendererDevice = Renderer::GetDevice();

	ResourceManager::LoadMesh(Meshes::Cube,     [&]() { return Primitives::CreateCube(rendererDevice); });
	ResourceManager::LoadMesh(Meshes::Pyramid,  [&]() { return Primitives::CreatePyramide(rendererDevice); });
	ResourceManager::LoadMesh(Meshes::Sphere,   [&]() { return Primitives::CreateSphere(rendererDevice); });
	ResourceManager::LoadMesh(Meshes::Capsule,  [&]() { return Primitives::CreateCapsule(rendererDevice); });
	ResourceManager::LoadMesh(Meshes::Cone,     [&]() { return Primitives::CreateCone(rendererDevice); });
}

void ResourceLoader::LoadTexturesNamingConvention()
{
	TextureNamingConventionsUtilities::LoadFromFile(VirtualFileSystem::Resolve("@engine/Textures/texture_naming.textureconfig"));
}

void ResourceLoader::LoadModels()
{
	using namespace Sunta::EngineAssets;


	ResourceManager::LoadModel(Models::Solaire, "@engine/Models/Solaire/Solaire All Animations.fbx", 0.05f);
	ResourceManager::LoadModel(Models::Backpack, "@engine/Models/backpack/backpack.obj", 0.5f);
}

void ResourceLoader::LoadIcons()
{
	using namespace Sunta::EngineAssets;

	ResourceManager::LoadEditorIcon(Icons::DefaultFolder,       "@engine/Textures/Icons/default_folder_icon.png");
	ResourceManager::LoadEditorIcon(Icons::CppFolder,           "@engine/Textures/Icons/cpp_folder_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ModelFolder,         "@engine/Textures/Icons/3d_model_folder_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ShaderFolder,        "@engine/Textures/Icons/shader_folder_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ImageFolder,         "@engine/Textures/Icons/image_folder_icon.png");
	ResourceManager::LoadEditorIcon(Icons::AudioFolder,         "@engine/Textures/Icons/audio_folder_icon.png");
	ResourceManager::LoadEditorIcon(Icons::FontsFolder,         "@engine/Textures/Icons/fonts_folder_icon.png");

	ResourceManager::LoadEditorIcon(Icons::DefaultFile,         "@engine/Textures/Icons/default_file_icon.png");
	ResourceManager::LoadEditorIcon(Icons::CppFile,             "@engine/Textures/Icons/cpp_file_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ModelFile,           "@engine/Textures/Icons/3d_model_file_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ShaderFile,          "@engine/Textures/Icons/shader_file_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ImageFile,           "@engine/Textures/Icons/image_file_icon.png");
	ResourceManager::LoadEditorIcon(Icons::AudioFile,           "@engine/Textures/Icons/audio_file_icon.png");
	ResourceManager::LoadEditorIcon(Icons::FontFile,            "@engine/Textures/Icons/font_file_icon.png");

	ResourceManager::LoadEditorIcon(Icons::Cube,                "@engine/Textures/Icons/cube_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Capsule,             "@engine/Textures/Icons/capsule_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Sphere,              "@engine/Textures/Icons/sphere_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Cone,                "@engine/Textures/Icons/cone_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Pyramid,             "@engine/Textures/Icons/pyramid_icon.png");

	ResourceManager::LoadEditorIcon(Icons::BoxCollider,         "@engine/Textures/Icons/box_collider_icon.png");
	ResourceManager::LoadEditorIcon(Icons::CapsuleCollider,     "@engine/Textures/Icons/capsule_collider_icon.png");
	ResourceManager::LoadEditorIcon(Icons::SphereCollider,      "@engine/Textures/Icons/sphere_collider_icon.png");
															    
	ResourceManager::LoadEditorIcon(Icons::DirectionalLight,    "@engine/Textures/Icons/directional_light_icon.png");
	ResourceManager::LoadEditorIcon(Icons::PointLight,          "@engine/Textures/Icons/point_light_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Spotlight,           "@engine/Textures/Icons/spotlight_icon.png");

	ResourceManager::LoadEditorIcon(Icons::MoveTool,            "@engine/Textures/Icons/move_tool_icon.png");
	ResourceManager::LoadEditorIcon(Icons::RotateTool,          "@engine/Textures/Icons/rotate_tool_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ScaleTool,           "@engine/Textures/Icons/scale_tool_icon.png");
	ResourceManager::LoadEditorIcon(Icons::SnapOption,          "@engine/Textures/Icons/snap_option_icon.png");
														        
	ResourceManager::LoadEditorIcon(Icons::PlayButton,          "@engine/Textures/Icons/play_button_icon.png");
	ResourceManager::LoadEditorIcon(Icons::PauseButton,         "@engine/Textures/Icons/pause_button_icon.png");
	ResourceManager::LoadEditorIcon(Icons::StopButton,          "@engine/Textures/Icons/stop_button_icon.png");
	ResourceManager::LoadEditorIcon(Icons::RewindButton,        "@engine/Textures/Icons/rewind_button_icon.png");
	ResourceManager::LoadEditorIcon(Icons::LoopButton,          "@engine/Textures/Icons/loop_button_icon.png");

	ResourceManager::LoadEditorIcon(Icons::AnimationClip,       "@engine/Textures/Icons/animation_clip_icon.png");
	ResourceManager::LoadEditorIcon(Icons::AnimationController, "@engine/Textures/Icons/animation_controller_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Pose,                "@engine/Textures/Icons/pose_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Skeleton,            "@engine/Textures/Icons/skeleton_icon.png");

	ResourceManager::LoadEditorIcon(Icons::BlendTree,           "@engine/Textures/Icons/blend_tree_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Particle,            "@engine/Textures/Icons/particle_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Physics,             "@engine/Textures/Icons/physics_icon.png");
	ResourceManager::LoadEditorIcon(Icons::PostProcessing,      "@engine/Textures/Icons/post_processing_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Skybox,              "@engine/Textures/Icons/skybox_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Transform,           "@engine/Textures/Icons/transform_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Console,             "@engine/Textures/Icons/console_icon.png");

	ResourceManager::LoadEditorIcon(Icons::Bonfire,             "@engine/Textures/Icons/bonfire_icon.png");
	ResourceManager::LoadEditorIcon(Icons::CharacterTarget,     "@engine/Textures/Icons/character_target_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Fog,                 "@engine/Textures/Icons/fog_icon.png");
	ResourceManager::LoadEditorIcon(Icons::StateMachine,        "@engine/Textures/Icons/state_machine_icon.png");
}

void ResourceLoader::LoadUIAssets()
{
	using namespace Sunta::EngineAssets;

	ResourceManager::LoadEditorIcon(UIAssets::EditorWindowBG,   "@engine/Textures/ui/editor_window_background.jpg");
	ResourceManager::LoadEditorIcon(UIAssets::FileBrowserBG ,   "@engine/Textures/ui/file_browser_background.jpg");
}

void ResourceLoader::LoadMaterialsFromDirectory(const std::filesystem::path& directoryPath)
{
	if (!std::filesystem::exists(directoryPath))
		return;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(directoryPath))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".material")
		{
			ResourceManager::LoadMaterialFromFile(entry.path().string());
		}
	}

}

}