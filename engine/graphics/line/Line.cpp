/*********************************************************************
 * \file   Line.cpp
 * \brief
 *
 * \author Harukichimaru
 * \date   January 2025
 * \note
 *********************************************************************/
#include "Line.h"
#include "Camera.h"
#include "DirectXCore.h"
#include "LineManager.h"
#include "LineSetup.h"
#include "Object3dSetup.h"
#include <algorithm>
#include <cstring>
//========================================
// 数学関数のインクルード
#define _USE_MATH_DEFINES
#include <math.h>
///=============================================================================
///                        namespace MagEngine
namespace MagEngine {
	namespace {
		constexpr float kMinLineThickness = 0.01f;
		constexpr float kHudThicknessScale = 0.01f;
		constexpr float kMinSegmentLength = 0.0001f;
		constexpr int kMaxDashSegmentsPerLine = 1024;

		float LineLength(const MagMath::Vector3 &v) {
			return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
		}

		MagMath::Vector3 Add(const MagMath::Vector3 &a, const MagMath::Vector3 &b) {
			return {a.x + b.x, a.y + b.y, a.z + b.z};
		}

		MagMath::Vector3 Subtract(const MagMath::Vector3 &a, const MagMath::Vector3 &b) {
			return {a.x - b.x, a.y - b.y, a.z - b.z};
		}

		MagMath::Vector3 Scale(const MagMath::Vector3 &v, float scale) {
			return {v.x * scale, v.y * scale, v.z * scale};
		}
	}

	Line::~Line() {
		Finalize();
	}

	///=============================================================================
	///						初期化
	void Line::Initialize(LineSetup *lineSetup) {
		//========================================
		// ラインセットアップの取得
		lineSetup_ = lineSetup;
		//========================================
		// 頂点バッファの作成
		CreateVertexBuffer();
		// トランスフォーメーションマトリックスバッファの作成
		CreateTransformationMatrixBuffer();
		//========================================
		// ワールド行列の初期化
		transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
		//========================================
		// カメラの取得
		camera_ = lineSetup_->GetDefaultCamera();
	}

	void Line::Finalize() {
		if (vertexBuffer_ && mappedVertexData_) {
			// NOTE: Upload Heapを永続Mapしているため、リソース解放前に必ずUnmapして参照を切る。
			vertexBuffer_->Unmap(0, nullptr);
		}
		mappedVertexData_ = nullptr;
		vertexBuffer_.Reset();
		transformationMatrixBuffer_.Reset();
		transformationMatrixData_ = nullptr;
		vertices_.clear();
		vertices_.shrink_to_fit();
		lineSetup_ = nullptr;
		camera_ = nullptr;
	}

	///=============================================================================
	///						更新
	void Line::Update() {
		// カメラの取得
		camera_ = lineSetup_->GetDefaultCamera();

		// ワールド行列の作成
		MagMath::Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
		MagMath::Matrix4x4 worldViewProjectionMatrix;

		if (camera_) {
			// ビュー行列とプロジェクション行列を取得
			const MagMath::Matrix4x4 &viewMatrix = camera_->GetViewMatrix();
			const MagMath::Matrix4x4 &projectionMatrix = camera_->GetProjectionMatrix();

			// 行列の乗算（ワールド → ビュー → プロジェクション）
			MagMath::Matrix4x4 worldViewMatrix = MagMath::Multiply4x4(worldMatrix, viewMatrix);
			worldViewProjectionMatrix = MagMath::Multiply4x4(worldViewMatrix, projectionMatrix);
		} else {
			worldViewProjectionMatrix = worldMatrix;
		}

		// 定数バッファへの書き込み
		transformationMatrixData_->WVP = worldViewProjectionMatrix;
		transformationMatrixData_->World = worldMatrix;
		transformationMatrixData_->WorldInvTranspose = Inverse4x4(worldMatrix);
	}

	///=============================================================================
	///						ライン描画
	void Line::DrawLine(const MagMath::Vector3 &start, const MagMath::Vector3 &end, const MagMath::Vector4 &color, float thickness) {
		LineStyle style{};
		style.color = color;
		style.thickness = thickness;
		DrawLine(start, end, style, LineRenderMode::World);
	}

