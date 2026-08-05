#include "SuntaPreCompiled.h"
#include "ResourceLoader.h"

#include "ResourceManager.h"
#include "Renderer/Primitives.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "EngineAssets.h"
#include "Renderer/Shader.h"

namespace Sunta
{

void ResourceLoader::Init(RendererDevice& rendererDevice)
{
	LoadShaders(rendererDevice);
	LoadTextures(rendererDevice);
	LoadMaterials(rendererDevice);
	LoadMeshes(rendererDevice);
	LoadIcons(rendererDevice);
	LoadUIAssets(rendererDevice);
}

void ResourceLoader::LoadShaders(RendererDevice& rendererDevice)
{
	using namespace Sunta::EngineAssets;
	ResourceManager::LoadShader(Shaders::Lit,		"res/Sunta/Shaders/Lit.shader");
	ResourceManager::LoadShader(Shaders::Unlit,		"res/Sunta/Shaders/Unlit.shader");
	ResourceManager::LoadShader(Shaders::Error,		"res/Sunta/Shaders/Error.shader");
}

void ResourceLoader::LoadTextures(RendererDevice& rendererDevice)
{
	using namespace Sunta::EngineAssets;

	ResourceManager::LoadTexture(Textures::Container2Diffuse,	"res/Sunta/Textures/container2.png");
	ResourceManager::LoadTexture(Textures::Container2Specular,	"res/Sunta/Textures/container2_specular.png");
	ResourceManager::LoadTexture(Textures::WhiteTexture,		"res/Sunta/Textures/whitePixel.png");
	ResourceManager::LoadTexture(Textures::ErrorTexture,		"res/Sunta/Textures/errorTexture.png");

	ResourceManager::LoadTexture(Textures::CubeContainer,		"res/Sunta/Textures/container.jpg");
	ResourceManager::LoadTexture(Textures::CubeChad,			"res/Sunta/Textures/chad.png");
}

void ResourceLoader::LoadMaterials(RendererDevice& rendererDevice)
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
	auto defaultMaterial = std::make_shared<Material>(litShader);
	defaultMaterial->SetAmbient(glm::vec3(0.25f, 0.2f, 0.05f))
		.SetDiffuse(glm::vec3(0.75f, 0.6f, 0.24f))
		.SetSpecular(glm::vec3(0.63, 0.56f, 0.37f))
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
}

void ResourceLoader::LoadMeshes(RendererDevice& rendererDevice)
{
	using namespace Sunta::EngineAssets;

	ResourceManager::LoadMesh(Meshes::Cube,     [&]() { return Primitives::CreateCube(rendererDevice); });
	ResourceManager::LoadMesh(Meshes::Pyramid,  [&]() { return Primitives::CreatePyramide(rendererDevice); });
	ResourceManager::LoadMesh(Meshes::Sphere,   [&]() { return Primitives::CreateSphere(rendererDevice); });
	ResourceManager::LoadMesh(Meshes::Capsule,  [&]() { return Primitives::CreateCapsule(rendererDevice); });
	ResourceManager::LoadMesh(Meshes::Cone,     [&]() { return Primitives::CreateCone(rendererDevice); });
}

