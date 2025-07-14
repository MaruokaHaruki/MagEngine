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
#include "AffineTransformations.h"
#include "MathFunc4x4.h"
#include "TextureManager.h"
#include <cmath>

///=============================================================================
///						初期化
void Model::Initialize(ModelSetup *modelSetup, const std::string &directorypath, const std::string &filename) {
	// modelSetupから受け取る
	modelSetup_ = modelSetup;
	// モデルデータの読み込み
	LoadModelFile(directorypath, filename);
	// 頂点バッファの作成
	CreateVertexBuffer();
	// マテリアルバッファの作成
	CreateMaterialBuffer();
	// テクスチャの読み込み
	TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);
	// テクスチャ番号を取得して、メンバ変数に格納
	textureIndex_ = TextureManager::GetInstance()->GetTextureIndex(modelData_.material.textureFilePath);
}

///=============================================================================
///
void Model::Update() {
}

///=============================================================================
///						描画
void Model::Draw() {

	if (!vertexBuffer_ || !materialBuffer_) {
		throw std::runtime_error("One or more buffers are not initialized.");
	}
	// コマンドリスト取得
	auto commandList = modelSetup_->GetDXManager()->GetCommandList();

	// VertexBufferViewの設定
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	// マテリアルバッファの設定
	commandList->SetGraphicsRootConstantBufferView(0, materialBuffer_->GetGPUVirtualAddress());

	// SRVのDescriptorTableの設定
	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData_.material.textureFilePath));

	// 環境マップテクスチャの設定
	if (modelSetup_->HasEnvironmentTexture()) {
		commandList->SetGraphicsRootDescriptorTable(7, TextureManager::GetInstance()->GetSrvHandleGPU(modelSetup_->GetEnvironmentTexture()));
	}

	// 描画(DrawCall)
	commandList->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);
}

// TODO: この関数はどこで使われているのか？
///=============================================================================
///						インスタンシング描画
void Model::InstancingDraw(uint32_t instanceCount) {
	if (!vertexBuffer_ || !materialBuffer_) {
		throw std::runtime_error("One or more buffers are not initialized.");
	}
	// コマンドリスト取得
	auto commandList = modelSetup_->GetDXManager()->GetCommandList();
	// VertexBufferViewの設定
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	// マテリアルバッファの設定
	commandList->SetGraphicsRootConstantBufferView(0, materialBuffer_->GetGPUVirtualAddress());
	// SRVのDescriptorTableの設定
	commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData_.material.textureFilePath));
	// 描画(DrawCall)
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
/// NOTE:ディレクトリパスとファイルネームの設定を忘れずに
MaterialData Model::LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename) {
	MaterialData materialData;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			materialData.textureFilePath = textureFilename;
		}
	}
	return materialData;
}

///--------------------------------------------------------------
///						 OBJファイル読み込み関数
void Model::LoadModelFile(const std::string &directoryPath, const std::string &filename) {
	//========================================
	// Assimpインポーターの作成
	Assimp::Importer importer;
	const std::string filePath = directoryPath + "/" + filename;
	const aiScene *scene = importer.ReadFile(filePath, aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	//========================================
	// 読み込みエラーのチェック
	assert(scene && scene->HasMeshes() && "Failed to load the model file.");
	//========================================
	// シーンの階層構造を構築

	//========================================
	// メッシュの取得
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh *mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals() && "Mesh does not have normals.");
		assert(mesh->HasTextureCoords(0) && "Mesh does not have texture coordinates.");
		//---------------------------------------
		// faseを解析
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace &face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3 && "Only triangular faces are supported.");
			//========================================
			// 頂点データの取得
			for (uint32_t vertexIndex = 0; vertexIndex < face.mNumIndices; ++vertexIndex) {
				uint32_t index = face.mIndices[vertexIndex];
				VertexData vertexData;
				vertexData.position.x = mesh->mVertices[index].x;
				vertexData.position.y = mesh->mVertices[index].y;
				vertexData.position.z = mesh->mVertices[index].z;
				vertexData.position.w = 1.0f; // w成分は1.0fに設定
				vertexData.texCoord.x = mesh->mTextureCoords[0][index].x;
				vertexData.texCoord.y = mesh->mTextureCoords[0][index].y;
				vertexData.normal.x = mesh->mNormals[index].x;
				vertexData.normal.y = mesh->mNormals[index].y;
				vertexData.normal.z = mesh->mNormals[index].z;
				// aiProcess_MakeLeftHandedはz* = -1で､右手系から左手系に変換するため手動で対処
				vertexData.position.z *= -1.0f;
				vertexData.normal.z *= -1.0f;
				modelData_.vertices.push_back(vertexData);
			}
		}
		//---------------------------------------
		// マテリアルデータの解析
		if (mesh->mMaterialIndex >= 0) {
			aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
			//========================================
			// マテリアルの取得
			aiString texturePath;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
				std::string textureFilePath = texturePath.C_Str();
				modelData_.material.textureFilePath = textureFilePath;
			}
		}
	}
}

