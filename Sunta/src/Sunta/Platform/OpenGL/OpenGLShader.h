#pragma once
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer/Shader.h"

namespace Sunta
{

class OpenGLShader : public Shader
{

public:
	OpenGLShader(const std::string& filepath);
	~OpenGLShader();

	virtual void Bind() const override;
	virtual void Unbind() const override;

	virtual bool HasUniform(const std::string& name) const override;

	virtual void SetUniform1i(const std::string& name, int value) const override;
	virtual void SetUniform1f(const std::string& name, float value) const override;
	virtual void SetUniform3f(const std::string& name, float v0, float v1, float v2) const override;
	virtual void SetUniform3f(const std::string& name, glm::vec3 vec) const override;
	virtual void SetUniform4f(const std::string& name, float f0, float f1, float f2, float f3) const override;
	virtual void SetUniformMatrix4fv(const std::string& name, const glm::mat4& matrix) const override;

	virtual void TrySetUniform1i(const std::string& name, int value) const override;
	virtual void TrySetUniform1f(const std::string& name, float value) const override;
	virtual void TrySetUniform3f(const std::string& name, float v0, float v1, float v2) const override;
	virtual void TrySetUniform3f(const std::string& name, glm::vec3 vec) const override;
	virtual void TrySetUniform4f(const std::string& name, float f0, float f1, float f2, float f3) const override;
	virtual void TrySetUniformMatrix4fv(const std::string& name, const glm::mat4& matrix) const override;
	
	virtual void TrySetBoneMatrices(const std::vector<glm::mat4>& matrices) override;

private:
	unsigned int id;
	mutable std::unordered_map<std::string, int> uniformLocationCache;

	unsigned int CompileShader(unsigned int type, const std::string& source);
	unsigned int CreateProgram(const std::string& vertexShader, const std::string& fragmentShader);
	int GetUniformLocation(const std::string& name) const;
};
}