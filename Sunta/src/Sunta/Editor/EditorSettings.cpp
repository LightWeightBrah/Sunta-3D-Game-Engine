#include "Core/SuntaPreCompiled.h"
#include "EditorSettings.h"

#include <nlohmann/json.hpp>

#include "Core/Log.h"

namespace Sunta
{

namespace
{

namespace fs = std::filesystem;

constexpr size_t MaxRecentProjects = 10;

constexpr const char* SettingsFolderName = "SuntaEngine";
constexpr const char* SettingsFileName   = "editor_settings.json";

namespace Keys
{
	constexpr const char* RecentProjects          = "recent_projects";
	constexpr const char* OpenLastProjectOnStart  = "open_last_project_on_startup";
	constexpr const char* BuildConfiguration      = "build_configuration";
	constexpr const char* CameraSpeed             = "camera_speed";
}

struct SettingsData
{
	std::vector<std::string> recentProjects;   // UTF-8, newest first
	bool        openLastProjectOnStartup = false;
	std::string buildConfiguration;
	float       cameraSpeed = 0.0f;
};

fs::path GetUserConfigDirectory()
{
#if defined(SUNTA_PLATFORM_WINDOWS)
	char*  appData = nullptr;
	size_t length  = 0;
	_dupenv_s(&appData, &length, "APPDATA");

	fs::path directory = appData ? fs::path(appData) : fs::temp_directory_path();
	free(appData);
	return directory;

#elif defined(SUNTA_PLATFORM_MAC)
	const char* home = std::getenv("HOME");
	return home ? fs::path(home) / "Library" / "Application Support" : fs::temp_directory_path();

#else
	if (const char* configHome = std::getenv("XDG_CONFIG_HOME"))
		return fs::path(configHome);

	const char* home = std::getenv("HOME");
	return home ? fs::path(home) / ".config" : fs::temp_directory_path();
#endif
}

fs::path GetSettingsFilePath()
{
	return GetUserConfigDirectory() / SettingsFolderName / SettingsFileName;
}

// One form of the path for comparing and storing: absolute, with '/' and without "..", UTF-8
std::string ToStoredPath(const fs::path& path)
{
	return fs::absolute(path).lexically_normal().generic_u8string();
}

fs::path FromStoredPath(const std::string& storedPath)
{
	return fs::u8path(storedPath);
}

SettingsData LoadSettings()
{
	SettingsData data;

	std::ifstream file(GetSettingsFilePath());
	if (!file.is_open())
		return data;

	try
	{
		nlohmann::json json;
		file >> json;

		data.recentProjects          = json.value(Keys::RecentProjects, std::vector<std::string>{});
		data.openLastProjectOnStartup = json.value(Keys::OpenLastProjectOnStart, false);
		data.buildConfiguration      = json.value(Keys::BuildConfiguration, std::string());
		data.cameraSpeed             = json.value(Keys::CameraSpeed, 0.0f);
	}
	catch (const nlohmann::json::exception& error)
	{
		SUNTA_ENGINE_LOG_WARNING("EditorSettings: Ignoring broken settings file: {0}", error.what());
		return SettingsData();
	}

	return data;
}

void SaveSettings(const SettingsData& data)
{
	nlohmann::ordered_json json;
	json[Keys::RecentProjects]         = data.recentProjects;
	json[Keys::OpenLastProjectOnStart] = data.openLastProjectOnStartup;
	json[Keys::BuildConfiguration]     = data.buildConfiguration;
	json[Keys::CameraSpeed]            = data.cameraSpeed;

	const fs::path settingsPath = GetSettingsFilePath();

	std::error_code error;
	fs::create_directories(settingsPath.parent_path(), error);

	std::ofstream file(settingsPath);
	if (!file.is_open())
	{
		SUNTA_ENGINE_LOG_WARNING("EditorSettings: Couldn't save '{0}'", settingsPath.string());
		return;
	}

	file << json.dump(1, '\t') << '\n';
}

SettingsData& GetData()
{
	static SettingsData data = LoadSettings();
	return data;
}

}

std::vector<fs::path> EditorSettings::GetRecentProjects()
{
	std::vector<fs::path> projects;
	for (const std::string& storedPath : GetData().recentProjects)
		projects.push_back(FromStoredPath(storedPath));

	return projects;
}

void EditorSettings::AddRecentProject(const fs::path& projectDirectory)
{
	SettingsData& data = GetData();
	const std::string storedPath = ToStoredPath(projectDirectory);

	// Moves the project to the front instead of keeping two entries of it
	data.recentProjects.erase(std::remove(data.recentProjects.begin(), data.recentProjects.end(), storedPath), data.recentProjects.end());
	data.recentProjects.insert(data.recentProjects.begin(), storedPath);

	if (data.recentProjects.size() > MaxRecentProjects)
		data.recentProjects.resize(MaxRecentProjects);

	SaveSettings(data);
}

void EditorSettings::RemoveRecentProject(const fs::path& projectDirectory)
{
	SettingsData& data = GetData();
	const std::string storedPath = ToStoredPath(projectDirectory);

	data.recentProjects.erase(std::remove(data.recentProjects.begin(), data.recentProjects.end(), storedPath), data.recentProjects.end());
	SaveSettings(data);
}

bool EditorSettings::ShouldOpenLastProjectOnStartup()
{
	return GetData().openLastProjectOnStartup;
}

void EditorSettings::SetOpenLastProjectOnStartup(bool shouldOpen)
{
	SettingsData& data = GetData();
	data.openLastProjectOnStartup = shouldOpen;
	SaveSettings(data);
}

std::string EditorSettings::GetBuildConfiguration()
{
	return GetData().buildConfiguration;
}

void EditorSettings::SetBuildConfiguration(const std::string& configurationName)
{
	SettingsData& data = GetData();
	data.buildConfiguration = configurationName;
	SaveSettings(data);
}

float EditorSettings::GetCameraSpeed()
{
	return GetData().cameraSpeed;
}

void EditorSettings::SetCameraSpeed(float speed)
{
	SettingsData& data = GetData();

	// Nothing to write when it didn't change (the editor calls this every time it closes)
	if (data.cameraSpeed == speed)
		return;

	data.cameraSpeed = speed;
	SaveSettings(data);
}

}
