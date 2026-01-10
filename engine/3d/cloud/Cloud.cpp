/*********************************************************************
 * \file   Cloud.cpp
 * \brief  雲描画クラス実装
 *
 * \author Harukichimaru
 * \date   December 2024
 * \note
 *********************************************************************/
#include "Cloud.h"
#include "Camera.h"
#include "CloudSetup.h"
#include "DirectXCore.h"
#include "Logger.h"
#include "TextureManager.h"
#include "externals/imgui/imgui.h"
#include <array>
#include <stdexcept>

///=============================================================================
///						初期化
void Cloud::Initialize(CloudSetup *setup) {
	//========================================
	// 引数チェック
	if (!setup) {
		throw std::invalid_argument("Cloud::Initialize requires CloudSetup.");
	}

	//========================================
	// 引数からSetupを受け取る
	setup_ = setup;

	//========================================
	// 各種バッファの作成
	CreateFullscreenVertexBuffer();
	CreateConstantBuffers();

	//========================================
	// デフォルト値の設定
	// 雲をまばらにし、自然な配置にする

	// 雲のサイズ（X, Y, Z方向の広がり）
	paramsCPU_.cloudSize = {300.0f, 100.0f, 300.0f};

	// 雲の中心座標（ワールド空間）
	paramsCPU_.cloudCenter = {0.0f, 150.0f, 0.0f};

	// 密度：雲の濃さを制御（値が小さいほど薄くなる）
	// 1.5 = かなり薄い雲（以前: 2.0f）
	paramsCPU_.density = 1.5f;

	// カバレッジ：雲の分布範囲（0.0～1.0）
	// 値が小さいほど雲がまばらになる
	// 0.3 = 30%の領域に雲が存在（以前: 0.4f）
	paramsCPU_.coverage = 0.3f;

	// レイマーチングのステップサイズ（大きいほど処理が軽いが粗くなる）
	paramsCPU_.stepSize = 5.0f;

	// ベースノイズスケール：雲の大きな形状を決定
	// 値が小さいほど大きな雲の塊ができる
	paramsCPU_.baseNoiseScale = 0.008f; // より大きな雲の形状（以前: 0.01f）

	// ディテールノイズスケール：雲の細かいディテールを追加
	// 値が大きいほど細かい模様が現れる
	paramsCPU_.detailNoiseScale = 0.025f; // 細かいディテールを少し抑える（以前: 0.03f）

	// ディテールノイズの影響度（0.0～1.0）
	// 値が小さいほどなめらかな雲になる
	paramsCPU_.detailWeight = 0.25f; // よりなめらかに（以前: 0.3f）

	// ノイズアニメーション速度：雲が流れる速さ
	// 値が小さいほどゆっくり動く
	paramsCPU_.noiseSpeed = 0.015f; // ゆっくりとした流れ（以前: 0.02f）

	// デバッグフラグ（0.0 = 通常モード、1.0 = デバッグモード）
	paramsCPU_.debugFlag = 0.0f;

	Logger::Log("Cloud initialized", Logger::LogLevel::Info);
}

