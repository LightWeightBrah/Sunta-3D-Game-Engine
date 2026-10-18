#include "Core/SuntaPreCompiled.h"
#include "StartScene.h"

#include "Core/GameConfig.h"
#include "Core/Log.h"
#include "Core/VirtualFileSystem.h"
#include "ECS/Component.h"
#include "Scene/EntityFactory.h"
#include "Scene/Scene.h"
#include "Serialization/SceneSerializer.h"

namespace Sunta
{

namespace
{

namespace fs = std::filesystem;

constexpr const char* DefaultSceneName        = "Main";
constexpr const char* DefaultSceneVirtualPath = "@game/Scenes/Main.scene";

constexpr float FloorSize      = 20.0f;
constexpr float FloorThickness = 0.5f;
constexpr float CubeSize       = 1.0f;

const glm::vec3 SunPosition        = glm::vec3(0.0f, 5.0f, 0.0f);
const glm::vec3 SunRotationDegrees = glm::vec3(-50.0f, -30.0f, 0.0f);

// Behind the cube and a bit above it, tilted down so the cube is in the middle of the screen
const glm::vec3 CameraPosition        = glm::vec3(0.0f, 4.0f, 9.0f);
const glm::vec3 CameraRotationDegrees = glm::vec3(-20.0f, 0.0f, 0.0f);

bool FileExists(const std::string& path)
{
	std::error_code error;
	return fs::exists(path, error);
}

// The factory creates the entity at the end of the list, so the newest entity is the last one
unsigned int GetNewestEntity(Scene& scene)
{
	return scene.GetEntityManager().GetEntityCount() - 1;
}

void SetTransform(Scene& scene, unsigned int entity, const glm::vec3& rotationDegrees, const glm::vec3& scale)
{
	auto* transform = scene.GetEntityManager().GetComponent<TransformComponent>(entity);
	if (!transform)
		return;

	transform->rotation = rotationDegrees;
	transform->scale    = scale;
	transform->SyncQuaternionFromEuler();
	transform->isDirty  = true;
}

}

void StartScene::OpenInEditor(Scene& scene)
{
	GameConfig gameConfig;
	GameConfig::TryLoad(GameConfig::DefaultPath, gameConfig);

	const bool hasConfiguredScene = !gameConfig.startScene.empty();
	const std::string virtualPath = hasConfiguredScene ? gameConfig.startScene : DefaultSceneVirtualPath;
	const std::string scenePath   = VirtualFileSystem::Resolve(virtualPath);

	if (FileExists(scenePath))
	{
		LoadFromFile(scenePath, scene);
		return;
	}

	CreateDefaultScene(scene);
	SaveAsStartScene(virtualPath, scene, gameConfig);
}

void StartScene::LoadFromFile(const std::string& scenePath, Scene& scene)
{
	// A scene that fails to load is left untouched on disk, the editor just starts empty
	if (!SceneSerializer::Deserialize(scenePath, scene))
		SUNTA_ENGINE_LOG_ERROR("StartScene: Couldn't load '{0}', starting with an empty scene", scenePath);
}

void StartScene::CreateDefaultScene(Scene& scene)
{
	scene.Clear();
	scene.SetName(DefaultSceneName);

	EntityFactory::CreateDirectionalLight(scene, SunPosition, "Sun");
	SetTransform(scene, GetNewestEntity(scene), SunRotationDegrees, glm::vec3(1.0f));

	EntityFactory::CreateCube(scene, glm::vec3(0.0f, -FloorThickness * 0.5f, 0.0f), "Floor");
	SetTransform(scene, GetNewestEntity(scene), glm::vec3(0.0f), glm::vec3(FloorSize, FloorThickness, FloorSize));

	// Stands on the floor: its center is half of its height above the floor's top side
	EntityFactory::CreateCube(scene, glm::vec3(0.0f, CubeSize * 0.5f, 0.0f), "Cube");
	SetTransform(scene, GetNewestEntity(scene), glm::vec3(0.0f), glm::vec3(CubeSize));

	// Without a camera the game would fly around on the free camera of the editor
	EntityFactory::CreateCamera(scene, CameraPosition, "Camera");
	SetTransform(scene, GetNewestEntity(scene), CameraRotationDegrees, glm::vec3(1.0f));
}

void StartScene::SaveAsStartScene(const std::string& virtualPath, Scene& scene, GameConfig& gameConfig)
{
	const std::string scenePath = VirtualFileSystem::Resolve(virtualPath);

	std::error_code error;
	fs::create_directories(fs::path(scenePath).parent_path(), error);

	scene.SetFilePath(scenePath);
	if (!SceneSerializer::Serialize(scenePath, scene))
	{
		SUNTA_ENGINE_LOG_ERROR("StartScene: Couldn't save the default scene to '{0}'", scenePath);
		return;
	}

	gameConfig.startScene = virtualPath;
	GameConfig::TrySave(GameConfig::DefaultPath, gameConfig);

	SUNTA_ENGINE_LOG_INFO("StartScene: Created the default scene '{0}' and set it as the start scene", virtualPath);
}

}
