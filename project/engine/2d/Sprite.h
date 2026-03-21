#pragma once
#include "MathManager.h"
#include <wrl.h>
#include <d3d12.h>
#include <stdint.h>

class SpriteManager;
class DirectXBasis;

struct VertexData
{
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct  Material
{
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};

struct TransformationMatrix
{
	Matrix4x4 WVP;
	Matrix4x4 World;
};

class Sprite
{
public:
	// 初期化
	void Initialize(SpriteManager* spriteManager);

	// 頂点データ作成
	void CreateVertexData();

	// マテリアルデータ作成
	void CreateMaterialData();

	// 座標変換行列データ作成
	void CreateTransformMatrixData();

	// 更新
	void Update();

	// 描画
	void Draw();

	// getter
	const Vector2& GetPosition() const { return pos; }
	float GetRotation() const { return rotation; }
	const Vector4& GetColor() const { return materialData->color; }
	const Vector2& GetSize() const { return size; }
	// setter
	void SetPosition(const Vector2& position) { this->pos = position; }
	void SetRotation(float rotation) { this->rotation = rotation; }
	void SetColor(const Vector4& color) { materialData->color = color; }
	void SetSize(const Vector2& size) { this->size = size; }

private:
	// ポインタ
	SpriteManager* spriteManager_;
	DirectXBasis* dxBasis_;

	// Index用の頂点リソース
	Microsoft::WRL::ComPtr <ID3D12Resource> indexResource;

	// Sprite用の頂点リソース
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource;

	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	uint32_t* indexData = nullptr;

	// バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	// マテリアル用のリソースを作る
	Microsoft::WRL::ComPtr <ID3D12Resource> materialResource;
	// マテリアルにデータを書き込む
	Material* materialData = nullptr;

	// WVP用のリソースを作る
	Microsoft::WRL::ComPtr <ID3D12Resource> transformationResource;
	// データを書き込む
	TransformationMatrix* transformationData = nullptr;

	// 座標
	Vector2 pos = { 0.0f,0.0f };
	// 回転
	float rotation = 0.0f;
	// サイズ
	Vector2 size = { 640.0f,360.0f };
};