///=============================================================================
///						フルスクリーン頂点バッファの作成
void Cloud::CreateFullscreenVertexBuffer() {
	//========================================
	// フルスクリーン三角形の頂点データを定義
	// NOTE: 画面全体を覆う大きな三角形を1つ描画することで、
	//       全ピクセルをレイマーチングで処理する
	const std::array<FullscreenVertex, 3> vertices{{
		{{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}, // 左下
		{{-1.0f, 3.0f, 0.0f}, {0.0f, -1.0f}}, // 左上（画面外）
		{{3.0f, -1.0f, 0.0f}, {2.0f, 1.0f}},  // 右下（画面外）
	}};

	//========================================
	// 頂点バッファの作成
	auto dxCore = setup_->GetDXCore();
	size_t bufferSize = sizeof(vertices);
	vertexBuffer_ = dxCore->CreateBufferResource(bufferSize);

	//========================================
	// 頂点データをバッファに書き込み
	void *mapped = nullptr;
	D3D12_RANGE readRange{0, 0};
	vertexBuffer_->Map(0, &readRange, &mapped);
	std::memcpy(mapped, vertices.data(), bufferSize);
	vertexBuffer_->Unmap(0, nullptr);

	//========================================
	// 頂点バッファビューの設定
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(bufferSize);
	vertexBufferView_.StrideInBytes = sizeof(FullscreenVertex);
}

///=============================================================================
///						定数バッファの作成
void Cloud::CreateConstantBuffers() {
	auto dxCore = setup_->GetDXCore();

	//========================================
	// カメラ用定数バッファの作成
	// 定数バッファのサイズを 256 バイトの倍数に設定
	size_t cameraSize = (sizeof(CloudCameraConstant) + 255) & ~255;
	cameraCB_ = dxCore->CreateBufferResource(cameraSize);
	// 書き込むためのアドレスを取得
	cameraCB_->Map(0, nullptr, reinterpret_cast<void **>(&cameraData_));
	// 初期化
	*cameraData_ = {};

	//========================================
	// パラメータ用定数バッファの作成
	// 定数バッファのサイズを 256 バイトの倍数に設定
	size_t paramsSize = (sizeof(CloudRenderParams) + 255) & ~255;
	paramsCB_ = dxCore->CreateBufferResource(paramsSize);
	// 書き込むためのアドレスを取得
	paramsCB_->Map(0, nullptr, reinterpret_cast<void **>(&paramsData_));
	// 初期化
	*paramsData_ = paramsCPU_;
}

///--------------------------------------------------------------
///						 位置の設定
void Cloud::SetPosition(const MagMath::Vector3 &pos) {
	// Transformの位置を更新
	transform_.translate = pos;
	// パラメータの雲中心座標を直接設定
	paramsCPU_.cloudCenter = pos;
}

///--------------------------------------------------------------
///						 スケールの設定
void Cloud::SetScale(const MagMath::Vector3 &scale) {
	// Transformのスケールを更新
	transform_.scale = scale;
	// NOTE: スケールは使わず、SetSizeを使う
}

///--------------------------------------------------------------
///						 雲パラメータの更新
void Cloud::UpdateCloudParams() {
	// NOTE: この関数は使わない - 直接cloudCenterを更新
}

///=============================================================================
///						更新処理
void Cloud::Update(const Camera &camera, float deltaTime) {
	//========================================
	// 無効化されている場合は更新をスキップ
	if (!enabled_)
		return;

	//========================================
	// 時間の累積（アニメーション用）
	accumulatedTime_ += deltaTime;
	paramsCPU_.time = accumulatedTime_;

	//========================================
	// Transformから雲の中心位置を更新
	paramsCPU_.cloudCenter = transform_.translate;

	//========================================
	// カメラ行列の設定
	// ビュープロジェクション行列を取得
	const MagMath::Matrix4x4 viewProj = camera.GetViewProjectionMatrix();

	// 逆ビュープロジェクション行列を計算（レイの方向計算に使用）
	cameraData_->invViewProj = MagMath::Inverse4x4(viewProj);

	// ビュープロジェクション行列を設定（深度値計算に使用）
	cameraData_->viewProj = viewProj;

	// カメラのワールド座標を設定（レイの原点）
	cameraData_->cameraPosition = camera.GetTransform().translate;

	// カメラのニア・ファープレーン設定
	cameraData_->nearPlane = 0.1f;
	cameraData_->farPlane = 10000.0f;

	//========================================
	// GPUバッファへパラメータをコピー
	*paramsData_ = paramsCPU_;
}

///=============================================================================
///						描画
void Cloud::Draw() {
	//========================================
	// 描画可能かチェック
	if (!enabled_ || !setup_ || !vertexBuffer_ || !cameraCB_ || !paramsCB_) {
		return;
	}

	//========================================
	// 共通描画設定
	setup_->CommonDrawSetup();
	auto commandList = setup_->GetDXCore()->GetCommandList();

	//========================================
	// 頂点バッファの設定
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

	//========================================
	// 定数バッファの設定
	// カメラ定数バッファ（b0）
	commandList->SetGraphicsRootConstantBufferView(0, cameraCB_->GetGPUVirtualAddress());
	// パラメータ定数バッファ（b1）
	commandList->SetGraphicsRootConstantBufferView(1, paramsCB_->GetGPUVirtualAddress());

	//========================================
	// ウェザーマップテクスチャの設定
	if (hasWeatherMapSrv_) {
		commandList->SetGraphicsRootDescriptorTable(2, weatherMapSrv_);
	}

	//========================================
	// 描画コール（フルスクリーン三角形）
	commandList->DrawInstanced(3, 1, 0, 0);
}

///--------------------------------------------------------------
///						 ウェザーマップの設定
void Cloud::SetWeatherMap(D3D12_GPU_DESCRIPTOR_HANDLE srv) {
	weatherMapSrv_ = srv;
	hasWeatherMapSrv_ = srv.ptr != 0;
}

///=============================================================================
///						ImGui描画
void Cloud::DrawImGui() {
	ImGui::Begin("Cloud Settings");

	//========================================
	// 基本設定
	ImGui::Checkbox("Enabled", &enabled_);
	ImGui::Checkbox("Debug Mode", (bool *)&paramsCPU_.debugFlag);
	ImGui::Separator();

	//========================================
	// Transform設定
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		// 位置設定
		if (ImGui::DragFloat3("Position", &transform_.translate.x, 5.0f, -2000.0f, 2000.0f)) {
			paramsCPU_.cloudCenter = transform_.translate;
		}
		// サイズ設定
		ImGui::DragFloat3("Size", &paramsCPU_.cloudSize.x, 5.0f, 10.0f, 1000.0f);

		// 便利ボタン
		if (ImGui::Button("Reset Position")) {
			transform_.translate = {0.0f, 150.0f, 0.0f};
			paramsCPU_.cloudCenter = transform_.translate;
		}
		if (ImGui::Button("Move to Camera Front")) {
			if (cameraData_) {
				MagMath::Vector3 forward = {0.0f, 0.0f, 1.0f};
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

	//========================================
	// 密度とカバレッジ設定
	if (ImGui::CollapsingHeader("Density & Coverage")) {
		ImGui::SliderFloat("Density", &paramsCPU_.density, 0.0f, 10.0f);
		ImGui::SliderFloat("Coverage", &paramsCPU_.coverage, 0.0f, 1.0f);
		ImGui::SliderFloat("Detail Weight", &paramsCPU_.detailWeight, 0.0f, 1.0f);
		ImGui::Text("Tip: Lower coverage = more visible clouds");
	}

	//========================================
	// ノイズ設定
	if (ImGui::CollapsingHeader("Noise Settings")) {
		ImGui::SliderFloat("Base Noise Scale", &paramsCPU_.baseNoiseScale, 0.0001f, 0.05f, "%.5f");
		ImGui::SliderFloat("Detail Noise Scale", &paramsCPU_.detailNoiseScale, 0.001f, 0.1f, "%.4f");
		ImGui::SliderFloat("Noise Speed", &paramsCPU_.noiseSpeed, 0.0f, 0.2f);
		ImGui::Text("Tip: Larger scale = bigger cloud features");
	}

	//========================================
	// ライティング設定
	if (ImGui::CollapsingHeader("Lighting")) {
		ImGui::DragFloat3("Sun Direction", &paramsCPU_.sunDirection.x, 0.01f, -1.0f, 1.0f);
		ImGui::ColorEdit3("Sun Color", &paramsCPU_.sunColor.x);
		ImGui::SliderFloat("Sun Intensity", &paramsCPU_.sunIntensity, 0.0f, 5.0f);
		ImGui::SliderFloat("Ambient", &paramsCPU_.ambient, 0.0f, 1.0f);
		ImGui::SliderFloat("Anisotropy", &paramsCPU_.anisotropy, -1.0f, 1.0f);
		ImGui::SliderFloat("Shadow Density", &paramsCPU_.shadowDensityMultiplier, 0.0f, 3.0f);
	}

	//========================================
	// レイマーチング設定
	if (ImGui::CollapsingHeader("Raymarching")) {
		ImGui::SliderFloat("Step Size", &paramsCPU_.stepSize, 0.5f, 20.0f);
		ImGui::SliderFloat("Light Step Size", &paramsCPU_.lightStepSize, 5.0f, 50.0f);
		ImGui::SliderFloat("Max Distance", &paramsCPU_.maxDistance, 100.0f, 5000.0f);
	}

	//========================================
	// デバッグ情報
	ImGui::Separator();
	ImGui::Text("Debug Info");
	ImGui::Text("Time: %.2f", paramsCPU_.time);

	// カメラ情報
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

	// 雲の情報
	ImGui::Text("Center: (%.1f, %.1f, %.1f)",
				paramsCPU_.cloudCenter.x, paramsCPU_.cloudCenter.y, paramsCPU_.cloudCenter.z);
	ImGui::Text("Size: (%.1f, %.1f, %.1f)",
				paramsCPU_.cloudSize.x, paramsCPU_.cloudSize.y, paramsCPU_.cloudSize.z);

	// AABB（軸平行境界ボックス）表示
	MagMath::Vector3 boxMin = {
		paramsCPU_.cloudCenter.x - paramsCPU_.cloudSize.x * 0.5f,
		paramsCPU_.cloudCenter.y - paramsCPU_.cloudSize.y * 0.5f,
		paramsCPU_.cloudCenter.z - paramsCPU_.cloudSize.z * 0.5f};
	MagMath::Vector3 boxMax = {
		paramsCPU_.cloudCenter.x + paramsCPU_.cloudSize.x * 0.5f,
		paramsCPU_.cloudCenter.y + paramsCPU_.cloudSize.y * 0.5f,
		paramsCPU_.cloudCenter.z + paramsCPU_.cloudSize.z * 0.5f};
	ImGui::Text("AABB Min: (%.1f, %.1f, %.1f)", boxMin.x, boxMin.y, boxMin.z);
	ImGui::Text("AABB Max: (%.1f, %.1f, %.1f)", boxMax.x, boxMax.y, boxMax.z);

	//========================================
	// 深度デバッグ情報
	if (ImGui::CollapsingHeader("Depth Debug")) {
		ImGui::Text("Near Plane: %.2f", cameraData_->nearPlane);
		ImGui::Text("Far Plane: %.2f", cameraData_->farPlane);
	}

	ImGui::End();
}
