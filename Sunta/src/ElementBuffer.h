#pragma once

class ElementBuffer
{
private:
	unsigned int id;
	unsigned int count;
public:
	ElementBuffer(const unsigned int* data, unsigned int size);
	~ElementBuffer();

	void Bind() const;
	void Unbind() const;
	inline unsigned int GetCount() const { return count; }

};