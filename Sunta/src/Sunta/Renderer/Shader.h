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

enum class ShaderFeature
{
	// Bitmasks allows us to store multiple options (bools / ON or OFF) flags
	// Bitmasking treats bits as binary switches packed into a single INT.
	// Use | (OR) to turn switches ON (set flags), 
	// Use & (AND) to check if a specific switch is currently ON

	None	 = 0,      // 
	Lighting = 1 << 0, // 1 (0001)
	Skinning = 1 << 1, // 2 (0010)
	Shadows  = 1 << 2  // 4 (0100)
};

class Shader
{
public:
	Shader(const std::string& filepath) : filepath(filepath), features(0) {}
	virtual ~Shader() = default;

	virtual void Bind() const = 0;
	virtual void Unbind() const = 0;

	inline const std::string& GetName() const { return name; }
	inline void SetName(const std::string& name) { this->name = name; }
	inline const std::string& GetFilePath() const { return filepath; }

	void AddFeature(ShaderFeature feature) { features |= (unsigned int)feature; }
	bool HasFeature(ShaderFeature feature) const { return (features & (unsigned int)feature) != 0; }

	virtual bool HasUniform(const std::string& name) const = 0;

	virtual void SetUniformBool(const std::string& name, bool value) const = 0;
	virtual void SetUniform1i(const std::string& name, int value) const = 0;
	virtual void SetUniform1f(const std::string& name, float value) const = 0;
	virtual void SetUniform3f(const std::string& name, float v0, float v1, float v2) const = 0;
	virtual void SetUniform3f(const std::string& name, glm::vec3 vec) const = 0;
	virtual void SetUniform4f(const std::string& name, float f0, float f1, float f2, float f3) const = 0;
	virtual void SetUniformMatrix4fv(const std::string& name, const glm::mat4& matrix) const = 0;

	virtual void TrySetUniformBool(const std::string& name, bool value) const = 0;
	virtual void TrySetUniform1i(const std::string& name, int value) const = 0;
	virtual void TrySetUniform1f(const std::string& name, float value) const = 0;
	virtual void TrySetUniform3f(const std::string& name, float v0, float v1, float v2) const = 0;
	virtual void TrySetUniform3f(const std::string& name, glm::vec3 vec) const = 0;
	virtual void TrySetUniform4f(const std::string& name, float f0, float f1, float f2, float f3) const = 0;
	virtual void TrySetUniformMatrix4fv(const std::string& name, const glm::mat4& matrix) const = 0;

	virtual void TrySetBoneMatrices(const std::vector<glm::mat4>& matrices) = 0;

protected:
	std::string filepath;
	std::string name = "";
	unsigned int features;

	ShaderProgramSource ParseShader();
};

}