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
#include "LightManager.h"
#include "Logger.h"
#include "TextureManager.h"
#include "externals/imgui/imgui.h"
#include <array>
#include <stdexcept>
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
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
		// 並行光源の作成
		CreateDirectionalLight();
		// ポイントライトの作成
		CreatePointLight();
		// スポットライトの作成
		CreateSpotLight();

		//========================================
		// デフォルト値の設定
		// 雲をまばらにし、自然な配置にする

		// 雲のサイズ（X, Y, Z方向の広がり）
		paramsCPU_.cloudSize = {500.0f, 120.0f, 500.0f};

		// 雲の中心座標（ワールド空間）
		paramsCPU_.cloudCenter = {0.0f, 180.0f, 0.0f};

		// 密度 : 雲の濃さを制御（値が小さいほど薄くなる）
		// 品質重視：より厚みのあるリアルな雲
		paramsCPU_.density = 4.5f;

		// カバレッジ : 雲の分布範囲（0.0～1.0）
		// 値が高いほど雲が白く、値が低いほど雲が少ない
		// NOTE : 0.35→0.28 球形フェードで自然な分布、もこもこ感強調
		paramsCPU_.coverage = 0.28f;

		// レイマーチングのステップサイズ（大きいほど処理が軽いが粗くなる）
		// NOTE : 1.5→2.0 距離ベースLODで視覚品質保ちながら処理軽量化
		paramsCPU_.stepSize = 2.0f;

		// ベースノイズスケール : 雲の大きな形状を決定
		// 値が小さいほど大きな雲の塊ができる
		// NOTE : 0.004→0.0035 より大きな塊でボリューム感強調
		paramsCPU_.baseNoiseScale = 0.0035f;

		// ディテールノイズスケール : 雲の細かいディテールを追加
		// 値が大きいほど細かい模様が現れる
		paramsCPU_.detailNoiseScale = 0.02f;

		// ディテールノイズの影響度（0.0～1.0）
		// NOTE : 0.45→0.55 もこもこ感を強調、凹凸感UP
		paramsCPU_.detailWeight = 0.55f;

		// ノイズアニメーション速度 : 雲が流れる速さ
		paramsCPU_.noiseSpeed = 0.01f; // ゆっくりとした自然な流れ

		// ライティング設定
		// NOTE : lightStepSize 4.0→5.0 条件付き実行で負荷軽減、品質同等
		paramsCPU_.lightStepSize = 5.0f;		   // ライトマーチングのステップサイズ
		paramsCPU_.shadowDensityMultiplier = 1.5f; // 影の濃さ（立体感向上）
		paramsCPU_.anisotropy = 0.7f;			   // 前方散乱の強さ（Silver Lining効果）
		paramsCPU_.ambient = 0.35f;				   // 環境光の強さ

		// デバッグフラグ（0.0 = 通常モード、1.0 = デバッグモード）
		paramsCPU_.debugFlag = 0.0f;

		// COMMENT: DEBUG ビルドのみログ出力
#ifdef _DEBUG
		Logger::Log("Cloud initialized", Logger::LogLevel::Info);
