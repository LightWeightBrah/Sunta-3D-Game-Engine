#include "Core/SuntaPreCompiled.h"
#include "RuntimeLayer.h"

#include "Core/Application.h"
#include "Core/InputManager.h"
#include "Core/Log.h"
#include "Core/VirtualFileSystem.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"
#include "Scene/Scene.h"
#include "Serialization/SceneSerializer.h"

namespace Sunta
{

RuntimeLayer::RuntimeLayer(Application& application)
	: application(application)
{

}

void RuntimeLayer::OnAttach()
{
	LoadStartScene();

	// The game has no editor mode: hides the cursor and unlocks the camera
	EventBus::Publish<EngineModeChangedEvent>({ EngineMode::Game });

	application.GetScene().BeginPlay();
}

void RuntimeLayer::OnProcessInput()
{
	if (InputManager::IsKeyDown(GLFW_KEY_ESCAPE))
		application.Close();
}

void RuntimeLayer::LoadStartScene()
{
	const std::string& startScene = application.GetGameConfig().startScene;

	if (startScene.empty())
	{
		SUNTA_ENGINE_LOG_ERROR("RuntimeLayer: 'start_scene' is not set in '{0}'", GameConfig::DefaultPath);
		return;
	}

	const std::string startScenePath = VirtualFileSystem::Resolve(startScene);

	if (!SceneSerializer::Deserialize(startScenePath, application.GetScene()))
		SUNTA_ENGINE_LOG_ERROR("RuntimeLayer: Failed to load the start scene '{0}'", startScenePath);
}

}
