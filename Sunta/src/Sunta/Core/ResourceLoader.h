#pragma once

namespace Sunta
{

class RendererDevice;

class ResourceLoader
{
public:
	static void Init(RendererDevice& rendererDevice);

private:
	static void LoadShaders(RendererDevice& rendererDevice);
	static void LoadMaterials(RendererDevice& rendererDevice);
	static void LoadMeshes(RendererDevice& rendererDevice);
	static void LoadIcons(RendererDevice& rendererDevice);
	static void LoadUIAssets(RendererDevice& rendererDevice);
};

}