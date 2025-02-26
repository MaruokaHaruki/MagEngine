/*********************************************************************
 * \file   Sprite.cpp
 * \brief  スプライト
 *
 * \author Harukichimaru
 * \date   October 2024
 * \note
 *********************************************************************/
#include "Sprite.h"
#include "SpriteSetup.h"	
#include "AffineTransformations.h"
#include "TextureManager.h"
#include "MathFunc4x4.h"

 ///=============================================================================
 ///								初期化
void Sprite::Initialize(SpriteSetup* SpriteSetup, std::string textureFilePath) {
	//引数で受け取ってメンバ変数に記録する
	this->spriteSetup_ = SpriteSetup;

	//頂点バッファの作成
	CreateVertexBuffer();
	//インデックスバッファの作成
	CreateIndexBuffer();
	//マテリアルバッファの作成
	CreateMaterialBuffer();
	//トランスフォーメーションマトリックスバッファの作成
	CreateTransformationMatrixBuffer();

	//ファイルパスの記録
	textureFilePath_ = textureFilePath;
	//textureIndex = TextureManager::GetInstance()->GetTextureIndex(textureFilePath);


	//テクスチャのサイズを取得
	AdjustTextureSize();
}

///=============================================================================
///									更新
//NOTE:引数としてローカル行列とビュー行列を持ってくること
void Sprite::Update(Matrix4x4 viewMatrix) {

	//---------------------------------------
	// テクスチャ範囲の反映
	ReflectTextureRange();

	//---------------------------------------
	// アンカーポイントの反映
	ReflectAnchorPointAndFlip();

	//---------------------------------------
	// SRTの反映
	ReflectSRT();

	//---------------------------------------
	// スプライトの変換行列を作成
	Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	transformationMatrixData_->World = worldMatrixSprite;

	//---------------------------------------
	// 正射影行列の作成
	Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(
		0.0f, 0.0f,
		float(spriteSetup_->GetDXManager()->GetWinApp().GetWindowWidth()),
		float(spriteSetup_->GetDXManager()->GetWinApp().GetWindowHeight()),
		0.0f, 100.0f);

	//---------------------------------------
	// ワールド・ビュー・プロジェクション行列を計算
	Matrix4x4 worldViewProjectionMatrixSprite = Multiply4x4(worldMatrixSprite, Multiply4x4(viewMatrix, projectionMatrixSprite));
	transformationMatrixData_->WVP = worldViewProjectionMatrixSprite;
}

///=============================================================================
///									描画
void Sprite::Draw() {
	///2D描画
	//NOTE:Material用のCBuffer(色)とSRV(Texture)は3Dの三角形と同じものを使用。無駄を省け。
	//NOTE:同じものを使用したな？気をつけろ、別々の描画をしたいときは個別のオブジェクトとして宣言し直せ。
	// まず、描画時に使うバッファが有効か確認する
	if(!vertexBuffer_ || !indexBuffer_ || !materialBuffer_ || !transfomationMatrixBuffer_) {
		throw std::runtime_error("One or more buffers are not initialized.");
	}

	// コマンドリスト取得
	auto commandList = spriteSetup_->GetDXManager()->GetCommandList();

	// 頂点バッファの設定
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// インデックスバッファの設定
	commandList->IASetIndexBuffer(&indexBufferView_);

	// プリミティブのトポロジーを設定
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Material と TransformationMatrix の設定
	commandList->SetGraphicsRootConstantBufferView(0, materialBuffer_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(1, transfomationMatrixBuffer_->GetGPUVirtualAddress());

	// テクスチャの設定
	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureFilePath_));
	
	// 描画コール
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}


///=============================================================================
///									ローカル関数
///--------------------------------------------------------------
///						 頂点バッファの作成
void Sprite::CreateVertexBuffer() {
	// 頂点データ用のバッファリソースを作成
	vertexBuffer_ = spriteSetup_->GetDXManager()->CreateBufferResource(sizeof(VertexData) * 4);

	if(!vertexBuffer_) {
		throw std::runtime_error("Failed to Create Vertex Buffer");
		return;
	}

	// バッファビューの設定
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// リソースにデータを書き込む
	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>( &vertexData_ ));

	// 2つの三角形の頂点データを設定
	vertexData_[0].position = { 0.0f, 1.0f, 0.0f, 1.0f };
	vertexData_[0].texCoord = { 0.0f, 1.0f };
	vertexData_[0].normal = { 0.0f, 0.0f, -1.0f };

	vertexData_[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	vertexData_[1].texCoord = { 0.0f, 0.0f };
	vertexData_[1].normal = { 0.0f, 0.0f, -1.0f };

	vertexData_[2].position = { 1.0f, 1.0f, 0.0f, 1.0f };
	vertexData_[2].texCoord = { 1.0f, 1.0f };
	vertexData_[2].normal = { 0.0f, 0.0f, -1.0f };

	vertexData_[3].position = { 1.0f, 0.0f, 0.0f, 1.0f };
	vertexData_[3].texCoord = { 1.0f, 0.0f };
	vertexData_[3].normal = { 0.0f, 0.0f, -1.0f };
}

