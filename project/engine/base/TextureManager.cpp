#include "TextureManager.h"
#include "DirectXBasis.h"
#include "StringUtility.h"
#include <algorithm>

using namespace StringUtility;


TextureManager* TextureManager::instance = nullptr;

// ImGuiで０番を使用するため1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

TextureManager* TextureManager::GetInstance()
{
	if (instance == nullptr)
	{
		instance = new TextureManager;
	}

	return instance;
}

void TextureManager::Finalize()
{
	delete instance;
	instance = nullptr;
}

void TextureManager::Initialize(DirectXBasis* dxBasis)
{
	// SRVの数と同数
	textureDatas.reserve(DirectXBasis::kMaxSRVCount);

	// 引数で受け取ってメンバ変数として記録する
	this->dxBasis_ = dxBasis;
}

void TextureManager::LoadTexture(const std::string& filePath)
{
	// 読み込み済みテクスチャを検索
	auto it = std::find_if(
		textureDatas.begin(),
		textureDatas.end(),
		[&](TextureData& textureData) {return textureData.filePath == filePath; }
	);
	if (it != textureDatas.end())
	{
		// 読み込み済みなら早期リターン
		return;
	}

	// テクスチャ枚数上限チェック
	assert(textureDatas.size() + kSRVIndexTop < dxBasis_->kMaxSRVCount);

	// テクスチャファイルを読み込んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミニマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	// テクスチャデータを追加
	textureDatas.resize(textureDatas.size() + 1);
	// 追加したテクスチャデータの参照を取得する
	TextureData& textureData = textureDatas.back();

	// 追加したテクスチャデータに書き込む
	textureData.filePath = filePath;
	textureData.metaData = mipImages.GetMetadata();
	textureData.resource = dxBasis_->CreateTextureResource(textureData.metaData);

	// テクスチャデータの要素数番号をSRVのインデックスとする
	uint32_t srvIndex = static_cast<uint32_t>(textureDatas.size() - 1) + kSRVIndexTop;

	textureData.srvHandleCPU = dxBasis_->GetSRVCPUDescriptorHandle(srvIndex);
	textureData.srvHandleGPU = dxBasis_->GetSRVGPUDescriptorHandle(srvIndex);

	// SRVを生成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	// SRVの設定を行う
	srvDesc.Format = image.GetMetadata().format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(image.GetMetadata().mipLevels);

	// srvの生成
	dxBasis_->GetDevice()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);

	// テクスチャデータを転送する
	// 転送用に生成した中間リソースをテクスチャデータ構造体に格納
	textureData.intermediateResource = dxBasis_->UploadTextureData(textureData.resource.Get(), mipImages);

}

void TextureManager::ReleaseIntermediateResources()
{
	for (TextureData& textureData : textureDatas) 
	{
		if (textureData.intermediateResource) 
		{
			// ComPtrのResetを呼び出してリソースを解放し、nullptrにする
			textureData.intermediateResource.Reset();
		}
	}
}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{
	// 読み込み済みテクスチャデータを検索
	auto it = std::find_if(
		textureDatas.begin(),
		textureDatas.end(),
		[&](TextureData& textureData) {return textureData.filePath == filePath; }
	);
	if (it != textureDatas.end())
	{
		// 読み込み済みなら要素番号を返す
		uint32_t textureIndex = static_cast<uint32_t>(std::distance(textureDatas.begin(), it));
		return textureIndex;
	}

	assert(0);
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSRVHandleGPU(uint32_t textureIndex)
{
	// 範囲外指定違反チェック
	assert(textureIndex < textureDatas.size());

	TextureData& textureData = textureDatas[textureIndex];
	return textureData.srvHandleGPU;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(uint32_t textureIndex)
{
	// 範囲外指定違反チェック
	assert(textureIndex < textureDatas.size());

	TextureData& textureData = textureDatas[textureIndex];
	return textureData.metaData;
}
