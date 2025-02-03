#include "DirectXCommon.h"
#include <cassert>
#include <format>
#include "Logger.h"
#include "StringUtility.h"
#include <dxcapi.h>
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/imgui/imgui_impl_win32.h"


#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace Microsoft::WRL;

void DirectXCommon::Initialize(WinApp* winApp)
{
	//NULL検出
	assert(winApp);
	//メンバ変数に記録
	this->winApp_ = winApp;

	InitializeDevice();
	InitializeCommand();
	CreateSwapChain();
	CreateDescriptorHeap();
	CreateDepthStencil();
	InitializeRenderTargetView();
	InitializeDSV();
	InitializeFence();
	InitializeViewportRect();
	InitializeScissorRect();
	CreateDXCCompiler();
	InitializeImGui();
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateDepthStencilTextureResource()
{
	//生成するリソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = winApp_->kClientWidth;//textureの幅
	resourceDesc.Height = winApp_->kClientHeight;//textureの高さ
	resourceDesc.MipLevels = 1;//mipMapの数
	resourceDesc.DepthOrArraySize = 1;//奥行or配列textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//depthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1;//サンプリングカット　1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//depthTencilとして使う通知

	//利用するHEAPの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;//VRAM上に作る

	//深度のクリア設定
	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.DepthStencil.Depth = 1.0f;//1.0f(最大値)でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//フォーマット Resourceと合わせる

	//Resourceの設定
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device_->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));
	return resource;
}

Microsoft::WRL::ComPtr<IDxcBlob> DirectXCommon::CompileShader(const std::wstring& filePath, const wchar_t* profile)
{
	//1.hlslファイルを読む
	//これからシェーダをコンパイルする旨をログに出す
	Logger::Log(StringUtility::ConvertString(std::format(L"Begin CompileShader, path:{},profile:{}\n", filePath, profile)));
	//hlslファイルを読む
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	//読めなかったら止める
	assert(SUCCEEDED(hr));
	//読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	//2.Compileする
	LPCWSTR arguments[] =
	{
		filePath.c_str(),		//コンパイル対象のhlsl名
		L"-E",L"main",			//エントリーポイントの指定、基本的にmain以外にはしない
		L"-T",profile,			//ShaderProfileの設定
		L"-Zi",L"-Qembed_debug",//デバッグ用の情報を埋め込む
		L"-Od",					//最適化を外しておく
		L"-Zpr",				//メモリレイアウトは行優先
	};
	//実際にshaderをコンパイルする
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler_->Compile
	(
		&shaderSourceBuffer,				//読み込んだファイル
		arguments,						//コンパイルオプション
		_countof(arguments),			//コンパイルオプションの数
		includeHandler_.Get(),		//includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult)	//コンパイル結果
	);
	//コンパイルエラーではなくdxcが起動できないなど致命的な状況
	assert(SUCCEEDED(hr));

	//3.警告・エラーが出ていないか確認する
	//警告・エラーが出てたらログに出して止める
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
	{
		Logger::Log(shaderError->GetStringPointer());
		//警告・エラーダメ！
		assert(false);
	}

	//4.compile結果を受け取って返す
	//コンパイル結果から実行用のバイナリ部分を取得
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	//成功したログを出す
	Logger::Log(StringUtility::ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));
	//もう使わないリソースを解放
	shaderSource->Release();
	shaderResult->Release();
	//実行用のバイナリを返却
	return shaderBlob;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{
	//ディスクリプタヒープを生成する
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;//レンダーターゲットビュー用
	descriptorHeapDesc.NumDescriptors = numDescriptors;//ダブルバッファように２つ、多くても構わない
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	//ディスクリプタヒープが作れなかったので起動できない
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

void DirectXCommon::InitializeDevice()
{
#ifdef _DEBUG

	ID3D12Debug1* debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		//デバッグレイヤーを有効化する
		debugController->EnableDebugLayer();
		//さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}

#endif // DEBUG

	//メインスレッドではMTAでCOM利用
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	//HRESULTはWindows系のエラーコードでありzz
	//関数が成功したかどうかをSUCCEEDEDマクロで判定できる
	hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));

	//初期化の根本的な部分でエラーが出た場合はコードが間違っているか
	//どうにもできない場合が多いのでassertにしておく
	assert(SUCCEEDED(hr));


	//使用するアダプタ用の変数　最初にnullptrを代入しておく
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

	//良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(
		i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		//アダプタの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));//取得できないのは一大事
		//ソフトウェアアダプタでなければ採用！
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			//採用したアダプタの情報をログに出力　wstringの方なので注意
			Logger::Log(StringUtility::ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;//ソフトウェアアダプタの場合は見なかったことにする
	}
	//適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);

	//機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i)
	{
		//採用したアダプタでデバイスを生成
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		//指定した機能レベルでデバイスが生成できたか確認
		if (SUCCEEDED(hr))
		{
			//生成できたのでログ出力を行ってループを抜ける
			Logger::Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}

	//デバイスの生成が上手くいかなかったので起動できない
	assert(device_ != nullptr);
	Logger::Log("Comprete Create D3D12Device!\n");


#ifdef _DEBUG

	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		//ヤバいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		//エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		//警告時に止まる
		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		//解放
		infoQueue->Release();

		//抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] =
		{
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		//抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		//指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);
	}

#endif // DEBUG
}

