#pragma once
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct ShaderProgramSource
{
	std::string vertexSource;
	std::string fragmentSource;
};

class Shader
{
private:
	std::string filepath;
	unsigned int id;
	mutable std::unordered_map<std::string, int> uniformLocationCache;
public:
	Shader(const std::string& filepath);
	~Shader();

	void Bind() const;
	void Unbind() const;

	void SetUniform1i(const std::string& name, int value) const;
	void SetUniform1f(const std::string& name, float value) const;
	void SetUniform3f(const std::string& name, float v0, float v1, float v2) const;
	void SetUniform3f(const std::string& name, glm::vec3 vec) const;
	void SetUniform4f(const std::string& name, float f0, float f1, float f2, float f3) const;
	void SetUniformMatrix4fv(const std::string& name, const glm::mat4& matrix) const;
	void SetBoneMatrices(const std::vector<glm::mat4>& matrices);
private:
	bool CompileShader();
	ShaderProgramSource ParseShader(const std::string& filepath);
	unsigned int CompileShader(unsigned int type, const std::string& source);
	unsigned int CreateProgram(const std::string& vertexShader, const std::string& fragmentShader);
	int GetUniformLocation(const std::string& name) const;
};