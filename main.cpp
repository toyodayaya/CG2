#pragma warning(push)
// C4023の警告を見なかったことにする
#pragma warning(disable:4023)
#include <Windows.h>
#pragma warning(pop)

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	// 出力ウインドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");

	return 0;
}