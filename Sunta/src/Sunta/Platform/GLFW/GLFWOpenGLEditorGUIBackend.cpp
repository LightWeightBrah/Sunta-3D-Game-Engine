#include "Core/SuntaPreCompiled.h"
#include "GLFWOpenGLEditorGUIBackend.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "Renderer/RendererAPI.h"
#include "Core/InputPlayback.h"
#include <GLFW/glfw3.h>

namespace Sunta
{

GLFWOpenGLEditorGUIBackend::~GLFWOpenGLEditorGUIBackend()
{

}

void GLFWOpenGLEditorGUIBackend::Init(void* window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		// Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;     // Enable Pulling Window Out Of Application

	//ImGuiStyle& style = ImGui::GetStyle();
	//style.ScaleAllSizes(main_scale);
	//style.FontScaleDpi = main_scale;

	ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)window, true);

	// ImplOpenGL3 is OpenGL beggining from version 3.0 (first MODERN OpenGL),
	// so it'll be the same even for (OpenGL version 4.6.0), it just means MODERN OpenGL
	// 
	//					     e.g #version 330
	const char* GLSLVersion = RendererAPI::GetConfig().ImGuiGLSLVersion.c_str();
	ImGui_ImplOpenGL3_Init(GLSLVersion);
}

void GLFWOpenGLEditorGUIBackend::Shutdown(void* window)
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void GLFWOpenGLEditorGUIBackend::NewFrame(void* window)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	// A test script drives the pointer: its position and its buttons, as if the mouse did it (with viewports the
	// GUI works in the coordinates of the desktop)
	glm::vec2 cursor;
	if (InputPlayback::TryGetCursor(cursor))
	{
		ImGuiIO& io = ImGui::GetIO();
		int windowX = 0, windowY = 0;
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			glfwGetWindowPos((GLFWwindow*)window, &windowX, &windowY);
		// (a hidden window of a background test run never gets the system's focus: the script's clicks count anyway)
		io.AddFocusEvent(true);
		io.AddMousePosEvent(windowX + cursor.x, windowY + cursor.y);
		// (each press and release on its own: ImGui trickles them over the next frames, so a double click is seen)
		for (const auto& [button, isDown] : InputPlayback::TakeMouseButtonChanges())
		{
			if (button >= 0 && button < 3)
				io.AddMouseButtonEvent(button, isDown);
		}
	}
	ImGui::NewFrame();
}

void GLFWOpenGLEditorGUIBackend::EndFrame(void* window)
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void GLFWOpenGLEditorGUIBackend::Render(void* window)
{

}

}