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
	CreateTransformationMatrixBuffer();

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
		// NOTE:カメラがセットされてなくても描画できるようにするため
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
	if (!vertexBuffer_ ){
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
	// トランスフォーメーションマトリックスバッファの設定
	commandList->SetGraphicsRootConstantBufferView(1, transfomationMatrixBuffer_->GetGPUVirtualAddress());
	// テクスチャの設定（ルートパラメータ1）
	commandList->SetGraphicsRootDescriptorTable(1, TextureManager::GetInstance()->GetSrvHandleGPU(texturePath_));

	//========================================
	// 描画コール
	commandList->DrawInstanced(vertices_.size(), 1, 0, 0);
}

///=============================================================================
///						Boxの頂点データ作成
void Skybox::CreateBoxVertices() {
	//========================================
	// デバイスの取得
	auto device = skyboxSetup_->GetDXManager()->GetDevice();
	// バッファサイズ
	// NOTE: 1000本のラインを描画できるようにしている
	auto bufferSize = sizeof(LineVertex) * 100000000;
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
		IID_PPV_ARGS(&vertexBuffer_)
	);
	//========================================
	// バーテックスバッファビューの設定
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	// バイトサイズ
	vertexBufferView_.SizeInBytes = static_cast<UINT>( bufferSize );
	// ストライド
	vertexBufferView_.StrideInBytes = sizeof(LineVertex);
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
