/*********************************************************************
 * \file   Model.cpp
 * \brief  モデルクラス
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#include "Model.h"
#include "ModelSetup.h"
//---------------------------------------
// ファイル読み込み関数
#include <fstream>
#include <sstream>
//---------------------------------------
// 数学関数　
#include <cmath>
#include "MathFunc4x4.h"
#include "AffineTransformations.h"
#include "TextureManager.h"


///=============================================================================
///						初期化
void Model::Initialize(ModelSetup *modelSetup, const std::string &directorypath, const std::string &filename) {
	//modelSetupから受け取る
	modelSetup_ = modelSetup;
	//モデルデータの読み込み
	LoadObjFile(directorypath, filename);
	//頂点バッファの作成
	CreateVertexBuffer();
	//マテリアルバッファの作成
	CreateMaterialBuffer();
	//テクスチャの読み込み
	TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);
	//テクスチャ番号を取得して、メンバ変数に格納
	textureIndex_ = TextureManager::GetInstance()->GetTextureIndex(modelData_.material.textureFilePath);

}

///=============================================================================
///						
void Model::Update() {
}

///=============================================================================
///						描画
void Model::Draw() {

	if(!vertexBuffer_ || !materialBuffer_) {
		throw std::runtime_error("One or more buffers are not initialized.");
	}
	// コマンドリスト取得
	auto commandList = modelSetup_->GetDXManager()->GetCommandList();

	//VertexBufferViewの設定
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	//マテリアルバッファの設定
	commandList->SetGraphicsRootConstantBufferView(0, materialBuffer_->GetGPUVirtualAddress());

	//SRVのDescriptorTableの設定
	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData_.material.textureFilePath));

	//描画(DrawCall)
	commandList->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);
}

// TODO: この関数はどこで使われているのか？
///=============================================================================
///						インスタンシング描画
void Model::InstancingDraw(uint32_t instanceCount) {
	if(!vertexBuffer_ || !materialBuffer_) {
		throw std::runtime_error("One or more buffers are not initialized.");
	}
	// コマンドリスト取得
	auto commandList = modelSetup_->GetDXManager()->GetCommandList();
	//VertexBufferViewの設定
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	//マテリアルバッファの設定
	commandList->SetGraphicsRootConstantBufferView(0, materialBuffer_->GetGPUVirtualAddress());
	//SRVのDescriptorTableの設定
	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData_.material.textureFilePath));
	//描画(DrawCall)
	commandList->DrawInstanced(6, instanceCount, 0, 0);
}

///=============================================================================
///						テクスチャの変更
void Model::ChangeTexture(const std::string &textureFilePath) {
	// 新しいテクスチャを読み込む
	TextureManager::GetInstance()->LoadTexture(textureFilePath);

	// 新しいテクスチャのインデックスを取得して設定
	textureIndex_ = TextureManager::GetInstance()->GetTextureIndex(textureFilePath);

	// マテリアルデータを更新
	modelData_.material.textureFilePath = textureFilePath;
}

///=============================================================================
///						ローカルメンバ関数
///--------------------------------------------------------------
///						 ファイル読み込み関数
///NOTE:ディレクトリパスとファイルネームの設定を忘れずに
MaterialData Model::LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename) {
	MaterialData materialData;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);

	while(std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if(identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			materialData.textureFilePath = textureFilename;
		}
	}
	return materialData;
}


///--------------------------------------------------------------
///						 OBJファイル読み込み関数
void Model::LoadObjFile(const std::string &directoryPath, const std::string &filename) {
	//========================================
	// 1.中で必要となる変数の宣言
	ModelData modelData;            //構築するModelData
	std::vector<Vector4> positions; //位置
	std::vector<Vector3> normals;   //法線
	std::vector<Vector2> texcoords; //テクスチャ座標
	std::string line;               //ファイルから読んだ1行を格納するもの

	//========================================
	// 2.ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	//========================================
	// 3.実際にファイルを読み、ModelDataを構築していく
	while(std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier; //先頭の識別子を読む

		//---------------------------------------
		// identifierに応じた処理
		if(identifier == "v") {
			//初期化
			Vector4 position = { 0.0f, 0.0f, 0.0f, 1.0f };
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f; //NOTE:同次座標を送っているためw=1
			position.x *= -1.0f; //反転
			positions.push_back(position);

		} else if(identifier == "vt") {
			Vector2 texcoord = { 0.0f, 0.0f };
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y; // y軸を反転
			texcoords.push_back(texcoord);

		} else if(identifier == "vn") {
			Vector3 normal = { 0.0f, 0.0f, 0.0f };
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f; //反転
			normals.push_back(normal);

		} else if(identifier == "f") {
			VertexData triangle[3] = {};
			//面は三角形限定。その他は未対応
			for(int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				//頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3] = {};
				for(int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline(v, index, '/'); // /区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}
				//要素へのIndexから、実際の値を取得して、頂点を構築する
				// NOTE:1始まりなので添字として利用するときは-1を忘れに
				// NOTE:static_cast<std::vector<Vector4, std::allocator<Vector4>>::size_type>はstd::vectorのサイズを取得する型変換
				Vector4 position = positions[static_cast<std::vector<Vector4, std::allocator<Vector4>>::size_type>( elementIndices[0] ) - 1];
				Vector2 texcoord = texcoords[static_cast<std::vector<Vector2, std::allocator<Vector2>>::size_type>( elementIndices[1] ) - 1];
				Vector3 normal = normals[static_cast<std::vector<Vector3, std::allocator<Vector3>>::size_type>( elementIndices[2] ) - 1];
				triangle[faceVertex] = { position, texcoord, normal };
			}
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);

		} else if(identifier == "mtllib") {
			//MaterialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			// NOTE:基本的にobjファイルと同１改装にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	//========================================
	// 4.ModelDataを返す
	modelData_ = modelData;
}

///--------------------------------------------------------------
///						 頂点データの作成
void Model::CreateVertexBuffer() {
	//========================================
	// 頂点リソースを作る
	vertexBuffer_ = modelSetup_->GetDXManager()->CreateBufferResource(sizeof(VertexData) * modelData_.vertices.size());
	//========================================
	// 頂点バッファビューを作成する
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();				//リソースの先頭アドレスから使う
	vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size());	//使用するリソースのサイズは頂点サイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);									//1頂点あたりのサイズ
	//========================================
	// 頂点リソースにデータを書き込む
	VertexData *vertexData = nullptr;
	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void **>( &vertexData ));
	std::memcpy(vertexData, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());
}

///--------------------------------------------------------------
///						 マテリアルデータの作成
void Model::CreateMaterialBuffer() {
	materialBuffer_ = modelSetup_->GetDXManager()->CreateBufferResource(sizeof(Material));
	//マテリアルデータ
	materialData_ = nullptr;
	//マテリアルデータ書き込み用変数
	Material material = { {1.0f, 1.0f, 1.0f, 1.0f},true };
	//書き込むためのアドレス取得
	materialBuffer_->Map(0, nullptr, reinterpret_cast<void **>( &materialData_ ));
	//今回は赤を書き込む
	*materialData_ = material;
	materialData_->uvTransform = Identity4x4();
	//光沢度
	materialData_->shininess = 32.0f;
}