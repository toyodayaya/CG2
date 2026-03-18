#pragma warning(push)
// C4023の警告を見なかったことにする
#pragma warning(disable:4023)
#include "WinAPIManager.h"
#include <string>
#include <format>
#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")
#include <strsafe.h>
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include <vector>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
#define _USE_MATH_DEFINES
#include <math.h>
#include <fstream>
#include <sstream>
#include <wrl.h>
#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")
#include "Input.h"
#include "DirectXBasis.h"
#include <dxcapi.h>
#include <Logger.h>
#include <StringUtility.h>

#pragma warning(pop)

// 構造体の宣言
struct Vector2
{
	float x, y;
};

struct Vector3
{
	float x, y, z;
};

struct Vector4
{
	float x, y, z, w;
};

struct Matrix3x3
{
	float m[3][3];
};

struct Matrix4x4
{
	float m[4][4];
};

struct Transform
{
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
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

struct TransformationMatrix
{
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct DirectionalLight
{
	Vector4 color;
	Vector3 direction;
	float intensity;
};

struct MaterialData
{
	std::string textureFilePath;
};


struct ModelData
{
	std::vector<VertexData> vertices;
	MaterialData material;
};

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

// リークチェック
struct D3DResourceLeakChecker
{
	~D3DResourceLeakChecker()
	{
		// リソースリークチェック
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}

	}
};

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

//// CompileShader関数
//IDxcBlob* CompileShader
//(
//	// CompileするShaderファイルへのパス
//	const std::wstring& filePath,
//	// Complierに使用するProfile
//	const wchar_t* profile,
//	// 初期化で生成したものを3つ
//	IDxcUtils* dxcUtils,
//	IDxcCompiler3* dxcCompiler,
//	IDxcIncludeHandler* includeHandler
//)
//{
//	// hlslファイルを読み込む
//
//	// シェーダーをコンパイルする旨をログに出す
//	Log(ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));
//	// hlslファイルを読む
//	IDxcBlobEncoding* shaderSource = nullptr;
//	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
//	// 読めなかったら止める
//	assert(SUCCEEDED(hr));
//	// 読み込んだファイルの内容を設定する
//	DxcBuffer shaderSourceBuffer;
//	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
//	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
//	shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF-8であることを通知
//
//	// Compileする
//	LPCWSTR arguments[] =
//	{
//		filePath.c_str(), // コンパイルするhlslファイル名
//		L"-E", L"main", // エントリーポイントの指定
//		L"-T", profile, // プロファイルの設定
//		L"-Zi", L"-Qembed_debug", // デバッグ用の情報を埋め込む
//		L"-Od", // 最適化をしない
//		L"-Zpr", // メモリレイアウトは行優先
//	};
//
//	// 実際にShaderをCompileする
//	IDxcResult* shaderResult = nullptr;
//	hr = dxcCompiler->Compile(
//		&shaderSourceBuffer,  // 読み込んだファイル
//		arguments, // コンパイルオプション
//		_countof(arguments),  // コンパイルオプションの数
//		includeHandler,  // includeを含んだ諸々
//		IID_PPV_ARGS(&shaderResult) // コンパイル結果
//	);
//
//	// dxcが起動できないなどの致命的な状況
//	assert(SUCCEEDED(hr));
//
//
//	// 警告・エラーが出ていないか確認する
//	// 出ていたらログに出して止める
//	IDxcBlobUtf8* shaderError = nullptr;
//	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
//	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
//	{
//		Log(shaderError->GetStringPointer());
//		// 警告・エラーが出ているので止める
//		assert(false);
//	}
//
//	// Compile結果を受け取って返す
//	// コンパイル結果から実行用のバイナリ部分を取得
//	Microsoft::WRL::ComPtr <IDxcBlob> shaderBlob = nullptr;
//	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
//	assert(SUCCEEDED(hr));
//	// 成功したログを出す
//	Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));
//	// 実行用のバイナリを返却
//	return shaderBlob.Get();
//}
//
//
//
//// BufferResourcesを作る関数
//Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResources(const Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes)
//{
//	// VertexResourceを生成する
//	// 頂点リソース用のヒープの設定
//	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
//	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
//	// 頂点リソースの設定
//	D3D12_RESOURCE_DESC vertexResourceDesc{};
//	// バッファリソース
//	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
//	vertexResourceDesc.Width = sizeInBytes;
//	// バッファの場合は以下を1にする
//	vertexResourceDesc.Height = 1;
//	vertexResourceDesc.DepthOrArraySize = 1;
//	vertexResourceDesc.MipLevels = 1;
//	vertexResourceDesc.SampleDesc.Count = 1;
//	// バッファの場合は以下の設定で行う
//	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
//	// 頂点リソースを作る
//	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
//	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
//		&vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
//		IID_PPV_ARGS(&resource));
//	assert(SUCCEEDED(hr));
//	return resource;
//
//}
//
//// LoadTexture関数
//DirectX::ScratchImage LoadTexture(const std::string& filePath)
//{
//	// テクスチャファイルを読み込んでプログラムで扱えるようにする
//	DirectX::ScratchImage image{};
//	std::wstring filePathW = ConvertString(filePath);
//	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
//	assert(SUCCEEDED(hr));
//
//	// ミニマップの作成
//	DirectX::ScratchImage mipImages{};
//	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
//	assert(SUCCEEDED(hr));
//
//	// ミニマップ付きのデータを返す
//	return mipImages;
//}
//
//// TextureResources関数
//Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device> device, const DirectX::TexMetadata& metaData)
//{
//	// metaDataを基にResourcesの設定
//	D3D12_RESOURCE_DESC resourceDesc{};
//	resourceDesc.Width = UINT(metaData.width); // Textureの幅
//	resourceDesc.Height = UINT(metaData.height); // Textureの高さ
//	resourceDesc.MipLevels = UINT16(metaData.mipLevels); // mipmapの数
//	resourceDesc.DepthOrArraySize = UINT16(metaData.arraySize); //奥行き
//	resourceDesc.Format = metaData.format;
//	resourceDesc.SampleDesc.Count = 1; // サンプリングの数
//	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metaData.dimension); // Textureの次元数
//
//	// 利用するHeapの設定
//	D3D12_HEAP_PROPERTIES heapProperties{};
//	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
//
//	// Resourcesの生成
//	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
//	HRESULT hr = device->CreateCommittedResource(
//		&heapProperties, // Heapの設定
//		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定(特になし)
//		&resourceDesc, // Resourcesの設定
//		D3D12_RESOURCE_STATE_COPY_DEST, // データ転送される設定
//		nullptr, // Clear最適値
//		IID_PPV_ARGS(&resource) // 作成するResourcesポインタへのポインタ
//	);
//
//	assert(SUCCEEDED(hr));
//	return resource;
//}
//
//// UploadTextureData関数
//[[nodiscard]]
//Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource> texture,
//	const DirectX::ScratchImage& mipImages, const Microsoft::WRL::ComPtr<ID3D12Device> device,
//	const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
//{
//	std::vector<D3D12_SUBRESOURCE_DATA> subreSources;
//	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subreSources);
//	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subreSources.size()));
//	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource = CreateBufferResources(device, intermediateSize);
//	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subreSources.size()), subreSources.data());
//	// Textureへの転送後は利用できるようにResourceStateを変更する
//	D3D12_RESOURCE_BARRIER barrier{};
//	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
//	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
//	barrier.Transition.pResource = texture.Get();
//	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
//	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
//	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
//	commandList->ResourceBarrier(1, &barrier);
//	return intermediateResource;
//}
//
//// DepthStencilTextureの作成関数
//Microsoft::WRL::ComPtr <ID3D12Resource> CreateDepthStancilTextureResource(const Microsoft::WRL::ComPtr <ID3D12Device> device,
//	int32_t width, int32_t height)
//{
//	// 生成するResourceの設定
//	D3D12_RESOURCE_DESC resourceDesc{};
//	resourceDesc.Width = width; // Textureの幅
//	resourceDesc.Height = height; // Textureの高さ
//	resourceDesc.MipLevels = 1; // mipmapの数
//	resourceDesc.DepthOrArraySize = 1; // 奥行き
//	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 利用可能なフォーマット
//	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント
//	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元
//	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知
//
//	// 利用するHeapの設定
//	D3D12_HEAP_PROPERTIES heapProperties{};
//	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る
//
//	// 深度値のクリア設定
//	D3D12_CLEAR_VALUE depthClearValue{};
//	depthClearValue.DepthStencil.Depth = 1.0f; // 最大値でクリア
//	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Format(Resourceと合わせる)
//
//	// Resourceの生成
//	Microsoft::WRL::ComPtr <ID3D12Resource> resource = nullptr;
//	HRESULT hr = device->CreateCommittedResource(
//		&heapProperties, // Heapの設定
//		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定(特になし)
//		&resourceDesc, // Resourceの設定
//		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態に設定
//		&depthClearValue, // Clear最適値
//		IID_PPV_ARGS(&resource) // 作成するResourcesポインタへのポインタ
//	);
//
//	assert(SUCCEEDED(hr));
//	return resource;
//}
//
//
//// mtlファイルを読む関数
//MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
//{
//	// 必要な変数を宣言する
//	MaterialData materialData; // 構築するMaterialData
//	std::string line; // ファイルから読んだ1行を格納
//
//	// ファイルを開く
//	std::ifstream file(directoryPath + "/" + filename);
//	assert(file.is_open()); // 開けなかったら止める
//
//	// 実際にファイルを読み込みMaterialDataを構築する
//	while (std::getline(file, line))
//	{
//		std::string identifier;
//		std::istringstream s(line);
//		s >> identifier;
//
//		// identifierに応じた処理
//		if (identifier == "map_Kd")
//		{
//			std::string textureFilename;
//			s >> textureFilename;
//			// 連結してファイルパスにする
//			materialData.textureFilePath = directoryPath + "/" + textureFilename;
//		}
//	}
//
//	// MaterialDataを返す
//	return materialData;
//}
//
//
//
//// Objファイルを読み込む関数
//ModelData LoadObjFile(const std::string& directoryPath, const std::string& fileName)
//{
//	// 必要な変数を宣言する
//	ModelData modelData; // 構築するModelData
//	std::vector<Vector4> positions; // 位置
//	std::vector<Vector3> normals; // 法線
//	std::vector<Vector2> texcoords; // テクスチャ座標
//	std::string line; // ファイルから読んだ1行を格納する
//
//	// ファイルを開く
//	std::ifstream file(directoryPath + "/" + fileName);
//	assert(file.is_open()); // 開けなかったら止める
//
//	// ファイルを読み込みModelDataを構築
//	while (std::getline(file, line))
//	{
//		std::string identifier;
//		std::istringstream s(line);
//		s >> identifier; // 先頭の識別子を読む
//
//		// identifierに応じた処理
//		// 頂点情報を読む
//		if (identifier == "v")
//		{
//			Vector4 position;
//			s >> position.x >> position.y >> position.z;
//			position.x *= -1.0f;
//			position.w = 1.0f;
//			positions.push_back(position);
//		}
//		else if (identifier == "vt")
//		{
//			Vector2 texcoord;
//			s >> texcoord.x >> texcoord.y;
//			texcoord.y = 1.0f - texcoord.y;
//			texcoords.push_back(texcoord);
//		}
//		else if (identifier == "vn")
//		{
//			Vector3 normal;
//			s >> normal.x >> normal.y >> normal.z;
//			normal.x *= -1.0f;
//			normals.push_back(normal);
//		}
//		else if (identifier == "f")
//		{
//			VertexData triangle[3];
//			// 三角形を作る
//			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex)
//			{
//				std::string vertexDefinition;
//				s >> vertexDefinition;
//				// 頂点の要素へのIndexを取得
//				std::istringstream v(vertexDefinition);
//				uint32_t elementIndices[3];
//				for (int32_t element = 0; element < 3; ++element)
//				{
//					std::string index;
//					std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
//					elementIndices[element] = std::stoi(index);
//				}
//				// 要素のインデックスから実際の要素の値を取得して頂点を構築する
//				Vector4 position = positions[elementIndices[0] - 1];
//				Vector2 texcoord = texcoords[elementIndices[1] - 1];
//				Vector3 normal = normals[elementIndices[2] - 1];
//				triangle[faceVertex] = { position,texcoord,normal };
//			}
//			modelData.vertices.push_back(triangle[2]);
//			modelData.vertices.push_back(triangle[1]);
//			modelData.vertices.push_back(triangle[0]);
//		}
//		else if (identifier == "mtllib")
//		{
//			// materialTemplateLibraryファイルの名前を取得する
//			std::string materialFileName;
//			s >> materialFileName;
//			// ディレクトリ名とファイル名を渡す
//			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFileName);
//		}
//	}
//
//	// ModelDataを返す
//	return modelData;
//}
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
//
//// 音声データの解放関数
//void SoundUnload(SoundData* soundData)
//{
//	// バッファのメモリを解放
//	delete[] soundData->pBuffer;
//	soundData->pBuffer = 0;
//	soundData->bufferSize = 0;
//	soundData->wfex = {};
//}
//
//// 音声データの再生関数
//void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData)
//{
//	HRESULT result;
//
//	// 波形フォーマットを基にSoundVoiceを生成
//	IXAudio2SourceVoice* pSourceVoice = nullptr;
//	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
//	assert(SUCCEEDED(result));
//
//	// 再生する波形データの設定
//	XAUDIO2_BUFFER buf{};
//	buf.pAudioData = soundData.pBuffer;
//	buf.AudioBytes = soundData.bufferSize;
//	buf.Flags = XAUDIO2_END_OF_STREAM;
//
//	// 波形データの再生
//	result = pSourceVoice->SubmitSourceBuffer(&buf);
//	result = pSourceVoice->Start();
//}

// 単位行列の作成
Matrix4x4 MakeIdentity4x4()
{
	Matrix4x4 ret;
	ret.m[0][0] = 1.0f; ret.m[0][1] = 0.0f; ret.m[0][2] = 0.0f; ret.m[0][3] = 0.0f;
	ret.m[1][0] = 0.0f; ret.m[1][1] = 1.0f; ret.m[1][2] = 0.0f; ret.m[1][3] = 0.0f;
	ret.m[2][0] = 0.0f; ret.m[2][1] = 0.0f; ret.m[2][2] = 1.0f; ret.m[2][3] = 0.0f;
	ret.m[3][0] = 0.0f; ret.m[3][1] = 0.0f; ret.m[3][2] = 0.0f; ret.m[3][3] = 1.0f;

	return ret;
}

// 回転行列
Matrix4x4 MakeRotateXMatrix(float radian)
{
	Matrix4x4 ret;
	ret.m[0][0] = 1.0f; ret.m[0][1] = 0.0f; ret.m[0][2] = 0.0f; ret.m[0][3] = 0.0f;
	ret.m[1][0] = 0.0f; ret.m[1][1] = std::cos(radian); ret.m[1][2] = std::sin(radian); ret.m[1][3] = 0.0f;
	ret.m[2][0] = 0.0f; ret.m[2][1] = -std::sin(radian); ret.m[2][2] = std::cos(radian); ret.m[2][3] = 0.0f;
	ret.m[3][0] = 0.0f; ret.m[3][1] = 0.0f; ret.m[3][2] = 0.0f; ret.m[3][3] = 1.0f;
	return ret;
}

Matrix4x4 MakeRotateYMatrix(float radian)
{
	Matrix4x4 ret;
	ret.m[0][0] = std::cos(radian); ret.m[0][1] = 0.0f; ret.m[0][2] = -std::sin(radian); ret.m[0][3] = 0.0f;
	ret.m[1][0] = 0.0f; ret.m[1][1] = 1.0f; ret.m[1][2] = 0.0f; ret.m[1][3] = 0.0f;
	ret.m[2][0] = std::sin(radian); ret.m[2][1] = 0.0f; ret.m[2][2] = std::cos(radian); ret.m[2][3] = 0.0f;
	ret.m[3][0] = 0.0f; ret.m[3][1] = 0.0f; ret.m[3][2] = 0.0f; ret.m[3][3] = 1.0f;
	return ret;
}

Matrix4x4 MakeRotateZMatrix(float radian)
{
	Matrix4x4 ret;
	ret.m[0][0] = std::cos(radian); ret.m[0][1] = std::sin(radian); ret.m[0][2] = 0.0f; ret.m[0][3] = 0.0f;
	ret.m[1][0] = -std::sin(radian); ret.m[1][1] = std::cos(radian); ret.m[1][2] = 0.0f; ret.m[1][3] = 0.0f;
	ret.m[2][0] = 0.0f; ret.m[2][1] = 0.0f; ret.m[2][2] = 1.0f; ret.m[2][3] = 0.0f;
	ret.m[3][0] = 0.0f; ret.m[3][1] = 0.0f; ret.m[3][2] = 0.0f; ret.m[3][3] = 1.0f;
	return ret;
}


Matrix4x4 MakeScaleMatrix(const Vector3& scale)
{
	Matrix4x4 m;
	m.m[0][0] = scale.x; m.m[0][1] = 0.0f; m.m[0][2] = 0.0f; m.m[0][3] = 0.0f;
	m.m[1][0] = 0.0f; m.m[1][1] = scale.y; m.m[1][2] = 0.0f; m.m[1][3] = 0.0f;
	m.m[2][0] = 0.0f; m.m[2][1] = 0.0f; m.m[2][2] = scale.z; m.m[2][3] = 0.0f;
	m.m[3][0] = 0.0f; m.m[3][1] = 0.0f; m.m[3][2] = 0.0f; m.m[3][3] = 1.0f;
	return m;
}


Matrix4x4 MakeTranslateMatrix(const Vector3& translate)
{
	Matrix4x4 m;
	m.m[0][0] = 1.0f; m.m[0][1] = 0.0f; m.m[0][2] = 0.0f; m.m[0][3] = 0.0f;
	m.m[1][0] = 0.0f; m.m[1][1] = 1.0f; m.m[1][2] = 0.0f; m.m[1][3] = 0.0f;
	m.m[2][0] = 0.0f; m.m[2][1] = 0.0f; m.m[2][2] = 1.0f; m.m[2][3] = 0.0f;
	m.m[3][0] = translate.x; m.m[3][1] = translate.y; m.m[3][2] = translate.z; m.m[3][3] = 0.0f;

	return m;
}

// 行列の積
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2)
{
	Matrix4x4 ret;
	ret.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] + m1.m[0][2] * m2.m[2][0] + m1.m[0][3] * m2.m[3][0];
	ret.m[0][1] = m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] + m1.m[0][2] * m2.m[2][1] + m1.m[0][3] * m2.m[3][1];
	ret.m[0][2] = m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] + m1.m[0][2] * m2.m[2][2] + m1.m[0][3] * m2.m[3][2];
	ret.m[0][3] = m1.m[0][0] * m2.m[0][3] + m1.m[0][1] * m2.m[1][3] + m1.m[0][2] * m2.m[2][3] + m1.m[0][3] * m2.m[3][3];
	ret.m[1][0] = m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] + m1.m[1][2] * m2.m[2][0] + m1.m[1][3] * m2.m[3][0];
	ret.m[1][1] = m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] + m1.m[1][2] * m2.m[2][1] + m1.m[1][3] * m2.m[3][1];
	ret.m[1][2] = m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] + m1.m[1][2] * m2.m[2][2] + m1.m[1][3] * m2.m[3][2];
	ret.m[1][3] = m1.m[1][0] * m2.m[0][3] + m1.m[1][1] * m2.m[1][3] + m1.m[1][2] * m2.m[2][3] + m1.m[1][3] * m2.m[3][3];
	ret.m[2][0] = m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] + m1.m[2][2] * m2.m[2][0] + m1.m[2][3] * m2.m[3][0];
	ret.m[2][1] = m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] + m1.m[2][2] * m2.m[2][1] + m1.m[2][3] * m2.m[3][1];
	ret.m[2][2] = m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] + m1.m[2][2] * m2.m[2][2] + m1.m[2][3] * m2.m[3][2];
	ret.m[2][3] = m1.m[2][0] * m2.m[0][3] + m1.m[2][1] * m2.m[1][3] + m1.m[2][2] * m2.m[2][3] + m1.m[2][3] * m2.m[3][3];
	ret.m[3][0] = m1.m[3][0] * m2.m[0][0] + m1.m[3][1] * m2.m[1][0] + m1.m[3][2] * m2.m[2][0] + m1.m[3][3] * m2.m[3][0];
	ret.m[3][1] = m1.m[3][0] * m2.m[0][1] + m1.m[3][1] * m2.m[1][1] + m1.m[3][2] * m2.m[2][1] + m1.m[3][3] * m2.m[3][1];
	ret.m[3][2] = m1.m[3][0] * m2.m[0][2] + m1.m[3][1] * m2.m[1][2] + m1.m[3][2] * m2.m[2][2] + m1.m[3][3] * m2.m[3][2];
	ret.m[3][3] = m1.m[3][0] * m2.m[0][3] + m1.m[3][1] * m2.m[1][3] + m1.m[3][2] * m2.m[2][3] + m1.m[3][3] * m2.m[3][3];
	return ret;
}

