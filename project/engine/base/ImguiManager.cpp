#include "ImguiManager.h"
#include <externals/imgui/imgui_impl_win32.h>
#include <externals/imgui/imgui_impl_dx12.h>
#include "SrvManager.h"

void ImguiManager::Initialize([[maybe_unused]] WinAPIManager* winAPIManager, [[maybe_unused]] DirectXBasis* dxBasis, [[maybe_unused]] SrvManager* srvManager)
{
#ifdef USE_IMGUI



	// 引数で受け取ってメンバ変数として記録する
	this->winAPIManager = winAPIManager;
	this->dxBasis = dxBasis;
	this->srvManager = srvManager;

	// ImGuiのコンテキストを作成する
	ImGui::CreateContext();
	// ImGuiのスタイルを設定する
	ImGui::StyleColorsDark();

	// win32用の初期化関数を呼び出す
	ImGui_ImplWin32_Init(winAPIManager->GetHwnd());

	// DirectX12用の初期化情報
	initInfo.Device = dxBasis->GetDevice();
	initInfo.CommandQueue = dxBasis->GetCommandQueue();
	initInfo.NumFramesInFlight = static_cast<int>(dxBasis->GetSwapChainResourceNum());
	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	initInfo.SrvDescriptorHeap = srvManager->GetDescriptorHeap();

	// SRV確保用関数の設定
	initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle,
		D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
		{
			SrvManager* srvManager = SrvManager::GetInstance();
			uint32_t srvIndex = srvManager->Allocate();
			*out_cpu_desc_handle = srvManager->GetCPUDescriptorHandle(srvIndex);
			*out_gpu_desc_handle = srvManager->GetGPUDescriptorHandle(srvIndex);
		};

	// UserDataにsrvManagerをセット
	initInfo.UserData = srvManager;

	// SRV解放用関数の設定
	initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle,
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle)
		{
			// 何もしない
		};

	// DirectX12用の初期化関数を呼び出す
	ImGui_ImplDX12_Init(&initInfo);
#endif // USE_IMGUI
}

void ImguiManager::Finalize()
{
#ifdef USE_IMGUI

	// DirectX12用の終了関数を呼び出す
	ImGui_ImplDX12_Shutdown();
	// win32用の終了関数を呼び出す
	ImGui_ImplWin32_Shutdown();
	// ImGuiのコンテキストを破棄する
	ImGui::DestroyContext();

#endif // USE_IMGUI

}

void ImguiManager::Begin()
{
#ifdef USE_IMGUI


	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

#endif // USE_IMGUI

}

void ImguiManager::End()
{
#ifdef USE_IMGUI

	ImGui::Render();

#endif // USE_IMGUI

}

void ImguiManager::Draw()
{
#ifdef USE_IMGUI


	// 描画コマンドリストを取得する
	ID3D12GraphicsCommandList* commandList = dxBasis->GetCommandList();
	
	// ディスクリプタヒープの配列をセットする
	ID3D12DescriptorHeap* descriptorHeaps[] = { srvManager->GetDescriptorHeap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	// 描画コマンドを発行する
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

#endif // USE_IMGUI

}