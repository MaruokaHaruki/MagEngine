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
	float nearPlane = 0.1f;
	float farPlane = 10000.0f;
	float padding2 = 0.0f;
	float padding3 = 0.0f;
	Matrix4x4 viewProj; // ビュープロジェクション行列を追加
};

struct alignas(16) CloudRenderParams {
	// 雲の位置とサイズ (16 bytes aligned)
	Vector3 cloudCenter{0.0f, 150.0f, 0.0f};
	float cloudSizeX = 300.0f; // 未使用だが構造体パディング用に維持

	Vector3 cloudSize{300.0f, 100.0f, 300.0f}; // XYZサイズ
	float padding0 = 0.0f;

	// ライティング (16 bytes aligned)
	Vector3 sunDirection{0.3f, 0.8f, 0.5f};
	float sunIntensity = 1.2f;

	Vector3 sunColor{1.0f, 0.96f, 0.88f};
	float ambient = 0.3f;

	// 雲の密度とノイズ (16 bytes aligned)
	float density = 1.0f;
	float coverage = 0.5f;
	float baseNoiseScale = 0.003f;
	float detailNoiseScale = 0.015f;

	// レイマーチング設定 (16 bytes aligned)
	float stepSize = 3.0f;
	float maxDistance = 2000.0f;
	float lightStepSize = 15.0f;
	float shadowDensityMultiplier = 1.2f;

	// アニメーション (16 bytes aligned)
	float time = 0.0f;
	float noiseSpeed = 0.05f;
	float detailWeight = 0.4f;
	float anisotropy = 0.6f;

	// デバッグ (16 bytes aligned)
	float debugFlag = 0.0f;
	float padding1 = 0.0f;
	float padding2 = 0.0f;
	float padding3 = 0.0f;
};

class Cloud {
public:
	void Initialize(CloudSetup *setup);
	void Update(const Camera &camera, float deltaTime);
	void Draw();
	void DrawImGui();

	// Transform操作
	Transform &GetTransform() {
		return transform_;
	}
	const Transform &GetTransform() const {
		return transform_;
	}
	void SetPosition(const Vector3 &pos);
	void SetScale(const Vector3 &scale);
	void SetSize(const Vector3 &size) {
		paramsCPU_.cloudSize = size;
	}

	// パラメータアクセス
	CloudRenderParams &GetMutableParams() {
		return paramsCPU_;
	}
	const CloudRenderParams &GetParams() const {
		return paramsCPU_;
	}

	void SetWeatherMap(D3D12_GPU_DESCRIPTOR_HANDLE srv);
	void SetEnabled(bool enabled) {
		enabled_ = enabled;
	}
	bool IsEnabled() const {
		return enabled_;
	}

private:
	struct FullscreenVertex {
		float position[3];
		float uv[2];
	};

	void CreateFullscreenVertexBuffer();
	void CreateConstantBuffers();
	void UpdateCloudParams();

private:
	CloudSetup *setup_ = nullptr;
	Transform transform_{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 150.0f, 0.0f}};

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> cameraCB_;
	Microsoft::WRL::ComPtr<ID3D12Resource> paramsCB_;

	CloudCameraConstant *cameraData_ = nullptr;
	CloudRenderParams *paramsData_ = nullptr;
	CloudRenderParams paramsCPU_;

	D3D12_GPU_DESCRIPTOR_HANDLE weatherMapSrv_{};
	bool hasWeatherMapSrv_ = false;
	bool enabled_ = true;

	float accumulatedTime_ = 0.0f;
};
