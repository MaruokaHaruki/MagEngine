#pragma once
#include "MathFunc4x4.h"
#include "Transform.h"
#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>

class Camera;
class CloudSetup;

struct alignas(16) CloudCameraConstant {
	Matrix4x4 invViewProj;
	Vector3 cameraPosition;
	float padding = 0.0f;
};

struct alignas(16) CloudRenderParams {
	Vector3 sunDirection{0.3f, 0.8f, 0.5f};
	float time = 0.0f;
	Vector3 sunColor{1.0f, 0.96f, 0.88f};
	float density = 0.7f;
	float baseHeight = 120.0f;
	float heightRange = 260.0f;
	float stepLength = 2.5f;
	float maxDistance = 1000.0f;
	float baseNoiseScale = 0.0045f;
	float detailNoiseScale = 0.018f;
	float detailWeight = 0.35f;
	float weatherMapScale = 0.0f;
	float coverage = 0.45f;
	float ambient = 0.25f;
	float lightStepLength = 12.0f;
	float shadowDensity = 1.1f;
	float anisotropy = 0.6f;
	float sunIntensity = 1.0f;
	float padding0 = 0.0f;
	float padding1 = 0.0f;
};

class Cloud {
public:
	void Initialize(CloudSetup *setup);
	void Update(const Camera &camera, float deltaTime);
	void Draw();

	CloudRenderParams &GetMutableParams() {
		return paramsCPU_;
	}
	const CloudRenderParams &GetParams() const {
		return paramsCPU_;
	}
	void SetWeatherMap(D3D12_GPU_DESCRIPTOR_HANDLE srv);

private:
	struct FullscreenVertex {
		float position[3];
		float uv[2];
	};

	void CreateFullscreenVertexBuffer();
	void CreateConstantBuffers();

private:
	CloudSetup *setup_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraCB_;
	Microsoft::WRL::ComPtr<ID3D12Resource> paramsCB_;

	CloudCameraConstant *cameraData_ = nullptr;
	CloudRenderParams *paramsData_ = nullptr;
	CloudRenderParams paramsCPU_;

	D3D12_GPU_DESCRIPTOR_HANDLE weatherMapSrv_{};
	bool hasWeatherMapSrv_ = false;

	float accumulatedTime_ = 0.0f;
};
