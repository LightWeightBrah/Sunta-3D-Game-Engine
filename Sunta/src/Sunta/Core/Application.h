#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Sunta/Renderer/Renderer.h"
#include "Core.h"
#include "Layer.h"
#include "GameConfig.h"
#include "VirtualFileSystem.h"
#include "Window.h"

namespace Sunta
{

class Scene;

struct ApplicationSpecification
{
	std::string  windowTitle  = "Sunta Engine";
	unsigned int windowWidth  = 1600;
	unsigned int windowHeight = 800;
	bool         fullscreen   = false;
	bool         vsync        = true;

	AssetSource assetSource = AssetSource::SourceTree;

	// Folder of the game project (contains "res" and "res/game.config")
	// Used when assets are read from the source tree, a packaged game has its own copy
	std::filesystem::path projectDirectory;

	// When true, window title, size, fullscreen and vsync are taken from "@game/game.config"
	bool loadGameConfig = false;

	// Shaders, textures, materials, meshes and models are loaded once, by the program that draws them
	// A program with nothing to draw (the project launcher) turns this off and starts much faster
	bool loadEngineResources = true;

	// Every log line is also written to this file (empty = only the console)
	// A shipped game sets it, because on Windows it has no console window
	std::filesystem::path logFilePath;
};

// The engine core: window, renderer, scene and the main loop
// What the application DOES (editor, running a game) is decided by the layers pushed to it
class Application
{
public:
	explicit Application(const ApplicationSpecification& specification);
	~Application();

	Application(const Application&)            = delete;
	Application& operator=(const Application&) = delete;

	void Run();
	void Close() { isRunning = false; }

	template<typename LayerType>
	LayerType& PushLayer()
	{
		auto layer = std::make_unique<LayerType>(*this);
		LayerType& layerReference = *layer;

		layers.push_back(std::move(layer));
		layerReference.OnAttach();

		return layerReference;
	}

	Window&           GetWindow()     { return *window; }
	Scene&            GetScene();
	const GameConfig& GetGameConfig() const { return gameConfig; }

private:
	ApplicationSpecification specification;
	GameConfig               gameConfig;

	Renderer                 renderer;
	std::unique_ptr<Window>  window;
	std::unique_ptr<Scene>   scene;
	std::vector<std::unique_ptr<Layer>> layers;

	bool isRunning   = true;
	bool isMinimized = false;

	void Init();
	void LoadGameConfig();
	void ApplyDeveloperMode();
	void ApplyGameWindowIcon();
	void ApplyWindowSettings();
	void SubscribeToWindowEvents();
	void RequestClose();

	void ProcessInput();
	void Update(float deltaTime);
	void Render();
	void Shutdown();
};

}