// アフィン変換行列
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate)
{
	Matrix4x4 rotateX = MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateY = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZ = MakeRotateZMatrix(rotate.z);
	Matrix4x4 rotateXYZ = Multiply(rotateX, Multiply(rotateY, rotateZ));

	Matrix4x4 ret;
	ret.m[0][0] = scale.x * rotateXYZ.m[0][0]; ret.m[0][1] = scale.x * rotateXYZ.m[0][1]; ret.m[0][2] = scale.x * rotateXYZ.m[0][2]; ret.m[0][3] = 0.0f;
	ret.m[1][0] = scale.y * rotateXYZ.m[1][0]; ret.m[1][1] = scale.y * rotateXYZ.m[1][1]; ret.m[1][2] = scale.y * rotateXYZ.m[1][2]; ret.m[1][3] = 0.0f;
	ret.m[2][0] = scale.z * rotateXYZ.m[2][0]; ret.m[2][1] = scale.z * rotateXYZ.m[2][1]; ret.m[2][2] = scale.z * rotateXYZ.m[2][2]; ret.m[2][3] = 0.0f;
	ret.m[3][0] = translate.x; ret.m[3][1] = translate.y; ret.m[3][2] = translate.z; ret.m[3][3] = 1.0f;

	return ret;

}

