#pragma once
#include "WinAPIManager.h"
#include "DirectXBasis.h"
#include "SrvManager.h"
#ifdef USE_IMGUI
#include <externals/imgui/imgui_impl_dx12.h>
#include <externals/imgui/imgui_impl_win32.h>
#include <externals/imgui/imgui_impl_dx12.h>
#endif // USE_IMGUI


class ImguiManager
{
public:
	// 初期化
	void Initialize(WinAPIManager* winAPIManager, DirectXBasis* dxBasis, SrvManager* srvManager);

	// 終了
	void Finalize();

	// ImGui受け付け開始
	void Begin();

	// ImGui受け付け終了
	void End();

	// 描画
	void Draw();

private:
	// ポインタ
	WinAPIManager* winAPIManager = nullptr;
	DirectXBasis* dxBasis = nullptr;
	SrvManager* srvManager = nullptr;

#ifdef USE_IMGUI


	// Dx12用の初期化情報
	ImGui_ImplDX12_InitInfo initInfo{};

#endif // USE_IMGUI

};

