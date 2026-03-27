#include "SrvManager.h"
#include <cassert>

const uint32_t SrvManager::kMaxSRVCount = 512;

void SrvManager::Initialize(DirectXBasis* dxBasis)
{
	// 引数で受け取ってメンバ変数として記録する
	this->dxBasis_ = dxBasis;

	// SRV用のディスクリプタヒープを作成
	descriptorHeap = dxBasis_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);
	// DescriptorSizeを取得しておく
	descriptorSize = dxBasis_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

}

uint32_t SrvManager::Allocate()
{
	// 上限に達していないかチェック
	assert(useIndex < kMaxSRVCount);

	// returnする番号を記録する
	int index = useIndex;
	// 番号を進める
	useIndex++;
	// 記録した番号をreturn
	return index;

}

void SrvManager::CreateSRVforTexture2D(uint32_t index, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels)
{
	// SRVを生成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	// SRVの設定を行う
	srvDesc.Format = format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = mipLevels;

	// srvの生成
	dxBasis_->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, GetCPUDescriptorHandle(index));

}

void SrvManager::CreateSRVforStructuredBuffer(uint32_t index, Microsoft::WRL::ComPtr <ID3D12Resource> pResource, DXGI_FORMAT format, UINT numElements, UINT structureByteStride)
{
	// SRVを生成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	// SRVの設定を行う
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

	srvDesc.Buffer.FirstElement = 0;                          // 先頭から
	srvDesc.Buffer.NumElements = numElements;                 // 要素数
	srvDesc.Buffer.StructureByteStride = structureByteStride; // 1要素のバイト数
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	// srvの生成
	dxBasis_->GetDevice()->CreateShaderResourceView(pResource.Get(), &srvDesc, GetCPUDescriptorHandle(index));
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

bool SrvManager::CanAllocate() const
{
	return useIndex < kMaxSRVCount;
}

void SrvManager::PreDraw()
{
	// 描画用のDescriptorHeapの設定
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeaps[] = { descriptorHeap };
	dxBasis_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps->GetAddressOf());
}

void SrvManager::SetGraphicsRootDiscriptorTable(UINT rootParameterIndex, uint32_t srvIndex)
{
	dxBasis_->GetCommandList()->SetGraphicsRootDescriptorTable(rootParameterIndex, GetGPUDescriptorHandle(srvIndex));
}
