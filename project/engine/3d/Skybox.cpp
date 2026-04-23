#include "Skybox.h"
#include "SkyboxCommon.h"
#include "TextureManager.h"
#include "ImGuiManager.h"
#include "StringUtility.h"
using namespace MathManager;
using namespace StringUtility;

void Skybox::Initialize(SkyboxCommon* skyboxCommon, std::string textureFilePath)
{
	// 引数で受け取ってメンバ変数に記録する
	this->skyboxManager_ = skyboxCommon;
	dxBasis_ = skyboxCommon->GetDxBasis();
	filePath = textureFilePath;

	// デフォルトカメラをセット
	this->camera = skyboxManager_->GetDefaultCamera();

	// 頂点データ作成
	CreateVertexData();

	// マテリアルデータ作成
	CreateMaterialData();

	// 座標変換行列データ作成
	CreateTransformMatrixData();

	// カメラデータ作成
	CreateCameraResource();

	// 単位行列を書き込んでおく
	textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);

	// テクスチャのサイズをイメージに合わせる
	AdjustTextureSize();
}

void Skybox::CreateVertexData()
{
	// 頂点リソースを作成
	vertexResource = dxBasis_->CreateBufferResources(sizeof(VertexDataSkybox) * 24);
	indexResource = dxBasis_->CreateBufferResources(sizeof(uint32_t) * 36);


	// 頂点バッファビューを作成する
	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(VertexDataSkybox) * 24;
	// 1頂点辺りのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexDataSkybox);


	// 頂点バッファビューを作成する
	// リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();

	// 頂点データを設定する
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSkybox));
	// 使用するリソースのサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 36;


	// 1頂点辺りのサイズ
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	// インデックスリソースにデータを書き込む
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
}

void Skybox::CreateMaterialData()
{
	// マテリアル用のリソースを作る
	materialResource = dxBasis_->CreateBufferResources(sizeof(Material));
	// マテリアルにデータを書き込む
	// 書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// 赤色に設定
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	// Lightingするのでtrueに設定
	materialData->enableLighting = true;
	// UVTransform行列を単位行列で初期化
	materialData->uvTransform = MakeIdentity4x4();
	// 光沢度
	materialData->shininess = 100.0f;
}

void Skybox::CreateTransformMatrixData()
{
	// WVP用のリソースを作る
	transformationResource = dxBasis_->CreateBufferResources(sizeof(TransformationMatrix));
	// データを書き込む
	// 書き込むためのアドレスを取得
	transformationResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationData));
	// 単位行列を書き込んでおく
	transformationData->World = MakeIdentity4x4();
	transformationData->WVP = MakeIdentity4x4();
	transformationData->WorldInverseTranspose = Inverse(transformationData->World);
}

void Skybox::CreateCameraResource()
{
	// カメラリソースの生成
	cameraResource = dxBasis_->CreateBufferResources(sizeof(CameraForGPU));
	cameraResource->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));

	cameraData_->worldPosition = camera->GetTranslate();
}

