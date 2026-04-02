#pragma once
#include "DirectXBasis.h"

class SpriteCommon
{
private:
	// コンストラクタ
	SpriteCommon() = default;
	// デストラクタ
	~SpriteCommon() = default;
	// コピーコンストラクタとコピー代入演算子を削除
	SpriteCommon(const SpriteCommon&) = delete;
	SpriteCommon& operator=(const SpriteCommon&) = delete;
	// インスタンス
	static SpriteCommon* instance;

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

	// インスタンス
	static SpriteCommon* GetInstance();
	// 終了
	void Finalize();
	
private:
	// ポインタ
	DirectXBasis* dxBasis_ = nullptr;
	// ルートシグネチャー
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature;
	// グラフィックスパイプラインステート
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicPipelineState;
};

