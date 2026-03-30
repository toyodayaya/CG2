#include "ParticleManager.h"
#include "DirectXBasis.h"
#include "Logger.h"
#include <random>
#include <cassert>
#include "MathManager.h"
#include "TextureManager.h"
#include "ImguiManager.h"

using namespace Logger;
using namespace MathManager;

ParticleManager* ParticleManager::instance = nullptr;

ParticleManager* ParticleManager::GetInstance()
{
	if (instance == nullptr)
	{
		instance = new ParticleManager;
	}

	return instance;
}

void ParticleManager::Finalize()
{
	delete instance;
	instance = nullptr;
}

void ParticleManager::Initialize(DirectXBasis* dxBasis, SrvManager* srvManager, Camera* camera)
{
	// 引数で受け取ってメンバ変数として記録する
	this->dxBasis_ = dxBasis;
	this->srvManager_ = srvManager;
	this->camera = camera;

	// ランダムエンジンの初期化
	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());


	// グラフィックスパイプラインの生成
	GenerateGraphicsPipeline();

	// 頂点データ作成
	CreateVertexData();
}

void ParticleManager::CreateRootSignature()
{
	// RootSignatureを作成
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;// 0から始まる
	descriptorRange[0].NumDescriptors = 1; // 数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootParameterを作成
	D3D12_ROOT_PARAMETER rootParameters[3] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // VertexShaderで使う
	rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
	//rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	//rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//rootParameters[3].Descriptor.ShaderRegister = 1;
	descriptionRootSignature.pParameters = rootParameters; // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters); // 配列の長さ

	// Samplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr <ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr <ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr))
	{
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	// バイナリを元に生成
	hr = dxBasis_->GetDevice()->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));


}

