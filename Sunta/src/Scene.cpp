#include "GL/glew.h"

#include <glm/glm.hpp>

#include "VertexBuffer.h"
#include "ElementBuffer.h"

#include "Scene.h"
#include "Time.h"
#include "InputManager.h"
#include "Camera.h"
#include "ResourceManager.h"
#include "Primitives.h"
#include "Shader.h"
#include "Renderer.h"
#include "Entity.h"
#include "Material.h"
#include "LightSource.h"

Scene::Scene() 
	: camera(glm::vec3(1.2f, 5.6f, 11.35f))
{

}

Scene::~Scene()
{

}

void Scene::Init(float windowWidth, float windowHeight)
{
	OnWindowResize(windowWidth, windowHeight);

	ResourceManager::LoadTexture("cube_container",	"res/Textures/container.jpg");
	ResourceManager::LoadTexture("cube_chad",		"res/Textures/chad.png");
	ResourceManager::LoadShader ("reflectable",		"res/shaders/Reflectable.shader");
	ResourceManager::LoadShader ("lightSource",		"res/shaders/LightSource.shader");

	auto cubeShader		= ResourceManager::GetShaderData("reflectable");
	auto lightShader	= ResourceManager::GetShaderData("lightSource");
	auto cubeMaterial	= std::make_shared<Material>(cubeShader);
	auto cubeMesh		= Primitives::CreateCube();
	auto lightMesh		= Primitives::CreateCube();

	cubeMaterial->SetAmbient(glm::vec3(1.0f, 0.5f, 0.31f))
		.SetDiffuse(glm::vec3(1.0f, 0.5f, 0.31f))
		.SetSpecular(glm::vec3(0.5, 0.5f, 0.5f))
		.SetShininess(32.0f);

	entity = std::make_unique<Entity>(std::move(cubeMesh), cubeMaterial);
	entity->SetPosition(glm::vec3(1.0f, 4.0f, 1.0f));

	lightSource = std::make_unique<LightSource>(std::move(lightMesh), lightShader);
	lightSource->SetPosition(glm::vec3(3.0f, 6.0f, 2.0f));

	lightSource->SetAmbient(glm::vec3(0.2f, 0.2f, 0.2f))
		.SetDiffuse(glm::vec3(0.5f, 0.5f, 0.5f))
		.SetSpecular(glm::vec3(1.0f, 1.0f, 1.0f));

}

void Scene::OnWindowResize(float windowWidth, float windowHeight)
{
	camera.SetViewportSize(windowWidth, windowHeight);
}

void Scene::ProcessInput()
{
	float deltaTime				= Time::deltaTime;
	glm::vec2 mouseDelta		= InputManager::GetMouseDelta();

	camera.HandleMouseMovement(mouseDelta.x, mouseDelta.y);
	float scroll = InputManager::GetScrollOffset();
	if (scroll != 0.0f)
		camera.HandleScrolling(scroll);

	camera.HandleStayOnHeight(InputManager::IsKeyPressed(GLFW_KEY_LEFT_ALT));

	if (InputManager::IsKeyPressed(GLFW_KEY_W))
		camera.HandleKeyboardMove(MOVEMENT::FORWARD	, deltaTime);
	if (InputManager::IsKeyPressed(GLFW_KEY_S))
		camera.HandleKeyboardMove(MOVEMENT::BACKWARD, deltaTime);
	if (InputManager::IsKeyPressed(GLFW_KEY_A))
		camera.HandleKeyboardMove(MOVEMENT::LEFT	, deltaTime);
	if (InputManager::IsKeyPressed(GLFW_KEY_D))
		camera.HandleKeyboardMove(MOVEMENT::RIGHT	, deltaTime);

	if (InputManager::IsKeyPressed(GLFW_KEY_SPACE))
		camera.HandleKeyboardMove(MOVEMENT::UP		, deltaTime);
	if (InputManager::IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
		camera.HandleKeyboardMove(MOVEMENT::DOWN	, deltaTime);

}

void Scene::Update()
{
	float time = glfwGetTime();

	glm::vec3 lightSourceColor;
	lightSourceColor.x = sin(time * 2.0f);
	lightSourceColor.y = sin(time * 0.7f);
	lightSourceColor.z = sin(time * 1.3f);

	glm::vec3 diffuseColor = lightSourceColor * glm::vec3(0.5f);
	glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);

	auto mat = entity->GetMaterial();

	mat->SetAmbient(glm::vec3(ambientColor))
		.SetDiffuse(glm::vec3(diffuseColor))
		.SetSpecular(glm::vec3(0.5, 0.5f, 0.5f))
		.SetShininess(32.0f);
}

void Scene::Render(Renderer& renderer)
{
	if (!entity)
	{
		std::cout << "ERROR: Scene::Render Entity is NULL" << std::endl;
		return;
	}

	if (!entity->GetMesh())
	{
		std::cout << "ERROR: Scene::Render Entity's Mesh is NULL" << std::endl;
		return;
	}

	renderer.Clear(0.05f, 0.15f, 0.25f, 1.0f);
	
	SceneData data		= camera.GetSceneData();
	data.lightSource	= lightSource.get();

	renderer.BeginScene(data);

	renderer.DrawScene(*entity);

}

void Scene::Clear()
{

}
