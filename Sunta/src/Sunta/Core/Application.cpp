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
#include "Assert.h"

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

			if (!isRunning)
				break;

			if (isMinimized)
				continue;

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
		EventBus::Subscribe<WindowResizeEvent>([this](const WindowResizeEvent& event) 
			{ 
				if (event.width == 0 || event.height == 0)
				{
					isMinimized = true;
					return;
				}

				isMinimized = false;
			}
		);

		Renderer::Init();
		ResourceManager::Init(Renderer::GetDevice());

		ResourceManager::LoadEditorIcon("defualt_folder",		"res/Sunta/Textures/Icons/defualt_folder_icon.png");
		ResourceManager::LoadEditorIcon("cpp_folder",			"res/Sunta/Textures/Icons/cpp_folder_icon.png");
		ResourceManager::LoadEditorIcon("3d_model_folder",		"res/Sunta/Textures/Icons/3d_model_folder_icon.png");
		ResourceManager::LoadEditorIcon("shader_folder",		"res/Sunta/Textures/Icons/shader_folder_icon.png");
		ResourceManager::LoadEditorIcon("image_folder",			"res/Sunta/Textures/Icons/image_folder_icon.png");
		ResourceManager::LoadEditorIcon("audio_folder",			"res/Sunta/Textures/Icons/audio_folder_icon.png");
		ResourceManager::LoadEditorIcon("fonts_folder",			"res/Sunta/Textures/Icons/fonts_folder_icon.png");

		ResourceManager::LoadEditorIcon("defualt_file",			"res/Sunta/Textures/Icons/defualt_file_icon.png");
		ResourceManager::LoadEditorIcon("cpp_file",				"res/Sunta/Textures/Icons/cpp_file_icon.png");
		ResourceManager::LoadEditorIcon("3d_model_file",		"res/Sunta/Textures/Icons/3d_model_file_icon.png");
		ResourceManager::LoadEditorIcon("shader_file",			"res/Sunta/Textures/Icons/shader_file_icon.png");
		ResourceManager::LoadEditorIcon("image_file",			"res/Sunta/Textures/Icons/image_file_icon.png");
		ResourceManager::LoadEditorIcon("audio_file",			"res/Sunta/Textures/Icons/audio_file_icon.png");
		ResourceManager::LoadEditorIcon("font_file",			"res/Sunta/Textures/Icons/font_file_icon.png");
		
		ResourceManager::LoadEditorIcon("cube",					"res/Sunta/Textures/Icons/cube_icon.png");
		ResourceManager::LoadEditorIcon("capsule",				"res/Sunta/Textures/Icons/capsule_icon.png");
		ResourceManager::LoadEditorIcon("sphere",				"res/Sunta/Textures/Icons/sphere_icon.png");
		ResourceManager::LoadEditorIcon("cone",					"res/Sunta/Textures/Icons/cone_icon.png");
		ResourceManager::LoadEditorIcon("pyramid",				"res/Sunta/Textures/Icons/pyramid_icon.png");

		ResourceManager::LoadEditorIcon("box_collider",			"res/Sunta/Textures/Icons/box_collider_icon.png");
		ResourceManager::LoadEditorIcon("capsule_collider",		"res/Sunta/Textures/Icons/capsule_collider_icon.png");
		ResourceManager::LoadEditorIcon("sphere_collider",		"res/Sunta/Textures/Icons/sphere_collider_icon.png");

		ResourceManager::LoadEditorIcon("diectional_light",		"res/Sunta/Textures/Icons/diectional_light_icon.png");
		ResourceManager::LoadEditorIcon("point_light",			"res/Sunta/Textures/Icons/point_light_icon.png");
		ResourceManager::LoadEditorIcon("spotlight",			"res/Sunta/Textures/Icons/spotlight_icon.png");

		ResourceManager::LoadEditorIcon("move_tool",			"res/Sunta/Textures/Icons/move_tool_icon.png");
		ResourceManager::LoadEditorIcon("rotate_tool",			"res/Sunta/Textures/Icons/rotate_tool_icon.png");
		ResourceManager::LoadEditorIcon("scale_tool",			"res/Sunta/Textures/Icons/scale_tool_icon.png");
		ResourceManager::LoadEditorIcon("snap_option",			"res/Sunta/Textures/Icons/snap_option_icon.png");

		ResourceManager::LoadEditorIcon("play_button",			"res/Sunta/Textures/Icons/play_button_icon.png");
		ResourceManager::LoadEditorIcon("pause_button",			"res/Sunta/Textures/Icons/pause_button_icon.png");
		ResourceManager::LoadEditorIcon("stop_button",			"res/Sunta/Textures/Icons/stop_button_icon.png");
		ResourceManager::LoadEditorIcon("rewind_button",		"res/Sunta/Textures/Icons/rewind_button_icon.png");
		ResourceManager::LoadEditorIcon("loop_button",			"res/Sunta/Textures/Icons/loop_button_icon.png");

		ResourceManager::LoadEditorIcon("animation_clip",		"res/Sunta/Textures/Icons/animation_clip_icon.png");
		ResourceManager::LoadEditorIcon("animation_controller",	"res/Sunta/Textures/Icons/animation_controller_icon.png");
		ResourceManager::LoadEditorIcon("pose",					"res/Sunta/Textures/Icons/pose_icon.png");
		ResourceManager::LoadEditorIcon("skeleton",				"res/Sunta/Textures/Icons/skeleton_icon.png");

		ResourceManager::LoadEditorIcon("blend_tree",			"res/Sunta/Textures/Icons/blend_tree_icon.png");
		ResourceManager::LoadEditorIcon("particle",				"res/Sunta/Textures/Icons/particle_icon.png");
		ResourceManager::LoadEditorIcon("physics",				"res/Sunta/Textures/Icons/physics_icon.png");
		ResourceManager::LoadEditorIcon("post_processing",		"res/Sunta/Textures/Icons/post_processing_icon.png");
		ResourceManager::LoadEditorIcon("skybox",				"res/Sunta/Textures/Icons/skybox_icon.png");
		ResourceManager::LoadEditorIcon("transform",			"res/Sunta/Textures/Icons/transform_icon.png");
		ResourceManager::LoadEditorIcon("console",				"res/Sunta/Textures/Icons/console_icon.png");

		ResourceManager::LoadEditorIcon("bonfire",				"res/Sunta/Textures/Icons/bonfire_icon.png");
		ResourceManager::LoadEditorIcon("character_target",		"res/Sunta/Textures/Icons/character_target_icon.png");
		ResourceManager::LoadEditorIcon("fog",					"res/Sunta/Textures/Icons/fog_icon.png");
		ResourceManager::LoadEditorIcon("state_machine",		"res/Sunta/Textures/Icons/state_machine_icon.png");

		ResourceManager::LoadEditorIcon("editor_window_bg",		"res/Sunta/Textures/ui/editor_window_background.jpg");
		ResourceManager::LoadEditorIcon("file_browser_bg",		"res/Sunta/Textures/ui/file_browser_background.jpg");

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
		SUNTA_ASSERT(scene, "CRITIC ERROR: Scene CAN'T BE NULL during Render");

		Renderer::Clear(0.02f, 0.01f, 0.01f, 1.0f);
	
		scene->Render(renderer);
		EditorGUIContext::RenderUI(window.get(), *scene, Renderer::GetDevice());
		
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
