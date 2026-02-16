#include "Engine.h"

int main()
{
	Engine engine;
	engine.Run();
}

//#include <iostream>
//#include <tuple>
//#include <string>
//#include <vector>
//#include <fstream>
//#include <sstream>
//
//#include "main.h"
//#include "Node.h"
//#include "Grid.h"
//#include "AStar.h"
//#include "Renderer.h"
//#include "Camera.h"
//#include "Model.h"
//
//#include "imgui/imgui.h"
//#include "imgui/imgui_impl_glfw.h"
//#include "imgui/imgui_impl_opengl3.h"
//
//#include "GL/glew.h"
//#include <GLFW/glfw3.h>
//#include "stb/stb_image.h"
//
//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//
//#include <assimp/Importer.hpp>
//#include <assimp/version.h>
//#include "Animator.h"
//#include "Solaire.h"
//#include "Event.h"
//
//const unsigned int WINDOW_WIDTH = 1200;
//const unsigned int WINDOW_HEIGHT = 800;
//
//const unsigned int WALL_HEIGHT = 2;
//
//float deltaTime = 0.0f;
//float lastFrame = 0.0f;
//
//float lastX = WINDOW_WIDTH / 2;
//float lastY = WINDOW_HEIGHT / 2;
//
//bool firstMouse = true;
//bool isMenuOpen = false;
//
//Camera camera(glm::vec3(9.4f, 34.2f, 30.8f), -60.1f, -89.9);
//
//AStar aStar("grid.txt");
//
//Event onResetEvent;
//Event onFullPathEvent;
//Event onMenuEvent;
//
//void OnWindowResized(GLFWwindow* window, int width, int height)
//{
//	glViewport(0, 0, width, height);
//}
//
//void HandleInput(GLFWwindow* window)
//{
//	const float cameraSpeed = 2.5f * deltaTime;
//
//	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
//		camera.HandleStayOnHeight(true);
//	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE)
//		camera.HandleStayOnHeight(false);
//
//	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
//		camera.HandleKeyboardMove(MOVEMENT::FORWARD, deltaTime);
//	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
//		camera.HandleKeyboardMove(MOVEMENT::BACKWARD, deltaTime);
//	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
//		camera.HandleKeyboardMove(MOVEMENT::LEFT, deltaTime);
//	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
//		camera.HandleKeyboardMove(MOVEMENT::RIGHT, deltaTime);
//	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
//		camera.HandleKeyboardMove(MOVEMENT::UP, deltaTime);
//	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
//		camera.HandleKeyboardMove(MOVEMENT::DOWN, deltaTime);
//}
//
//void OnSingleKey(GLFWwindow* window, int key, int scancode, int action, int mods)
//{
//	if (action == GLFW_PRESS)
//	{
//		switch (key)
//		{
//		case GLFW_KEY_R:
//			onResetEvent.Invoke();
//			break;
//
//		case GLFW_KEY_F:
//			onResetEvent.Invoke();
//			onFullPathEvent.Invoke();
//			break;
//
//		case GLFW_KEY_TAB:
//			onMenuEvent.Invoke();
//			break;
//
//		case GLFW_KEY_ESCAPE:
//			glfwSetWindowShouldClose(window, true);
//			break;
//		}
//	}
//}
//
//void OnMenuOpen(GLFWwindow* window)
//{
//	isMenuOpen = !isMenuOpen;
//
//	if (isMenuOpen)
//	{
//		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
//	}
//	else
//	{
//		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//		firstMouse = true;
//	}
//}
//
//void OnMouse(GLFWwindow* window, double xPos, double yPos)
//{
//	if (isMenuOpen)
//		return;
//
//	if (firstMouse)
//	{
//		lastX = xPos;
//		lastY = yPos;
//		firstMouse = false;
//	}
//
//	float xoffset = xPos  - lastX;
//	float yoffset = lastY - yPos;
//	
//	lastX = xPos;
//	lastY = yPos;
//
//	camera.HandleMouseMovement(xoffset, yoffset);
//}
//
//void OnScroll(GLFWwindow* window, double xOffset, double yOffset)
//{
//	//camera.HandleScrolling(yOffset);
//}
//
//float verticies[] = {
//	//positions				//colors			//textures
//	//front
//	 0.5f,  0.5f,  0.5f,	0.0f, 0.0f, 1.0f,	 1.0f, 1.0f,	//top right		
//	 0.5f, -0.5f,  0.5f,	0.0f, 0.0f, 1.0f,	 1.0f, 0.0f,	//bottom right	
//	-0.5f, -0.5f,  0.5f,	0.0f, 0.0f, 1.0f,	 0.0f, 0.0f,	//bottom left	
//	-0.5f,  0.5f,  0.5f,	0.0f, 0.0f, 1.0f,    0.0f, 1.0f,	//top left		
//
//	//back
//	 0.5f,  0.5f, -0.5f,	0.0f, 0.0f, -1.0f,	 1.0f, 1.0f,	//top right		
//	 0.5f, -0.5f, -0.5f,	0.0f, 0.0f, -1.0f,	 1.0f, 0.0f,	//bottom right	
//	-0.5f, -0.5f, -0.5f,	0.0f, 0.0f, -1.0f,	 0.0f, 0.0f,	//bottom left	
//	-0.5f,  0.5f, -0.5f,	0.0f, 0.0f, -1.0f,   0.0f, 1.0f,	//top left		
//
//	//up
//	 0.5f,  0.5f, -0.5f,	0.0f, 1.0f, 0.0f,	 1.0f, 1.0f,	//top right		
//	 0.5f,  0.5f,  0.5f,	0.0f, 1.0f, 0.0f,	 1.0f, 0.0f,	//bottom right	
//	-0.5f,  0.5f,  0.5f,	0.0f, 1.0f, 0.0f,	 0.0f, 0.0f,	//bottom left	
//	-0.5f,  0.5f, -0.5f,	0.0f, 1.0f, 0.0f,    0.0f, 1.0f,	//top left		
//
//	//bottom
//	 0.5f, -0.5f, -0.5f,	0.0f, -1.0f, 0.0f,	 1.0f, 1.0f,	//top right		
//	 0.5f, -0.5f,  0.5f,	0.0f, -1.0f, 0.0f,	 1.0f, 0.0f,	//bottom right	
//	-0.5f, -0.5f,  0.5f,	0.0f, -1.0f, 0.0f,	 0.0f, 0.0f,	//bottom left	
//	-0.5f, -0.5f, -0.5f,	0.0f, -1.0f, 0.0f,   0.0f, 1.0f,	//top left
//
//	//right
//	 0.5f,  0.5f, -0.5f,	1.0f, 0.0f, 0.0f,	 1.0f, 1.0f,	//top right		
//	 0.5f, -0.5f, -0.5f,	1.0f, 0.0f, 0.0f,	 1.0f, 0.0f,	//bottom right	
//	 0.5f, -0.5f,  0.5f,	1.0f, 0.0f, 0.0f,	 0.0f, 0.0f,	//bottom left	
//	 0.5f,  0.5f,  0.5f,	1.0f, 0.0f, 0.0f,    0.0f, 1.0f,	//top left		
//
//	//left
//	-0.5f,  0.5f,  0.5f,	-1.0f, 0.0f, 0.0f,	 1.0f, 1.0f,	//top right		
//	-0.5f, -0.5f,  0.5f,	-1.0f, 0.0f, 0.0f,	 1.0f, 0.0f,	//bottom right	
//	-0.5f, -0.5f, -0.5f,	-1.0f, 0.0f, 0.0f,	 0.0f, 0.0f,	//bottom left	
//	-0.5f,  0.5f, -0.5f,	-1.0f, 0.0f, 0.0f,   0.0f, 1.0f,	//top left		
//};
//
//unsigned int indicies[] = {
//	//front
//	0,  1,  2, //first triangle  
//	2,  3,  0, //2nd triangle	   
//	//back					   
//	4,  5,  6, 				   
//	6,  7,  4,				   
//	//up
//	8,  9,  10,
//	10, 11,  8,
//	//down
//	12, 13, 14,
//	14, 15, 12,
//	////right
//	16, 17, 18,
//	18, 19, 16,
//	//left
//	20, 21, 22,
//	22, 23, 20,
//};
//
//int main()
//{
//	glfwInit();
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//
//	float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
//
//	GLFWwindow* window = glfwCreateWindow((int)(WINDOW_WIDTH * main_scale), (int)(WINDOW_HEIGHT * main_scale), "SunFinder", NULL, NULL);
//	if (!window)
//	{
//		std::cout << "ERROR: Failed to create GLFW window" << std::endl;
//		glfwTerminate();
//		return -1;
//	}
//	glfwMakeContextCurrent(window);
//	if (glewInit() != GLEW_OK)
//	{
//		std::cout << "ERROR: Failed to initalize GLEW" << std::endl;
//		return -1;
//	}
//	
//	//INFO DEBUG
//	std::cout << glGetString(GL_VERSION) << std::endl;
//	
//	int nrAttributes;
//	GLCall(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes));
//	std::cout << "Maximum nr of vertex attributes supported: " << nrAttributes << std::endl;
//
//	Assimp::Importer importer;
//	std::cout << "Wersja Assimpa: " << aiGetVersionMajor() << std::endl;
//	//INFO DEBUG
//
//	GLCall(glEnable(GL_DEPTH_TEST));
//	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
//
//	glfwSetFramebufferSizeCallback(window, OnWindowResized);
//	glfwSetCursorPosCallback(window, OnMouse);
//	glfwSetKeyCallback(window, OnSingleKey);
//	glfwSetScrollCallback(window, OnScroll);
//
//	Renderer renderer;
//
//	//SETUP IM_GUI
//	IMGUI_CHECKVERSION();
//	ImGui::CreateContext();
//	ImGuiIO& io = ImGui::GetIO(); (void)io;
//	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
//
//	ImGui::StyleColorsDark();
//
//	ImGuiStyle& style = ImGui::GetStyle();
//	style.ScaleAllSizes(main_scale);
//	style.FontScaleDpi = main_scale;
//
//	ImGui_ImplGlfw_InitForOpenGL(window, true);
//	ImGui_ImplOpenGL3_Init("#version 330");
//	//SETUP IM_GUI
//
//
//
//	renderer.Clear(0.05f, 0.05f, 0.05f, 1.0f);
//	glfwSwapBuffers(window);
//	glfwPollEvents();
//
//	{
//		Shader shader("res/shaders/Basic.shader");
//
//		VertexArray   VAO;
//		VertexBuffer  VBO(verticies, sizeof(verticies));
//		ElementBuffer EBO(indicies, sizeof(indicies));
//
//		VAO.AddAttrib(0, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)0);
//		VAO.AddAttrib(1, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)(3 * sizeof(float)));
//		VAO.AddAttrib(2, 2, GL_FLOAT, false, 8 * sizeof(float), (void*)(6 * sizeof(float)));
//
//		Texture texture1("res/Textures/container.jpg");
//		Texture texture2("res/Textures/chad.png");
//
//		shader.SetUniform1i("texture1", 0);
//		shader.SetUniform1i("texture2", 1);
//
//		VAO.Unbind();
//		VBO.Unbind();
//		EBO.Unbind();
//		shader.Unbind();
//
//		ResourceManager::LoadModel("Solaire", "res/Models/solair masterpiece/Solaire All Animations.fbx");
//		Solaire solaireEntity(glm::vec3(0.0f, 0.55f, 19.0f), "Solaire");
//
//		onResetEvent.AddListener([&]() { solaireEntity.Reset(); });
//		onResetEvent.AddListener([&]() { aStar.Reset(); });
//
//		onFullPathEvent.AddListener([&]() { aStar.FindPathFull(); });
//		
//		onMenuEvent.AddListener([&]() { OnMenuOpen(window); });
//
//		Shader characterShader("res/shaders/Model.shader");
//		characterShader.Unbind();
//
//
//		//TESTING LIGHTING BELOW
//		//TESTING LIGHTING BELOW
//		//TESTING LIGHTING BELOW
//
//		//LIGHT SOURCE
//		Shader lightSourceShader("res/shaders/LightSource.shader");
//
//		VertexArray   lightSourceVAO;
//		VertexBuffer  lightSourceVBO(verticies, sizeof(verticies));
//		ElementBuffer lightSourceEBO(indicies, sizeof(indicies));
//
//		lightSourceVAO.AddAttrib(0, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)0);
//		lightSourceVAO.AddAttrib(1, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)(3 * sizeof(float)));
//		lightSourceVAO.AddAttrib(2, 2, GL_FLOAT, false, 8 * sizeof(float), (void*)(6 * sizeof(float)));
//
//		lightSourceVAO.Unbind();
//		lightSourceVBO.Unbind();
//		lightSourceEBO.Unbind();
//		lightSourceShader.Unbind();
//
//		//REFLECTABLE OBJECT
//		Shader reflectableShader("res/shaders/Reflectable.shader");
//
//		VertexArray   reflectableVAO;
//		VertexBuffer  reflectableVBO(verticies, sizeof(verticies));
//		ElementBuffer reflectableEBO(indicies, sizeof(indicies));
//
//		reflectableVAO.AddAttrib(0, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)0);
//		reflectableVAO.AddAttrib(1, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)(3 * sizeof(float)));
//		reflectableVAO.AddAttrib(2, 2, GL_FLOAT, false, 8 * sizeof(float), (void*)(6 * sizeof(float)));
//
//		reflectableVAO.Unbind();
//		reflectableVBO.Unbind();
//		reflectableEBO.Unbind();
//		reflectableShader.Unbind();
//
//		//TESTING LIGHTING ABOVE
//		//TESTING LIGHTING ABOVE
//		//TESTING LIGHTING ABOVE
//
//		const float lightStartPosition = 17.0f;
//		glm::vec3 lightSourcePosition = glm::vec3(lightStartPosition);
//
//		glm::vec3 materialAmbientColor  = glm::vec3(0.2f);
//		glm::vec3 materialDiffuseColor  = glm::vec3(0.5f);
//		glm::vec3 materialSpecularColor = glm::vec3(0.5f);
//		float shininess = 32.0f;
//
//		glm::vec3 lightSourceAmbientColor = glm::vec3(0.2f);
//		glm::vec3 lightSourceDiffuseColor = glm::vec3(0.5f);
//		glm::vec3 lightSourceSpecularColor = glm::vec3(1.0f);
//
//		while (!glfwWindowShouldClose(window))
//		{
//			float currentFrame = glfwGetTime();
//			deltaTime = currentFrame - lastFrame;
//			lastFrame = currentFrame;
//
//			HandleInput(window);
//
//			renderer.Clear(0.05f, 0.05f, 0.05f, 1.0f);
//
//			//IM_GUI START
//			ImGui_ImplOpenGL3_NewFrame();
//			ImGui_ImplGlfw_NewFrame();
//			ImGui::NewFrame();
//			ImGui::Begin("Oh, hello there...");
//
//			shader.Bind();
//			texture1.Bind();
//			texture2.Bind(1);
//
//			glm::mat4 projection = glm::perspective(glm::radians(camera.GetFov()), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
//			shader.SetUniformMatrix4fv("projection", projection);
//
//			glm::mat4 view = camera.GetViewMatrix();
//			shader.SetUniformMatrix4fv("view", view);
//
//			aStar.FindPathBySteps(deltaTime);
//
//			for (int z = 0; z < aStar.height; z++)
//			{
//				for (int x = 0; x < aStar.width; x++)
//				{
//					bool isWall = false;
//
//					glm::vec3 color;
//					Node* currentNode = &aStar.grid[z][x];
//
//					shader.Bind();
//					texture1.Bind();
//					texture2.Bind(1);
//
//					if (currentNode == aStar.GetStartNode())
//					{
//						color = glm::vec3(1.0f, 1.0f, 1.0f);
//					}
//					else if (currentNode == aStar.GetEndNode())
//					{
//						color = glm::vec3(1.0f, 1.0f, 1.0f);
//					}
//					else
//					{
//						switch (currentNode->cellType)
//						{
//						case CELL::WALL:	color = glm::vec3(1.0f, 0.0f, 0.0f); isWall = true; break;
//						case CELL::ROUTE:	color = glm::vec3(1.0f, 1.0f, 0.0f);				break;
//						case CELL::OPEN:	color = glm::vec3(0.36f, 0.84f, 0.42f);				break;
//						case CELL::CLOSED:	color = glm::vec3(0.0f, 0.0f, 0.3f);				break;
//						default:			color = glm::vec3(0.6f, 0.0f, 0.6f);				break;
//						}
//					}
//
//					shader.SetUniform3f("objectColor", color.x, color.y, color.z);
//
//					int maxHeight = isWall ? WALL_HEIGHT : 1;
//					for (int y = 0; y < maxHeight; y++)
//					{
//						glm::mat4 model = glm::mat4(1.0f);
//						model = glm::translate(model, glm::vec3(x, y, z));
//
//						shader.SetUniformMatrix4fv("model", model);
//						renderer.Draw(VAO, EBO, shader);
//					}
//
//					
//				}
//			}
//
//			characterShader.Bind();
//			characterShader.SetUniform1i("texture_diffuse1", 0);
//			characterShader.SetUniformMatrix4fv("projection", projection);
//			characterShader.SetUniformMatrix4fv("view", view);
//
//			if (aStar.IsFinished())
//			{
//				auto movingPath = aStar.GetFullPath();
//				if (!movingPath.empty())
//				{
//					solaireEntity.SetPath(movingPath);
//				}
//			}
//
//			solaireEntity.Update(deltaTime);
//
//			glm::mat4 solaireModelMatrix = solaireEntity.GetModelMatrix();
//			characterShader.SetUniformMatrix4fv("model", solaireModelMatrix);
//			
//			renderer.DrawModel(solaireEntity.GetCharacterModel(), characterShader, solaireEntity.GetAnimator());
//			 
//
//
//			//TESTING LIGHTING BELOW
//			//TESTING LIGHTING BELOW
//			//TESTING LIGHTING BELOW
//
//
//			//LIGHT SOURCE
//			glm::mat4 lightSourceModelMatrix = glm::mat4(1.0f);
//			lightSourceModelMatrix = glm::translate(lightSourceModelMatrix, lightSourcePosition);
//			lightSourceModelMatrix = glm::scale(lightSourceModelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
//			
//			lightSourceVAO.Bind();
//			lightSourceEBO.Bind();
//			lightSourceShader.Bind();
//
//			lightSourceShader.SetUniformMatrix4fv("projection",		projection			  );
//			lightSourceShader.SetUniformMatrix4fv("view"      ,		view				  );
//			lightSourceShader.SetUniformMatrix4fv("model"     ,		lightSourceModelMatrix);
//
//
//			//REFLECTABLE OBJECT
//			glm::mat4 reflectableModelMatrix = glm::mat4(1.0f);
//			reflectableModelMatrix = glm::translate(reflectableModelMatrix, glm::vec3(14.0f, 18.0f, 23.0f));
//			reflectableVAO.Bind();
//			reflectableEBO.Bind();
//			reflectableShader.Bind();
//
//			reflectableShader.SetUniformMatrix4fv("projection",		projection			  );
//			reflectableShader.SetUniformMatrix4fv("view"	  ,		view				  );
//			reflectableShader.SetUniformMatrix4fv("model"	  ,		reflectableModelMatrix);
//
//			//MATERIAL
//			ImGui::Text("Material settings:");
//			ImGui::ColorEdit3("materialAmbientColor" ,		(float*)(&materialAmbientColor)   );
//			ImGui::ColorEdit3("materialDiffuseColor" ,		(float*)(&materialDiffuseColor)   );
//			ImGui::ColorEdit3("materialSpecularColor",		(float*)(&materialSpecularColor)  );
//			ImGui::SliderFloat("shininess"			 ,		(float*)(&shininess), 0.0f, 256.0f);
//
//			reflectableShader.SetUniform3f("material.ambientColor" ,	materialAmbientColor);
//			reflectableShader.SetUniform3f("material.diffuseColor" ,	materialDiffuseColor);
//			reflectableShader.SetUniform3f("material.specularColor",	materialSpecularColor);
//			reflectableShader.SetUniform1f("material.shininess"	   ,	shininess);
//			//MATERIAL
//
//			//LIGHT SOURCE
//			ImGui::Text("Light source settings:");
//			ImGui::ColorEdit3("lightSourceAmbientColor" ,		(float*)(&lightSourceAmbientColor));
//			ImGui::ColorEdit3("lightSourceDiffuseColor" ,		(float*)(&lightSourceDiffuseColor));
//			ImGui::ColorEdit3("lightSourceSpecularColor",		(float*)(&lightSourceSpecularColor));
//			ImGui::SliderFloat3("lightSourcePosition"   ,		(float*)(&lightSourcePosition), -10.0f + lightStartPosition, 10.0f + lightStartPosition);
//
//			reflectableShader.SetUniform3f("lightSource.position"        ,	lightSourcePosition);
//			reflectableShader.SetUniform3f("lightSource.ambientStrength" ,  lightSourceAmbientColor);
//			reflectableShader.SetUniform3f("lightSource.diffuseStrength" ,  lightSourceDiffuseColor);
//			reflectableShader.SetUniform3f("lightSource.specularStrength",  lightSourceSpecularColor);
//			//LIGHT SOURCE
//		
//			reflectableShader.SetUniform3f("viewerPosition", camera.GetPosition());
//
//			renderer.Draw(reflectableVAO, reflectableEBO, reflectableShader);
//			renderer.Draw(lightSourceVAO, lightSourceEBO, lightSourceShader);
//
//			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
//			ImGui::End();
//
//			ImGui::Render();
//			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//
//			glfwSwapBuffers(window);
//			glfwPollEvents();
//		}
//	}
//
//	//IM_GUI CLEANUP
//	ImGui_ImplOpenGL3_Shutdown();
//	ImGui_ImplGlfw_Shutdown();
//	ImGui::DestroyContext();
//
//	glfwTerminate();
//	return 0;
//}