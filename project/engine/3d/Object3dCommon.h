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
	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob;
	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob;
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = { };
	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// ブレンドモード
	BlendMode blendMode_ = kBlendModeNone;

	// デフォルトカメラ
	Camera* defaultCamera_ = nullptr;

};

