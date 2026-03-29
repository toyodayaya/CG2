#pragma once
#include "DirectXBasis.h"
#include <wrl.h>
#include <d3d12.h>
#include <dxgi.h>

class SrvManager
{
public:
	// シングルトンインスタンス取得関数
	static SrvManager* GetInstance();

	// 初期化
	void Initialize(DirectXBasis* dxBasis);

	// 確保
	uint32_t Allocate();

	// SRV生成(テクスチャ用)
	void CreateSRVforTexture2D(uint32_t index, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels);
	// SRV生成(StructuredBuffer用)
	void CreateSRVforStructuredBuffer(uint32_t index, Microsoft::WRL::ComPtr <ID3D12Resource> pResource, DXGI_FORMAT format, UINT numElements,
		UINT structureByteStride);

	// 描画前処理
	void PreDraw();

	// SRVセットコマンド
	void SetGraphicsRootDiscriptorTable(UINT rootParameterIndex, uint32_t srvIndex);


	// SRV専用のディスクリプタハンドル取得関数
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

	// ディスクリプタヒープ取得関数
	ID3D12DescriptorHeap* GetDescriptorHeap() const { return descriptorHeap.Get(); }

	// テクスチャ最大数チェック
	bool CanAllocate() const;

	// 最大SRV数(最大テクスチャ枚数)
	static const uint32_t kMaxSRVCount;

private:
	static SrvManager* instance;

	DirectXBasis* dxBasis_ = nullptr;

	// ディスクリプタサイズ
	uint32_t descriptorSize;
	// SRV用のディスクリプタヒープを作成
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeap;

	// 次に使用するSRVインデックス
	uint32_t useIndex = 0;

};

