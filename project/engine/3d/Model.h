#pragma once
#include "ModelCommon.h"
#include "MathManager.h"
#include <wrl.h>
#include <d3d12.h>


class Model
{
private:
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
		float shininess;
		float paddings[3];
	};

	struct MaterialData
	{
		std::string textureFilePath;
		uint32_t textureIndex = 0;
	};


	struct ModelData
	{
		std::vector<VertexData> vertices;
		MaterialData material;
	};

public:
	// 初期化
	void Initialize(ModelCommon* modelManager, const std::string& directoryPath, const std::string& filePath);
	// 描画
	void Draw();
	// mtlファイルを読む関数
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	// Objファイルを読み込む関数
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& fileName);
	// 頂点データを作成
	void CreateVertexData3d();
	// マテリアルデータを作成
	void CreateMaterialData3d();

private:


	ModelCommon* modelManager = nullptr;
	DirectXBasis* dxBasis_;
	// objファイルのデータ
	ModelData modelData;
	// VertexResource
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource;
	// 頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	// 頂点データ
	VertexData* vertexData = nullptr;
	// マテリアルリソース
	Microsoft::WRL::ComPtr <ID3D12Resource> materialResource;
	Material* materialData = nullptr;
};