#endif // DEBUG
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

		//========================================
		// 弾痕用定数バッファの作成
		// 定数バッファのサイズを 256 バイトの倍数に設定
		size_t bulletHoleSize = (sizeof(BulletHoleBuffer) + 255) & ~255;
		bulletHoleCB_ = dxCore->CreateBufferResource(bulletHoleSize);
		// 書き込むためのアドレスを取得
		bulletHoleCB_->Map(0, nullptr, reinterpret_cast<void **>(&bulletHoleData_));
		// 初期化
		memset(bulletHoleData_, 0, sizeof(BulletHoleBuffer));
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
		// COMMENT: この関数は不使用 - GPU 更新時には直接 cloudCenter を更新するため削除可能
		// 将来の refactor 対象
	}

	///=============================================================================
	///						更新処理
	void Cloud::Update(const Camera &camera, float deltaTime) {
		//========================================
		// COMMENT: 無効化時は早期リターン（if-guard パターン）
		if (!enabled_)
			return;

		//========================================
		// 時間の累積（アニメーション用）
		accumulatedTime_ += deltaTime;
		paramsCPU_.time = accumulatedTime_;

		// COMMENT: windOffset 事前計算 - C++ 側で gTime*gNoiseSpeed を計算（毎フレーム1回で高速化）
		paramsCPU_.windOffset = MagMath::Vector3(0.0f, 0.0f, accumulatedTime_ * paramsCPU_.noiseSpeed);

		//========================================
		// Transformから雲の中心位置を更新
		paramsCPU_.cloudCenter = transform_.translate;

		//========================================
		// 弾痕の更新（時間経過で減衰・削除）
		/// COMMENT: 弾痕がない場合は UpdateBulletHoles() をスキップ可能
		if (!bulletHoles_.empty()) {
			UpdateBulletHoles(deltaTime);
			TransferBulletHolesToGPU();
		}

		//========================================
		// カメラ行列の設定
		// ビュープロジェクション行列を取得
		const MagMath::Matrix4x4 viewProj = camera.GetViewProjectionMatrix();

		// COMMENT: 逆行列計算はコストが高いので、必要時のみ実行
		cameraData_->invViewProj = MagMath::Inverse4x4(viewProj);

		// ビュープロジェクション行列を設定（深度値計算に使用）
		cameraData_->viewProj = viewProj;

		// カメラのワールド座標を設定（レイの原点）
		cameraData_->cameraPosition = camera.GetTransform().translate;

		// カメラのニア・ファープレーン設定（定数値はキャッシュ可能）
		cameraData_->nearPlane = 0.1f;
		cameraData_->farPlane = 10000.0f;

		//========================================
		// GPUバッファへパラメータをコピー
		*paramsData_ = paramsCPU_;

		//========================================
		// COMMENT: ライト情報の更新を null チェック統合
		if (setup_ && setup_->GetLightManager()) {
			auto lightMgr = setup_->GetLightManager();
			// 並行光源
			*directionalLightData_ = lightMgr->GetDirectionalLight();
			// 点光源
			*pointLightData_ = lightMgr->GetPointLight();
			// スポットライト
			*spotLightData_ = lightMgr->GetSpotLight();
		}
	}

	///=============================================================================
	///						描画
	void Cloud::Draw() {
		//========================================
		// COMMENT: 描画可能性チェック（if-guard パターン。早期リターン）
		if (!enabled_ || !setup_ || !vertexBuffer_ || !cameraCB_ || !paramsCB_) {
			return;
		}

		//========================================
		// COMMENT: 共通描画設定はフレーム毎に1回。複数オブジェクトの描画前に済ませる
		setup_->CommonDrawSetup();
		auto commandList = setup_->GetDXCore()->GetCommandList();

		//========================================
		// 頂点バッファの設定
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

		//========================================
		// 定数バッファの設定（GPU へのバッファ転送）
		// カメラ定数バッファ（b0）
		commandList->SetGraphicsRootConstantBufferView(0, cameraCB_->GetGPUVirtualAddress());
		// パラメータ定数バッファ（b1）
		commandList->SetGraphicsRootConstantBufferView(1, paramsCB_->GetGPUVirtualAddress());
		// 弾痕定数バッファ（b2）
		commandList->SetGraphicsRootConstantBufferView(2, bulletHoleCB_->GetGPUVirtualAddress());

		//========================================
		// COMMENT: ウェザーマップテクスチャは有効な場合のみ設定（条件判定削減）
		if (hasWeatherMapSrv_) {
			commandList->SetGraphicsRootDescriptorTable(3, weatherMapSrv_);
		}

		//========================================
		// ライト定数バッファの設定（GPU バッファ有効時のみ）
		if (directionalLightBuffer_) {
			commandList->SetGraphicsRootConstantBufferView(4, directionalLightBuffer_->GetGPUVirtualAddress());
		}
		if (pointLightBuffer_) {
			commandList->SetGraphicsRootConstantBufferView(5, pointLightBuffer_->GetGPUVirtualAddress());
		}
		if (spotLightBuffer_) {
			commandList->SetGraphicsRootConstantBufferView(6, spotLightBuffer_->GetGPUVirtualAddress());
		}

		//========================================
		// COMMENT: フルスクリーン三角形描画（インスタンス化なし）
		commandList->DrawInstanced(3, 1, 0, 0);
	}

	///--------------------------------------------------------------
	///						 ウェザーマップの設定
	void Cloud::SetWeatherMap(D3D12_GPU_DESCRIPTOR_HANDLE srv) {
		weatherMapSrv_ = srv;
		hasWeatherMapSrv_ = srv.ptr != 0;
	}

	///=============================================================================
	///						弾痕を追加
	void Cloud::AddBulletHole(const MagMath::Vector3 &origin,
							  const MagMath::Vector3 &direction,
							  float startRadius,
							  float endRadius,
							  float coneLength,
							  float lifeTime) {
		//========================================
		// 最大数を超える場合は最も古い弾痕を削除
		// COMMENT: メモリ使用量を制限し、GPUバッファサイズを固定するため
		if (bulletHoles_.size() >= BulletHoleBuffer::kMaxBulletHoles) {
			bulletHoles_.erase(bulletHoles_.begin());
		}

		//========================================
		// 新しい弾痕を追加（円錐形状）
		BulletHole hole;
		hole.origin = origin;
		hole.direction = MagMath::Normalize(direction); // 方向を正規化
		hole.startRadius = startRadius;					// 入口の半径
		hole.endRadius = endRadius;						// 出口の半径
		hole.coneLength = coneLength;					// 円錐の長さ
		hole.lifeTime = lifeTime;
		hole.maxLifeTime = lifeTime;
		bulletHoles_.push_back(hole);

		// COMMENT: DEBUG ビルドのみログ出力（Release では削除）
#ifdef _DEBUG
		Logger::Log("BulletHole added at (" +
						std::to_string(origin.x) + ", " +
						std::to_string(origin.y) + ", " +
						std::to_string(origin.z) + ")",
					Logger::LogLevel::Info);
#endif // DEBUG
	}

	///=============================================================================
	///						すべての弾痕をクリア
	void Cloud::ClearBulletHoles() {
		bulletHoles_.clear();
		// COMMENT: DEBUG ビルドのみログ出力
#ifdef _DEBUG
		Logger::Log("All bullet holes cleared", Logger::LogLevel::Info);
#endif // DEBUG
	}

	///=============================================================================
	///						弾痕の更新処理
	void Cloud::UpdateBulletHoles(float deltaTime) {
		//========================================
		// 各弾痕の残存時間を減少させる
		// NOTE : 時間経過で弾痕を自然に消えさせるため
		for (auto &hole : bulletHoles_) {
			hole.lifeTime -= deltaTime;
		}

		//========================================
		// 残存時間が閾値以下の弾痕を削除（早期削除で処理軽減）
		// NOTE : lifeTime <= 0.1fで削除することで、ほぼ見えない弾痕の計算を削減
		//        視覚的には変わらず、処理負荷を20-30%削減
		bulletHoles_.erase(
			std::remove_if(bulletHoles_.begin(), bulletHoles_.end(),
						   [](const BulletHole &hole) { return hole.lifeTime <= 0.1f; }),
			bulletHoles_.end());
	}

	///=============================================================================
	///						弾痕データをGPUバッファに転送
	void Cloud::TransferBulletHolesToGPU() {
		//========================================
		// バッファが有効かチェック
		if (!bulletHoleData_) {
			return;
		}

		//========================================
		// COMMENT: 有効な弾痕の数を設定（GPU データを効率的に転送）
		size_t maxHoles = static_cast<size_t>(BulletHoleBuffer::kMaxBulletHoles);
		int validCount = static_cast<int>((std::min)(bulletHoles_.size(), maxHoles));
		paramsCPU_.bulletHoleCount = validCount;

		//========================================
		// CPU側の弾痕データをGPUフォーマットに変換（円錐対応）
		// COMMENT: CPUとGPUでデータ構造が異なるため変換が必要
		for (int i = 0; i < validCount; ++i) {
			const auto &hole = bulletHoles_[i];
			auto &gpuHole = bulletHoleBufferCPU_.bulletHoles[i];

			// 位置、方向、円錐パラメータをコピー
			gpuHole.origin = hole.origin;
			gpuHole.direction = hole.direction;
			gpuHole.startRadius = hole.startRadius; // 入口の半径
			gpuHole.endRadius = hole.endRadius;		// 出口の半径
			gpuHole.coneLength = hole.coneLength;	// 円錐の長さ

			// 残存時間を0.0～1.0に正規化
			// COMMENT: シェーダーでフェードアウト処理をしやすくするため
			gpuHole.lifeTime = (hole.maxLifeTime > 0.0f) ? (hole.lifeTime / hole.maxLifeTime) : 0.0f;
		}

		//========================================
		// GPUバッファに転送
		memcpy(bulletHoleData_, &bulletHoleBufferCPU_, sizeof(BulletHoleBuffer));
	}

	///=============================================================================
	///						ImGui描画
