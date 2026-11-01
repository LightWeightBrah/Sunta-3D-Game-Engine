#pragma once

#include <string>

namespace Sunta
{

// Six square pictures around a point, read by a direction instead of a position (see learnopengl.com: Cubemaps)
// Used for the sky: whichever way the camera looks, the shader reads the picture of the sky in that direction
//
// Loaded from a folder with right, left, top, bottom, front and back (.jpg or .png): the +X, -X, +Y, -Y, +Z, -Z faces
class Cubemap
{
public:
	virtual ~Cubemap() = default;

	virtual void Bind(unsigned int slot = 0) const = 0;

	// False when a face could not be read (the folder or a file is missing)
	virtual bool IsLoaded() const = 0;

	// The names of the faces in a folder, in OpenGL's order (+X, -X, +Y, -Y, +Z, -Z)
	static constexpr const char* FaceNames[6] = { "right", "left", "top", "bottom", "front", "back" };
};

}
