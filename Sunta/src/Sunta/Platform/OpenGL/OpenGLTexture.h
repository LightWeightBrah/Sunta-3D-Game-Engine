#pragma once
#include "Renderer/Texture.h"

namespace Sunta
{

class OpenGLTexture : public Texture
{

public:
	OpenGLTexture(const std::string& filepath, bool flip = true, bool isPixelArt = false);

	// From pixels in memory: 4 bytes per pixel (RGBA), the first row is the top. Smooth when scaled, no repeating at the edges
	OpenGLTexture(int width, int height, const unsigned char* rgbaPixels);
	virtual ~OpenGLTexture() override;
	
	virtual unsigned int GetID() const override { return id; }
	
	virtual void Bind(unsigned int slot = 0) const override;
	virtual void Unbind() const override;

	virtual int GetWidth() const override { return width; }
	virtual int GetHeight() const override { return height; }

private:
	unsigned int id;
	unsigned char* data;
	int width, height, nrChannels;

};

}