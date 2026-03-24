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

		window = Window::CreateWindow("Sunta Engine", WINDOW_WIDTH, WINDOW_HEIGHT);
		EditorGUIContext::Init(window.get());

		EventBus::Subscribe<WindowCloseEvent>([this](const auto& event) { isRunning = false; });

		Renderer::Init();
		ResourceManager::Init(Renderer::GetDevice());

		scene = std::make_unique<Scene>();
		scene->Init(Renderer::GetDevice(), window->GetWidth(), window->GetHeight());

		TransformComponent::RegisterToInspector();
		LightComponent::RegisterToInspector();

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
		Renderer::Clear(0.05f, 0.05f, 0.05f, 1.0f);
	
		if (!scene)
		{
			SUNTA_ENGINE_LOG_ERROR("ERROR: Scene is NULL during Render");
			return;
		}
	
		scene->Render(renderer);

		EditorGUIContext::NewFrame(window.get());
		
		EditorGUIContext::BeginDockingSpace(window.get());

		EditorGUI::Begin(EditorGUIContext::GetInspectorName());

		EditorGUI::DrawInspector(scene->GetEntityManager());

		EditorGUI::End();
		EditorGUIContext::EndFrame(window.get());
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
