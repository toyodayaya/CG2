#pragma once
#include <string>
#include <vector>
#include <wrl.h>
#include <d3d12.h>
#include "DirectXBasis.h"
#include "MathManager.h"
#include "Model.h"
#include "Camera.h"

class Object3dCommon;

struct TransformationMatrix
{
	Matrix4x4 WVP;
	Matrix4x4 World;
};



struct DirectionalLight
{
	Vector4 color;
	Vector3 direction;
	float intensity;
};

class Object3d
{
public:
	// 初期化
	void Initialize(Object3dCommon* object3dManager);
	// 更新
	void Update();
	// 描画
	void Draw();
	
	// 座標変換行列データ作成
	void CreateTransformMatrixData3d();
	// 平行光源データ作成
	void CreateDirectionalLight();

	// setter
	void SetModel(const std::string& filePath);
	void SetScale(const Vector3& scale) { this->transform.scale = scale; }
	void SetRotate(const Vector3& rotate) { this->transform.rotate = rotate; }
	void SetTranslate(const Vector3& translate) { this->transform.translate = translate; }
	void SetCamera(Camera* camera) { this->camera = camera; }

	// getter
	const Vector3& GetScale() const { return transform.scale; }
	const Vector3& GetRotate() const { return transform.rotate; }
	const Vector3& GetTranslate() const { return transform.translate; }


private:
	// ポインタ
	Object3dCommon* object3dManager = nullptr;
	DirectXBasis* dxBasis_;
	Model* model = nullptr;
	Camera* camera = nullptr;
	
	// WVP用のリソースを作る
	Microsoft::WRL::ComPtr <ID3D12Resource> transformationResource;
	// データを書き込む
    TransformationMatrix* transformationData = nullptr;
	// 平行光源リソース
	Microsoft::WRL::ComPtr <ID3D12Resource> directionalLightResource;
	// データを書き込む
	DirectionalLight* directionalLightData = nullptr;


	Transform cameraTransform;
	Transform transform;
};

