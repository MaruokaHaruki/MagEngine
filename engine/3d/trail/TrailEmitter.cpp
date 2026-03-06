/*********************************************************************
 * \file   TrailEmitter.cpp
 * \brief  トレイルパーティクル生成・管理クラス実装
 *
 * \author MagEngine
 * \date   March 2026
 * \note
 *********************************************************************/
#include "TrailEmitter.h"
#include "DirectXCore.h"
#include "Logger.h"
#include "TrailEffectSetup.h"
#include <algorithm>
#include <cstring>

using namespace Logger;

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///						初期化
	void TrailEmitter::Initialize(TrailEffectSetup *setup) {
		//========================================
		// 引数チェック
		if (!setup) {
			throw std::runtime_error("TrailEmitter: TrailEffectSetup is null.");
		}

		//========================================
		// 引数からSetupsを受け取る
		setup_ = setup;

		//========================================
		// 各種バッファの作成
		CreateVertexBuffer();
		CreateConstantBuffers();

		//========================================
		// デフォルト値の設定
		paramsCPU_.color = {0.5f, 0.5f, 0.5f};
		paramsCPU_.opacity = 0.8f;
		paramsCPU_.width = 2.0f;
		paramsCPU_.lifeTime = 3.0f;
		paramsCPU_.velocityDamping = 0.95f;
		paramsCPU_.gravityInfluence = 0.2f;

		Log("TrailEmitter initialized", LogLevel::Info);
	}

	///=============================================================================
	///						頂点バッファの作成
	void TrailEmitter::CreateVertexBuffer() {
		//========================================
		// 頂点バッファの作成
		auto dxCore = setup_->GetDXCore();
		size_t bufferSize = sizeof(TrailVertex) * maxVertices_;
		vertexBuffer_ = dxCore->CreateBufferResource(bufferSize);

		//========================================
		// 頂点バッファビューの設定
		vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
		vertexBufferView_.SizeInBytes = static_cast<UINT>(bufferSize);
		vertexBufferView_.StrideInBytes = sizeof(TrailVertex);
	}

	///=============================================================================
	///						定数バッファの作成
	void TrailEmitter::CreateConstantBuffers() {
		auto dxCore = setup_->GetDXCore();

		//========================================
		// パラメータ用定数バッファの作成
		// 定数バッファのサイズを 256 バイトの倍数に設定
		size_t paramsSize = (sizeof(TrailRenderParams) + 255) & ~255;
		paramsCB_ = dxCore->CreateBufferResource(paramsSize);
		// 書き込むためのアドレスを取得
		paramsCB_->Map(0, nullptr, reinterpret_cast<void **>(&paramsData_));
		// 初期化
		*paramsData_ = paramsCPU_;
	}

	///=============================================================================
	///						更新処理
	void TrailEmitter::Update(float deltaTime) {
		//========================================
		// 時間の累積
		accumulatedTime_ += deltaTime;
		paramsCPU_.time = accumulatedTime_;

		//========================================
		// 各パーティクルの更新
		for (auto &particle : particles_) {
			// ライフタイム進行
			particle.age += deltaTime;

			// 速度減衰を適用
			particle.velocity *= paramsCPU_.velocityDamping;

			// 重力を適用
			particle.velocity.y -= 9.8f * paramsCPU_.gravityInfluence * deltaTime;

			// 位置更新
			particle.position += particle.velocity * deltaTime;
		}

		//========================================
		// ライフタイムが終了したパーティクルを削除
		particles_.erase(
			std::remove_if(particles_.begin(), particles_.end(),
						   [this](const TrailParticle &p) { return p.age >= paramsCPU_.lifeTime; }),
			particles_.end());

		//========================================
		// GPUバッファへパラメータをコピー
		*paramsData_ = paramsCPU_;

		//========================================
		// パーティクルデータをGPUに転送
		TransferParticlesToGPU();
	}

	///=============================================================================
	///						描画
	void TrailEmitter::Draw() {
		//========================================
		// 描画可能かチェック
		if (!setup_ || !vertexBuffer_ || !paramsCB_ || particles_.empty()) {
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
		// パラメータ定数バッファ（b0）
		commandList->SetGraphicsRootConstantBufferView(0, paramsCB_->GetGPUVirtualAddress());

		//========================================
		// 描画コール
		commandList->DrawInstanced(static_cast<UINT>(particles_.size() * 3), 1, 0, 0);
	}

	///=============================================================================
	///						軌跡を生成
	void TrailEmitter::EmitTrail(const MagMath::Vector3 &position,
								 const MagMath::Vector3 &velocity) {
		//========================================
		// 最大数を超える場合は最も古いパーティクルを削除
		if (particles_.size() >= maxVertices_ / 3) {
			particles_.erase(particles_.begin());
		}

		//========================================
		// 新しいパーティクルを追加
		TrailParticle particle;
		particle.position = position;
		particle.velocity = velocity;
		particle.age = 0.0f;
		particle.maxLifeTime = paramsCPU_.lifeTime;
		particles_.push_back(particle);
	}

	///=============================================================================
	///						すべての軌跡をクリア
	void TrailEmitter::ClearTrails() {
		particles_.clear();
		Log("All trail particles cleared", LogLevel::Info);
	}

	///=============================================================================
	///						パーティクルデータをGPUバッファに転送
	void TrailEmitter::TransferParticlesToGPU() {
		//========================================
		// バッファが有効かチェック
		if (!vertexBuffer_) {
			return;
		}

		//========================================
		// CPU側のパーティクルデータをGPU形式に変換
		std::vector<TrailVertex> vertices;
		vertices.reserve(particles_.size() * 3);

		for (const auto &particle : particles_) {
			float ageRatio = particle.age / particle.maxLifeTime;

			// 3頂点からなる三角形を生成（ビルボード的に）
			TrailVertex v;
			v.age = ageRatio;

			// 頂点1
			v.position = particle.position + MagMath::Vector3{-paramsCPU_.width * 0.5f, 0.0f, 0.0f};
			v.normal = {-1.0f, 0.0f, 0.0f};
			vertices.push_back(v);

			// 頂点2
			v.position = particle.position + MagMath::Vector3{paramsCPU_.width * 0.5f, 0.0f, 0.0f};
			v.normal = {1.0f, 0.0f, 0.0f};
			vertices.push_back(v);

			// 頂点3
			v.position = particle.position + MagMath::Vector3{0.0f, paramsCPU_.width, 0.0f};
			v.normal = {0.0f, 1.0f, 0.0f};
			vertices.push_back(v);
		}

		//========================================
		// GPUバッファに転送
		if (vertices.size() > 0) {
			void *mapped = nullptr;
			D3D12_RANGE readRange{0, 0};
			vertexBuffer_->Map(0, &readRange, &mapped);
			std::memcpy(mapped, vertices.data(), vertices.size() * sizeof(TrailVertex));
			vertexBuffer_->Unmap(0, nullptr);
		}
	}
}
