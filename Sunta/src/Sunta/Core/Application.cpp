#include "SuntaPreCompiled.h"

#include <assimp/version.h>

#include "Application.h"
#include "Scene/Scene.h"
#include "Renderer/Renderer.h"
#include "EngineTime.h"
#include "InputManager.h"

#include "Log.h"
#include "Events/EventBus.h"
#include "Events/EventTypes.h"
#include "Editor/EditorGUIContext.h"
#include "Editor/EditorGUI.h"
#include "ECS/ComponentLayout.h"
#include "ECS/Component.h"
#include "ResourceManager.h"
#include "Renderer/RendererAPI.h"

namespace Sunta
{
	Application::Application()
		: window(nullptr)
	{
		Init();
	}
	Application::~Application()
	{
	
	}
	
	void Application::Run()
	{
		while (isRunning)
		{
			EngineTime::Update();
	
			window->Update();

			ProcessInput();
	
			Update(EngineTime::deltaTime);
	
			Render();

			InputManager::Clear();
	
		}
	
		Shutdown();
	}
	
	void Application::Init()
	{
		Log::Init();
		InputManager::Init();

		RendererAPI::SetAPI(RendererAPI::API::OpenGL);

		window = Window::CreateWindow("Sunta Engine", WINDOW_WIDTH, WINDOW_HEIGHT);
		EditorGUIContext::Init(window.get());
		EditorGUI::SetDarkTheme();

		EventBus::Subscribe<WindowCloseEvent>([this](const auto& event) { isRunning = false; });

		Renderer::Init();
		ResourceManager::Init(Renderer::GetDevice());

		ResourceManager::LoadEditorIcon("defualt_folder",	"res/Sunta/Textures/Icons/defualt_folder_icon.png");
		ResourceManager::LoadEditorIcon("cpp_folder",		"res/Sunta/Textures/Icons/cpp_folder_icon.png");
		ResourceManager::LoadEditorIcon("3d_model_folder",	"res/Sunta/Textures/Icons/3d_model_folder_icon.png");
		ResourceManager::LoadEditorIcon("shader_folder",	"res/Sunta/Textures/Icons/shader_folder_icon.png");
		ResourceManager::LoadEditorIcon("image_folder",		"res/Sunta/Textures/Icons/image_folder_icon.png");
		ResourceManager::LoadEditorIcon("audio_folder",		"res/Sunta/Textures/Icons/audio_folder_icon.png");

		ResourceManager::LoadEditorIcon("defualt_file",		"res/Sunta/Textures/Icons/defualt_file_icon.png");
		ResourceManager::LoadEditorIcon("cpp_file",			"res/Sunta/Textures/Icons/cpp_file_icon.png");
		ResourceManager::LoadEditorIcon("3d_model_file",	"res/Sunta/Textures/Icons/3d_model_file_icon.png");
		ResourceManager::LoadEditorIcon("shader_file",		"res/Sunta/Textures/Icons/shader_file_icon.png");
		ResourceManager::LoadEditorIcon("image_file",		"res/Sunta/Textures/Icons/image_file_icon.png");
		ResourceManager::LoadEditorIcon("audio_file",		"res/Sunta/Textures/Icons/audio_file_icon.png");
		
		ResourceManager::LoadEditorIcon("editor_window_bg",	"res/Sunta/Textures/ui/editor_window_background.jpg");
		ResourceManager::LoadEditorIcon("file_browser_bg",	"res/Sunta/Textures/ui/file_browser_background.jpg");

		scene = std::make_unique<Scene>();
		scene->Init(Renderer::GetDevice(), window->GetWidth(), window->GetHeight());

		TagComponent::RegisterToInspector();
		TransformComponent::RegisterToInspector();
		DirectionalLightComponent::RegisterToInspector();
		PointLightComponent::RegisterToInspector();
		SpotlightComponent::RegisterToInspector();

		SUNTA_ENGINE_LOG_INFO("Sunta Engine is running!");
	}
	
	void Application::Update(float deltaTime)
	{
		scene->Update();
	}
	
	void Application::ProcessInput()
	{
		if (InputManager::IsKeyDown(GLFW_KEY_ESCAPE))
			isRunning = false;
	
		if (InputManager::IsKeyDown(GLFW_KEY_TAB))
		{
			currentEngineMode = (currentEngineMode == EngineMode::Game) ? EngineMode::Editor : EngineMode::Game;

			if (currentEngineMode == EngineMode::Game)
			{
				EditorGUI::ClearFocus();
				InputManager::SetFirstMouse(true);
			}

			EventBus::Publish<EngineModeChangedEvent>({ currentEngineMode });
		}
	
		scene->ProcessInput();
	}
	
	void Application::Render()
	{
		Renderer::Clear(0.02f, 0.01f, 0.01f, 1.0f);
	
		if (!scene)
		{
			SUNTA_ENGINE_LOG_ERROR("ERROR: Scene is NULL during Render");
			return;
		}
	
		scene->Render(renderer);

		EditorGUIContext::RenderUI(window.get(), scene->GetEntityManager());
		
		//EditorGUIContext::BeginDockingSpace(window.get());
		//
		//EditorGUI::Begin(EditorGUIContext::GetInspectorName());
		//
		//EditorGUI::DrawInspector(scene->GetEntityManager());
		//
		//EditorGUI::End();
		//EditorGUIContext::EndFrame(window.get());
	}
	
	void Application::Shutdown()
	{
		EditorGUIContext::Shutdown(window.get());
		scene->Clear();
	
		if (window)
			window.reset();
	
		glfwTerminate();
	}
	
}