void Skybox::Update()
{
	for (uint32_t i = 0; i < 6; ++i) {
		uint32_t offset = i * 4;
		uint32_t idx = i * 6;
		indexData[idx + 0] = offset + 0; indexData[idx + 1] = offset + 1; indexData[idx + 2] = offset + 2;
		indexData[idx + 3] = offset + 1; indexData[idx + 4] = offset + 3; indexData[idx + 5] = offset + 2;
	}

	// 右面
	vertexDataSkybox[0].position = { 1.0f,1.0f,1.0f,1.0f };
	vertexDataSkybox[1].position = { 1.0f,1.0f,-1.0f,1.0f };
	vertexDataSkybox[2].position = { 1.0f,-1.0f,1.0f,1.0f };
	vertexDataSkybox[3].position = { 1.0f,-1.0f,-1.0f,1.0f };
	// 左面
	vertexDataSkybox[4].position = { -1.0f,1.0f,-1.0f,1.0f };
	vertexDataSkybox[5].position = { -1.0f,1.0f,1.0f,1.0f };
	vertexDataSkybox[6].position = { -1.0f,-1.0f,-1.0f,1.0f };
	vertexDataSkybox[7].position = { -1.0f,-1.0f,1.0f,1.0f };
	// 前面
	vertexDataSkybox[8].position = { -1.0f,1.0f,1.0f,1.0f };
	vertexDataSkybox[9].position = { 1.0f,1.0f,1.0f,1.0f };
	vertexDataSkybox[10].position = { -1.0f,-1.0f,1.0f,1.0f };
	vertexDataSkybox[11].position = { 1.0f,-1.0f,1.0f,1.0f };
	// 後面
	vertexDataSkybox[12].position = { 1.0f,1.0f,-1.0f,1.0f };
	vertexDataSkybox[13].position = { -1.0f,1.0f,-1.0f,1.0f };
	vertexDataSkybox[14].position = { 1.0f,-1.0f,-1.0f,1.0f };
	vertexDataSkybox[15].position = { -1.0f,-1.0f,-1.0f,1.0f };
	// 上面
	vertexDataSkybox[16].position = { -1.0f,1.0f,-1.0f,1.0f };
	vertexDataSkybox[17].position = { 1.0f,1.0f,-1.0f,1.0f };
	vertexDataSkybox[18].position = { -1.0f,1.0f,1.0f,1.0f };
	vertexDataSkybox[19].position = { 1.0f,1.0f,1.0f,1.0f };
	// 下面
	vertexDataSkybox[20].position = { -1.0f,-1.0f,1.0f,1.0f };
	vertexDataSkybox[21].position = { 1.0f,-1.0f,1.0f,1.0f };
	vertexDataSkybox[22].position = { -1.0f,-1.0f,-1.0f,1.0f };
	vertexDataSkybox[23].position = { 1.0f,-1.0f,-1.0f,1.0f };

	// Transform変数を作る
	Transform transform{ {10.0f,10.0f,10.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };


	// ゲームの処理
	// 更新処理
	transform.translate = { pos.x,pos.y,0.0f };
	transform.rotate = { 0.0f,0.0f,rotation };
	transform.scale = { size.x,size.y,1.0f };

	// Sprite用のWorldViewProjectionMatrixを作る
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrix = MakeIdentity4x4();
	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, float(WinAPIManager::kClientWidth), float(WinAPIManager::kClientHeight), 0.0f, 100.0f);
	Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
	transformationData->WVP = worldViewProjectionMatrix;
	transformationData->World = worldMatrix;
	transformationData->WorldInverseTranspose = Inverse(transformationData->World);
}

void Skybox::Draw()
{
	// VBVを設定
	dxBasis_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
	// IBVを設定
	dxBasis_->GetCommandList()->IASetIndexBuffer(&indexBufferView);
	// マテリアルCBufferの場所を設定
	dxBasis_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	// wvp用のCBufferの場所を設定
	dxBasis_->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationResource->GetGPUVirtualAddress());
	// カメラリソース用のCBufferの場所を設定
	dxBasis_->GetCommandList()->SetGraphicsRootConstantBufferView(3, cameraResource->GetGPUVirtualAddress());
	// SRVのDescriptorTableの先頭を設定
	dxBasis_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSRVHandleGPU(filePath));
	// 描画
	dxBasis_->GetCommandList()->DrawIndexedInstanced(36, 1, 0, 0, 0);

}

void Skybox::AdjustTextureSize()
{
	// テクスチャメタデータを取得
	const DirectX::TexMetadata& metaData = TextureManager::GetInstance()->GetMetaData(filePath);

	textureSize.x = static_cast<float>(metaData.width);
	textureSize.y = static_cast<float>(metaData.height);
	// 画像サイズをテクスチャサイズに合わせる
	size = textureSize;
}
