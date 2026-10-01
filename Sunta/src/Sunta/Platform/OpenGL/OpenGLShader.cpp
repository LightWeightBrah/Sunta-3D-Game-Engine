#include "Core/SuntaPreCompiled.h"
#include "OpenGLShader.h"

#include <glad/glad.h>

#include "Core/Log.h"
#include "OpenGLUtilities.h"

namespace Sunta
{
OpenGLShader::OpenGLShader(const std::string& filepath)
	: Shader(filepath)
	, id(0)
{
	ShaderProgramSource source = ParseShader();
	id = CreateProgram(source.vertexSource, source.fragmentSource);
	Bind();
}

OpenGLShader::~OpenGLShader()
{
	GLCall(glDeleteProgram(id));
}

unsigned int OpenGLShader::CompileShader(unsigned int type, const std::string& source)
{
	unsigned int id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	int success;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)alloca(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);

		std::string info = (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
		SUNTA_ENGINE_LOG_ERROR("ERROR: COULDN'T COMPILE {} SHADER {}", info, message);

		glDeleteShader(id);
		return 0;
	}

	return id;
}

unsigned int OpenGLShader::CreateProgram(const std::string& vertexShader, const std::string& fragmentShader)
{
	unsigned int program = glCreateProgram();
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);

	int success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		int length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)alloca(length * sizeof(char));
		glGetProgramInfoLog(program, length, &length, message);
		SUNTA_ENGINE_LOG_ERROR("ERROR: LINKING SHADERS TO PROGRAM FAILED: {}", message);
		return 0;
	}

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

void OpenGLShader::Bind() const
{
	GLCall(glUseProgram(id));
}

void OpenGLShader::Unbind() const
{
	GLCall(glUseProgram(0));
}

bool OpenGLShader::HasUniform(const std::string& name) const
{
	GLCall(int location = glGetUniformLocation(id, name.c_str()));
	return location != -1;
}

void OpenGLShader::SetUniformBool(const std::string& name, bool value) const
{
	SetUniform1i(name, value ? 1 : 0);
}

void OpenGLShader::SetUniform1i(const std::string& name, int value) const
{
	GLCall(glUniform1i(GetUniformLocation(name), value));
}

void OpenGLShader::SetUniform1f(const std::string& name, float value) const
{
	GLCall(glUniform1f(GetUniformLocation(name), value));
}

void OpenGLShader::SetUniform3f(const std::string& name, float v0, float v1, float v2) const
{
	GLCall(glUniform3f(GetUniformLocation(name), v0, v1, v2));
}

void OpenGLShader::SetUniform3f(const std::string& name, glm::vec3 vec) const
{
	GLCall(glUniform3f(GetUniformLocation(name), vec.x, vec.y, vec.z));
}

void OpenGLShader::SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3) const
{
	GLCall(glUniform4f(GetUniformLocation(name), v0, v1, v2, v3));
}

void OpenGLShader::SetUniformMatrix4fv(const std::string& name, const glm::mat4& matrix) const
{
	GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]));
}

void OpenGLShader::TrySetUniformBool(const std::string& name, bool value) const
{
	if (HasUniform(name))
		SetUniformBool(name, value);
}

void OpenGLShader::TrySetUniform1i(const std::string& name, int value) const
{
	if (HasUniform(name))
		SetUniform1i(name, value);
}

void OpenGLShader::TrySetUniform1f(const std::string& name, float value) const
{
	if (HasUniform(name))
		SetUniform1f(name, value);
}

void OpenGLShader::TrySetUniform3f(const std::string& name, float v0, float v1, float v2) const
{
	if (HasUniform(name))
		SetUniform3f(name, v0, v1, v2);
}

void OpenGLShader::TrySetUniform3f(const std::string& name, glm::vec3 vec) const
{
	if (HasUniform(name))
		SetUniform3f(name, vec);
}

void OpenGLShader::TrySetUniform4f(const std::string& name, float f0, float f1, float f2, float f3) const
{
	if (HasUniform(name))
		SetUniform4f(name, f0, f1, f2, f3);
}

void OpenGLShader::TrySetUniformMatrix4fv(const std::string& name, const glm::mat4& matrix) const
{
	if (HasUniform(name))
		SetUniformMatrix4fv(name, matrix);
}

void OpenGLShader::TrySetBoneMatrices(const std::vector<glm::mat4>& matrices)
{
	if (HasUniform("bones[0]") && !matrices.empty())
	{
		int location = GetUniformLocation("bones[0]");
		unsigned int count = std::min((unsigned int)matrices.size(), 200u);
		GLCall(glUniformMatrix4fv(location, count, GL_FALSE, glm::value_ptr(matrices[0])));
	}
}

int OpenGLShader::GetUniformLocation(const std::string& name) const
{
	if (uniformLocationCache.find(name) != uniformLocationCache.end())
		return uniformLocationCache[name];

	GLCall(int location = glGetUniformLocation(id, name.c_str()));
	if (location == -1)
		SUNTA_ENGINE_LOG_WARNING("WARNING: UNIFORM: {} DOESN'T EXIST", name);

	uniformLocationCache[name] = location;
	return location;
}
}
