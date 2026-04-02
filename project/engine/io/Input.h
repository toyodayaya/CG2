#pragma once
#include <Windows.h>
#include <wrl.h>
#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <dinput.h>
#include "WinAPIManager.h"

// 入力クラス
class Input
{
private:
	// コンストラクタ
	Input() = default;
	// デストラクタ
	~Input() = default;
	// コピーコンストラクタとコピー代入演算子を削除
	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;
	// インスタンス
	static Input* instance;

public:
	// namespaceを省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	// 初期化
	void Initialize(WinAPIManager* winApiManager);

	// 更新
	void Update();

	// キーのプッシュ判定を取得
	bool PushKey(BYTE keyNumber);

	// キーのトリガー判定を取得
	bool TriggerKey(BYTE keyNumber);

	// インスタンス
	static Input* GetInstance();

	// 終了
	void Finalize();

private:
	// キーボードデバイスの生成
	ComPtr <IDirectInputDevice8> keyboard;
	// DirectInputの初期化
	ComPtr<IDirectInput8> directInput;
	// 全キーの入力状態を取得する
	BYTE key[256] = {};
	// 全キーの前回の入力状態を取得する
	BYTE preKey[256] = {};
	// WindowsAPI
	WinAPIManager* winApiManager_ = nullptr;
};