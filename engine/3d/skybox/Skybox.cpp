/*********************************************************************
 * \file   Skybox.cpp
 * \brief  スカイボックス描画クラス
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#include "Skybox.h"
#include "AffineTransformations.h"
#include "Camera.h"
#include "MathFunc4x4.h"
#include "SkyboxSetup.h"
#include "TextureManager.h"
///=============================================================================
///						初期化
void Skybox::Initialize(SkyboxSetup *skyboxSetup) {
	//========================================
	// 引数からSetupを受け取る
	this->skyboxSetup_ = skyboxSetup;

	//========================================
	// Boxの頂点データ作成
	CreateBoxVertices();

	//========================================
	// ViewProjection行列バッファの作成
	CreateViewProjectionBuffer();

	//========================================
	// ワールド行列の初期化（スカイボックスは大きくする）
	transform_ = {{1000.0f, 1000.0f, 1000.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

	//========================================
	// カメラの取得
	camera_ = skyboxSetup_->GetDefaultCamera();
}

///=============================================================================
///						更新
void Skybox::Update() {
	//========================================
	// カメラの位置を取得
	camera_ = skyboxSetup_->GetDefaultCamera();

	//========================================
	// TransformからWorld行列を作成（スケールのみ適用）
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f});
	Matrix4x4 worldViewProjectionMatrix;

	//========================================
	// カメラがセットされている場合はビュー行列を作成
	if (camera_) {
		// カメラのビュー行列を取得
		Matrix4x4 viewMatrix = camera_->GetViewMatrix();
		Matrix4x4 projectionMatrix = camera_->GetProjectionMatrix();

		// ビュー行列から平行移動成分を除去（3x3の回転行列のみ使用）
		Matrix4x4 rotationOnlyViewMatrix = {
			viewMatrix.m[0][0], viewMatrix.m[0][1], viewMatrix.m[0][2], 0.0f,
			viewMatrix.m[1][0], viewMatrix.m[1][1], viewMatrix.m[1][2], 0.0f,
			viewMatrix.m[2][0], viewMatrix.m[2][1], viewMatrix.m[2][2], 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f};

		// ワールドビュープロジェクション行列を計算
		Matrix4x4 worldViewMatrix = Multiply4x4(worldMatrix, rotationOnlyViewMatrix);
		worldViewProjectionMatrix = Multiply4x4(worldViewMatrix, projectionMatrix);
	} else {
		// カメラがセットされていない場合はワールド行列をそのまま使う
		worldViewProjectionMatrix = worldMatrix;
	}

	//========================================
	// ViewProjection行列バッファを更新
	viewProjectionData_->viewProjection = worldViewProjectionMatrix;
}

///=============================================================================
///						描画
void Skybox::Draw() {
	//========================================
	// バッファが存在しない場合は描画しない
	if (!vertexBuffer_ || !indexBuffer_ || !viewProjectionBuffer_) {
		throw std::runtime_error("Skybox buffers are not initialized.");
	}

	//========================================
	// テクスチャが設定されていない場合は描画しない
	if (texturePath_.empty()) {
		return;
	}

	//========================================
	// コマンドリスト取得
	auto commandList = skyboxSetup_->GetDXManager()->GetCommandList();

	//========================================
	// 頂点バッファの設定
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	// インデックスバッファの設定
	commandList->IASetIndexBuffer(&indexBufferView_);

	//========================================
	// ルートパラメータの設定
	// ViewProjection行列の設定（ルートパラメータ0）
	commandList->SetGraphicsRootConstantBufferView(0, viewProjectionBuffer_->GetGPUVirtualAddress());
	// テクスチャの設定（ルートパラメータ1）
	commandList->SetGraphicsRootDescriptorTable(1, TextureManager::GetInstance()->GetSrvHandleGPU(texturePath_));

	//========================================
	// 描画コール
	commandList->DrawIndexedInstanced(kIndexCount, 1, 0, 0, 0);
}

///=============================================================================
///						Boxの頂点データ作成
void Skybox::CreateBoxVertices() {
	//========================================
	// 頂点バッファの作成
	vertexBuffer_ = skyboxSetup_->GetDXManager()->CreateBufferResource(sizeof(SkyboxVertex) * kVertexCount);

	//========================================
	// 頂点バッファビューの作成
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(SkyboxVertex) * kVertexCount;
	vertexBufferView_.StrideInBytes = sizeof(SkyboxVertex);

	//========================================
	// 頂点データをマップ
	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData_));

	//========================================
	// Boxの頂点座標を設定（-1.0f ~ 1.0f の範囲）
	vertexData_[0].position = {-1.0f, -1.0f, -1.0f, 1.0f}; // 左下奥
	vertexData_[1].position = {-1.0f, 1.0f, -1.0f, 1.0f};  // 左上奥
	vertexData_[2].position = {1.0f, 1.0f, -1.0f, 1.0f};   // 右上奥
	vertexData_[3].position = {1.0f, -1.0f, -1.0f, 1.0f};  // 右下奥
	vertexData_[4].position = {-1.0f, -1.0f, 1.0f, 1.0f};  // 左下手前
	vertexData_[5].position = {-1.0f, 1.0f, 1.0f, 1.0f};   // 左上手前
	vertexData_[6].position = {1.0f, 1.0f, 1.0f, 1.0f};	   // 右上手前
	vertexData_[7].position = {1.0f, -1.0f, 1.0f, 1.0f};   // 右下手前

	//========================================
	// インデックスバッファの作成
	indexBuffer_ = skyboxSetup_->GetDXManager()->CreateBufferResource(sizeof(uint32_t) * kIndexCount);

	//========================================
	// インデックスバッファビューの作成
	indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * kIndexCount;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	//========================================
	// インデックスデータをマップ
	indexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&indexData_));

	//========================================
	// Boxのインデックスを設定（内側から見るので反時計回り）
	uint32_t indices[kIndexCount] = {
		// 奥面 (Z = -1)
		0, 2, 1, 0, 3, 2,
		// 手前面 (Z = 1)
		4, 5, 6, 4, 6, 7,
		// 左面 (X = -1)
		0, 1, 5, 0, 5, 4,
		// 右面 (X = 1)
		3, 6, 2, 3, 7, 6,
		// 下面 (Y = -1)
		0, 4, 7, 0, 7, 3,
		// 上面 (Y = 1)
		1, 2, 6, 1, 6, 5};

	// インデックスデータをコピー
	for (uint32_t i = 0; i < kIndexCount; ++i) {
		indexData_[i] = indices[i];
	}
}

///=============================================================================
///						ViewProjection行列バッファの作成
void Skybox::CreateViewProjectionBuffer() {
	// 定数バッファのサイズを 256 バイトの倍数に設定
	size_t bufferSize = (sizeof(SkyboxViewProjection) + 255) & ~255;
	viewProjectionBuffer_ = skyboxSetup_->GetDXManager()->CreateBufferResource(bufferSize);

	// 書き込み用変数
	SkyboxViewProjection viewProjection = {};
	// 書き込むためのアドレスを取得
	viewProjectionBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&viewProjectionData_));
	// 単位行列を書き込む
	viewProjection.viewProjection = Identity4x4();
	*viewProjectionData_ = viewProjection;
}
