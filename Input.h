#pragma once
#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800 //バージョン指定
#include <dinput.h>
#include <wrl.h>

class Input
{
public:

	//nameSpace省略
	template <class T>using ComPtr = Microsoft::WRL::ComPtr<T>;

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize(HINSTANCE hInstance, HWND hwnd);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

private:

	//キーボードデバイス生成
	ComPtr<IDirectInputDevice8>keyboard;
};

