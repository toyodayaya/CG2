#include "Object3d.h" 
#include "Object3dCommon.h"
#include "MathManager.h"
#include "ModelManager.h"
#include "ImGuiManager.h"

using namespace MathManager;

void Object3d::Initialize(Object3dCommon* object3dManager)
{
	// 引数で受け取ってメンバ変数として記録する
	this->object3dManager = object3dManager;
	dxBasis_ = object3dManager->GetDxBasis();


	// デフォルトカメラをセット
	this->camera = object3dManager->GetDefaultCamera();
	
	// 座標変換行列データ作成
	CreateTransformMatrixData3d();

	// 平行光源データ作成
	CreateDirectionalLight();

	// Transform変数を作る
	cameraTransform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} };
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

}

void Object3d::CreateTransformMatrixData3d()
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

void Object3d::CreateDirectionalLight()
{


	// 平行光源用のリソースを作る
	directionalLightResource = dxBasis_->CreateBufferResources(sizeof(DirectionalLight));


	//書き込むためのアドレスを取得
	directionalLightResource->Map(
		0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 1.0f;

}

void Object3d::SetModel(const std::string& filePath)
{
	// モデルを検索してセットする
	model = ModelManager::GetInstance()->FindModel(filePath);
}

void Object3d::Update()
{
	//transform.translate.y += 0.01f;
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 worldViewProjectionMatrix;

	if (camera)
	{
		const Matrix4x4& viewProjectionMatrix = camera->GetViewProjectionMatrix();
		worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
	}
	else
	{
		worldViewProjectionMatrix = worldMatrix;
	}

	transformationData->WVP = worldViewProjectionMatrix;
	transformationData->World = worldMatrix;

}

void Object3d::Draw()
{
	
	// wvp用のCBufferの場所を設定
	dxBasis_->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationResource->GetGPUVirtualAddress());
	// 平行光源用のCBufferの場所を設定
	dxBasis_->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

	// 3Dモデルが割り当てられていれば描画する
	if (model)
	{
		model->Draw();
	}
}