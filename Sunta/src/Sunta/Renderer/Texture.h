#pragma once

namespace Sunta
{

class Texture
{

public:
	Texture(const std::string& filepath, bool flip = true) : filepath(filepath) { }
	virtual ~Texture() = default;

	virtual unsigned int GetID() const = 0;

	virtual void Bind(unsigned int slot = 0) const = 0;
	virtual void Unbind() const = 0;

	virtual int GetWidth() const = 0;
	virtual int GetHeight() const = 0;

protected:
	std::string filepath;

};

}