#include "Sprite.h"
#include "SpriteManager.h"
using namespace MathManager;

void Sprite::Initialize(SpriteManager* spriteManager)
{
	// 引数で受け取ってメンバ変数に記録する
	this->spriteManager_ = spriteManager;
	dxBasis_ = spriteManager->GetDxBasis();

	// 頂点データ作成
	CreateVertexData();

	// マテリアルデータ作成
	CreateMaterialData();

	// 座標変換行列データ作成
	CreateTransformMatrixData();

}

void Sprite::CreateVertexData()
{
	// 頂点リソースを作成
	vertexResource = dxBasis_->CreateBufferResources(sizeof(VertexData) * 4);
	indexResource = dxBasis_->CreateBufferResources(sizeof(uint32_t) * 6);

	// 頂点バッファビューを作成する
	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 4;
	// 1頂点辺りのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// 頂点バッファビューを作成する
	// リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();

	// 頂点データを設定する
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// 使用するリソースのサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	// 1頂点辺りのサイズ
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	// インデックスリソースにデータを書き込む
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
}

void Sprite::CreateMaterialData()
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
}

void Sprite::CreateTransformMatrixData()
{
	// WVP用のリソースを作る
	transformationResource = dxBasis_->CreateBufferResources(sizeof(TransformationMatrix));
	// データを書き込む
	// 書き込むためのアドレスを取得
	transformationResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationData));
	// 単位行列を書き込んでおく
	transformationData->World = MakeIdentity4x4();
	transformationData->WVP = MakeIdentity4x4();

}

void Sprite::Update()
{
	
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 1;
	indexData[4] = 3;
	indexData[5] = 2;


	// 1枚目
	vertexData[0].position = { 0.0f,1.0f,0.0f,1.0f }; // 左下
	vertexData[0].texcoord = { 0.0f,1.0f };
	vertexData[0].normal = { 0.0f,0.0f,-1.0f };
	vertexData[1].position = { 0.0f,0.0f,0.0f,1.0f }; // 左上
	vertexData[1].texcoord = { 0.0f,0.0f };
	vertexData[1].normal = { 0.0f,0.0f,-1.0f };
	vertexData[2].position = { 1.0f,1.0f,0.0f,1.0f }; // 右下
	vertexData[2].texcoord = { 1.0f,1.0f };
	vertexData[2].normal = { 0.0f,0.0f,-1.0f };
	vertexData[3].position = { 1.0f,0.0f,0.0f,1.0f }; // 左上
	vertexData[3].texcoord = { 1.0f,0.0f };
	vertexData[3].normal = { 0.0f,0.0f,-1.0f };

	// Transform変数を作る
	Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	

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

	
}

void Sprite::Draw()
{
	// VBVを設定
	dxBasis_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
	// IBVを設定
	dxBasis_->GetCommandList()->IASetIndexBuffer(&indexBufferView);
	// マテリアルCBufferの場所を設定
	dxBasis_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	// wvp用のCBufferの場所を設定
	dxBasis_->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationResource->GetGPUVirtualAddress());
	// SRVのDescriptorTableの先頭を設定
	dxBasis_->GetCommandList()->SetGraphicsRootDescriptorTable(2, dxBasis_->GetSRVGPUDescriptorHandle(1));
	
	// 描画
	dxBasis_->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}
