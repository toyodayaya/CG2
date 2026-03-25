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

#pragma warning(pop)

using namespace MathManager;

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
	camera->SetRotate({ 0.0f,0.0f,0.0f });
	camera->SetTranslate({ 0.0f,0.0f,-10.0f });

	// テクスチャマネージャーの初期化
	TextureManager::GetInstance()->Initialize(dxBasis,srvManager);

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

	// 音声再生
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
			object3d->Draw();

		}

		// Spriteの描画
		for (Sprite* sprite : sprites)
		{
			sprite->Draw();
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