void Sprite::CreateIndexBuffer() {
	// インデックスデータ用のバッファリソースを作成
	indexBuffer_ = spriteSetup_->GetDXManager()->CreateBufferResource(sizeof(uint32_t) * 6);

	if(!indexBuffer_) {
		throw std::runtime_error("Failed to Create Index Buffer");
		return;
	}

	// バッファビューの設定
	indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// リソースにデータを書き込む
	indexBuffer_->Map(0, nullptr, reinterpret_cast<void**>( &indexData_ ));

	// インデックスデータを設定
	indexData_[0] = 0;
	indexData_[1] = 1;
	indexData_[2] = 2;
	indexData_[3] = 1;
	indexData_[4] = 3;
	indexData_[5] = 2;
}

///--------------------------------------------------------------
///						マテリアルバッファの作成
void Sprite::CreateMaterialBuffer() {
	// マテリアルデータ用のバッファリソースを作成
	materialBuffer_ = spriteSetup_->GetDXManager()->CreateBufferResource(sizeof(Material));
	//書き込むためのアドレス取得
	materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>( &materialData_ ));
	//マテリアルデータ書き込み用変数(赤色を書き込み)
	Material materialSprite = { {1.0f, 1.0f, 1.0f, 1.0f},false };
	//UVトランスフォーム用の単位行列の書き込み
	materialSprite.uvTransform = Identity4x4();
	//マテリアルデータの書き込み
	*materialData_ = materialSprite;
}

///--------------------------------------------------------------
///			トランスフォーメーションマトリックスバッファの作成
void Sprite::CreateTransformationMatrixBuffer() {
	//wvp用のリソースを作る
	transfomationMatrixBuffer_ = spriteSetup_->GetDXManager()->CreateBufferResource(sizeof(TransformationMatrix));
	//書き込むためのアドレスを取得
	transfomationMatrixBuffer_->Map(0, nullptr, reinterpret_cast<void**>( &transformationMatrixData_ ));
	//書き込み用変数
	TransformationMatrix transformationMatrix;
	//単位行列の書き込み
	transformationMatrix.WVP = Identity4x4();
	//トランスフォーメーションマトリックスの書き込み
	*transformationMatrixData_ = transformationMatrix;
}


///=============================================================================
///								反映処理
///--------------------------------------------------------------
///						 SRTの反映
void Sprite::ReflectSRT() {
	//サイズの反映
	transform_.scale = { size_.x,size_.y,0 };
	//回転の反映
	transform_.rotate = { 0.0f,0.0f,rotation_ };
	//座標の反映
	transform_.translate = { position_.x,position_.y,0.0f };
}

///--------------------------------------------------------------
/// 						アンカーポイントとフリップの反映
void Sprite::ReflectAnchorPointAndFlip() {
	//---------------------------------------
	// アンカーポイントの反映
	float left = 0.0f - anchorPoint_.x;
	float right = 1.0f - anchorPoint_.x;
	float top = 0.0f - anchorPoint_.y;
	float bottom = 1.0f - anchorPoint_.y;

	//---------------------------------------
	// フリップの反映
	//左右フリップの反映
	if(isFlipX_) {
		left = -left;
		right = -right;
	}
	//上下フリップの反映
	if(isFlipY_) {
		top = -top;
		bottom = -bottom;
	}

	//vertexDataにアンカーポイントを反映
	vertexData_[0].position = { left, bottom, 0.0f, 1.0f };	//左下
	vertexData_[1].position = { left, top, 0.0f, 1.0f };	//左上
	vertexData_[2].position = { right, bottom, 0.0f, 1.0f };//右下
	vertexData_[3].position = { right, top, 0.0f, 1.0f };	//左上
}

///--------------------------------------------------------------
///						 テクスチャ範囲の反映
void Sprite::ReflectTextureRange() {
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetadata(textureFilePath_);
	//テクスチャの幅と高さを取得
	float textureWidth = static_cast<float>( metadata.width );
	float textureHeight = static_cast<float>( metadata.height );

	//テクスチャの左上座標を取得
	float textureLeft = textureLeftTop_.x / textureWidth;
	float textureRight = ( textureLeftTop_.x + textureSize_.x ) / textureWidth;
	float textureTop = textureLeftTop_.y / textureHeight;	
	float textureBottom = ( textureLeftTop_.y + textureSize_.y ) / textureHeight;

	//頂点リソースにデータを書き込む
	vertexData_[0].texCoord = { textureLeft, textureBottom };
	vertexData_[1].texCoord = { textureLeft, textureTop };
	vertexData_[2].texCoord = { textureRight, textureBottom };
	vertexData_[3].texCoord = { textureRight, textureTop };
}

///--------------------------------------------------------------
///						 テクスチャサイズをイメージと統合
void Sprite::AdjustTextureSize() {
	// テクスチャのメタデータを取得
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetadata(textureFilePath_);

	// テクスチャの幅と高さを取得
	float textureWidth = static_cast<float>( metadata.width );
	float textureHeight = static_cast<float>( metadata.height );

	// スプライトのサイズをテクスチャのサイズに設定
	size_ = { textureWidth, textureHeight };

	// テクスチャの切り取り範囲をテクスチャ全体に設定
	textureLeftTop_ = { 0.0f, 0.0f };
	textureSize_ = { textureWidth, textureHeight };
}
