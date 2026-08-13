#pragma once
#include "Core/Platform.h"

namespace Sunta
{

class WindowsPlatform : public Platform
{
public:
	virtual bool OpenInExplorer(const std::string& path) override;
	virtual bool OpenFileExternally(const std::string& path) override;
};

}