#pragma once

#include <string>
#include <memory>

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
	
private:
	static std::unique_ptr<Platform> Create();
	inline static std::unique_ptr<Platform> instance = nullptr;
};

}