void ParticleManager::GenerateGraphicsPipeline()
{
	// ルートシグネチャーの作成
	CreateRootSignature();

	// InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = { };
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// BlendStateの設定
	// 全ての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	// RasterizerStateの設定を行う
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	// カリングしない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	// ShaderをCompileする
	vertexShaderBlob = dxBasis_->CompileShader(L"resources/shaders/Particle.VS.hlsl",
		L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	pixelShaderBlob = dxBasis_->CompileShader(L"resources/shaders/Particle.PS.hlsl",
		L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	// DepthStencilStateの設定
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	// 書き込む
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	// 比較関数
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// PSOを生成する
	graphicPipelineStateDesc.pRootSignature = rootSignature.Get();
	graphicPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicPipelineStateDesc.VS =
	{
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize()
	};
	graphicPipelineStateDesc.PS =
	{
		pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize()
	};
	graphicPipelineStateDesc.BlendState = blendDesc;
	graphicPipelineStateDesc.RasterizerState = rasterizerDesc;
	// 書き込むRTVの情報
	graphicPipelineStateDesc.NumRenderTargets = 1;
	graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// 利用するトポロジのタイプ(三角形)
	graphicPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むかの設定
	graphicPipelineStateDesc.SampleDesc.Count = 1;
	graphicPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// DepthStencilの設定
	graphicPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 生成
	HRESULT hr = dxBasis_->GetDevice()->CreateGraphicsPipelineState(&graphicPipelineStateDesc,
		IID_PPV_ARGS(&graphicPipelineState));

	assert(SUCCEEDED(hr));

}

void ParticleManager::CreateVertexData()
{
	// パーティクルの初期化
	modelData.vertices.push_back({ .position = {1.0f,1.0f,0.0f,1.0f},.texcoord = {0.0f,0.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {-1.0f,1.0f,0.0f,1.0f},.texcoord = {1.0f,0.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {1.0f,-1.0f,0.0f,1.0f},.texcoord = {0.0f,1.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {1.0f,-1.0f,0.0f,1.0f},.texcoord = {0.0f,1.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {-1.0f,1.0f,0.0f,1.0f},.texcoord = {1.0f,0.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {-1.0f,-1.0f,0.0f,1.0f},.texcoord = {1.0f,1.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.material.textureFilePath = "resources/circle.png";

	// 頂点リソースを作る
	vertexResource = dxBasis_->CreateBufferResources(sizeof(VertexData) * modelData.vertices.size());

	// 頂点バッファビューを作成する
	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点3つ分のサイズ
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	// 1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	// 頂点リソースにデータを書き込む
	// 書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// 頂点データにリソースをコピー
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());


}

ParticleManager::Particle ParticleManager::MakeNewParticle(const Vector3& translate)
{
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	particle.transform.scale = { 1.0f,1.0f,1.0f };
	particle.transform.rotate = { 0.0f,0.0f,0.0f };
	randomTranslate = { distribution(randomEngine),distribution(randomEngine) ,distribution(randomEngine) };
	particle.transform.translate = Vector3Add(translate, randomTranslate);
	particle.velocity = { distribution(randomEngine),distribution(randomEngine),distribution(randomEngine) };
	particle.color = { distribution(randomEngine),distribution(randomEngine),distribution(randomEngine) ,1.0f };
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);
	particle.lifeTime = distTime(randomEngine);
	particle.currentTime = 0;
	return particle;
}


bool ParticleManager::IsCollision(const AABB& aabb, const Vector3& point) {
	if (aabb.min.x <= point.x && aabb.max.x >= point.x &&
		aabb.min.y <= point.y && aabb.max.y >= point.y &&
		aabb.min.z <= point.z && aabb.max.z >= point.z) {
		return true;
	}

	return false;
}

void ParticleManager::CreateParticleGroup(const std::string name, const std::string textureFilePath)
{
	// 登録済みの名前かチェック
	const bool alreadyExists =
		(particleGroups.find(name) != particleGroups.end());
	assert(!alreadyExists);

	// 新たなパーティクルグループを作成
	ParticleGroup newGrounp{};
	particleGroups.emplace(name, std::move(newGrounp));

	// 追加したマテリアルデータの参照を取得する
	ParticleGroup& particleData = particleGroups.at(name);

	// テクスチャファイルを読み込む
	particleData.materialData.textureFilePath = textureFilePath;
	particleData.materialData.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);

	// インスタンシング用リソースを生成
	particleData.instancingResource = dxBasis_->CreateBufferResources(sizeof(ParticleForGPU) * kMaxInstanceCount);
	// 書き込むためのアドレスを取得
	particleData.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&particleData.instancingData));
	// 単位行列を書き込んでおく
	for (uint32_t index = 0; index < kMaxInstanceCount; ++index)
	{
		particleData.instancingData[index].WVP = MakeIdentity4x4();
		particleData.instancingData[index].World = MakeIdentity4x4();
		particleData.instancingData[index].Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// インスタンシング用にSRVを確保してインデックスを記録
	particleData.srvIndex = srvManager_->Allocate();

	// SRVの生成
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = kMaxInstanceCount;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
	D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU = srvManager_->GetCPUDescriptorHandle(3);
	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU = srvManager_->GetGPUDescriptorHandle(3);
	srvManager_->CreateSRVforStructuredBuffer(particleData.srvIndex, particleData.instancingResource.Get(), instancingSrvDesc.Format, kMaxInstanceCount, sizeof(ParticleForGPU));
}

void ParticleManager::Update()
{
#ifdef USE_IMGUI

	ImGui::Begin("Billboard");
	ImGui::Checkbox("Billboard", &isBillboard);
	ImGui::End();

#endif


	// ビルボードの計算処理
	Matrix4x4 cameraMatrix = camera->GetWorldMatrix();
	Matrix4x4 billboardMatrix = MakeIdentity4x4();

	if (isBillboard)
	{
		Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
		billboardMatrix = Multiply(backToFrontMatrix, cameraMatrix);
		billboardMatrix.m[3][0] = 0.0f;
		billboardMatrix.m[3][1] = 0.0f;
		billboardMatrix.m[3][2] = 0.0f;
	}

	// 全てのパーティクルグループ
	for (auto& [name, group] : particleGroups)
	{
		group.instanceCount = 0;
		// 各グループのリスト
		for (auto it = group.particles.begin(); it != group.particles.end();)
		{
			if (it->lifeTime <= it->currentTime)
			{
				// 生存時間を過ぎていたら表示しない
				it = group.particles.erase(it);
				continue;
			}

			// Fieldの範囲内のParticleにはAccelerationを適用
			if (IsCollision(accelerationField.area, it->transform.translate))
			{
				Vector3 accelDelta =
				{
				   accelerationField.acceleration.x * kDeltaTime,
				   accelerationField.acceleration.y * kDeltaTime,
				   accelerationField.acceleration.z * kDeltaTime
				};
				it->velocity = Vector3Add(it->velocity, accelDelta);
			}

			it->transform.translate.y += it->velocity.y * kDeltaTime;
			it->currentTime += kDeltaTime;
			float alpha = 1.0f - (it->currentTime / it->lifeTime);

			Matrix4x4 scaleMatrix = MakeScaleMatrix(it->transform.scale);
			Matrix4x4 translateMatrix = MakeTranslateMatrix(it->transform.translate);
			Matrix4x4 worldMatrix;

			if (isBillboard)
			{
				worldMatrix = Multiply(scaleMatrix, billboardMatrix);
				worldMatrix = Multiply(worldMatrix, translateMatrix);
			}
			else
			{
				worldMatrix = MakeAffineMatrix(it->transform.scale, it->transform.rotate, it->transform.translate);
			}

			Matrix4x4 viewProjectionMatrix = camera->GetViewProjectionMatrix();
			Matrix4x4 wvp = Multiply(worldMatrix,viewProjectionMatrix);

			// インスタンシング用データ1個分を書き込み
			if (group.instanceCount < kMaxInstanceCount) {
				ParticleForGPU& out = group.instancingData[group.instanceCount];
				out.World = worldMatrix;
				out.WVP = wvp;
				out.Color = it->color;
				out.Color.w = alpha;

				group.instanceCount++;
			}

#ifdef USE_IMGUI

			ImGui::Begin("Particle Manager");
			ImGui::DragFloat3("Position", &it->transform.translate.x, 0.1f);
			
			ImGui::End();

#endif

			++it;

		}

	}

}

void ParticleManager::Draw()
{
	// RootSignatureを設定
	dxBasis_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
	// PSOを設定
	dxBasis_->GetCommandList()->SetPipelineState(graphicPipelineState.Get());
	// 形状を設定
	dxBasis_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// VBVを設定
	dxBasis_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	
	// グループごとに描画
	for (auto& [name, group] : particleGroups)
	{
		if (group.instanceCount == 0)
		{
			// 一つもなかったら描画をスキップ
			continue;
		}

		// テクスチャのSRVのDescriptorTableを設定
		dxBasis_->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSRVHandleGPU(group.materialData.textureFilePath));
		// SRVのDescriptorTableの先頭を設定
		dxBasis_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group.srvIndex));
		// 描画
		dxBasis_->GetCommandList()->DrawInstanced(static_cast<UINT>(modelData.vertices.size()), group.instanceCount, 0, 0);
	}
}

void ParticleManager::Emit(const std::string name, const Vector3& position, uint32_t count)
{
	// 登録済みかチェック
	auto it = particleGroups.find(name);
	assert(it != particleGroups.end() && "Particle group not found.");
	ParticleGroup& group = it->second;

	// パーティクルグループを作成して登録
	for (uint32_t i = 0; i < count; ++i) {
		if (group.particles.size() >= kMaxInstanceCount) {
			break; // 上限以上は積まない
		}
		group.particles.push_back(MakeNewParticle(position));
	}
}