#include "Core/SuntaPreCompiled.h"

#include <glad/glad.h>

#include "OpenGLTexture.h"

#include "stb/stb_image.h"
#include "Core/Log.h"
#include "OpenGLUtilities.h"

namespace Sunta
{
OpenGLTexture::OpenGLTexture(const std::string& filepath) 
	: Texture(filepath)
	, id(0)
	, data(nullptr)
	, width(0)
	, height(0)
	, nrChannels(0)
{
	SUNTA_ENGINE_LOG_INFO("Loading texture: {}", filepath);
	stbi_set_flip_vertically_on_load(true);
	data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 0);
	if (!data)
	{
		SUNTA_ENGINE_LOG_ERROR("ERROR: COULDN'T LOAD TEXTURE: {}", filepath);
		return;
	}
	unsigned int openGLFormat = GL_RGB8;
	unsigned int imageFormat  = GL_RGB;
	if (nrChannels == 4)
	{
		openGLFormat = GL_RGBA8;
		imageFormat  = GL_RGBA;
	}

	GLCall(glGenTextures(1, &id));
	GLCall(glBindTexture(GL_TEXTURE_2D, id));

	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, openGLFormat, width, height, 0, imageFormat, GL_UNSIGNED_BYTE, data));
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));
	stbi_image_free(data);
}

OpenGLTexture::~OpenGLTexture()
{
	GLCall(glDeleteTextures(1, &id));
}

void OpenGLTexture::Bind(unsigned int slot) const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_2D, id));
}

void OpenGLTexture::Unbind() const
{
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

}
