#include "Core/SuntaPreCompiled.h"
#include "LinuxOpenGLEditorGUIBackend.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

namespace Sunta
{

LinuxOpenGLEditorGUIBackend::~LinuxOpenGLEditorGUIBackend()
{

}

void LinuxOpenGLEditorGUIBackend::Init(void* window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		// Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;     // Enable Pulling Window Out Of Application

	ImGui::StyleColorsDark();

	//ImGuiStyle& style = ImGui::GetStyle();
	//style.ScaleAllSizes(main_scale);
	//style.FontScaleDpi = main_scale;

	ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
}

void LinuxOpenGLEditorGUIBackend::Shutdown(void* window)
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void LinuxOpenGLEditorGUIBackend::NewFrame(void* window)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void LinuxOpenGLEditorGUIBackend::EndFrame(void* window)
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void LinuxOpenGLEditorGUIBackend::Render(void* window)
{

}

}