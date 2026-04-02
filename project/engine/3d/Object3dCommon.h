#pragma once
#include "DirectXBasis.h"
#include "Camera.h"

class Object3dCommon
{
private:
	// コンストラクタ
	Object3dCommon() = default;
	// デストラクタ
	~Object3dCommon() = default;
	// コピーコンストラクタとコピー代入演算子を削除
	Object3dCommon(const Object3dCommon&) = delete;
	Object3dCommon& operator=(const Object3dCommon&) = delete;
	// インスタンス
	static Object3dCommon* instance;

public:
	// 初期化
	void Initialize(DirectXBasis* directXBasis);
	// ルートシグネチャーの作成
	void CreateRootSignature();
	// グラフィックスパイプラインの生成
	void GenerateGraphicsPipeline();
	// 共通描画設定
	void DrawSettingCommon();
	// getter
	DirectXBasis* GetDxBasis() const { return dxBasis_; }
	Camera* GetDefaultCamera() const { return defaultCamera_; }
	// setter
	void SetDefaultCamera(Camera* camera) { this->defaultCamera_ = camera; }

	// インスタンス
	static Object3dCommon* GetInstance();
	// 終了
	void Finalize();

private:
	// ポインタ
	DirectXBasis* dxBasis_ = nullptr;
	// ルートシグネチャー
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature;
	// グラフィックスパイプラインステート
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicPipelineState;

	// デフォルトカメラ
	Camera* defaultCamera_ = nullptr;

};

