#pragma once
#include <filesystem>

namespace Sunta
{

class RendererDevice;

class ResourceLoader
{
public:
	static void Init();

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
};

}