///--------------------------------------------------------------
///                      Nodeの読み込み
Node Model::ReadNode(aiNode *node) {
	Node result;

	// Assimpの行列を自前の行列形式に変換
	aiMatrix4x4 aiLocalMatrix = node->mTransformation;
	// 行列変換（aiMatrixは行優先、自前のMatrix4x4は列優先と仮定）
	Matrix4x4 localMatrix;
	localMatrix.m[0][0] = aiLocalMatrix.a1;
	localMatrix.m[0][1] = aiLocalMatrix.b1;
	localMatrix.m[0][2] = aiLocalMatrix.c1;
	localMatrix.m[0][3] = aiLocalMatrix.d1;
	localMatrix.m[1][0] = aiLocalMatrix.a2;
	localMatrix.m[1][1] = aiLocalMatrix.b2;
	localMatrix.m[1][2] = aiLocalMatrix.c2;
	localMatrix.m[1][3] = aiLocalMatrix.d2;
	localMatrix.m[2][0] = aiLocalMatrix.a3;
	localMatrix.m[2][1] = aiLocalMatrix.b3;
	localMatrix.m[2][2] = aiLocalMatrix.c3;
	localMatrix.m[2][3] = aiLocalMatrix.d3;
	localMatrix.m[3][0] = aiLocalMatrix.a4;
	localMatrix.m[3][1] = aiLocalMatrix.b4;
	localMatrix.m[3][2] = aiLocalMatrix.c4;
	localMatrix.m[3][3] = aiLocalMatrix.d4;

	// ノードの情報を設定
	result.name = node->mName.C_Str();
	result.localMatrix = localMatrix;

	// 子ノードを再帰的に読み込む
	result.children.resize(node->mNumChildren);
	for (uint32_t i = 0; i < node->mNumChildren; ++i) {
		result.children[i] = ReadNode(node->mChildren[i]);
	}

	return result;
}

///--------------------------------------------------------------
///						 頂点データの作成
void Model::CreateVertexBuffer() {
	//========================================
	// 頂点リソースを作る
	vertexBuffer_ = modelSetup_->GetDXManager()->CreateBufferResource(sizeof(VertexData) * modelData_.vertices.size());
	//========================================
	// 頂点バッファビューを作成する
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();			   // リソースの先頭アドレスから使う
	vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices.size()); // 使用するリソースのサイズは頂点サイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);								   // 1頂点あたりのサイズ
	//========================================
	// 頂点リソースにデータを書き込む
	VertexData *vertexData = nullptr;
	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));
	std::memcpy(vertexData, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());
}

///--------------------------------------------------------------
///						 マテリアルデータの作成
void Model::CreateMaterialBuffer() {
	materialBuffer_ = modelSetup_->GetDXManager()->CreateBufferResource(sizeof(Material));
	// マテリアルデータ
	materialData_ = nullptr;
	// マテリアルデータ書き込み用変数
	Material material = {{1.0f, 1.0f, 1.0f, 1.0f}, true};
	// 書き込むためのアドレス取得
	materialBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&materialData_));
	// 今回は赤を書き込む
	*materialData_ = material;
	materialData_->uvTransform = Identity4x4();
	// 光沢度
	materialData_->shininess = 32.0f;
	// 環境マップの初期設定
	materialData_->enableEnvironmentMap = 0;
	materialData_->environmentMapStrength = 1.0f;
}