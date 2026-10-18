#pragma once
#include <filesystem>

namespace Sunta
{

class RendererDevice;

class ResourceLoader
{
public:
	// Everything the game needs: shaders, textures, materials, meshes and models
	static void Init();

	// Icons and backgrounds used only by the editor UI, so the game build doesn't need them
	static void InitEditorAssets();

private:
	static void LoadShaders();
	static void LoadTextures();
	static void LoadMaterials();
	static void LoadMeshes();
	static void LoadTexturesNamingConvention();
	static void LoadModels();
	static void LoadIcons();
	static void LoadUIAssets();

	static void LoadMaterialsFromDirectory(const std::filesystem::path& directoryPath);
	static void LoadModelsFromDirectory(const std::filesystem::path& directoryPath);
};

}