	void Line::DrawLine(const MagMath::Vector3 &start, const MagMath::Vector3 &end, const LineStyle &style, LineRenderMode batchMode) {
		const LineStyle sanitizedStyle = SanitizeStyle(style);
		if (sanitizedStyle.color.w <= 0.0f) {
			return;
		}

		if (batchMode == LineRenderMode::Hud && sanitizedStyle.dashed) {
			const MagMath::Vector3 delta = Subtract(end, start);
			const float length = LineLength(delta);
			if (length <= kMinSegmentLength || sanitizedStyle.dashLength <= 0.0f || sanitizedStyle.gapLength < 0.0f) {
				return;
			}

			const MagMath::Vector3 direction = Scale(delta, 1.0f / length);
			float cursor = 0.0f;
			int segmentCount = 0;
			while (cursor < length && segmentCount < kMaxDashSegmentsPerLine) {
				const float dashEnd = (cursor + sanitizedStyle.dashLength < length) ? cursor + sanitizedStyle.dashLength : length;
				if (dashEnd > cursor) {
					AppendLineSegment(Add(start, Scale(direction, cursor)), Add(start, Scale(direction, dashEnd)), sanitizedStyle, batchMode);
				}
				cursor = dashEnd + sanitizedStyle.gapLength;
				++segmentCount;
			}
			return;
		}

		AppendLineSegment(start, end, sanitizedStyle, batchMode);
	}

	void Line::AddPolyline(std::span<const MagMath::Vector3> points, const LineStyle &style, LineRenderMode batchMode) {
		if (points.size() < 2) {
			return;
		}
		for (size_t i = 1; i < points.size(); ++i) {
			DrawLine(points[i - 1], points[i], style, batchMode);
		}
	}

	void Line::AddRect(const MagMath::Vector3 &min, const MagMath::Vector3 &max, const LineStyle &style, LineRenderMode batchMode) {
		const MagMath::Vector3 tl{min.x, max.y, min.z};
		const MagMath::Vector3 tr{max.x, max.y, min.z};
		const MagMath::Vector3 bl{min.x, min.y, min.z};
		const MagMath::Vector3 br{max.x, min.y, min.z};
		DrawLine(tl, tr, style, batchMode);
		DrawLine(tr, br, style, batchMode);
		DrawLine(br, bl, style, batchMode);
		DrawLine(bl, tl, style, batchMode);
	}

	void Line::AddCornerBracket(const MagMath::Vector3 &center, const MagMath::Vector3 &halfSize, float cornerLength, const LineStyle &style, LineRenderMode batchMode) {
		float arm = cornerLength;
		if (halfSize.x * 2.0f < arm) {
			arm = halfSize.x * 2.0f;
		}
		if (halfSize.y * 2.0f < arm) {
			arm = halfSize.y * 2.0f;
		}
		if (arm < 0.0f) {
			arm = 0.0f;
		}
		if (arm <= kMinSegmentLength) {
			return;
		}

		const MagMath::Vector3 tl{center.x - halfSize.x, center.y + halfSize.y, center.z};
		const MagMath::Vector3 tr{center.x + halfSize.x, center.y + halfSize.y, center.z};
		const MagMath::Vector3 bl{center.x - halfSize.x, center.y - halfSize.y, center.z};
		const MagMath::Vector3 br{center.x + halfSize.x, center.y - halfSize.y, center.z};

		DrawLine(tl, {tl.x + arm, tl.y, tl.z}, style, batchMode);
		DrawLine(tl, {tl.x, tl.y - arm, tl.z}, style, batchMode);
		DrawLine(tr, {tr.x - arm, tr.y, tr.z}, style, batchMode);
		DrawLine(tr, {tr.x, tr.y - arm, tr.z}, style, batchMode);
		DrawLine(bl, {bl.x + arm, bl.y, bl.z}, style, batchMode);
		DrawLine(bl, {bl.x, bl.y + arm, bl.z}, style, batchMode);
		DrawLine(br, {br.x - arm, br.y, br.z}, style, batchMode);
		DrawLine(br, {br.x, br.y + arm, br.z}, style, batchMode);
	}

