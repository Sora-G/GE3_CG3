#pragma once
#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800 //バージョン指定
#include <dinput.h>
#include <wrl.h>
#include "WinApp.h"

class Input
{
private:
	//WindowsAPI
	WinApp* winApp_ = nullptr;

public:

	//nameSpace省略
	template <class T>using ComPtr = Microsoft::WRL::ComPtr<T>;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize(WinApp* winApp);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// キーの押したかをチェック
	/// </summary>
	/// <param name="keyNumber">キー番号（DIK_0等）</param>
	/// <returns>押されているか</returns>
	bool PushKey(BYTE keyNumber);

	/// <summary>
	/// キーのトリガーをチェック
	/// </summary>
	/// <param name="keyNumber">キー番号（DIK_0等）</param>
	/// <returns>トリガーか</returns>
	bool TriggerKey(BYTE keyNumber);

private:

	//全キーの入力情報を取得する
	BYTE key[256] = {};
	//前回の全キーの入力情報を取得する
	BYTE keyPre[256] = {};

	//キーボードデバイス生成
	ComPtr<IDirectInputDevice8>keyboard;

	//DirectInputのインスタンス
	ComPtr<IDirectInput8>directInput;
};

