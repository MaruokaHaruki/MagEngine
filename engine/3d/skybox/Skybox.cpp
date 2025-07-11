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
	// キューブの頂点データ作成
	CreateCubeVertices();

	//========================================
	// ViewProjection行列バッファの作成
	CreateTransformationMatrixBuffer();

	//========================================
	// ワールド行列の初期化（スカイボックスは大きくする）
	transform_ = {{100.0f, 100.0f, 100.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

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
	// TransformからWorld行列を作成
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	Matrix4x4 worldViewProjectionMatrix;

	//========================================
	// カメラがセットされている場合はビュー行列を作成
	if (camera_) {
		// カメラのビュー行列を取得
		const Matrix4x4 &viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		// ワールドビュープロジェクション行列を計算
		worldViewProjectionMatrix = Multiply4x4(worldMatrix, viewProjectionMatrix);
	} else {
		// カメラがセットされていない場合はワールド行列をそのまま使う
		worldViewProjectionMatrix = worldMatrix;
	}
	//========================================
	// トランスフォーメーションマトリックスバッファに書き込む
	transformationMatrixData_->WVP = worldViewProjectionMatrix;
	transformationMatrixData_->World = worldMatrix;
	transformationMatrixData_->WorldInvTranspose = Inverse4x4(worldMatrix);
}

///=============================================================================
///						描画
void Skybox::Draw() {
	//========================================
	// バッファが存在しない場合は描画しない
	if (!vertexBuffer_ || !indexBuffer_) {
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
	// 頂点バッファとインデックスバッファの設定
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);

	//========================================
	// ルートパラメータの設定
	// トランスフォーメーションマトリックスバッファの設定
	commandList->SetGraphicsRootConstantBufferView(0, transfomationMatrixBuffer_->GetGPUVirtualAddress());
	// テクスチャの設定
	commandList->SetGraphicsRootDescriptorTable(1, TextureManager::GetInstance()->GetSrvHandleGPU(texturePath_));

	//========================================
	// 描画コール
	commandList->DrawIndexedInstanced(static_cast<UINT>(indices_.size()), 1, 0, 0, 0);
}

///=============================================================================
///						キューブの頂点データ作成
void Skybox::CreateCubeVertices() {
	//========================================
	// キューブの頂点データを作成
	vertices_ = {
		// 前面
		{{-1.0f, -1.0f, -1.0f, 1.0f}},
		{{1.0f, -1.0f, -1.0f, 1.0f}},
		{{1.0f, 1.0f, -1.0f, 1.0f}},
		{{-1.0f, 1.0f, -1.0f, 1.0f}},
		// 背面
		{{1.0f, -1.0f, 1.0f, 1.0f}},
		{{-1.0f, -1.0f, 1.0f, 1.0f}},
		{{-1.0f, 1.0f, 1.0f, 1.0f}},
		{{1.0f, 1.0f, 1.0f, 1.0f}},
	};

	//========================================
	// インデックスデータ（時計回り）
	indices_ = {
		// 前面
		0,
		1,
		2,
		2,
		3,
		0,
		// 背面
		4,
		5,
		6,
		6,
		7,
		4,
		// 左面
		5,
		0,
		3,
		3,
		6,
		5,
		// 右面
		1,
		4,
		7,
		7,
		2,
		1,
		// 上面
		3,
		2,
		7,
		7,
		6,
		3,
		// 下面
		5,
		4,
		1,
		1,
		0,
		5,
	};

	//========================================
	// デバイスの取得
	auto device = skyboxSetup_->GetDXManager()->GetDevice();

	//========================================
	// 頂点バッファの作成
	auto vertexBufferSize = sizeof(SkyboxVertex) * vertices_.size();
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = vertexBufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer_));

	// 頂点データをバッファに書き込み
	SkyboxVertex *vertexData = nullptr;
	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));
	std::memcpy(vertexData, vertices_.data(), vertexBufferSize);
	vertexBuffer_->Unmap(0, nullptr);

	// 頂点バッファビューの設定
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
	vertexBufferView_.StrideInBytes = sizeof(SkyboxVertex);

	//========================================
	// インデックスバッファの作成
	auto indexBufferSize = sizeof(uint32_t) * indices_.size();
	bufferDesc.Width = indexBufferSize;

	device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuffer_));

	// インデックスデータをバッファに書き込み
	uint32_t *indexData = nullptr;
	indexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
	std::memcpy(indexData, indices_.data(), indexBufferSize);
	indexBuffer_->Unmap(0, nullptr);

	// インデックスバッファビューの設定
	indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = static_cast<UINT>(indexBufferSize);
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
}

void Skybox::CreateTransformationMatrixBuffer() {
	// 定数バッファのサイズを 256 バイトの倍数に設定
	size_t bufferSize = (sizeof(TransformationMatrix) + 255) & ~255;
	transfomationMatrixBuffer_ = skyboxSetup_->GetDXManager()->CreateBufferResource(bufferSize);
	// 書き込み用変数
	TransformationMatrix transformationMatrix = {};
	// 書き込むためのアドレスを取得
	transfomationMatrixBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&transformationMatrixData_));
	// 書き込み
	transformationMatrix.WVP = Identity4x4();
	// 単位行列を書き込む
	*transformationMatrixData_ = transformationMatrix;
}
