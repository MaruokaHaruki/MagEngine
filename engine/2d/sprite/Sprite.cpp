/*********************************************************************
 * \file   Sprite.cpp
 * \brief  スプライトクラス実装 - 2D画像描画システム
 *
 * \author Harukichimaru
 * \date   October 2024
 * \note   軽量で高速な2Dスプライト描画を実現
 *********************************************************************/
#include "Sprite.h"
#include "SpriteSetup.h"
#include "TextureManager.h"

///=============================================================================
///                        namespace MagEngine
namespace MagEngine {

	///=============================================================================
	///                        初期化・終了
	///=============================================================================

	///--------------------------------------------------------------
	///                         初期化
	void Sprite::Initialize(SpriteSetup *spriteSetup, std::string textureFilePath) {
		// スプライト管理クラスを記録
		this->spriteSetup_ = spriteSetup;

		// GPUリソースの作成
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateMaterialBuffer();
		CreateTransformationMatrixBuffer();

		// テクスチャの読み込みと設定
		textureFilePath_ = textureFilePath;
		TextureManager::GetInstance()->LoadTexture(textureFilePath);

		// テクスチャのサイズに合わせて初期化
		AdjustTextureSize();
	}

	///=============================================================================
	///                        更新・描画
	///=============================================================================

	///--------------------------------------------------------------
	///                         更新
	void Sprite::Update(MagMath::Matrix4x4 viewMatrix) {
		//---------------------------------------
		// テクスチャ範囲の反映
		ReflectTextureRange();

		//---------------------------------------
		// アンカーポイントとフリップの反映
		ReflectAnchorPointAndFlip();

		//---------------------------------------
		// SRT（スケール・回転・平行移動）の反映
		ReflectSRT();

		//---------------------------------------
		// ワールド行列の作成
		MagMath::Matrix4x4 worldMatrix = MakeAffineMatrix(
			transform_.scale,
			transform_.rotate,
			transform_.translate);
		transformationMatrixData_->World = worldMatrix;

		//---------------------------------------
		// 正射影行列の作成（画面サイズに合わせた2D座標系）
		MagMath::Matrix4x4 projectionMatrix = MagMath::MakeOrthographicMatrix(
			0.0f, 0.0f,
			static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth()),
			static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight()),
			0.0f, 100.0f);

		//---------------------------------------
		// WVP行列の計算
		MagMath::Matrix4x4 worldViewProjectionMatrix = Multiply4x4(
			worldMatrix,
			Multiply4x4(viewMatrix, projectionMatrix));
		transformationMatrixData_->WVP = worldViewProjectionMatrix;
	}

	///--------------------------------------------------------------
	///                         描画
	void Sprite::Draw() {
		// バッファの有効性チェック（デバッグビルドのみ）
		#ifdef _DEBUG
		if (!vertexBuffer_ || !indexBuffer_ || !materialBuffer_ || !transformationMatrixBuffer_) {
			throw std::runtime_error("Sprite: One or more buffers are not initialized.");
		}
		#endif

		// コマンドリスト取得（キャッシュして再利用）
		auto commandList = spriteSetup_->GetDXManager()->GetCommandList();

		// 頂点・インデックスバッファの設定
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
		commandList->IASetIndexBuffer(&indexBufferView_);

		// プリミティブトポロジー設定（三角形リスト）
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// マテリアルとトランスフォーム行列の設定
		commandList->SetGraphicsRootConstantBufferView(0, materialBuffer_->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixBuffer_->GetGPUVirtualAddress());

		// テクスチャの設定
		commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureFilePath_));

		// 描画コール（インスタンス描画、6頂点 = 2三角形）
		commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

	///=============================================================================
	///                        便利機能
	///=============================================================================

	///--------------------------------------------------------------
	///                    テクスチャサイズに合わせる
	Sprite *Sprite::FitToTexture() {
		AdjustTextureSize();
		return this;
	}

	///--------------------------------------------------------------
	///                    画面中央に配置
	Sprite *Sprite::CenterOnScreen() {
		if (!spriteSetup_) {
			return this;
		}

		// 画面サイズの取得
		float screenWidth = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth());
		float screenHeight = static_cast<float>(spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight());

		// 画面中央に配置
		position_.x = screenWidth * 0.5f;
		position_.y = screenHeight * 0.5f;

		// アンカーポイントも中央に設定
		anchorPoint_ = {0.5f, 0.5f};

		return this;
	}

	///=============================================================================
	///                        バッファ作成
	///=============================================================================

	///--------------------------------------------------------------
	///                    頂点バッファの作成
	void Sprite::CreateVertexBuffer() {
		// 頂点データ用のGPUリソースを作成（4頂点分）
		vertexBuffer_ = spriteSetup_->GetDXManager()->CreateBufferResource(sizeof(MagMath::VertexData) * 4);

		#ifdef _DEBUG
		if (!vertexBuffer_) {
			throw std::runtime_error("Sprite: Failed to create vertex buffer");
		}
		#endif

		// バッファビューの設定
		vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
		vertexBufferView_.SizeInBytes = sizeof(MagMath::VertexData) * 4;
		vertexBufferView_.StrideInBytes = sizeof(MagMath::VertexData);

		// CPUからアクセス可能なようにマップ
		vertexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));

		// 矩形の4頂点を設定（左下原点の2D座標系）
		// 左下
		vertexData_[0].position = {0.0f, 1.0f, 0.0f, 1.0f};
		vertexData_[0].texCoord = {0.0f, 1.0f};
		vertexData_[0].normal = {0.0f, 0.0f, -1.0f};

		// 左上
		vertexData_[1].position = {0.0f, 0.0f, 0.0f, 1.0f};
		vertexData_[1].texCoord = {0.0f, 0.0f};
		vertexData_[1].normal = {0.0f, 0.0f, -1.0f};

		// 右下
		vertexData_[2].position = {1.0f, 1.0f, 0.0f, 1.0f};
		vertexData_[2].texCoord = {1.0f, 1.0f};
		vertexData_[2].normal = {0.0f, 0.0f, -1.0f};

		// 右上
		vertexData_[3].position = {1.0f, 0.0f, 0.0f, 1.0f};
		vertexData_[3].texCoord = {1.0f, 0.0f};
		vertexData_[3].normal = {0.0f, 0.0f, -1.0f};
	}

	///--------------------------------------------------------------
	///                    インデックスバッファの作成
	void Sprite::CreateIndexBuffer() {
		// インデックスデータ用のGPUリソースを作成（6インデックス = 2三角形）
		indexBuffer_ = spriteSetup_->GetDXManager()->CreateBufferResource(sizeof(uint32_t) * 6);

		#ifdef _DEBUG
		if (!indexBuffer_) {
			throw std::runtime_error("Sprite: Failed to create index buffer");
		}
		#endif

		// バッファビューの設定
		indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
		indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
		indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

		// CPUからアクセス可能なようにマップ
		indexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&indexData_));

		// 2つの三角形でスプライトを構成
		// 三角形1: 左下(0) → 左上(1) → 右下(2)
		indexData_[0] = 0;
		indexData_[1] = 1;
		indexData_[2] = 2;

		// 三角形2: 左上(1) → 右上(3) → 右下(2)
		indexData_[3] = 1;
		indexData_[4] = 3;
		indexData_[5] = 2;
	}

	///--------------------------------------------------------------
	///                    マテリアルバッファの作成
	void Sprite::CreateMaterialBuffer() {
		// マテリアルデータ用のGPUリソースを作成
		materialBuffer_ = spriteSetup_->GetDXManager()->CreateBufferResource(sizeof(MagMath::Material));

		// CPUからアクセス可能なようにマップ
		materialBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));

		// マテリアルの初期値を設定（白色・不透明）
		MagMath::Material material = {{1.0f, 1.0f, 1.0f, 1.0f}, false};
		material.uvTransform = MagMath::Identity4x4();

		// マテリアルデータの書き込み
		*materialData_ = material;
	}

	///--------------------------------------------------------------
	///                    トランスフォーム行列バッファの作成
	void Sprite::CreateTransformationMatrixBuffer() {
		// トランスフォーム行列用のGPUリソースを作成
		transformationMatrixBuffer_ = spriteSetup_->GetDXManager()->CreateBufferResource(sizeof(MagMath::TransformationMatrix));

		// CPUからアクセス可能なようにマップ
		transformationMatrixBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&transformationMatrixData_));

		// 単位行列で初期化
		MagMath::TransformationMatrix transformationMatrix;
		transformationMatrix.WVP = MagMath::Identity4x4();
		transformationMatrix.World = MagMath::Identity4x4();

		// トランスフォーム行列データの書き込み
		*transformationMatrixData_ = transformationMatrix;
	}

	///=============================================================================
	///                        更新処理
	///=============================================================================

	///--------------------------------------------------------------
	///                    SRTの反映
	void Sprite::ReflectSRT() {
		// スケール（サイズをそのまま使用）
		transform_.scale = {size_.x, size_.y, 1.0f};

		// 回転（Z軸回りの回転）
		transform_.rotate = {0.0f, 0.0f, rotation_};

		// 平行移動（2D座標を3D空間に配置）
		transform_.translate = {position_.x, position_.y, 0.0f};
	}

	///--------------------------------------------------------------
	///                    アンカーポイントとフリップの反映
	void Sprite::ReflectAnchorPointAndFlip() {
		//---------------------------------------
		// アンカーポイントの計算
		// (0.0, 0.0) = 左上
		// (0.5, 0.5) = 中央
		// (1.0, 1.0) = 右下
		float left = 0.0f - anchorPoint_.x;
		float right = 1.0f - anchorPoint_.x;
		float top = 0.0f - anchorPoint_.y;
		float bottom = 1.0f - anchorPoint_.y;

		//---------------------------------------
		// フリップの適用
		if (isFlipX_) {
			// 左右フリップ: 左右を反転
			left = -left;
			right = -right;
		}
		if (isFlipY_) {
			// 上下フリップ: 上下を反転
			top = -top;
			bottom = -bottom;
		}

		//---------------------------------------
		// 頂点位置の更新
		vertexData_[0].position = {left, bottom, 0.0f, 1.0f};	  // 左下
		vertexData_[1].position = {left, top, 0.0f, 1.0f};		  // 左上
		vertexData_[2].position = {right, bottom, 0.0f, 1.0f};	  // 右下
		vertexData_[3].position = {right, top, 0.0f, 1.0f};		  // 右上
	}

	///--------------------------------------------------------------
	///                    テクスチャ範囲の反映
	void Sprite::ReflectTextureRange() {
		// テクスチャメタデータの取得
		const DirectX::TexMetadata &metadata = TextureManager::GetInstance()->GetMetadata(textureFilePath_);

		// テクスチャのサイズ
		float textureWidth = static_cast<float>(metadata.width);
		float textureHeight = static_cast<float>(metadata.height);

		// UV座標の計算（0.0～1.0の範囲に正規化）
		float textureLeft = textureLeftTop_.x / textureWidth;
		float textureRight = (textureLeftTop_.x + textureSize_.x) / textureWidth;
		float textureTop = textureLeftTop_.y / textureHeight;
		float textureBottom = (textureLeftTop_.y + textureSize_.y) / textureHeight;

		// UV座標の更新
		vertexData_[0].texCoord = {textureLeft, textureBottom};	  // 左下
		vertexData_[1].texCoord = {textureLeft, textureTop};	  // 左上
		vertexData_[2].texCoord = {textureRight, textureBottom};  // 右下
		vertexData_[3].texCoord = {textureRight, textureTop};	  // 右上
	}

	///--------------------------------------------------------------
	///                    テクスチャサイズをスプライトに統合
	void Sprite::AdjustTextureSize() {
		// テクスチャメタデータの取得
		const DirectX::TexMetadata &metadata = TextureManager::GetInstance()->GetMetadata(textureFilePath_);

		// テクスチャの幅と高さ
		float textureWidth = static_cast<float>(metadata.width);
		float textureHeight = static_cast<float>(metadata.height);

		// スプライトのサイズをテクスチャのサイズに設定
		size_ = {textureWidth, textureHeight};

		// テクスチャの切り出し範囲をテクスチャ全体に設定
		textureLeftTop_ = {0.0f, 0.0f};
		textureSize_ = {textureWidth, textureHeight};
	}
}
