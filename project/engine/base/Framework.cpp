#include "Framework.h"

void Framework::Initialize()
{
	// ゲーム固有の初期化処理

	// 誰も捕捉しなかった場合に捕捉する関数を登録
	SetUnhandledExceptionFilter(ExportDump);

	// WinAPIの初期化
	winAPIManager = std::make_unique<WinAPIManager>();
	winAPIManager->Initialize();

	// DirectX基盤の初期化
	dxBasis = std::make_unique <DirectXBasis>();
	dxBasis->Initialize(winAPIManager.get());

	// SRVマネージャーの初期化
	srvManager = std::make_unique <SrvManager>();
	srvManager->Initialize(dxBasis.get());

	// カメラの初期化
	camera = new Camera();

	// テクスチャマネージャーの初期化
	TextureManager::GetInstance()->Initialize(dxBasis.get(), srvManager.get());

	// RenderTextureの初期化
	RenderTexture::GetInstance()->Initialize(dxBasis.get(),srvManager.get());

	// スプライト共通部の初期化
	SpriteCommon::GetInstance()->Initialize(dxBasis.get());
	SpriteCommon::GetInstance()->SetDefaultCamera(camera);
	// 3dオブジェクト共通部の初期化
	Object3dCommon::GetInstance()->Initialize(dxBasis.get());
	Object3dCommon::GetInstance()->SetDefaultCamera(camera);
	// Skybox共通部の初期化
	SkyboxCommon::GetInstance()->Initialize(dxBasis.get());
	SkyboxCommon::GetInstance()->SetDefaultCamera(camera);

	// 3Dモデルマネージャーの初期化
	ModelManager::GetInstance()->Initialize(dxBasis.get());

	// Audioの初期化
	Audio::GetInstance()->Initialize();

	// 入力の初期化
	Input::GetInstance()->Initialize(winAPIManager.get());

	// Fenceのsignalを待つためのイベントを作成する
	fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);

	// パーティクルマネージャーの初期化
	ParticleManager::GetInstance()->Initialize(dxBasis.get(), srvManager.get(), camera);
	
}

void Framework::Update()
{
	// Windowsのメッセージ処理
	if (winAPIManager->ProcessMessage())
	{
		// ゲームループを抜けるフラグを立てる
		isEndRequest = true;
	}

	// 入力の更新処理
	Input::GetInstance()->Update();

	// 数字の0キーが押されていたら
	if (Input::GetInstance()->TriggerKey(DIK_0))
	{
		OutputDebugStringA("Hit 0\n");
	}

	// カメラの更新
	camera->Update();

	// パーティクルマネージャーの更新
	ParticleManager::GetInstance()->Update();


}

void Framework::Finalize()
{
	// 汎用部の終了処理
	CloseHandle(fenceEvent);
	// パーティクルマネージャーの終了
	ParticleManager::GetInstance()->Finalize();
	// 入力クラスの終了
	Input::GetInstance()->Finalize();
	// 音声データ解放
	Audio::GetInstance()->Finalize();
	
	// 3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();
	// Skybox共通部の終了
	SkyboxCommon::GetInstance()->Finalize();
	// 3dオブジェクト共通部の終了
	Object3dCommon::GetInstance()->Finalize();
	// スプライト共通部の終了
	SpriteCommon::GetInstance()->Finalize();
	// RenderTextureの終了
	RenderTexture::GetInstance()->Finalize();
	// テクスチャマネージャーの終了
	TextureManager::GetInstance()->Finalize();
	// カメラの終了
	delete camera;
	
	// WinAPIの終了処理
	winAPIManager->Finalize();

}

void Framework::Run()
{
	// ゲームの初期化
	Initialize();

	// メインループ
	while (true)
	{
		// 更新処理
		Update();

		// 終了リクエストが来たらループを抜ける
		if (IsEndRequest())
		{
			break;
		}

		// 描画処理
		Draw();
	}

	// 解放処理
	Finalize();
}

LONG __stdcall Framework::ExportDump(EXCEPTION_POINTERS* exception)
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
