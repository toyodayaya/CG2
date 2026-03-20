#include "DirectXBasis.h"
#include <cassert>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#include "Logger.h"
#include "StringUtility.h"
#include <format>
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
#include <externals/DirectXTex/d3dx12.h>
#include <thread>

void DirectXBasis::Initialize(WinAPIManager* winApiManager)
{
	// NULL検出
	assert(winApiManager);
	// メンバ変数に記録
	this->winApiManager_ = winApiManager;

	// FPS固定初期化
	InitializeFixFPS();

	// デバイスの生成
	DeviceInitialize();

	// コマンド関連の生成
	CommandInitialize();

	// スワップチェーンの生成
	SwapChainGenerate();

	// 深度バッファの生成
	DepthStencilGenerate();

	// 各種ディスクリプタヒープの生成
	DescriptorHeapGenerate();

	// レンダーターゲットビューの初期化
	RenderTargetViewInitialize();

	// 深度ステンシルビューの初期化
	dsvInitialize();

	// フェンスの生成
	FenceInitialize();

	// ビューポート矩形の初期化
	ViewportInitialize();

	// シザー矩形の生成
	ScissorRectInitialize();

	// DCXコンパイラの生成
	DXCCompilerGenerate();

	// ImGuiの初期化
	ImGuiInitialize();

}

void DirectXBasis::DeviceInitialize()
{
#ifdef _DEBUG
	Microsoft::WRL::ComPtr <ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		// デバッグレイヤーを有効化する
		debugController->EnableDebugLayer();
		// GPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}

#endif // 


	
	// 関数が成功したかどうかをSUCCEEDEDマクロで確認する
	hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	// 初期化の根本的な部分でエラーが出た場合はassertにしておく
	assert(SUCCEEDED(hr));

	// 使用するアダプタ用の変数
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
	// いい順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(
		i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter))
		!= DXGI_ERROR_NOT_FOUND; ++i)
	{
		// アダプタの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		// 取得できたかどうか判定
		assert(SUCCEEDED(hr));
		// ソフトウェアアダプタでなければ採用
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			// 採用したアダプタの情報をログに出力
			Logger::Log(StringUtility::ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}

		// ソフトウェアアダプタだった場合は見なかったことにする
		useAdapter = nullptr;
	}

	// 適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);

	
	// 機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	// 高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i)
	{
		// 採用したアダプタでデバイスを生成
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));
		// 指定した機能レベルでデバイスが生成できたかどうかを判定
		if (SUCCEEDED(hr))
		{
			// 生成できたのでログ出力を行ってループを抜ける
			Logger::Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}

	// デバイスの生成が上手く行かなかったので起動できない
	assert(device != nullptr);
	// 初期化完了のログを出す
	Logger::Log("Complete create D3D12Device!!!\n");

#ifdef  _DEBUG

	Microsoft::WRL::ComPtr <ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		// ヤバいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		// エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		// 警告時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		// 抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] =
		{
			// Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
		};

		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY serverities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO,
		};
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(serverities);
		filter.DenyList.pSeverityList = serverities;
		// 指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);
	}




#endif //  _DEBUG

}

void DirectXBasis::CommandInitialize()
{
	// コマンドアロケータを生成する
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	// コマンドアロケータの生成が上手く行かなかったので起動できない
	assert(SUCCEEDED(hr));

	// コマンドリストを生成する
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	// コマンドリストの生成が上手く行かなかったので起動できない
	assert(SUCCEEDED(hr));


	// コマンドキューを生成する
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	// コマンドキューの生成が上手く行かなかったので起動できない
	assert(SUCCEEDED(hr));

}

