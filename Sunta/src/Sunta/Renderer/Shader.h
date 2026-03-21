#pragma once
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Sunta
{

struct ShaderProgramSource
{
	std::string vertexSource;
	std::string fragmentSource;
};

class Shader
{
public:
	Shader(const std::string& filepath) : filepath(filepath) {}
	virtual ~Shader() = default;

	virtual void Bind() const = 0;
	virtual void Unbind() const = 0;

	virtual void SetUniform1i(const std::string& name, int value) const = 0;
	virtual void SetUniform1f(const std::string& name, float value) const = 0;
	virtual void SetUniform3f(const std::string& name, float v0, float v1, float v2) const = 0;
	virtual void SetUniform3f(const std::string& name, glm::vec3 vec) const = 0;
	virtual void SetUniform4f(const std::string& name, float f0, float f1, float f2, float f3) const = 0;
	virtual void SetUniformMatrix4fv(const std::string& name, const glm::mat4& matrix) const = 0;
	virtual void SetBoneMatrices(const std::vector<glm::mat4>& matrices) = 0;

protected:
	std::string filepath;

	ShaderProgramSource ParseShader();
};

}