void ResourceLoader::LoadIcons(RendererDevice& rendererDevice)
{
	using namespace Sunta::EngineAssets;

	ResourceManager::LoadEditorIcon(Icons::DefaultFolder,       "res/Sunta/Textures/Icons/default_folder_icon.png");
	ResourceManager::LoadEditorIcon(Icons::CppFolder,           "res/Sunta/Textures/Icons/cpp_folder_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ModelFolder,         "res/Sunta/Textures/Icons/3d_model_folder_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ShaderFolder,        "res/Sunta/Textures/Icons/shader_folder_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ImageFolder,         "res/Sunta/Textures/Icons/image_folder_icon.png");
	ResourceManager::LoadEditorIcon(Icons::AudioFolder,         "res/Sunta/Textures/Icons/audio_folder_icon.png");
	ResourceManager::LoadEditorIcon(Icons::FontsFolder,         "res/Sunta/Textures/Icons/fonts_folder_icon.png");

	ResourceManager::LoadEditorIcon(Icons::DefaultFile,         "res/Sunta/Textures/Icons/default_file_icon.png");
	ResourceManager::LoadEditorIcon(Icons::CppFile,             "res/Sunta/Textures/Icons/cpp_file_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ModelFile,           "res/Sunta/Textures/Icons/3d_model_file_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ShaderFile,          "res/Sunta/Textures/Icons/shader_file_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ImageFile,           "res/Sunta/Textures/Icons/image_file_icon.png");
	ResourceManager::LoadEditorIcon(Icons::AudioFile,           "res/Sunta/Textures/Icons/audio_file_icon.png");
	ResourceManager::LoadEditorIcon(Icons::FontFile,            "res/Sunta/Textures/Icons/font_file_icon.png");

	ResourceManager::LoadEditorIcon(Icons::Cube,                "res/Sunta/Textures/Icons/cube_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Capsule,             "res/Sunta/Textures/Icons/capsule_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Sphere,              "res/Sunta/Textures/Icons/sphere_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Cone,                "res/Sunta/Textures/Icons/cone_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Pyramid,             "res/Sunta/Textures/Icons/pyramid_icon.png");

	ResourceManager::LoadEditorIcon(Icons::BoxCollider,         "res/Sunta/Textures/Icons/box_collider_icon.png");
	ResourceManager::LoadEditorIcon(Icons::CapsuleCollider,     "res/Sunta/Textures/Icons/capsule_collider_icon.png");
	ResourceManager::LoadEditorIcon(Icons::SphereCollider,      "res/Sunta/Textures/Icons/sphere_collider_icon.png");
															    
	ResourceManager::LoadEditorIcon(Icons::DirectionalLight,    "res/Sunta/Textures/Icons/directional_light_icon.png");
	ResourceManager::LoadEditorIcon(Icons::PointLight,          "res/Sunta/Textures/Icons/point_light_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Spotlight,           "res/Sunta/Textures/Icons/spotlight_icon.png");

	ResourceManager::LoadEditorIcon(Icons::MoveTool,            "res/Sunta/Textures/Icons/move_tool_icon.png");
	ResourceManager::LoadEditorIcon(Icons::RotateTool,          "res/Sunta/Textures/Icons/rotate_tool_icon.png");
	ResourceManager::LoadEditorIcon(Icons::ScaleTool,           "res/Sunta/Textures/Icons/scale_tool_icon.png");
	ResourceManager::LoadEditorIcon(Icons::SnapOption,          "res/Sunta/Textures/Icons/snap_option_icon.png");
														        
	ResourceManager::LoadEditorIcon(Icons::PlayButton,          "res/Sunta/Textures/Icons/play_button_icon.png");
	ResourceManager::LoadEditorIcon(Icons::PauseButton,         "res/Sunta/Textures/Icons/pause_button_icon.png");
	ResourceManager::LoadEditorIcon(Icons::StopButton,          "res/Sunta/Textures/Icons/stop_button_icon.png");
	ResourceManager::LoadEditorIcon(Icons::RewindButton,        "res/Sunta/Textures/Icons/rewind_button_icon.png");
	ResourceManager::LoadEditorIcon(Icons::LoopButton,          "res/Sunta/Textures/Icons/loop_button_icon.png");

	ResourceManager::LoadEditorIcon(Icons::AnimationClip,       "res/Sunta/Textures/Icons/animation_clip_icon.png");
	ResourceManager::LoadEditorIcon(Icons::AnimationController, "res/Sunta/Textures/Icons/animation_controller_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Pose,                "res/Sunta/Textures/Icons/pose_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Skeleton,            "res/Sunta/Textures/Icons/skeleton_icon.png");

	ResourceManager::LoadEditorIcon(Icons::BlendTree,           "res/Sunta/Textures/Icons/blend_tree_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Particle,            "res/Sunta/Textures/Icons/particle_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Physics,             "res/Sunta/Textures/Icons/physics_icon.png");
	ResourceManager::LoadEditorIcon(Icons::PostProcessing,      "res/Sunta/Textures/Icons/post_processing_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Skybox,              "res/Sunta/Textures/Icons/skybox_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Transform,           "res/Sunta/Textures/Icons/transform_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Console,             "res/Sunta/Textures/Icons/console_icon.png");

	ResourceManager::LoadEditorIcon(Icons::Bonfire,             "res/Sunta/Textures/Icons/bonfire_icon.png");
	ResourceManager::LoadEditorIcon(Icons::CharacterTarget,     "res/Sunta/Textures/Icons/character_target_icon.png");
	ResourceManager::LoadEditorIcon(Icons::Fog,                 "res/Sunta/Textures/Icons/fog_icon.png");
	ResourceManager::LoadEditorIcon(Icons::StateMachine,        "res/Sunta/Textures/Icons/state_machine_icon.png");
}

void ResourceLoader::LoadUIAssets(RendererDevice& rendererDevice)
{
	using namespace Sunta::EngineAssets;

	ResourceManager::LoadEditorIcon(UIAssets::EditorWindowBG,   "res/Sunta/Textures/ui/editor_window_background.jpg");
	ResourceManager::LoadEditorIcon(UIAssets::FileBrowserBG ,   "res/Sunta/Textures/ui/file_browser_background.jpg");
}

}