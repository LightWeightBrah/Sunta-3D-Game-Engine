#pragma once

#include <string>
#include <memory>
#include <vector>

namespace Sunta
{

class Platform
{
public:
	virtual ~Platform() = default;

	static void Init();
	static void Shutdown();

	static Platform& Get() { return *instance; }

	virtual bool OpenInExplorer(const std::string& path) = 0;
	virtual bool OpenFileExternally(const std::string& path) = 0;
	std::string OpenFileDialog(const std::string& filterName = "Supported Files", const std::vector<std::string>&extensions = {});
	std::string SaveFileDialog(const std::string& filterName = "Supported Files", const std::vector<std::string>&extensions = {});
	
private:
	static std::unique_ptr<Platform> Create();
	inline static std::unique_ptr<Platform> instance = nullptr;
};

}