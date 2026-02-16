#include <Sunta.h>

class Game : public Sunta::Engine
{
public:
	Game()
	{

	}

	~Game()
	{

	}
};

int main()
{
	Game* game = new Game();
	game->Run();
	delete game;
}