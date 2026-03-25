#pragma once
#include "Renderer/Texture.h"

namespace Sunta
{

class OpenGLTexture : public Texture
{

public:
	OpenGLTexture(const std::string& filepath);
	virtual ~OpenGLTexture() override;
	
	virtual void Bind(unsigned int slot = 0) const override;
	virtual void Unbind() const override;

private:
	unsigned int id;
	unsigned char* data;
	int width, height, nrChannels;

};

}