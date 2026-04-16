#pragma once
#include "DirectXBasis.h"
#include "Camera.h"

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

	enum BlendMode
	{
		// ブレンドなし
		kBlendModeNone,
		// 通常ブレンド
		kBlendModeNormal,
		// 加算
		kBlendModeAdd,
		// 減算
		kBlendModeSubstract,
		// 乗算
		kBlendModeMultiply,
		// スクリーン
		kBlendModeScreen,
		// 利用禁止
		kCountOfBlendMode
	};

public:
	// 初期化
	void Initialize(DirectXBasis* directXBasis);
	// ルートシグネチャーの作成
	void CreateRootSignature();
	// グラフィックスパイプラインの生成
	void GenerateGraphicsPipeline();
	// 共通描画設定
	void DrawSettingCommon();
	// ブレンドモード設定
	void BlendModeSetting();
	// getter
	DirectXBasis* GetDxBasis() const { return dxBasis_; }
	Camera* GetDefaultCamera() const { return defaultCamera_; }
	// setter
	void SetDefaultCamera(Camera* camera) { this->defaultCamera_ = camera; }

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
	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	// ブレンドモード
	BlendMode blendMode_ = kBlendModeNormal;
	// デフォルトカメラ
	Camera* defaultCamera_ = nullptr;
};

