#include "Core/SuntaPreCompiled.h"
#include "OpenGLCubemap.h"

#include <filesystem>
#include <glad/glad.h>

#include "stb/stb_image.h"
#include "Core/Log.h"
#include "OpenGLUtilities.h"

namespace Sunta
{

namespace
{

// The face's picture with either extension
std::string FindFaceFile(const std::string& folder, const char* faceName)
{
	for (const char* extension : { ".jpg", ".png" })
	{
		const std::string path = folder + "/" + faceName + extension;
		if (std::filesystem::exists(path))
			return path;
	}

	return "";
}

}

OpenGLCubemap::OpenGLCubemap(const std::string& folder)
{
	GLCall(glGenTextures(1, &id));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, id));

	// A cubemap's pictures start at their top row (not flipped like the textures of models)
	stbi_set_flip_vertically_on_load(false);

	isLoaded = true;
	for (unsigned int face = 0; face < 6; face++)
	{
		const std::string path = FindFaceFile(folder, FaceNames[face]);

		int width = 0, height = 0, channels = 0;
		unsigned char* pixels = path.empty() ? nullptr : stbi_load(path.c_str(), &width, &height, &channels, 3);
		if (!pixels)
		{
			SUNTA_ENGINE_LOG_ERROR("Cubemap: couldn't read the '{0}' face (.jpg or .png) in '{1}'", FaceNames[face], folder);
			isLoaded = false;
			continue;
		}

		GLCall(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels));
		stbi_image_free(pixels);
	}

	// Smooth, and no seams where two faces meet
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
}

OpenGLCubemap::~OpenGLCubemap()
{
	GLCall(glDeleteTextures(1, &id));
}

void OpenGLCubemap::Bind(unsigned int slot) const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, id));
}

}
