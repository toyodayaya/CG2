#include "Input.h"

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#include <cassert>

void Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
	
	HRESULT inputResult = DirectInput8Create(
		hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&directInput, nullptr);
	assert(SUCCEEDED(inputResult));
	
	inputResult = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(inputResult));
	// 入力データ形式のセット
	inputResult = keyboard->SetDataFormat(&c_dfDIKeyboard); // 標準形式
	assert(SUCCEEDED(inputResult));
	// 排他制御レベルのセット
	inputResult = keyboard->SetCooperativeLevel(
		hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(inputResult));
}

void Input::Update()
{
	// 前回のキー入力を保存
	memcpy(preKey, key, sizeof(key));

	// キーボード情報の取得開始
	keyboard->Acquire();
	
	keyboard->GetDeviceState(sizeof(key), key);
	
}

bool Input::PushKey(BYTE keyNumber)
{
	// 指定キーを押していたらtrueを返す
	if (key[keyNumber])
	{
		return true;
	}

	// そうでなければfalseを返す
	return false;
}

bool Input::TriggerKey(BYTE keyNumber)
{
	// 指定キーをトリガーしていたらtrueを返す
	if (!preKey[keyNumber] && key[keyNumber])
	{
		return true;
	}

	// そうでなければfalseを返す
	return false;
}
