#include "Core/SuntaPreCompiled.h"

#include "Shader.h"

#include "Core/Log.h"

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

	while (getline(stream, line))
	{
		if (line.find("#shader") != std::string::npos)
		{
			if (line.find("vertex") != std::string::npos)
				type = ShaderType::VERTEX;
			else if (line.find("fragment") != std::string::npos)
				type = ShaderType::FRAGMENT;
		}
		else if(type != ShaderType::NONE)
		{
			ss[(int)type] << line << '\n';
		}
	}

	return  { ss[0].str(), ss[1].str() };
}

}