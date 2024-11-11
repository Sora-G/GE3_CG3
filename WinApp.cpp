#include "WinApp.h"
#include "externals/imgui/imgui_impl_win32.cpp"

//ウィンドウプロージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true;
	}

	//メッセージに応じてゲームの固有処理を行う
	switch (msg)
	{
		//ウィンドウ破棄された
	case WM_DESTROY:
		//OSに対してアプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void WinApp::Initialize()
{
	CoInitializeEx(0, COINIT_MULTITHREADED);

	WNDCLASS wc{};

	//ウィンドウプロージャ
	wc.lpfnWndProc = WindowProc;

	//ウィンドウクラス名(なんでもいい)
	wc.lpszClassName = L"CG2WindowClass";

	//インスタンスバンドル
	wc.hInstance = GetModuleHandle(nullptr);

	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);


	//ウィンドウクラスを登録する
	RegisterClass(&wc);


	//クライアント領域のサイズ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	//ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWidth, kClientHeight };

	//クライアント領域を基に実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName,		//利用するクラス名
		L"GE3_01-02",			//タイトルバーの文字(なんでもいい)
		WS_OVERLAPPEDWINDOW,	//よく見るウィンドウスタイル
		CW_USEDEFAULT,			//表示X座標(Windousに任せる)
		CW_USEDEFAULT,			//表示Y座標(WindousOSに任せる)
		wrc.right - wrc.left,	//ウィンドウの横幅
		wrc.bottom - wrc.top,	//ウィンドウの立幅
		nullptr,				//親ウィンドウバンドル
		nullptr,				//メニューバンドル
		wc.hInstance,			//インスタンスバンドル
		nullptr);				//オプション

	//ウィンドウの表示
	ShowWindow(hwnd, SW_SHOW);
}

void WinApp::Update()
{

}
