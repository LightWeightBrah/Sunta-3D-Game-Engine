#include "SuntaPreCompiled.h"
#include "Application.h"

#include "Scene/Scene.h"
#include "Renderer/Renderer.h"
#include "Renderer/RendererAPI.h"
#include "Renderer/DebugRenderer.h"
#include "EngineTime.h"
#include "FatalError.h"
#include "InputManager.h"
#include "Log.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"
#include "ResourceManager.h"
#include "ResourceLoader.h"
#include "Platform.h"
#include "Scripting/ScriptingEngine.h"

namespace Sunta
{

namespace
{

// Frames drawn while the window is still hidden. ImGui needs a few frames to place its docked windows
// (and a Debug frame is slow), so a window shown at once would first look like the bare scene
constexpr int HiddenWarmUpFrames = 3;

// The SuntaRuntime program from the engine package is only the template of a game, it has no game files next to it,
// so the message also tells a person who started it what to do instead
std::string MakeMissingGameFilesMessage(const std::string& gameResourcesDirectory)
{
	return "The game files are missing: the folder '" + gameResourcesDirectory + "' doesn't exist.\n"
		   "Unpack the whole game folder (not only the .exe) before starting it.\n\n"
		   "If this is SuntaRuntime from the engine package: it is only a template and is not meant to be started. "
		   "Build your game in the editor (Project Settings > Save and Build Game), the game is made in the Build folder of the project.";
}

}

Application::Application(const ApplicationSpecification& specification)
	: specification(specification)
	, window(nullptr)
{
	Init();
}

Application::~Application() = default;

Scene& Application::GetScene()
{
	return *scene;
}

void Application::Run()
{
	SUNTA_ENGINE_LOG_INFO("Sunta Engine is running!");

	bool isWindowShown = false;
	int  framesDrawn   = 0;

	while (isRunning)
	{
		EngineTime::Update();

		// Shown here, not in Init(), and only after a few frames, so the window doesn't appear empty while layers are still loading
		if (!isWindowShown && framesDrawn >= HiddenWarmUpFrames)
		{
			window->Show();
			isWindowShown = true;
		}

		window->Update();

		if (!isRunning)
			break;

		if (isMinimized)
			continue;

		ProcessInput();
		Update(EngineTime::deltaTime);
		Render();

		InputManager::Clear();

		if (!isWindowShown)
			++framesDrawn;
	}

	// Shutdown takes a moment, the window shouldn't sit there frozen in the meantime
	window->Hide();

	Shutdown();
}

void Application::Init()
{
	Log::Init(specification.logFilePath);
	VirtualFileSystem::Init(specification.assetSource, specification.projectDirectory);

	// Early, so a fatal error can already show its message window
	Platform::Init();

	const std::string gameResourcesDirectory = VirtualFileSystem::Resolve("@game");
	if (!std::filesystem::exists(gameResourcesDirectory))
	{
		SUNTA_ENGINE_LOG_ERROR("Application: Game resources folder '{0}' doesn't exist", gameResourcesDirectory);

		if (specification.assetSource == AssetSource::PackagedNextToExecutable)
			FatalError::Report(MakeMissingGameFilesMessage(gameResourcesDirectory));
	}

	if (specification.loadGameConfig)
	{
		LoadGameConfig();
		ApplyDeveloperMode();
	}

	ScriptingEngine::Init();
	InputManager::Init();

	RendererAPI::SetAPI(RendererAPI::API::OpenGL);

	window = Window::CreateWindow(specification.windowTitle, specification.windowWidth, specification.windowHeight);
	SubscribeToWindowEvents();
	ApplyWindowSettings();

	if (specification.loadGameConfig)
		ApplyGameWindowIcon();

	Renderer::Init();
	DebugRenderer::Init();
	ResourceManager::Init(Renderer::GetDevice());
	if (specification.loadEngineResources)
		ResourceLoader::Init();

	scene = std::make_unique<Scene>();
	scene->Init(Renderer::GetDevice(), window->GetWidth(), window->GetHeight());
}

void Application::LoadGameConfig()
{
	if (!GameConfig::TryLoad(GameConfig::DefaultPath, gameConfig))
	{
		SUNTA_ENGINE_LOG_WARNING("Application: Using default game settings");
		return;
	}

	specification.windowTitle  = gameConfig.name;
	specification.windowWidth  = gameConfig.windowWidth;
	specification.windowHeight = gameConfig.windowHeight;
	specification.fullscreen   = gameConfig.fullscreen;
	specification.vsync        = gameConfig.vsync;

	CollisionSettings::SetActive(gameConfig.collisionSettings);
}

// A development build (see GameBuilder) shows its log in a console window, other builds stay clean
void Application::ApplyDeveloperMode()
{
	if (!gameConfig.developerMode)
		return;

	Terminal::OpenConsole();
	SUNTA_ENGINE_LOG_INFO("Application: Development build, the log is shown in the console too");
}

void Application::ApplyGameWindowIcon()
{
	const std::string iconPath = VirtualFileSystem::Resolve(gameConfig.windowIcon);

	// No icon in the project: the window keeps the default engine icon
	if (!std::filesystem::exists(iconPath))
		return;

	window->SetIcon(iconPath);
}

// Before the window is shown, so a fullscreen game doesn't flash as a small window first
void Application::ApplyWindowSettings()
{
	window->SetVSync(specification.vsync);
	window->SetFullscreen(specification.fullscreen);
}

// The user closed the window: closes at once, unless a layer wants to ask something first
void Application::RequestClose()
{
	for (auto& layer : layers)
	{
		if (!layer->OnCloseRequested())
			return;
	}

	Close();
}

void Application::SubscribeToWindowEvents()
{
	EventBus::Subscribe<WindowCloseEvent>([this](const auto& event) { RequestClose(); });

	EventBus::Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event)
		{
			isMinimized = (event.width == 0 || event.height == 0);
		}
	);
}

void Application::ProcessInput()
{
	for (auto& layer : layers)
		layer->OnProcessInput();

	scene->ProcessInput();
}

void Application::Update(float deltaTime)
{
	scene->Update();

	for (auto& layer : layers)
		layer->OnUpdate(deltaTime);
}

void Application::Render()
{
	Renderer::Clear(0.02f, 0.01f, 0.01f, 1.0f);

	scene->Render(renderer);

	for (auto& layer : layers)
		layer->OnRender();
}

void Application::Shutdown()
{
	// Last attached layer is detached first
	for (auto layerIterator = layers.rbegin(); layerIterator != layers.rend(); ++layerIterator)
		(*layerIterator)->OnDetach();

	layers.clear();

	scene->Clear();
	ScriptingEngine::Shutdown();

	if (window)
		window.reset();

	Platform::Shutdown();

	glfwTerminate();
}

}
