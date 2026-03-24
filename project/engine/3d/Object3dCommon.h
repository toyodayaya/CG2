#pragma once
#include "DirectXBasis.h"

class Object3dCommon
{
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

private:
	// ポインタ
	DirectXBasis* dxBasis_ = nullptr;
	// ルートシグネチャー
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature;
	// グラフィックスパイプラインステート
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicPipelineState;

};

