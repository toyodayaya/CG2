#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "WinAPIManager.h"
#include <array>

class DirectXBasis
{
private:
	// DescriptorHandleを取得する関数
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeap,
		uint32_t descriptorSize, uint32_t index);
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeap,
		uint32_t descriptorSize, uint32_t index);

public:
	// 初期化
	void Initialize(WinAPIManager* winApiManager);

	// デバイスの初期化
	void DeviceInitialize();

	// コマンド関連の初期化
	void CommandInitialize();

	// スワップチェーンの生成
	void SwapChainGenerate();

	// 深度バッファの生成
	void DepthStencilGenerate();

	// 各種ディスクリプタヒープの生成
	void DescriptorHeapGenerate();

	// ディスクリプタヒープを生成
	// DescriptorHeapの作成関数
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	// レンダーターゲットビューの初期化
	void RenderTargetViewInitialize();

	// SRV専用のディスクリプタハンドル取得関数
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

	// 深度ステンシルビューの初期化
	void dsvInitialize();

	// フェンスの初期化
	void FenceInitialize();

	// ビューポート矩形の初期化
	void ViewportInitialize();

	// シザー矩形の初期化
	void ScissorRectInitialize();

	// DXCコンパイラの生成
	void DXCCompilerGenerate();

	// ImGuiの初期化
	void ImGuiInitialize();

	// 描画前処理
	void PreDraw();

	// 描画後処理
	void PostDraw();

private:
	HRESULT hr;
	// DXGIファクトリー
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	// D3D12Device
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	// コマンドアロケータ
	Microsoft::WRL::ComPtr <ID3D12CommandAllocator> commandAllocator;
	// コマンドリスト
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> commandList;
	// コマンドキュー
	Microsoft::WRL::ComPtr <ID3D12CommandQueue> commandQueue;
	// スワップチェーン
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	// スワップチェーンリソース
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;
	// RTVを二つ作るのでディスクリプタを二つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	// ディスクリプタサイズ
	uint32_t descriptorSizeSRV;
	uint32_t descriptorSizeRTV;
	uint32_t descriptorSizeDSV;
	// RTV用のディスクリプタヒープを作成
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> rtvDescriptorHeap;
	// SRV用のディスクリプタヒープを作成
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> srvDescriptHeap;
	// DSV用のディスクリプタヒープを作成
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> dsvDescriptorHeap; 
	Microsoft::WRL::ComPtr <ID3D12Resource> depthStencilResource;
	// ビューポート
	D3D12_VIEWPORT viewport{};
	// シザー矩形
	D3D12_RECT scissorRect{};
	// フェンス
	Microsoft::WRL::ComPtr <ID3D12Fence> fence;
	uint64_t fenceValue = 0;
	HANDLE fenceEvent;
	// TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	// WindowsAPI
	WinAPIManager* winApiManager_ = nullptr;
};

