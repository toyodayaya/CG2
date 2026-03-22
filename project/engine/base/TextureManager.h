#pragma once
#include <string>
#include <vector>
#include <d3d12.h>
#include <externals/DirectXTex/DirectXTex.h>
#include"externals/DirectXTex/d3dx12.h"
#include <wrl.h>
#include "DirectXBasis.h"

class TextureManager
{
private:
	static TextureManager* instance;

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;

	// テクスチャ1枚分のデータ
	struct TextureData
	{
		std::string filePath;
		DirectX::TexMetadata metaData;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
	};

	// テクスチャデータ
	std::vector<TextureData> textureDatas;

	// ポインタ
	DirectXBasis* dxBasis_ = nullptr;

	// SRVインデックスの開始番号
	static uint32_t kSRVIndexTop;

public:
	// シングルトンインスタンスの取得
	static TextureManager* GetInstance();
	// 終了
	void Finalize();
	// 初期化
	void Initialize(DirectXBasis* dxBasis);

	// LoadTexture関数
	void LoadTexture(const std::string& filePath);

	// 中間リソースを開放
	void ReleaseIntermediateResources();

	// SRVインデックスの開始番号
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);

	// テクスチャ番号からGPUハンドルを取得する
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU(uint32_t textureIndex);

	// メタデータを取得
	const DirectX::TexMetadata& GetMetaData(uint32_t textureIndex);

};

