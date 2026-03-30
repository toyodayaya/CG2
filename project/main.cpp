#pragma warning(push)
// C4023の警告を見なかったことにする
#pragma warning(disable:4023)
#include "WinAPIManager.h"
#include <dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")
#include <strsafe.h>
#include <MathManager.h>

#include <wrl.h>

#include "Input.h"
#include "DirectXBasis.h"
#include <dxcapi.h>
#include <Logger.h>
#include <StringUtility.h>

#ifdef USE_IMGUI
#include <externals/imgui/imgui_impl_dx12.h>
#include <externals/imgui/imgui_impl_win32.h>
#include <externals/imgui/imgui_impl_dx12.h>
#endif // USE_IMGUI

#include "ImguiManager.h"

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
#include <numbers>
#include "ParticleManager.h"
#include "ParticleEmitter.h"
#include "Audio.h"

#pragma warning(pop)

using namespace MathManager;
using namespace Logger;

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

	// Imguiマネージャーの初期化
	ImguiManager* imguiManager = nullptr;
	imguiManager = new ImguiManager();
	imguiManager->Initialize(winAPIManager, dxBasis, srvManager);

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

	// Audioの初期化
	Audio* audio = new Audio();
	audio->Initialize();
	// 音声読み込み
	SoundData soundData1 = audio->SoundLoadFile("resources/fanfare.mp3");

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

	// パーティクルマネージャーの初期化
	ParticleManager::GetInstance()->Initialize(dxBasis, srvManager, camera);
	ParticleManager::GetInstance()->CreateParticleGroup("Particle", "resources/circle.png");
	// パーティクルエミッターのポインタ
	ParticleEmitter* emitter = nullptr;

	// 音声再生
	audio->SoundPlayWave(audio->GetXAudio2().Get(), soundData1);

	// パーティクルエミッターの宣言
	Transform translate;
	translate.translate = { 0.0f,0.0f,0.0f };
	translate.rotate = { 0.0f,0.0f,0.0f };
	translate.scale = { 1.0f,1.0f,1.0f };
	emitter = new ParticleEmitter("Particle", translate.translate, 0.5f, 2);


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

		Transform uvTransformSprite
		{
			{1.0f,1.0f,1.0f},
			{0.0f,0.0f,0.0f},
			{0.0f,0.0f,0.0f}
		};

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

#ifdef USE_IMGUI

		// 開発用UIの処理
		imguiManager->Begin();

#endif // USE_IMGUI


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

		// パーティクルの更新処理
		ParticleManager::GetInstance()->Update();
		emitter->Update();

#ifdef USE_IMGUI

		// ImGuiの受け付け終了
		imguiManager->End();

#endif // USE_IMGUI


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


		// Spriteの描画
		for (Sprite* sprite : sprites)
		{
			//sprite->Draw();
		}

		// パーティクルの描画
		ParticleManager::GetInstance()->Draw();

#ifdef USE_IMGUI

		// ImGuiの描画
		imguiManager->Draw();

#endif // USE_IMGUI

		// 描画後処理
		dxBasis->PostDraw();

		TextureManager::GetInstance()->ReleaseIntermediateResources();

	}

	// 解放処理
	CloseHandle(fenceEvent);

	// テクスチャマネージャーの終了
	TextureManager::GetInstance()->Finalize();
	// 3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();
	// パーティクルマネージャーの終了
	ParticleManager::GetInstance()->Finalize();
	// ImGuiマネージャーの終了
	imguiManager->Finalize();

	delete input;

	// 音声データ解放
	audio->Finalize();
	audio->SoundUnload(&soundData1);
	delete audio;

	delete emitter;

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
	delete imguiManager;
	delete srvManager;
	delete dxBasis;

	
	// WinAPIの終了処理
	winAPIManager->Finalize();

	// WinAPIの解放処理
	delete winAPIManager;

	return 0;
}


