#include "Renderer/GraphicsContext.h"

typedef struct GLFWwindow GLFWwindow;

namespace Sunta
{

class OpenGLGraphicsContext : public GraphicsContext
{
public:
	OpenGLGraphicsContext(GLFWwindow* window);

	virtual void Init() override;
	virtual void SwapBuffers() override;
	virtual void MakeContextCurrent() override;

	static void Configure();

private:
	GLFWwindow* window;
};

}