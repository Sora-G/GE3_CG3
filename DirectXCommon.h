#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "WinApp.h"

class DirectXCommon
{
public:

	//初期化
	void Initialize();
	
	//デバイスの初期化
	void InitializeDevice();

	//コマンド関連の初期化
	void InitializeCommand();

	//スワップチェーンの生成
	void CreateSwapChain();

private:
	
	HRESULT hr;
	//WindowsAPI
	WinApp* winapp = nullptr;
	//DXGIファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;
	//デバイスの生成
	Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;
	//コマンドアロケーターの生成
 	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	//コマンドリストの生成
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	//コマンドキューの生成
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	//スワップチェーンの生成
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
};

