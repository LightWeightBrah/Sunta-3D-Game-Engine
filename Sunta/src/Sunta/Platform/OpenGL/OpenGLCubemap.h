#pragma once

#include "Renderer/Cubemap.h"

namespace Sunta
{

class OpenGLCubemap : public Cubemap
{
public:
	// Reads <folder>/right.jpg (or .png), left, top, bottom, front and back
	explicit OpenGLCubemap(const std::string& folder);
	virtual ~OpenGLCubemap() override;

	virtual void Bind(unsigned int slot = 0) const override;
	virtual bool IsLoaded() const override { return isLoaded; }

private:
	unsigned int id = 0;
	bool isLoaded = false;
};

}
