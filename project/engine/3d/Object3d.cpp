#include "Object3d.h" 
#include "Object3dCommon.h"
#include "MathManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <externals/imgui/imgui.h>
#include <externals/imgui/imgui_impl_dx12.h>
#include <externals/imgui/imgui_impl_win32.h>

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

#ifdef USE_IMGUI
	//// 開発用UIの処理
	//ImGui::ShowDemoWindow();

	//ImGui::Separator();
	//ImGui::Text("Camera");
	//ImGui::DragFloat3("Camera Pos", &cameraTransform.translate.x, 0.1f);
	//ImGui::SliderAngle("Camera Rot X", &cameraTransform.rotate.x);
	//ImGui::SliderAngle("Camera Rot Y", &cameraTransform.rotate.y);
	//ImGui::SliderAngle("Camera Rot Z", &cameraTransform.rotate.z);

	//ImGui::Separator();
	//ImGui::DragFloat3("model Pos", &transform.translate.x, 0.1f);
	//ImGui::Text("Model Rotation");
	//ImGui::SliderAngle("Rot X", &transform.rotate.x, -180.0f, 180.0f);
	//ImGui::SliderAngle("Rot Y", &transform.rotate.y, -180.0f, 180.0f);
	//ImGui::SliderAngle("Rot Z", &transform.rotate.z, -180.0f, 180.0f);

	//// ImGuiの内部コマンドを生成する
	//ImGui::Render();
#endif // USE_IMGUI
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