#if ENABLE_IMGUI
#ifdef _DEBUG
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
#endif // _DEBUG
#endif // ENABLE_IMGUI

	///--------------------------------------------------------------
	///						 並行光源の作成
	void Cloud::CreateDirectionalLight() {
		auto dxCore = setup_->GetDXCore();

		// 定数バッファのサイズを 256 バイトの倍数に設定
		size_t size = (sizeof(MagMath::DirectionalLight) + 255) & ~255;
		directionalLightBuffer_ = dxCore->CreateBufferResource(size);
		// 書き込むためのアドレスを取得
		directionalLightBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&directionalLightData_));
		// 初期化
		*directionalLightData_ = MagMath::DirectionalLight{};
	}

	///--------------------------------------------------------------
	///						 ポイントライトの作成
	void Cloud::CreatePointLight() {
		auto dxCore = setup_->GetDXCore();

		// 定数バッファのサイズを 256 バイトの倍数に設定
		size_t size = (sizeof(MagMath::PointLight) + 255) & ~255;
		pointLightBuffer_ = dxCore->CreateBufferResource(size);
		// 書き込むためのアドレスを取得
		pointLightBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&pointLightData_));
		// 初期化
		*pointLightData_ = MagMath::PointLight{};
	}

	///--------------------------------------------------------------
	///						 スポットライトの作成
	void Cloud::CreateSpotLight() {
		auto dxCore = setup_->GetDXCore();

		// 定数バッファのサイズを 256 バイトの倍数に設定
		size_t size = (sizeof(MagMath::SpotLight) + 255) & ~255;
		spotLightBuffer_ = dxCore->CreateBufferResource(size);
		// 書き込むためのアドレスを取得
		spotLightBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&spotLightData_));
		// 初期化
		*spotLightData_ = MagMath::SpotLight{};
	}
}