#include "Cloud.h"
#include "Camera.h"
#include "CloudSetup.h"
#include "DirectXCore.h"
#include "Logger.h"
#include "TextureManager.h"
#include "externals/imgui/imgui.h"
#include <array>
#include <stdexcept>

void Cloud::Initialize(CloudSetup *setup) {
	if (!setup) {
		throw std::invalid_argument("Cloud::Initialize requires CloudSetup.");
	}
	setup_ = setup;
	CreateFullscreenVertexBuffer();
	CreateConstantBuffers();

	// デフォルト値の設定 - より確実に見えるように調整
	paramsCPU_.cloudSize = {300.0f, 100.0f, 300.0f};
	paramsCPU_.cloudCenter = {0.0f, 150.0f, 0.0f};
	paramsCPU_.density = 3.0f;	// 非常に高密度
	paramsCPU_.coverage = 0.3f; // 低いカバレッジ
	paramsCPU_.stepSize = 5.0f;
	paramsCPU_.baseNoiseScale = 0.01f;	 // より大きなスケール
	paramsCPU_.detailNoiseScale = 0.03f; // より大きなスケール
	paramsCPU_.detailWeight = 0.3f;
	paramsCPU_.debugFlag = 0.0f; // デバッグモード無効化

	Logger::Log("Cloud initialized", Logger::LogLevel::Info);
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

void Cloud::SetPosition(const Vector3 &pos) {
	transform_.translate = pos;
	paramsCPU_.cloudCenter = pos; // 直接設定
}

void Cloud::SetScale(const Vector3 &scale) {
	transform_.scale = scale;
	// スケールは使わず、SetSizeを使う
}

void Cloud::UpdateCloudParams() {
	// この関数は使わない - 直接cloudCenterを更新
}

void Cloud::Update(const Camera &camera, float deltaTime) {
	if (!enabled_)
		return;

	accumulatedTime_ += deltaTime;
	paramsCPU_.time = accumulatedTime_;

	paramsCPU_.cloudCenter = transform_.translate;

	const Matrix4x4 viewProj = camera.GetViewProjectionMatrix();
	cameraData_->invViewProj = Inverse4x4(viewProj);
	cameraData_->cameraPosition = camera.GetTransform().translate;
	cameraData_->nearPlane = 0.1f;
	cameraData_->farPlane = 10000.0f;

	*paramsData_ = paramsCPU_;
}

void Cloud::Draw() {
	if (!enabled_ || !setup_ || !vertexBuffer_ || !cameraCB_ || !paramsCB_) {
		return;
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

	ImGui::Checkbox("Enabled", &enabled_);
	ImGui::Checkbox("Debug Mode", (bool *)&paramsCPU_.debugFlag);
	ImGui::Separator();

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::DragFloat3("Position", &transform_.translate.x, 5.0f, -2000.0f, 2000.0f)) {
			paramsCPU_.cloudCenter = transform_.translate;
		}
		ImGui::DragFloat3("Size", &paramsCPU_.cloudSize.x, 5.0f, 10.0f, 1000.0f);
		if (ImGui::Button("Reset Position")) {
			transform_.translate = {0.0f, 150.0f, 0.0f};
			paramsCPU_.cloudCenter = transform_.translate;
		}
		if (ImGui::Button("Move to Camera Front")) {
			if (cameraData_) {
				Vector3 forward = {0.0f, 0.0f, 1.0f}; // カメラの前方方向を簡易的に計算
				transform_.translate.x = cameraData_->cameraPosition.x + forward.x * 200.0f;
				transform_.translate.y = cameraData_->cameraPosition.y + 50.0f;
				transform_.translate.z = cameraData_->cameraPosition.z + forward.z * 200.0f;
				paramsCPU_.cloudCenter = transform_.translate;
			}
		}
		if (ImGui::Button("Set Default Visible Params")) {
			paramsCPU_.density = 3.0f;
			paramsCPU_.coverage = 0.3f;
			paramsCPU_.baseNoiseScale = 0.01f;
			paramsCPU_.detailNoiseScale = 0.03f;
			paramsCPU_.ambient = 0.4f;
			paramsCPU_.sunIntensity = 2.0f;
		}
	}

	if (ImGui::CollapsingHeader("Density & Coverage")) {
		ImGui::SliderFloat("Density", &paramsCPU_.density, 0.0f, 10.0f);
		ImGui::SliderFloat("Coverage", &paramsCPU_.coverage, 0.0f, 1.0f);
		ImGui::SliderFloat("Detail Weight", &paramsCPU_.detailWeight, 0.0f, 1.0f);
		ImGui::Text("Tip: Lower coverage = more visible clouds");
	}

	if (ImGui::CollapsingHeader("Noise Settings")) {
		ImGui::SliderFloat("Base Noise Scale", &paramsCPU_.baseNoiseScale, 0.0001f, 0.05f, "%.5f");
		ImGui::SliderFloat("Detail Noise Scale", &paramsCPU_.detailNoiseScale, 0.001f, 0.1f, "%.4f");
		ImGui::SliderFloat("Noise Speed", &paramsCPU_.noiseSpeed, 0.0f, 0.2f);
		ImGui::Text("Tip: Larger scale = bigger cloud features");
	}

	if (ImGui::CollapsingHeader("Lighting")) {
		ImGui::DragFloat3("Sun Direction", &paramsCPU_.sunDirection.x, 0.01f, -1.0f, 1.0f);
		ImGui::ColorEdit3("Sun Color", &paramsCPU_.sunColor.x);
		ImGui::SliderFloat("Sun Intensity", &paramsCPU_.sunIntensity, 0.0f, 5.0f);
		ImGui::SliderFloat("Ambient", &paramsCPU_.ambient, 0.0f, 1.0f);
		ImGui::SliderFloat("Anisotropy", &paramsCPU_.anisotropy, -1.0f, 1.0f);
		ImGui::SliderFloat("Shadow Density", &paramsCPU_.shadowDensityMultiplier, 0.0f, 3.0f);
	}

	if (ImGui::CollapsingHeader("Raymarching")) {
		ImGui::SliderFloat("Step Size", &paramsCPU_.stepSize, 0.5f, 20.0f);
		ImGui::SliderFloat("Light Step Size", &paramsCPU_.lightStepSize, 5.0f, 50.0f);
		ImGui::SliderFloat("Max Distance", &paramsCPU_.maxDistance, 100.0f, 5000.0f);
	}

	ImGui::Separator();
	ImGui::Text("Debug Info");
	ImGui::Text("Time: %.2f", paramsCPU_.time);
	if (cameraData_) {
		ImGui::Text("Camera: (%.1f, %.1f, %.1f)",
					cameraData_->cameraPosition.x,
					cameraData_->cameraPosition.y,
					cameraData_->cameraPosition.z);

		// カメラから雲までの距離
		float dx = paramsCPU_.cloudCenter.x - cameraData_->cameraPosition.x;
		float dy = paramsCPU_.cloudCenter.y - cameraData_->cameraPosition.y;
		float dz = paramsCPU_.cloudCenter.z - cameraData_->cameraPosition.z;
		float distance = sqrtf(dx * dx + dy * dy + dz * dz);
		ImGui::Text("Distance to Cloud: %.1f", distance);
	}
	ImGui::Text("Center: (%.1f, %.1f, %.1f)",
				paramsCPU_.cloudCenter.x, paramsCPU_.cloudCenter.y, paramsCPU_.cloudCenter.z);
	ImGui::Text("Size: (%.1f, %.1f, %.1f)",
				paramsCPU_.cloudSize.x, paramsCPU_.cloudSize.y, paramsCPU_.cloudSize.z);

	// AABB表示
	Vector3 boxMin = {
		paramsCPU_.cloudCenter.x - paramsCPU_.cloudSize.x * 0.5f,
		paramsCPU_.cloudCenter.y - paramsCPU_.cloudSize.y * 0.5f,
		paramsCPU_.cloudCenter.z - paramsCPU_.cloudSize.z * 0.5f};
	Vector3 boxMax = {
		paramsCPU_.cloudCenter.x + paramsCPU_.cloudSize.x * 0.5f,
		paramsCPU_.cloudCenter.y + paramsCPU_.cloudSize.y * 0.5f,
		paramsCPU_.cloudCenter.z + paramsCPU_.cloudSize.z * 0.5f};
	ImGui::Text("AABB Min: (%.1f, %.1f, %.1f)", boxMin.x, boxMin.y, boxMin.z);
	ImGui::Text("AABB Max: (%.1f, %.1f, %.1f)", boxMax.x, boxMax.y, boxMax.z);

	ImGui::End();
}
