#pragma once
#include <Windows.h>

class Input
{
public:

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize(HINSTANCE hInstance, HWND hwnd);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();
};