	void Line::AddArrow(const MagMath::Vector3 &position, const MagMath::Vector3 &direction, float size, const LineStyle &style, LineRenderMode batchMode) {
		const float length = LineLength(direction);
		if (length <= kMinSegmentLength || size <= 0.0f) {
			return;
		}
		const MagMath::Vector3 forward = Scale(direction, 1.0f / length);
		const MagMath::Vector3 normal{-forward.y, forward.x, 0.0f};
		const MagMath::Vector3 tip = Add(position, Scale(forward, size));
		const MagMath::Vector3 base = Add(position, Scale(forward, size * 0.35f));
		DrawLine(position, tip, style, batchMode);
		DrawLine(tip, Add(base, Scale(normal, size * 0.28f)), style, batchMode);
		DrawLine(tip, Add(base, Scale(normal, -size * 0.28f)), style, batchMode);
	}

	void Line::AppendLineSegment(const MagMath::Vector3 &start, const MagMath::Vector3 &end, const LineStyle &style, LineRenderMode batchMode) {
		const MagMath::Vector3 delta = Subtract(end, start);
		if (LineLength(delta) <= kMinSegmentLength) {
			return;
		}

		if (batchMode == LineRenderMode::Hud) {
			AppendHudLineQuad(start, end, style);
			return;
		}

		if (vertices_.size() + 2 > maxVertexCount_) {
			// NOTE: 固定Upload Bufferを越える書き込みはGPUメモリ破壊につながるため、超過分だけ捨てる。
			return;
		}
		// 頂点データを追加
		vertices_.push_back({start, style.color, style.thickness, {0.0f, 0.0f, 0.0f}});
		// 頂点データを追加
		vertices_.push_back({end, style.color, style.thickness, {0.0f, 0.0f, 0.0f}});
	}

	void Line::AppendHudLineQuad(const MagMath::Vector3 &start, const MagMath::Vector3 &end, const LineStyle &style) {
		if (vertices_.size() + 6 > maxVertexCount_) {
			// NOTE: HUD Quadは6頂点単位。途中まで追加すると三角形が壊れるため丸ごと破棄する。
			return;
		}

		const MagMath::Vector3 delta = Subtract(end, start);
		const float length = LineLength(delta);
		if (length <= kMinSegmentLength) {
			return;
		}

		const MagMath::Vector3 direction = Scale(delta, 1.0f / length);
		// NOTE: HUDの既存thickness指定は旧Line幅の感覚で使われているため、Quadのワールド幅へ直接使わない。
		const float effectiveThickness = (style.thickness * kHudThicknessScale < kMinLineThickness) ? kMinLineThickness : style.thickness * kHudThicknessScale;
		const MagMath::Vector3 normal = Scale({-direction.y, direction.x, 0.0f}, effectiveThickness * 0.5f);
		const MagMath::Vector3 v0 = Add(start, normal);
		const MagMath::Vector3 v1 = Subtract(start, normal);
		const MagMath::Vector3 v2 = Add(end, normal);
		const MagMath::Vector3 v3 = Subtract(end, normal);

		vertices_.push_back({v0, style.color, effectiveThickness, {0.0f, 0.0f, 0.0f}});
		vertices_.push_back({v1, style.color, effectiveThickness, {0.0f, 0.0f, 0.0f}});
		vertices_.push_back({v2, style.color, effectiveThickness, {0.0f, 0.0f, 0.0f}});
		vertices_.push_back({v2, style.color, effectiveThickness, {0.0f, 0.0f, 0.0f}});
		vertices_.push_back({v1, style.color, effectiveThickness, {0.0f, 0.0f, 0.0f}});
		vertices_.push_back({v3, style.color, effectiveThickness, {0.0f, 0.0f, 0.0f}});
	}

