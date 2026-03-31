#include "Framework.h"

void Framework::Initialize()
{
	// ゲーム固有の初期化処理

	// 誰も捕捉しなかった場合に捕捉する関数を登録
	SetUnhandledExceptionFilter(ExportDump);

	// WinAPIの初期化
	winAPIManager = new WinAPIManager();
	winAPIManager->Initialize();

	// DirectX基盤の初期化
	dxBasis = new DirectXBasis();
	dxBasis->Initialize(winAPIManager);

	// SRVマネージャーの初期化
	srvManager = new SrvManager();
	srvManager->Initialize(dxBasis);

	// Imguiマネージャーの初期化
	imguiManager = new ImguiManager();
	imguiManager->Initialize(winAPIManager, dxBasis, srvManager);

	// カメラの初期化
	camera = new Camera();

	// テクスチャマネージャーの初期化
	TextureManager::GetInstance()->Initialize(dxBasis, srvManager);

	// スプライト共通部の初期化
	spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxBasis);

	// 3dオブジェクト共通部の初期化
	object3dCommon = new Object3dCommon();
	object3dCommon->Initialize(dxBasis);
	object3dCommon->SetDefaultCamera(camera);

	// 3Dモデルマネージャーの初期化
	ModelManager::GetInstance()->Initialize(dxBasis);

	// Audioの初期化
	audio = new Audio();
	audio->Initialize();

	// 入力の初期化
	input = new Input();
	input->Initialize(winAPIManager);

	// Fenceのsignalを待つためのイベントを作成する
	fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);

	// パーティクルマネージャーの初期化
	ParticleManager::GetInstance()->Initialize(dxBasis, srvManager, camera);
	
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
	input->Update();

	// 数字の0キーが押されていたら
	if (input->TriggerKey(DIK_0))
	{
		OutputDebugStringA("Hit 0\n");
	}

#ifdef USE_IMGUI

	// 開発用UIの処理
	imguiManager->Begin();

#endif // USE_IMGUI


	// カメラの更新
	camera->Update();

	// パーティクルマネージャーの更新
	ParticleManager::GetInstance()->Update();

#ifdef USE_IMGUI

	// ImGuiの受け付け終了
	imguiManager->End();

#endif // USE_IMGUI
}

void Framework::Finalize()
{
	// 汎用部の終了処理
	CloseHandle(fenceEvent);
	// パーティクルマネージャーの終了
	ParticleManager::GetInstance()->Finalize();
	delete input;
	// 音声データ解放
	audio->Finalize();
	delete audio;
	// 3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();
	delete object3dCommon;
	delete spriteCommon;
	// テクスチャマネージャーの終了
	TextureManager::GetInstance()->Finalize();
	delete camera;
	// ImGuiマネージャーの終了
	imguiManager->Finalize();
	delete imguiManager;
	delete srvManager;
	delete dxBasis;


	// WinAPIの終了処理
	winAPIManager->Finalize();

	// WinAPIの解放処理
	delete winAPIManager;
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
