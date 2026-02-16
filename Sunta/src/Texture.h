#pragma once

class Texture
{
private:
	unsigned int id;
	std::string filepath;
	unsigned char* data;
	int width, height, nrChannels;
public:
	Texture(const std::string& filepath);
	~Texture();
	void Bind(unsigned int slot = 0) const;
	void Unbind() const;
	inline std::string GetFilepath() const { return filepath; }
};