void DirectXBasis::SwapChainGenerate()
{
	// スワップチェーンを生成する
	
	swapChainDesc.Width = winApiManager_->kClientWidth; // ウインドウの幅
	swapChainDesc.Height = winApiManager_->kClientHeight; // ウインドウの高さ
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色のフォーマット
	swapChainDesc.SampleDesc.Count = 1; // マルチサンプリングしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして使う
	swapChainDesc.BufferCount = 2; // ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // モニタに映したら中身を破棄
	// コマンドキュー、ウインドウハンドル、設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), winApiManager_->GetHwnd(), &swapChainDesc, nullptr, nullptr,
		reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

void DirectXBasis::DepthStencilGenerate()
{
	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = winApiManager_->kClientWidth; // Textureの幅
	resourceDesc.Height = winApiManager_->kClientHeight; // Textureの高さ
	resourceDesc.MipLevels = 1; // mipmapの数
	resourceDesc.DepthOrArraySize = 1; // 奥行き
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f; // 最大値でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Format(Resourceと合わせる)

	// Resourceの生成
	hr = device->CreateCommittedResource(
		&heapProperties, // Heapの設定
		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定(特になし)
		&resourceDesc, // Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態に設定
		&depthClearValue, // Clear最適値
		IID_PPV_ARGS(&depthStencilResource) // 作成するResourcesポインタへのポインタ
	);

	assert(SUCCEEDED(hr));
}

void DirectXBasis::DescriptorHeapGenerate()
{
	// DescriptorSizeを取得しておく
	descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// RTV用のディスクリプタヒープを作成
	rtvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	// SRV用のディスクリプタヒープを作成
	srvDescriptHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	// DSV用のディスクリプタヒープを作成
	dsvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXBasis::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{
	// ディスクリプタヒープを生成する
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	// ディスクリプタヒープの生成が上手く行かなかったので起動できない
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

void DirectXBasis::RenderTargetViewInitialize()
{
	// スワップチェーンからリソースを引っ張ってくる
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	// リソースの取得が上手く行かなかったので起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	// rtvの設定
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャとして扱う
	// ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = GetCPUDescriptorHandle(rtvDescriptorHeap, descriptorSizeRTV, 0);
	

	for (uint32_t i = 0; i < 2; ++i)
	{
		// 作成する
		rtvHandles[i] = GetCPUDescriptorHandle(rtvDescriptorHeap, descriptorSizeRTV, i);
		device->CreateRenderTargetView(swapChainResources[i].Get(), &rtvDesc, rtvHandles[i]);
	}

}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXBasis::GetSRVCPUDescriptorHandle(uint32_t index)
{
	return GetCPUDescriptorHandle(srvDescriptHeap, descriptorSizeSRV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXBasis::GetSRVGPUDescriptorHandle(uint32_t index)
{
	return GetGPUDescriptorHandle(srvDescriptHeap, descriptorSizeSRV, index);
}

void DirectXBasis::dsvInitialize()
{
	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	// DSVHeapの先頭にDSVを作成
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void DirectXBasis::FenceInitialize()
{
	// 初期値0でFenceを作る
	fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	// Fenceのsignalを待つためのイベントを作成する
	fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);
}

void DirectXBasis::ViewportInitialize()
{
	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = WinAPIManager::kClientWidth;
	viewport.Height = WinAPIManager::kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

void DirectXBasis::ScissorRectInitialize()
{
	// ビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = WinAPIManager::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WinAPIManager::kClientHeight;
}

void DirectXBasis::DXCCompilerGenerate()
{
	// dxcCompilerを初期化
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	// includeに対応するための設定
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

}

void DirectXBasis::ImGuiInitialize()
{
#ifdef USE_IMGUI
	// ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApiManager_->GetHwnd());
	ImGui_ImplDX12_Init(device.Get(),
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptHeap.Get(),
		srvDescriptHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptHeap->GetGPUDescriptorHandleForHeapStart()
	);
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Build();

#endif // 
}

Microsoft::WRL::ComPtr<IDxcBlob> DirectXBasis::CompileShader(const std::wstring& filePath, const wchar_t* profile)
{
	// hlslファイルを読み込む

	// シェーダーをコンパイルする旨をログに出す
	Logger::Log(StringUtility::ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));
	// hlslファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	// 読めなかったら止める
	assert(SUCCEEDED(hr));
	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF-8であることを通知

	// Compileする
	LPCWSTR arguments[] =
	{
		filePath.c_str(), // コンパイルするhlslファイル名
		L"-E", L"main", // エントリーポイントの指定
		L"-T", profile, // プロファイルの設定
		L"-Zi", L"-Qembed_debug", // デバッグ用の情報を埋め込む
		L"-Od", // 最適化をしない
		L"-Zpr", // メモリレイアウトは行優先
	};

	// 実際にShaderをCompileする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,  // 読み込んだファイル
		arguments, // コンパイルオプション
		_countof(arguments),  // コンパイルオプションの数
		includeHandler,  // includeを含んだ諸々
		IID_PPV_ARGS(&shaderResult) // コンパイル結果
	);

	// dxcが起動できないなどの致命的な状況
	assert(SUCCEEDED(hr));


	// 警告・エラーが出ていないか確認する
	// 出ていたらログに出して止める
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
	{
		Logger::Log(shaderError->GetStringPointer());
		// 警告・エラーが出ているので止める
		assert(false);
	}

	// Compile結果を受け取って返す
	// コンパイル結果から実行用のバイナリ部分を取得
	Microsoft::WRL::ComPtr <IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	// 成功したログを出す
	Logger::Log(StringUtility::ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));
	// 実行用のバイナリを返却
	return shaderBlob.Get();
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXBasis::CreateBufferResources(size_t sizeInBytes)
{
	// VertexResourceを生成する
	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	// バッファリソース
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes;
	// バッファの場合は以下を1にする
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	// バッファの場合は以下の設定で行う
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	// 頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXBasis::CreateTextureResource(const DirectX::TexMetadata& metaData)
{
	// metaDataを基にResourcesの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metaData.width); // Textureの幅
	resourceDesc.Height = UINT(metaData.height); // Textureの高さ
	resourceDesc.MipLevels = UINT16(metaData.mipLevels); // mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metaData.arraySize); //奥行き
	resourceDesc.Format = metaData.format;
	resourceDesc.SampleDesc.Count = 1; // サンプリングの数
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metaData.dimension); // Textureの次元数

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// Resourcesの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties, // Heapの設定
		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定(特になし)
		&resourceDesc, // Resourcesの設定
		D3D12_RESOURCE_STATE_COPY_DEST, // データ転送される設定
		nullptr, // Clear最適値
		IID_PPV_ARGS(&resource) // 作成するResourcesポインタへのポインタ
	);

	assert(SUCCEEDED(hr));
	return resource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXBasis::UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages)
{
	std::vector<D3D12_SUBRESOURCE_DATA> subreSources;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subreSources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subreSources.size()));
	Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource = CreateBufferResources(intermediateSize);
	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subreSources.size()), subreSources.data());
	// Textureへの転送後は利用できるようにResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

