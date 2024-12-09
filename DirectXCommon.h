#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

class DirectXCommon
{
public:

	//初期化
	void Initialize();
	
	//デバイスの初期化
	void InitializeDevice();

private:
	
	HRESULT hr;

	//DXGIファクトリーの生成
	IDXGIFactory7* dxgiFactory = nullptr;
	//デバイスの生成
	ID3D12Device* device = nullptr;
};

