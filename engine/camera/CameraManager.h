/*********************************************************************
 * \file   CameraManager.h
 * \brief  
 * 
 * \author Harukichimaru
 * \date   January 2025
 * \note   
 *********************************************************************/
#pragma once
#include <memory>
#include <map>
#include <string>
#include "Camera.h"

///=============================================================================
///						カメラマネージャークラス
class CameraManager {
    ///--------------------------------------------------------------
	///						 メンバ関数
public:
	/**----------------------------------------------------------------------------
	 * \brief  GetInstance インスタンスの取得
	 * \return 
	 */
    static CameraManager* GetInstance();
    /**----------------------------------------------------------------------------
	 * \brief  ~CameraManager デストラクタ
     */
    ~CameraManager();
	/**----------------------------------------------------------------------------
	 * \brief  Finalize 終了処理
	 */
    void Finalize();
	/**----------------------------------------------------------------------------
	 * \brief  Initialize 初期化
	 */
    void Initialize();
	/**----------------------------------------------------------------------------
	 * \brief  AddCamera カメラの追加
	 * \param  name カメラ名
	 */
    void AddCamera(const std::string& name);
	/**----------------------------------------------------------------------------
	 * \brief  GetCamera カメラの取得
	 * \param  name カメラ名
	 * \return 
	 */
    Camera* GetCamera(const std::string& name) const;

	/// \brief 現在のカメラの設定
    void SetCurrentCamera(const std::string& name);

	/// \brief 現在のカメラの取得
    Camera* GetCurrentCamera() const;

	/// \brief 全てのカメラの更新
    void UpdateAll();

	/// @brief デバックカメラの更新
    void DebugCameraUpdate();

    /// @brief デバックカメラの呼び出し
	void ChangeDebugCamera();

	/// @brief ImGuiの描画
	void DrawImGui();

    ///--------------------------------------------------------------
	///						 メンバ変数
private:
    //========================================
	//　コンストラクタ
    CameraManager() = default;
    //========================================
	// カメラのコンテナ
    std::map<std::string, std::unique_ptr<Camera>> cameras_;
	//========================================
	//現在のカメラ名
    std::string currentCameraName_;
	//過去のカメラ名
	std::string previousCameraName_;
};
