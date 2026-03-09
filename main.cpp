#include <Windows.h>

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	// 出力ウインドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");

	return 0;
}