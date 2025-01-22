#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "WinApp.h"
#include "array"
#include "iostream"
#include <string>
#include "dxcapi.h"

#pragma comment(lib,"dxcompiler.lib")

class WinApp;

class DirectXCommon
{
public:

	//初期化
	void Initialize(WinApp* winApp);
	
	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
		//CompilerするShaderファイルへのパス
		const std::wstring& filePath,
		//Compilerに使用するProfile
		const wchar_t* profile
	);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource();

	//デバイスの初期化
	void InitializeDevice();

	//コマンド関連の初期化
	void InitializeCommand();

	//スワップチェーンの生成
	void CreateSwapChain();

	//深度バッファの生成
	void CreateDepthStencil();

	//各種デスクリプタヒープの生成
	void CreateDescriptorHeap();

	//レンダーターゲットビューの初期化
	void InitializeRenderTargetView();

	/// <summary>
	/// 指定番号のCPUデスクリプタハンドルを取得する
	/// </summary>
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/// <summary>
	/// 指定番号のGPUデスクリプタハンドルを取得する
	/// </summary>
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

	/// <summary>
	/// SRV指定番号のCPUデスクリプタハンドルう取得する
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);

	/// <summary>
	/// SRV指定番号のGPUデスクリプタハンドルう取得する
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

	/// <summary>
	/// 深度ステンシルビューの初期化
	/// </summary>
	void InitializeDSV();

	/// <summary>
	/// フェンスの初期化
	/// </summary>
	void InitializeFence();

	/// <summary>
	/// ビューポート短形の初期化
	/// </summary>
	void InitializeViewportRect();

	/// <summary>
	/// シザリング短形の初期化
	/// </summary>
	void InitializeScissorRect();

	/// <summary>
	/// DXCコンパイラの生成
	/// </summary>
	void CreateDXCCompiler();

	/// <summary>
	/// ImGuiの初期化
	/// </summary>
	void InitializeImGui();

private:
	
	HRESULT hr_;
	//WindowsAPI
	WinApp* winApp_ = nullptr;
	//DXGIファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;
	//デバイスの生成
	Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;
	//コマンドアロケーターの生成
 	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;
	//コマンドリストの生成
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;
	//コマンドキューの生成
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
	//スワップチェーンの生成
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
	//スワップチェインを生成する
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;

	//デスクリプタのサイズ
	uint32_t descriptorSizeSRV_;
	uint32_t descriptorSizeRTV_;
	uint32_t descriptorSizeDSV_;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;

	//SwapChainからResourceを引っ張ってくる
	//ID3D12Resource* swapChainResources_[2] = { nullptr };
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2>swapChainResources_;

	//RTVの生成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};

	//フェンスの生成
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;

	//ビューポートを生成
	D3D12_VIEWPORT viewport_{};

	//シザーを作成
	D3D12_RECT scissorRect_{};

	////dxCompilerを初期化
	//IDxcCompiler3* dxcCompiler_ = nullptr;

	////インクルードハンドラー
	//IDxcIncludeHandler* includeHandler_ = nullptr;
};

