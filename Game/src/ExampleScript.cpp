#include <SuntaEngine.h>

using namespace Sunta;

class ExampleScript : public ScriptableEntity
{
protected:
	void OnUpdate(float deltaTime) override
	{
		if (auto* transform = GetComponent<TransformComponent>())
		{
			// DO STUFF
		}
	}
};