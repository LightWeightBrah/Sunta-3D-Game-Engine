#pragma once
#include "Core/Platform.h"

namespace Sunta
{

class MacPlatform : public Platform
{
public:
	virtual bool OpenInExplorer(const std::string& path) override;
};

}