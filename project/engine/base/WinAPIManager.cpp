#include "WinAPIManager.h"
#pragma comment(lib,"winmm.lib")
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // USE_IMGUI

void WinAPIManager::Initialize()
{
	// COMの初期化
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	// ウインドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	// ウインドウクラス名
	wc.lpszClassName = L"CG2WindowClass";
	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	// ウインドウクラスを登録する
	RegisterClass(&wc);


	// ウインドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0, 0, kClientWidth, kClientHeight };

	// クライアント領域を元に実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウインドウの生成
	hwnd = CreateWindow(
		wc.lpszClassName, // 利用するクラス名
		L"CG2", // タイトルバーの文字
		WS_OVERLAPPEDWINDOW, // ウインドウスタイル
		CW_USEDEFAULT, // 表示X座標（OSに任せる）
		CW_USEDEFAULT, // 表示Y座標（OSに任せる）
		wrc.right - wrc.left, // ウインドウ横幅
		wrc.bottom - wrc.top, // ウインドウ縦幅
		nullptr, // ウインドウハンドル
		nullptr, // メニューハンドル
		wc.hInstance, // インスタンスハンドル
		nullptr // オプション
	);



	// ウインドウを表示する
	ShowWindow(hwnd, SW_SHOW);

	// 出力ウインドウへの文字出力
	OutputDebugStringA("Hello,DirectX!\n");

	// システムタイマーの分解能を上げる
	timeBeginPeriod(1);
}

void WinAPIManager::Finalize()
{
	CloseWindow(hwnd);
	// COMの終了処理
	CoUninitialize();
}

bool WinAPIManager::ProcessMessage()
{
	// メインループ
	MSG msg{};

	// ウインドウにメッセージが来ていたら最優先で処理を行う
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	if (msg.message == WM_QUIT)
	{
		return true;
	}

	return false;
}

// ウインドウプロシージャ
LRESULT CALLBACK WinAPIManager::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
#ifdef USE_IMGUI
	// ImGuiの操作受け付け
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true;
	}
#endif // USE_IMGUI

	// メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		// ウインドウが破棄された
	case WM_DESTROY:
		// OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
