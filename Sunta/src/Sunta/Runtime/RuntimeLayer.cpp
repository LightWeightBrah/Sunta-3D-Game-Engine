#include "Core/SuntaPreCompiled.h"
#include "RuntimeLayer.h"

#include "Core/Application.h"
#include "Core/FatalError.h"
#include "Core/InputManager.h"
#include "Core/Log.h"
#include "Core/VirtualFileSystem.h"
#include "Gameplay/PauseMenu.h"
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
	// A development build reloads a script when its file changes, so values can be tried out while the game runs
	application.GetScene().SetScriptHotReloadEnabled(application.GetGameConfig().developerMode);

	LoadStartScene();

	// The game has no editor mode: hides the cursor and unlocks the camera
	EventBus::Publish<EngineModeChangedEvent>({ EngineMode::Game });

	application.GetScene().BeginPlay();
}

// Escape opens the pause menu (see PauseMenu, run by the scene): its "Quit" closes the game
void RuntimeLayer::OnProcessInput()
{
	if (PauseMenu::TakeQuitRequest())
		application.Close();

	// The cursor shows over the pause menu, the game has it hidden
	if (PauseMenu::IsPaused() != isCursorShown)
	{
		isCursorShown = PauseMenu::IsPaused();
		application.GetWindow().EnableMouseCursor(isCursorShown);
	}
}

void RuntimeLayer::LoadStartScene()
{
	const std::string& startScene = application.GetGameConfig().startScene;

	if (startScene.empty())
		FatalError::Report("The game has no start scene: 'start_scene' is not set in '" + std::string(GameConfig::DefaultPath) + "'.");

	const std::string startScenePath = VirtualFileSystem::Resolve(startScene);

	if (!SceneSerializer::Deserialize(startScenePath, application.GetScene()))
		FatalError::Report("Couldn't load the start scene '" + startScenePath + "'.");
}

}
