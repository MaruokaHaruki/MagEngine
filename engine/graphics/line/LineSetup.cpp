/*********************************************************************
 * \file   LineSetup.cpp
 * \brie
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#include "LineSetup.h"
#include "engine/render/pipeline/PipelineBuilder.h"
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	///=============================================================================
	///						初期化
	void LineSetup::Initialize(DirectXCore *dxCore, SrvSetup *srvSetup) {
		/// ===引数でdxManagerを受取=== ///
		dxCore_ = dxCore;

		//========================================
		// SrvSetupの取得
		srvSetup_ = srvSetup;

		/// ===グラフィックスパイプラインの生成=== ///
		CreateGraphicsPipeline();
	}

	///=============================================================================
	///						共通化処理
	void LineSetup::CommonDrawSetup(LineRenderMode renderMode) {
		// コマンドリストの取得
		//  NOTE:Getを複数回呼び出すのは非効率的なので、変数に保持しておく
		auto commandList = dxCore_->GetCommandList();
		// ルートシグネイチャのセット
		commandList->SetGraphicsRootSignature(rootSignature_.Get());
		// グラフィックスパイプラインステートをセット
		commandList->SetPipelineState(renderMode == LineRenderMode::Hud ? hudPipelineState_.Get() : worldPipelineState_.Get());
		// プリミティブトポロジーをセットする(Line用にLINELISTに変更)
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	}

	///=============================================================================
	///						ルートシグネチャーの作成
	void LineSetup::CreateRootSignature() {
		// ルートパラメータの設定
		D3D12_ROOT_PARAMETER rootParameters[1] = {};

		// 定数バッファ（TransformationMatrix）の設定（b0、頂点シェーダーで使用）
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[0].Descriptor.ShaderRegister = 0; // b0
		rootParameters[0].Descriptor.RegisterSpace = 0;

		// ルートシグネチャの設定
		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = _countof(rootParameters);
		rootSignatureDesc.pParameters = rootParameters;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// シリアライズとルートシグネチャの作成
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc,
												 D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		if (FAILED(hr)) {
			throw std::runtime_error(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
		}

		hr = dxCore_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
													   signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
		if (FAILED(hr)) {
			throw std::runtime_error("Failed to create root signature");
		}
		Logger::Log("Particle Root signature created successfully :)", Logger::LogLevel::Success);
	}

	///=============================================================================
	///						グラフィックスパイプラインの作成
	PipelineRecipe LineSetup::CreateWorldPipelineRecipe(ID3D12RootSignature *rootSignature) {
		PipelineRecipe recipe{};
		recipe.vertexShader = {L"resources/shader/Line.VS.hlsl", L"main", L"vs_6_0"};
		recipe.pixelShader = {L"resources/shader/Line.PS.hlsl", L"main", L"ps_6_0"};
		recipe.rootSignature = rootSignature;
		recipe.inputLayout = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"THICKNESS", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};
		recipe.blendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		recipe.rasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		recipe.rasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		recipe.depthStencilState.DepthEnable = true;
		recipe.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		recipe.depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		recipe.renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		recipe.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		recipe.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		return recipe;
	}

	PipelineRecipe LineSetup::CreateHudPipelineRecipe(ID3D12RootSignature *rootSignature) {
		PipelineRecipe recipe = CreateWorldPipelineRecipe(rootSignature);
		// HUD系は既存のScene深度に依存させず、画面前面の安定描画を優先する。
		recipe.depthStencilState.DepthEnable = false;
		recipe.depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		return recipe;
	}

	PipelineRecipe LineSetup::CreateRecipe() const {
		return CreateWorldPipelineRecipe(rootSignature_.Get());
	}

	void LineSetup::CreateGraphicsPipeline() {
		CreateRootSignature();
		// NOTE: World/HUDで深度方針を分けるが、PSO生成の責務はBuilderへ閉じる。
		PipelineBuilder builder(*dxCore_->GetDevice().Get(), *dxCore_);
		worldPipelineState_ = builder.CreateGraphicsPipeline(CreateWorldPipelineRecipe(rootSignature_.Get()));
		hudPipelineState_ = builder.CreateGraphicsPipeline(CreateHudPipelineRecipe(rootSignature_.Get()));
		Logger::Log("Line Graphics pipeline states created successfully :)", Logger::LogLevel::Success);
	}
}
