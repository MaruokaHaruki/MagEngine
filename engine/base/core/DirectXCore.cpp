/*********************************************************************
 * \file   DirectXCore.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#include "DirectXCore.h"
#include "PostEffectManager.h"
#include "TextureManager.h"
//========================================
// 標準ライブラリ
#include <vector>
//========================================
// DirectXTex
#include "d3dx12.h"
#pragma comment(lib, "winmm.lib")
// HOTFIX:リンクエラー対策
#include <fstream>
#include <iostream>
#include <string>

///=============================================================================
///						描画前処理
// TODO:ループ内の前処理後処理を作成
void DirectXCore::PreDraw(PostEffectManager *postEffectManager) {
	/// バックバッファの決定
	SettleCommandList();
	/// バリア設定
	// SetupTransitionBarrier();
	//  描画ターゲットの設定とクリア
	// RenderTargetPreference();

	// ViewPortとScissorRectの設定
	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_);

	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// デフォルトのレンダーテクスチャ描画
	commandList_->SetGraphicsRootSignature(renderTextureRootSignature_.Get());
	commandList_->SetPipelineState(renderTextureGraphicsPipelineState_.Get());

	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle;

	if (renderResourceIndex_ == 0) {
		srvHandle = TextureManager::GetInstance()->GetSrvHandleGPU("RenderTexture0");
	} else {
		srvHandle = TextureManager::GetInstance()->GetSrvHandleGPU("RenderTexture1");
	}

	// srvHandle.ptr が 0 または異常な値でないか確認
	assert(srvHandle.ptr != 0);

	commandList_->SetGraphicsRootDescriptorTable(0, srvHandle);

	commandList_->DrawInstanced(3, 1, 0, 0);

	//========================================
	// ポストエフェクトがある場合は適用
	// TODO:ポストエフェクトマネージャをDirectXCoreに持たせるか検討
	if (postEffectManager) {
		postEffectManager->ApplyEffects();
	}
}

///=============================================================================
///						描画後処理
void DirectXCore::PostDraw() {
	// FPS固定
	UpdateFixFPS();
	// コマンドリストのクローズと実行
	// CloseCommandList();
	ExecuteCommandList();
}

///=============================================================================
///						DirectXの初期化
void DirectXCore::InitializeDirectX(WinApp *winApp) {
	// FPS固定初期化
	InitializeFixFPS();

	// WinApp NULL検出
	assert(winApp);
	this->winApp_ = winApp;

	//=======================================
	// 新しいマネージャークラスで初期化
	graphicsDevice_.Initialize(winApp);
	commandContext_.Initialize(graphicsDevice_.GetDevice());
	swapChainManager_.Initialize(
		graphicsDevice_.GetFactory(),
		graphicsDevice_.GetDevice(),
		commandContext_.GetCommandQueue(),
		winApp);
	renderTargetManager_.Initialize(graphicsDevice_.GetDevice());
	shaderCompiler_.Initialize();

	//=======================================
	// 深度バッファの生成
	depthStencilResource_ = ResourceFactory::CreateDepthStencil(
		graphicsDevice_.GetDevice(),
		winApp->GetWindowWidth(),
		winApp->GetWindowHeight());

	// DSV作成
	renderTargetManager_.CreateDSV(depthStencilResource_, graphicsDevice_.GetDevice());
	dsvHandle_ = renderTargetManager_.GetDSVHandle();

	//=======================================
	// スワップチェーンのRTV作成
	for (uint32_t i = 0; i < 2; i++) {
		renderTargetManager_.CreateRTV(i, swapChainManager_.GetBackBuffer(i), graphicsDevice_.GetDevice());
		rtvHandles_[i] = renderTargetManager_.GetRTVHandle(i);
	}

	//=======================================
	// レンダーテクスチャのRTVを生成
	CreateRenderTextureRTV();

	//=======================================
	// ビューポートとシザーレクトの生成
	CreateViewportAndScissorRect();

	//=======================================
	// 初回コマンド実行
	commandContext_.Begin();
	commandContext_.Close();
	commandContext_.Execute();
	commandContext_.WaitForGPU();

	//=======================================
	// オフスクリーンの初期化
	CreateOffScreenPipeLine();
}

///=============================================================================
///						開放処理
void DirectXCore::ReleaseDirectX() {
	commandContext_.Finalize();
	graphicsDevice_.Finalize();
	shaderCompiler_.Finalize();
	// ReleaseResources();
}

///=============================================================================
///						コマンド積み込んで確定させる
void DirectXCore::SettleCommandList() {
	backBufferIndex_ = swapChainManager_.GetCurrentBackBufferIndex();
}

///=============================================================================
///						コマンドのキック
void DirectXCore::ExecuteCommandList() {
	// コマンド実行
	commandContext_.Execute();

	// Present
	swapChainManager_.Present(true);

	// GPU待機
	commandContext_.WaitForGPU();

	// 次フレーム用のコマンドリストを準備
	commandContext_.Begin();
}

///=============================================================================
///						シェーダーのコンパイル
IDxcBlob *DirectXCore::CompileShader(const std::wstring &filePath, const wchar_t *profile) {
	return shaderCompiler_.CompileShader(filePath, profile);
}

///=============================================================================
///						バッファーリソースの生成
Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCore::CreateBufferResource(size_t sizeInByte) {
	return ResourceFactory::CreateBuffer(graphicsDevice_.GetDevice(), sizeInByte);
}

///=============================================================================
///						テクスチャリソースの生成
Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCore::CreateTextureResource(const DirectX::TexMetadata &metadata) {
	return ResourceFactory::CreateTexture(graphicsDevice_.GetDevice(), metadata);
}

///=============================================================================
///						テクスチャデータの転送
Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCore::UploadTextureData(
	Microsoft::WRL::ComPtr<ID3D12Resource> texture,
	const DirectX::ScratchImage &mipImages) {
	return ResourceFactory::UploadTexture(
		graphicsDevice_.GetDevice(),
		commandContext_.GetCommandList(),
		texture,
		mipImages);
}

///=============================================================================
///						DXCコンパイラーの初期化
void DirectXCore::CreateDXCCompiler() {
	// dxcCompilerを初期化
	hr_ = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr_));
	hr_ = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr_));
	// 現時点でincludeはしないが、includeに対応するために設定を行う
	hr_ = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr_));
}

///=============================================================================
///						深度BufferステンシルBufferの生成関数
Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCore::CreateDepthStencilTextureResource(int32_t width, int32_t height) {
	//=======================================
	// 生成するResouceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;									  // テクスチャの幅
	resourceDesc.Height = height;								  // テクスチャの高さ
	resourceDesc.MipLevels = 1;									  // mipmapの数
	resourceDesc.DepthOrArraySize = 1;							  // 奥行きor配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;		  // DepthStencillとして利用可能なFormat
	resourceDesc.SampleDesc.Count = 1;							  // サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencillとして使う通知
	//=======================================
	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る
	//=======================================
	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;				// 1.0F(最大値)でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット。Resourceと合わせる
	//=======================================
	// 設定を元にResourceの生成を行う
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device_->CreateCommittedResource(
		&heapProperties,				  // Heapの設定
		D3D12_HEAP_FLAG_NONE,			  // heepの特殊な設定。特になし。
		&resourceDesc,					  // Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にしておく
		&depthClearValue,				  // Clear最適値
		IID_PPV_ARGS(&resource));		  // 作成するResourceポインタへのポインタ
										  //========================================
										  // エラーチェック
#ifdef _DEBUG
	assert(SUCCEEDED(hr));
#endif // _DEBUG
	if (FAILED(hr)) {
		// 深度ステンシルテクスチャの生成がうまくいかなかったので起動できない
		Log("Failed to create depth stencil texture resource.", LogLevel::Error);
		return nullptr;
	}
	//========================================
	// 出力
	return resource;
}

///=============================================================================
///						DescriptorHeapの生成関数
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCore::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	// NOTE:debugモードでのみエラーチェックを行う理由は、assertを使っているため
	HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
#ifdef _DEBUG
	assert(SUCCEEDED(hr));
#endif // _DEBUG
	//
	if (FAILED(hr)) {
		// ディスクリプタヒープの生成がうまくいかなかったので起動できない
		Log("Failed to Create Descriptor Heap.");
		return nullptr;
	}
	// 成功したログを出力
	Log("Descriptor heap created successfully.");
	return descriptorHeap;
}

// HOTFIX :
void WriteToFile(const std::string &fileName, const std::string &text) {
	// 出力ファイルストリームを生成
	std::ofstream outputFile(fileName);

	// ファイルが開けなかった場合のエラーチェック
	if (!outputFile) {
		std::cerr << "ファイルを開けませんでした: " << fileName << std::endl;
		return;
	}

	// 指定の文字列を書き込む
	outputFile << text;

	// ファイルを閉じる（ofstreamはスコープを抜けると自動で閉じられるが明示的に閉じてもよい）
	outputFile.close();

	std::cout << "ファイルに書き込みました: " << fileName << std::endl;
}

///=============================================================================
///						ビューポートとシザーレクトの生成
void DirectXCore::CreateViewportAndScissorRect() {
	//========================================
	// ビューポート
	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport_.Width = static_cast<float>(winApp_->GetWindowWidth());
	viewport_.Height = static_cast<float>(winApp_->GetWindowHeight());
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0;
	viewport_.MaxDepth = 1.0f;
	//========================================
	// シザー矩形
	// 基本的にビューポートと同じ矩形が構成されるようになる
	scissorRect_.left = 0;
	scissorRect_.right = winApp_->GetWindowWidth();
	scissorRect_.top = 0;
	scissorRect_.bottom = winApp_->GetWindowHeight();
}

///=============================================================================
///						オフスクリーン用のルートシグネチャを生成
void DirectXCore::CreateOffScreenRootSignature() {
	HRESULT hr;

	// ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// SRV の Descriptor Range
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;													 // t0: Shader Register
	descriptorRange[0].NumDescriptors = 1;														 // 1つのSRV
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;								 // SRV
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // 自動計算

	// Root Parameter: SRV (gTexture)
	D3D12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // Pixel Shaderで使用
	// rootParameters[0].DescriptorTable.NumDescriptorRanges = 1; // 1つの範囲
	// rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRange; // SRV の範囲

	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;			   // Tableの中身の配列を指定
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数

	// Static Sampler
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;						// 全MipMap使用
	staticSamplers[0].ShaderRegister = 0;								// s0: Shader Register
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // Pixel Shaderで使用

	// ルートシグネチャの構築
	descriptionRootSignature.pParameters = rootParameters;				   // ルートパラメーター配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);	   // 配列の長さ
	descriptionRootSignature.pStaticSamplers = staticSamplers;			   // サンプラー配列へのポインタ
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers); // サンプラーの数

	// シリアライズしてバイナリにする
	// D3D12SerializeRootSignatureは、ルートシグネチャをバイナリ形式に変換する関数
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob_ = nullptr;
	// エラーが発生した場合のBlob
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob_ = nullptr;
	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob_, &errorBlob_);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char *>(errorBlob_->GetBufferPointer()));
		assert(false);
	}

	// バイナリを元に生成
	renderTextureRootSignature_ = nullptr;
	hr = device_->CreateRootSignature(0, signatureBlob_->GetBufferPointer(), signatureBlob_->GetBufferSize(), IID_PPV_ARGS(&renderTextureRootSignature_));
	assert(SUCCEEDED(hr));
}

///--------------------------------------------------------------
///                        オフスクリーン用のパイプラインを生成
void DirectXCore::CreateOffScreenPipeLine() {
	CreateOffScreenRootSignature();

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = nullptr;
	inputLayoutDesc.NumElements = 0;

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};

	// すべての要素数を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// RasiterzerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	// 裏面(時計回り)を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"resources/shader/FullScreen.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"resources/shader/FullScreen.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = false;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = renderTextureRootSignature_.Get();							  // RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;												  // InputLayout
	graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize()}; // VertexShader
	graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize()};	  // PixelShader
	graphicsPipelineStateDesc.BlendState = blendDesc;														  // BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;												  // RasterizerState

	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// 利用するトポロジ(形状)のタイプ.。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// どのように画面に色をつけるか
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 実際に生成
	renderTextureGraphicsPipelineState_ = nullptr;
	device_->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&renderTextureGraphicsPipelineState_));
}

///=============================================================================
///						DescriptorHandleの取得を関数化
///--------------------------------------------------------------
///						 CPU
D3D12_CPU_DESCRIPTOR_HANDLE DirectXCore::GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// NOTE:サブ式よりも先に乗算を行うため、括弧をつける
	handleCPU.ptr += (static_cast<unsigned long long>(descriptorSize) * index);
	return handleCPU;
}

///--------------------------------------------------------------
///						 GPU
D3D12_GPU_DESCRIPTOR_HANDLE DirectXCore::GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	// NOTE:サブ式よりも先に乗算を行うため、括弧をつける
	handleGPU.ptr += (static_cast<unsigned long long>(descriptorSize) * index);
	return handleGPU;
}

///=============================================================================
///						60FPS固定の処理
///--------------------------------------------------------------
///						 InitializeFixFPS
void DirectXCore::InitializeFixFPS() {
	// システムタイマーの分解能を上げる
	timeBeginPeriod(1);
	// 現在時間を記録する
	reference_ = std::chrono::steady_clock::now();
}

///--------------------------------------------------------------
///						 UpdateFixFPS
void DirectXCore::UpdateFixFPS() {
	// 1/60秒ピッタリの時間
	const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
	// 1/60秒よりわずかに短い時間
	const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

	// 現在時間を取得する
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	// 前回記録からの経過時間を取得する
	std::chrono::microseconds elapsed =
		std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	// 1/60秒(よりわずかに短い時間)経っていない場合
	if (elapsed < kMinCheckTime) {
		// 1/60秒経過するまで軽微なスリープを繰り返す
		while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
			// 1マイクロ秒スリープ
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}

	// 現在の時間を記録する
	reference_ = std::chrono::steady_clock::now();
}

///=============================================================================
///						レンダーテクスチャ系
///--------------------------------------------------------------
///						 レンダーテクスチャの前処理
void DirectXCore::RenderTexturePreDraw() {
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = renderTextureResources_[renderResourceIndex_].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandContext_.GetCommandList()->ResourceBarrier(1, &barrier);

	uint32_t renderTargetIndex = 2 + renderResourceIndex_;
	commandContext_.GetCommandList()->OMSetRenderTargets(1, &rtvHandles_[renderTargetIndex], false, &dsvHandle_);

	float clearColor[] = {0.298f, 0.427f, 0.698f, 1.0f};
	commandContext_.GetCommandList()->ClearRenderTargetView(rtvHandles_[renderTargetIndex], clearColor, 0, nullptr);
	commandContext_.GetCommandList()->ClearDepthStencilView(dsvHandle_, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	commandContext_.GetCommandList()->RSSetViewports(1, &viewport_);
	commandContext_.GetCommandList()->RSSetScissorRects(1, &scissorRect_);
}

///--------------------------------------------------------------
///						 レンダーテクスチャの後処理
void DirectXCore::RenderTexturePostDraw() {
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = renderTextureResources_[renderResourceIndex_].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	commandContext_.GetCommandList()->ResourceBarrier(1, &barrier);
}

///--------------------------------------------------------------
///                        レンダーテクスチャのRTVを生成
void DirectXCore::CreateRenderTextureRTV() {
	const Vector4 kRenderTargetClearValue{0.298f, 0.427f, 0.698f, 1.0f};

	// 1つ目の作成
	renderTextureResources_[0] = renderTargetManager_.CreateRenderTexture(
		graphicsDevice_.GetDevice(),
		winApp_->GetWindowWidth(),
		winApp_->GetWindowHeight(),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		kRenderTargetClearValue);

	renderTargetManager_.CreateRTV(2, renderTextureResources_[0], graphicsDevice_.GetDevice());
	rtvHandles_[2] = renderTargetManager_.GetRTVHandle(2);
	renderTextureResources_[0]->SetName(L"renderTexture0");

	// 2つ目の作成
	renderTextureResources_[1] = renderTargetManager_.CreateRenderTexture(
		graphicsDevice_.GetDevice(),
		winApp_->GetWindowWidth(),
		winApp_->GetWindowHeight(),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		kRenderTargetClearValue);

	renderTargetManager_.CreateRTV(3, renderTextureResources_[1], graphicsDevice_.GetDevice());
	rtvHandles_[3] = renderTargetManager_.GetRTVHandle(3);
	renderTextureResources_[1]->SetName(L"renderTexture1");

	renderResourceIndex_ = 0;
	renderTargetIndex_ = 1;
}
