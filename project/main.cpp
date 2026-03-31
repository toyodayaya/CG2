#include "Game.h"
#include "Framework.h"

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	Framework* game = new Game();

	game->Run();

	delete game;

	return 0;
}