// 透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip)
{
	Matrix4x4 ret;
	float cot = 1.0f / tanf(fovY / 2.0f);
	ret.m[0][0] = (1.0f / aspectRatio) * cot; ret.m[0][1] = 0.0f; ret.m[0][2] = 0.0f; ret.m[0][3] = 0.0f;
	ret.m[1][0] = 0.0f; ret.m[1][1] = cot; ret.m[1][2] = 0.0f; ret.m[1][3] = 0.0f;
	ret.m[2][0] = 0.0f; ret.m[2][1] = 0.0f; ret.m[2][2] = farClip / (farClip - nearClip); ret.m[2][3] = 1.0f;
	ret.m[3][0] = 0.0f; ret.m[3][1] = 0.0f; ret.m[3][2] = (-1.0f * nearClip) * farClip / (farClip - nearClip); ret.m[3][3] = 0.0f;

	return ret;
}

// 正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip)
{
	Matrix4x4 ret;
	ret.m[0][0] = 2.0f / (right - left); ret.m[0][1] = 0.0f; ret.m[0][2] = 0.0f; ret.m[0][3] = 0.0f;
	ret.m[1][0] = 0.0f; ret.m[1][1] = 2.0f / (top - bottom); ret.m[1][2] = 0.0f; ret.m[1][3] = 0.0f;
	ret.m[2][0] = 0.0f; ret.m[2][1] = 0.0f; ret.m[2][2] = 1.0f / (farClip - nearClip); ret.m[2][3] = 0.0f;
	ret.m[3][0] = (left + right) / (left - right); ret.m[3][1] = (top + bottom) / (bottom - top); ret.m[3][2] = nearClip / (nearClip - farClip); ret.m[3][3] = 1.0f;
	return ret;
}

