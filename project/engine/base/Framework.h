#pragma once

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

#include "AbstractSceneFactory.h"

#include "D3DResourceLeakChecker.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "Camera.h"
#include "SrvManager.h"
#include "ParticleManager.h"
#include "Audio.h"


class Framework
{
public:
	// 初期化
	virtual void Initialize();
	// 更新
	virtual void Update();
	// 描画
	virtual void Draw() = 0;
	// 終了
	virtual void Finalize();
	// 終了リクエストの確認
	virtual bool IsEndRequest() const { return isEndRequest; }
	// 仮想デストラクタ
	virtual ~Framework() = default;
	// 実行
	void Run();
	// CrashHandlerの登録
	static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

protected:
	// リークチェック用のインスタンス
	D3DResourceLeakChecker leakCheck;
	// WinAPIのポインタ
	WinAPIManager* winAPIManager = nullptr;
	// DirectX基盤のポインタ
	DirectXBasis* dxBasis = nullptr;
	// SRVマネージャーのポインタ
	SrvManager* srvManager = nullptr;
	// ImGuiマネージャーのポインタ
	ImguiManager* imguiManager = nullptr;
	// カメラのポインタ
	Camera* camera = nullptr;
	
	// Fenceのsignalを待つためのイベント
	HANDLE fenceEvent = nullptr;

	// 終了リクエストフラグ
	bool isEndRequest = false;

	// シーンファクトリー
	AbstractSceneFactory* sceneFactory = nullptr;
};

