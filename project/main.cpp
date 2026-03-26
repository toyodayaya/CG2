#pragma warning(push)
// C4023の警告を見なかったことにする
#pragma warning(disable:4023)
#include "WinAPIManager.h"
#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")
#include <strsafe.h>
#include <MathManager.h>

#include <wrl.h>
#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")
#include "Input.h"
#include "DirectXBasis.h"
#include <dxcapi.h>
#include <Logger.h>
#include <StringUtility.h>
#include <externals/imgui/imgui.h>
#include <externals/imgui/imgui_impl_dx12.h>
#include <externals/imgui/imgui_impl_win32.h>
#include "D3DResourceLeakChecker.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "ModelCommon.h"
#include "Model.h"
#include "ModelManager.h"
#include "Camera.h"
#include "SrvManager.h"
#include<random>
#include <numbers>

#pragma warning(pop)

using namespace MathManager;
using namespace Logger;

// チャンクヘッダ
struct ChunkHeader
{
	char id[4];
	int32_t size;
};

// RIFFヘッダチャンク
struct RiffHeader
{
	ChunkHeader chunk;
	char type[4];
};

// FMTチャンク
struct FormatChunk
{
	ChunkHeader chunk;
	WAVEFORMATEX fmt;
};

struct SoundData
{
	// 波形フォーマット
	WAVEFORMATEX wfex;
	// バッファの先頭アドレス
	BYTE* pBuffer;
	// バッファのサイズ
	unsigned int bufferSize;
};

struct VertexData
{
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct  Material
{
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};

struct MaterialData
{
	std::string textureFilePath;
	uint32_t textureIndex = 0;
};


struct ModelData
{
	std::vector<VertexData> vertices;
	MaterialData material;
};

struct Particle
{
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
};

struct ParticleForGPU
{
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 Color;
};

struct Emitter
{
	Transform transform;
	uint32_t count;
	float frequency;
	float frequencyTime;
};

struct AccelerationField
{
	Vector3 acceleration;
	AABB area;
};


// Particle生成関数
Particle MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate)
{
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	Particle particle;
	particle.transform.scale = { 1.0f,1.0f,1.0f };
	particle.transform.rotate = { 0.0f,0.0f,0.0f };
	Vector3 randomTranslate{ distribution(randomEngine),distribution(randomEngine) ,distribution(randomEngine) };
	particle.transform.translate = Vector3Add(translate, randomTranslate);
	particle.velocity = { distribution(randomEngine),distribution(randomEngine),distribution(randomEngine) };
	particle.color = { distribution(randomEngine),distribution(randomEngine),distribution(randomEngine) ,1.0f };
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);
	particle.lifeTime = distTime(randomEngine);
	particle.currentTime = 0;
	return particle;
}

// ParticleEmitter
std::list<Particle> Emit(const Emitter& emitter, std::mt19937& randomEngine)
{
	std::list<Particle> particles;
	for (uint32_t count = 0; count < emitter.count; ++count)
	{
		particles.push_back(MakeNewParticle(randomEngine, emitter.transform.translate));
	}

	return particles;
}

bool IsCollision(const AABB& aabb, const Vector3& point) {
	if (aabb.min.x <= point.x && aabb.max.x >= point.x &&
		aabb.min.y <= point.y && aabb.max.y >= point.y &&
		aabb.min.z <= point.z && aabb.max.z >= point.z) {
		return true;
	}

	return false;
}

// CrashHandlerの登録
static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception)
{
	// 時刻を取得し、時刻を名前に入れてファイルを出力し、Dumpsディレクトリ以下に出力
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpfileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	// processIdとクラッシュの発生したThreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;
	// Dumpを出力
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpfileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
	// 他に関連づけられているSEH例外ハンドラがあれば実行
	return EXCEPTION_EXECUTE_HANDLER;
}


