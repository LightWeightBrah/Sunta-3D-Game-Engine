#pragma once

namespace Sunta
{

// Factory of Graphics Context once per Window

class GraphicsContext
{
public:
	virtual ~GraphicsContext() = default;

	virtual void Init() = 0;
	virtual void SwapBuffers() = 0;
	virtual void MakeContextCurrent() = 0;

	static void Configure();
	static std::unique_ptr<GraphicsContext> Create(void* window);
};

}