	LineStyle Line::SanitizeStyle(const LineStyle &style) {
		LineStyle sanitized = style;
		if (sanitized.thickness < kMinLineThickness) {
			sanitized.thickness = kMinLineThickness;
		}
		if (sanitized.color.w < 0.0f) {
			sanitized.color.w = 0.0f;
		} else if (sanitized.color.w > 1.0f) {
			sanitized.color.w = 1.0f;
		}
		if (sanitized.dashLength <= 0.0f || sanitized.gapLength < 0.0f) {
			// NOTE: 不正な破線指定は大量分割や無限ループを避けるため通常線にフォールバックする。
			sanitized.dashed = false;
		}
		return sanitized;
	}

	///=============================================================================
	///                     描画
	void Line::Draw() {
		//========================================
		// 描画するラインがない場合は何もしない
		if (vertices_.empty())
			return;

		//========================================
		// バッファサイズを動的に拡張
		if (vertices_.size() > maxVertexCount_) {
			// バッファを拡張し直す必要があります
			// 現在のフレームは描画をスキップして、次フレームに備える
			return;
		}

		if (!mappedVertexData_) {
			return;
		}
		// NOTE: Lineは毎フレーム全頂点を作り直すため、Upload Heapを永続MapしてMap/Unmapコストを避ける。
		std::memcpy(mappedVertexData_, vertices_.data(), sizeof(LineVertex) * vertices_.size());

		//========================================
		// 描画設定
		auto commandList = lineSetup_->GetDXManager()->GetCommandList();
		// transformationMatrixBufferのセット
		commandList->SetGraphicsRootConstantBufferView(0, transformationMatrixBuffer_->GetGPUVirtualAddress());
		// vertexBufferの設定
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
		//========================================
		// 描画
		commandList->DrawInstanced(static_cast<UINT>(vertices_.size()), 1, 0, 0);
		// NOTE:描画した後はラインをクリアするのを忘れるな
	}

	///=============================================================================
	///						ラインのクリア
	void Line::ClearLines() {
		// ラインのクリア
		vertices_.clear();
	}

	///=============================================================================
	///						バーテックスバッファの作成
	void Line::CreateVertexBuffer() {
		//========================================
		// デバイスの取得
		auto device = lineSetup_->GetDXManager()->GetDevice();
		// バッファサイズ
		// NOTE: 初期配置は50000ラインまで描画可能。必要に応じて動的に拡張。
		maxVertexCount_ = 100000; // 50000ラインx2頂点
		size_t bufferSize = sizeof(LineVertex) * maxVertexCount_;
		vertices_.reserve(maxVertexCount_);
		//========================================
		// バーテックスバッファの作成
		D3D12_HEAP_PROPERTIES heapProps = {};
		// ヒープタイプ
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		//========================================
		// リソースの設定
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Width = bufferSize;
		bufferDesc.Height = 1;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		//========================================
		// リソースの作成
		device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer_));
		// NOTE: Line頂点はCPUから毎フレーム更新する一時データなので、Upload Heapを永続Mapして使い回す。
		D3D12_RANGE range = {0, 0};
		vertexBuffer_->Map(0, &range, reinterpret_cast<void **>(&mappedVertexData_));
		//========================================
		// バーテックスバッファビューの設定
		vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
		// バイトサイズ
		vertexBufferView_.SizeInBytes = static_cast<UINT>(bufferSize);
		// ストライド
		vertexBufferView_.StrideInBytes = sizeof(LineVertex);
	}

	///=============================================================================
	///
	void Line::CreateTransformationMatrixBuffer() {
		// 定数バッファのサイズを 256 バイトの倍数に設定
		size_t bufferSize = (sizeof(MagMath::TransformationMatrix) + 255) & ~255;
		transformationMatrixBuffer_ = lineSetup_->GetDXManager()->CreateBufferResource(bufferSize);
		// 書き込み用変数
		MagMath::TransformationMatrix transformationMatrix = {};
		// 書き込むためのアドレスを取得
		transformationMatrixBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&transformationMatrixData_));
		// 書き込み
		transformationMatrix.WVP = MagMath::Identity4x4();
		// 単位行列を書き込む
		*transformationMatrixData_ = transformationMatrix;
	}

}