//
//// 音声データ読み込み関数
//SoundData SoundLoadWave(const char* filename)
//{
//	//HRESULT result;
//
//	// ファイルを開く
//	std::ifstream file;
//	file.open(filename, std::ios_base::binary);
//	assert(file.is_open());
//
//	// wavデータ読み込み
//	// RIFFヘッダー読み込み
//	RiffHeader riff;
//	file.read((char*)&riff, sizeof(riff));
//
//	// バイナリがRIFFと一致するかチェック
//	if (strncmp(riff.chunk.id, "RIFF", 4) != 0)
//	{
//		assert(0);
//	}
//
//	// WAVEかどうかチェック
//	if (strncmp(riff.type, "WAVE", 4) != 0)
//	{
//		assert(0);
//	}
//
//	// Formatチャンクの読み込み
//	FormatChunk format = {};
//	// チャンクヘッダーの確認
//	file.read((char*)&format, sizeof(ChunkHeader));
//	if (strncmp(format.chunk.id, "fmt ", 4) != 0)
//	{
//		assert(0);
//	}
//
//	// チャンク本体の読み込み
//	assert(format.chunk.size <= sizeof(format.fmt));
//	file.read((char*)&format.fmt, format.chunk.size);
//
//	// Dataチャンクの読み込み
//	ChunkHeader data;
//	file.read((char*)&data, sizeof(data));
//	// JUNKチャンクを検出したら
//	if (strncmp(data.id, "JUNK", 4) == 0)
//	{
//		// 読み取り位置をJUNKチャンクの最後まで飛ばす
//		file.seekg(data.size, std::ios_base::cur);
//		// 再読み込み
//		file.read((char*)&data, sizeof(data));
//	}
//
//	if (strncmp(data.id, "data", 4) != 0)
//	{
//		assert(0);
//	}
//
//	// Dataチャンクのデータ部の読み込み
//	char* pBuffer = new char[data.size];
//	file.read(pBuffer, data.size);
//
//	// ファイルを閉じる
//	file.close();
//
//	// 読み込んだ音声データを返す
//	// 返すための音声データ
//	SoundData soundData = {};
//	soundData.wfex = format.fmt;
//	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
//	soundData.bufferSize = data.size;
//
//	return soundData;
//}

// 音声データの解放関数
void SoundUnload(SoundData* soundData)
{
	// バッファのメモリを解放
	delete[] soundData->pBuffer;
	soundData->pBuffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

// 音声データの再生関数
void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData)
{
	HRESULT result;

	// 波形フォーマットを基にSoundVoiceを生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	// 波形データの再生
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start();
}



