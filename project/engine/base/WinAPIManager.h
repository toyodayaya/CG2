#pragma once
#include <Windows.h>
#include <cstdint>

class WinAPIManager
{
public:
	// 静的メンバ変数
	// ウインドウプロシージャ
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	// 定数
	// クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;


public:
	// 初期化
	void Initialize();

	// 終了
	void Finalize();

	// メッセージ処理
	bool ProcessMessage();

	// getter
	HWND GetHwnd() const { return hwnd; }
	HINSTANCE GetHInstance() const { return wc.hInstance; }

	
private:
	// ウインドウハンドル
	HWND hwnd = nullptr;
	// ウインドウクラスの設定
	WNDCLASS wc{};
};

