#include "Cloud.h"
#include "Camera.h"
#include "CloudSetup.h"
#include "DirectXCore.h"
#include "TextureManager.h"
#include <array>
#include <stdexcept>

void Cloud::Initialize(CloudSetup *setup) {
	if (!setup) {
		throw std::invalid_argument("Cloud::Initialize requires CloudSetup.");
	}
	setup_ = setup;
	CreateFullscreenVertexBuffer();
	CreateConstantBuffers();
}

void Cloud::CreateFullscreenVertexBuffer() {
	const std::array<FullscreenVertex, 3> vertices{{
		{{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
		{{-1.0f, 3.0f, 0.0f}, {0.0f, -1.0f}},
		{{3.0f, -1.0f, 0.0f}, {2.0f, 1.0f}},
	}};

	auto dxCore = setup_->GetDXCore();
	size_t bufferSize = sizeof(vertices);
	vertexBuffer_ = dxCore->CreateBufferResource(bufferSize);

	void *mapped = nullptr;
	D3D12_RANGE readRange{0, 0};
	vertexBuffer_->Map(0, &readRange, &mapped);
	std::memcpy(mapped, vertices.data(), bufferSize);
	vertexBuffer_->Unmap(0, nullptr);

	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(bufferSize);
	vertexBufferView_.StrideInBytes = sizeof(FullscreenVertex);
}

void Cloud::CreateConstantBuffers() {
	auto dxCore = setup_->GetDXCore();

	size_t cameraSize = (sizeof(CloudCameraConstant) + 255) & ~255;
	cameraCB_ = dxCore->CreateBufferResource(cameraSize);
	cameraCB_->Map(0, nullptr, reinterpret_cast<void **>(&cameraData_));
	*cameraData_ = {};

	size_t paramsSize = (sizeof(CloudRenderParams) + 255) & ~255;
	paramsCB_ = dxCore->CreateBufferResource(paramsSize);
	paramsCB_->Map(0, nullptr, reinterpret_cast<void **>(&paramsData_));
	*paramsData_ = paramsCPU_;
}

void Cloud::Update(const Camera &camera, float deltaTime) {
	accumulatedTime_ += deltaTime;
	paramsCPU_.time = accumulatedTime_;

	const Matrix4x4 invViewProj = Inverse4x4(camera.GetViewProjectionMatrix());
	cameraData_->invViewProj = invViewProj;
	cameraData_->cameraPosition = camera.GetTransform().translate;

	*paramsData_ = paramsCPU_;
}

void Cloud::Draw() {
	if (!setup_ || !vertexBuffer_ || !cameraCB_ || !paramsCB_) {
		throw std::runtime_error("Cloud::Draw missing initialization.");
	}

	setup_->CommonDrawSetup();
	auto commandList = setup_->GetDXCore()->GetCommandList();

	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->SetGraphicsRootConstantBufferView(0, cameraCB_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(1, paramsCB_->GetGPUVirtualAddress());

	if (hasWeatherMapSrv_) {
		commandList->SetGraphicsRootDescriptorTable(2, weatherMapSrv_);
	}

	commandList->DrawInstanced(3, 1, 0, 0);
}

void Cloud::SetWeatherMap(D3D12_GPU_DESCRIPTOR_HANDLE srv) {
	weatherMapSrv_ = srv;
	hasWeatherMapSrv_ = srv.ptr != 0;
}