void DirectXCommon::InitializeCommand()
{
	//コマンドアロケーターを生成する
	hr_ = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	//コマンドアロケーターの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr_));

	//コマンドリストを生成する
	hr_ = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
	//コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr_));

	//コマンドキューを生成する
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr_ = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	//コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr_));
}

void DirectXCommon::CreateSwapChain()
{
	swapChainDesc.Width = WinApp::kClientWidth;					//画面の幅　ウィンドウのクライアント領域を同じものにしておく				
	swapChainDesc.Height = WinApp::kClientHeight;				//画面の高さ　ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			//色の形式
	swapChainDesc.SampleDesc.Count = 1;							//マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;//描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2;								//ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	//モニタにうつしたら中身を破棄
	//コマンドキュー、ウィンドウバンドル、設定を渡して生成する
	hr_ = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_.Get(), winApp_->GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
	assert(SUCCEEDED(hr_));
}

void DirectXCommon::CreateDepthStencil()
{
	depthStencilResource_ = CreateDepthStencilTextureResource();

	//DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//DSVHeapの先頭にDSVをつくる
	device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::CreateDescriptorHeap()
{
	//DescriptorSizeを取得しておく
	descriptorSizeSRV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeRTV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//ディスクリプタヒープを生成する
	srvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	rtvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	dsvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

void DirectXCommon::InitializeRenderTargetView()
{
	//上手く取得できなければ起動できない
	hr_ = swapChain_->GetBuffer(0, IID_PPV_ARGS(&swapChainResources_[0]));
	assert(SUCCEEDED(hr_));
	hr_ = swapChain_->GetBuffer(1, IID_PPV_ARGS(&swapChainResources_[1]));
	assert(SUCCEEDED(hr_));

	rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//出力結果をSRGBに変換して書き込む
	rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;//2Dテクスチャとして書き込む
	//ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	//裏表の２つ分
	for (uint32_t i = 0; i < 2; ++i)
	{
		//まず１つ目を作る。１つ目は最初の所に作る。作る場所をこちらが指定して上げる必要がある。
		rtvHandles[0] = rtvStartHandle;
		//２つ目のディスクリプタハンドルを得る(自力で)
		rtvHandles[1].ptr = rtvHandles[0].ptr + device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		//２つ目を作る
		device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc_, rtvHandles[i]);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index)
{
	return GetCPUDescriptorHandle(srvDescriptorHeap_.Get(), descriptorSizeSRV_, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index)
{
	return GetGPUDescriptorHandle(srvDescriptorHeap_.Get(), descriptorSizeSRV_, index);
}

void DirectXCommon::InitializeDSV()
{
	//DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	//DSVHeapの先頭にDSVをつくる
	device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::InitializeFence()
{
	//初期値0でFenceを作る
	fence_ = nullptr;
	fenceValue = 0;
	hr_ = device_->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr_));

	//fenceのsignalを待つためのイベントを作成する
	fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);
}

void DirectXCommon::InitializeViewportRect()
{
	//クライアント領域のサイズと一緒にして画面全体に表示
	viewport_.Width = WinApp::kClientWidth;
	viewport_.Height = WinApp::kClientHeight;
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
}

void DirectXCommon::InitializeScissorRect()
{
	//基本的にビューポートと同じ短径が構成されるようにする
	scissorRect_.left = 0;
	scissorRect_.right = WinApp::kClientWidth;
	scissorRect_.top = 0;
	scissorRect_.bottom = WinApp::kClientHeight;
}

void DirectXCommon::CreateDXCCompiler()
{
	hr_ = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr_));
	hr_ = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr_));

	//現時点でincludeしないが、includeに対応するための設定を行っておく
	hr_ = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr_));
}

