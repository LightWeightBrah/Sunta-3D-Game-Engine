#include <string>
#include <iostream>
#include <GL/glew.h>

#include "Texture.h"
#include "Renderer.h"
#include "stb/stb_image.h"

Texture::Texture(const std::string& filepath): 
	id(0), filepath(filepath), data(nullptr), 
	width(0), height(0), nrChannels(0)
{
	std::cout << "Ladowanie tekstury: " << filepath << std::endl;
	stbi_set_flip_vertically_on_load(true);
	data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 0);
	if(!data)
	{
		std::cout << "ERROR: COULDN'T LOAD TEXTURE: " << filepath << std::endl;
		return;
	}
	unsigned int format = GL_RGB;
	if (nrChannels == 4)
		format = GL_RGBA;

	GLCall(glGenTextures(1, &id));
	GLCall(glBindTexture(GL_TEXTURE_2D, id));

	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data));
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));
	stbi_image_free(data);
}

Texture::~Texture()
{
	GLCall(glDeleteTextures(1, &id));
}

void Texture::Bind(unsigned int slot) const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_2D, id));
}

void Texture::Unbind() const
{
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

