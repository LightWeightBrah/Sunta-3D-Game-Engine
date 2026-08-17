#include <SuntaEngine.h>

class Game : public Sunta::Application
{
public:
	Game()
	{
		/*using namespace Sunta::EngineAssets;

		auto texturedMaterial = ResourceManager::GetMaterialData(Materials::Textured);

		EntityFactory::CreateCube(*this, glm::vec3(7.5f, 5.0f, 3.0f), "Cube");
		EntityFactory::CreateCube(*this, glm::vec3(0.0f, 5.0f, -0.5), "Textured Cube", texturedMaterial);
		EntityFactory::CreateDirectionalLight(*this, glm::vec3(3.0f, 6.0f, 2.0f), "Directional Light");
		EntityFactory::CreatePointLight(*this, glm::vec3(-4.0f, 2.0f, 0.0f), "Point Light");
		EntityFactory::CreateSpotLight(*this, glm::vec3(-2.5f, 4.5f, 0.0f), "Spot Light");*/
	}

	~Game()
	{

	}
};

SUNTA_NEW_APPLICATION(Game)