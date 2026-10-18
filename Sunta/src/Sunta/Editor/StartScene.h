#pragma once
#include <string>

namespace Sunta
{

class Scene;
struct GameConfig;

// What the editor shows right after a project opens
// The scene named by "start_scene" in game.config, or, when the project has none yet, a small default scene
// The default scene is saved into the project and becomes its start scene, so a new project can be played and built at once
class StartScene
{
public:
	static void OpenInEditor(Scene& scene);

private:
	static void LoadFromFile(const std::string& scenePath, Scene& scene);
	static void CreateDefaultScene(Scene& scene);
	static void SaveAsStartScene(const std::string& virtualPath, Scene& scene, GameConfig& gameConfig);
};

}