// 逆行列
Matrix4x4 Inverse(const Matrix4x4& m)
{
	Matrix4x4 ret;
	float a = (m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3]) + (m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1]) + (m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2])
		- (m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1]) - (m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3]) - (m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2])
		- (m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3]) - (m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1]) - (m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2])
		+ (m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1]) + (m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3]) + (m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2])
		+ (m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3]) + (m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1]) + (m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2])
		- (m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1]) - (m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3]) - (m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2])
		- (m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0]) - (m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0]) - (m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0])
		+ (m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0]) + (m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0]) + (m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0]);

	ret.m[0][0] = (1.0f / a) * ((m.m[1][1] * m.m[2][2] * m.m[3][3]) + (m.m[1][2] * m.m[2][3] * m.m[3][1]) + (m.m[1][3] * m.m[2][1] * m.m[3][2])
		- (m.m[1][3] * m.m[2][2] * m.m[3][1]) - (m.m[1][2] * m.m[2][1] * m.m[3][3]) - (m.m[1][1] * m.m[2][3] * m.m[3][2]));

	ret.m[0][1] = (1.0f / a) * ((-1.0f * (m.m[0][1] * m.m[2][2] * m.m[3][3])) - (m.m[0][2] * m.m[2][3] * m.m[3][1]) - (m.m[0][3] * m.m[2][1] * m.m[3][2])
		+ (m.m[0][3] * m.m[2][2] * m.m[3][1]) + (m.m[0][2] * m.m[2][1] * m.m[3][3]) + (m.m[0][1] * m.m[2][3] * m.m[3][2]));

	ret.m[0][2] = (1.0f / a) * ((m.m[0][1] * m.m[1][2] * m.m[3][3]) + (m.m[0][2] * m.m[1][3] * m.m[3][1]) + (m.m[0][3] * m.m[1][1] * m.m[3][2])
		- (m.m[0][3] * m.m[1][2] * m.m[3][1]) - (m.m[0][2] * m.m[1][1] * m.m[3][3]) - (m.m[0][1] * m.m[1][3] * m.m[3][2]));

	ret.m[0][3] = (1.0f / a) * ((-1.0f * (m.m[0][1] * m.m[1][2] * m.m[2][3])) - (m.m[0][2] * m.m[1][3] * m.m[2][1]) - (m.m[0][3] * m.m[1][1] * m.m[2][2])
		+ (m.m[0][3] * m.m[1][2] * m.m[2][1]) + (m.m[0][2] * m.m[1][1] * m.m[2][3]) + (m.m[0][1] * m.m[1][3] * m.m[2][2]));

	ret.m[1][0] = (1.0f / a) * (-1.0f * ((m.m[1][0] * m.m[2][2] * m.m[3][3])) - (m.m[1][2] * m.m[2][3] * m.m[3][0]) - (m.m[1][3] * m.m[2][0] * m.m[3][2])
		+ (m.m[1][3] * m.m[2][2] * m.m[3][0]) + (m.m[1][2] * m.m[2][0] * m.m[3][3]) + (m.m[1][0] * m.m[2][3] * m.m[3][2]));

	ret.m[1][1] = (1.0f / a) * ((m.m[0][0] * m.m[2][2] * m.m[3][3]) + (m.m[0][2] * m.m[2][3] * m.m[3][0]) + (m.m[0][3] * m.m[2][0] * m.m[3][2])
		- (m.m[0][3] * m.m[2][2] * m.m[3][0]) - (m.m[0][2] * m.m[2][1] * m.m[3][3]) - (m.m[0][0] * m.m[2][3] * m.m[3][2]));

	ret.m[1][2] = (1.0f / a) * (-1.0f * ((m.m[0][0] * m.m[1][2] * m.m[3][3])) - (m.m[0][2] * m.m[1][3] * m.m[3][0]) - (m.m[0][3] * m.m[1][0] * m.m[3][2])
		+ (m.m[0][3] * m.m[1][2] * m.m[3][0]) + (m.m[0][2] * m.m[1][0] * m.m[3][3]) + (m.m[0][1] * m.m[1][3] * m.m[3][2]));

	ret.m[1][3] = (1.0f / a) * ((m.m[0][0] * m.m[1][2] * m.m[2][3]) + (m.m[0][2] * m.m[1][3] * m.m[2][0]) + (m.m[0][3] * m.m[1][0] * m.m[2][2])
		- (m.m[0][3] * m.m[1][2] * m.m[2][0]) - (m.m[0][2] * m.m[1][0] * m.m[2][3]) - (m.m[0][0] * m.m[1][3] * m.m[2][2]));

	ret.m[2][0] = (1.0f / a) * ((m.m[1][0] * m.m[2][1] * m.m[3][3]) + (m.m[1][1] * m.m[2][3] * m.m[3][0]) + (m.m[1][3] * m.m[2][0] * m.m[3][1])
		- (m.m[1][3] * m.m[2][1] * m.m[3][0]) - (m.m[1][1] * m.m[2][0] * m.m[3][3]) - (m.m[1][0] * m.m[2][3] * m.m[3][1]));

	ret.m[2][1] = (1.0f / a) * ((-1.0f * (m.m[0][0] * m.m[2][1] * m.m[3][3])) - (m.m[0][1] * m.m[2][3] * m.m[3][0]) - (m.m[0][3] * m.m[2][0] * m.m[3][1])
		+ (m.m[0][3] * m.m[2][1] * m.m[3][0]) + (m.m[0][1] * m.m[2][0] * m.m[3][3]) + (m.m[0][0] * m.m[2][3] * m.m[3][1]));

	ret.m[2][2] = (1.0f / a) * ((m.m[0][0] * m.m[1][1] * m.m[3][3]) + (m.m[0][1] * m.m[1][3] * m.m[3][0]) + (m.m[0][3] * m.m[1][0] * m.m[3][1])
		- (m.m[0][3] * m.m[1][1] * m.m[3][0]) - (m.m[0][1] * m.m[1][0] * m.m[3][3]) - (m.m[0][0] * m.m[1][3] * m.m[3][1]));

	ret.m[2][3] = (1.0f / a) * ((-1.0f * (m.m[0][0] * m.m[1][1] * m.m[2][3])) - (m.m[0][1] * m.m[1][3] * m.m[2][0]) - (m.m[0][3] * m.m[1][0] * m.m[2][1])
		+ (m.m[0][3] * m.m[1][1] * m.m[2][0]) + (m.m[0][1] * m.m[1][0] * m.m[2][3]) + (m.m[0][0] * m.m[1][3] * m.m[2][1]));

	ret.m[3][0] = (1.0f / a) * (-1.0f * ((m.m[1][0] * m.m[2][1] * m.m[3][2])) - (m.m[1][1] * m.m[2][2] * m.m[3][0]) - (m.m[1][2] * m.m[2][0] * m.m[3][1])
		+ (m.m[1][2] * m.m[2][1] * m.m[3][0]) + (m.m[1][1] * m.m[2][0] * m.m[3][2]) + (m.m[1][1] * m.m[2][2] * m.m[2][1]));

	ret.m[3][1] = (1.0f / a) * ((m.m[0][0] * m.m[2][1] * m.m[3][2]) + (m.m[0][1] * m.m[2][2] * m.m[3][0]) + (m.m[0][2] * m.m[2][0] * m.m[3][1])
		- (m.m[0][2] * m.m[2][1] * m.m[3][0]) - (m.m[0][1] * m.m[2][0] * m.m[3][2]) - (m.m[0][0] * m.m[2][2] * m.m[3][1]));

	ret.m[3][2] = (1.0f / a) * (-1.0f * ((m.m[0][0] * m.m[1][1] * m.m[3][2])) - (m.m[0][1] * m.m[1][2] * m.m[3][0]) - (m.m[0][2] * m.m[1][0] * m.m[3][1])
		+ (m.m[0][2] * m.m[1][1] * m.m[3][0]) + (m.m[0][1] * m.m[1][0] * m.m[3][2]) + (m.m[0][0] * m.m[1][2] * m.m[3][1]));

	ret.m[3][3] = (1.0f / a) * ((m.m[0][0] * m.m[1][1] * m.m[2][2]) + (m.m[0][1] * m.m[1][2] * m.m[2][0]) + (m.m[0][2] * m.m[1][0] * m.m[2][1])
		- (m.m[0][2] * m.m[1][1] * m.m[2][0]) - (m.m[0][1] * m.m[1][0] * m.m[2][2]) - (m.m[0][0] * m.m[1][2] * m.m[2][1]));

	return ret;
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

	////　音声データ用の変数宣言
	//Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	//IXAudio2MasteringVoice* masterVoice;
	//// xAudio2エンジンのインスタンスを生成
	//HRESULT soundResult;
	//soundResult = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	//// マスターボイスを生成
	//soundResult = xAudio2->CreateMasteringVoice(&masterVoice);
	//// 音声読み込み
	//SoundData soundData1 = SoundLoadWave("resources/Alarm01.wav");

	// 入力クラスのポインタ
	Input* input = nullptr;
	// 入力の初期化
	input = new Input();
	input->Initialize(winAPIManager);

	//// モデル読み込み
	//ModelData modelData = LoadObjFile("resources", "axis.obj");

	//// VertexResourceを生成する
	//// 頂点リソースを作る
	//Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource = CreateBufferResources(device, sizeof(VertexData) * modelData.vertices.size());

	//// 頂点バッファビューを作成する
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	//// リソースの先頭のアドレスから使う
	//vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//// 使用するリソースのサイズは頂点3つ分のサイズ
	//vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	//// 1頂点あたりのサイズ
	//vertexBufferView.StrideInBytes = sizeof(VertexData);

	//// 頂点リソースにデータを書き込む
	//VertexData* vertexData = nullptr;
	//// 書き込むためのアドレスを取得
	//vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	//// 頂点データにリソースをコピー
	//std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());



	//// Textureを読んで転送する
	//DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	//const DirectX::TexMetadata& metaData = mipImages.GetMetadata();
	//Microsoft::WRL::ComPtr <ID3D12Resource> textureResource = CreateTextureResource(device, metaData);
	//Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource = UploadTextureData(textureResource, mipImages, device, commandList);

	//// 2枚目のTextureを読んで転送する
	//DirectX::ScratchImage mipImages2 = LoadTexture(modelData.material.textureFilePath);
	//const DirectX::TexMetadata& metaData2 = mipImages2.GetMetadata();
	//Microsoft::WRL::ComPtr <ID3D12Resource> textureResource2 = CreateTextureResource(device, metaData2);
	//Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource2 = UploadTextureData(textureResource2, mipImages2, device, commandList);

	//// DepthStencilTextureをウインドウのサイズで作成
	//Microsoft::WRL::ComPtr <ID3D12Resource> depthStencilResource = CreateDepthStancilTextureResource(device, WinAPIManager::kClientWidth, WinAPIManager::kClientHeight);
	//
	//// DepthStencilStateの設定
	//D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//// Depthの機能を有効化する
	//depthStencilDesc.DepthEnable = true;
	//// 書き込む
	//depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//// 比較関数
	//depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//// metaDataを基にSRVを設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	//srvDesc.Format = metaData.format;
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//srvDesc.Texture2D.MipLevels = UINT(metaData.mipLevels);

	//// srvを作成するDescriptHeapの場所を決める
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUDescriptorHandle(srvDescriptHeap, descriptorSizeSRV, 1);
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUDescriptorHandle(srvDescriptHeap, descriptorSizeSRV, 1);
	//// srvの生成
	//device->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);

	//// metaDataを基に2枚目のSRVを設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	//srvDesc2.Format = metaData2.format;
	//srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//srvDesc2.Texture2D.MipLevels = UINT(metaData2.mipLevels);

	//// 2枚目のsrvを作成するDescriptHeapの場所を決める
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptHeap, descriptorSizeSRV, 2);
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptHeap, descriptorSizeSRV, 2);
	//// srvの生成
	//device->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);


	//// RootSignatureを作成
	//D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	//descriptionRootSignature.Flags =
	//	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	//descriptorRange[0].BaseShaderRegister = 0;
	//descriptorRange[0].NumDescriptors = 1;
	//descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//// RootParameterを作成
	//D3D12_ROOT_PARAMETER rootParameters[4] = {};
	//rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	//rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	//rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド
	//rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	//rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // VertexShaderで使う
	//rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド
	//rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	//rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
	//rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	//rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//rootParameters[3].Descriptor.ShaderRegister = 1;
	//descriptionRootSignature.pParameters = rootParameters; // ルートパラメータ配列へのポインタ
	//descriptionRootSignature.NumParameters = _countof(rootParameters); // 配列の長さ


	//// Samplerの設定
	//D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	//staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	//staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	//staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	//staticSamplers[0].ShaderRegister = 0;
	//staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//descriptionRootSignature.pStaticSamplers = staticSamplers;
	//descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);


	//// Index用の頂点リソースを作る
	//Microsoft::WRL::ComPtr <ID3D12Resource> indexResourceSprite = CreateBufferResources(device, sizeof(uint32_t) * 6);
	//// 頂点バッファビューを作成する
	//D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	//// リソースの先頭のアドレスから使う
	//indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	//// 使用するリソースのサイズ
	//indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	//// 1頂点辺りのサイズ
	//indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;
	//// インデックスリソースにデータを書き込む
	//uint32_t* indexDataSprite = nullptr;
	//indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	//indexDataSprite[0] = 0;
	//indexDataSprite[1] = 1;
	//indexDataSprite[2] = 2;
	//indexDataSprite[3] = 1;
	//indexDataSprite[4] = 3;
	//indexDataSprite[5] = 2;

	//// Sprite用の頂点リソースを作る
	//Microsoft::WRL::ComPtr <ID3D12Resource> vertexResourceSprite = CreateBufferResources(device, sizeof(VertexData) * 6);

	//// 頂点バッファビューを作成する
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	//// リソースの先頭のアドレスから使う
	//vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	//// 使用するリソースのサイズ
	//vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	//// 1頂点辺りのサイズ
	//vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	//// 頂点データを設定する
	//VertexData* vertexDataSprite = nullptr;
	//vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	//// 1枚目
	//vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f }; // 左下
	//vertexDataSprite[0].texcoord = { 0.0f,1.0f };
	//vertexDataSprite[0].normal = { 0.0f,0.0f,-1.0f };
	//vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f }; // 左上
	//vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	//vertexDataSprite[1].normal = { 0.0f,0.0f,-1.0f };
	//vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f }; // 右下
	//vertexDataSprite[2].texcoord = { 1.0f,1.0f };
	//vertexDataSprite[2].normal = { 0.0f,0.0f,-1.0f };
	//vertexDataSprite[3].position = { 640.0f,0.0f,0.0f,1.0f }; // 左上
	//vertexDataSprite[3].texcoord = { 1.0f,0.0f };
	//vertexDataSprite[3].normal = { 0.0f,0.0f,-1.0f };



	//// マテリアル用のリソースを作る
	//Microsoft::WRL::ComPtr <ID3D12Resource> materialResource = CreateBufferResources(device, sizeof(Material));
	//// マテリアルにデータを書き込む
	//Material* materialData = nullptr;
	//// 書き込むためのアドレスを取得
	//materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//// 赤色に設定
	//materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//// Lightingするのでtrueに設定
	//materialData->enableLighting = true;
	//// UVTransform行列を単位行列で初期化
	//materialData->uvTransform = MakeIdentity4x4();

	//// Sprite用のマテリアルリソースを作る
	//Microsoft::WRL::ComPtr <ID3D12Resource> materialResourceSprite = CreateBufferResources(device, sizeof(Material));
	//// マテリアルにデータを書き込む
	//Material* materialDataSprite = nullptr;
	//// 書き込むためのアドレスを取得
	//materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	//// 白色に設定
	//materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//// SpriteはLightingしないのでfalseを設定
	//materialDataSprite->enableLighting = false;
	//// UVTransform行列を単位行列で初期化
	//materialDataSprite->uvTransform = MakeIdentity4x4();

	//// 平行光源用のリソースを作る
	//Microsoft::WRL::ComPtr <ID3D12Resource> directionalLightResource =
	//	CreateBufferResources(device, sizeof(DirectionalLight));

	//// データを書き込む
	//DirectionalLight* directionalLightData = nullptr;
	////書き込むためのアドレスを取得
	//directionalLightResource->Map(
	//	0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	//directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	//directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	//directionalLightData->intensity = 1.0f;

	//// WVP用のリソースを作る
	//Microsoft::WRL::ComPtr <ID3D12Resource> wvpResource = CreateBufferResources(device, sizeof(TransformationMatrix));
	//// データを書き込む
	//TransformationMatrix* wvpData = nullptr;
	//// 書き込むためのアドレスを取得
	//wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	//// 単位行列を書き込んでおく
	//wvpData->World = MakeIdentity4x4();
	//wvpData->WVP = MakeIdentity4x4();

	//// Sprite用のリソースを作る
	//Microsoft::WRL::ComPtr <ID3D12Resource> transformationMatrixResourceSprite = CreateBufferResources(device, sizeof(TransformationMatrix));
	//// データを書き込む
	//TransformationMatrix* transformationMatrixDataSprite = nullptr;
	//// 書き込むためのアドレスを取得
	//transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	//// 単位行列を書き込んでおく
	//transformationMatrixDataSprite->World = MakeIdentity4x4();
	//transformationMatrixDataSprite->WVP = MakeIdentity4x4();

	//// シリアライズしてバイナリにする
	//Microsoft::WRL::ComPtr <ID3DBlob> signatureBlob = nullptr;
	//Microsoft::WRL::ComPtr <ID3DBlob> errorBlob = nullptr;
	//hr = D3D12SerializeRootSignature(&descriptionRootSignature,
	//	D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	//if (FAILED(hr))
	//{
	//	Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
	//	assert(false);
	//}
	//// バイナリを元に生成
	//Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;
	//hr = device->CreateRootSignature(0,
	//	signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
	//	IID_PPV_ARGS(&rootSignature));
	//assert(SUCCEEDED(hr));

	//// InputLayout
	//D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = { };
	//inputElementDescs[0].SemanticName = "POSITION";
	//inputElementDescs[0].SemanticIndex = 0;
	//inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//inputElementDescs[1].SemanticName = "TEXCOORD";
	//inputElementDescs[1].SemanticIndex = 0;
	//inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	//inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//inputElementDescs[2].SemanticName = "NORMAL";
	//inputElementDescs[2].SemanticIndex = 0;
	//inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	//inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	//inputLayoutDesc.pInputElementDescs = inputElementDescs;
	//inputLayoutDesc.NumElements = _countof(inputElementDescs);

	//// BlendStateの設定
	//D3D12_BLEND_DESC blendDesc{};
	//// 全ての色要素を書き込む
	//blendDesc.RenderTarget[0].RenderTargetWriteMask =
	//	D3D12_COLOR_WRITE_ENABLE_ALL;

	//// RasterizerStateの設定を行う
	//D3D12_RASTERIZER_DESC rasterizerDesc{};
	//// 裏面を表示しない
	//rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//// 三角形の中を塗りつぶす
	//rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//// ShaderをCompileする
	//Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = CompileShader(L"resources/shaders/Object3d.VS.hlsl",
	//	L"vs_6_0", dxcUtils, dxcCompiler, includeHandler);
	//assert(vertexShaderBlob != nullptr);

	//Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = CompileShader(L"resources/shaders/Object3d.PS.hlsl",
	//	L"ps_6_0", dxcUtils, dxcCompiler, includeHandler);
	//assert(pixelShaderBlob != nullptr);

	//// PSOを生成する
	//D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
	//graphicPipelineStateDesc.pRootSignature = rootSignature.Get();
	//graphicPipelineStateDesc.InputLayout = inputLayoutDesc;
	//graphicPipelineStateDesc.VS =
	//{
	//	vertexShaderBlob->GetBufferPointer(),
	//	vertexShaderBlob->GetBufferSize()
	//};
	//graphicPipelineStateDesc.PS =
	//{
	//	pixelShaderBlob->GetBufferPointer(),
	//	pixelShaderBlob->GetBufferSize()
	//};
	//graphicPipelineStateDesc.BlendState = blendDesc;
	//graphicPipelineStateDesc.RasterizerState = rasterizerDesc;
	//// 書き込むRTVの情報
	//graphicPipelineStateDesc.NumRenderTargets = 1;
	//graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//// 利用するトポロジのタイプ(三角形)
	//graphicPipelineStateDesc.PrimitiveTopologyType =
	//	D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//// どのように画面に色を打ち込むかの設定
	//graphicPipelineStateDesc.SampleDesc.Count = 1;
	//graphicPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//// DepthStencilの設定
	//graphicPipelineStateDesc.DepthStencilState = depthStencilDesc;
	//graphicPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//// 生成
	//Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicPipelineState = nullptr;
	//hr = device->CreateGraphicsPipelineState(&graphicPipelineStateDesc,
	//	IID_PPV_ARGS(&graphicPipelineState));
	//assert(SUCCEEDED(hr));



	//
	//
	//// Fenceのsignalを待つためのイベントを作成する
	//HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	//assert(fenceEvent != nullptr);


	//// Transform変数を作る
	//Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	//Transform cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} };
	//Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	//Transform uvTransformSprite
	//{
	//	{1.0f,1.0f,1.0f},
	//	{0.0f,0.0f,0.0f},
	//	{0.0f,0.0f,0.0f}
	//};


	//// スプライト切り替えフラグ
	//bool useMonsterBall = true;

	//// 音声再生
	//SoundPlayWave(xAudio2.Get(), soundData1);

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
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
#endif // USE_IMGUI




		//// ゲームの処理
		//// 更新処理
		////transform.rotate.y += 0.01f;
		//Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		//Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
		//Matrix4x4 viewMatrix = Inverse(cameraMatrix);
		//Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(WinAPIManager::kClientWidth) / float(WinAPIManager::kClientHeight), 0.1f, 100.0f);
		//Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
		//wvpData->WVP = worldViewProjectionMatrix;
		//wvpData->World = worldMatrix;

		//// Sprite用のWorldViewProjectionMatrixを作る
		//Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
		//Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
		//Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(WinAPIManager::kClientWidth), float(WinAPIManager::kClientHeight), 0.0f, 100.0f);
		//Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
		//transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;

		//// UVTransform用の行列を作成する
		//Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
		//uvTransformMatrix = Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
		//uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
		//materialDataSprite->uvTransform = uvTransformMatrix;

		// 入力の更新処理
		input->Update();

		// 数字の0キーが押されていたら
		if (input->TriggerKey(DIK_0))
		{
			OutputDebugStringA("Hit 0\n");
		}

