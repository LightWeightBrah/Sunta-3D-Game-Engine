#pragma once

namespace Sunta
{

class Texture
{

public:
	Texture(const std::string& filepath, bool flip = true, bool isPixelArt = false) : filepath(filepath) { }
	virtual ~Texture() = default;

	virtual unsigned int GetID() const = 0;

	virtual void Bind(unsigned int slot = 0) const = 0;
	virtual void Unbind() const = 0;

	virtual int GetWidth() const = 0;
	virtual int GetHeight() const = 0;

	inline const std::string& GetName() const { return name; }
	inline void SetName(const std::string& name) { this->name = name; }
	inline const std::string& GetFilePath() const { return filepath; }

protected:
	std::string filepath;
	std::string name = "";

};

}