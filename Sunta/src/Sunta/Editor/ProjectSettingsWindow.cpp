#include "Core/SuntaPreCompiled.h"
#include "ProjectSettingsWindow.h"

#include <imgui/imgui.h>

#include "EditorSettings.h"
#include "ExecutableIcon.h"
#include "GameBuilder.h"
#include "ImGuiTextInput.h"
#include "ProjectManager.h"
#include "Core/GameConfig.h"
#include "Core/Log.h"
#include "Core/Platform.h"
#include "Core/VirtualFileSystem.h"

namespace Sunta
{

namespace
{
	namespace fs = std::filesystem;

	constexpr const char* WindowTitle           = "Project Settings";
	constexpr const char* ProjectIconVirtualPath = "@game/App/window_icon.png";

	constexpr float WindowWidth        = 640.0f;
	constexpr int   MinWindowSize      = 320;
	constexpr int   MaxWindowSize      = 7680;
	constexpr int   RecommendedIconSize = 256;
	constexpr int   MinIconSize         = 16;

	const ImVec4 ErrorColor   = ImVec4(1.0f, 0.45f, 0.4f, 1.0f);
	const ImVec4 SuccessColor = ImVec4(0.5f, 0.9f, 0.5f, 1.0f);
	const ImVec4 WarningColor = ImVec4(1.0f, 0.8f, 0.4f, 1.0f);

	const std::vector<BuildConfiguration> AllConfigurations = { BuildConfiguration::Release, BuildConfiguration::Debug };

	// Everything the window remembers between frames
	struct WindowState
	{
		bool               isOpen        = false;
		bool               needsReload   = true;
		GameConfig         config;
		BuildConfiguration configuration = BuildConfiguration::Release;

		std::string statusMessage;
		ImVec4      statusColor = SuccessColor;

		int iconWidth  = 0;
		int iconHeight = 0;
	};

	WindowState state;

	void SetStatus(const std::string& message, const ImVec4& color)
	{
		state.statusMessage = message;
		state.statusColor   = color;
	}

	fs::path GetProjectIconPath()
	{
		return fs::path(VirtualFileSystem::Resolve(state.config.windowIcon));
	}

	void RefreshIconInfo()
	{
		state.iconWidth  = 0;
		state.iconHeight = 0;

		const fs::path iconPath = GetProjectIconPath();

		std::error_code error;
		if (fs::exists(iconPath, error))
			ExecutableIcon::TryReadImageSize(iconPath, state.iconWidth, state.iconHeight);
	}

	// Release is the version to share, but when only the configuration of the editor was compiled that one is chosen
	BuildConfiguration ChooseInitialConfiguration()
	{
		const std::string savedName = EditorSettings::GetBuildConfiguration();

		if (!savedName.empty())
			return GameBuilder::ParseConfiguration(savedName, BuildConfiguration::Release);

		return GameBuilder::IsConfigurationAvailable(BuildConfiguration::Release) ? BuildConfiguration::Release : GameBuilder::GetEditorConfiguration();
	}

	void Reload()
	{
		state.config = GameConfig();
		GameConfig::TryLoad(GameConfig::DefaultPath, state.config);

		state.configuration = ChooseInitialConfiguration();
		state.needsReload   = false;

		RefreshIconInfo();
	}

	void Open()
	{
		state.isOpen      = true;
		state.needsReload = true;
	}

	bool Save()
	{
		const bool isSaved = GameConfig::TrySave(GameConfig::DefaultPath, state.config);

		if (isSaved)
			SetStatus("Saved to game.config", SuccessColor);
		else
			SetStatus("Couldn't save game.config, see the log", ErrorColor);

		return isSaved;
	}

	void SetConfiguration(BuildConfiguration configuration)
	{
		state.configuration = configuration;
		EditorSettings::SetBuildConfiguration(GameBuilder::GetConfigurationName(configuration));
	}

	void Build(BuildConfiguration configuration)
	{
		if (GameBuilder::BuildGame(configuration))
			SetStatus(std::string("Build finished (") + GameBuilder::GetConfigurationName(configuration) + "), the folder was opened", SuccessColor);
		else
			SetStatus("Build failed, see the log for the reason", ErrorColor);
	}

	std::string GetConfigurationLabel(BuildConfiguration configuration)
	{
		std::string label = GameBuilder::GetConfigurationName(configuration);

		if (configuration == GameBuilder::GetEditorConfiguration())
			label += "  (the same as this editor)";
		else if (!GameBuilder::IsConfigurationAvailable(configuration))
			label += "  (not compiled yet)";

		return label;
	}

	// Copies the chosen PNG to the project (always the same file, game.config points to it)
	void ChooseIcon()
	{
		const std::string chosen = Platform::Get().OpenFileDialog("PNG image", { "png" }, "");
		if (chosen.empty())
			return;

		const fs::path chosenPath = fs::absolute(fs::path(chosen));

		int width = 0, height = 0;
		if (!ExecutableIcon::TryReadImageSize(chosenPath, width, height))
		{
			SetStatus("This file can't be read as an image", ErrorColor);
			return;
		}

		const fs::path targetPath = fs::path(VirtualFileSystem::Resolve(ProjectIconVirtualPath));

		std::error_code error;
		fs::create_directories(targetPath.parent_path(), error);

		const bool isSameFile = fs::exists(targetPath, error) && fs::equivalent(chosenPath, targetPath, error);
		if (!isSameFile)
			fs::copy_file(chosenPath, targetPath, fs::copy_options::overwrite_existing, error);

		if (error)
		{
			SetStatus("Couldn't copy the icon into the project: " + error.message(), ErrorColor);
			return;
		}

		state.config.windowIcon = ProjectIconVirtualPath;
		RefreshIconInfo();
		Save();
	}

