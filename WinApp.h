#pragma once
#include <Windows.h>

class WinApp
{
public://静的メンバ変数
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


public://定数
	//クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();
};

