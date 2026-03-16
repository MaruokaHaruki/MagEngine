/*********************************************************************
 * \file   TrailEmitter.cpp
 * \brief  トレイルパーティクル生成・管理クラス実装
 *
 * \author MagEngine
 * \date   March 2026
 * \note
 *********************************************************************/
#define _USE_MATH_DEFINES
// 以下はstd::maxを使用する場合に必要
#define NOMINMAX
#include "TrailEmitter.h"
#include "Camera.h"
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
		// メッシュデータ初期化
		trailHistory_.clear();
		vertices_.clear();
		indices_.clear();
		lastEmitPosition_ = {0.0f, 0.0f, 0.0f};
		accumulatedTime_ = 0.0f;

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
		// 頂点バッファの作成（リボンメッシュ用）
		// 最大 maxTrailPoints * 2 (左右の頂点) * 3 インデックス/セグメント
		auto dxCore = setup_->GetDXCore();
		size_t maxVertexCount = maxTrailPoints_ * 2;
		size_t vertexBufferSize = sizeof(TrailVertex) * maxVertexCount;
		vertexBuffer_ = dxCore->CreateBufferResource(vertexBufferSize);

		//========================================
		// 頂点バッファビューの設定
		vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
		vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
		vertexBufferView_.StrideInBytes = sizeof(TrailVertex);

		//========================================
		// インデックスバッファの作成
		// 最大 (maxTrailPoints - 1) * 6 インデックス（2三角形/セグメント）
		size_t maxIndexCount = (maxTrailPoints_ - 1) * 6;
		size_t indexBufferSize = sizeof(uint32_t) * maxIndexCount;
		indexBuffer_ = dxCore->CreateBufferResource(indexBufferSize);

		//========================================
		// インデックスバッファビューの設定
		indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
		indexBufferView_.SizeInBytes = static_cast<UINT>(indexBufferSize);
		indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
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

		//========================================
		// カメラ用定数バッファの作成
		size_t cameraSize = (sizeof(CameraConstant) + 255) & ~255;
		cameraCB_ = dxCore->CreateBufferResource(cameraSize);
		// 書き込むためのアドレスを取得
		cameraCB_->Map(0, nullptr, reinterpret_cast<void **>(&cameraData_));
		// 初期化
		*cameraData_ = cameraCPU_;
	}

	///=============================================================================
	///						更新処理
	void TrailEmitter::Update(float deltaTime) {
		//========================================
		// 時間の累積
		accumulatedTime_ += deltaTime;
		paramsCPU_.time = accumulatedTime_;

		//========================================
		// 古いポイントを削除
		RemoveExpiredPoints(accumulatedTime_);

		//========================================
		// GPUバッファへパラメータをコピー
		if (paramsData_) {
			*paramsData_ = paramsCPU_;
		}

		//========================================
		// カメラ情報を更新
		auto camera = setup_->GetDefaultCamera();
		if (camera && cameraData_) {
			cameraCPU_.viewProj = camera->GetViewProjectionMatrix();
			cameraCPU_.worldPosition = camera->GetTranslate();
			cameraCPU_.time = accumulatedTime_;
			*cameraData_ = cameraCPU_;
		}
	}

	///=============================================================================
	///						描画
	void TrailEmitter::Draw() {
		//========================================
		// 描画可能かチェック
		if (!setup_ || !vertexBuffer_ || !indexBuffer_ || trailHistory_.size() < 2) {
			return;
		}

		//========================================
		// メッシュを再構築
		BuildRibbonMesh();

		//========================================
		// インデックス数が0の場合はスキップ
		if (indices_.empty()) {
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
		// インデックスバッファの設定
		commandList->IASetIndexBuffer(&indexBufferView_);

		//========================================
		// 定数バッファの設定
		// パラメータ定数バッファ（b0）
		commandList->SetGraphicsRootConstantBufferView(0, paramsCB_->GetGPUVirtualAddress());

		//========================================
		// カメラ定数バッファの設定（b1）
		commandList->SetGraphicsRootConstantBufferView(1, cameraCB_->GetGPUVirtualAddress());

		//========================================
		// 描画コール
		commandList->DrawIndexedInstanced(static_cast<UINT>(indices_.size()), 1, 0, 0, 0);
	}

	///=============================================================================
	///						軌跡を生成
	void TrailEmitter::EmitTrail(const MagMath::Vector3 &position,
								 const MagMath::Vector3 &velocity) {
		//========================================
		// 最小距離以上の距離がある場合のみポイントを追加
		float distance = MagMath::Length(position - lastEmitPosition_);
		if (distance < minPointDistance_ && !trailHistory_.empty()) {
			return; // 最小距離に達していない
		}

		//========================================
		// 新しいポイントを追加
		TrailPoint newPoint;
		newPoint.position = position;
		newPoint.time = accumulatedTime_;
		trailHistory_.push_back(newPoint);
		lastEmitPosition_ = position;

		//========================================
		// 最大ポイント数を超えた場合は古いものから削除
		if (trailHistory_.size() > maxTrailPoints_) {
			trailHistory_.erase(trailHistory_.begin());
		}
	}

	///=============================================================================
	///						すべての軌跡をクリア
	void TrailEmitter::ClearTrails() {
		trailHistory_.clear();
		vertices_.clear();
		indices_.clear();
		Log("All trail particles cleared", LogLevel::Info);
	}

	///=============================================================================
	///						古いポイントを削除
	void TrailEmitter::RemoveExpiredPoints(float currentTime) {
		//========================================
		// ライフタイムが終わったポイントを削除
		while (!trailHistory_.empty()) {
			float pointAge = currentTime - trailHistory_.front().time;
			if (pointAge > paramsCPU_.lifeTime) {
				trailHistory_.erase(trailHistory_.begin());
			} else {
				break;
			}
		}
	}

	///=============================================================================
	///						リボンメッシュを生成
	void TrailEmitter::BuildRibbonMesh() {
		vertices_.clear();
		indices_.clear();

		if (trailHistory_.size() < 2) {
			return;
		}

		//========================================
		// ポイント履歴から頂点を生成
		GenerateVerticesFromHistory();

		//========================================
		// メモリに転送
		if (!vertices_.empty() && vertexBuffer_) {
			size_t bufferSize = sizeof(TrailVertex) * vertices_.size();
			if (vertexBuffer_->GetDesc().Width >= bufferSize) {
				void *vertexData = nullptr;
				D3D12_RANGE readRange{0, 0};
				vertexBuffer_->Map(0, &readRange, &vertexData);
				std::memcpy(vertexData, vertices_.data(), bufferSize);
				vertexBuffer_->Unmap(0, nullptr);
			}
		}

		if (!indices_.empty() && indexBuffer_) {
			size_t bufferSize = sizeof(uint32_t) * indices_.size();
			if (indexBuffer_->GetDesc().Width >= bufferSize) {
				void *indexData = nullptr;
				D3D12_RANGE readRange{0, 0};
				indexBuffer_->Map(0, &readRange, &indexData);
				std::memcpy(indexData, indices_.data(), bufferSize);
				indexBuffer_->Unmap(0, nullptr);
			}
		}
	}

	void TrailEmitter::GenerateVerticesFromHistory() {
		// 簡易実装：後で詳細版に置き換え
		if (trailHistory_.empty()) {
			return;
		}

		MagMath::Vector3 cameraPos = cameraCPU_.worldPosition;

		for (size_t i = 0; i < trailHistory_.size(); ++i) {
			const TrailPoint &point = trailHistory_[i];
			float pointAge = paramsCPU_.time - point.time;
			float lifeFraction = std::max(0.0f, std::min(1.0f, pointAge / paramsCPU_.lifeTime));

			MagMath::Vector3 direction{0.0f, 0.0f, 1.0f};
			if (i < trailHistory_.size() - 1) {
				MagMath::Vector3 delta = trailHistory_[i + 1].position - point.position;
				float len = MagMath::Length(delta);
				if (len > 0.001f) {
					direction = MagMath::Normalize(delta);
				}
			}

			MagMath::Vector3 toCamera = cameraPos - point.position;
			float toLen = MagMath::Length(toCamera);
			if (toLen > 0.001f) {
				toCamera = MagMath::Normalize(toCamera);
			} else {
				toCamera = {0.0f, 0.0f, 1.0f};
			}

			MagMath::Vector3 normal = MagMath::Cross(direction, toCamera);
			float normLen = MagMath::Length(normal);
			if (normLen > 0.001f) {
				normal = MagMath::Normalize(normal);
			} else {
				normal = {1.0f, 0.0f, 0.0f};
			}

			float width = paramsCPU_.width * (1.0f - lifeFraction);

			TrailVertex leftVertex;
			leftVertex.position = point.position - normal * width;
			leftVertex.age = lifeFraction;
			leftVertex.normal = -normal;

			TrailVertex rightVertex;
			rightVertex.position = point.position + normal * width;
			rightVertex.age = lifeFraction;
			rightVertex.normal = normal;

			vertices_.push_back(leftVertex);
			vertices_.push_back(rightVertex);
		}

		for (size_t i = 0; i + 3 < vertices_.size(); i += 2) {
			uint32_t l0 = static_cast<uint32_t>(i);
			uint32_t r0 = static_cast<uint32_t>(i + 1);
			uint32_t l1 = static_cast<uint32_t>(i + 2);
			uint32_t r1 = static_cast<uint32_t>(i + 3);

			indices_.push_back(l0);
			indices_.push_back(r0);
			indices_.push_back(l1);

			indices_.push_back(r0);
			indices_.push_back(r1);
			indices_.push_back(l1);
		}
	}
}
