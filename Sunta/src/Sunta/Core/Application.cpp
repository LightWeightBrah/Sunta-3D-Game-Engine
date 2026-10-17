#include "SuntaPreCompiled.h"
#include "Application.h"

#include "Scene/Scene.h"
#include "Renderer/Renderer.h"
#include "Renderer/RendererAPI.h"
#include "Renderer/DebugRenderer.h"
#include "EngineTime.h"
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
	// Shown here, not in Init(), so the window doesn't appear empty while layers are still loading
	window->Show();
	SUNTA_ENGINE_LOG_INFO("Sunta Engine is running!");

	while (isRunning)
	{
		EngineTime::Update();

		window->Update();

		if (!isRunning)
			break;

		if (isMinimized)
			continue;

		ProcessInput();
		Update(EngineTime::deltaTime);
		Render();

		InputManager::Clear();
	}

	// Shutdown takes a moment, the window shouldn't sit there frozen in the meantime
	window->Hide();

	Shutdown();
}

void Application::Init()
{
	Log::Init();
	VirtualFileSystem::Init(specification.assetSource, specification.projectDirectory);

	const std::string gameResourcesDirectory = VirtualFileSystem::Resolve("@game");
	if (!std::filesystem::exists(gameResourcesDirectory))
		SUNTA_ENGINE_LOG_ERROR("Application: Game resources folder '{0}' doesn't exist", gameResourcesDirectory);

	if (specification.loadGameConfig)
		LoadGameConfig();

	ScriptingEngine::Init();
	Platform::Init();
	InputManager::Init();

	RendererAPI::SetAPI(RendererAPI::API::OpenGL);

	window = Window::CreateWindow(specification.windowTitle, specification.windowWidth, specification.windowHeight);
	SubscribeToWindowEvents();

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
}

void Application::ApplyGameWindowIcon()
{
	const std::string iconPath = VirtualFileSystem::Resolve(gameConfig.windowIcon);

	// No icon in the project: the window keeps the default engine icon
	if (!std::filesystem::exists(iconPath))
		return;

	window->SetIcon(iconPath);
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
