#include "Cloud.h"
#include "Camera.h"
#include "CloudSetup.h"
#include "DirectXCore.h"
#include "TextureManager.h"
#include "externals/imgui/imgui.h" // ImGuiをインクルード
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

	// Transformを雲のパラメータに反映
	paramsCPU_.cloudPosition = transform_.translate;
	paramsCPU_.cloudScale = transform_.scale.x; // スケールの平均値を使用

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

void Cloud::DrawImGui() {
	ImGui::Begin("Cloud Settings");

	ImGui::Text("Transform");
	ImGui::DragFloat3("Position", &transform_.translate.x, 1.0f, -1000.0f, 1000.0f);
	ImGui::DragFloat3("Scale", &transform_.scale.x, 0.1f, 0.1f, 10.0f);
	ImGui::SliderFloat("Cloud Radius", &paramsCPU_.cloudRadius, 100.0f, 2000.0f);

	ImGui::Separator();
	ImGui::Text("Rendering");
	ImGui::SliderFloat("Density", &paramsCPU_.density, 0.0f, 2.0f);
	ImGui::SliderFloat("Coverage", &paramsCPU_.coverage, 0.0f, 1.0f);
	ImGui::SliderFloat("Max Distance", &paramsCPU_.maxDistance, 100.0f, 5000.0f);

	ImGui::Separator();
	ImGui::Text("Height");
	ImGui::SliderFloat("Base Height", &paramsCPU_.baseHeight, 0.0f, 500.0f);
	ImGui::SliderFloat("Height Range", &paramsCPU_.heightRange, 50.0f, 1000.0f);

	ImGui::Separator();
	ImGui::Text("Noise");
	ImGui::SliderFloat("Base Scale", &paramsCPU_.baseNoiseScale, 0.001f, 0.02f, "%.4f");
	ImGui::SliderFloat("Detail Scale", &paramsCPU_.detailNoiseScale, 0.005f, 0.05f, "%.4f");
	ImGui::SliderFloat("Detail Weight", &paramsCPU_.detailWeight, 0.0f, 1.0f);

	ImGui::Separator();
	ImGui::Text("Lighting");
	ImGui::DragFloat3("Sun Direction", &paramsCPU_.sunDirection.x, 0.01f, -1.0f, 1.0f);
	ImGui::ColorEdit3("Sun Color", &paramsCPU_.sunColor.x);
	ImGui::SliderFloat("Sun Intensity", &paramsCPU_.sunIntensity, 0.0f, 3.0f);
	ImGui::SliderFloat("Ambient", &paramsCPU_.ambient, 0.0f, 1.0f);
	ImGui::SliderFloat("Anisotropy", &paramsCPU_.anisotropy, -1.0f, 1.0f);

	ImGui::Separator();
	ImGui::Text("Raymarching");
	ImGui::SliderFloat("Step Length", &paramsCPU_.stepLength, 0.5f, 10.0f);
	ImGui::SliderFloat("Light Step", &paramsCPU_.lightStepLength, 1.0f, 50.0f);
	ImGui::SliderFloat("Shadow Density", &paramsCPU_.shadowDensity, 0.0f, 3.0f);

	ImGui::Separator();
	ImGui::Text("Debug");
	ImGui::Text("Time: %.2f", paramsCPU_.time);
	ImGui::Text("Weather Map: %s", hasWeatherMapSrv_ ? "Loaded" : "Not Set");

	ImGui::End();
}
