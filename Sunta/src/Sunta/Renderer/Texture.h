#pragma once

namespace Sunta
{

class Texture
{

public:
	Texture(const std::string& filepath) : filepath(filepath) { }
	virtual ~Texture() = default;

	virtual void Bind(unsigned int slot = 0) const = 0;
	virtual void Unbind() const = 0;

protected:
	std::string filepath;

};

}