#include "Core/SuntaPreCompiled.h"

#include "Shader.h"

#include "Core/Log.h"
#include "RendererAPI.h"

namespace Sunta
{

ShaderProgramSource Shader::ParseShader()
{
	std::ifstream stream(filepath);

	if (!stream.is_open())
	{
		SUNTA_ENGINE_LOG_ERROR("Couldn't open shader file: {0}", filepath);
		return { "", "" };
	}

	enum class ShaderType
	{
		NONE = -1,
		VERTEX = 0,
		FRAGMENT = 1
	};

	std::string line;
	std::stringstream ss[2];
	ShaderType type = ShaderType::NONE;

	std::string versionHeader = RendererAPI::GetConfig().GLSLVersion + "\n";
	bool versionWarningLogged = false;

	while (getline(stream, line))
	{
		if (line.find("#shader") != std::string::npos)
		{
			if (line.find("vertex") != std::string::npos)
			{
				type = ShaderType::VERTEX;
				ss[(int)type] << versionHeader;
			}
			else if (line.find("fragment") != std::string::npos)
			{
				type = ShaderType::FRAGMENT;
				ss[(int)type] << versionHeader;
			}
		}
		else if (line.find("#version") != std::string::npos)
		{
			// ignore line if found #version since we have
			// global RendererAPI GLSL version config now
			// this just prevents possible erros (safety check)
			if (!versionWarningLogged)
			{
				SUNTA_ENGINE_LOG_WARNING(R"([Shader Parser]: file '{0}': contains '#version' in your source code. 
Automatically changed it to version '{1}' based on global RendererAPI config)", 
				filepath, 
				RendererAPI::GetConfig().GLSLVersion);

				versionWarningLogged = true;
			}
			continue;
		}
		else if(type != ShaderType::NONE)
		{
			ss[(int)type] << line << '\n';
		}
	}

	return  { ss[0].str(), ss[1].str() };
}

}