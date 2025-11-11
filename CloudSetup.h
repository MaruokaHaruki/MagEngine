#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class DirectXCore;

class CloudSetup {
public:
	void Initialize(DirectXCore *dxCore);
	void CommonDrawSetup();
	DirectXCore *GetDXCore() const {
		return dxCore_;
	}

private:
	void CreateRootSignature();
	void CreateGraphicsPipeline();

private:
	DirectXCore *dxCore_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
};
