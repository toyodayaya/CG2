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
	modelData.vertices.clear();
	// 立方体の8つの角の座標（中心 0,0,0 からの距離）
	modelData.vertices.push_back({ -1.0f,  1.0f, -1.0f, 1.0f }); // 0: 左上奥
	modelData.vertices.push_back({ 1.0f,  1.0f, -1.0f, 1.0f }); // 1: 右上奥
	modelData.vertices.push_back({ -1.0f, -1.0f, -1.0f, 1.0f }); // 2: 左下奥
	modelData.vertices.push_back({ 1.0f, -1.0f, -1.0f, 1.0f }); // 3: 右下奥
	modelData.vertices.push_back({ -1.0f,  1.0f,  1.0f, 1.0f }); // 4: 左上正面
	modelData.vertices.push_back({ 1.0f,  1.0f,  1.0f, 1.0f }); // 5: 右上正面
	modelData.vertices.push_back({ -1.0f, -1.0f,  1.0f, 1.0f }); // 6: 左下正面
	modelData.vertices.push_back({ 1.0f, -1.0f,  1.0f, 1.0f }); // 7: 右下正面

	// 頂点リソースを作成
	vertexResource = dxBasis_->CreateBufferResources(sizeof(VertexDataSkybox) * modelData.vertices.size());
	indexResource = dxBasis_->CreateBufferResources(sizeof(uint32_t) * 36);


	// 頂点バッファビューを作成する
	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(VertexDataSkybox) * static_cast<UINT>(modelData.vertices.size());
	// 1頂点辺りのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexDataSkybox);

	uint32_t indices[] = {
		// 前面
		4, 5, 6, 7, 6, 5,
		// 後面
		1, 0, 3, 2, 3, 0,
		// 左面
		0, 4, 2, 6, 2, 4,
		// 右面
		5, 1, 7, 3, 7, 1,
		// 上面
		0, 1, 4, 5, 4, 1,
		// 下面
		6, 7, 2, 3, 2, 7
	};

	// 頂点バッファビューを作成する
	// リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();

	// 頂点データを設定する
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSkybox));
	// 頂点データにリソースをコピー
	std::memcpy(vertexDataSkybox, modelData.vertices.data(), sizeof(VertexDataSkybox) * modelData.vertices.size());

	// 使用するリソースのサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 36;
	// 1頂点辺りのサイズ
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	// インデックスリソースにデータを書き込む
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	std::memcpy(indexData, indices, sizeof(uint32_t) * 36);
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
	// 座標を設定
	Transform transform{ {500.0f, 500.0f, 500.0f}, {0.0f, 0.0f, 0.0f}, camera->GetTranslate() };

	// ワールド行列を作成
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

	// カメラから各行列を個別に取得
	Matrix4x4 viewMatrix = camera->GetViewMatrix();
	Matrix4x4 projectionMatrix = camera->GetProjectionMatrix();

	// ビュー行列から移動成分を消す
	viewMatrix.m[3][0] = 0.0f;
	viewMatrix.m[3][1] = 0.0f;
	viewMatrix.m[3][2] = 0.0f;

	// 行列を合成
	Matrix4x4 worldView = Multiply(worldMatrix, viewMatrix);
	Matrix4x4 wvp = Multiply(worldView, projectionMatrix);

	// データを転送
	transformationData->WVP = wvp;
	transformationData->World = worldMatrix;
}

void Skybox::Draw()
{
	// 設定
	SkyboxCommon::GetInstance()->DrawSettingCommon();

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
