/*********************************************************************
 * \file   Model.h
 * \brief
 *
 * \author Harukichimaru
 * \date   November 2024
 * \note
 *********************************************************************/
#pragma once
#include "MagMath.h"
//========================================
// DX12include
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#pragma comment(lib, "d3d12.lib")	
#pragma comment(lib, "dxgi.lib")
//========================================
// DXC
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")
//---------------------------------------
// assimpライブラリ
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

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
	MagMath::MaterialData LoadMaterialTemplateFile(const std::string &directoryPath, const std::string &filename);

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
	MagMath::Node ReadNode(aiNode *node);

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
	void SetMaterialColor(const MagMath::Vector4 &color) {
		materialData_->color = color;
	}

	/**----------------------------------------------------------------------------
	 * \brief  GetMaterialColor マテリアルカラーの取得
	 * \return
	 */
	MagMath::Vector4 GetMaterialColor() const {
		return materialData_->color;
	}

	/**----------------------------------------------------------------------------
	 * \brief  SetShininess 光沢度の設定
	 * \param  shininess
	 */
	void SetShininess(float shininess) {
		materialData_->shininess = shininess;
	}

	/**----------------------------------------------------------------------------
	 * \brief  GetShininess 光沢度の取得
	 * \return
	 */
	float GetShininess() const {
		return materialData_->shininess;
	}

	/**----------------------------------------------------------------------------
	 * \brief  SetEnvironmentMapEnabled 環境マップの有効/無効設定
	 * \param  enabled 有効フラグ
	 */
	void SetEnvironmentMapEnabled(bool enabled) {
		materialData_->enableEnvironmentMap = enabled ? 1 : 0;
	}

	/**----------------------------------------------------------------------------
	 * \brief  GetEnvironmentMapEnabled 環境マップの有効/無効取得
	 * \return
	 */
	bool GetEnvironmentMapEnabled() const {
		return materialData_->enableEnvironmentMap != 0;
	}

	/**----------------------------------------------------------------------------
	 * \brief  SetEnvironmentMapStrength 環境マップの強度設定
	 * \param  strength 強度
	 */
	void SetEnvironmentMapStrength(float strength) {
		materialData_->environmentMapStrength = strength;
	}

	/**----------------------------------------------------------------------------
	 * \brief  GetEnvironmentMapStrength 環境マップの強度取得
	 * \return
	 */
	float GetEnvironmentMapStrength() const {
		return materialData_->environmentMapStrength;
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//---------------------------------------
	// ModelSetupポインタ
	ModelSetup *modelSetup_ = nullptr;

	//---------------------------------------
	// モデルデータ
	MagMath::ModelData modelData_;

	//---------------------------------------
	// 頂点データ
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	// マテリアル
	Microsoft::WRL::ComPtr<ID3D12Resource> materialBuffer_;

	///---------------------------------------
	/// バッファリソース内のデータを指すポインタ
	// 頂点
	MagMath::VertexData *vertexData_ = nullptr;
	// マテリアル
	MagMath::Material *materialData_ = nullptr;

	///---------------------------------------
	/// バッファリソースの使い道を指すポインタ
	// 頂点
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ = {};

	//---------------------------------------
	// テクスチャ用変数
	uint32_t textureIndex_ = 0;
};
