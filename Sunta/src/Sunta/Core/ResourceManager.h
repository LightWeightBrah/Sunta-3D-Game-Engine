#pragma once

#include <map>
#include <string>
#include <iostream>

namespace Sunta
{

class RendererDevice;
class Animation;
class Model;
class Texture;
class Shader;
class Material;

enum class AnimationType
{
	IDLE,
	GESTURE,
	RUNNING
};
	
struct ModelData
{
	std::shared_ptr<Model>             model;
	std::map<AnimationType, Animation> animations;
};
	
class ResourceManager
{
public:
	static void Init(RendererDevice& device) { rendererDevice = &device; }

	static void LoadEditorIcon  (const std::string& name, const std::string& filepath);
	static void LoadModel       (const std::string& name, const std::string& filepath);
	static void LoadTexture     (const std::string& name, const std::string& filepath);
	static void LoadShader      (const std::string& name, const std::string& filepath);
		
	static std::shared_ptr<Texture>	  GetEditorIcon      (const std::string& name);
	static std::shared_ptr<ModelData> GetModelData       (const std::string& name);
	static std::shared_ptr<Texture>	  GetTextureData     (const std::string& name);
	static std::shared_ptr<Shader>	  GetShaderData      (const std::string& name);
														    
	static std::shared_ptr<Material>  LoadOrGetModelMaterial (const std::string& name, std::shared_ptr<Shader> shader);
	static std::shared_ptr<Texture>   LoadOrGetModelTexture  (const std::string& filepath);

private:
	inline static RendererDevice* rendererDevice;

	inline static std::map<std::string, std::shared_ptr<Texture>>    editorIconsRegistered;
	inline static std::map<std::string, std::shared_ptr<ModelData>>  modelsRegistered;
	inline static std::map<std::string, std::shared_ptr<Texture>>	 texturesRegistered;
	inline static std::map<std::string, std::shared_ptr<Shader>>	 shadersRegistered;
	inline static std::map<std::string, std::shared_ptr<Material>>	 materialsRegistered;
};

}