DirectX::ScratchImage DirectXBasis::LoadTexture(const std::string& filePath)
{
	// テクスチャファイルを読み込んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = StringUtility::ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミニマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	// ミニマップ付きのデータを返す
	return mipImages;
}

void DirectXBasis::PreDraw()
{
	// これから書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();


	// 今回のバリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	// Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	// バリアを張る対象のリソース(現在のバックバッファに対して行う)
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	// 遷移前(現在)のResourceState
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	// 遷移後のResourceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	// TransitionBarrierを張る
	commandList->ResourceBarrier(1, &barrier);

	// 描画先のRTVとDSVを設定する
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
	// 指定した色で画面全体をクリアする
	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
	// 指定した深度で画面全体をクリアする
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	
	// 描画用のDescriptorHeapの設定
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeaps[] = { srvDescriptHeap };
	commandList->SetDescriptorHeaps(1, descriptorHeaps->GetAddressOf());

	// viewportを設定
	commandList->RSSetViewports(1, &viewport);
	// Scissorを設定
	commandList->RSSetScissorRects(1, &scissorRect);
}

void DirectXBasis::PostDraw()
{
	// これから書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	// 画面に映す状態に遷移
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	// TransitionBarrierを張る
	commandList->ResourceBarrier(1, &barrier);

	// コマンドリストの内容を確定させる
	hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// GPUにコマンドリストの実行を行わせる
	Microsoft::WRL::ComPtr <ID3D12CommandList> commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, commandLists->GetAddressOf());
	// GPUとOSに画面の交換を行うよう通知する
	swapChain->Present(1, 0);


	// Fenceの値を更新する
	fenceValue++;
	//GPUがここまで辿り着いた時に、Fenceの値を指定した値に代入するようにSignalを送る
	commandQueue->Signal(fence.Get(), fenceValue);

	// Fenceの値が指定したSignal値に到達しているか確認する
	if (fence->GetCompletedValue() < fenceValue)
	{
		// 指定したSignal値にたどり着くまで待つようにイベントを設定する
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		// イベントが発生するまで待つ
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// FPS固定
	UpdateFixFPS();

	// 次のフレーム用のコマンドリストを準備
	hr = commandAllocator->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(hr));


}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXBasis::GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXBasis::GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

void DirectXBasis::InitializeFixFPS()
{
	// 現在時間を記録する
	reference_ = std::chrono::steady_clock::now();
}

void DirectXBasis::UpdateFixFPS()
{
	// 1/60秒ぴったりの時間
	const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
	// 1/60秒よりわずかに短い時間
	const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

	// 現在時間を取得する
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	// 前回記録からの経過時間を取得する
	std::chrono::microseconds elapsed =
		std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	// 1/60秒経っていない場合
	if (elapsed < kMinCheckTime)
	{
		// 1/60秒経過するまでスリープを繰り返す
		while (std::chrono::steady_clock::now() - reference_ < kMinCheckTime)
		{
			// 1マイクロ秒スリープ
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}

	// 現在時間を記録する
	reference_ = std::chrono::steady_clock::now();
}