#ifdef USE_IMGUI
		// 開発用UIの処理
		ImGui::ShowDemoWindow();

		ImGui::Checkbox("useMonsterBall", &useMonsterBall);

		ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
		ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
		ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

		ImGui::Separator();
		ImGui::Text("Camera");
		ImGui::DragFloat3("Camera Pos", &cameraTransform.translate.x, 0.1f);
		ImGui::SliderAngle("Camera Rot X", &cameraTransform.rotate.x);
		ImGui::SliderAngle("Camera Rot Y", &cameraTransform.rotate.y);
		ImGui::SliderAngle("Camera Rot Z", &cameraTransform.rotate.z);

		ImGui::Separator();
		ImGui::Text("Model Rotation");
		ImGui::SliderAngle("Rot X", &transform.rotate.x, -180.0f, 180.0f);
		ImGui::SliderAngle("Rot Y", &transform.rotate.y, -180.0f, 180.0f);
		ImGui::SliderAngle("Rot Z", &transform.rotate.z, -180.0f, 180.0f);

		// ImGuiの内部コマンドを生成する
		ImGui::Render();
#endif // USE_IMGUI




		// 描画処理
		// 描画前処理
		dxBasis->PreDraw();

		//// RootSignatureを設定
		//commandList->SetGraphicsRootSignature(rootSignature.Get());
		//// PSOを設定
		//commandList->SetPipelineState(graphicPipelineState.Get());
		//// VBVを設定
		//commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		//// 形状を設定
		//commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//// マテリアルCBufferの場所を設定
		//commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
		//// wvp用のCBufferの場所を設定
		//commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
		//// SRVのDescriptorTableの先頭を設定
		//commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
		//// 平行光源用のCBufferの場所を設定
		//commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
		//// 描画
		//commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

		//// Spriteの描画処理
		//// マテリアルのCBufferの場所を設定
		//commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
		//// VBVを設定
		//commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
		//// IBVを設定
		//commandList->IASetIndexBuffer(&indexBufferViewSprite);
		//// SRVを設定
		//commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
		//// SpriteCBufferの場所を設定
		//commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
		//// 描画
		////commandList->DrawIndexedInstanced(6, 1, 0, 0,0);

#ifdef USE_IMGUI

			// ImGuiを描画
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

#endif

		// 描画後処理
		dxBasis->PostDraw();

	}

#ifdef USE_IMGUI
	// ImGui終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif



	// 解放処理
	//CloseHandle(fenceEvent);

	delete input;
	delete dxBasis;

	// xAudio2解放
	//xAudio2.Reset();
	// 音声データ解放
	//SoundUnload(&soundData1);

	// WinAPIの終了処理
	winAPIManager->Finalize();

	// WinAPIの解放処理
	delete winAPIManager;

	return 0;
}


