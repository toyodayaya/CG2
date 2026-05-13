#pragma once
#include "DirectXBasis.h"
#include "Camera.h"
#include "SrvManager.h"

class RenderTexture
{
private:
	// コンストラクタ
	RenderTexture() = default;
	// デストラクタ
	~RenderTexture() = default;
	// コピーコンストラクタとコピー代入演算子を削除
	RenderTexture(const RenderTexture&) = delete;
	RenderTexture& operator=(const RenderTexture&) = delete;
	// インスタンス
	static RenderTexture* instance;

public:
	// 初期化
	void Initialize(DirectXBasis* directXBasis, SrvManager* srvManager);
	// ルートシグネチャーの作成
	void CreateRootSignature();
	// グラフィックスパイプラインの生成
	void GenerateGraphicsPipeline();
	// 共通描画設定
	void DrawSettingCommon(D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU);
	// getter
	DirectXBasis* GetDxBasis() const { return dxBasis_; }
	Camera* GetDefaultCamera() const { return defaultCamera_; }
	// setter
	void SetDefaultCamera(Camera* camera) { this->defaultCamera_ = camera; }

	// インスタンス
	static RenderTexture* GetInstance();
	// 終了
	void Finalize();

private:
	// ポインタ
	DirectXBasis* dxBasis_ = nullptr;
	SrvManager* srvManager_ = nullptr;
	// ルートシグネチャー
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature;
	// グラフィックスパイプラインステート
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicPipelineState;
	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	// デフォルトカメラ
	Camera* defaultCamera_ = nullptr;
};