// Windowsアプリでのエントリーポイント
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	// リークチェック用のインスタンスを生成
	D3DResourceLeakChecker leakCheck;

	// WinAPIのポインタ
	WinAPIManager* winAPIManager = nullptr;


	// 誰も捕捉しなかった場合に捕捉する関数を登録
	SetUnhandledExceptionFilter(ExportDump);

	// WinAPIの初期化
	winAPIManager = new WinAPIManager();
	winAPIManager->Initialize();

	// DirectX基盤のポインタ
	DirectXBasis* dxBasis = nullptr;
	// DirectX基盤の初期化
	dxBasis = new DirectXBasis();
	dxBasis->Initialize(winAPIManager);

	// SRVマネージャーの初期化
	SrvManager* srvManager = nullptr;
	srvManager = new SrvManager();
	srvManager->Initialize(dxBasis);

	// カメラの初期化
	Camera* camera = new Camera();
	camera->SetRotate({ std::numbers::pi_v<float> / 3.0f,std::numbers::pi_v<float> ,0.0f });
	camera->SetTranslate({ 0.0f,23.0f,10.0f });


	// テクスチャマネージャーの初期化
	TextureManager::GetInstance()->Initialize(dxBasis, srvManager);

	// スプライト共通部のポインタ
	SpriteCommon* spriteManager = nullptr;
	// スプライト共通部の初期化
	spriteManager = new SpriteCommon();
	spriteManager->Initialize(dxBasis);

	// 3dオブジェクト共通部のポインタ
	Object3dCommon* object3dCommon = nullptr;
	// 3dオブジェクト共通部の初期化
	object3dCommon = new Object3dCommon();
	object3dCommon->Initialize(dxBasis);
	object3dCommon->SetDefaultCamera(camera);

	// 3Dモデルマネージャーの初期化
	ModelManager::GetInstance()->Initialize(dxBasis);

	//　音声データ用の変数宣言
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;
	// xAudio2エンジンのインスタンスを生成
	HRESULT soundResult;
	soundResult = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	// マスターボイスを生成
	soundResult = xAudio2->CreateMasteringVoice(&masterVoice);
	// 音声読み込み
	//SoundData soundData1 = SoundLoadWave("resources/Alarm01.wav");

	// 入力クラスのポインタ
	Input* input = nullptr;
	// 入力の初期化
	input = new Input();
	input->Initialize(winAPIManager);

	// Fenceのsignalを待つためのイベントを作成する
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);

	// スプライトの初期化
	std::vector<Sprite*> sprites;
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("resources/circle.png");
	for (uint32_t i = 0; i < 5; ++i)
	{
		Sprite* sprite = new Sprite();
		sprite->Initialize(spriteManager, "resources/uvChecker.png");
		sprites.push_back(sprite);
	}

	// スプライト切り替えフラグ
	bool useMonsterBall = true;

	// objファイルからモデルを読み込む
	ModelManager::GetInstance()->LoadModel("plane.obj");
	ModelManager::GetInstance()->LoadModel("axis.obj");

	// 3dオブジェクトの初期化
	std::vector<Object3d*> object3ds;
	for (uint32_t i = 0; i < 2; ++i)
	{
		Object3d* object3d = new Object3d();
		object3d->Initialize(object3dCommon);
		object3d->SetModel("plane.obj");
		Vector3 pos = object3d->GetTranslate();
		pos.x += (1.0f * (i + 1));
		object3d->SetTranslate(pos);
		object3ds.push_back(object3d);
	}

	object3ds[1]->SetModel("axis.obj");

	// パーティクルの初期化
	const uint32_t kMaxInstanceCount = 10;
	ModelData modelData;
	modelData.vertices.push_back({ .position = {1.0f,1.0f,0.0f,1.0f},.texcoord = {0.0f,0.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {-1.0f,1.0f,0.0f,1.0f},.texcoord = {1.0f,0.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {1.0f,-1.0f,0.0f,1.0f},.texcoord = {0.0f,1.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {1.0f,-1.0f,0.0f,1.0f},.texcoord = {0.0f,1.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {-1.0f,1.0f,0.0f,1.0f},.texcoord = {1.0f,0.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {-1.0f,-1.0f,0.0f,1.0f},.texcoord = {1.0f,1.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.material.textureFilePath = "resources/circle.png";

	// Instancing用のTransformationMatrixを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource =
		dxBasis->CreateBufferResources(sizeof(ParticleForGPU) * kMaxInstanceCount);
	// 書き込むためのアドレスを取得
	ParticleForGPU* instancingData = nullptr;
	instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&instancingData));
	// 単位行列を書き込んでおく
	for (uint32_t index = 0; index < kMaxInstanceCount; ++index)
	{
		instancingData[index].WVP = MakeIdentity4x4();
		instancingData[index].World = MakeIdentity4x4();
		instancingData[index].Color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// ルートシグネチャー
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature;
	// グラフィックスパイプラインステート
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicPipelineState;
	// RootSignatureを作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
	descriptorRangeForInstancing[0].BaseShaderRegister = 0;// 0から始まる
	descriptorRangeForInstancing[0].NumDescriptors = 1; // 数は1つ
	descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// RootParameterを作成
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // CBVを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // VertexShaderで使う
	rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;
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
	hr = dxBasis->GetDevice()->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

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
	D3D12_BLEND_DESC blendDesc{};
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
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	// カリングしない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	// ShaderをCompileする
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = dxBasis->CompileShader(L"resources/shaders/Particle.VS.hlsl",
		L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = dxBasis->CompileShader(L"resources/shaders/Particle.PS.hlsl",
		L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	// 書き込む
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	// 比較関数
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// PSOを生成する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
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
	hr = dxBasis->GetDevice()->CreateGraphicsPipelineState(&graphicPipelineStateDesc,
		IID_PPV_ARGS(&graphicPipelineState));

	assert(SUCCEEDED(hr));

	// SRVの生成
	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = kMaxInstanceCount;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
	D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU = srvManager->GetCPUDescriptorHandle(3);
	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU = srvManager->GetGPUDescriptorHandle(3);
	dxBasis->GetDevice()->CreateShaderResourceView(instancingResource.Get(), &instancingSrvDesc, instancingSrvHandleCPU);

	// WVP用のリソースを作る
	Microsoft::WRL::ComPtr <ID3D12Resource> transformationResource;
	// データを書き込む
	TransformationMatrix* transformationData = nullptr;

	// WVP用のリソースを作る
	transformationResource = dxBasis->CreateBufferResources(sizeof(TransformationMatrix));
	// データを書き込む
	// 書き込むためのアドレスを取得
	transformationResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationData));
	// 単位行列を書き込んでおく
	transformationData->World = MakeIdentity4x4();
	transformationData->WVP = MakeIdentity4x4();

	// VertexResource
	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource;
	// 頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	// 頂点データ
	VertexData* vertexData = nullptr;
	// VertexResourceを生成する
	// 頂点リソースを作る
	vertexResource = dxBasis->CreateBufferResources(sizeof(VertexData) * modelData.vertices.size());

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


	// マテリアルリソース
	Microsoft::WRL::ComPtr <ID3D12Resource> materialResource;
	Material* materialData = nullptr;
	// マテリアルリソースを作る
	materialResource = dxBasis->CreateBufferResources(sizeof(Material) * modelData.vertices.size());
	// マテリアルにデータを書き込む
	// 書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// 白色に設定
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	// Lighting設定
	materialData->enableLighting = false;
	// UVTransform行列を単位行列で初期化
	materialData->uvTransform = MakeIdentity4x4();


	// 音声再生
	//SoundPlayWave(xAudio2.Get(), soundData1);

	// 乱数生成器の初期化
	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	Emitter emitter{};
	emitter.count = 3;
	emitter.frequency = 0.5f;
	emitter.frequencyTime = 0.0f;
	emitter.transform.translate = { 0.0f,0.0f,0.0f };
	emitter.transform.rotate = { 0.0f,0.0f,0.0f };
	emitter.transform.scale = { 1.0f,1.0f,1.0f };

	AccelerationField accelerationField;
	accelerationField.acceleration = { 15.0f,0.0f,0.0f };
	accelerationField.area.min = { -1.0f,-1.0f,-1.0f };
	accelerationField.area.max = { 1.0f,1.0f,1.0f };

	std::list<Particle> particles;

	particles.push_back(MakeNewParticle(randomEngine, emitter.transform.translate));
	particles.push_back(MakeNewParticle(randomEngine, emitter.transform.translate));
	particles.push_back(MakeNewParticle(randomEngine, emitter.transform.translate));


	const float kDeltaTime = 1.0f / 60.0f;


	// メインループ
	MSG msg{};
	// ウインドウの×ボタンが押されるまでループ
	while (true)
	{

		// Windowsのメッセージ処理
		if (winAPIManager->ProcessMessage())
		{
			// ゲームループを抜ける
			break;
		}

#ifdef USE_IMGUI
		// ImGuiに通知
		/*ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();*/
#endif // USE_IMGUI


		Transform uvTransformSprite
		{
			{1.0f,1.0f,1.0f},
			{0.0f,0.0f,0.0f},
			{0.0f,0.0f,0.0f}
		};

		// 板ポリを回転させる変数
		Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
		Matrix4x4 billboardMatrix = Multiply(backToFrontMatrix, camera->GetWorldMatrix());
		billboardMatrix.m[3][0] = 0.0f;
		billboardMatrix.m[3][1] = 0.0f;
		billboardMatrix.m[3][2] = 0.0f;

		uint32_t numInstance = 0;
		for (std::list<Particle>::iterator particleIterator = particles.begin();
			particleIterator != particles.end(); )
		{
			if ((*particleIterator).lifeTime <= (*particleIterator).currentTime)
			{
				// 生存時間を過ぎていたら表示しない
				particleIterator = particles.erase(particleIterator);
			}

			// Fieldの範囲内のParticleにはAccelerationを適用
			if (IsCollision(accelerationField.area, (*particleIterator).transform.translate))
			{
				Vector3 accelDelta =
				{
		accelerationField.acceleration.x * kDeltaTime,
		accelerationField.acceleration.y * kDeltaTime,
		accelerationField.acceleration.z * kDeltaTime
				};
				(*particleIterator).velocity = Vector3Add((*particleIterator).velocity, accelDelta);
			}

			(*particleIterator).transform.translate.y += (*particleIterator).velocity.y * kDeltaTime;
			(*particleIterator).currentTime += kDeltaTime;
			float alpha = 1.0f - ((*particleIterator).currentTime / (*particleIterator).lifeTime);

			if (numInstance < kMaxInstanceCount)
			{
				Matrix4x4 translateMatrix = MakeTranslateMatrix((*particleIterator).transform.translate);
				Matrix4x4 scaleMatrix = MakeScaleMatrix((*particleIterator).transform.scale);
				Matrix4x4 worldMatrix = Multiply(Multiply(scaleMatrix, billboardMatrix), translateMatrix);
				Matrix4x4 worldViewProjectionMatrix;
				worldViewProjectionMatrix = Multiply(worldMatrix, camera->GetViewProjectionMatrix());
				instancingData[numInstance].WVP = worldViewProjectionMatrix;
				instancingData[numInstance].World = worldMatrix;
				instancingData[numInstance].Color = (*particleIterator).color;
				instancingData[numInstance].Color.w = alpha;
				++numInstance;
			}

			++particleIterator;

		}

		// Emitterの更新処理
		emitter.frequencyTime += kDeltaTime;
		if (emitter.frequency <= emitter.frequencyTime)
		{
			particles.splice(particles.end(), Emit(emitter, randomEngine));
			emitter.frequencyTime -= emitter.frequency;
		}

		// UVTransform用の行列を作成する
	/*Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
	uvTransformMatrix = Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
	uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
	materialData->uvTransform = uvTransformMatrix;*/


	// 入力の更新処理
		input->Update();

		// 数字の0キーが押されていたら
		if (input->TriggerKey(DIK_0))
		{
			OutputDebugStringA("Hit 0\n");
		}

		//#ifdef USE_IMGUI
		//		// 開発用UIの処理
		//		ImGui::ShowDemoWindow();
		//
		//		ImGui::Checkbox("useMonsterBall", &useMonsterBall);
		//
		//		ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
		//		ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
		//		ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);
		//
		//		ImGui::Separator();
		//		ImGui::Text("Camera");
		//		ImGui::DragFloat3("Camera Pos", &cameraTransform.translate.x, 0.1f);
		//		ImGui::SliderAngle("Camera Rot X", &cameraTransform.rotate.x);
		//		ImGui::SliderAngle("Camera Rot Y", &cameraTransform.rotate.y);
		//		ImGui::SliderAngle("Camera Rot Z", &cameraTransform.rotate.z);
		//
		//		ImGui::Separator();
		//		ImGui::Text("Model Rotation");
		//		ImGui::SliderAngle("Rot X", &transform.rotate.x, -180.0f, 180.0f);
		//		ImGui::SliderAngle("Rot Y", &transform.rotate.y, -180.0f, 180.0f);
		//		ImGui::SliderAngle("Rot Z", &transform.rotate.z, -180.0f, 180.0f);
		//
		//		// ImGuiの内部コマンドを生成する
		//		ImGui::Render();
		//#endif // USE_IMGUI


		// カメラの更新
		camera->Update();

		// 3Dモデルの更新処理
		for (Object3d* object3d : object3ds)
		{
			object3d->Update();

		}

		// スプライトの更新処理
		for (Sprite* sprite : sprites)
		{
			sprite->Update();
		}

		// 描画処理
		// 描画前処理
		dxBasis->PreDraw();
		srvManager->PreDraw();

		// 3dモデルの描画準備
		object3dCommon->DrawSettingCommon();

		// Spriteの描画準備
		spriteManager->DrawSettingCommon();

		// 3dモデルの描画
		for (Object3d* object3d : object3ds)
		{
			//object3d->Draw();

		}

		// パーティクルの描画
		// // RootSignatureを設定
		dxBasis->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());

		// PSOを設定
		dxBasis->GetCommandList()->SetPipelineState(graphicPipelineState.Get());

		// 形状を設定
		dxBasis->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// VBVを設定
		dxBasis->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
		dxBasis->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSRVHandleGPU(modelData.material.textureFilePath));
		// マテリアルCBufferの場所を設定
		dxBasis->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
		// wvp用のCBufferの場所を設定
		dxBasis->GetCommandList()->SetGraphicsRootConstantBufferView(3, transformationResource->GetGPUVirtualAddress());
		// SRVのDescriptorTableの先頭を設定
		dxBasis->GetCommandList()->SetGraphicsRootDescriptorTable(1, instancingSrvHandleGPU);
		// 描画
		dxBasis->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), numInstance, 0, 0);

		// Spriteの描画
		for (Sprite* sprite : sprites)
		{
			//sprite->Draw();
		}


#ifdef USE_IMGUI

		// ImGuiを描画
		//ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxBasis->GetCommandList());

#endif

		// 描画後処理
		dxBasis->PostDraw();

		TextureManager::GetInstance()->ReleaseIntermediateResources();

	}

#ifdef USE_IMGUI
	// ImGui終了処理
	/*ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();*/
#endif



	// 解放処理
	CloseHandle(fenceEvent);

	// テクスチャマネージャーの終了
	TextureManager::GetInstance()->Finalize();
	// 3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();

	delete input;
	for (Sprite* sprite : sprites)
	{
		delete sprite;
	}
	delete spriteManager;

	for (Object3d* object3d : object3ds)
	{
		delete object3d;
	}

	delete object3dCommon;
	delete camera;
	delete srvManager;
	delete dxBasis;

	// xAudio2解放
	xAudio2.Reset();
	// 音声データ解放
	//SoundUnload(&soundData1);

	// WinAPIの終了処理
	winAPIManager->Finalize();

	// WinAPIの解放処理
	delete winAPIManager;

	return 0;
}