void DirectXCommon::InitializeImGui()
{
	//ImGuiの初期化
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp_->GetHwnd());
	ImGui_ImplDX12_Init
	(
		device_.Get(),
		swapChainDesc.BufferCount,
		rtvDesc_.Format,
		srvDescriptorHeap_.Get(),
		srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart()
	);
}

void DirectXCommon::PreDraw()
{
	//これから書き込むバックバッファのインデックスを取得
	backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
	
	//TransitionBarrierの設定
	//今回のバリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//バリアを張る対象のリソース。現在のバックバッファに対して行う。
	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
	//還移前(現在)のResourceState
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	//還移後のResourceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//TranslationのBarrier
	commandList_->ResourceBarrier(1, &barrier);
	
	//描画先のRTVとDSVを設定する
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList_->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);

	//指定した深度で画面全体ををクリアする
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//指定した色で画面全体をクリアする
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	commandList_->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

	//描画用のDescriptorHeapの設定
	ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get()};
	commandList_->SetDescriptorHeaps(1, descriptorHeaps);

	//ビューポート
	D3D12_VIEWPORT viewport{};
	//クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = winApp_->kClientWidth;
	viewport.Height = winApp_->kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//シザーを作成
	D3D12_RECT scissorRect{};
	//基本的にビューポートと同じ短径が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = winApp_->kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = winApp_->kClientHeight;
}

void DirectXCommon::PostDraw()
{
	//これから書き込むバックバッファのインデックスを取得
	backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	//画面に描画する処理は全て終わり、画面に映すので、状態を遷移
	//今回はRenderTargetからPresent
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	//TranslationBarrierを張る
	commandList_->ResourceBarrier(1, &barrier);

	//コマンドリストの内容を確定させる　全てのコマンドを積んでからCloseすること
	hr_ = commandList_->Close();
	assert(SUCCEEDED(hr_));

	//GPUにコマンドリストの実行を行わせる
	ID3D12CommandList* commandLists[] = { commandList_.Get()};
	commandQueue_->ExecuteCommandLists(1, commandLists);
	//GPUとOSに画面の交換を行うよう通知する
	swapChain_->Present(1, 0);

	//fenceの値を更新
	fenceValue++;
	//GPUがここまでたどり着いたときに、fenceの値を指定した値に代入するようにsignalを送る
	commandQueue_->Signal(fence_.Get(), fenceValue);

	//fenceの値が指定したsignal値にたどり着いているか確認する
	//GetCompletedValueの初期値はfence作成時に渡した初期値
	if (fence_->GetCompletedValue())
	{
		//指定したsignalにたどりついていないので、たどりつくまで待つようにイベントを設定する
		fence_->SetEventOnCompletion(fenceValue, fenceEvent);
		//イベントを待つ
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	
	//次のフレーム用のコマンドリストを準備
	hr_ = commandAllocator_->Reset();
	assert(SUCCEEDED(hr_));
	hr_ = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr_));
}
