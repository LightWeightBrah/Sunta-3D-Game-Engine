#include "Core/SuntaPreCompiled.h"
#include "AssetTemplates.h"

#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

#include "Core/Log.h"

namespace Sunta
{

namespace
{

// The same keys as SceneSerializer writes
constexpr const char* SceneNameKey = "scene_name";
constexpr const char* EntitiesKey  = "entities";
constexpr int         JsonIndent   = 4;

const char* const LuaScriptTemplate =
	"function OnStart()\n"
	"\tprint(this.name .. \" started, id = \" .. this.id)\n"
	"end\n"
	"\n"
	"function OnUpdate(deltaTime)\n"
	"\n"
	"end\n";

// Draws the mesh white, like the Unlit shader of the engine
const char* const ShaderTemplate =
	"#shader vertex\n"
	"\n"
	"layout (location = 0) in vec3 aPos;\n"
	"\n"
	"uniform mat4 model;\n"
	"uniform mat4 view;\n"
	"uniform mat4 projection;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tgl_Position = projection * view * model * vec4(aPos, 1.0);\n"
	"}\n"
	"\n"
	"#shader fragment\n"
	"\n"
	"out vec4 FragColor;\n"
	"\n"
	"void main()\n"
	"{\n"
	"\tFragColor = vec4(1.0);\n"
	"}\n";

bool WriteTextFile(const std::filesystem::path& path, const std::string& text)
{
	std::ofstream outFile(path);
	if (!outFile.is_open())
	{
		SUNTA_ENGINE_LOG_ERROR("AssetTemplates: Couldn't open file for writing: '{0}'", path.string());
		return false;
	}

	outFile << text;
	return outFile.good();
}

}

bool AssetTemplates::WriteScene(const std::filesystem::path& path)
{
	nlohmann::json sceneJson;
	sceneJson[SceneNameKey] = path.stem().string();
	sceneJson[EntitiesKey]  = nlohmann::json::array();

	return WriteTextFile(path, sceneJson.dump(JsonIndent));
}

bool AssetTemplates::WriteLuaScript(const std::filesystem::path& path)
{
	return WriteTextFile(path, LuaScriptTemplate);
}

bool AssetTemplates::WriteShader(const std::filesystem::path& path)
{
	return WriteTextFile(path, ShaderTemplate);
}

}
