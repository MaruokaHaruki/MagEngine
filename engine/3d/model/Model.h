/*********************************************************************
 * \file   Model.h
 * \brief
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#pragma once
#include "ModelData.h"
#include "VertexData.h"
#include "Material.h"
 //========================================
 // DX12include
#include<d3d12.h>
#include<dxgi1_6.h>
#include <wrl/client.h>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
//========================================
// DXC
#include <dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")
//---------------------------------------
// assimpライブラリ
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class ModelSetup;
class Model {

	///--------------------------------------------------------------
	///							メンバ関数
public:

	/// \brief 初期化
	void Initialize(ModelSetup *modelSetup, const std::string &directorypath, const std::string &filename);

	/// \brief 更新
	void Update();

	/// \brief 描画 
	void Draw();

	// TODO: この関数はどこで使われているのか？
	/// @brief インスタンス描画
	void InstancingDraw(uint32_t instanceCount);

	/// \brief テクスチャの変更
	void ChangeTexture(const std::string &textureFilePath);

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	/**----------------------------------------------------------------------------
	 * \brief  LoadMaterialTemplateFile .mtlファイル読み込み
	 * \param  directoryPath ディレクトリパス
	 * \param  filename ファイルネーム
	 * \return materialData マテリアルデータ
	 * \note
	*/
	MaterialData LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename);

	/**----------------------------------------------------------------------------
	 * \brief  LoadObjFile .objファイル読み込み
	 * \param  directoryPath ディレクトリパス
	 * \param  filename ファイルネーム
	 * \note   そのままmodelDataに格納
	 */
	void LoadModelFile(const std::string &directoryPath, const std::string &filename);

	/// @brief ReadNode ノードの読み込み
	/// @param node ノード
	/// @return ノードデータ
	Node ReadNode(aiNode* node);

	/**----------------------------------------------------------------------------
	 * \brief  頂点バッファの作成
	 * \note
	 */
	void CreateVertexBuffer();

	/**----------------------------------------------------------------------------
	 * \brief  マテリアルバッファの作成
	 * \note
	 */
	void CreateMaterialBuffer();

	///--------------------------------------------------------------
	///							入出力関数
public:

	/**----------------------------------------------------------------------------
	 * \brief  SetMaterialColor マテリアルカラーの設定
	 * \param  color カラー
	 */
	void SetMaterialColor(const Vector4 &color) { materialData_->color = color; }

	/**----------------------------------------------------------------------------
	 * \brief  GetMaterialColor マテリアルカラーの取得
	 * \return 
	 */
	Vector4 GetMaterialColor() const { return materialData_->color; }

	/**----------------------------------------------------------------------------
	 * \brief  SetShininess 光沢度の設定
	 * \param  shininess
	 */
	void SetShininess(float shininess) { materialData_->shininess = shininess; }

	/**----------------------------------------------------------------------------
	 * \brief  GetShininess 光沢度の取得
	 * \return 
	 */
	float GetShininess() const { return materialData_->shininess; }

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//---------------------------------------
	// ModelSetupポインタ
	ModelSetup *modelSetup_ = nullptr;

	//---------------------------------------
	// モデルデータ
	ModelData modelData_;

	//---------------------------------------
	// 頂点データ
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	//マテリアル
	Microsoft::WRL::ComPtr <ID3D12Resource> materialBuffer_;

	///---------------------------------------
	/// バッファリソース内のデータを指すポインタ
	//頂点
	VertexData *vertexData_ = nullptr;
	//マテリアル
	Material *materialData_ = nullptr;

	///---------------------------------------
	/// バッファリソースの使い道を指すポインタ
	//頂点
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ = {};

	//---------------------------------------
	// テクスチャ用変数
	uint32_t textureIndex_ = 0;
};