	void RemoveIcon()
	{
		std::error_code error;
		fs::remove(GetProjectIconPath(), error);

		RefreshIconInfo();
		SetStatus("Icon removed, the game will use the icon of the engine", SuccessColor);
	}

	void ChooseStartScene()
	{
		const std::string startFolder = fs::path(VirtualFileSystem::Resolve("@game/Scenes")).make_preferred().string();
		const std::string chosen      = Platform::Get().OpenFileDialog("Scene Files", { "scene" }, startFolder);

		if (!chosen.empty())
			state.config.startScene = VirtualFileSystem::ToVirtualPath(chosen);
	}

	void DrawSectionTitle(const char* title)
	{
		ImGui::Spacing();
		ImGui::TextUnformatted(title);
		ImGui::Separator();
	}

	void DrawGameSection()
	{
		DrawSectionTitle("Game");

		InputString("Name", state.config.name);

		InputString("Start scene", state.config.startScene);
		ImGui::SameLine();
		if (ImGui::Button("Choose scene..."))
			ChooseStartScene();

		int windowSize[2] = { static_cast<int>(state.config.windowWidth), static_cast<int>(state.config.windowHeight) };
		if (ImGui::InputInt2("Window size", windowSize))
		{
			state.config.windowWidth  = static_cast<unsigned int>(std::clamp(windowSize[0], MinWindowSize, MaxWindowSize));
			state.config.windowHeight = static_cast<unsigned int>(std::clamp(windowSize[1], MinWindowSize, MaxWindowSize));
		}
	}

	void DrawIconStatus()
	{
		if (state.iconWidth == 0)
		{
			ImGui::TextDisabled("No icon. The game uses the icon of the engine.");
			return;
		}

		ImGui::Text("Icon: %d x %d  (%s)", state.iconWidth, state.iconHeight, state.config.windowIcon.c_str());

		if (state.iconWidth != state.iconHeight)
			ImGui::TextColored(WarningColor, "The image is not square, it will be centered with transparent margins.");
		else if (state.iconWidth < RecommendedIconSize)
			ImGui::TextColored(WarningColor, "Use at least %d x %d pixels for a sharp icon.", RecommendedIconSize, RecommendedIconSize);
	}

	void DrawIconSection()
	{
		DrawSectionTitle("Icon");

		DrawIconStatus();

		if (ImGui::Button("Choose icon (PNG)..."))
			ChooseIcon();

		if (state.iconWidth != 0)
		{
			ImGui::SameLine();
			if (ImGui::Button("Remove icon"))
				RemoveIcon();
		}

		ImGui::TextDisabled("The window of the game gets the icon at runtime. The icon of the game file is made from this PNG while building:");
		ImGui::TextDisabled("the .exe on Windows, the .icns inside the .app on macOS, the PNG itself on Linux.");
		ImGui::TextDisabled("Windows may show the old icon for a while: it keeps icons in a cache.");
	}

	void DrawConfigurationCombo()
	{
		const std::string selectedLabel = GetConfigurationLabel(state.configuration);

		if (!ImGui::BeginCombo("Configuration", selectedLabel.c_str()))
			return;

		for (BuildConfiguration configuration : AllConfigurations)
		{
			const bool isSelected = (configuration == state.configuration);

			if (ImGui::Selectable(GetConfigurationLabel(configuration).c_str(), isSelected))
				SetConfiguration(configuration);

			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	void DrawBuildSection()
	{
		DrawSectionTitle("Build");

		DrawConfigurationCombo();

		const bool isAvailable = GameBuilder::IsConfigurationAvailable(state.configuration);

		if (!isAvailable)
		{
			ImGui::TextColored(WarningColor, "The game executable for %s was not compiled yet.", GameBuilder::GetConfigurationName(state.configuration));
			ImGui::TextDisabled("Compile the SuntaRuntime target in this configuration (see README), then try again.");
		}
		else if (state.configuration == BuildConfiguration::Debug)
		{
			ImGui::TextDisabled("Debug is slow and needs the debug runtime of the compiler. Use Release for the version you share.");
		}

		if (ImGui::Button("Save"))
			Save();

		ImGui::SameLine();

		ImGui::BeginDisabled(!isAvailable);
		if (ImGui::Button("Save and Build Game"))
		{
			if (Save())
				Build(state.configuration);
		}
		ImGui::EndDisabled();
	}
}

void ProjectSettingsWindow::DrawProjectMenuItems()
{
	if (ImGui::MenuItem("Project Settings..."))
		Open();

	if (ImGui::MenuItem("Build Game"))
	{
		// The window shows why a build can't be done (e.g. the configuration is not compiled)
		const BuildConfiguration configuration = ChooseInitialConfiguration();

		if (GameBuilder::IsConfigurationAvailable(configuration))
			Build(configuration);
		else
			Open();
	}

	ImGui::Separator();

	if (ImGui::MenuItem("Switch Project..."))
		ProjectManager::RequestSwitchProject();
}

void ProjectSettingsWindow::Draw()
{
	if (!state.isOpen)
		return;

	if (state.needsReload)
		Reload();

	ImGui::SetNextWindowSize(ImVec2(WindowWidth, 0.0f), ImGuiCond_Appearing);

	if (ImGui::Begin(WindowTitle, &state.isOpen))
	{
		ImGui::TextDisabled("%s", ProjectManager::GetCurrentProjectDirectory().string().c_str());

		DrawGameSection();
		DrawIconSection();
		DrawBuildSection();

		if (!state.statusMessage.empty())
		{
			ImGui::Spacing();
			ImGui::TextColored(state.statusColor, "%s", state.statusMessage.c_str());
		}
	}
	ImGui::End();
}

}
