#pragma once
#include <memory>

namespace Sunta
{

class Window;
class EditorGUIBackend;

class EditorGUIContext
{
public:
	//DESTRUCTOR required for unique_ptr to work
	~EditorGUIContext();

	static void Init(Window* window);
	static void Shutdown(Window* window);

	static void NewFrame(Window* window);
	static void EndFrame(Window* window);

private:
	static std::unique_ptr<EditorGUIBackend> backend;
};

}