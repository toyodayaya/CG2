#include "Camera.h"
#include "WinAPIManager.h"
#include "ImguiManager.h"

using namespace MathManager;

void Camera::Update()
{
	// ビュー行列の計算処理
	worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	viewMatrix = Inverse(worldMatrix);

	// プロジェクション行列の計算処理
	projectionMatrix = MakePerspectiveFovMatrix(fovY, aspectRatio, nearClip, farClip);

	// 合成行列の計算処理
	viewProjectionMatrix = Multiply(viewMatrix,projectionMatrix);

#ifdef USE_IMGUI

	// 開発用UIの処理
	ImGui::Begin("Camera");
	ImGui::DragFloat3("Rotate", &transform.rotate.x, 0.01f);
	ImGui::DragFloat3("Translate", &transform.translate.x, 0.01f);
	ImGui::End();

#endif // USE_IMGUI


}

Camera::Camera()
	: transform({{1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f}})
	, fovY(0.45f)
	, aspectRatio(float(WinAPIManager::kClientWidth) / float(WinAPIManager::kClientHeight))
	, nearClip(0.1f)
	, farClip(100.0f)
	, worldMatrix(MakeAffineMatrix(transform.scale, transform.rotate, transform.translate))
	, viewMatrix(Inverse(worldMatrix))
	, projectionMatrix(MakePerspectiveFovMatrix(fovY, aspectRatio, nearClip, farClip))
	, viewProjectionMatrix(Multiply(viewMatrix, projectionMatrix))
{ }
