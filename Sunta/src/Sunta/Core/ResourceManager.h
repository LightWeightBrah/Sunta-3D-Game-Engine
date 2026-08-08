#pragma once

#include <map>
#include <string>
#include <iostream>
#include <functional>
#include <vector>

namespace Sunta
{

class RendererDevice;
class Animation;
class Model;
class Texture;
class Shader;
class Material;
class Mesh;

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
	static void LoadMesh		(const std::string& name, std::function<std::shared_ptr<Mesh>()> primitiveFactory);
	static void LoadMaterial    (const std::string& name, std::shared_ptr<Material> material);
	static void LoadTexture     (const std::string& name, const std::string& filepath);
	static void LoadShader      (const std::string& name, const std::string& filepath);
		
	static void RenameMaterial  (const std::string& oldName, const std::string& newName);

	static std::shared_ptr<Texture>	  GetEditorIcon      (const std::string& name);
	static std::shared_ptr<ModelData> GetModelData       (const std::string& name);
	static std::shared_ptr<Mesh>	  GetMeshData		 (const std::string& name);
	static std::shared_ptr<Texture>	  GetTextureData     (const std::string& name);
	static std::shared_ptr<Shader>	  GetShaderData      (const std::string& name);
	static std::shared_ptr<Material>  GetMaterialData	 (const std::string& name);
										    
	static std::shared_ptr<Material>  LoadOrGetMaterial (const std::string& name, std::shared_ptr<Shader> shader);
	static std::shared_ptr<Texture>   LoadOrGetTexture  (const std::string& filepath);

	static std::shared_ptr<Material>  LoadMaterialFromFile(const std::string& filepath);


	static std::vector<std::string>	  GetMeshesNames();
	static std::vector<std::string>	  GetMaterialsNames();

private:
	inline static RendererDevice* rendererDevice;

	inline static std::map<std::string, std::shared_ptr<Texture>>    editorIconsRegistered;
	inline static std::map<std::string, std::shared_ptr<ModelData>>  modelsRegistered;
	inline static std::map<std::string, std::shared_ptr<Mesh>>		 meshesRegistered;
	inline static std::map<std::string, std::shared_ptr<Texture>>	 texturesRegistered;
	inline static std::map<std::string, std::shared_ptr<Shader>>	 shadersRegistered;
	inline static std::map<std::string, std::shared_ptr<Material>>	 materialsRegistered;
};

}